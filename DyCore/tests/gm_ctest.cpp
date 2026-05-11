#include <doctest/doctest.h>

#include <json.hpp>
#include <string>

#include "gm.h"

extern "C" double DyCore_has_async_event();
extern "C" const char* DyCore_get_async_event();

TEST_CASE("GmAnnouncementUtf8") {
    while (DyCore_has_async_event() > 0) {
        DyCore_get_async_event();
    }

    std::string invalidUtf8 = "bad ";
    invalidUtf8.push_back(static_cast<char>(0xff));

    CHECK_NOTHROW(gamemaker_announcement(GM_ANNOUNCEMENT_TYPE::ANNO_ERROR,
                                          "anno_project_load_failed",
                                          {invalidUtf8}));

    REQUIRE(DyCore_has_async_event() > 0);

    const auto event = nlohmann::json::parse(DyCore_get_async_event());
    const auto content = nlohmann::json::parse(event.at("content").get_ref<
                                               const std::string&>());

    CHECK(content.at("msg") == "anno_project_load_failed");
    CHECK(content.at("args").is_array());
    CHECK(content.at("args").size() == 1);
}
