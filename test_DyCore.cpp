// test_DyCore.cpp
#include <zstd.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

#include "DyCore.h"
#include "gtest/gtest.h"
#include "json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

TEST(DyCore, Init) {
    const char* res = DyCore_init();
    EXPECT_STREQ(res, "success");
}

TEST(DyCore, CompressDecompress) {
    std::string original = "Hello, world! This is a test string.";
    size_t compBound = ZSTD_compressBound(original.size());
    std::vector<char> compBuffer(compBound);

    double cSize =
        DyCore_compress_string(original.c_str(), compBuffer.data(), 3);
    EXPECT_GT(cSize, 0);

    double isComp = DyCore_is_compressed(compBuffer.data(), cSize);
    EXPECT_EQ(isComp, 0.0);

    const char* decompressed =
        DyCore_decompress_string(compBuffer.data(), cSize);
    EXPECT_EQ(original, std::string(decompressed));
}

TEST(DyCore, BufferCopy) {
    const char* src = "TestBuffer";
    char dst[20] = {0};
    double result = DyCore_buffer_copy(dst, (void*)src, strlen(src) + 1);
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(dst, src);
}

TEST(DyCore, NotesManagement) {
    double clearRes = DyCore_clear_notes();
    EXPECT_EQ(clearRes, 0.0);

    auto note = dyn::Note({100, 0, 1, 2.5, 0, 0, "test", 100});
    std::string noteStr = note.full_dump();

    double insertRes = DyCore_insert_note(noteStr.c_str());
    EXPECT_EQ(insertRes, 0.0);

    double insertResDuplicate = DyCore_insert_note(noteStr.c_str());
    EXPECT_EQ(insertResDuplicate, -1.0);

    note.time = 200.0;
    std::string modNoteStr = note.full_dump();
    double modifyRes = DyCore_modify_note(modNoteStr.c_str());
    EXPECT_EQ(modifyRes, 0.0);

    double deleteRes = DyCore_delete_note(noteStr.c_str());
    EXPECT_EQ(deleteRes, 0.0);

    double deleteResNonExistent = DyCore_delete_note(noteStr.c_str());
    EXPECT_EQ(deleteResNonExistent, -1.0);
}

TEST(DyCore, SaveProject) {
    std::string filename = "temp_project_save.dyc";
    if (fs::exists(filename))
        fs::remove(filename);

    json project;
    project["version"] = "1.0";
    project["charts"] = json::object();
    std::string projectStr = project.dump();

    double ret = DyCore_save_project(projectStr.c_str(), filename.c_str(), 3);
    EXPECT_EQ(ret, 0.0);

    bool eventReceived = false;
    std::string eventJson;
    for (int i = 0; i < 50; i++) {
        if (DyCore_has_async_event() > 0) {
            eventJson = DyCore_get_async_event();
            eventReceived = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    EXPECT_TRUE(eventReceived) << "未收到异步保存事件通知";

    if (eventReceived) {
        json eventObj = json::parse(eventJson);
        EXPECT_EQ(eventObj["status"], 0);
    }

    EXPECT_TRUE(fs::exists(filename));

    std::ifstream ifs(filename, std::ios::binary);
    std::string fileContent((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
    ifs.close();

    const char* decompressed =
        DyCore_decompress_string(fileContent.c_str(), fileContent.size());
    json savedProject = json::parse(decompressed);
    EXPECT_TRUE(savedProject.contains("charts"));
    EXPECT_TRUE(savedProject.contains("version"));

    fs::remove(filename);
}