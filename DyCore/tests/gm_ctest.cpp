#include <doctest/doctest.h>

#include <json.hpp>
#include <string>

#include "gm.h"

extern "C" double DyCore_has_async_event();
extern "C" const char* DyCore_get_async_event();

namespace {
void clear_async_events() {
    while (DyCore_has_async_event() > 0) {
        DyCore_get_async_event();
    }
}
}  // namespace

TEST_CASE("AsyncEventsAreFifo") {
    clear_async_events();

    push_async_event({PROJECT_SAVING, 1, "first"});
    push_async_event({GENERAL_ERROR, -1, "second"});

    const auto first = nlohmann::json::parse(DyCore_get_async_event());
    const auto second = nlohmann::json::parse(DyCore_get_async_event());

    CHECK(first.at("type") == PROJECT_SAVING);
    CHECK(first.at("status") == 1);
    CHECK(first.at("content") == "first");
    CHECK(second.at("type") == GENERAL_ERROR);
    CHECK(second.at("status") == -1);
    CHECK(second.at("content") == "second");
    CHECK(DyCore_has_async_event() == 0);
}

TEST_CASE("GmAnnouncementUtf8") {
    clear_async_events();

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

TEST_CASE("GameMakerExecuteQueuesFunctionNameAndArguments") {
    clear_async_events();

    gamemaker_execute("editor_set_editmode", nlohmann::json::array({0}));

    REQUIRE(DyCore_has_async_event() > 0);
    const auto event = nlohmann::json::parse(DyCore_get_async_event());
    const auto content = nlohmann::json::parse(
        event.at("content").get_ref<const std::string&>());

    CHECK(event.at("type") == GM_EXECUTE);
    CHECK(event.at("status") == 0);
    CHECK(content.at("name") == "editor_set_editmode");
    REQUIRE(content.at("args").is_array());
    REQUIRE(content.at("args").size() == 1);
    CHECK(content.at("args").at(0) == 0);
}

TEST_CASE("GameMakerExecuteRequiresArgumentArray") {
    clear_async_events();

    CHECK_THROWS_AS(gamemaker_execute("editor_set_editmode", 1),
                    std::invalid_argument);
    CHECK(DyCore_has_async_event() == 0);
}
