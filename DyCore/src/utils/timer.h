#pragma once

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

#include "utils.h"

class Timer {
   public:
    explicit Timer(std::string title)
        : title_(std::move(title)), start_(Clock::now()) {
    }

    ~Timer() {
        const auto end = Clock::now();
        const auto elapsedMs =
            std::chrono::duration<double, std::milli>(end - start_).count();

        std::ostringstream message;
        message << title_ << " took " << std::fixed << std::setprecision(3)
                << elapsedMs << " ms.";
        print_debug_message(message.str());
    }

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

   private:
    using Clock = std::chrono::steady_clock;

    std::string title_;
    Clock::time_point start_;
};

#define ___TIMER_CONCAT_INNER(a, b) a##b
#define ___TIMER_CONCAT(a, b) ___TIMER_CONCAT_INNER(a, b)
#define TIMER_SCOPE(title) Timer ___TIMER_CONCAT(_scopeTimer_, __LINE__)(title)
