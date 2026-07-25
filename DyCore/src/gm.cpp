
#include "gm.h"

#include <stdexcept>
#include <utility>

#include "api.h"
#include "utils.h"

std::queue<AsyncEvent> asyncEventQueue;
std::mutex mtxAsyncEvents;

// Pushes a general error event to the asynchronous event queue.
void throw_error_event(std::string error_info) {
    std::lock_guard<std::mutex> lock(mtxAsyncEvents);
    asyncEventQueue.push({GENERAL_ERROR, -1, error_info});
}

// Pushes an asynchronous event to the event queue.
void push_async_event(AsyncEvent asyncEvent) {
    std::lock_guard<std::mutex> lock(mtxAsyncEvents);
    asyncEventQueue.push(asyncEvent);
}

// Checks if there are any pending asynchronous events.
// Returns 1.0 if there are events, 0.0 otherwise.
DYCORE_API double DyCore_has_async_event() {
    std::lock_guard<std::mutex> lock(mtxAsyncEvents);
    return !asyncEventQueue.empty();
}

// Retrieves the oldest asynchronous event from the queue as a JSON string.
// The event is removed from the queue after retrieval.
// Returns an empty string if no events are available.
DYCORE_API const char* DyCore_get_async_event() {
    std::lock_guard<std::mutex> lock(mtxAsyncEvents);
    if (asyncEventQueue.empty())
        return "";
    static string result = "";
    try {
        json j = asyncEventQueue.front();
        asyncEventQueue.pop();
        result = nlohmann::to_string(j);
        return result.c_str();
    } catch (json::exception& e) {
        print_debug_message("Async events stringify failed:" +
                            string(e.what()));
        return "";
    } catch (std::exception& e) {
        print_debug_message("Async events unknown error:" + string(e.what()));
        return "";
    }
}

/// Sends a GameMaker announcement event with the specified type, message,
/// and optional arguments to fill in the i18n placeholders.
void gamemaker_announcement(GM_ANNOUNCEMENT_TYPE type, string message,
                            std::vector<string> args, int lastTime) {
    try {
        json j = json::object();
        j["msg"] = message;
        j["args"] = args;
        if (lastTime >= 0) {
            j["lastTime"] = lastTime;
        }
        AsyncEvent event = {
            GM_ANNOUNCEMENT, (int)type,
            j.dump(-1, ' ', false, json::error_handler_t::replace)};
        push_async_event(event);
    } catch (const std::exception& e) {
        print_debug_message("GameMaker announcement failed: " +
                            string(e.what()));
        push_async_event(
            {GM_ANNOUNCEMENT, (int)GM_ANNOUNCEMENT_TYPE::ANNO_ERROR,
             R"({"msg":"anno_dycore_error","args":["Failed to create announcement."]})"});
    }
    return;
}

void gamemaker_execute(string functionName, json args) {
    if (!args.is_array()) {
        throw std::invalid_argument(
            "GameMaker execute arguments must be an array.");
    }

    json content = {
        {"name", std::move(functionName)},
        {"args", std::move(args)},
    };
    push_async_event(
        {GM_EXECUTE, 0,
         content.dump(-1, ' ', false, json::error_handler_t::replace)});
}
