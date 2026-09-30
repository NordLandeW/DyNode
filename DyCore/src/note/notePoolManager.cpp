#include "notePoolManager.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <taskflow/algorithm/for_each.hpp>
#include <taskflow/algorithm/sort.hpp>
#include <taskflow/taskflow.hpp>
#include <vector>

#include "note.h"
#include "notePoolManager.h"
#include "profile.h"
#include "taskflow/core/executor.hpp"
#include "utils.h"

namespace {
class NoteCallbackScope {
   public:
    explicit NoteCallbackScope(const NotePoolManager* manager,
                               const NoteCallbackScope* parent = current)
        : manager(manager), parent(parent), previous(current) {
        current = this;
    }
    ~NoteCallbackScope() {
        current = previous;
    }
    NoteCallbackScope(const NoteCallbackScope&) = delete;
    NoteCallbackScope& operator=(const NoteCallbackScope&) = delete;

    static const NoteCallbackScope* context() {
        return current;
    }

    static void check_reentry(const NotePoolManager* manager) {
        for (auto scope = current; scope; scope = scope->parent) {
            if (scope->manager == manager) {
                throw std::logic_error(
                    "Cannot reenter a locking note API from its callback");
            }
        }
    }

   private:
    const NotePoolManager* manager;
    const NoteCallbackScope* parent;
    const NoteCallbackScope* previous;
    static inline thread_local const NoteCallbackScope* current = nullptr;
};
}  // namespace

std::shared_lock<std::shared_mutex> NotePoolManager::lock_shared() const {
    NoteCallbackScope::check_reentry(this);
    return std::shared_lock(mtxNoteOps);
}

std::unique_lock<std::shared_mutex> NotePoolManager::lock_exclusive() const {
    NoteCallbackScope::check_reentry(this);
    return std::unique_lock(mtxNoteOps);
}

NotePoolManager::NotePoolManager(size_t workerCount)
    : executorWorkerCount(workerCount),
      monotonic_res(initial_buffer.data(), initial_buffer.size(),
                    std::pmr::new_delete_resource()),
      pool_res(&monotonic_res),
      arrayOutOfOrder(false) {
}

NotePoolManager::~NotePoolManager() {
    shutdown_executor();
}

// Requires executorLifecycleMutex.
tf::Executor& NotePoolManager::ensure_executor() {
    if (!noteExecutor) {
        const size_t workers =
            executorWorkerCount == 0
                ? static_cast<size_t>(std::max(1, hardware_concurrency()))
                : executorWorkerCount;
        noteExecutor = std::make_unique<tf::Executor>(workers);
        ++executorCreationCount;
    }
    return *noteExecutor;
}

void NotePoolManager::initialize_executor() {
    std::lock_guard lock(executorLifecycleMutex);
    if (executorStopped && activeExecutorCalls != 0) {
        throw std::logic_error("Note executor is still shutting down");
    }
    (void)ensure_executor();
    executorStopped = false;
}

size_t NotePoolManager::executor_creation_count() const {
    std::lock_guard lock(executorLifecycleMutex);
    return executorCreationCount;
}

void NotePoolManager::shutdown_executor() {
    std::unique_lock lock(executorLifecycleMutex);
    if (noteExecutor && noteExecutor->this_worker() != nullptr) {
        throw std::logic_error(
            "Cannot shut down note executor from its worker");
    }
    executorStopped = true;
    executorIdle.wait(lock, [&] { return activeExecutorCalls == 0; });
    noteExecutor.reset();
}

void NotePoolManager::execute_tasks(tf::Taskflow& taskflow) {
    tf::Executor* executor;
    {
        std::lock_guard lock(executorLifecycleMutex);
        if (executorStopped) {
            throw std::logic_error("Note executor has been shut down");
        }
        executor = &ensure_executor();
        ++activeExecutorCalls;
    }
    auto release = [&] {
        std::lock_guard lock(executorLifecycleMutex);
        --activeExecutorCalls;
        executorIdle.notify_all();
    };
    try {
        executor->run(taskflow).get();
    } catch (...) {
        release();
        throw;
    }
    release();
}

