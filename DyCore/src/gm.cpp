
#include "gm.h"

#include "api.h"
#include "utils.h"

std::vector<AsyncEvent> asyncEventStack;
std::mutex mtxAsyncEvents;

// Pushes a general error event to the asynchronous event stack.
void throw_error_event(std::string error_info) {
    std::lock_guard<std::mutex> lock(mtxAsyncEvents);
    asyncEventStack.push_back({GENERAL_ERROR, -1, error_info});
}

// Pushes an asynchronous event to the event stack.
void push_async_event(AsyncEvent asyncEvent) {
    std::lock_guard<std::mutex> lock(mtxAsyncEvents);
    asyncEventStack.push_back(asyncEvent);
}

// Checks if there are any pending asynchronous events.
// Returns 1.0 if there are events, 0.0 otherwise.
DYCORE_API double DyCore_has_async_event() {
    std::lock_guard<std::mutex> lock(mtxAsyncEvents);
    return asyncEventStack.size() > 0;
}

// Retrieves the latest asynchronous event from the stack as a JSON string.
// The event is removed from the stack after retrieval.
// Returns an empty string if no events are available.
DYCORE_API const char* DyCore_get_async_event() {
    std::lock_guard<std::mutex> lock(mtxAsyncEvents);
    if (asyncEventStack.size() == 0)
        return "";
    static string result = "";
    try {
        json j = asyncEventStack.back();
        asyncEventStack.pop_back();
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

void gamemaker_announcement(GM_ANNOUNCEMENT_TYPE type, string message,
                            std::vector<string> args) {
    json j = json::object();
    j["msg"] = message;
    j["args"] = args;
    AsyncEvent event = {GM_ANNOUNCEMENT, (int)type, j.dump()};
    push_async_event(event);
    return;
}
