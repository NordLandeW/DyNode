
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "api.h"

using std::lock_guard;
using std::mutex;
using std::string;
using std::string_view;
using std::unordered_set;
using std::vector;

struct StringHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const {
        return std::hash<std::string_view>{}(sv);
    }
};

using TransparentSet =
    std::unordered_set<std::string, StringHash, std::equal_to<>>;

class Class_DyCoreUnorderedSetManager {
   private:
    vector<TransparentSet> sets;
    vector<int> freeIndexes;
    mutex mtx;

    void acquire() {
        sets.push_back(TransparentSet());
        freeIndexes.push_back(sets.size() - 1);
    }

   public:
    Class_DyCoreUnorderedSetManager() = default;

    int get() {
        lock_guard<mutex> lock(mtx);
        if (freeIndexes.empty()) {
            acquire();
        }
        int result = freeIndexes.back();
        freeIndexes.pop_back();
        return result;
    }

    bool has(int index, const char* value) {
        lock_guard<mutex> lock(mtx);
        if (index < 0 || index >= sets.size()) {
            return false;  // Invalid index
        }
        return sets[index].count(value);
    }

    bool add(int index, const char* value) {
        lock_guard<mutex> lock(mtx);
        if (index < 0 || index >= sets.size()) {
            return false;  // Invalid index
        }
        auto& set = sets[index];
        auto result = set.insert(value);
        return result.second;  // Returns true if the value was added, false if
                               // it already exists
    }

    bool remove(int index, const char* value) {
        lock_guard<mutex> lock(mtx);
        if (index < 0 || index >= sets.size()) {
            return false;  // Invalid index
        }
        return sets[index].erase(value) >
               0;  // Returns true if the value was removed
    }

    bool merge(int destIndex, int fromIndex) {
        lock_guard<mutex> lock(mtx);
        if (destIndex < 0 || destIndex >= sets.size() || fromIndex < 0 ||
            fromIndex >= sets.size()) {
            return false;  // Invalid index
        }
        auto &destSet = sets[destIndex], &fromSet = sets[fromIndex];
        destSet.insert(fromSet.begin(), fromSet.end());
        return true;
    }

    bool clear(int index) {
        lock_guard<mutex> lock(mtx);
        if (index < 0 || index >= sets.size()) {
            return false;  // Invalid index
        }
        sets[index].clear();
        return true;
    }

    bool free(int index) {
        lock_guard<mutex> lock(mtx);
        if (index < 0 || index >= sets.size()) {
            return false;
        }
        sets[index].clear();
        freeIndexes.push_back(index);
        return true;
    }
} DyCoreUnorderedSetManager;

DYCORE_API double DyCore_ds_unordered_set_get() {
    return DyCoreUnorderedSetManager.get();
}

DYCORE_API double DyCore_ds_unordered_set_has(double index, const char* value) {
    if (!value)
        return -1.0;
    return DyCoreUnorderedSetManager.has(index, value) ? 1.0 : -1.0;
}

DYCORE_API double DyCore_ds_unordered_set_add(double index, const char* value) {
    if (!value)
        return -1.0;
    return DyCoreUnorderedSetManager.add(index, value) ? 1.0 : -1.0;
}

DYCORE_API double DyCore_ds_unordered_set_remove(double index,
                                                 const char* value) {
    if (!value)
        return -1.0;
    return DyCoreUnorderedSetManager.remove(index, value) ? 1.0 : -1.0;
}

DYCORE_API double DyCore_ds_unordered_set_merge(double destIndex,
                                                double fromIndex) {
    return DyCoreUnorderedSetManager.merge(destIndex, fromIndex) ? 1.0 : -1.0;
}

DYCORE_API double DyCore_ds_unordered_set_clear(double index) {
    return DyCoreUnorderedSetManager.clear(index) ? 1.0 : -1.0;
}

DYCORE_API double DyCore_ds_unordered_set_free(double index) {
    return DyCoreUnorderedSetManager.free(index) ? 1.0 : -1.0;
}