Note NotePoolManager::operator[](int index) const {
    auto lock = lock_shared();
    if (index < 0 || index >= noteArray.size())
        throw std::out_of_range(
            "Index out of range in NotePoolManager. Range: " +
            std::to_string(noteArray.size()) +
            ", requested: " + std::to_string(index));
    if (arrayOutOfOrder)
        throw std::runtime_error(
            "Note array is out of order. Use get_note_direct() instead.");

    return *noteArray[index];
}

bool NotePoolManager::note_exists(const std::string& noteID) const {
    auto lock = lock_shared();
    return noteInfoMap.find(noteID) != noteInfoMap.end();
}

int NotePoolManager::get_note_count() const {
    auto lock = lock_shared();
    return noteCount;
}

bool NotePoolManager::create_note(const Note& note) {
    auto lock = lock_exclusive();
    if (noteInfoMap.find(note.noteID) != noteInfoMap.end()) {
        return false;
    }
    try {
        std::pmr::polymorphic_allocator<Note> alloc(&pool_res);
        auto ptr = std::allocate_shared<Note>(alloc);

        *ptr = note;

        noteMemoryList.emplace_back(ptr);
        noteInfoMap[note.noteID] = {--noteMemoryList.end(), ptr,
                                    static_cast<int>(noteArray.size()),
                                    note.get_note_type() == NOTE_TYPE::HOLD
                                        ? static_cast<int>(holdArray.size())
                                        : -1};

        noteArray.push_back(ptr);
        if (note.get_note_type() == NOTE_TYPE::HOLD)
            holdArray.push_back(ptr);

        set_ooo();
        noteCount++;

        return true;
    } catch (const std::bad_alloc& e) {
        print_debug_message("Failed to create note: " + std::string(e.what()));
        return false;
    }
}

Note NotePoolManager::get_note(const std::string& noteID) const {
    auto lock = lock_shared();
    return *get_note_pointer(noteID);
}

void NotePoolManager::get_notes(std::vector<Note>& outNotes,
                                bool excludeSub) const {
    auto lock = lock_shared();
    outNotes.clear();
    for (const auto& note_ptr : noteArray) {
        if (note_ptr) {
            if (excludeSub && note_ptr->get_note_type() == NOTE_TYPE::SUB) {
                continue;  // Skip sub notes
            }
            outNotes.push_back(*note_ptr);
        }
    }
}

Note NotePoolManager::get_note_direct(int index) const {
    auto lock = lock_shared();
    if (index < 0 || index >= static_cast<int>(noteArray.size())) {
        throw std::out_of_range("Index out of range in NotePoolManager");
    }
    if (!noteArray[index]) {
        throw std::out_of_range("Note index refers to a deleted note");
    }
    return *noteArray[index];
}

void NotePoolManager::set_note(const Note& note) {
    auto lock = lock_exclusive();
    auto note_ptr = get_note_pointer(note.noteID);
    if (note_ptr->time != note.time)
        set_ooo();
    *note_ptr = note;

    sync_head_note_to_sub(*note_ptr);
    sync_hold_note_length(*note_ptr);
}

void NotePoolManager::set_note_bitwise(const char* prop) {
    Note note;
    note.read(prop);
    set_note(note);
}

void NotePoolManager::read_all_notes(
    std::function<void(const Note&)> reader) const {
    auto lock = lock_shared();
    NoteCallbackScope scope(this);
    for (const auto& note_ptr : noteArray) {
        if (note_ptr) {
            reader(*note_ptr);
        }
    }
}

void NotePoolManager::execute_note_callback(
    Note& note, const std::function<void(Note&)>& executor) {
    const double origTime = note.time;
    try {
        executor(note);
    } catch (...) {
        if (origTime != note.time)
            set_ooo();
        throw;
    }
    if (origTime != note.time)
        set_ooo();
}

