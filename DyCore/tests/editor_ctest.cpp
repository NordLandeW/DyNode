#include <doctest/doctest.h>

#include "editor.h"

extern "C" double DyCore_gmeditor_get_ready();
extern "C" double DyCore_gmeditor_set_ready(double flag);
extern "C" double DyCore_gmeditor_sync_states(const char* states);

TEST_CASE("GameMakerEditorReadinessAndStateSync") {
    while (DyCore_gmeditor_get_ready() != 0.0) {
        REQUIRE(DyCore_gmeditor_set_ready(0.0) == 0.0);
    }

    CHECK_FALSE(gmeditor_is_ready());

    REQUIRE(DyCore_gmeditor_set_ready(1.0) == 0.0);
    REQUIRE(DyCore_gmeditor_set_ready(1.0) == 0.0);
    CHECK(DyCore_gmeditor_get_ready() == 1.0);

    REQUIRE(DyCore_gmeditor_sync_states(R"({"editMode":3})") == 0.0);
    CHECK(GMEditorManager::inst().get_editmode() == 3);
    CHECK(DyCore_gmeditor_sync_states("{invalid json") == -1.0);
    CHECK(DyCore_gmeditor_sync_states(R"({"otherState":1})") == -1.0);
    CHECK(DyCore_gmeditor_sync_states(nullptr) == -1.0);
    CHECK(GMEditorManager::inst().get_editmode() == 3);

    REQUIRE(DyCore_gmeditor_set_ready(0.0) == 0.0);
    CHECK(DyCore_gmeditor_get_ready() == 1.0);
    REQUIRE(DyCore_gmeditor_set_ready(0.0) == 0.0);
    CHECK(DyCore_gmeditor_get_ready() == 0.0);

    // An unmatched cleanup must not make the readiness counter negative.
    REQUIRE(DyCore_gmeditor_set_ready(0.0) == 0.0);
    CHECK_FALSE(gmeditor_is_ready());
}
