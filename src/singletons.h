#include <taskflow/taskflow.hpp>

#include "notePoolManager.h"
#include "taskflow/core/executor.hpp"

NotePoolManager& get_note_pool_manager();
tf::Executor& get_taskflow_executor();