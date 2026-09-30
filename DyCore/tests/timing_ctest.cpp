#include <doctest/doctest.h>

#include <array>
#include <barrier>
#include <cmath>
#include <json.hpp>
#include <string>
#include <thread>
#include <vector>

#include "timing.h"

extern "C" double DyCore_insert_timing_point(const char* timingPointObject);
extern "C" const char* DyCore_get_timing_point_at(double time);
extern "C" double DyCore_timing_points_reset();
extern "C" double DyCore_timing_points_change(double, const char*);

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

    REQUIRE(DyCore_timing_points_change(
                100, R"({"time":400,"beatLength":500,"meter":4})") == 0);
    check_timing_point_at(350, 300, 750, 3);
    check_timing_point_at(450, 400, 500, 4);
    REQUIRE(DyCore_timing_points_change(
                400, R"({"time":100,"beatLength":500,"meter":4})") == 0);
    check_timing_point_at(150, 100, 500, 4);
    check_timing_point_at(450, 300, 750, 3);

    DyCore_timing_points_reset();
}

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

TEST_CASE("TimingOrderedReadsSortOnceAndPreserveLookupBoundaries") {
    TimingManager manager;
    manager.append_timing_points({{300, 750, 3}, {100, 500, 4}, {200, 600, 5}});
    CHECK(manager.count() == 3);
    CHECK(manager.size() == 3);
    CHECK(manager.get_last_modified_time() == 1);

    auto check_points = [](const std::vector<TimingPoint>& points) {
        REQUIRE(points.size() == 3);
        CHECK(points[0].time == 100);
        CHECK(points[0].beatLength == 500);
        CHECK(points[0].meter == 4);
        CHECK(points[1].time == 200);
        CHECK(points[2].time == 300);
    };
    SUBCASE("Array copy") {
        std::vector<TimingPoint> points;
        manager.get_timing_points(points);
        check_points(points);
    }
    SUBCASE("Existence query") {
        CHECK(manager.has_timing_point_at(99.5));
        CHECK(manager.has_timing_point_at(100.5));
        CHECK_FALSE(manager.has_timing_point_at(99));
        CHECK_FALSE(manager.has_timing_point_at(101));
    }
    SUBCASE("Point at time") {
        TimingPoint point{};
        REQUIRE(manager.get_timing_point_at(50, point));
        CHECK(point.time == 100);
        REQUIRE(manager.get_timing_point_at(200, point));
        CHECK(point.time == 200);
        REQUIRE(manager.get_timing_point_at(250, point));
        CHECK(point.time == 200);
        REQUIRE(manager.get_timing_point_at(350, point));
        CHECK(point.time == 300);
    }
    SUBCASE("Index") {
        CHECK(manager[0].time == 100);
        CHECK(manager[2].time == 300);
    }
    SUBCASE("JSON") {
        check_points(manager.dump_json().get<std::vector<TimingPoint>>());
    }
    SUBCASE("String") {
        check_points(nlohmann::json::parse(manager.dump())
                         .get<std::vector<TimingPoint>>());
    }
    SUBCASE("Explicit sort") {
        manager.sort();
    }
    CHECK(manager.get_last_modified_time() == 2);
    std::vector<TimingPoint> points;
    manager.get_timing_points(points);
    check_points(points);
    manager.sort();
    CHECK(manager.get_last_modified_time() == 2);
}

