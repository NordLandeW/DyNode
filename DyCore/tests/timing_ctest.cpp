#include <doctest/doctest.h>

#include <json.hpp>
#include <string>

extern "C" double DyCore_insert_timing_point(const char* timingPointObject);
extern "C" const char* DyCore_get_timing_point_at(double time);
extern "C" double DyCore_timing_points_reset();

namespace {

void check_timing_point_at(double queryTime, double expectedTime,
                           double expectedBeatLength, int expectedMeter) {
    const auto point =
        nlohmann::json::parse(DyCore_get_timing_point_at(queryTime));

    CHECK(point.at("time").get<double>() == doctest::Approx(expectedTime));
    CHECK(point.at("beatLength").get<double>() ==
          doctest::Approx(expectedBeatLength));
    CHECK(point.at("meter").get<int>() == expectedMeter);
}

}  // namespace

TEST_CASE("TimingPointAtTime") {
    DyCore_timing_points_reset();

    CHECK(std::string(DyCore_get_timing_point_at(100.0)).empty());

    REQUIRE(DyCore_insert_timing_point(
                R"({"time":300.0,"beatLength":750.0,"meter":3})") == 0);
    REQUIRE(DyCore_insert_timing_point(
                R"({"time":100.0,"beatLength":500.0,"meter":4})") == 0);
    REQUIRE(DyCore_insert_timing_point(
                R"({"time":200.0,"beatLength":600.0,"meter":5})") == 0);

    check_timing_point_at(50.0, 100.0, 500.0, 4);
    check_timing_point_at(100.0, 100.0, 500.0, 4);
    check_timing_point_at(150.0, 100.0, 500.0, 4);
    check_timing_point_at(200.0, 200.0, 600.0, 5);
    check_timing_point_at(250.0, 200.0, 600.0, 5);
    check_timing_point_at(300.0, 300.0, 750.0, 3);
    check_timing_point_at(350.0, 300.0, 750.0, 3);

    DyCore_timing_points_reset();
}

extern "C" double DyCore_timing_points_change(double, const char*);
extern "C" double DyCore_get_timing_points_last_modified_time();
extern "C" const char* DyCore_get_timing_array_string();

TEST_CASE("InvalidTimingEditsLeaveDataAndRevisionUnchanged") {
    DyCore_timing_points_reset();
    REQUIRE(DyCore_insert_timing_point(
                R"({"time":-100,"beatLength":500,"meter":4})") == 0);
    const std::string before = DyCore_get_timing_array_string();
    const double revision = DyCore_get_timing_points_last_modified_time();
    const char* invalid[] = {
        R"({"time":-100,"beatLength":-500,"meter":4})",
        R"({"time":-100,"beatLength":0,"meter":4})",
        R"({"time":-100,"beatLength":500,"meter":0})",
        R"({"time":-100,"beatLength":500,"meter":-4})",
        R"({"time":-100,"beatLength":500,"meter":1.5})",
        R"({"time":-100,"beatLength":500,"meter":4294967300})",
        R"({"time":null,"beatLength":500,"meter":4})",
        R"({"time":-100,"beatLength":null,"meter":4})",
        R"({"time":-100,"beatLength":1e999,"meter":4})",
        "{",
        nullptr};
    for (const char* input : invalid) {
        CAPTURE(input ? input : "null");
        CHECK(DyCore_insert_timing_point(input) == -1);
        CHECK(DyCore_timing_points_change(-100, input) == -1);
        CHECK(std::string(DyCore_get_timing_array_string()) == before);
        CHECK(DyCore_get_timing_points_last_modified_time() == revision);
    }
    REQUIRE(DyCore_timing_points_change(
                -100, R"({"time":-100,"beatLength":250.5,"meter":3})") == 0);
    check_timing_point_at(0, -100, 250.5, 3);
    CHECK(DyCore_get_timing_points_last_modified_time() > revision);
    DyCore_timing_points_reset();
}
