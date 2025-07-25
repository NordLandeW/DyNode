#pragma once

#if !defined(_MSC_VER)
#define EXPORTED_FN __attribute__((visibility("default")))
#else
#define EXPORTED_FN __declspec(dllexport)
#endif

#define DYCORE_API extern "C" EXPORTED_FN