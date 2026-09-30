#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "dlgbox.h"

TEST_CASE("SaveFilenameReplacesAllForbiddenCharacters") {
    for (wchar_t ch = 0; ch < 32; ++ch) {
        const std::wstring input = L"title" + std::wstring(1, ch) + L".xml";
        CHECK(sanitize_save_filename(input) == L"title_.xml");
    }
    for (const auto ch : std::wstring(L"<>:\"/\\|?*")) {
        CHECK(sanitize_save_filename(L"title" + std::wstring(1, ch) +
                                     L".dyn") == L"title_.dyn");
    }
    CHECK(sanitize_save_filename(
              L"_map_Butterfly Specimen\r\n_G-2026-7-29-22-22-32.xml") ==
          L"_map_Butterfly Specimen___G-2026-7-29-22-22-32.xml");
}

TEST_CASE("SaveFilenameHandlesReservedNamesAndEmptySuggestions") {
    for (const auto* stem :
         {L"CON", L"prn", L"Aux", L"NUL", L"CONIN$", L"CONOUT$", L"COM1",
          L"com9", L"LPT1", L"lpt9", L"COM\u00b9", L"COM\u00b2", L"COM\u00b3",
          L"LPT\u00b9", L"LPT\u00b2", L"LPT\u00b3"}) {
        for (const auto* extension : {L"", L".dyn", L".tar.gz"}) {
            const std::wstring input = std::wstring(stem) + extension;
            CHECK(sanitize_save_filename(input) == L"_" + input);
        }
    }
    CHECK(sanitize_save_filename(L"CON .xml") == L"_CON .xml");
    CHECK(sanitize_save_filename(L"  CON.xml  ") == L"_CON.xml");
    for (const auto* input : {L"", L".", L"..", L" . . "}) {
        CHECK(sanitize_save_filename(input) == L"example");
    }
    CHECK(sanitize_save_filename(L"title.xml . ") == L"title.xml");
    for (const auto* input :
         {L"COM0.dyn", L"COM10.dyn", L"LPT0.xml", L"CONcert.xml", L".hidden",
          L"my song.dyn", L"\u4e2d\u6587 \u66f2\u540d \U0001f98b.xml"}) {
        CHECK(sanitize_save_filename(input) == input);
    }
}

TEST_CASE("SaveFilenameBoundsLengthWithoutLosingExtensionOrUnicode") {
    CHECK(sanitize_save_filename(std::wstring(251, L'a') + L".xml") ==
          std::wstring(251, L'a') + L".xml");
    CHECK(sanitize_save_filename(std::wstring(500, L'a') + L".xml") ==
          std::wstring(251, L'a') + L".xml");
    CHECK(sanitize_save_filename(std::wstring(500, L'a')) ==
          std::wstring(255, L'a'));
    CHECK(sanitize_save_filename(std::wstring(250, L'a') + L"\U0001f98b" +
                                 std::wstring(20, L'b') + L".xml") ==
          std::wstring(250, L'a') + L".xml");
    // Truncation can itself produce a reserved name or a trailing period.
    const auto reserved = sanitize_save_filename(
        L"COM1" + std::wstring(300, L'a') + L"." + std::wstring(250, L'x'));
    CHECK(reserved.size() <= 255);
    CHECK(reserved.front() == L'_');
    CHECK(sanitize_save_filename(std::wstring(254, L'a') + L"." +
                                 std::wstring(300, L'b')) ==
          std::wstring(254, L'a'));
    CHECK(sanitize_save_filename(reserved) == reserved);
}

TEST_CASE("SanitizedSaveFilenamesCanBeCreatedOnWindows") {
    namespace fs = std::filesystem;
    const auto dir =
        fs::temp_directory_path() /
        ("dynode_filename_test_" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    REQUIRE(fs::create_directory(dir));
    struct Cleanup {
        fs::path path;
        ~Cleanup() {
            std::error_code ec;
            fs::remove_all(path, ec);
        }
    } cleanup{dir};

    const std::vector<std::wstring> suggestions = {
        L"_map_Butterfly Specimen\r\n_G-2026-7-29-22-22-32.xml",
        L"CON.dyn",
        L"LPT\u00b3.xml",
        L"\u4e2d\u6587 \U0001f98b.xml",
        L"title.xml . ",
        L"..",
        std::wstring(500, L'a') + L".xml"};
    for (const auto& suggestion : suggestions) {
        const auto filename = sanitize_save_filename(suggestion);
        CHECK(sanitize_save_filename(filename) == filename);
        auto path = dir / filename;
        // Exercise the component length limit independently of MAX_PATH.
        if (path.native().size() >= 260) {
            path = fs::path(L"\\\\?\\" + path.native());
        }
        std::ofstream out(path, std::ios::binary);
        REQUIRE(out.is_open());
        out << "filename regression";
        out.close();
        CHECK(fs::is_regular_file(path));
    }
}
