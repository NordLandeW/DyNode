#pragma once

#include <string>

#include "api.h"
#include "json.hpp"

using nlohmann::json;
using std::string;

struct Note;
struct NoteExportView;

bool note_exists(const Note &note);
bool note_exists(const char *noteID);
void clear_notes();
int insert_note(Note note);
int delete_note(Note note);
int modify_note(Note note);
Note &get_note_ref(const string &noteID);

DYCORE_API double DyCore_clear_notes();
DYCORE_API double DyCore_sync_notes_array(const char *notesArray);
DYCORE_API double DyCore_modify_note(const char *prop);
DYCORE_API double DyCore_delete_note(const char *prop);
DYCORE_API double DyCore_insert_note(const char *prop);
DYCORE_API double DyCore_modify_note_bitwise(const char *noteID,
                                             const char *prop);

extern std::unordered_map<string, Note> currentNoteMap;
inline void from_json(const json &j, Note &n);
inline void to_json(json &j, const Note &n);
inline void to_json(json &j, const NoteExportView &n);

struct Note {
   private:
    static void bitwrite_int(char *&buffer, const int &value) {
        memcpy(buffer, &value, sizeof(int));
        buffer += sizeof(int);
    }
    static void bitwrite_double(char *&buffer, const double &value) {
        memcpy(buffer, &value, sizeof(double));
        buffer += sizeof(double);
    }
    static void bitwrite_string(char *&buffer, const string &value) {
        memcpy(buffer, value.data(), value.size() + 1);
        buffer += value.size() + 1;
    }
    static void bitread_int(const char *&buffer, int &value) {
        memcpy(&value, buffer, sizeof(int));
        buffer += sizeof(int);
    }
    static void bitread_double(const char *&buffer, double &value) {
        memcpy(&value, buffer, sizeof(double));
        buffer += sizeof(double);
    }
    static void bitread_string(const char *&buffer, string &value) {
        int size = strlen(buffer);
        value.assign(buffer, size);
        buffer += size + 1;
    }

   public:
    int side;
    int noteType;
    double time;
    double width;
    double position;
    double lastTime;
    double beginTime;
    string noteID;
    string subNoteID;

    string dump() {
        return json(*this).dump();
    }

    size_t bitsize() {
        return sizeof(int) * 2 + sizeof(double) * 5 +
               sizeof(char) * (noteID.size() + subNoteID.size() + 4);
    }

    void bitwrite(char *buffer) {
        char *ptr = buffer;
        bitwrite_int(ptr, side);
        bitwrite_int(ptr, noteType);
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
        bitread_int(ptr, noteType);
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
    const Note note;
    NoteExportView(const Note n) : note(n) {
    }
};

inline void to_json(json &j, const NoteExportView &view) {
    j = json{
        {"time", view.note.time},         {"side", view.note.side},
        {"width", view.note.width},       {"position", view.note.position},
        {"lastTime", view.note.lastTime}, {"noteType", view.note.noteType}};
}

inline void from_json(const json &j, Note &n) {
    j.at("time").get_to(n.time);
    j.at("side").get_to(n.side);
    j.at("width").get_to(n.width);
    j.at("position").get_to(n.position);
    j.at("lastTime").get_to(n.lastTime);
    j.at("noteType").get_to(n.noteType);
    j.at("noteID").get_to(n.noteID);
    j.at("subNoteID").get_to(n.subNoteID);
    j.at("beginTime").get_to(n.beginTime);
}

inline void to_json(json &j, const Note &n) {
    j = json{{"time", n.time},          {"side", n.side},
             {"width", n.width},        {"position", n.position},
             {"lastTime", n.lastTime},  {"noteType", n.noteType},
             {"noteID", n.noteID},      {"subNoteID", n.subNoteID},
             {"beginTime", n.beginTime}};
}