#include <doctest/doctest.h>

#ifdef _WIN32
#include <winsock2.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <future>
#include <sstream>
#include <stdexcept>
#include <string>

#include "aptabase.h"

namespace {
std::string header_value(const std::string& request, const std::string& name) {
    std::istringstream headers(request.substr(0, request.find("\r\n\r\n")));
    std::string line;
    while (std::getline(headers, line)) {
        const auto colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string key = line.substr(0, colon);
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char value) {
                           return static_cast<char>(std::tolower(value));
                       });
        if (key != name)
            continue;
        const auto begin = line.find_first_not_of(" \t", colon + 1);
        if (begin == std::string::npos)
            return {};
        const auto end = line.find_last_not_of(" \t\r");
        return line.substr(begin, end - begin + 1);
    }
    return {};
}

struct SocketRuntime {
    SocketRuntime() {
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
            throw std::runtime_error("WSAStartup failed");
    }
    ~SocketRuntime() {
        WSACleanup();
    }
};
struct Socket {
    SOCKET value = INVALID_SOCKET;
    ~Socket() {
        close();
    }
    void close() {
        if (value != INVALID_SOCKET) {
            shutdown(value, SD_BOTH);
            closesocket(value);
            value = INVALID_SOCKET;
        }
    }
};

class OneRequestServer {
    SocketRuntime runtime;
    Socket listener;
    std::future<std::string> worker;

   public:
    unsigned short port = 0;

    OneRequestServer(int status, size_t expectedBodyBytes) {
        listener.value = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listener.value == INVALID_SOCKET)
            throw std::runtime_error("socket failed");
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (bind(listener.value, reinterpret_cast<sockaddr*>(&address),
                 sizeof(address)) != 0 ||
            listen(listener.value, 1) != 0)
            throw std::runtime_error("listen failed");
        int addressSize = sizeof(address);
        if (getsockname(listener.value, reinterpret_cast<sockaddr*>(&address),
                        &addressSize) != 0) {
            throw std::runtime_error("getsockname failed");
        }
        port = ntohs(address.sin_port);
        const SOCKET listening = listener.value;
        worker = std::async(
            std::launch::async, [listening, status, expectedBodyBytes] {
                Socket client{accept(listening, nullptr, nullptr)};
                if (client.value == INVALID_SOCKET)
                    throw std::runtime_error("accept failed");
                // Isolation only: synchronization comes from listen/accept and
                // a complete request, never from sleeps or elapsed-time
                // assertions.
                DWORD timeout = 30000;
                setsockopt(client.value, SOL_SOCKET, SO_RCVTIMEO,
                           reinterpret_cast<const char*>(&timeout),
                           sizeof(timeout));
                std::string received;
                std::array<char, 4096> bytes{};
                for (;;) {
                    const int count = recv(client.value, bytes.data(),
                                           static_cast<int>(bytes.size()), 0);
                    if (count <= 0)
                        throw std::runtime_error("incomplete request");
                    received.append(bytes.data(), count);
                    const auto headerEnd = received.find("\r\n\r\n");
                    if (headerEnd != std::string::npos &&
                        received.size() >= headerEnd + 4 + expectedBodyBytes)
                        break;
                }
                // Deliberately omit the advertised body: only the response
                // status is relevant to acknowledgment, not a complete entity
                // download.
                const std::string response =
                    "HTTP/1.1 " + std::to_string(status) +
                    " Result\r\nContent-Length: 100000\r\nLocation: "
                    "/redirected\r\nConnection: close\r\n\r\n";
                size_t offset = 0;
                while (offset < response.size()) {
                    const int count =
                        send(client.value, response.data() + offset,
                             static_cast<int>(response.size() - offset), 0);
                    if (count <= 0)
                        throw std::runtime_error("send response failed");
                    offset += static_cast<size_t>(count);
                }
                return received;
            });
    }
    ~OneRequestServer() {
        listener.close();
        if (worker.valid())
            worker.wait();
    }
    std::string request() {
        return worker.get();
    }
};
}  // namespace

TEST_CASE("AptabaseExitPostPreservesPayloadAndReturnsTheActualHttpStatus") {
    const std::string payload = "[{\"eventName\":\"close-\xE7\x8C\xAB\"}]";
    for (const int status : {202, 302, 429}) {
        OneRequestServer server(status, payload.size());
        const auto endpoint =
            "http://127.0.0.1:" + std::to_string(server.port) +
            "/api/v0/events?test=1";
        REQUIRE(post_aptabase_events(endpoint, "A-SH-test", payload, 60000,
                                     false) == status);
        const auto request = server.request();
        CHECK(request.starts_with("POST /api/v0/events?test=1 "));
        CHECK(header_value(request, "app-key") == "A-SH-test");
        CHECK(header_value(request, "content-type") == "application/json");
        const auto bodyAt = request.find("\r\n\r\n");
        REQUIRE(bodyAt != std::string::npos);
        CHECK(request.substr(bodyAt + 4) == payload);
    }
}

TEST_CASE("AptabaseExitPostRejectsInvalidArgumentsBeforeNetworking") {
    CHECK(post_aptabase_events("invalid", "test", "[]", 0, false) < 0);
    CHECK(post_aptabase_events("invalid", "bad\r\nheader", "[]", 1000, false) <
          0);
    CHECK(post_aptabase_events("not a URL", "test", "[]", 1000, false) < 0);
}
#endif
