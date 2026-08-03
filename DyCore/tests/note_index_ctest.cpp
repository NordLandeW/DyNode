#include <doctest/doctest.h>

#include "note.h"

extern "C" double DyCore_clear_notes();
extern "C" double DyCore_get_note_index_lower_bound(double time);
extern "C" double DyCore_get_note_index_upper_bound(double time);
extern "C" double DyCore_get_note_index_on_side_after_index(
    double side, double index, double untilTime);

TEST_CASE("NoteIndexBounds") {
    DyCore_clear_notes();

    const auto insertTestNote = [](double time, int side, double position,
                                   const char* noteID) {
        Note note{};
        note.time = time;
        note.side = side;
        note.width = 1.0;
        note.position = position;
        note.noteID = noteID;
        return insert_note(note);
    };

    REQUIRE(insertTestNote(100.0, 0, 1.0, "note-a") == 0);
    REQUIRE(insertTestNote(200.0, 0, 2.0, "note-b") == 0);
    REQUIRE(insertTestNote(200.0, 1, 3.0, "note-c") == 0);
    REQUIRE(insertTestNote(300.0, 0, 4.0, "note-d") == 0);

    CHECK(DyCore_get_note_index_lower_bound(50.0) == 0);
    CHECK(DyCore_get_note_index_upper_bound(50.0) == 0);

    CHECK(DyCore_get_note_index_lower_bound(100.0) == 0);
    CHECK(DyCore_get_note_index_upper_bound(100.0) == 1);

    CHECK(DyCore_get_note_index_lower_bound(200.0) == 1);
    CHECK(DyCore_get_note_index_upper_bound(200.0) == 3);

    CHECK(DyCore_get_note_index_lower_bound(250.0) == 3);
    CHECK(DyCore_get_note_index_upper_bound(250.0) == 3);

    CHECK(DyCore_get_note_index_lower_bound(300.0) == 3);
    CHECK(DyCore_get_note_index_upper_bound(300.0) == 4);

    CHECK(DyCore_get_note_index_lower_bound(350.0) == 4);
    CHECK(DyCore_get_note_index_upper_bound(350.0) == 4);

    DyCore_clear_notes();
}

TEST_CASE("NoteIndexOnSideAfterIndex") {
    DyCore_clear_notes();

    const auto insertTestNote = [](double time, int side,
                                   const char* noteID) {
        Note note{};
        note.time = time;
        note.side = side;
        note.width = 1.0;
        note.noteID = noteID;
        return insert_note(note);
    };

    REQUIRE(insertTestNote(100.0, 0, "note-a") == 0);
    REQUIRE(insertTestNote(200.0, 1, "note-b") == 0);
    REQUIRE(insertTestNote(300.0, 0, "note-c") == 0);
    REQUIRE(insertTestNote(400.0, 1, "note-d") == 0);

    CHECK(DyCore_get_note_index_on_side_after_index(1, 0, -1) == 1);
    CHECK(DyCore_get_note_index_on_side_after_index(1, 2, -1) == 3);
    CHECK(DyCore_get_note_index_on_side_after_index(2, 0, -1) == -1);
    CHECK(DyCore_get_note_index_on_side_after_index(1, 0, 199) == -1);
    CHECK(DyCore_get_note_index_on_side_after_index(1, 0, 200) == 1);
    CHECK(DyCore_get_note_index_on_side_after_index(1, -1, -1) == -1);

    DyCore_clear_notes();
}
