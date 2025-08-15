
#include "DyCore.h"

#include <iostream>

// Initializes the DyCore library.
//
// @return "success" on successful initialization.
DYCORE_API const char* DyCore_init() {
    std::ios::sync_with_stdio(false);
    return "success";
}

#ifdef WIN32
#include <windows.h>
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif  // def WIN32