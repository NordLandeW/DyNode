#include <doctest/doctest.h>

#include <atomic>
#include <future>
#include <latch>
#include <memory>
#include <stdexcept>
#include <string>

#include "telemetry.h"

namespace {
using telemetry::Clock;
using telemetry::ExitTasks;
using telemetry::ExitTaskStatus;

struct BlockedUploads {
    std::latch entered{2};
    std::latch release{1};
    std::latch returned{2};
    std::atomic<int> calls = 0;

    void upload() {
        ++calls;
        entered.count_down();
        release.wait();
        returned.count_down();
    }
};
}  // namespace

TEST_CASE("TelemetryExitStartsBothJobsWithoutWaitingForEither") {
    auto uploads = std::make_shared<BlockedUploads>();
    {
        ExitTasks tasks([uploads] { uploads->upload(); },
                        [uploads] { uploads->upload(); }, Clock::now());
        // If launch became serial, neither the second entry nor this latch
        // could complete. No sleeps or elapsed-time assertions are needed.
        uploads->entered.wait();
        const auto status = tasks.wait();
        CHECK(status[0] == ExitTaskStatus::pending);
        CHECK(status[1] == ExitTaskStatus::pending);
        CHECK(uploads->calls == 2);
        CHECK(tasks.wait() == status);
    }
    // Destroying the caller after expiry must not join either blocked job.
    uploads->release.count_down();
    uploads->returned.wait();
}

TEST_CASE("TelemetryExitKeepsTaskInputAliveAfterTheCallerReturns") {
    struct Input {
        std::string payload = "owned-event";
        std::shared_ptr<std::promise<void>> destroyed;
        ~Input() {
            destroyed->set_value();
        }
    };
    auto destroyed = std::make_shared<std::promise<void>>();
    auto gone = destroyed->get_future();
    auto input = std::make_shared<Input>();
    input->destroyed = destroyed;
    std::weak_ptr<Input> lifetime = input;
    auto entered = std::make_shared<std::promise<void>>();
    auto ready = entered->get_future();
    auto release = std::make_shared<std::latch>(1);
    auto observed = std::make_shared<std::promise<std::string>>();
    auto payload = observed->get_future();
    {
        ExitTasks tasks(
            [input, entered, release, observed] {
                entered->set_value();
                release->wait();
                observed->set_value(input->payload);
            },
            {}, Clock::now());
        ready.get();
        input.reset();
        CHECK(tasks.wait()[0] == ExitTaskStatus::pending);
    }
    CHECK_FALSE(lifetime.expired());
    release->count_down();
    CHECK(payload.get() == "owned-event");
    gone.get();
    CHECK(lifetime.expired());
}

TEST_CASE("TelemetryExitReportsCompletionAndIsolatesWorkerExceptions") {
    auto completed = std::make_shared<std::atomic<int>>(0);
    // This deadline only isolates a stuck test; correctness is determined by
    // task completion/status, not how fast this device runs the callbacks.
    ExitTasks tasks([] { throw std::runtime_error("upload failed"); },
                    [completed] { ++*completed; },
                    Clock::now() + std::chrono::seconds(30));
    const auto status = tasks.wait();
    CHECK(status[0] == ExitTaskStatus::failed);
    CHECK(status[1] == ExitTaskStatus::succeeded);
    CHECK(*completed == 1);
    CHECK(tasks.wait() == status);
}

TEST_CASE("TelemetryExitWithNoWorkCompletesWithoutConsumingItsBudget") {
    ExitTasks tasks({}, {}, Clock::now() + std::chrono::seconds(30));
    const auto status = tasks.wait();
    CHECK(status[0] == ExitTaskStatus::succeeded);
    CHECK(status[1] == ExitTaskStatus::succeeded);
}
