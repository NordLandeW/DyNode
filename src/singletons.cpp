
#include "singletons.h"

NotePoolManager& get_note_pool_manager() {
    static NotePoolManager instance;
    return instance;
}