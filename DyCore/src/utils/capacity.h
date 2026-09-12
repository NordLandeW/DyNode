#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

// Grow once to cover the whole request, retaining headroom for later frames.
inline size_t capacity_with_headroom(size_t current, size_t required,
                                     size_t maximum) {
    if (required > maximum) {
        throw std::length_error("Requested capacity exceeds container limit");
    }
    if (required <= current) {
        return current;
    }
    const size_t extra = std::max<size_t>(required / 2, 64);
    return extra > maximum - required ? maximum : required + extra;
}

template <typename T>
bool reserve_with_headroom(std::vector<T>& values, size_t required) {
    if (required <= values.capacity()) {
        return false;
    }
    values.reserve(
        capacity_with_headroom(values.capacity(), required, values.max_size()));
    return true;
}
