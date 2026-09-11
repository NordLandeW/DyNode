#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <future>
#include <latch>
#include <string>

#include "gm.h"
#include "note.h"
#include "project.h"
#include "project/format/dyn.h"
#include "projectManager.h"
#include "timing.h"

extern "C" double DyCore_save_project(const char*, double);
extern "C" double DyCore_save_project_request(const char*, double);
extern "C" double DyCore_has_async_event();
extern "C" const char* DyCore_get_async_event();

namespace {
struct SaveFixture {
    std::filesystem::path dir =
        std::filesystem::temp_directory_path() /
        ("dynode_save_request_" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    SaveFixture() {
        std::filesystem::create_directory(dir);
        while (DyCore_has_async_event() > 0) {
            DyCore_get_async_event();
        }
        ProjectManager::inst().setup_default_chart();
    }
    ~SaveFixture() {
        ProjectManager::inst().setup_default_chart();
        while (DyCore_has_async_event() > 0) {
            DyCore_get_async_event();
        }
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
};

void set_chart(const std::string& title, double time) {
    auto& manager = ProjectManager::inst();
    manager.setup_default_chart();
    auto meta = manager.get_chart_metadata();
    meta.title = title;
    manager.set_chart_metadata(meta);
    manager.set_version("save-test");
    manager.set_project_metadata({{"marker", title}});
    manager.set_chart_path({"music.ogg", "video.mp4", "image.png"});
    get_timing_manager().add_timing_point({time, 500, 4});
    Note note{};
    note.noteID = title;
    note.time = time + 100;
    note.position = 2.5;
    note.width = 1;
    REQUIRE(insert_note(note) == 0);
}

void check_completion(uint64_t requestId, bool success) {
    REQUIRE(DyCore_has_async_event() > 0);
    const auto event = nlohmann::json::parse(DyCore_get_async_event());
    CHECK(event.at("type") == PROJECT_SAVING);
    CHECK(event.at("requestId") == requestId);
    CHECK((event.at("status").get<int>() >= 0) == success);
}
}  // namespace

TEST_CASE("ProjectSaveRequestDoesNotReadReplacementProject") {
    SaveFixture fixture;
    set_chart("A", 0);
    auto first =
        prepare_project_save((fixture.dir / "A.dyn").string().c_str(), 1);
    set_chart("B", 1000);
    auto second =
        prepare_project_save((fixture.dir / "B.dyn").string().c_str(), 1);
    REQUIRE(first.requestId != second.requestId);
    REQUIRE(first.requestId > 0);
    const auto firstId = first.requestId, secondId = second.requestId;

    // Execute the real workers only after replacing all live project data.
    // No scheduler timing assumptions: these are the requests save_project
    // dispatches.
    __async_save_project(std::move(first));
    __async_save_project(std::move(second));
    check_completion(firstId, false);
    check_completion(secondId, true);
    CHECK_FALSE(std::filesystem::exists(fixture.dir / "A.dyn"));

    Project savedB;
    REQUIRE(project_import_dyn((fixture.dir / "B.dyn").string().c_str(),
                               savedB) == 0);
    REQUIRE(savedB.charts.size() == 1);
    CHECK(savedB.metadata.at("marker") == "B");
    CHECK(savedB.charts[0].metadata.title == "B");
    REQUIRE(savedB.charts[0].notes.size() == 1);
    CHECK(savedB.charts[0].notes[0].time == 1100);
    REQUIRE(savedB.charts[0].timingPoints.size() == 1);
    CHECK(savedB.charts[0].timingPoints[0].time == 1000);
    CHECK(savedB.charts[0].path.music == "music.ogg");
    CHECK(ProjectManager::inst().get_chart_metadata().title == "B");
}

TEST_CASE("ProjectSaveCapturedDataSurvivesProjectSwitch") {
    SaveFixture fixture;
    set_chart("A", 0);
    const auto generation = ProjectManager::inst().get_project_generation();
    std::promise<void> captured;
    auto ready = captured.get_future();
    std::latch resume(1);
    auto worker = std::async(std::launch::async, [&] {
        auto snapshot = ProjectManager::inst().create_save_snapshot(generation);
        captured.set_value();
        resume.wait();
        return nlohmann::json(snapshot);
    });
    ready.get();
    set_chart("B", 1000);
    resume.count_down();
    const auto saved = worker.get();
    CHECK(saved.at("metadata").at("marker") == "A");
    CHECK(saved.at("charts")[0].at("metadata").at("title") == "A");
    CHECK(saved.at("charts")[0].at("notes")[0].at("time") == 100);
    CHECK(saved.at("charts")[0].at("timingPoints")[0].at("offset") == 0);
}

TEST_CASE("ProjectSaveCloseInvalidatesUnreadRequest") {
    SaveFixture fixture;
    set_chart("A", 0);
    auto request =
        prepare_project_save((fixture.dir / "A.dyn").string().c_str(), 1);
    const auto requestId = request.requestId;
    ProjectManager::inst().invalidate_pending_saves();
    __async_save_project(std::move(request));
    check_completion(requestId, false);
    CHECK_FALSE(std::filesystem::exists(fixture.dir / "A.dyn"));
}

TEST_CASE("ProjectSaveFailureRetainsRequestIdentity") {
    SaveFixture fixture;
    set_chart("A", 0);
    auto request = prepare_project_save(
        (fixture.dir / "missing" / "A.dyn").string().c_str(), 1);
    const auto requestId = request.requestId;
    __async_save_project(std::move(request));
    check_completion(requestId, false);
    CHECK_FALSE(std::filesystem::exists(fixture.dir / "missing" / "A.dyn"));
}

TEST_CASE("ProjectSaveSerializationFailureRetainsRequestIdentity") {
    SaveFixture fixture;
    set_chart("A", 0);
    auto request =
        prepare_project_save((fixture.dir / "A.dyn").string().c_str(), 1);
    auto meta = ProjectManager::inst().get_chart_metadata();
    meta.title = std::string(1, static_cast<char>(0xff));
    ProjectManager::inst().set_chart_metadata(meta);
    const auto requestId = request.requestId;
    __async_save_project(std::move(request));
    check_completion(requestId, false);
    CHECK_FALSE(std::filesystem::exists(fixture.dir / "A.dyn"));
}

TEST_CASE("ProjectSaveRejectedRequestDoesNotDispatchCompletion") {
    SaveFixture fixture;
    CHECK(DyCore_save_project_request(nullptr, 1) == -1);
    CHECK(DyCore_save_project("", 1) == -1);
    CHECK(DyCore_save_project_request(
              (fixture.dir / "missing" / "A.dyn").string().c_str(), 1) == -1);
    int count = 0;
    while (DyCore_has_async_event() > 0) {
        const auto event = nlohmann::json::parse(DyCore_get_async_event());
        CHECK(event.at("type") == GENERAL_ERROR);
        CHECK_FALSE(event.contains("requestId"));
        ++count;
    }
    CHECK(count == 3);
}
