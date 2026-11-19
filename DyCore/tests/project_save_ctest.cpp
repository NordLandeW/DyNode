#include <doctest/doctest.h>

#include <barrier>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <latch>
#include <set>
#include <string>

#include "gm.h"
#include "note.h"
#include "notePoolManager.h"
#include "project.h"
#include "project/format/dyn.h"
#include "projectManager.h"
#include "timing.h"

extern "C" double DyCore_save_project(const char*, double);
extern "C" double DyCore_save_project_request(const char*, double);
extern "C" double DyCore_has_async_event();
extern "C" const char* DyCore_get_async_event();
extern "C" double DyCore_chart_import_dyn(const char*, double, double);
extern "C" double DyCore_import_chart_file(const char*, double, double, double);
extern "C" double DyCore_project_load(const char*);
extern "C" double DyCore_chart_export_xml(const char*, double, double);

namespace {
struct SaveFixture {
    std::filesystem::path dir =
        std::filesystem::temp_directory_path() /
        ("dynode_save_request_" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    SaveFixture() {
        initialize_project_saves();
        std::filesystem::create_directory(dir);
        while (DyCore_has_async_event() > 0) {
            DyCore_get_async_event();
        }
        ProjectManager::inst().setup_default_chart();
    }
    ~SaveFixture() {
        shutdown_project_saves();
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

TEST_CASE("ChartImportExportReportsFileFailures") {
    SaveFixture fixture;
    set_chart("export", 0);
    const auto missing = fixture.dir / "missing" / "chart.xml";
    CHECK(DyCore_chart_export_xml(missing.string().c_str(), 1, 0) == -1);
    CHECK_FALSE(std::filesystem::exists(missing));
    const auto output = fixture.dir / "chart.xml";
    REQUIRE(DyCore_chart_export_xml(output.string().c_str(), 1, 0) == 0);
    CHECK(std::filesystem::file_size(output) > 0);

    CHECK(DyCore_chart_import_dyn(missing.string().c_str(), 1, 1) == -1);
    const auto input = fixture.dir / "invalid.dyn";
    for (const auto& content :
         {std::string("{"), nlohmann::json(Project{}).dump()}) {
        std::ofstream(input, std::ios::binary) << content;
        CHECK(DyCore_chart_import_dyn(input.string().c_str(), 1, 1) == -1);
    }
    auto request =
        prepare_project_save((fixture.dir / "valid.dyn").string().c_str(), 1);
    __async_save_project(std::move(request));
    REQUIRE(DyCore_chart_import_dyn(
                (fixture.dir / "valid.dyn").string().c_str(), 1, 1) == 0);
}

TEST_CASE("FailedImportPreservesLiveIdsAndSavedProject") {
    SaveFixture fixture;
    set_chart("original", 250);
    auto& manager = ProjectManager::inst();
    const auto generation = manager.get_project_generation();
    const nlohmann::json before = manager.create_save_snapshot(generation);
    const auto note = get_note_pool_manager().get_note("original");
    const auto metadataVersion =
        manager.get_chart_metadata_last_modified_time();
    const auto copy = (fixture.dir / "copy.dyn").string();
    std::ofstream(copy) << before;
    auto request = prepare_project_save(copy.c_str(), 1);
    const std::pair<const char*, const char*> examples[] = {
        {"unfinished.xml",
         R"(<DynamixMap MapID="m_map_wip_H" BarPerMinute="100"><Center><Note Type="HOLD" Index="1" SubIndex="2" Time="0" /></Center></DynamixMap>)"},
        {"early.dy", R"({"DynamixMap":{}})"},
        {"partial.dyn", "{"},
        {"partial.osu",
         "osu file format "
         "v14\n[TimingPoints]\n0,500,4\n[HitObjects]\n0,0,100,1,0,0:0\n256,0,"
         "unfinished"}};
    for (auto [name, text] : examples) {
        CAPTURE(name);
        const auto path = (fixture.dir / name).string();
        std::ofstream(path) << text;
        CHECK(DyCore_import_chart_file(path.c_str(), 1, 1, 7) != 0);
        if (std::string(name) == "partial.dyn")
            CHECK(DyCore_project_load(path.c_str()) < 0);
        REQUIRE(manager.get_project_generation() == generation);
        CHECK(manager.get_chart_metadata_last_modified_time() ==
              metadataVersion);
        CHECK(nlohmann::json(manager.create_save_snapshot(generation)) ==
              before);
        REQUIRE(note_exists("original"));
        CHECK(get_note_pool_manager().get_note("original").dump() ==
              note.dump());
    }
    __async_save_project(std::move(request));
    Project saved;
    REQUIRE(project_import_dyn(copy.c_str(), saved) == 0);
    CHECK(nlohmann::json(saved) == before);
    CHECK(std::filesystem::exists(fixture.dir / "backups" / "copy.bak.0.dyn"));
}

TEST_CASE("ImportCommitsSelectedNotesTimingAndProjectGeneration") {
    SaveFixture fixture;
    set_chart("original", 0);
    auto& manager = ProjectManager::inst();
    auto generation = manager.get_project_generation();
    const auto osu = (fixture.dir / "keys.OSU").string();
    std::ofstream(osu) << "osu file format v14\n[General]\nMode: "
                          "3\n[TimingPoints]\n0.5,700,4\n100,400,3\n150,-80,"
                          "4\n[HitObjects]\n256,192,1000,128,0,1350:0:0:0:0:\n";
    REQUIRE(DyCore_import_chart_file(osu.c_str(), 1, 1, 0) == 0);
    CHECK(note_exists("original"));
    auto snapshot = manager.create_save_snapshot(generation);
    auto& chart = snapshot.charts.front();
    REQUIRE(chart.timingPoints.size() == 2);
    CHECK(chart.timingPoints.front().beatLength == 500);
    CHECK(chart.timingPoints.back().meter == 3);
    REQUIRE(chart.notes.size() == 2);
    const auto head = chart.notes.back();
    REQUIRE(head.get_note_type() == NOTE_TYPE::HOLD);
    CHECK(head.position == 2.5);
    REQUIRE(note_exists(head.subNoteID));
    const auto tail = get_note_pool_manager().get_note(head.subNoteID);
    CHECK(tail.time == 1350);
    CHECK(tail.subNoteID == head.noteID);
    const auto input = (fixture.dir / "copy.dyn").string();
    chart.metadata.title = "replacement";
    std::ofstream(input) << nlohmann::json(snapshot);
    // Replace notes only, then reset timing and take the file metadata.
    REQUIRE(DyCore_import_chart_file(input.c_str(), 0, 0, 1) == 0);
    CHECK_FALSE(note_exists("original"));
    CHECK(manager.create_save_snapshot(generation).charts[0].metadata.title ==
          "original");
    REQUIRE(DyCore_import_chart_file(input.c_str(), 1, 0, 2) == 0);
    CHECK(manager.create_save_snapshot(generation).charts[0].notes.size() == 4);
    CHECK(manager.get_chart_metadata().title == "replacement");
    CHECK(get_timing_manager().count() == 0);
    // A successful side-load no longer allows an old save to read this chart.
    REQUIRE(DyCore_import_chart_file(osu.c_str(), 0, 1, 7) == 0);
    CHECK_THROWS(manager.create_save_snapshot(generation));
    CHECK(get_note_pool_manager().get_note_count() == 0);
    CHECK(get_timing_manager().count() == 2);
    REQUIRE(DyCore_project_load(input.c_str()) == 0);
    CHECK(manager.get_project_path() == input);
    manager.set_current_chart(0);
    CHECK(nlohmann::json(manager.create_save_snapshot(
              manager.get_project_generation())) == nlohmann::json(snapshot));
}

TEST_CASE("ProjectSaveShutdownDrainsAcceptedWorkersBeforeReleasingData") {
    SaveFixture fixture;
    set_chart("shutdown", 0);
    const auto first = fixture.dir / "first.dyn";
    const auto second = fixture.dir / "second.dyn";
    const auto firstId = save_project(first.string().c_str(), 1);
    const auto secondId = save_project(second.string().c_str(), 1);
    shutdown_project_saves();
    CHECK_NOTHROW(shutdown_project_saves());
    std::set<uint64_t> completed;
    while (DyCore_has_async_event() > 0) {
        const auto event = nlohmann::json::parse(DyCore_get_async_event());
        REQUIRE(event.at("type") == PROJECT_SAVING);
        CHECK(event.at("status").get<int>() >= 0);
        completed.insert(event.at("requestId").get<uint64_t>());
    }
    CHECK(completed == std::set<uint64_t>{firstId, secondId});
    for (const auto& path : {first, second}) {
        Project saved;
        REQUIRE(project_import_dyn(path.string().c_str(), saved) == 0);
        CHECK(saved.metadata.at("marker") == "shutdown");
        REQUIRE(saved.charts.size() == 1);
        REQUIRE(saved.charts[0].notes.size() == 1);
        CHECK(saved.charts[0].notes[0].time == 100);
    }
    CHECK_THROWS_AS(
        save_project((fixture.dir / "late.dyn").string().c_str(), 1),
        std::runtime_error);
    CHECK_FALSE(std::filesystem::exists(fixture.dir / "late.dyn"));
}

TEST_CASE("ProjectSaveSnapshotsStayCoherentDuringNoteAndTimingEdits") {
    SaveFixture fixture;
    set_chart("concurrent", 0);
    const auto generation = ProjectManager::inst().get_project_generation();
    auto& pool = get_note_pool_manager();
    pool.access_note("concurrent", [](Note& note) {
        note.time = 0;
        note.width = 1;
        note.subNoteID = std::string(32, 'a');
    });
    std::barrier phase(2);
    auto edits = std::async(std::launch::async, [&] {
        for (int i = 1; i <= 128; ++i) {
            phase.arrive_and_wait();
            pool.access_note("concurrent", [i](Note& note) {
                note.time = i;
                note.width = i + 1;
                note.subNoteID = std::string(32 + i % 64, 'a' + i % 26);
            });
            get_timing_manager().add_offset(1);
            phase.arrive_and_wait();
        }
    });
    for (int i = 0; i < 128; ++i) {
        phase.arrive_and_wait();
        const auto snapshot =
            ProjectManager::inst().create_save_snapshot(generation);
        CHECK(snapshot.charts.size() == 1);
        if (snapshot.charts.size() == 1) {
            const auto& chart = snapshot.charts[0];
            CHECK(chart.notes.size() == 1);
            CHECK(chart.timingPoints.size() == 1);
            if (chart.notes.size() == 1 && chart.timingPoints.size() == 1) {
                const auto& note = chart.notes[0];
                const int revision = static_cast<int>(note.time);
                CHECK(note.width == revision + 1);
                CHECK(note.subNoteID ==
                      std::string(32 + revision % 64, 'a' + revision % 26));
                CHECK(chart.timingPoints[0].beatLength == 500);
            }
        }
        phase.arrive_and_wait();
    }
    edits.get();
    const auto path = fixture.dir / "concurrent.dyn";
    auto request = prepare_project_save(path.string().c_str(), 1);
    const auto requestId = request.requestId;
    __async_save_project(std::move(request));
    check_completion(requestId, true);
    Project saved;
    REQUIRE(project_import_dyn(path.string().c_str(), saved) == 0);
    REQUIRE(saved.charts.size() == 1);
    REQUIRE(saved.charts[0].notes.size() == 1);
    REQUIRE(saved.charts[0].timingPoints.size() == 1);
    CHECK(saved.charts[0].notes[0].time == 128);
    CHECK(saved.charts[0].notes[0].width == 129);
    CHECK(saved.charts[0].timingPoints[0].time == 128);
}
