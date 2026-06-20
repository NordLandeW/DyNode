#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>

#include "utils.h"

namespace {

std::filesystem::path make_temp_file() {
    const auto ticks = std::chrono::steady_clock::now()
                           .time_since_epoch()
                           .count();
    auto path = std::filesystem::temp_directory_path() /
                ("dynode_utils_test_" + std::to_string(ticks) + ".dyn");
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out.is_open());
    out << "timestamp source";
    return path;
}

std::string path_to_utf8_string(const std::filesystem::path& path) {
    const auto utf8 = path.u8string();
    return std::string(reinterpret_cast<const char*>(utf8.c_str()),
                       utf8.size());
}

std::chrono::system_clock::time_point parse_timestamp(
    const std::string& timestamp) {
    std::tm localTime{};
    std::istringstream input(timestamp);
    input >> std::get_time(&localTime, "%Y_%m_%d_%H_%M_%S");
    REQUIRE_FALSE(input.fail());

    const std::time_t timeValue = std::mktime(&localTime);
    REQUIRE(timeValue != static_cast<std::time_t>(-1));
    return std::chrono::system_clock::from_time_t(timeValue);
}

}  // namespace

TEST_CASE("FileModificationTimeUsesPortableTimestampConversion") {
    namespace fs = std::filesystem;

    const fs::path path = make_temp_file();
    try {
        std::string pathString = path_to_utf8_string(path);
        const std::string timestamp =
            get_file_modification_time(pathString.data());

        CHECK(timestamp != "1970_01_01_00_00_00");
        CHECK(std::regex_match(
            timestamp, std::regex(R"(^\d{4}_\d{2}_\d{2}_\d{2}_\d{2}_\d{2}$)")));
    } catch (...) {
        std::error_code ec;
        fs::remove(path, ec);
        FAIL("exception thrown while reading file modification time");
    }

    std::error_code ec;
    fs::remove(path, ec);
}

TEST_CASE("FileModificationTimeFallbackUsesRecentCurrentTime") {
    namespace fs = std::filesystem;

    const auto ticks = std::chrono::steady_clock::now()
                           .time_since_epoch()
                           .count();
    const fs::path path = fs::temp_directory_path() /
                          ("dynode_missing_utils_test_" +
                           std::to_string(ticks) + ".dyn");
    std::error_code ec;
    fs::remove(path, ec);

    std::string pathString = path_to_utf8_string(path);
    const auto before = std::chrono::system_clock::now();
    const std::string timestamp = get_file_modification_time(pathString.data());
    const auto after = std::chrono::system_clock::now();

    CHECK(timestamp != "1970_01_01_00_00_00");
    CHECK(std::regex_match(
        timestamp, std::regex(R"(^\d{4}_\d{2}_\d{2}_\d{2}_\d{2}_\d{2}$)")));

    const auto parsed = parse_timestamp(timestamp);
    CHECK(parsed >= before - std::chrono::seconds(5));
    CHECK(parsed <= after);
}
