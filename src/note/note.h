#pragma once

#include <mutex>
#include <string>

#include "json.hpp"

using nlohmann::json;
using std::string;

struct Note;
struct NoteExportView;

bool note_exists(const Note &note);
bool note_exists(const char *noteID);
bool note_exists(const string &noteID);
void clear_notes();
int insert_note(const Note &note);
int delete_note(const Note &note);
int delete_note(const std::string &noteID);
int modify_note(const Note &note);

void get_note_array(std::vector<Note> &notes);
void get_note_array(std::vector<NoteExportView> &notes);

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
    mutable std::mutex mtx;

    Note() = default;

    Note(const Note &other) {
        std::lock_guard<std::mutex> lock(other.mtx);

        // 直接从 'other' 拷贝数据
        side = other.side;
        noteType = other.noteType;
        time = other.time;
        width = other.width;
        position = other.position;
        lastTime = other.lastTime;
        beginTime = other.beginTime;
        noteID = other.noteID;
        subNoteID = other.subNoteID;
    }

    Note &operator=(const Note &other) {
        if (this == &other) {
            return *this;
        }
        std::scoped_lock lock(this->mtx, other.mtx);

        side = other.side;
        noteType = other.noteType;
        time = other.time;
        width = other.width;
        position = other.position;
        lastTime = other.lastTime;
        beginTime = other.beginTime;
        noteID = other.noteID;
        subNoteID = other.subNoteID;
        return *this;
    }

    string dump() const {
        return json(*this).dump();
    }

    size_t bitsize() {
        return sizeof(int) * 2 + sizeof(double) * 5 +
               sizeof(char) * (noteID.size() + subNoteID.size() + 4);
    }

    void bitwrite(char *buffer) {
        std::lock_guard<std::mutex> lock(mtx);
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
        std::lock_guard<std::mutex> lock(mtx);
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
    const Note &note;
    NoteExportView(const Note &n) : note(n) {
    }
};

inline void to_json(json &j, const NoteExportView &view) {
    std::lock_guard<std::mutex> lock(view.note.mtx);
    j = json{
        {"time", view.note.time},         {"side", view.note.side},
        {"width", view.note.width},       {"position", view.note.position},
        {"lastTime", view.note.lastTime}, {"noteType", view.note.noteType}};
}

inline void from_json(const json &j, Note &n) {
    std::lock_guard<std::mutex> lock(n.mtx);
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
    std::lock_guard<std::mutex> lock(n.mtx);
    j = json{{"time", n.time},          {"side", n.side},
             {"width", n.width},        {"position", n.position},
             {"lastTime", n.lastTime},  {"noteType", n.noteType},
             {"noteID", n.noteID},      {"subNoteID", n.subNoteID},
             {"beginTime", n.beginTime}};
}