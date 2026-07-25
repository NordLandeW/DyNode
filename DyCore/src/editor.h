#pragma once

#include "json.hpp"

#include <mutex>

class GMEditorManager {
    mutable std::mutex mutex;
    int ready = 0;
    int editMode = 0;

   public:
    static GMEditorManager& inst();

    // Setters - getters
   public:
    int get_editmode() const;
    bool is_ready() const;
    void set_ready();
    void unset_ready();
    void sync_states(const nlohmann::json& states);
};

bool gmeditor_is_ready();
void gmeditor_set_ready(bool flag);
void gmeditor_sync_states(const nlohmann::json& states);
