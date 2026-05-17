#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "project.h"

namespace {

std::filesystem::path make_temp_dir() {
    const auto ticks = std::chrono::steady_clock::now()
                           .time_since_epoch()
                           .count();
    auto dir = std::filesystem::temp_directory_path() /
               ("dynode_project_backup_test_" + std::to_string(ticks));
    std::filesystem::create_directories(dir);
    return dir;
}

void write_text(const std::filesystem::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out.is_open());
    out << text;
}

std::string read_text(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    REQUIRE(in.is_open());
    return {std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};
}

std::filesystem::path backup_path(const std::filesystem::path& finalPath,
                                  int version) {
    std::filesystem::path backupName = finalPath.stem();
    backupName += ".bak.";
    backupName += std::to_string(version);
    backupName += finalPath.extension();
    return finalPath.parent_path() / "backups" / backupName;
}

}  // namespace

TEST_CASE("ProjectBackupRotate") {
    namespace fs = std::filesystem;

    const fs::path dir = make_temp_dir();
    try {
        const fs::path missingProject = dir / "missing file.dyn";
        backup_existing_project_file(missingProject);
        CHECK_FALSE(fs::exists(dir / "backups"));

        const fs::path projectPath = dir / "X File.dyn";

        write_text(projectPath, "current0");
        backup_existing_project_file(projectPath);
        CHECK(read_text(projectPath) == "current0");
        CHECK(read_text(backup_path(projectPath, 0)) == "current0");
        CHECK_FALSE(fs::exists(backup_path(projectPath, 1)));

        write_text(projectPath, "current1");
        backup_existing_project_file(projectPath);
        CHECK(read_text(projectPath) == "current1");
        CHECK(read_text(backup_path(projectPath, 0)) == "current1");
        CHECK(read_text(backup_path(projectPath, 1)) == "current0");

        write_text(projectPath, "current2");
        backup_existing_project_file(projectPath);
        CHECK(read_text(projectPath) == "current2");
        CHECK(read_text(backup_path(projectPath, 0)) == "current2");
        CHECK(read_text(backup_path(projectPath, 1)) == "current1");
        CHECK(read_text(backup_path(projectPath, 2)) == "current0");

        write_text(projectPath, "current3");
        backup_existing_project_file(projectPath);
        CHECK(read_text(projectPath) == "current3");
        CHECK(read_text(backup_path(projectPath, 0)) == "current3");
        CHECK(read_text(backup_path(projectPath, 1)) == "current2");
        CHECK(read_text(backup_path(projectPath, 2)) == "current1");
        CHECK_FALSE(fs::exists(backup_path(projectPath, 3)));
    } catch (...) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        FAIL("exception thrown during project backup rotation test");
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST_CASE("ProjectBackupPreservesSource") {
    namespace fs = std::filesystem;

    const fs::path dir = make_temp_dir();
    try {
        const fs::path projectPath = dir / "Rollback Source.dyn";

        write_text(projectPath, "original project");
        backup_existing_project_file(projectPath);

        CHECK(read_text(projectPath) == "original project");
        CHECK(read_text(backup_path(projectPath, 0)) == "original project");
    } catch (...) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        FAIL("exception thrown during project backup preserve test");
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
}