void NotePoolManager::edit_note(Note& note,
                                const std::function<void(Note&)>& executor) {
    std::exception_ptr failure;
    try {
        execute_note_callback(note, executor);
    } catch (...) {
        failure = std::current_exception();
    }
    sync_head_note_to_sub(note);
    sync_hold_note_length(note);
    if (failure)
        std::rethrow_exception(failure);
}

void NotePoolManager::access_note(const std::string& noteID,
                                  std::function<void(Note&)> executor) {
    auto lock = lock_exclusive();
    NoteCallbackScope scope(this);
    edit_note(*get_note_pointer(noteID), executor);
}

void NotePoolManager::access_all_notes(std::function<void(Note&)> executor) {
    auto lock = lock_exclusive();
    NoteCallbackScope scope(this);
    for (const auto& note_ptr : noteArray) {
        if (note_ptr) {
            edit_note(*note_ptr, executor);
        }
    }
}

void NotePoolManager::access_all_notes_parallel(
    std::function<void(Note&)> executor) {
    auto lock = lock_exclusive();
    tf::Taskflow taskflow;
    // Submission is synchronous, so inherited callback scopes remain alive
    // until every worker finishes, including when another manager calls us.
    const auto parent = NoteCallbackScope::context();
    taskflow.for_each(noteArray.begin(), noteArray.end(),
                      [&, parent](nptr note_ptr) {
                          if (note_ptr) {
                              NoteCallbackScope scope(this, parent);
                              execute_note_callback(*note_ptr, executor);
                          }
                      });
    execute_tasks(taskflow);
}

void NotePoolManager::sync_head_note_to_sub(const Note& note) {
    if (note.get_note_type() != NOTE_TYPE::HOLD)
        return;
    auto subNote = get_note_pointer(note.subNoteID);
    if (subNote) {
        subNote->beginTime = note.time;
        subNote->position = note.position;
        subNote->width = note.width;
        subNote->side = note.side;
    }
}

void NotePoolManager::sync_hold_note_length(const Note& note) {
    if (note.get_note_type() != NOTE_TYPE::HOLD &&
        note.get_note_type() != NOTE_TYPE::SUB)
        return;
    nptr holdNote = get_note_pointer(note.noteID);
    nptr subNote = get_note_pointer(note.subNoteID);
    if (holdNote->get_note_type() == NOTE_TYPE::SUB)
        std::swap(holdNote, subNote);
    holdNote->lastTime = subNote->time - holdNote->time;
}

void NotePoolManager::clear_notes() {
    auto lock = lock_exclusive();
    noteArray.clear();
    noteArray.shrink_to_fit();
    holdArray.clear();
    holdArray.shrink_to_fit();
    noteMemoryList.clear();
    noteInfoMap.clear();
    // In C++20, there's no shrink_to_fit for unordered_map,
    // but rehash(0) can help reduce bucket count.
    noteInfoMap.rehash(0);

    noteCount = 0;
    get_note_activation_manager().clear();
    reclaim_memory();
    return;
}

int NotePoolManager::get_index(const std::string& noteID) {
    auto lock = lock_shared();

    if (arrayOutOfOrder) {
        throw std::runtime_error(
            "Note array is out of order, cannot get index directly.");
    }

    auto it = noteInfoMap.find(noteID);
    if (it == noteInfoMap.end()) {
        throw std::runtime_error("Note not found: " + noteID);
    }
    return it->second.index;
}

bool NotePoolManager::release_note(std::string noteID) {
    auto lock = lock_exclusive();
    auto it = noteInfoMap.find(noteID);
    if (it == noteInfoMap.end()) {
        return false;
    }

    auto info = it->second;

    array_markdel_index(info);
    noteMemoryList.erase(info.iter);
    noteInfoMap.erase(it);

    set_ooo();
    noteCount--;
    return true;
}

bool NotePoolManager::release_note(const Note& note) {
    return release_note(note.noteID);
}

bool NotePoolManager::array_sort_request() {
    auto lock = lock_exclusive();
    if (!arrayOutOfOrder) {
        return false;
    }

    array_sort();
    unset_ooo();
    return true;
}

