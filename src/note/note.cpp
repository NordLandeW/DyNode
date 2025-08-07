#include "note.h"

#include <string>

#include "gm.h"
#include "notePoolManager.h"
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
        print_debug_message("Warning: the given note has existed. " +
                            note.noteID);
        return -1;
    }

    get_note_pool_manager().create_note(note);
    return 0;
}

// Creates a note with a random ID and optionally creates a sub-note.
// Returns 0 on success, -1 if the note already exists.
int create_note(const Note& note, bool randomID, bool createSub) {
    if (note_exists(note)) {
        print_debug_message("Warning: the given note has existed. " +
                            note.noteID);
        return -1;
    }

    Note newNote(note);
    if (randomID)
        newNote.noteID = generate_note_id();
    if (note.type == 2 && createSub) {
        newNote.subNoteID = generate_note_id();
        Note newSubNote(newNote);
        std::swap(newSubNote.noteID, newSubNote.subNoteID);
        newSubNote.time = newNote.time + newNote.lastTime;
        newSubNote.lastTime = 0;
        newSubNote.beginTime = newNote.time;
        newSubNote.type = 3;
        insert_note(newSubNote);
    }
    insert_note(newNote);
    return 0;
}

// Deletes a note from the note map.
// Returns 0 on success, -1 if the note does not exist.
int delete_note(const Note& note) {
    if (!note_exists(note)) {
        print_debug_message("Warning: the given note is not existed. " +
                            note.noteID);
        return -1;
    }

    get_note_pool_manager().release_note(note);
    return 0;
}
int delete_note(const std::string& noteID) {
    if (!note_exists(noteID)) {
        print_debug_message("Warning: the given note is not existed. " +
                            noteID);
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

void get_notes_array(std::vector<Note>& notes, bool excludeSub) {
    get_note_pool_manager().get_notes(notes, excludeSub);
}
void get_notes_array(std::vector<NoteExportView>& notes, bool excludeSub) {
    std::vector<Note> noteArray;
    get_note_pool_manager().get_notes(noteArray, excludeSub);
    for (const auto& note : noteArray) {
        notes.push_back(NoteExportView(note));
    }
}

string generate_note_id() {
    return random_string(NOTE_ID_LENGTH);
}

string get_notes_array_string() {
    json js;

    std::vector<NoteExportView> notes;
    get_notes_array(notes);

    js = notes;

    return nlohmann::to_string(js);
}
