#include <doctest/doctest.h>

#include <barrier>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "notePoolManager.h"

namespace {
Note make_note(const std::string& id, double time = 0) {
    Note note{};
    note.noteID = id;
    note.type = static_cast<int>(NOTE_TYPE::NORMAL);
    note.time = time;
    note.width = 1;
    return note;
}

void add_pair(NotePoolManager& pool, int index, bool subFirst = false) {
    auto head = make_note("hold-" + std::to_string(index), index * 100.0);
    auto sub = make_note("sub-" + std::to_string(index), head.time + 50);
    head.type = static_cast<int>(NOTE_TYPE::HOLD);
    sub.type = static_cast<int>(NOTE_TYPE::SUB);
    head.subNoteID = sub.noteID;
    sub.subNoteID = head.noteID;
    head.lastTime = 50;
    sub.beginTime = head.time;
    REQUIRE(pool.create_note(subFirst ? sub : head));
    REQUIRE(pool.create_note(subFirst ? head : sub));
}

void check_pair(const Note& head, const Note& sub) {
    CHECK(head.lastTime == sub.time - head.time);
    CHECK(sub.beginTime == head.time);
    CHECK(sub.position == head.position);
    CHECK(sub.width == head.width);
    CHECK(sub.side == head.side);
}
}  // namespace

TEST_CASE("NoteValueReadsRemainIndependentOfLaterEdits") {
    auto pool = std::make_unique<NotePoolManager>(1);
    auto note = make_note("note", 10);
    REQUIRE(pool->create_note(note));
    REQUIRE(pool->array_sort_request());
    const auto& byId = pool->get_note("note");
    const auto& byIndex = (*pool)[0];
    const auto& direct = pool->get_note_direct(0);
    note.time = 20;
    note.subNoteID = std::string(256, 'x');
    pool->set_note(note);
    CHECK(byId.time == 10);
    CHECK(byId.subNoteID.empty());
    CHECK(byIndex.time == 10);
    CHECK(direct.time == 10);
    pool->clear_notes();
    CHECK(byId.noteID == "note");
    CHECK(byIndex.noteID == "note");
    CHECK(direct.noteID == "note");
}

TEST_CASE("NoteDirectReadsRejectDeletedSlotsBeforeCompaction") {
    auto pool = std::make_unique<NotePoolManager>(1);
    REQUIRE(pool->create_note(make_note("removed", 10)));
    REQUIRE(pool->create_note(make_note("retained", 20)));
    pool->array_sort_request();
    REQUIRE(pool->release_note("removed"));
    CHECK_THROWS_AS(pool->get_note_direct(0), std::out_of_range);
    CHECK_THROWS_AS(pool->get_note_direct(-1), std::out_of_range);
    CHECK_THROWS_AS(pool->get_note_direct(2), std::out_of_range);
    CHECK(pool->get_note_direct(1).noteID == "retained");
    REQUIRE(pool->array_sort_request());
    CHECK(pool->get_note_direct(0).noteID == "retained");
    CHECK(pool->get_note_count() == 1);
}

TEST_CASE("NoteReadTraversalSharesItsLockAndDoesNotSynchronizeFields") {
    auto pool = std::make_unique<NotePoolManager>(1);
    auto head = make_note("head", 10);
    head.type = static_cast<int>(NOTE_TYPE::HOLD);
    head.subNoteID = "sub";
    head.lastTime = 123;
    auto sub = make_note("sub", 50);
    sub.type = static_cast<int>(NOTE_TYPE::SUB);
    sub.subNoteID = "head";
    sub.position = 7;
    sub.beginTime = -1;
    REQUIRE(pool->create_note(head));
    REQUIRE(pool->create_note(sub));
    pool->array_sort_request();
    std::vector<Note> before, after;
    pool->get_notes(before, false);
    std::barrier rendezvous(2);
    auto read = [&] {
        int count = 0;
        pool->read_all_notes([&](const Note&) {
            if (++count == 1) {
                // Both readers must hold their locks concurrently to proceed.
                rendezvous.arrive_and_wait();
            }
        });
        return count;
    };
    auto first = std::async(std::launch::async, read);
    auto second = std::async(std::launch::async, read);
    CHECK(first.get() == 2);
    CHECK(second.get() == 2);
    pool->get_notes(after, false);
    CHECK(nlohmann::json(before) == nlohmann::json(after));
    CHECK_FALSE(pool->is_ooo());
}