void NotePoolManager::set_ooo() {
    arrayOutOfOrder = true;
}

void NotePoolManager::unset_ooo() {
    arrayOutOfOrder = false;
}

void NotePoolManager::array_markdel_index(const NoteMemoryInfo& info) {
    noteArray[info.index] = nullptr;
    if (info.holdIndex >= 0) {
        holdArray[info.holdIndex] = nullptr;
    }
    set_ooo();
}

// Should only be called when mtxNoteOps is locked
void NotePoolManager::array_sort() {
    PROFILE_STATIC_SCOPE("Note Pool Manager Array Sort");
    static auto noteArray_cmp = [](const nptr& a, const nptr& b) {
        if (a == nullptr)
            return false;
        if (b == nullptr)
            return true;
        return a->time < b->time;
    };
    static auto holdArray_cmp = [](const nptr& a, const nptr& b) {
        if (a == nullptr)
            return false;
        if (b == nullptr)
            return true;
        return a->lastTime > b->lastTime;
    };

    auto single_array_pop = [&](std::vector<nptr>& array) {
        while (!array.empty() && array.back() == nullptr) {
            array.pop_back();
        }
    };

    auto start = std::chrono::high_resolution_clock::now();
    bool enableParallelSort;
    enableParallelSort =
        noteArray.size() >= NOTES_ARRAY_PARALLEL_SORT_THRESHOLD &&
        hardware_concurrency() > 1;
    if (enableParallelSort) {
        // Use parallel sort
        tf::Taskflow taskflow;
        taskflow.sort(noteArray.begin(), noteArray.end(), noteArray_cmp);
        taskflow.sort(holdArray.begin(), holdArray.end(), holdArray_cmp);
        execute_tasks(taskflow);
    } else {
        std::sort(noteArray.begin(), noteArray.end(), noteArray_cmp);
        std::sort(holdArray.begin(), holdArray.end(), holdArray_cmp);
    }

    single_array_pop(noteArray);
    single_array_pop(holdArray);

    for (size_t i = 0; i < noteArray.size(); ++i) {
        noteInfoMap[noteArray[i]->noteID].index = i;
    }
    for (size_t i = 0; i < holdArray.size(); ++i) {
        noteInfoMap[holdArray[i]->noteID].holdIndex = i;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - start);
    print_debug_message("array_sort took " + std::to_string(duration.count()) +
                        "ms");
}

NotePoolManager::nptr NotePoolManager::get_note_pointer(
    const std::string& noteID) const {
    // Requires mtxNoteOps or an externally guaranteed stable read phase.
    auto it = noteInfoMap.find(noteID);
    if (it == noteInfoMap.end()) {
        throw std::runtime_error("Note not found: " + noteID);
    }
    return it->second.pointer;
}

int NotePoolManager::get_index_upperbound(double time) {
    auto lock = lock_shared();
    if (arrayOutOfOrder)
        throw std::runtime_error(
            "Note array is out of order, cannot get index directly.");

    auto it = std::upper_bound(
        noteArray.begin(), noteArray.end(), time,
        [](double t, const nptr& note) { return t < note->time; });
    if (it == noteArray.end()) {
        return static_cast<int>(noteArray.size());
    }
    return static_cast<int>(it - noteArray.begin());
}

int NotePoolManager::get_index_lowerbound(double time) {
    auto lock = lock_shared();
    if (arrayOutOfOrder)
        throw std::runtime_error(
            "Note array is out of order, cannot get index directly.");

    auto it = std::lower_bound(
        noteArray.begin(), noteArray.end(), time,
        [](const nptr& note, double t) { return note->time < t; });
    if (it == noteArray.end()) {
        return static_cast<int>(noteArray.size());
    }
    return static_cast<int>(it - noteArray.begin());
}

// Thread unsafe function.
void NotePoolManager::reclaim_memory() {
    pool_res.release();
    monotonic_res.release();
}

// Singleton getter.
NotePoolManager& get_note_pool_manager() {
    static NotePoolManager instance;
    return instance;
}
