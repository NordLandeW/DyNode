
#include "api.h"

// A test function to construct a vertex buffer for debugging purposes.
// Fills the buffer with sample vertex data.
//
// @param data A pointer to the vertex buffer.
// @param size The number of vertices to create.
// @return 0.0 on completion.
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
