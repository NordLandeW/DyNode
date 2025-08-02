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

    print_debug_message("Insert note: " + note.dump());

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
int delete_note(const std::string& noteID) {
    if (!note_exists(noteID)) {
        print_debug_message("Warning: the given note is not existed." + noteID);
        return -1;
    }

    get_note_pool_manager().release_note(noteID);
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