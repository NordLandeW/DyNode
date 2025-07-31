
#include <array>
#include <chrono>
#include <cstdio>
#include <json.hpp>
#include <string>

#include "api.h"
#include "note.h"
#include "singletons.h"

using json = nlohmann::json;

DYCORE_API const char *DyCore_note_count() {
    auto &noteMan = get_note_pool_manager();
    // 3 Types + Total, 3 Sides + Total
    std::array<std::array<int, 4>, 4> counts = {};
    noteMan.access_all_notes([&](Note &note) {
        if (note.noteType > 2)
            return;
        counts[note.side][note.noteType]++;
        counts[note.side][3] += 1 + (note.noteType == 2 ? 1 : 0);
        counts[3][note.noteType]++;
        counts[3][3] += 1 + (note.noteType == 2 ? 1 : 0);
    });

    json js = counts;
    static std::string result;
    result = js.dump();

    return result.c_str();
}
