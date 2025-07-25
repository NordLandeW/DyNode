
#include "async.h"
#include "api.h"
#include "utils.h"

std::vector<AsyncEvent> asyncEventStack;
std::mutex mtxSaveProject;
std::mutex mtxAsyncEvents;

void throw_error_event(std::string error_info) {
    std::lock_guard<std::mutex> lock(mtxAsyncEvents);
    asyncEventStack.push_back({GENERAL_ERROR, -1, error_info});
}
void push_async_event(AsyncEvent asyncEvent) {
    std::lock_guard<std::mutex> lock(mtxAsyncEvents);
    asyncEventStack.push_back(asyncEvent);
}

DYCORE_API double DyCore_has_async_event() {
    std::lock_guard<std::mutex> lock(mtxAsyncEvents);
    return asyncEventStack.size() > 0;
}

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
