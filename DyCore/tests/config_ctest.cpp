#include <doctest/doctest.h>

#include "config.h"

extern "C" double DyCore_is_debug_build();

TEST_CASE("DyCoreDebugBuildFlag") {
    CHECK(DyCore_is_debug_build() == (DYCORE_DEBUG_BUILD ? 1.0 : 0.0));
}
