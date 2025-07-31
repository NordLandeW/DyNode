#include <memory_resource>

#include "note.h"

class NotePoolManager {
   public:
    NotePoolManager();
    ~NotePoolManager();

    bool note_exists(const std::string &noteID);
    bool create_note(std::string noteID);
    Note get_note(const std::string &noteID);
    void set_note(const std::string &noteID, const Note &note);
    void set_note_bitwise(const std::string &noteID, const char *prop);
    void access_note(const std::string &noteID,
                     std::function<void(Note &)> executor);
    int get_index(const std::string &noteID);
    bool release_note(std::string noteID);
    bool release_note(const Note &note);
    bool array_sort_request();

   private:
    using nptr = std::shared_ptr<Note>;

    struct NoteMemoryInfo {
        std::pmr::list<nptr>::iterator iter;
        nptr pointer;
        int index;
    };

    static bool noteArray_cmp(const nptr a, const nptr b);
    void set_ooo();
    void unset_ooo();
    void array_markdel_index(int index);
    void array_sort();
    nptr get_note_pointer(const std::string &noteID);

    std::array<std::byte, 64 * 1024 * 1024> initial_buffer;
    std::pmr::monotonic_buffer_resource monotonic_res;
    std::pmr::unsynchronized_pool_resource pool_res;
    std::vector<nptr> noteArray;
    std::pmr::list<nptr> noteMemoryList;
    std::unordered_map<std::string, NoteMemoryInfo> noteInfoMap;
    std::mutex mtxNoteOps;
    bool arrayOutOfOrder = false;
};