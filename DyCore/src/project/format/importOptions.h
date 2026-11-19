#pragma once

struct ChartImportOptions {
    bool clearNotes = false;
    bool resetTiming = false;
    // A side-loaded chart starts a new editor session, invalidating unread
    // saves.
    bool newProject = false;
};
