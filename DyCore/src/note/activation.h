#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "layout.h"
#include "note.h"

class NotePoolManager;

class NoteActivationManager {
   public:
    // Own the ID. A physical index is only a lookup hint, never a note
    // lifetime.
    struct Entry {
        double time;
        std::string id;
        int index;
    };
    using ActiveLists = std::vector<Entry>;

   private:
    struct Candidate {
        double time;
        std::string_view id;
        int index;
        bool hold, lasting;
    };
    std::vector<Candidate> heads, extra;
    PlaybackFrame frame{};
    size_t revision = 0;
    size_t activeBytes = sizeof(int);

    ActiveLists activeNotes, activeHolds, lastingHolds;

    void collect_upcoming(NotePoolManager& pool, int first, double frontEnd,
                          double sideEnd);
    void collect_long_holds(const NotePoolManager& pool, double minEnd);
    void collect_visual_tails(NotePoolManager& pool, int firstUpcoming);
    void merge_candidates();
    void emit_candidate(const Candidate& candidate);

   public:
    // Set the range settings.
    void set_range(double curTime, double curSpeed) {
        set_range({curTime, curSpeed, curSpeed, false});
    }
    void set_range(PlaybackFrame settings) {
        frame = settings;
    }

    // Recalculate the active notes based on current range setting.
    void recalculate();

    // Clear all active notes and release memory.
    void clear();

    size_t get_revision() const {
        return revision;
    }
    // Get the currently active notes.
    const ActiveLists& get_active_notes() const {
        return activeNotes;
    }
    const ActiveLists& get_active_holds() const {
        return activeHolds;
    }
    const ActiveLists& get_lasting_holds() const {
        return lastingHolds;
    }
    void bitwrite_active_notes(char* buffer) const;
    void bitwrite_lasting_holds(char* buffer) const;
    size_t get_bitwrite_bound() const {
        return activeBytes;
    }

    NoteActivationManager operator=(const NoteActivationManager& other) =
        delete;
};

NoteActivationManager& get_note_activation_manager();