TEST_CASE("TimingMutationsPreserveRevisionAndExactEditMatching") {
    TimingManager manager;
    TimingPoint point{42, 500, 4};
    CHECK_FALSE(manager.get_timing_point_at(0, point));
    CHECK(point.time == 42);
    CHECK_FALSE(manager.has_timing_point_at(0));
    CHECK(manager.get_last_modified_time() == 0);

    manager.clear();
    manager.delete_timing_point_at_time(0);
    manager.add_offset(0);
    CHECK(manager.get_last_modified_time() == 3);
    manager.append_timing_points({});
    CHECK(manager.get_last_modified_time() == 4);
    manager.clear();
    manager.sort();
    CHECK(manager.get_last_modified_time() == 6);
    CHECK(manager.dump_json().empty());
    CHECK(manager.get_last_modified_time() == 6);

    manager.add_timing_point({100, 500, 4});
    manager.change_timing_point_at_time(100.5, {300, 600, 3});
    CHECK(manager.get_last_modified_time() == 7);
    manager.change_timing_point_at_time(100, {300, 600, 3});
    CHECK(manager.get_last_modified_time() == 8);
    manager.delete_timing_point_at_time(300.5);
    CHECK(manager.count() == 1);
    CHECK(manager.get_last_modified_time() == 9);
    CHECK(manager[0].time == 300);
    CHECK(manager.get_last_modified_time() == 10);

    manager.append_timing_points({{100, 500, 4}, {200, 750, 5}});
    manager.add_offset(5);
    CHECK(manager.get_last_modified_time() == 12);
    std::vector<TimingPoint> points;
    manager.get_timing_points(points);
    REQUIRE(points.size() == 3);
    CHECK(points[0].time == 105);
    CHECK(points[1].time == 205);
    CHECK(points[2].time == 305);
    CHECK(manager.get_last_modified_time() == 13);
    manager.delete_timing_point_at_time(205);
    CHECK(manager.count() == 2);
    CHECK(manager[1].time == 305);
    CHECK(manager.get_last_modified_time() == 14);
    manager.change_timing_point_at_time(305, {55, 250, 7});
    CHECK(manager.get_last_modified_time() == 15);
    CHECK(manager[0].time == 55);
    CHECK(manager[0].beatLength == 250);
    CHECK(manager[0].meter == 7);
    CHECK(manager.get_last_modified_time() == 16);
}

