#include <doctest/doctest.h>

#include <string>

extern "C" double DyCore_clear_notes();
extern "C" double DyCore_get_note_index_lower_bound(double time);
extern "C" double DyCore_get_note_index_upper_bound(double time);
extern "C" double DyCore_sync_notes_array(const char* notesArray);

TEST_CASE("NoteIndexBounds") {
    DyCore_clear_notes();

    const std::string notes = R"([
        {
            "time": 100.0,
            "side": 0,
            "width": 1.0,
            "position": 1.0,
            "length": 0.0,
            "type": 0,
            "noteID": "note-a",
            "subNoteID": "",
            "beginTime": 0.0
        },
        {
            "time": 200.0,
            "side": 0,
            "width": 1.0,
            "position": 2.0,
            "length": 0.0,
            "type": 0,
            "noteID": "note-b",
            "subNoteID": "",
            "beginTime": 0.0
        },
        {
            "time": 200.0,
            "side": 1,
            "width": 1.0,
            "position": 3.0,
            "length": 0.0,
            "type": 0,
            "noteID": "note-c",
            "subNoteID": "",
            "beginTime": 0.0
        },
        {
            "time": 300.0,
            "side": 0,
            "width": 1.0,
            "position": 4.0,
            "length": 0.0,
            "type": 0,
            "noteID": "note-d",
            "subNoteID": "",
            "beginTime": 0.0
        }
    ])";

    REQUIRE(DyCore_sync_notes_array(notes.c_str()) == 0);

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
