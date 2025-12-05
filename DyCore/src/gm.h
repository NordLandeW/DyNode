// gm.h: Bridge for asynchronous communication between DyCore and GameMaker.
//
// This header defines the data structures and functions used to pass events
// from DyCore to GameMaker. It utilizes a thread-safe event stack to handle
// asynchronous operations, allowing DyCore to perform tasks in the background
// and notify GameMaker upon completion.

#pragma once
#include <json.hpp>
#include <mutex>
#include <string>
#include <vector>

using nlohmann::json;
using std::string;

// Represents the parameters for saving a project.
// PROJECT_SAVING: Called when an async project event is done.
// GENERAL_ERROR: Called when an error occurs.
// GM_ANNOUNCEMENT: Call GM announcement function.
// ON_FILES_DROPPED: Called when files are dropped into the window.
enum ASYNC_EVENT_TYPE {
    PROJECT_SAVING,
    GENERAL_ERROR,
    GM_ANNOUNCEMENT,
    ON_FILES_DROPPED
};

struct AsyncEvent {
    ASYNC_EVENT_TYPE type;
    int status;
    string content;
};
inline void to_json(json &j, const AsyncEvent &a) {
    j = json{{"type", a.type}, {"status", a.status}, {"content", a.content}};
}

extern std::vector<AsyncEvent> asyncEventStack;
extern std::mutex mtxSaveProject;

void throw_error_event(std::string error_info);
void push_async_event(AsyncEvent asyncEvent);

enum class GM_ANNOUNCEMENT_TYPE { ANNO_INFO, ANNO_WARNING, ANNO_ERROR };

void gamemaker_announcement(GM_ANNOUNCEMENT_TYPE type, string message,
                            std::vector<string> args = {});