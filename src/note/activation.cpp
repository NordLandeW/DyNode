#include "activation.h"

#include "bitio.h"
#include "layout.h"
#include "notePoolManager.h"

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
    activeNotes.clear();

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
        } else {
            activeNotes.push_back(
                {noteArray[i]->beginTime, noteArray[i]->subNoteID});
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
        }
    }

    // Remove duplicates
    std::sort(activeNotes.begin(), activeNotes.end());
    activeNotes.erase(std::unique(activeNotes.begin(), activeNotes.end()),
                      activeNotes.end());

    // Sort based on time
    std::sort(activeNotes.begin(), activeNotes.end(),
              [&](const std::pair<double, std::string>& a,
                  const std::pair<double, std::string>& b) {
                  return a.first < b.first;
              });
}

void NoteActivationManager::bitwrite_active_notes(char* buffer) const {
    char* ptr = buffer;
    bitwrite<int>(ptr, activeNotes.size());
    for (const auto& note : activeNotes) {
        bitwrite<string>(ptr, note.second);
    }
}

NoteActivationManager& get_note_activation_manager() {
    static NoteActivationManager instance;
    return instance;
}
