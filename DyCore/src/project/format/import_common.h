#pragma once

#include <unordered_map>
#include <vector>

#include "note.h"
#include "timing.h"

struct ImportedNoteData {
    std::string id;
    std::string subid;
    int type = 0;
    int side = 0;
    double bar = 0.0;
    double position = 0.0;
    double width = 0.0;
    double time = 0.0;
};

struct ImportedTimingData {
    double time = 0.0;
    double barPerMinute = 0.0;
};

inline double imported_bar_to_time(double offset, double barPerMinute) {
    return (offset * 60000.0) / barPerMinute;
}

inline void import_timing_points(bool importTiming, bool hasTimingData,
                                 const std::vector<ImportedTimingData>& timings,
                                 double offset, double barPerMin) {
    if (!importTiming) {
        return;
    }

    auto& timingMan = get_timing_manager();
    timingMan.clear();

    if (hasTimingData) {
        double runningTime = imported_bar_to_time(-offset, barPerMin);

        for (int i = 0; i < timings.size(); i++) {
            double timingPointTime = timings[i].time;
            if (i > 0) {
                timingPointTime =
                    imported_bar_to_time(timingPointTime - timings[i - 1].time,
                                         timings[i - 1].barPerMinute) +
                    runningTime;
            } else {
                timingPointTime = runningTime;
            }

            runningTime = timingPointTime;

            TimingPoint tp;
            tp.meter = 4;
            tp.time = timingPointTime;
            tp.set_bpm(timings[i].barPerMinute * 4);
            timingMan.add_timing_point(tp);
        }
    } else {
        timingMan.add_timing_point({imported_bar_to_time(-offset, barPerMin),
                                    60000.0 / (barPerMin * 4), 4});
    }

    timingMan.sort();
}

inline void fix_imported_note_times(
    std::vector<ImportedNoteData>& notes,
    const std::vector<ImportedTimingData>& timings, double offset,
    double barPerMin) {
    const double fixedOffset = imported_bar_to_time(offset, barPerMin);

    for (auto& note : notes) {
        if (timings.size() > 1) {
            const double noteBar = note.bar;
            double runningTime = 0.0;

            for (int i = 1, l = timings.size(); i <= l; i++) {
                if (i == l || timings[i].time > noteBar) {
                    runningTime +=
                        imported_bar_to_time(noteBar - timings[i - 1].time,
                                             timings[i - 1].barPerMinute);
                    break;
                }

                runningTime +=
                    imported_bar_to_time(timings[i].time - timings[i - 1].time,
                                         timings[i - 1].barPerMinute);
            }

            note.time = runningTime;
        } else {
            note.time = imported_bar_to_time(note.bar, barPerMin);
        }

        note.time -= fixedOffset;
    }
}

inline std::unordered_map<std::string, double> build_note_id_time_map(
    const std::vector<ImportedNoteData>& notes) {
    std::unordered_map<std::string, double> noteIDTimeMap;
    for (const auto& note : notes) {
        noteIDTimeMap[note.id] = note.time;
    }
    return noteIDTimeMap;
}

inline void add_imported_notes_to_project(
    const std::vector<ImportedNoteData>& notes,
    const std::unordered_map<std::string, double>& noteIDTimeMap) {
    for (const auto& note : notes) {
        if (note.type == 3) {
            continue;
        }

        Note newNote;
        newNote.time = note.time;
        newNote.side = note.side;
        newNote.lastTime = 0;
        newNote.width = note.width;
        newNote.position = note.position;
        newNote.type = note.type;
        newNote.beginTime = note.time;

        if (note.type == 2) {
            newNote.lastTime = noteIDTimeMap.at(note.subid) - note.time;
        }

        create_note(newNote);
    }
}