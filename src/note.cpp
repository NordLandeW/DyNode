#include "note.h"

#include <string>
#include <unordered_map>

#include "async.h"
#include "utils.h"

using std::string;

std::unordered_map<string, Note> currentNoteMap;

bool note_exists(const Note& note) {
    return currentNoteMap.find(note.inst) != currentNoteMap.end();
}

void clear_notes() {
    mtxSaveProject.lock();
    currentNoteMap.clear();
    mtxSaveProject.unlock();
}

int insert_note(Note note) {
    if (note_exists(note)) {
        print_debug_message("Warning: the given note has existed." + note.inst);
        return -1;
    }

    mtxSaveProject.lock();
    currentNoteMap[note.inst] = note;
    mtxSaveProject.unlock();
    // print_debug_message("Insert note at:" + std::to_string(note.time));
    return 0;
}

int delete_note(Note note) {
    if (!note_exists(note)) {
        print_debug_message("Warning: the given note is not existed." +
                            note.inst);
        return -1;
    }

    mtxSaveProject.lock();
    currentNoteMap.erase(note.inst);
    mtxSaveProject.unlock();
    return 0;
}

int modify_note(Note note) {
    if (!note_exists(note)) {
        print_debug_message("Warning: the given note is not existed." +
                            note.inst);
        return -1;
    }
    mtxSaveProject.lock();
    currentNoteMap[note.inst] = note;
    mtxSaveProject.unlock();
    return 0;
}

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

DYCORE_API double DyCore_clear_notes() {
    clear_notes();
    return 0;
}

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