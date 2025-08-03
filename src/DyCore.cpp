
#include "DyCore.h"

#include <iostream>

// Initializes the DyCore library.
//
// @return "success" on successful initialization.
DYCORE_API const char* DyCore_init() {
    std::ios::sync_with_stdio(false);
    return "success";
}
