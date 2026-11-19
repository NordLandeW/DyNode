#include "activation.h"

#include <algorithm>
#include <tuple>

#include "bitio.h"
#include "layout.h"
#include "notePoolManager.h"
#include "profile.h"

void NoteActivationManager::recalculate() {
    PROFILE_STATIC_SCOPE("Note Activation Manager Recalculate");
    ++revision;

    heads.clear();
    extra.clear();
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
            const auto& n = *noteArray[i];
            heads.push_back({n.time, n.noteID, i,
                             n.get_note_type() == NOTE_TYPE::HOLD, false});
        } else {
            const auto& n = *noteArray[i];
            extra.push_back({n.beginTime, n.subNoteID, -1, true,
                             n.beginTime < currentTime &&
                                 (!frame.dynamite || n.time > currentTime)});
        }
    }

    // Check long holds.
    for (const auto& note_ptr : poolMan.holdArray) {
        const auto& note = *note_ptr;
        if (note.lastTime < minEnd - currentTime)
            break;
        if (note.time <= currentTime && note.time + note.lastTime > minEnd) {
            extra.push_back({note.time, note.noteID, note.index, true, true});
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
                extra.push_back({note.time, note.noteID, i, true, false});
            }
        }
    }

    // Heads already arrive in chronological order. Only ties and supplementary
    // HOLD sources need sorting; merge once for all three public lists.
    auto less = [](const Candidate& a, const Candidate& b) {
        return std::tie(a.time, a.id) < std::tie(b.time, b.id);
    };
    for (auto it = heads.begin(); it != heads.end();) {
        auto end = it + 1;
        while (end != heads.end() && it->time == end->time)
            ++end;
        if (end - it > 1)
            std::sort(it, end, less);
        it = end;
    }
    std::sort(extra.begin(), extra.end(), less);
    auto emit = [](ActiveLists& list, const Candidate& c) {
        if (list.empty() || list.back().time != c.time ||
            list.back().id != c.id)
            list.push_back({c.time, std::string(c.id), c.index});
        else if (c.index >= 0)
            list.back().index = c.index;
    };
    activeBytes = sizeof(int);
    auto head = heads.begin();
    auto other = extra.begin();
    while (head != heads.end() || other != extra.end()) {
        const Candidate& c = other == extra.end() || (head != heads.end() &&
                                                      !less(*other, *head))
                                 ? *head++
                                 : *other++;
        const auto count = activeNotes.size();
        emit(activeNotes, c);
        if (activeNotes.size() != count)
            activeBytes += c.id.size() + 1;
        if (c.hold)
            emit(activeHolds, c);
        if (c.lasting)
            emit(lastingHolds, c);
    }
    heads.clear();
    extra.clear();
}

void NoteActivationManager::clear() {
    ++revision;
    activeBytes = sizeof(int);
    heads.clear();
    extra.clear();
    heads.shrink_to_fit();
    extra.shrink_to_fit();
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
        bitwrite<string>(ptr, note.id);
    }
}

void NoteActivationManager::bitwrite_lasting_holds(char* buffer) const {
    char* ptr = buffer;
    bitwrite<int>(ptr, lastingHolds.size());
    for (const auto& note : lastingHolds) {
        bitwrite<string>(ptr, note.id);
    }
}

NoteActivationManager& get_note_activation_manager() {
    static NoteActivationManager instance;
    return instance;
}
