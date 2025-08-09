#include "notePoolManager.h"

#include <algorithm>
#include <chrono>
#include <mutex>
#include <stdexcept>
#include <taskflow/algorithm/for_each.hpp>
#include <taskflow/algorithm/sort.hpp>
#include <taskflow/taskflow.hpp>
#include <vector>

#include "note.h"
#include "notePoolManager.h"
#include "taskflow/core/executor.hpp"
#include "utils.h"

NotePoolManager::NotePoolManager()
    : monotonic_res(initial_buffer.data(), initial_buffer.size(),
                    std::pmr::new_delete_resource()),
      pool_res(&monotonic_res),
      arrayOutOfOrder(false) {
}

NotePoolManager::~NotePoolManager() {
}

const Note& NotePoolManager::operator[](int index) {
    std::lock_guard<std::mutex> lock(mtxNoteOps);
    if (index < 0 || index >= noteArray.size())
        throw std::out_of_range("Index out of range in NotePoolManager");
    if (arrayOutOfOrder)
        throw std::runtime_error(
            "Note array is out of order. Use get_note_direct() instead.");

    return *noteArray[index];
}

bool NotePoolManager::note_exists(const std::string& noteID) {
    return noteInfoMap.find(noteID) != noteInfoMap.end();
}

bool NotePoolManager::create_note(const Note& note) {
    std::lock_guard<std::mutex> lock(mtxNoteOps);
    if (note_exists(note.noteID)) {
        return false;
    }
    try {
        std::pmr::polymorphic_allocator<Note> alloc(&pool_res);
        auto ptr = std::allocate_shared<Note>(alloc);

        *ptr = note;

        noteMemoryList.emplace_back(ptr);
        noteInfoMap[note.noteID] = {
            --noteMemoryList.end(), ptr, static_cast<int>(noteArray.size()),
            note.type == 2 ? static_cast<int>(holdArray.size()) : -1};

        noteArray.push_back(ptr);
        if (note.type == 2)
            holdArray.push_back(ptr);

        set_ooo();
        noteCount++;

        return true;
    } catch (const std::bad_alloc& e) {
        print_debug_message("Failed to create note: " + std::string(e.what()));
        return false;
    }
}

const Note& NotePoolManager::get_note(const std::string& noteID) {
    nptr note_ptr;
    {
        std::lock_guard<std::mutex> lock(mtxNoteOps);
        if (!note_exists(noteID)) {
            throw std::runtime_error("Note not found: " + noteID);
        }
        note_ptr = get_note_pointer(noteID);
    }  // Release the manager lock

    return *note_ptr;
}

void NotePoolManager::get_notes(std::vector<Note>& outNotes,
                                bool excludeSub) const {
    std::lock_guard<std::mutex> lock(mtxNoteOps);
    outNotes.clear();
    for (const auto& note_ptr : noteArray) {
        if (note_ptr) {
            if (excludeSub && note_ptr->type == 3) {
                continue;  // Skip sub notes
            }
            outNotes.push_back(*note_ptr);
        }
    }
}

const Note& NotePoolManager::get_note_direct(int index) {
    std::lock_guard<std::mutex> lock(mtxNoteOps);
    if (index < 0 || index >= static_cast<int>(noteArray.size())) {
        throw std::out_of_range("Index out of range in NotePoolManager");
    }
    return *noteArray[index];
}

void NotePoolManager::set_note(const std::string& noteID, const Note& note) {
    nptr note_ptr;
    {
        std::lock_guard<std::mutex> lock(mtxNoteOps);
        if (!note_exists(noteID)) {
            throw std::runtime_error("Note not found: " + noteID);
        }
        note_ptr = get_note_pointer(noteID);
    }  // Release the manager lock
    if (note_ptr->time != note.time)
        set_ooo();
    *note_ptr = note;
}

void NotePoolManager::set_note_bitwise(const std::string& noteID,
                                       const char* prop) {
    nptr note_ptr;
    {
        std::lock_guard<std::mutex> lock(mtxNoteOps);
        if (!note_exists(noteID)) {
            throw std::runtime_error("Note not found: " + noteID);
        }
        note_ptr = get_note_pointer(noteID);
    }  // Release the manager lock

    double origTime = note_ptr->time;
    note_ptr->read(prop);
    if (origTime != note_ptr->time)
        set_ooo();
}

