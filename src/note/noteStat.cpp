
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

/// Caculate the avg notes' count between [_time-_range, _time] (in ms)
DYCORE_API double DyCore_kps_count(double time, double range) {
    auto &noteMan = get_note_pool_manager();
    if (noteMan.get_note_count() == 0) {
        return 0.0;
    }

    double ub = noteMan.get_index_upperbound(time);
    double lb = noteMan.get_index_lowerbound(time - range);

    return (ub - lb) * 1000.0 / range;
}