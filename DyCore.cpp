// DyCore.cpp: 定义应用程序的入口点。
//

#include "DyCore.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "json.hpp"

const int RETURN_BUFFER_SIZE = 50 * 1024 * 1024;
char _return_buffer[RETURN_BUFFER_SIZE];

void print_debug_message(std::string str) {
    std::cout << "[DyCore] " << str << std::endl;
}

DYCORE_API const char* DyCore_init() {
    return "success";
}

DYCORE_API const char* DyCore_delaunator(char* in_struct) {
    json j = json::parse(in_struct);

    if (!j.is_array())
        return "Error!in_struct must be an array.";

    auto coords = j.get<std::vector<double>>();

    delaunator::Delaunator d(coords);

    json r = json::array();

    for (std::size_t i = 0; i < d.triangles.size(); i += 3) {
        r.push_back(
            {{"p1",
              {d.coords[2 * d.triangles[i]], d.coords[2 * d.triangles[i] + 1]}},
             {"p2",
              {d.coords[2 * d.triangles[i + 1]],
               d.coords[2 * d.triangles[i + 1] + 1]}},
             {"p3",
              {d.coords[2 * d.triangles[i + 2]],
               d.coords[2 * d.triangles[i + 2] + 1]}}});
    }

    strcpy_s(_return_buffer, r.dump().c_str());

    return _return_buffer;
}

double executeCmdScript(const std::string& _workDir) {
    const std::string workDir = _workDir;
    std::string script = R"(
    @echo off
    echo Starting update process...
    set workDir=)" + workDir +
                         R"(
    echo Working directory is set to %workDir%
    :waitLoop
    tasklist | find /i "DyNode.exe" >nul
    if %ERRORLEVEL% equ 0 (
        echo DyNode.exe is still running, waiting...
        timeout /t 5 /nobreak >nul
        goto waitLoop
    )
    echo DyNode.exe is not running, proceeding with file operations...
    xcopy /e /i /y "%workDir%tmp\\" "%workDir%"
    if %ERRORLEVEL% neq 0 (
        echo Error copying files from tmp directory
        pause
        exit /b 1
    )
    echo Files copied successfully
    rmdir /s /q "%workDir%tmp"
    if %ERRORLEVEL% neq 0 (
        echo Error deleting tmp directory
        pause
        exit /b 1
    )
    echo tmp directory deleted successfully
    del "%workDir%update.zip"
    if %ERRORLEVEL% neq 0 (
        echo Error deleting update.zip
        pause
        exit /b 1
    )
    echo update.zip deleted successfully
    echo Update script completed successfully
    echo Exiting in 2 seconds...
    timeout /t 2 /nobreak >nul
    exit
    )";

    std::string filename = workDir + "update_script.bat";
    std::ofstream file(filename);
    if (file.is_open()) {
        file << script;
        file.close();
    } else {
        std::cout << "Unable to create script file" << std::endl;
        return -1;
    }

    std::string command = "cmd /c start \"Updater\" \"" + filename + "\"";
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cout << "Failed to execute script" << std::endl;
        return -2;
    }
    return 0;
}

DYCORE_API double DyCore_update(char* workDir) {
    return executeCmdScript(workDir);
}

namespace fs = std::filesystem;
double cleanupTempFiles(const std::string& workDir) {
    try {
        for (const auto& entry : fs::directory_iterator(workDir)) {
            if (entry.path().extension() == ".zip" &&
                entry.path().filename().string().find("DyNode") == 0) {
                fs::remove(entry.path());
                std::cout << "Deleted: " << entry.path() << std::endl;
            }
        }

        fs::path tmpDir = workDir + "tmp";
        if (fs::exists(tmpDir) && fs::is_directory(tmpDir)) {
            fs::remove_all(tmpDir);
            std::cout << "Deleted directory: " << tmpDir << std::endl;
        }

        fs::path batFile = workDir + "update_script.bat";
        if (fs::exists(batFile)) {
            fs::remove(batFile);
            std::cout << "Deleted: " << batFile << std::endl;
        }

        return 0.0;
    } catch (const fs::filesystem_error& e) {
        std::cout << "Filesystem error: " << e.what() << std::endl;
        return -1.0;
    } catch (const std::exception& e) {
        std::cout << "General error: " << e.what() << std::endl;
        return -1.0;
    }
}

DYCORE_API double DyCore_cleanup_tmpfiles(char* workDir) {
    return cleanupTempFiles(workDir);
}

DYCORE_API double DyCore_compress_string(const char* str, char* targetBuffer,
                                         double compressionLevel) {
    size_t fSize = strlen(str);
    size_t cBuffSize = ZSTD_compressBound(fSize);

    char* const fBuff = new char[fSize];
    char* const cBuff = new char[cBuffSize];

    memcpy(fBuff, str, fSize);

    std::cout << "[DyCore] Start compressing..." << std::endl;

    size_t const cSize =
        ZSTD_compress(cBuff, cBuffSize, fBuff, fSize, (int)compressionLevel);

    std::cout << "[DyCore] Finish compressing, checking..." << std::endl;

    try {
        CHECK_ZSTD(cSize);
    } catch (...) {
        delete[] cBuff;
        delete[] fBuff;
        return -1;
    };

    std::cout << "[DyCore] No error found. Success. " << fSize << "->" << cSize
              << std::endl;

    // Success
    memcpy(targetBuffer, cBuff, cSize);

    delete[] cBuff;
    delete[] fBuff;

    return (double)cSize;
}