TEST_CASE("NoteCallbacksRejectEveryDataLockEntryAndRestoreTheirContext") {
    auto owner = std::make_unique<NotePoolManager>(1);
    auto& pool = *owner;
    const auto note = make_note("note");
    REQUIRE(pool.create_note(note));
    pool.array_sort_request();
    std::vector<Note> copies;
    const std::vector<std::function<void()>> entries = {
        [&] { (void)pool.note_exists("note"); },
        [&] { (void)pool.get_note_count(); },
        [&] { (void)pool.get_note("note"); },
        [&] { (void)pool[0]; },
        [&] { (void)pool.get_note_direct(0); },
        [&] { pool.get_notes(copies, false); },
        [&] { (void)pool.get_index("note"); },
        [&] { (void)pool.get_index_lowerbound(0); },
        [&] { (void)pool.get_index_upperbound(0); },
        [&] { (void)pool.array_sort_request(); },
        [&] { (void)pool.create_note(note); },
        [&] { (void)pool.release_note("note"); },
        [&] { pool.clear_notes(); },
        [&] { pool.set_note(note); },
        [&] { pool.access_note("note", [](Note&) {}); },
        [&] { pool.access_all_notes([](Note&) {}); },
        [&] { pool.access_all_notes_parallel([](Note&) {}); },
        [&] { pool.read_all_notes([](const Note&) {}); }};
    for (size_t i = 0; i < entries.size(); ++i) {
        CAPTURE(i);
        const auto& entry = entries[i];
        CHECK_THROWS_AS(pool.access_note("note", [&](Note&) { entry(); }),
                        std::logic_error);
        CHECK_THROWS_AS(pool.access_all_notes([&](Note&) { entry(); }),
                        std::logic_error);
        CHECK_THROWS_AS(pool.access_all_notes_parallel([&](Note&) { entry(); }),
                        std::logic_error);
        CHECK_THROWS_AS(pool.read_all_notes([&](const Note&) { entry(); }),
                        std::logic_error);
    }
    CHECK(pool.get_note_count() == 1);
    pool.access_note("note", [](Note& value) { value.position = 3; });
    CHECK(pool.get_note("note").position == 3);
}

TEST_CASE("NoteReentryDetectionTracksTheManagerAcrossWorkerHandoffs") {
    auto first = std::make_unique<NotePoolManager>(1);
    auto second = std::make_unique<NotePoolManager>(1);
    REQUIRE(first->create_note(make_note("a")));
    REQUIRE(second->create_note(make_note("b")));
    CHECK_NOTHROW(first->access_note("a", [&](Note&) {
        second->access_all_notes_parallel(
            [](Note& note) { note.position = 5; });
    }));
    CHECK(second->get_note("b").position == 5);
    CHECK_THROWS_AS(first->access_all_notes_parallel([&](Note&) {
        second->access_all_notes_parallel(
            [&](Note&) { (void)first->get_note_count(); });
    }),
                    std::logic_error);
    CHECK(first->get_note_count() == 1);
    CHECK(second->get_note_count() == 1);
}

TEST_CASE("NoteSerialBatchSynchronizesEachEditBeforeTheNextCallback") {
    for (bool subFirst : {false, true}) {
        CAPTURE(subFirst);
        auto pool = std::make_unique<NotePoolManager>(1);
        add_pair(*pool, 0, subFirst);
        pool->access_all_notes([&](Note& note) {
            if (note.get_note_type() == NOTE_TYPE::HOLD) {
                note.width = 3;
                note.position = 4;
            } else if (!subFirst) {
                CHECK(note.beginTime == 10);
                CHECK(note.width == 3);
                CHECK(note.position == 4);
            }
            note.time += 10;
        });
        const auto head = pool->get_note("hold-0");
        const auto sub = pool->get_note("sub-0");
        CHECK(head.time == 10);
        CHECK(sub.time == 60);
        check_pair(head, sub);
    }
}

