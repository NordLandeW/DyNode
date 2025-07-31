
#include "singletons.h"

NotePoolManager& get_note_pool_manager() {
    static NotePoolManager instance;
    return instance;
}

tf::Executor& get_taskflow_executor() {
    static tf::Executor executor;
    return executor;
}