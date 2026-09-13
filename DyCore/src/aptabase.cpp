#include "aptabase.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

#include "api.h"
#include "utils.h"

namespace {
using HttpHandle = std::unique_ptr<void, decltype(&WinHttpCloseHandle)>;

int http_error() {
    return -static_cast<int>(GetLastError());
}
struct HttpTarget {
    std::wstring host;
    std::wstring path;
    INTERNET_PORT port = 0;
    DWORD flags = 0;
};

int parse_http_target(const std::string& endpoint, HttpTarget& target) {
    const std::wstring url = s2ws(endpoint);
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts))
        return http_error();
    if (parts.nScheme != INTERNET_SCHEME_HTTP &&
        parts.nScheme != INTERNET_SCHEME_HTTPS) {
        return -ERROR_WINHTTP_UNRECOGNIZED_SCHEME;
    }
    target.host.assign(parts.lpszHostName, parts.dwHostNameLength);
    target.port = parts.nPort;
    target.flags =
        parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    auto& path = target.path;
    if (parts.dwUrlPathLength)
        path.assign(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (path.empty())
        path = L"/";
    if (parts.dwExtraInfoLength)
        path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);

    return 0;
}

int send_aptabase_request(HINTERNET request, const std::string& appKey,
                          const std::string& payload) {
    DWORD disabled = WINHTTP_DISABLE_REDIRECTS |
                     WINHTTP_DISABLE_AUTHENTICATION | WINHTTP_DISABLE_COOKIES;
    DWORD attempts = 1;
    if (!WinHttpSetOption(request, WINHTTP_OPTION_CONNECT_RETRIES, &attempts,
                          sizeof(attempts)))
        return http_error();
    if (!WinHttpSetOption(request, WINHTTP_OPTION_DISABLE_FEATURE, &disabled,
                          sizeof(disabled)))
        return http_error();
    const std::wstring headers =
        L"Content-Type: application/json\r\nApp-Key: " + s2ws(appKey) + L"\r\n";
    const DWORD bytes = static_cast<DWORD>(payload.size());
    if (!WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(-1),
                            const_cast<char*>(payload.data()), bytes, bytes,
                            0) ||
        !WinHttpReceiveResponse(request, nullptr)) {
        return http_error();
    }
    DWORD status = 0;
    DWORD size = sizeof(status);
    if (!WinHttpQueryHeaders(
            request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &size,
            WINHTTP_NO_HEADER_INDEX)) {
        return http_error();
    }
    return static_cast<int>(status);
}
}  // namespace

int post_aptabase_events(const std::string& endpoint, const std::string& appKey,
                         const std::string& payload, int timeoutMs,
                         bool useSystemProxy) {
    if (timeoutMs <= 0 || appKey.empty() ||
        appKey.find_first_of("\r\n") != std::string::npos ||
        payload.size() > (std::numeric_limits<DWORD>::max)()) {
        return -ERROR_INVALID_PARAMETER;
    }
    HttpTarget target;
    const int parseResult = parse_http_target(endpoint, target);
    if (parseResult != 0)
        return parseResult;

    HttpHandle session(
        WinHttpOpen(L"DyNode/Aptabase",
                    useSystemProxy ? WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
                                   : WINHTTP_ACCESS_TYPE_NO_PROXY,
                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0),
        &WinHttpCloseHandle);
    if (!session)
        return http_error();
    const int phaseTimeout = std::max(1, timeoutMs / 4);
    if (!WinHttpSetTimeouts(session.get(), phaseTimeout, phaseTimeout,
                            phaseTimeout, phaseTimeout))
        return http_error();
    HttpHandle connection(
        WinHttpConnect(session.get(), target.host.c_str(), target.port, 0),
        &WinHttpCloseHandle);
    if (!connection)
        return http_error();
    HttpHandle request(
        WinHttpOpenRequest(connection.get(), L"POST", target.path.c_str(),
                           nullptr, WINHTTP_NO_REFERER,
                           WINHTTP_DEFAULT_ACCEPT_TYPES, target.flags),
        &WinHttpCloseHandle);
    if (!request)
        return http_error();
    return send_aptabase_request(request.get(), appKey, payload);
}

DYCORE_API double DyCore_aptabase_post(const char* endpoint, const char* appKey,
                                       const char* payload, double timeoutMs) {
    if (!endpoint || !appKey || !payload || !std::isfinite(timeoutMs) ||
        timeoutMs < 1 || timeoutMs > (std::numeric_limits<int>::max)()) {
        return -ERROR_INVALID_PARAMETER;
    }
    try {
        return post_aptabase_events(endpoint, appKey, payload,
                                    static_cast<int>(timeoutMs));
    } catch (const std::exception& error) {
        print_debug_message(std::string("Aptabase exit POST failed: ") +
                            error.what());
        return -ERROR_INVALID_DATA;
    }
}
