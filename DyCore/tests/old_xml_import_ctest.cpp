#include <doctest/doctest.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <vector>

#include "note.h"
#include "project.h"
#include "project/format/xml.h"
#include "timing.h"

TEST_CASE("old xml import parses legacy DynamixMap correctly") {
    namespace fs = std::filesystem;

    constexpr const char* oldXmlContent =
        R"(<?xml version="1.0" encoding="UTF-8" ?>
<DynamixMap MapID="m_map_songname_H" BarPerMinute="130" TimeOffset="0.15">
    <Center>
        <Note Index="0" SubIndex="-1" Position="1.0" Size="1.0" Time="0.0" Type="NORMAL" />
        <Note Index="1" SubIndex="2" Position="2.0" Size="1.5" Time="1.0" Type="HOLD" />
        <Note Index="2" SubIndex="-1" Position="2.0" Size="1.5" Time="3.0" Type="SUB" />
    </Center>
    <Left Type="PAD">
        <Note Index="3" SubIndex="-1" Position="0.0" Size="1.0" Time="2.5" Type="CHAIN" />
    </Left>
    <Right Type="MIXER">
        <Note Index="4" SubIndex="-1" Position="3.0" Size="1.0" Time="4.0" Type="NORMAL" />
    </Right>
</DynamixMap>)";

    fs::path tempPath;
    try {
        clear_notes();
        get_timing_manager().clear();

        tempPath = fs::temp_directory_path() / "dynode_old_xml_import_test.xml";
        {
            std::ofstream out(tempPath, std::ios::binary);
            REQUIRE(out.is_open());
            out << oldXmlContent;
        }

        auto importResult =
            chart_import_xml(tempPath.string().c_str(), true, true);
        CHECK(importResult == IMPORT_XML_RESULT_STATES::SUCCESS);

        auto metadata = chart_get_metadata();
        CHECK(metadata.sideType[0] == "PAD");
        CHECK(metadata.sideType[1] == "MIXER");

        CHECK(get_timing_manager().count() == 1);

        auto tp = get_timing_manager()[0];
        CHECK(std::abs(tp.get_bpm() - 520.0) <= 0.01);

        std::vector<Note> notes;
        get_notes_array(notes);
        CHECK(notes.size() == 4);

        bool hasHold = false;
        bool hasLeftChain = false;
        bool hasRightNormal = false;

        for (const auto& note : notes) {
            if (note.type == 2) {
                hasHold = true;
                CHECK(std::abs(note.lastTime - ((2.0 * 60000.0) / 130.0)) <=
                      1.0);
            }
            if (note.type == 1 && note.side == 1) {
                hasLeftChain = true;
            }
            if (note.type == 0 && note.side == 2) {
                hasRightNormal = true;
            }
        }

        CHECK(hasHold);
        CHECK(hasLeftChain);
        CHECK(hasRightNormal);
    } catch (...) {
        std::error_code ec;
        if (!tempPath.empty()) {
            fs::remove(tempPath, ec);
        }
        FAIL("exception thrown during old xml import test");
    }

    std::error_code ec;
    if (!tempPath.empty()) {
        fs::remove(tempPath, ec);
    }
}
