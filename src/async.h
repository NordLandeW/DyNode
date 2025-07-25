#pragma once
#include <json.hpp>
#include <mutex>
#include <string>
#include <vector>

using nlohmann::json;
using std::string;

enum ASYNC_EVENT_TYPE { PROJECT_SAVING, GENERAL_ERROR };

struct AsyncEvent {
    ASYNC_EVENT_TYPE type;
    int status;
    string message;
};
inline void to_json(json &j, const AsyncEvent &a) {
    j = json{{"type", a.type}, {"status", a.status}, {"message", a.message}};
}

extern std::vector<AsyncEvent> asyncEventStack;
extern std::mutex mtxSaveProject;

void throw_error_event(std::string error_info);
void push_async_event(AsyncEvent asyncEvent);