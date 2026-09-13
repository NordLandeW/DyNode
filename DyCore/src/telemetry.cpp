#include "telemetry.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <process.h>
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <json.hpp>
#include <stdexcept>
#include <system_error>
#include <utility>

#include "analytics.h"
#include "api.h"
#include "aptabase.h"

namespace telemetry {
struct ExitTasks::State {
    HANDLE done = nullptr;
    HANDLE timer = nullptr;
    std::atomic<unsigned> completed = 0;
    std::array<std::atomic<ExitTaskStatus>, 2> status{ExitTaskStatus::pending,
                                                      ExitTaskStatus::pending};
    Clock::time_point deadline;

    explicit State(Clock::time_point until) : deadline(until) {
        done = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!done)
            throw std::system_error(GetLastError(), std::system_category());
        timer = CreateWaitableTimerExW(nullptr, nullptr,
                                       CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                       TIMER_ALL_ACCESS);
        if (!timer) {
            const DWORD error = GetLastError();
            CloseHandle(done);
            throw std::system_error(error, std::system_category());
        }
    }
    ~State() {
        CloseHandle(timer);
        CloseHandle(done);
    }

    void complete(size_t index, ExitTaskStatus result) {
        status[index].store(result, std::memory_order_release);
        if (completed.fetch_add(1) == 1)
            SetEvent(done);
    }

    struct Invocation {
        std::shared_ptr<State> owner;
        std::function<void()> task;
        size_t index;
    };

    static unsigned __stdcall execute(void* context) {
        std::unique_ptr<Invocation> invocation(
            static_cast<Invocation*>(context));
        auto result = ExitTaskStatus::succeeded;
        try {
            invocation->task();
        } catch (...) {
            result = ExitTaskStatus::failed;
        }
        invocation->owner->complete(invocation->index, result);
        return 0;
    }

    static void launch(const std::shared_ptr<State>& owner, size_t index,
                       std::function<void()> task) {
        if (!task) {
            owner->complete(index, ExitTaskStatus::succeeded);
            return;
        }
        try {
            auto invocation = std::make_unique<Invocation>(
                Invocation{owner, std::move(task), index});
            const uintptr_t thread = _beginthreadex(
                nullptr, 0, execute, invocation.get(), 0, nullptr);
            if (!thread) {
                owner->complete(index, ExitTaskStatus::failed);
                return;
            }
            invocation.release();
            // The invocation owns its state. Closing this kernel handle does
            // not stop the thread and never waits for it.
            CloseHandle(reinterpret_cast<HANDLE>(thread));
        } catch (...) {
            owner->complete(index, ExitTaskStatus::failed);
        }
    }
};

ExitTasks::ExitTasks(std::function<void()> aptabase,
                     std::function<void()> sentry, Clock::time_point deadline)
    : state(std::make_shared<State>(deadline)) {
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_PIN,
                            reinterpret_cast<LPCWSTR>(&State::execute),
                            &module)) {
        throw std::system_error(GetLastError(), std::system_category());
    }
    State::launch(state, 0, std::move(aptabase));
    State::launch(state, 1, std::move(sentry));
}

std::array<ExitTaskStatus, 2> ExitTasks::wait() const {
    const auto remaining =
        std::chrono::duration_cast<std::chrono::milliseconds>(state->deadline -
                                                              Clock::now());
    // A normal wait timeout can round up to the system tick. A private
    // high-resolution timer avoids that extra delay without changing global
    // timer resolution. Round down to leave the sub-ms tail for returning.
    const DWORD timeout = static_cast<DWORD>(std::clamp<int64_t>(
        remaining.count(), 0, static_cast<int64_t>(INFINITE) - 1));
    if (timeout > 0) {
        LARGE_INTEGER due{};
        due.QuadPart = -static_cast<LONGLONG>(timeout) * 10000;
        if (!SetWaitableTimerEx(state->timer, &due, 0, nullptr, nullptr,
                                nullptr, 0)) {
            throw std::system_error(GetLastError(), std::system_category());
        }
        const HANDLE events[] = {state->done, state->timer};
        if (WaitForMultipleObjects(2, events, FALSE, timeout) == WAIT_FAILED) {
            throw std::system_error(GetLastError(), std::system_category());
        }
    }
    // There is deliberately no SDK close, cancellation wait, or worker join.
    return {state->status[0].load(std::memory_order_acquire),
            state->status[1].load(std::memory_order_acquire)};
}
}  // namespace telemetry

