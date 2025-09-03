#include "activation.h"

#include "bitio.h"
#include "layout.h"
#include "notePoolManager.h"
#include "profile.h"

void NoteActivationManager::set_range(double curTime, double curSpeed) {
    currentTime = curTime;
    noteSpeed = curSpeed;

    timeRange = {
        currentTime,
        currentTime + (ACTIVATION_AHEAD_PIXELS +
                       std::max(BASE_RES_H - JUDGE_LINE_BELOW_FROM_BOTTOM,
                                BASE_RES_W / 2 - JUDGE_LINE_SIDE_FROM_EDGE)) /
                          noteSpeed};
    timeRangeMin = {
        currentTime,
        currentTime + std::min(BASE_RES_H - JUDGE_LINE_BELOW_FROM_BOTTOM,
                               BASE_RES_W / 2 - JUDGE_LINE_SIDE_FROM_EDGE) /
                          noteSpeed};
}

void NoteActivationManager::recalculate() {
    PROFILE_SCOPE("Note Activation Manager Recalculate");

    activeNotes.clear();
    activeHolds.clear();
    lastingHolds.clear();

    auto& poolMan = get_note_pool_manager();
    poolMan.array_sort_request();
    auto& noteArray = poolMan.noteArray;

    // Check normal notes
    int lb = poolMan.get_index_lowerbound(timeRange.first);
    int hb = poolMan.get_index_upperbound(timeRange.second);
    for (int i = lb; i < hb; i++) {
        // Use strict condition for LR side notes.
        if (noteArray[i]->side > 0 &&
            noteArray[i]->time >
                timeRange.first +
                    (BASE_RES_W / 2.0 - JUDGE_LINE_SIDE_FROM_EDGE) / noteSpeed)
            continue;
        if (noteArray[i]->type <= 2) {
            activeNotes.push_back({noteArray[i]->time, noteArray[i]->noteID});
            if (noteArray[i]->type == 2) {
                activeHolds.push_back(
                    {noteArray[i]->time, noteArray[i]->noteID});
            }
        } else {
            activeNotes.push_back(
                {noteArray[i]->beginTime, noteArray[i]->subNoteID});
            activeHolds.push_back(
                {noteArray[i]->beginTime, noteArray[i]->subNoteID});
            if (noteArray[i]->beginTime < currentTime) {
                lastingHolds.push_back(
                    {noteArray[i]->beginTime, noteArray[i]->subNoteID});
            }
        }
    }

    // Check long holds.
    for (const auto& note_ptr : poolMan.holdArray) {
        const auto& note = *note_ptr;
        if (note.lastTime < timeRangeMin.second - timeRangeMin.first)
            break;
        if (note.time <= timeRangeMin.first &&
            note.time + note.lastTime > timeRangeMin.second) {
            activeNotes.push_back({note.time, note.noteID});
            lastingHolds.push_back({note.time, note.noteID});
            activeHolds.push_back({note.time, note.noteID});
        }
    }

    // Remove duplicates
    static auto unique_and_sort = [](auto& vec) {
        std::sort(vec.begin(), vec.end());
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
        std::sort(vec.begin(), vec.end(),
                  [&](const std::pair<double, std::string>& a,
                      const std::pair<double, std::string>& b) {
                      return a.first < b.first;
                  });
    };

    unique_and_sort(activeNotes);
    unique_and_sort(activeHolds);
    unique_and_sort(lastingHolds);
}

void NoteActivationManager::bitwrite_active_notes(char* buffer) const {
    char* ptr = buffer;
    bitwrite<int>(ptr, activeNotes.size());
    for (const auto& note : activeNotes) {
        bitwrite<string>(ptr, note.second);
    }
}

void NoteActivationManager::bitwrite_lasting_holds(char* buffer) const {
    char* ptr = buffer;
    bitwrite<int>(ptr, lastingHolds.size());
    for (const auto& note : lastingHolds) {
        bitwrite<string>(ptr, note.second);
    }
}

NoteActivationManager& get_note_activation_manager() {
    static NoteActivationManager instance;
    return instance;
}
