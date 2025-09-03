#pragma once

#include <string>

template <typename T>
    requires(std::is_trivially_copyable_v<T>)
void bitwrite(char *&buffer, const T &value) {
    memcpy(buffer, &value, sizeof(T));
    buffer += sizeof(T);
}

template <typename T>
    requires(std::is_same_v<T, std::string>)
void bitwrite(char *&buffer, const T &value) {
    memcpy(buffer, value.data(), value.size() + 1);
    buffer += value.size() + 1;
}

template <typename T>
    requires(std::is_trivially_copyable_v<T>)
void bitread(const char *&buffer, T &value) {
    memcpy(&value, buffer, sizeof(T));
    buffer += sizeof(T);
}

template <typename T>
    requires(std::is_same_v<T, std::string>)
void bitread(const char *&buffer, T &value) {
    int size = strlen(buffer);
    value.assign(buffer, size);
    buffer += size + 1;
}