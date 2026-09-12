#include <doctest/doctest.h>

#include <sstream>
#include <string>

#include "profile.h"

TEST_CASE(
    "ProfileSamplesRetainRecentDurationsWithoutGrowingAfterConstruction") {
    ProfileData data;
    const auto capacity = data.all_durations.capacity();
    for (size_t i = 0; i < ProfileData::MAX_DURATIONS + 3; ++i) {
        data.record(static_cast<double>(i));
    }
    CHECK(data.all_durations.capacity() == capacity);
    CHECK(data.all_durations.size() == ProfileData::MAX_DURATIONS);
    CHECK(*std::min_element(data.all_durations.begin(),
                            data.all_durations.end()) == 3.0);
    CHECK(data.last_duration ==
          static_cast<double>(ProfileData::MAX_DURATIONS + 2));
    CHECK(data.call_count == ProfileData::MAX_DURATIONS + 3);
}

TEST_CASE("ProfileStaticNamesAndOwnedDynamicNamesSurviveTheirInputLifetime") {
    auto& profiler = Profiler::get();
    profiler.reset();
    const std::string original(80, 'x');
    {
        std::string dynamic = original;
        ScopedTimer timer(dynamic, profiler);
        dynamic.assign(100, 'y');
    }
    CHECK(profiler.get_last_duration_ms(original).has_value());
    {
        ScopedTimer timer(std::string(90, 'z'), profiler);
    }
    CHECK(profiler.get_last_duration_ms(std::string(90, 'z')).has_value());
    {
        PROFILE_STATIC_SCOPE("static scope");
    }
    CHECK(profiler.get_last_duration_ms("static scope").has_value());
    {
        PROFILE_STATIC_SCOPE_CONDITIONAL("disabled scope", false);
    }
    CHECK_FALSE(profiler.get_last_duration_ms("disabled scope").has_value());
    profiler.reset();
    CHECK_FALSE(profiler.get_last_duration_ms(original).has_value());
    std::ostringstream report;
    profiler.generate_report(report);
    CHECK(report.str().find(original) == std::string::npos);
}
