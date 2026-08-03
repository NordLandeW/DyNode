
#include <string>

#include "activation.h"
#include "api.h"
#include "note.h"
#include "notePoolManager.h"
#include "utils.h"

// Clears all notes from the note map.
DYCORE_API double DyCore_clear_notes() {
    clear_notes();
    return 0;
}

// Inserts a single note.
DYCORE_API double DyCore_insert_note(const char* prop) {
    Note note;
    note.read(prop);
    return insert_note(note);
}

// Deletes a single note from noteID.
DYCORE_API double DyCore_delete_note(const char* noteID) {
    return delete_note(noteID);
}

DYCORE_API double DyCore_modify_note(const char* prop) {
    try {
        return modify_note(prop);
    } catch (...) {
        return -1;
    }
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
        note.write(prop);
    } catch (const std::exception& e) {
        print_debug_message("Error: " + std::string(e.what()));
        return false;
    }
    return true;
}
bool get_note_bitwise(int index, char* prop) {
    try {
        auto note = get_note_pool_manager()[index];
        note.write(prop);
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
        note.write(prop);
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
    double time;
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

DYCORE_API double DyCore_cac_active_notes(double nowTime, double noteSpeed) {
    auto& man = get_note_activation_manager();
    man.set_range(nowTime, noteSpeed);
    man.recalculate();
    return man.get_bitwrite_bound();
}

DYCORE_API double DyCore_get_active_notes(char* buffer) {
    auto& man = get_note_activation_manager();
    man.bitwrite_active_notes(buffer);
    return 0;
}

DYCORE_API double DyCore_get_lasting_holds(char* buffer) {
    auto& man = get_note_activation_manager();
    man.bitwrite_lasting_holds(buffer);
    return 0;
}

DYCORE_API double DyCore_get_active_notes_bound() {
    auto& man = get_note_activation_manager();
    return man.get_bitwrite_bound();
}

DYCORE_API double DyCore_get_note_index_lower_bound(double time) {
    auto& noteMan = get_note_pool_manager();
    noteMan.array_sort_request();
    int lowerBound = noteMan.get_index_lowerbound(time);
    return lowerBound;
}

DYCORE_API double DyCore_get_note_index_upper_bound(double time) {
    auto& noteMan = get_note_pool_manager();
    noteMan.array_sort_request();
    int upperBound = noteMan.get_index_upperbound(time);
    return upperBound;
}

DYCORE_API const char* DyCore_get_note_hash(const char* noteID,
                                            double includeID) {
    if (!note_exists(noteID))
        return "";
    static string hashStr;
    auto note = get_note_pool_manager().get_note(noteID);
    hashStr = note.get_hash_string(includeID != 0);
    return hashStr.c_str();
}

// This function is linear time.
DYCORE_API double DyCore_get_note_index_on_side_after_index(
    double side, double index, double untilTime = -1) {
    auto& noteMan = get_note_pool_manager();
    const int noteSide = static_cast<int>(side);
    const int beginIndex = static_cast<int>(index);
    noteMan.array_sort_request();

    if (beginIndex < 0)
        return -1;

    for (int i = beginIndex, l = noteMan.get_note_count(); i < l; i++) {
        const auto& note = noteMan[i];
        if (untilTime != -1 && note.time > untilTime)
            return -1;
        if (note.side == noteSide) {
            return i;
        }
    }

    return -1;
}
