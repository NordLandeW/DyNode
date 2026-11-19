#pragma once
#include <string>
#include <utility>
#include <vector>

#include "layout.h"
#include "note.h"

class NoteActivationManager {
   private:
    PlaybackFrame frame{};
    size_t revision = 0;

    std::vector<std::pair<double, std::string>> activeNotes, activeHolds,
        lastingHolds;

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

    using ActiveLists = std::vector<std::pair<double, std::string>>;
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
        return (NOTE_ID_LENGTH + 1) * activeNotes.size();
    }

    NoteActivationManager operator=(const NoteActivationManager& other) =
        delete;
};

NoteActivationManager& get_note_activation_manager();