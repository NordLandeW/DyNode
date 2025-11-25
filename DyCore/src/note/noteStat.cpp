
#include <array>
#include <json.hpp>
#include <string>

#include "api.h"
#include "note.h"
#include "notePoolManager.h"
#include "profile.h"

using json = nlohmann::json;

DYCORE_API const char *DyCore_note_count() {
    PROFILE_SCOPE("DyCore_note_count");
    auto &noteMan = get_note_pool_manager();
    // 3 Types + Total, 3 Sides + Total
    std::array<std::array<int, 4>, 4> counts = {};
    noteMan.access_all_notes([&](Note &note) {
        if (note.get_note_type() == NOTE_TYPE::SUB)
            return;
        counts[note.side][note.type]++;
        counts[note.side][3] +=
            1 + (note.get_note_type() == NOTE_TYPE::HOLD ? 1 : 0);
        counts[3][note.type]++;
        counts[3][3] += 1 + (note.get_note_type() == NOTE_TYPE::HOLD ? 1 : 0);
    });

    json js = counts;
    static std::string result;
    result = js.dump();

    return result.c_str();
}

/// Caculate the avg notes' count between [_time-_range, _time] (in ms)
DYCORE_API double DyCore_kps_count(double time, double range) {
    PROFILE_SCOPE("DyCore_kps_count");
    auto &noteMan = get_note_pool_manager();
    if (noteMan.get_note_count() == 0) {
        return 0.0;
    }

    noteMan.array_sort_request();

    double ub = noteMan.get_index_upperbound(time);
    double lb = noteMan.get_index_lowerbound(time - range);

    return (ub - lb) * 1000.0 / range;
}