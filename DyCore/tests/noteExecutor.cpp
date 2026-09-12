#include <doctest/doctest.h>

#include <atomic>
#include <format>
#include <future>
#include <latch>
#include <memory>

#include "notePoolManager.h"

namespace {
void add_note(NotePoolManager& pool, int index, double time) {
    Note note{};
    note.noteID = std::format("{:09}", index);
    note.type = static_cast<int>(NOTE_TYPE::NORMAL);
    note.time = time;
    note.width = 1.0;
    REQUIRE(pool.create_note(note));
}
}  // namespace

TEST_CASE("NoteExecutorReusePreservesSortingAndSurvivesClear") {
    auto owner = std::make_unique<NotePoolManager>(2);
    auto& pool = *owner;
    pool.initialize_executor();
    for (int i = 0; i < NOTES_ARRAY_PARALLEL_SORT_THRESHOLD; ++i) {
        add_note(pool, i, NOTES_ARRAY_PARALLEL_SORT_THRESHOLD - i);
    }
    REQUIRE(pool.array_sort_request());
    CHECK(pool.get_note(0).time == 1.0);
    CHECK(pool.get_note(NOTES_ARRAY_PARALLEL_SORT_THRESHOLD - 1).time ==
          NOTES_ARRAY_PARALLEL_SORT_THRESHOLD);
    pool.access_all_notes_parallel([](Note& note) { note.time += 10.0; });
    REQUIRE(pool.array_sort_request());
    CHECK(pool.get_note(0).time == 11.0);
    CHECK(pool.executor_creation_count() == 1);
    pool.clear_notes();
    add_note(pool, 0, 100);
    pool.access_all_notes_parallel_safe(
        [](Note& note) { note.position = 3.0; });
    CHECK(pool.get_note("000000000").position == 3.0);
    CHECK(pool.executor_creation_count() == 1);
    pool.shutdown_executor();
    CHECK_NOTHROW(pool.shutdown_executor());
    CHECK_THROWS_AS(pool.access_all_notes_parallel_safe([](Note&) {}),
                    std::logic_error);
}

TEST_CASE(
    "NoteExecutorSingleWorkerSupportsNestedSafeCallsAndRecoversFromException"
    "s") {
    auto owner = std::make_unique<NotePoolManager>(1);
    auto& pool = *owner;
    add_note(pool, 0, 100);
    std::atomic<int> visited = 0;
    pool.access_all_notes_parallel_safe([&](Note&) {
        pool.access_all_notes_parallel_safe([&](Note& note) {
            note.position = 4.0;
            ++visited;
        });
    });
    CHECK(visited == 1);
    CHECK(pool.get_note("000000000").position == 4.0);
    CHECK_THROWS_AS(pool.access_all_notes_parallel_safe([](Note&) {
        throw std::runtime_error("callback failure");
    }),
                    std::runtime_error);
    pool.access_all_notes_parallel_safe([&](Note&) { ++visited; });
    CHECK(visited == 2);
    CHECK(pool.executor_creation_count() == 1);
    CHECK_NOTHROW(pool.shutdown_executor());
}

TEST_CASE("NoteExecutorShutdownDrainsAnActiveCallback") {
    auto owner = std::make_unique<NotePoolManager>(1);
    auto& pool = *owner;
    add_note(pool, 0, 100);
    std::promise<void> entered;
    std::latch release(1);
    std::atomic<bool> finished = false;
    auto run = std::async(std::launch::async, [&] {
        pool.access_all_notes_parallel_safe([&](Note&) {
            entered.set_value();
            release.wait();
            finished = true;
        });
    });
    entered.get_future().get();
    auto shutdown = std::async(std::launch::async, [&] {
        pool.shutdown_executor();
        return finished.load();
    });
    release.count_down();
    CHECK(shutdown.get());
    run.get();
    CHECK_THROWS_AS(pool.access_all_notes_parallel_safe([](Note&) {}),
                    std::logic_error);
}
