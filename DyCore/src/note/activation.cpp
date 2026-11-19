#include "activation.h"

#include "bitio.h"
#include "layout.h"
#include "notePoolManager.h"
#include "profile.h"

void NoteActivationManager::recalculate() {
    PROFILE_STATIC_SCOPE("Note Activation Manager Recalculate");
    ++revision;

    activeNotes.clear();
    activeHolds.clear();
    lastingHolds.clear();

    auto& poolMan = get_note_pool_manager();
    poolMan.array_sort_request();
    auto& noteArray = poolMan.noteArray;

    // Check normal notes
    const double currentTime = frame.time;
    const double frontTravel = BASE_RES_H - JUDGE_LINE_BELOW_FROM_BOTTOM;
    const double sideTravel = BASE_RES_W / 2.0 - JUDGE_LINE_SIDE_FROM_EDGE;
    const double frontEnd =
        currentTime +
        (frontTravel + ACTIVATION_AHEAD_PIXELS) / frame.frontSpeed;
    const double sideEnd = currentTime + sideTravel / frame.sideSpeed;
    const double minEnd = currentTime + std::min(frontTravel / frame.frontSpeed,
                                                 sideTravel / frame.sideSpeed);
    int lb = poolMan.get_index_lowerbound(currentTime);
    int hb = poolMan.get_index_upperbound(std::max(frontEnd, sideEnd));
    for (int i = lb; i < hb; i++) {
        // Use strict condition for LR side notes.
        if (noteArray[i]->time >
                (noteArray[i]->side == 0 ? frontEnd : sideEnd) ||
            (frame.dynamite && noteArray[i]->side > 0 &&
             noteArray[i]->time == sideEnd &&
             noteArray[i]->get_note_type() != NOTE_TYPE::SUB))
            continue;
        if (noteArray[i]->get_note_type() <= NOTE_TYPE::HOLD) {
            activeNotes.push_back({noteArray[i]->time, noteArray[i]->noteID});
            if (noteArray[i]->get_note_type() == NOTE_TYPE::HOLD) {
                activeHolds.push_back(
                    {noteArray[i]->time, noteArray[i]->noteID});
            }
        } else {
            activeNotes.push_back(
                {noteArray[i]->beginTime, noteArray[i]->subNoteID});
            activeHolds.push_back(
                {noteArray[i]->beginTime, noteArray[i]->subNoteID});
            if (noteArray[i]->beginTime < currentTime &&
                (!frame.dynamite || noteArray[i]->time > currentTime)) {
                lastingHolds.push_back(
                    {noteArray[i]->beginTime, noteArray[i]->subNoteID});
            }
        }
    }

    // Check long holds.
    for (const auto& note_ptr : poolMan.holdArray) {
        const auto& note = *note_ptr;
        if (note.lastTime < minEnd - currentTime)
            break;
        if (note.time <= currentTime && note.time + note.lastTime > minEnd) {
            activeNotes.push_back({note.time, note.noteID});
            lastingHolds.push_back({note.time, note.noteID});
            activeHolds.push_back({note.time, note.noteID});
        }
    }

    // A short HOLD can outlive its chart endpoint. Find those heads even after
    // a seek, without feeding a visual-only tail to the particle list.
    if (frame.dynamite) {
        const double minimum[] = {dynamite_min_duration(frame.frontSpeed, 0),
                                  dynamite_min_duration(frame.sideSpeed, 1)};
        const int first = poolMan.get_index_lowerbound(
            currentTime - std::max(minimum[0], minimum[1]));
        for (int i = first; i < lb; ++i) {
            const auto& note = *noteArray[i];
            if (note.get_note_type() == NOTE_TYPE::HOLD &&
                note.time + note.lastTime <= currentTime &&
                note.time + minimum[note.side != 0] > currentTime) {
                activeNotes.emplace_back(note.time, note.noteID);
                activeHolds.emplace_back(note.time, note.noteID);
            }
        }
    }

    // Remove duplicates
    static auto unique_and_sort = [](auto& vec) {
        std::sort(vec.begin(), vec.end());
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    };

    unique_and_sort(activeNotes);
    unique_and_sort(activeHolds);
    unique_and_sort(lastingHolds);
}

void NoteActivationManager::clear() {
    ++revision;
    activeNotes.clear();
    activeNotes.shrink_to_fit();
    activeHolds.clear();
    activeHolds.shrink_to_fit();
    lastingHolds.clear();
    lastingHolds.shrink_to_fit();
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
