
#include <json.hpp>
#include <string>

#include "api.h"
#include "note.h"
#include "notePoolManager.h"
#include "utils.h"

DYCORE_API double DyCore_note_add_offset(double time) {
    auto &noteMan = get_note_pool_manager();
    try {
        noteMan.access_all_notes([time](Note &note) { note.time += time; });
    } catch (const std::exception &e) {
        print_debug_message("Error: " + std::string(e.what()));
        return -1;
    }
    return 0;
}