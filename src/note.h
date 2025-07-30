#pragma once

#include <string>

#include "api.h"
#include "json.hpp"

using nlohmann::json;
using std::string;

DYCORE_API double DyCore_clear_notes();
DYCORE_API double DyCore_sync_notes_array(const char *notesArray);
DYCORE_API double DyCore_modify_note(const char *prop);
DYCORE_API double DyCore_delete_note(const char *prop);
DYCORE_API double DyCore_insert_note(const char *prop);

struct Note;
struct NoteExportView;
extern std::unordered_map<string, Note> currentNoteMap;
inline void from_json(const json &j, Note &n);
inline void to_json(json &j, const Note &n);
inline void to_json(json &j, const NoteExportView &n);

struct Note {
    double time;
    int side;
    double width;
    double position;
    double lastTime;
    int noteType;
    string noteID;
    string subNoteID;
    double beginTime;

    string dump() {
        return json(*this).dump();
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