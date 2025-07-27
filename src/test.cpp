
#include <taskflow/taskflow.hpp>

#include "api.h"

DYCORE_API double DyCore__test_vbuff_construct(void* data, double size) {
    int vCount = size;

    char* pointer = static_cast<char*>(data);
    for (int i = 0; i < vCount; i++) {
        *((float*)pointer) = i, pointer += 4;
        *((float*)pointer) = i, pointer += 4;
        *((float*)pointer) = 0, pointer += 4;
        *((float*)pointer) = 0, pointer += 4;
        *((int*)pointer) = 0xFFFFFFFF, pointer += 4;  // color
    }
    return 0.0;
}