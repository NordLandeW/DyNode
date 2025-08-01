#include "note.h"

#include <string>

#include "async.h"
#include "notePoolManager.h"
#include "singletons.h"
#include "utils.h"

using std::string;

// Checks if a note exists in the current note map.
bool note_exists(const Note& note) {
    return note_exists(note.noteID.c_str());
}
bool note_exists(const char* noteID) {
    return get_note_pool_manager().note_exists(noteID);
}
bool note_exists(const std::string& noteID) {
    return get_note_pool_manager().note_exists(noteID);
}

// Clears all notes from the current note map.
void clear_notes() {
    get_note_pool_manager().clear_notes();
}

// Inserts a new note into the note map.
// Returns 0 on success, -1 if the note already exists.
int insert_note(const Note& note) {
    if (note_exists(note)) {
        print_debug_message("Warning: the given note has existed." +
                            note.noteID);
        return -1;
    }

    get_note_pool_manager().create_note(note);
    return 0;
}

// Deletes a note from the note map.
// Returns 0 on success, -1 if the note does not exist.
int delete_note(const Note& note) {
    if (!note_exists(note)) {
        print_debug_message("Warning: the given note is not existed." +
                            note.noteID);
        return -1;
    }

    get_note_pool_manager().release_note(note);
    return 0;
}

// Modifies an existing note in the note map.
// Returns 0 on success, -1 if the note does not exist.
int modify_note(const Note& note) {
    if (!note_exists(note)) {
        print_debug_message("Warning: Trying to modify a non-existing note: " +
                            note.noteID);
        return -1;
    }
    get_note_pool_manager().set_note(note.noteID, note);
    return 0;
}

// For DYCORE_API.
bool get_note_bitwise(const std::string& noteID, char* prop) {
    if (!note_exists(noteID)) {
        return false;
    }
    try {
        auto note = get_note_pool_manager().get_note(noteID);
        note.bitwrite(prop);
    } catch (const std::exception& e) {
        print_debug_message("Error: " + std::string(e.what()));
        return false;
    }
    return true;
}
bool get_note_bitwise(int index, char* prop) {
    try {
        auto note = get_note_pool_manager()[index];
        note.bitwrite(prop);
    } catch (const std::exception& e) {
        print_debug_message("Error: " + std::string(e.what()));
        return false;
    }
    return true;
}

void get_note_array(std::vector<Note>& notes) {
    auto& noteMan = get_note_pool_manager();
    noteMan.access_all_notes([&](Note& note) {
        if (note.noteType != 3)
            notes.push_back(note);
    });
}
void get_note_array(std::vector<NoteExportView>& notes) {
    auto& noteMan = get_note_pool_manager();
    noteMan.access_all_notes([&](Note& note) {
        if (note.noteType != 3)
            notes.push_back(NoteExportView(note));
    });
}

// Synchronizes the note map with a given array of notes in JSON format.
// This will clear all existing notes and replace them with the new ones.
//
// @param notesArray A JSON string representing an array of Note objects.
// @return 0 on success, -1 on parsing failure.
DYCORE_API double DyCore_sync_notes_array(const char* notesArray) {
    print_debug_message("Start syncing... ");

    json j;
    try {
        j = json::parse(notesArray);
    } catch (json::exception& e) {
        print_debug_message("Parse failed:" + string(e.what()));
        return -1;
    }

    print_debug_message("Parse finished.");

    auto array = j.template get<std::vector<Note>>();

    print_debug_message(string("Sync notes successfully. ") +
                        std::to_string(array.size()));
    clear_notes();
    for (auto note : array)
        insert_note(note);
    return 0;
}

// Clears all notes from the note map.
DYCORE_API double DyCore_clear_notes() {
    clear_notes();
    return 0;
}

// Inserts a single note from a JSON string.
//
// @param prop A JSON string representing a Note object.
// @return 0 on success, -1 on failure.
DYCORE_API double DyCore_insert_note(const char* prop) {
    json j;
    try {
        j = json::parse(prop);
    } catch (json::exception& e) {
        print_debug_message("Parse failed:" + string(e.what()));
        return -1;
    }

    Note note = j.template get<Note>();
    return insert_note(note);
}

// Deletes a single note from a JSON string.
//
// @param prop A JSON string representing a Note object.
// @return 0 on success, -1 on failure.
DYCORE_API double DyCore_delete_note(const char* prop) {
    json j;
    try {
        j = json::parse(prop);
    } catch (json::exception& e) {
        print_debug_message("Parse failed:" + string(e.what()));
        return -1;
    }

    Note note = j.template get<Note>();
    return delete_note(note);
}

// Modifies a single note from a JSON string.
//
// @param prop A JSON string representing a Note object.
// @return 0 on success, -1 on failure.
DYCORE_API double DyCore_modify_note(const char* prop) {
    json j;
    try {
        j = json::parse(prop);
    } catch (json::exception& e) {
        print_debug_message("Parse failed:" + string(e.what()));
        return -1;
    }

    Note note = j.template get<Note>();
    return modify_note(note);
}

DYCORE_API double DyCore_modify_note_bitwise(const char* noteID,
                                             const char* prop) {
    if (!note_exists(noteID)) {
        return -1;
    }
    get_note_pool_manager().set_note_bitwise(noteID, prop);
    return 0;
}

DYCORE_API double DyCore_sort_notes() {
    return get_note_pool_manager().array_sort_request() ? 0 : -1;
}

DYCORE_API double DyCore_get_note(const char* noteID, char* propBuffer) {
    return get_note_bitwise(noteID, propBuffer) ? 0 : -1;
}

DYCORE_API double DyCore_get_note_at_index(double index, char* propBuffer) {
    return get_note_bitwise(static_cast<int>(index), propBuffer) ? 0 : -1;
}