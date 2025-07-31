
#include <array>
#include <json.hpp>
#include <string>

#include "api.h"
#include "note.h"
#include "singletons.h"

using json = nlohmann::json;

DYCORE_API const char *DyCore_note_count() {
    auto &noteMan = get_note_pool_manager();
    // 3 Types + Total, 3 Sides + Total
    std::array<std::array<std::atomic<int>, 4>, 4> counts = {};
    noteMan.access_all_notes_parallel([&](Note &note) {
        if (note.noteType > 2)
            return;
        counts[note.side][note.noteType]++;
        counts[note.side][3] += 1 + (note.noteType == 2 ? 1 : 0);
        counts[3][note.noteType]++;
        counts[3][3] += 1 + (note.noteType == 2 ? 1 : 0);
    });

    std::array<std::array<int, 4>, 4> final_counts;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            final_counts[i][j] = counts[i][j].load(std::memory_order_relaxed);
        }
    }

    json js = final_counts;
    static std::string result;
    result = js.dump();

    return result.c_str();
}