TEST_CASE(
    "NoteParallelCallbacksOwnLinkedFieldsWithoutImplicitSynchronization") {
    auto pool = std::make_unique<NotePoolManager>(2);
    add_pair(*pool, 0);
    std::mutex pairMutex;
    pool->access_all_notes_parallel([&](Note& note) {
        // The caller serializes work on this pair and supplies derived fields.
        std::lock_guard lock(pairMutex);
        note.time += 10;
        note.width = 3;
        note.position = 4;
        if (note.get_note_type() == NOTE_TYPE::HOLD) {
            note.lastTime = 50;
        } else {
            note.beginTime = 10;
        }
    });
    check_pair(pool->get_note("hold-0"), pool->get_note("sub-0"));
    pool->access_all_notes_parallel([](Note& note) {
        if (note.get_note_type() == NOTE_TYPE::HOLD) {
            note.lastTime = 123;
        }
    });
    // The manager must preserve the caller's value rather than recomputing it.
    CHECK(pool->get_note("hold-0").lastTime == 123);
    CHECK(pool->get_note("sub-0").beginTime == 10);
}

TEST_CASE("NoteCallbackFailuresStillSynchronizeCompletedEdits") {
    auto pool = std::make_unique<NotePoolManager>(2);
    add_pair(*pool, 0);
    pool->array_sort_request();
    CHECK_THROWS_AS(
        pool->access_note("hold-0",
                          [](Note& note) {
                              note.time += 5;
                              note.width = 3;
                              throw std::runtime_error("partial edit");
                          }),
        std::runtime_error);
    CHECK(pool->is_ooo());
    CHECK(pool->get_note("hold-0").time == 5);
    check_pair(pool->get_note("hold-0"), pool->get_note("sub-0"));
    CHECK_THROWS_AS(pool->access_all_notes([](Note& note) {
        note.time += 1;
        if (note.get_note_type() == NOTE_TYPE::HOLD) {
            throw std::runtime_error("partial batch");
        }
    }),
                    std::runtime_error);
    check_pair(pool->get_note("hold-0"), pool->get_note("sub-0"));
    CHECK_NOTHROW(pool->access_all_notes([](Note& note) { note.time += 10; }));
    check_pair(pool->get_note("hold-0"), pool->get_note("sub-0"));
}

TEST_CASE("NoteParallelFailuresPreserveEditsWithoutImplicitSynchronization") {
    auto pool = std::make_unique<NotePoolManager>(2);
    add_pair(*pool, 0);
    pool->array_sort_request();
    CHECK_THROWS_AS(pool->access_all_notes_parallel([](Note& note) {
        if (note.get_note_type() == NOTE_TYPE::HOLD) {
            note.time = 10;
            throw std::runtime_error("partial parallel edit");
        }
    }),
                    std::runtime_error);
    CHECK(pool->is_ooo());
    const auto head = pool->get_note("hold-0");
    const auto sub = pool->get_note("sub-0");
    CHECK(head.time == 10);
    CHECK(head.lastTime == 50);
    CHECK(sub.beginTime == 0);
    CHECK_NOTHROW(pool->access_all_notes_parallel([](Note&) {}));
}

TEST_CASE("NoteSnapshotsStayConsistentDuringSingleAndLinkedEdits") {
    auto pool = std::make_unique<NotePoolManager>(2);
    add_pair(*pool, 0);
    auto normal = make_note("normal");
    normal.subNoteID = std::string(32, 'a');
    REQUIRE(pool->create_note(normal));
    std::barrier phase(2);
    auto writer = std::async(std::launch::async, [&] {
        for (int i = 1; i <= 256; ++i) {
            phase.arrive_and_wait();
            normal.time = i;
            normal.width = i + 1;
            normal.subNoteID = std::string(32 + i % 64, 'a' + i % 26);
            pool->set_note(normal);
            pool->access_note("hold-0", [i](Note& head) {
                head.time = i;
                head.position = i * 2;
                head.width = i + 1;
            });
            pool->access_note("sub-0", [i](Note& sub) { sub.time = i + 50; });
            phase.arrive_and_wait();
        }
    });
    for (int i = 0; i < 256; ++i) {
        phase.arrive_and_wait();
        std::vector<Note> snapshot;
        pool->get_notes(snapshot, false);
        CHECK(snapshot.size() == 3);
        if (snapshot.size() == 3) {
            check_pair(snapshot[0], snapshot[1]);
            const auto& value = snapshot[2];
            const int revision = static_cast<int>(value.time);
            CHECK(value.width == revision + 1);
            CHECK(value.subNoteID ==
                  std::string(32 + revision % 64, 'a' + revision % 26));
        }
        phase.arrive_and_wait();
    }
    writer.get();
}
