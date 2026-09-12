#pragma once

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct ProfileData {
    std::string name;
    double total_duration = 0.0;
    long long call_count = 0;
    double min_duration = (std::numeric_limits<double>::max)();
    double max_duration = 0.0;
    double last_duration = 0.0;
    std::vector<double> all_durations;
    size_t next_duration_index = 0;
    static constexpr size_t MAX_DURATIONS = 10000;

    ProfileData() {
        all_durations.reserve(MAX_DURATIONS);
    }

    void record(double duration) {
        total_duration += duration;
        call_count++;
        if (duration < min_duration)
            min_duration = duration;
        if (duration > max_duration)
            max_duration = duration;
        last_duration = duration;

        if (all_durations.size() < MAX_DURATIONS) {
            all_durations.push_back(duration);
        } else {
            all_durations[next_duration_index] = duration;
            next_duration_index = (next_duration_index + 1) % MAX_DURATIONS;
        }
    }

    double average_duration() const {
        return call_count > 0 ? total_duration / call_count : 0.0;
    }
};

class Profiler;

// Explicit borrowing is reserved for names that outlive the entire scope.
struct StaticProfileName {
    const char* value;
};

class ScopedTimer {
   private:
    Profiler& profiler;
    std::string ownedName;
    std::string_view name;
    std::chrono::high_resolution_clock::time_point start_time;
    bool enabled;

   public:
    ScopedTimer(StaticProfileName name, Profiler& profiler_instance,
                bool is_enabled = true);
    ScopedTimer(std::string name, Profiler& profiler_instance,
                bool is_enabled = true);
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    ~ScopedTimer();
};

class Profiler {
   private:
    struct NameHash {
        using is_transparent = void;
        size_t operator()(std::string_view name) const noexcept {
            return std::hash<std::string_view>{}(name);
        }
    };
    std::unordered_map<std::string, ProfileData, NameHash, std::equal_to<>>
        records;
    mutable std::mutex mtx;

    ProfileData& find_or_create(std::string_view name) {
        auto it = records.find(name);
        if (it == records.end()) {
            it = records.try_emplace(std::string(name)).first;
            it->second.name = name;
        }
        return it->second;
    }

    Profiler() {
        for (const char* name :
             {"Note Activation Manager Recalculate",
              "Note Pool Manager Array Sort", "DyCore_note_count",
              "DyCore_kps_count", "Render Active Notes (State 0)",
              "Render Active Notes (State 1)",
              "Render Active Notes (State 2)"}) {
            (void)find_or_create(name);
        }
    }

   public:
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

    static Profiler& get() {
        static Profiler instance;
        return instance;
    }

    void record(std::string_view name, double duration) {
        std::lock_guard<std::mutex> lock(mtx);
        find_or_create(name).record(duration);
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mtx);
        records.clear();
    }

    std::optional<double> get_last_duration_ms(std::string_view name) const {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = records.find(name);
        if (it == records.end() || it->second.call_count == 0) {
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
            if (pair.second.call_count != 0) {
                sorted_records.push_back(pair.second);
            }
        }

        std::sort(sorted_records.begin(), sorted_records.end(),
                  [](const auto& a, const auto& b) {
                      return a.average_duration() > b.average_duration();
                  });

        for (auto& data : sorted_records) {
            double p1 = 0.0, p99 = 0.0;
            int lag_count = 0;
            double max_lag_duration = 0.0;

            if (data.all_durations.size() > 1) {
                std::vector<double> sorted_durations = data.all_durations;
                std::sort(sorted_durations.begin(), sorted_durations.end());

                size_t p1_index =
                    static_cast<size_t>(sorted_durations.size() * 0.01);
                size_t p99_index =
                    static_cast<size_t>(sorted_durations.size() * 0.99);
                if (p99_index >= sorted_durations.size())
                    p99_index = sorted_durations.size() - 1;

                p1 = sorted_durations[p1_index];
                p99 = sorted_durations[p99_index];

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
            } else if (!data.all_durations.empty()) {
                p1 = data.all_durations[0];
                p99 = data.all_durations[0];
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

inline ScopedTimer::ScopedTimer(StaticProfileName name,
                                Profiler& profiler_instance, bool is_enabled)
    : profiler(profiler_instance), name(name.value), enabled(is_enabled) {
    if (enabled) {
        start_time = std::chrono::high_resolution_clock::now();
    }
}

inline ScopedTimer::ScopedTimer(std::string name, Profiler& profiler_instance,
                                bool is_enabled)
    : profiler(profiler_instance),
      ownedName(std::move(name)),
      name(ownedName),
      enabled(is_enabled) {
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

#define PROFILE_STATIC_SCOPE_CONDITIONAL(name, enabled)                   \
    ScopedTimer timer##__LINE__(StaticProfileName{name}, Profiler::get(), \
                                enabled)
#define PROFILE_STATIC_SCOPE(name) PROFILE_STATIC_SCOPE_CONDITIONAL(name, true)

#define PROFILE_FUNCTION_CONDITIONAL(enabled) \
    PROFILE_STATIC_SCOPE_CONDITIONAL(__FUNCTION__, enabled)

#define PROFILE_SCOPE(name) PROFILE_SCOPE_CONDITIONAL(name, true)
#define PROFILE_FUNCTION() PROFILE_FUNCTION_CONDITIONAL(true)
