#include "note.h"

#include <string>
#include <unordered_map>

#include "async.h"
#include "utils.h"

using std::string;

std::unordered_map<string, Note> currentNoteMap;

// Checks if a note exists in the current note map.
bool note_exists(const Note& note) {
    return currentNoteMap.find(note.noteID) != currentNoteMap.end();
}

// Clears all notes from the current note map.
void clear_notes() {
    mtxSaveProject.lock();
    currentNoteMap.clear();
    mtxSaveProject.unlock();
}

// Inserts a new note into the note map.
// Returns 0 on success, -1 if the note already exists.
int insert_note(Note note) {
    if (note_exists(note)) {
        print_debug_message("Warning: the given note has existed." +
                            note.noteID);
        return -1;
    }

    mtxSaveProject.lock();
    currentNoteMap[note.noteID] = note;
    mtxSaveProject.unlock();
    // print_debug_message("Insert note at:" + std::to_string(note.time));
    return 0;
}

// Deletes a note from the note map.
// Returns 0 on success, -1 if the note does not exist.
int delete_note(Note note) {
    if (!note_exists(note)) {
        print_debug_message("Warning: the given note is not existed." +
                            note.noteID);
        return -1;
    }

    mtxSaveProject.lock();
    currentNoteMap.erase(note.noteID);
    mtxSaveProject.unlock();
    return 0;
}

// Modifies an existing note in the note map.
// Returns 0 on success, -1 if the note does not exist.
int modify_note(Note note) {
    if (!note_exists(note)) {
        print_debug_message("Warning: Trying to modify a non-existing note: " +
                            note.noteID);
        return -1;
    }
    mtxSaveProject.lock();
    currentNoteMap[note.noteID] = note;
    mtxSaveProject.unlock();
    return 0;
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