bool check_compressed(const char* str, double _sSize) {
    size_t sSize = (size_t)_sSize;
    unsigned long long const rSize = ZSTD_getFrameContentSize(str, sSize);
    return rSize != ZSTD_CONTENTSIZE_ERROR && rSize != ZSTD_CONTENTSIZE_UNKNOWN;
}

DYCORE_API double DyCore_is_compressed(const char* str, double _sSize) {
    size_t sSize = (size_t)_sSize;
    return check_compressed(str, sSize) ? 0.0 : -1.0;
}

DYCORE_API const char* DyCore_decompress_string(const char* str,
                                                double _sSize) {
    size_t sSize = (size_t)_sSize;
    if (!check_compressed(str, sSize)) {
        std::cout << "[DyCore] Not a zstd file!" << std::endl;

        return "failed";
    }

    std::cout << "[DyCore] Start decompressing..." << std::endl;

    size_t const rSize = ZSTD_getFrameContentSize(str, sSize);
    char* const rBuff = new char[rSize];
    size_t const dSize = ZSTD_decompress(rBuff, rSize, str, sSize);

    if (dSize != rSize)
        return "failed";

    // Success
    memcpy(_return_buffer, rBuff, rSize);

    print_debug_message("No error found. Sucess.");

    delete[] rBuff;

    return _return_buffer;
}

DYCORE_API double DyCore_buffer_copy(void* dst, void* src, double size) {
    memcpy(dst, src, (size_t)size);
    return 0;
}

std::string get_file_modification_time(char* file_path) {
    namespace fs = std::filesystem;
    std::error_code ec;
    auto ftime = fs::last_write_time(fs::u8path(file_path), ec);

    if (ec) {
        std::cout << "Error reading file time: " << file_path << std::endl;
        return "failed";
    }

    auto sctp =
        std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() +
            std::chrono::system_clock::now());

    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    std::tm tm = *std::localtime(&tt);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S");
    return oss.str();
}

DYCORE_API const char* DyCore_get_file_modification_time(char* filePath) {
    static std::string ret;
    ret = get_file_modification_time(filePath);

    return ret.c_str();
}

/// Notes management

namespace dyn {
std::unordered_map<string, Note> currentNoteMap;

bool note_exists(const Note& note) {
    return currentNoteMap.find(note.inst) != currentNoteMap.end();
}

void clear_notes() {
    currentNoteMap.clear();
}

int insert_note(Note note) {
    if (note_exists(note)) {
        print_debug_message("Warning: the given note has existed." + note.inst);
        return -1;
    }

    currentNoteMap[note.inst] = note;
    // print_debug_message("Insert note at:" + std::to_string(note.time));
    return 0;
}

int delete_note(Note note) {
    if (!note_exists(note)) {
        print_debug_message("Warning: the given note is not existed." +
                            note.inst);
        return -1;
    }

    currentNoteMap.erase(note.inst);
    // print_debug_message("Delete note at:" + std::to_string(note.time));
    return 0;
}

int modify_note(Note note) {
    if (!note_exists(note)) {
        print_debug_message("Warning: the given note is not existed." +
                            note.inst);
        return -1;
    }

    currentNoteMap[note.inst] = note;
    return 0;
}

DYCORE_API double DyCore_sync_notes_array(const char* notesArray) {
    print_debug_message("Start syncing... ");

    json j;
    try {
        j = json::parse(notesArray);
    } catch (json::exception& e) {
        print_debug_message("Parse failed:" + string(e.what()));
        return -1;
    }

    print_debug_message("Parse finished.");

    auto array = j.template get<std::vector<Note>>();

    print_debug_message(string("Sync notes successfully. ") +
                        std::to_string(array.size()));
    for (auto note : array)
        insert_note(note);
    return 0;
}

DYCORE_API double DyCore_clear_notes() {
    clear_notes();
    return 0;
}

DYCORE_API double DyCore_insert_note(const char* prop) {
    json j;
    try {
        j = json::parse(prop);
    } catch (json::exception& e) {
        print_debug_message("Parse failed:" + string(e.what()));
        return -1;
    }

    Note note = j.template get<Note>();
    return insert_note(note);
}

DYCORE_API double DyCore_delete_note(const char* prop) {
    json j;
    try {
        j = json::parse(prop);
    } catch (json::exception& e) {
        print_debug_message("Parse failed:" + string(e.what()));
        return -1;
    }

    Note note = j.template get<Note>();
    return delete_note(note);
}

DYCORE_API double DyCore_modify_note(const char* prop) {
    json j;
    try {
        j = json::parse(prop);
    } catch (json::exception& e) {
        print_debug_message("Parse failed:" + string(e.what()));
        return -1;
    }

    Note note = j.template get<Note>();
    return modify_note(note);
}
}  // namespace dyn

// Insert the notes array into the project property.
DYCORE_API double DyCore_get_project_buffer(const char* projectProp,
                                            char* targetBuffer,
                                            double compressionLevel) {
    using namespace dyn;
    json js;
    try {
        js = json::parse(projectProp);
    } catch (json::exception& e) {
        print_debug_message("Parse failed:" + string(e.what()));
        return -1;
    }

    // Get the final notes array.
    std::vector<Note> notes;
    for (auto note : currentNoteMap)
        if (note.second.noteType != 3)
            notes.push_back(note.second);

    js["charts"]["notes"] = notes;

    string project = nlohmann::to_string(js);

    // print_debug_message("Final project json:\n" + project);

    return DyCore_compress_string(project.c_str(), targetBuffer,
                                  compressionLevel);
}