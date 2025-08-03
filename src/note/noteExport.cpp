
#include <json.hpp>
#include <string>

#include "api.h"
#include "note.h"
#include "singletons.h"
#include "utils.h"

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
    Note note;
    note.bitread(prop);
    return insert_note(note);
}

// Deletes a single note from noteID.
//
// @param noteID A string representing the ID of the Note to delete.
// @return 0 on success, -1 on failure.
DYCORE_API double DyCore_delete_note(const char* noteID) {
    return delete_note(noteID);
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

/// Gets a note's bitwise properties directly by index.
/// This is unsafe and should only be used when you are sure the index is valid.
bool get_note_bitwise_direct(int index, char* prop) {
    try {
        auto note = get_note_pool_manager().get_note_direct(index);
        note.bitwrite(prop);
    } catch (const std::exception& e) {
        print_debug_message("Error: " + std::string(e.what()));
        return false;
    }
    return true;
}

DYCORE_API double DyCore_get_note(const char* noteID, char* propBuffer) {
    if (!note_exists(noteID))
        return -1;
    return get_note_bitwise(noteID, propBuffer) ? 0 : -1;
}

DYCORE_API double DyCore_get_note_count() {
    return get_note_pool_manager().get_note_count();
}

DYCORE_API double DyCore_get_note_at_index(double index, char* propBuffer) {
    return get_note_bitwise(static_cast<int>(index), propBuffer) ? 0 : -1;
}

DYCORE_API double DyCore_get_note_at_index_direct(double index,
                                                  char* propBuffer) {
    return get_note_bitwise_direct(static_cast<int>(index), propBuffer) ? 0
                                                                        : -1;
}

DYCORE_API double DyCore_get_note_time_at_index(double index) {
    auto& noteMan = get_note_pool_manager();
    int time;
    try {
        time = noteMan[static_cast<int>(index)].time;
    } catch (const std::exception& e) {
        print_debug_message("Error: " + std::string(e.what()));
        return -1;
    }
    return time;
}

DYCORE_API const char* DyCore_get_note_id_at_index(double index) {
    auto& noteMan = get_note_pool_manager();
    static string noteID;
    try {
        noteID = noteMan[static_cast<int>(index)].noteID;
    } catch (const std::exception& e) {
        print_debug_message("Error: " + std::string(e.what()));
        return "";
    }
    return noteID.c_str();
}

DYCORE_API double DyCore_get_note_array_index(const char* noteID) {
    if (!note_exists(noteID))
        return -1;
    auto& noteMan = get_note_pool_manager();
    int ind;
    try {
        ind = noteMan.get_index(noteID);
    } catch (const std::exception& e) {
        print_debug_message("Error: " + std::string(e.what()));
        return -1;
    }
    return ind;
}

DYCORE_API double DyCore_note_exists(const char* noteID) {
    if (!note_exists(noteID))
        return -1;
    return 0;
}

DYCORE_API const char* DyCore_generate_note_id() {
    static string noteID;
    noteID = generate_note_id();
    return noteID.c_str();
}