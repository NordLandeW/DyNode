#pragma once

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct ProfileData {
    std::string name;
    double total_duration = 0.0;
    long long call_count = 0;
    double min_duration = std::numeric_limits<double>::max();
    double max_duration = 0.0;
    double last_duration = 0.0;
    std::vector<double> all_durations;

    void record(double duration) {
        total_duration += duration;
        call_count++;
        if (duration < min_duration)
            min_duration = duration;
        if (duration > max_duration)
            max_duration = duration;
        last_duration = duration;
        all_durations.push_back(duration);
    }

    double average_duration() const {
        return call_count > 0 ? total_duration / call_count : 0.0;
    }
};

class Profiler;

class ScopedTimer {
   private:
    Profiler& profiler;
    std::string name;
    std::chrono::high_resolution_clock::time_point start_time;
    bool enabled;

   public:
    ScopedTimer(const std::string& name, Profiler& profiler_instance,
                bool is_enabled = true);
    ~ScopedTimer();
};

class Profiler {
   private:
    std::unordered_map<std::string, ProfileData> records;
    mutable std::mutex mtx;

    Profiler() = default;

   public:
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

    static Profiler& get() {
        static Profiler instance;
        return instance;
    }

    void record(const std::string& name, double duration) {
        std::lock_guard<std::mutex> lock(mtx);
        records[name].name = name;
        records[name].record(duration);
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mtx);
        records.clear();
    }

    std::optional<double> get_last_duration_ms(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = records.find(name);
        if (it == records.end()) {
            return std::nullopt;
        }
        return it->second.last_duration * 1000.0;
    }

    void generate_report(std::ostream& os = std::cout,
                         double lag_threshold_multiplier = 100.0) const {
        std::lock_guard<std::mutex> lock(mtx);
        os << "----------------------------------------------------------------"
              "----------------------------------------------------------------"
              "------------------------------------------\n";
        os << "Profiler Report\n";
        os << "----------------------------------------------------------------"
              "----------------------------------------------------------------"
              "------------------------------------------\n";
        os << std::left << std::setw(50) << "Name" << std::setw(15)
           << "Total (ms)" << std::setw(10) << "Calls" << std::setw(15)
           << "Avg (ms)" << std::setw(15) << "Min (ms)" << std::setw(15)
           << "Max (ms)" << std::setw(15) << "Last (ms)" << std::setw(15)
           << "P1 (ms)" << std::setw(15) << "P99 (ms)" << std::setw(12)
           << "Lag Count" << std::setw(15) << "Max Lag (ms)"
           << "\n";
        os << "----------------------------------------------------------------"
              "----------------------------------------------------------------"
              "------------------------------------------\n";

        std::vector<ProfileData> sorted_records;
        sorted_records.reserve(records.size());
        for (const auto& pair : records) {
            sorted_records.push_back(pair.second);
        }

        std::sort(sorted_records.begin(), sorted_records.end(),
                  [](const auto& a, const auto& b) {
                      return a.average_duration() > b.average_duration();
                  });

        for (auto& data : sorted_records) {
            double p1 = 0.0, p99 = 0.0;
            int lag_count = 0;
            double max_lag_duration = 0.0;

            if (data.call_count > 1) {
                std::sort(data.all_durations.begin(), data.all_durations.end());

                size_t p1_index = static_cast<size_t>(data.call_count * 0.01);
                size_t p99_index = static_cast<size_t>(data.call_count * 0.99);
                if (p99_index >= data.all_durations.size())
                    p99_index = data.all_durations.size() - 1;

                p1 = data.all_durations[p1_index];
                p99 = data.all_durations[p99_index];

                double avg = data.average_duration();
                if (avg > 0) {
                    double lag_threshold = avg * lag_threshold_multiplier;
                    for (double d : data.all_durations) {
                        if (d > lag_threshold) {
                            lag_count++;
                            if (d > max_lag_duration) {
                                max_lag_duration = d;
                            }
                        }
                    }
                }
            } else if (data.call_count == 1) {
                p1 = data.max_duration;
                p99 = data.max_duration;
            }

            os << std::left << std::setw(50) << data.name << std::fixed
               << std::setprecision(4) << std::setw(15)
               << data.total_duration * 1000.0 << std::setw(10)
               << data.call_count << std::setw(15)
               << data.average_duration() * 1000.0 << std::setw(15)
               << data.min_duration * 1000.0 << std::setw(15)
               << data.max_duration * 1000.0 << std::setw(15)
               << data.last_duration * 1000.0 << std::setw(15) << p1 * 1000.0
               << std::setw(15) << p99 * 1000.0 << std::setw(12) << lag_count
               << std::setw(15) << max_lag_duration * 1000.0 << "\n";
        }
        os << "----------------------------------------------------------------"
              "----------------------------------------------------------------"
              "------------------------------------------\n";
    }
};

inline ScopedTimer::ScopedTimer(const std::string& name,
                                Profiler& profiler_instance, bool is_enabled)
    : profiler(profiler_instance), name(name), enabled(is_enabled) {
    if (enabled) {
        start_time = std::chrono::high_resolution_clock::now();
    }
}

inline ScopedTimer::~ScopedTimer() {
    if (enabled) {
        auto end_time = std::chrono::high_resolution_clock::now();
        double duration =
            std::chrono::duration<double>(end_time - start_time).count();
        profiler.record(name, duration);
    }
}

#define PROFILE_SCOPE_CONDITIONAL(name, enabled) \
    ScopedTimer timer##__LINE__(name, Profiler::get(), enabled)

#define PROFILE_FUNCTION_CONDITIONAL(enabled) \
    PROFILE_SCOPE_CONDITIONAL(__FUNCTION__, enabled)

#define PROFILE_SCOPE(name) PROFILE_SCOPE_CONDITIONAL(name, true)
#define PROFILE_FUNCTION() PROFILE_FUNCTION_CONDITIONAL(true)