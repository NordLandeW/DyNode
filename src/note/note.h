#pragma once

#include <string>

#include "bitio.h"
#include "json.hpp"

const int NOTE_ID_LENGTH = 9;

using nlohmann::json;
using std::string;

struct Note;
struct NoteExportView;

bool note_exists(const Note &note);
bool note_exists(const char *noteID);
bool note_exists(const string &noteID);
void clear_notes();
int insert_note(const Note &note);
int create_note(const Note &note, bool randomID = true, bool createSub = true);
int delete_note(const Note &note);
int delete_note(const std::string &noteID);
int modify_note(const Note &note);

string generate_note_id();

void get_notes_array(std::vector<Note> &notes, bool excludeSub = true);
void get_notes_array(std::vector<NoteExportView> &notes,
                     bool excludeSub = true);
std::string get_notes_array_string();

inline void from_json(const json &j, Note &n);
inline void to_json(json &j, const Note &n);
inline void to_json(json &j, const NoteExportView &n);

enum NOTE_TYPE { NORMAL, CHAIN, HOLD, SUB };

struct Note {
   public:
    int side;
    int type;
    double time;
    double width;
    double position;
    double lastTime;
    double beginTime;
    string noteID;
    string subNoteID;

    string dump() const {
        return json(*this).dump();
    }

    size_t bitsize() {
        return sizeof(int) * 2 + sizeof(double) * 5 +
               sizeof(char) * (noteID.size() + subNoteID.size() + 4);
    }

    void bitwrite(char *buffer) {
        char *ptr = buffer;
        bitwrite_int(ptr, side);
        bitwrite_int(ptr, type);
        bitwrite_double(ptr, time);
        bitwrite_double(ptr, width);
        bitwrite_double(ptr, position);
        bitwrite_double(ptr, lastTime);
        bitwrite_double(ptr, beginTime);
        bitwrite_string(ptr, noteID);
        bitwrite_string(ptr, subNoteID);
    }

    void bitread(const char *&buffer) {
        const char *ptr = buffer;
        bitread_int(ptr, side);
        bitread_int(ptr, type);
        bitread_double(ptr, time);
        bitread_double(ptr, width);
        bitread_double(ptr, position);
        bitread_double(ptr, lastTime);
        bitread_double(ptr, beginTime);
        bitread_string(ptr, noteID);
        bitread_string(ptr, subNoteID);
    }
};

struct NoteExportView {
    const Note &note;
    NoteExportView(const Note &n) : note(n) {
    }
};

inline void to_json(json &j, const NoteExportView &view) {
    j = json{{"time", view.note.time},       {"side", view.note.side},
             {"width", view.note.width},     {"position", view.note.position},
             {"length", view.note.lastTime}, {"type", view.note.type}};
}

inline void from_json(const json &j, Note &n) {
    j.at("time").get_to(n.time);
    j.at("side").get_to(n.side);
    j.at("width").get_to(n.width);
    j.at("position").get_to(n.position);
    j.at("length").get_to(n.lastTime);
    j.at("type").get_to(n.type);
    j.at("noteID").get_to(n.noteID);
    j.at("subNoteID").get_to(n.subNoteID);
    j.at("beginTime").get_to(n.beginTime);
}

inline void to_json(json &j, const Note &n) {
    j = json{{"time", n.time},          {"side", n.side},
             {"width", n.width},        {"position", n.position},
             {"length", n.lastTime},    {"type", n.type},
             {"noteID", n.noteID},      {"subNoteID", n.subNoteID},
             {"beginTime", n.beginTime}};
}
