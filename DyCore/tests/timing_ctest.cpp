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
