#pragma once
#include <string>
#include <utility>
#include <vector>

#include "note.h"

class NoteActivationManager {
   private:
    double currentTime;
    double noteSpeed;
    std::pair<double, double> timeRange, timeRangeMin;

    std::vector<std::pair<double, std::string>> activeNotes, activeHolds,
        lastingHolds;

   public:
    // Set the range settings.
    void set_range(double curTime, double curSpeed);

    // Recalculate the active notes based on current range setting.
    void recalculate();

    using ActiveLists = std::vector<std::pair<double, std::string>>;
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