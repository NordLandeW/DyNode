#pragma once

#include <cstddef>
#include <vector>

#include "layout.h"

struct Note;
namespace tf {
class Executor;
class Taskflow;
}  // namespace tf

// Sorted chart notes are resolved only within a synchronous draw. An item owns
// one disjoint range per layer, including independently clipped HOLD parts.
class DynamiteBatch {
   public:
    static size_t vertex_buffer_bound(size_t notes, size_t holds);
    bool reserve(size_t notes);
    size_t capacity_bytes() const;
    size_t hold_fill_vertices() const {
        return fillVertices;
    }
    size_t render(char* buffer, const PlaybackFrame& frame,
                  tf::Executor& executor, tf::Taskflow& tasks,
                  size_t workerCount, size_t& taskSubmissions);

   private:
    struct Item {
        const Note* note;
        size_t offset, edgeOffset;
        double head, tail;
        unsigned parts;
    };
    size_t fillVertices = 0;
    std::vector<Item> items;
};
