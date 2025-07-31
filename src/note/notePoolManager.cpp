#include "notePoolManager.h"

#include <algorithm>

#include "note.h"
#include "utils.h"

extern NotePoolManager notePoolManager;

NotePoolManager::NotePoolManager()
    : monotonic_res(initial_buffer.data(), initial_buffer.size(),
                    std::pmr::new_delete_resource()),
      pool_res(&monotonic_res),
      arrayOutOfOrder(false) {
}

NotePoolManager::~NotePoolManager() {
}

bool NotePoolManager::note_exists(const std::string& noteID) {
    return noteInfoMap.find(noteID) != noteInfoMap.end();
}

bool NotePoolManager::create_note(std::string noteID) {
    std::lock_guard<std::mutex> lock(mtxNoteOps);
    if (note_exists(noteID)) {
        return false;
    }
    try {
        std::pmr::polymorphic_allocator<Note> alloc(&pool_res);
        auto ptr = std::allocate_shared<Note>(alloc);

        ptr->noteID = noteID;

        noteMemoryList.emplace_back(ptr);
        noteInfoMap[noteID] = {--noteMemoryList.end(), ptr,
                               static_cast<int>(noteArray.size())};

        noteArray.push_back(ptr);
        set_ooo();

        return true;
    } catch (const std::bad_alloc& e) {
        print_debug_message("Failed to create note: " + std::string(e.what()));
        return false;
    }
}

Note NotePoolManager::get_note(const std::string& noteID) {
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

void NotePoolManager::set_note(const std::string& noteID, const Note& note) {
    nptr note_ptr;
    {
        std::lock_guard<std::mutex> lock(mtxNoteOps);
        if (!note_exists(noteID)) {
            throw std::runtime_error("Note not found: " + noteID);
        }
        note_ptr = get_note_pointer(noteID);
    }  // Release the manager lock

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

    note_ptr->bitread(prop);
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

    std::lock_guard<std::mutex> noteLock(note_ptr->mtx);
    executor(*note_ptr);
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

    array_markdel_index(info.index);
    noteMemoryList.erase(info.iter);
    noteInfoMap.erase(it);

    set_ooo();
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

bool NotePoolManager::noteArray_cmp(const nptr a, const nptr b) {
    if (a == nullptr)
        return false;
    if (b == nullptr)
        return true;
    return a->time < b->time;
}

void NotePoolManager::set_ooo() {
    arrayOutOfOrder = true;
}

void NotePoolManager::unset_ooo() {
    arrayOutOfOrder = false;
}

void NotePoolManager::array_markdel_index(int index) {
    if (index < 0 || index >= static_cast<int>(noteArray.size())) {
        return;
    }

    noteArray[index] = nullptr;
    set_ooo();
}

void NotePoolManager::array_sort() {
    // Should only be called when mtxNoteOps is locked
    std::sort(noteArray.begin(), noteArray.end(), noteArray_cmp);

    while (!noteArray.empty() && noteArray.back() == nullptr) {
        noteArray.pop_back();
    }

    for (size_t i = 0; i < noteArray.size(); ++i) {
        noteInfoMap[noteArray[i]->noteID].index = i;
    }
}

NotePoolManager::nptr NotePoolManager::get_note_pointer(
    const std::string& noteID) {
    // Should only be called when mtxNoteOps is locked
    return noteInfoMap.find(noteID)->second.pointer;
}