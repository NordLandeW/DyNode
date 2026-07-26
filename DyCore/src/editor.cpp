#include "editor.h"

GMEditorManager& GMEditorManager::inst() {
    static GMEditorManager inst;
    return inst;
}

int GMEditorManager::get_editmode() const {
    std::lock_guard<std::mutex> lock(mutex);
    return editMode;
}

bool GMEditorManager::is_ready() const {
    std::lock_guard<std::mutex> lock(mutex);
    return ready > 0;
}

void GMEditorManager::set_ready() {
    std::lock_guard<std::mutex> lock(mutex);
    ++ready;
}

void GMEditorManager::unset_ready() {
    std::lock_guard<std::mutex> lock(mutex);
    if (ready > 0) {
        --ready;
    }
}

void GMEditorManager::sync_states(const nlohmann::json& states) {
    const int nextEditMode = states.at("editMode").get<int>();

    std::lock_guard<std::mutex> lock(mutex);
    editMode = nextEditMode;
}

bool gmeditor_is_ready() {
    return GMEditorManager::inst().is_ready();
}

void gmeditor_set_ready(bool flag) {
    if (flag) {
        GMEditorManager::inst().set_ready();
    } else {
        GMEditorManager::inst().unset_ready();
    }
}

void gmeditor_sync_states(const nlohmann::json& states) {
    GMEditorManager::inst().sync_states(states);
}
