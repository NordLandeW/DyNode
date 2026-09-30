#pragma once
#include <atomic>
#include <condition_variable>
#include <memory_resource>
#include <mutex>
#include <shared_mutex>
#include <string>

#include "activation.h"
#include "note.h"

inline constexpr int NOTES_ARRAY_PARALLEL_SORT_THRESHOLD = 10000;

namespace tf {
class Executor;
class Taskflow;
}  // namespace tf

class NotePoolManager {
    friend NoteActivationManager;

   public:
    using nptr = std::shared_ptr<Note>;

    explicit NotePoolManager(size_t workerCount = 0);
    ~NotePoolManager();

    // Shutdown rejects external submissions and drains submitted task graphs.
    // It must not be called from a note worker or while holding a note lock.
    void initialize_executor();
    void shutdown_executor();
    size_t executor_creation_count() const;

    NotePoolManager operator=(const NotePoolManager &other) = delete;

    bool note_exists(const std::string &noteID) const;
    bool create_note(const Note &note);
    // Copy the note while holding a shared lock.
    Note get_note(const std::string &noteID) const;
    Note get_note(int index) const {
        return operator[](index);
    }
    // The caller must exclude concurrent modification and keep the note alive.
    const Note &get_note_unsafe(const std::string &noteID) {
        return *get_note_pointer(noteID);
    }
    void get_notes(std::vector<Note> &outNotes, bool excludeSub) const;
    // Read by physical index without requiring the array to be sorted.
    Note get_note_direct(int index) const;
    void set_note(const Note &note);
    void set_note_bitwise(const char *prop);

    // Callbacks must not retain references or reenter locking APIs on this
    // manager. Such reentry throws std::logic_error before acquiring the lock.
    void read_all_notes(std::function<void(const Note &)> reader) const;
    void access_note(const std::string &noteID,
                     std::function<void(Note &)> executor);
    // Synchronize linked HOLD/SUB fields after each callback, before visiting
    // the next note. Partial edits are also synchronized if a callback throws.
    void access_all_notes(std::function<void(Note &)> executor);
    // The caller owns HOLD/SUB synchronization and must prevent different
    // workers from processing both ends of the same pair concurrently.
    // This traversal performs no automatic linked-note synchronization.
    // Callbacks must also synchronize their own shared captures.
    void access_all_notes_parallel(std::function<void(Note &)> executor);

    int get_index(const std::string &noteID);
    bool release_note(std::string noteID);
    bool release_note(const Note &note);
    void clear_notes();
    bool array_sort_request();
    int get_index_upperbound(double time);
    int get_index_lowerbound(double time);

    Note operator[](int index) const;

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
    nptr get_note_pointer(const std::string &noteID) const;
    std::shared_lock<std::shared_mutex> lock_shared() const;
    std::unique_lock<std::shared_mutex> lock_exclusive() const;
    void execute_note_callback(Note &note,
                               const std::function<void(Note &)> &executor);
    void edit_note(Note &note, const std::function<void(Note &)> &executor);
    void sync_head_note_to_sub(const Note &note);
    void sync_hold_note_length(const Note &note);

    void execute_tasks(tf::Taskflow &taskflow);
    tf::Executor &ensure_executor();
    std::unique_ptr<tf::Executor> noteExecutor;
    mutable std::mutex executorLifecycleMutex;
    std::condition_variable executorIdle;
    bool executorStopped = false;
    size_t activeExecutorCalls = 0;
    size_t executorCreationCount = 0;
    size_t executorWorkerCount = 0;  // 0 keeps hardware concurrency.

    std::array<std::byte, 64 * 1024 * 1024> initial_buffer;
    std::pmr::monotonic_buffer_resource monotonic_res;
    std::pmr::unsynchronized_pool_resource pool_res;
    std::pmr::list<nptr> noteMemoryList;

    std::unordered_map<std::string, NoteMemoryInfo> noteInfoMap;
    mutable std::shared_mutex mtxNoteOps;
    std::atomic<bool> arrayOutOfOrder = false;
    int noteCount = 0;

   public:
    bool is_ooo() {
        return arrayOutOfOrder;
    }
    bool clear_ooo() {
        return array_sort_request();
    }
    int get_note_count() const;
};

NotePoolManager &get_note_pool_manager();