TEST_CASE("TimingConcurrentOffsetsPublishWholeSnapshotsAndExactRevision") {
    constexpr int POINT_COUNT = 64;
    constexpr int WRITER_COUNT = 2;
    constexpr int READER_COUNT = 3;
    constexpr int ROUNDS = 32;
    constexpr double OFFSET = 1024;
    TimingManager manager;
    std::vector<TimingPoint> points;
    for (int i = 0; i < POINT_COUNT; ++i) {
        points.push_back({i * 10.0, 500.0 + i, 4});
    }
    manager.append_timing_points(points);
    manager.sort();
    const auto initialRevision = manager.get_last_modified_time();

    auto valid_snapshot = [](const std::vector<TimingPoint>& snapshot) {
        if (snapshot.size() != POINT_COUNT) {
            return false;
        }
        const auto offset = snapshot.front().time;
        if (offset < 0 || std::fmod(offset, OFFSET) != 0) {
            return false;
        }
        for (int i = 0; i < POINT_COUNT; ++i) {
            if (snapshot[i].time != offset + i * 10 ||
                snapshot[i].beatLength != 500 + i || snapshot[i].meter != 4) {
                return false;
            }
        }
        return true;
    };
    std::barrier phase(WRITER_COUNT + READER_COUNT);
    std::array<bool, READER_COUNT> validReaders;
    validReaders.fill(true);
    std::vector<std::jthread> threads;
    for (int writer = 0; writer < WRITER_COUNT; ++writer) {
        threads.emplace_back([&] {
            for (int round = 0; round < ROUNDS; ++round) {
                phase.arrive_and_wait();
                manager.add_offset(OFFSET);
                phase.arrive_and_wait();
            }
        });
    }
    for (int reader = 0; reader < READER_COUNT; ++reader) {
        threads.emplace_back([&, reader] {
            bool valid = true;
            auto lastRevision = initialRevision;
            for (int round = 0; round < ROUNDS; ++round) {
                phase.arrive_and_wait();
                std::vector<TimingPoint> snapshot;
                manager.get_timing_points(snapshot);
                valid &= valid_snapshot(snapshot);
                valid &= valid_snapshot(
                    manager.dump_json().get<std::vector<TimingPoint>>());
                valid &= valid_snapshot(nlohmann::json::parse(manager.dump())
                                            .get<std::vector<TimingPoint>>());
                valid &= manager.count() == POINT_COUNT;
                valid &= manager.size() == POINT_COUNT;
                valid &= !manager.has_timing_point_at(-100);
                TimingPoint first{};
                valid &= manager.get_timing_point_at(-1, first);
                valid &= first.beatLength == 500 && first.meter == 4;
                valid &= first.time >= 0 && std::fmod(first.time, OFFSET) == 0;
                const auto last = manager[POINT_COUNT - 1];
                valid &= last.beatLength == 500 + POINT_COUNT - 1;
                valid &=
                    std::fmod(last.time - (POINT_COUNT - 1) * 10, OFFSET) == 0;
                const auto revision = manager.get_last_modified_time();
                valid &= revision >= lastRevision;
                lastRevision = revision;
                phase.arrive_and_wait();
                // Writers wait at the next start barrier during this check.
                valid &= manager.get_last_modified_time() ==
                         initialRevision + (round + 1) * WRITER_COUNT;
            }
            validReaders[reader] = valid;
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    for (bool valid : validReaders) {
        CHECK(valid);
    }
    CHECK(manager.get_last_modified_time() ==
          initialRevision + ROUNDS * WRITER_COUNT);
    manager.get_timing_points(points);
    REQUIRE(valid_snapshot(points));
    CHECK(points.front().time == ROUNDS * WRITER_COUNT * OFFSET);
}

TEST_CASE("TimingConcurrentAppendsAndLazySortReturnCompleteBatches") {
    constexpr int WRITER_COUNT = 2;
    constexpr int READER_COUNT = 3;
    constexpr int ROUNDS = 32;
    constexpr int BATCH_SIZE = 4;
    constexpr int BATCH_COUNT = WRITER_COUNT * ROUNDS;
    TimingManager manager;
    manager.add_timing_point({0, 500, 4});
    manager.sort();
    const auto initialRevision = manager.get_last_modified_time();

    auto valid_snapshot = [](const std::vector<TimingPoint>& snapshot) {
        if (snapshot.empty() || snapshot.front().time != 0 ||
            snapshot.front().beatLength != 500 || snapshot.front().meter != 4) {
            return false;
        }
        std::array<int, BATCH_COUNT> batchSizes{};
        for (size_t i = 1; i < snapshot.size(); ++i) {
            const auto& point = snapshot[i];
            if (point.time <= snapshot[i - 1].time || point.time < 10 ||
                point.time > BATCH_COUNT * BATCH_SIZE * 10 ||
                std::fmod(point.time, 10) != 0) {
                return false;
            }
            const int id = static_cast<int>(point.time / 10);
            if (point.beatLength != 500 + id || point.meter != 4) {
                return false;
            }
            ++batchSizes[(id - 1) / BATCH_SIZE];
        }
        for (int size : batchSizes) {
            if (size != 0 && size != BATCH_SIZE) {
                return false;
            }
        }
        return true;
    };
    std::barrier phase(WRITER_COUNT + READER_COUNT);
    std::array<bool, READER_COUNT> validReaders;
    validReaders.fill(true);
    std::vector<std::jthread> threads;
    for (int writer = 0; writer < WRITER_COUNT; ++writer) {
        threads.emplace_back([&, writer] {
            for (int round = 0; round < ROUNDS; ++round) {
                std::vector<TimingPoint> batch;
                for (int i = BATCH_SIZE; i > 0; --i) {
                    const int id =
                        (round * WRITER_COUNT + writer) * BATCH_SIZE + i;
                    batch.push_back({id * 10.0, 500.0 + id, 4});
                }
                phase.arrive_and_wait();
                manager.append_timing_points(batch);
                phase.arrive_and_wait();
            }
        });
    }
    for (int reader = 0; reader < READER_COUNT; ++reader) {
        threads.emplace_back([&, reader] {
            bool valid = true;
            auto lastRevision = initialRevision;
            for (int round = 0; round < ROUNDS; ++round) {
                phase.arrive_and_wait();
                std::vector<TimingPoint> snapshot;
                manager.get_timing_points(snapshot);
                valid &= valid_snapshot(snapshot);
                valid &= valid_snapshot(
                    manager.dump_json().get<std::vector<TimingPoint>>());
                valid &= manager.has_timing_point_at(0);
                valid &= manager[0].time == 0;
                phase.arrive_and_wait();
                // All appends for this round have completed; no next-round
                // write can begin until every reader reaches the start barrier.
                manager.get_timing_points(snapshot);
                valid &= valid_snapshot(snapshot);
                valid &= snapshot.size() ==
                         1 + (round + 1) * WRITER_COUNT * BATCH_SIZE;
                const auto revision = manager.get_last_modified_time();
                valid &= revision > lastRevision;
                valid &= revision >=
                         initialRevision + (round + 1) * (WRITER_COUNT + 1);
                valid &= revision <=
                         initialRevision + (round + 1) * WRITER_COUNT * 2;
                lastRevision = revision;
            }
            validReaders[reader] = valid;
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    for (bool valid : validReaders) {
        CHECK(valid);
    }
    CHECK(manager.count() == 1 + BATCH_COUNT * BATCH_SIZE);
    const auto revision = manager.get_last_modified_time();
    manager.sort();
    CHECK(manager.get_last_modified_time() == revision);
}