void NotePoolManager::access_note(const std::string& noteID,
                                  std::function<void(Note&)> executor) {
    nptr note_ptr;
    {
        std::lock_guard<std::mutex> lock(mtxNoteOps);
        if (!note_exists(noteID)) {
            throw std::runtime_error("Note not found: " + noteID);
        }
        note_ptr = get_note_pointer(noteID);
    }  // Release the manager lock

    executor(*note_ptr);
}

// This function is unsafe (DEADLOCK RISK). Do not access notePoolManager in
// your executor.
void NotePoolManager::access_all_notes(std::function<void(Note&)> executor) {
    std::lock_guard<std::mutex> lock(mtxNoteOps);
    for (const auto& note_ptr : noteArray) {
        if (note_ptr) {
            executor(*note_ptr);
        }
    }
}

// This function is slower (but safer)
void NotePoolManager::access_all_notes_safe(
    std::function<void(Note&)> executor) {
    std::vector<nptr> notes;
    {
        std::lock_guard<std::mutex> lock(mtxNoteOps);
        notes.reserve(get_note_count());
        for (const auto& note_ptr : noteArray) {
            if (note_ptr) {
                notes.push_back(note_ptr);
            }
        }
    }
    for (const auto& note_ptr : notes) {
        executor(*note_ptr);
    }
}

// This function is unsafe (DEADLOCK RISK). Do not access notePoolManager in
// your executor.
void NotePoolManager::access_all_notes_parallel(
    std::function<void(Note&)> executor) {
    std::lock_guard<std::mutex> lock(mtxNoteOps);
    tf::Executor tfexecutor;
    tf::Taskflow taskflow;
    taskflow.for_each(noteArray.begin(), noteArray.end(), [&](nptr note_ptr) {
        if (note_ptr) {
            executor(*note_ptr);
        }
    });
    tfexecutor.run(taskflow).wait();
}

void NotePoolManager::access_all_notes_parallel_safe(
    std::function<void(Note&)> executor) {
    std::vector<nptr> notes;
    {
        std::lock_guard<std::mutex> lock(mtxNoteOps);
        notes.reserve(get_note_count());
        for (const auto& note_ptr : noteArray) {
            if (note_ptr) {
                notes.push_back(note_ptr);
            }
        }
    }
    tf::Executor tfexecutor;
    tf::Taskflow taskflow;
    taskflow.for_each(notes.begin(), notes.end(),
                      [&](nptr note_ptr) { executor(*note_ptr); });
    tfexecutor.run(taskflow).wait();
}

void NotePoolManager::clear_notes() {
    std::lock_guard<std::mutex> lock(mtxNoteOps);
    noteArray.clear();
    holdArray.clear();
    noteMemoryList.clear();
    noteInfoMap.clear();
    noteCount = 0;
    reclaim_memory();
    return;
}

int NotePoolManager::get_index(const std::string& noteID) {
    std::lock_guard<std::mutex> lock(mtxNoteOps);

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
    std::lock_guard<std::mutex> lock(mtxNoteOps);
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
    std::lock_guard<std::mutex> lock(mtxNoteOps);
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
        tf::Executor tfexecutor;
        taskflow.sort(noteArray.begin(), noteArray.end(), noteArray_cmp);
        taskflow.sort(holdArray.begin(), holdArray.end(), holdArray_cmp);
        tfexecutor.run(taskflow).wait();
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
    const std::string& noteID) {
    // Should only be called when mtxNoteOps is locked
    return noteInfoMap.find(noteID)->second.pointer;
}

int NotePoolManager::get_index_upperbound(double time) {
    std::lock_guard<std::mutex> lock(mtxNoteOps);
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
    std::lock_guard<std::mutex> lock(mtxNoteOps);
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
}

// Singleton getter.
NotePoolManager& get_note_pool_manager() {
    static NotePoolManager instance;
    return instance;
}