namespace {
struct UploadResult {
    std::atomic<size_t> accepted = 0;
    std::atomic<int> status = 0;
    std::atomic<int> sentryPending = 0;
};

void upload_exit_events(const std::string& endpoint, const std::string& appKey,
                        const std::string& events, int batchSize,
                        telemetry::Clock::time_point deadline,
                        UploadResult& result) {
    const auto queue = nlohmann::json::parse(events);
    if (!queue.is_array())
        throw std::invalid_argument("Aptabase exit events must be an array");
    if (batchSize <= 0)
        return;
    batchSize = std::min(batchSize, 25);
    for (size_t offset = 0; offset < queue.size();) {
        const size_t count = std::min<size_t>(batchSize, queue.size() - offset);
        const nlohmann::json batch(queue.begin() + offset,
                                   queue.begin() + offset + count);
        const auto payload = batch.dump();
        const auto remaining =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - telemetry::Clock::now());
        if (remaining.count() < 1)
            break;
        const int status = post_aptabase_events(
            endpoint, appKey, payload, static_cast<int>(remaining.count()));
        result.status = status;
        if (status < 200 || status >= 300)
            break;
        offset += count;
        result.accepted = offset;
    }
}
}  // namespace

std::string shutdown_telemetry(const std::string& endpoint,
                               const std::string& appKey,
                               const std::string& events, int batchSize) {
    using telemetry::ExitTaskStatus;
    const auto deadline = telemetry::Clock::now() + telemetry::EXIT_BUDGET;
    auto result = std::make_shared<UploadResult>();
    auto closeSentry = take_analytics_shutdown();
    std::function<void()> sentry;
    if (closeSentry) {
        sentry = [result, close = std::move(closeSentry)] {
            result->sentryPending = close();
        };
    }
    telemetry::ExitTasks tasks(
        [result, endpoint, appKey, events, batchSize, deadline] {
            upload_exit_events(endpoint, appKey, events, batchSize, deadline,
                               *result);
        },
        std::move(sentry), deadline);
    const auto status = tasks.wait();
    return nlohmann::json{
        {"accepted", result->accepted.load()},
        {"status", result->status.load()},
        {"aptabaseFinished", status[0] != ExitTaskStatus::pending},
        {"sentryFinished", status[1] != ExitTaskStatus::pending},
        {"aptabaseFailed", status[0] == ExitTaskStatus::failed},
        {"sentryFailed", status[1] == ExitTaskStatus::failed},
        {"sentryPending", result->sentryPending.load()},
        {"timedOut", status[0] == ExitTaskStatus::pending ||
                         status[1] == ExitTaskStatus::pending}}
        .dump();
}

DYCORE_API const char* DyCore_shutdown_telemetry(const char* endpoint,
                                                 const char* appKey,
                                                 const char* events,
                                                 double batchSize) {
    static bool started = false;
    static std::string result;
    if (started)
        return result.c_str();
    started = true;
    try {
        const int count = std::isfinite(batchSize) && batchSize >= 1
                              ? static_cast<int>(std::min(batchSize, 25.0))
                              : 0;
        result =
            shutdown_telemetry(endpoint ? endpoint : "", appKey ? appKey : "",
                               events ? events : "[]", count);
    } catch (...) {
        // Failure to start safe independent work is not permission to fall
        // back to synchronous network/SDK cleanup on the owner thread.
        result =
            R"({"accepted":0,"status":0,"aptabaseFinished":false,"sentryFinished":false,"aptabaseFailed":true,"sentryFailed":true,"sentryPending":0,"timedOut":false})";
    }
    return result.c_str();
}
