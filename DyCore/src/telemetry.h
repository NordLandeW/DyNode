#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace telemetry {
using Clock = std::chrono::steady_clock;
inline constexpr auto EXIT_BUDGET = std::chrono::milliseconds(5000);

enum class ExitTaskStatus { pending, succeeded, failed };

// Process-exit only: tasks own all input and may outlive this object. The
// implementation module stays mapped until process termination. Never capture
// GML memory, project objects, stack references, or runtime-reloadable modules.
class ExitTasks {
   public:
    ExitTasks(std::function<void()> aptabase, std::function<void()> sentry,
              Clock::time_point deadline);
    std::array<ExitTaskStatus, 2> wait() const;

   private:
    struct State;
    std::shared_ptr<State> state;
};
}  // namespace telemetry

// Owner-thread, process-exit entry. Both uploads and SDK teardown share one
// deadline; unfinished jobs never add a destructor/join wait on this thread.
std::string shutdown_telemetry(const std::string& endpoint,
                               const std::string& appKey,
                               const std::string& events, int batchSize);
