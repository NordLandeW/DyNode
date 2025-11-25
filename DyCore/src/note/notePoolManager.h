#pragma once
#include <memory_resource>
#include <shared_mutex>
#include <string>

#include "activation.h"
#include "note.h"

inline constexpr int NOTES_ARRAY_PARALLEL_SORT_THRESHOLD = 10000;

class NotePoolManager {
    friend NoteActivationManager;

   public:
    using nptr = std::shared_ptr<Note>;

    NotePoolManager();
    ~NotePoolManager();

    NotePoolManager operator=(const NotePoolManager &other) = delete;

    bool note_exists(const std::string &noteID);
    bool create_note(const Note &note);
    const Note &get_note(const std::string &noteID);
    const Note &get_note(int index) {
        return operator[](index);
    }
    const Note &get_note_unsafe(const std::string &noteID) {
        return *get_note_pointer(noteID);
    }
    void get_notes(std::vector<Note> &outNotes, bool excludeSub) const;
    /// Returns a direct reference to the note at the given index.
    /// This is unsafe and should only be used when you are sure the index is
    /// valid.
    const Note &get_note_direct(int index);
    void set_note(const Note &note);
    void set_note_bitwise(const std::string &noteID, const char *prop);
    void access_note(const std::string &noteID,
                     std::function<void(Note &)> executor);
    void access_all_notes(std::function<void(Note &)> executor);
    void access_all_notes_safe(std::function<void(Note &)> executor);
    void access_all_notes_parallel(std::function<void(Note &)> executor);
    void access_all_notes_parallel_safe(std::function<void(Note &)> executor);
    void sync_head_note_to_sub(const Note &note);
    void sync_hold_note_length(const Note &note);

    int get_index(const std::string &noteID);
    bool release_note(std::string noteID);
    bool release_note(const Note &note);
    void clear_notes();
    bool array_sort_request();
    int get_index_upperbound(double time);
    int get_index_lowerbound(double time);

    const Note &operator[](int index);

   protected:
    std::vector<nptr> noteArray, holdArray;

   private:
    struct NoteMemoryInfo {
        std::pmr::list<nptr>::iterator iter;
        nptr pointer;
        int index, holdIndex;
    };

    void set_ooo();
    void unset_ooo();
    void array_markdel_index(const NoteMemoryInfo &info);
    void array_sort();
    void reclaim_memory();
    nptr get_note_pointer(const std::string &noteID);

    std::array<std::byte, 64 * 1024 * 1024> initial_buffer;
    std::pmr::monotonic_buffer_resource monotonic_res;
    std::pmr::unsynchronized_pool_resource pool_res;
    std::pmr::list<nptr> noteMemoryList;

    std::unordered_map<std::string, NoteMemoryInfo> noteInfoMap;
    mutable std::shared_mutex mtxNoteOps;
    bool arrayOutOfOrder = false;
    int noteCount = 0;

   public:
    bool is_ooo() {
        return arrayOutOfOrder;
    }
    bool clear_ooo() {
        return array_sort_request();
    }
    int get_note_count() {
        return noteCount;
    }
};

NotePoolManager &get_note_pool_manager();
