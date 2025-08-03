#include "api.h"
#include "format/xml.h"
#include "gm.h"
#include "project.h"

// Saves the project to a file.
// This is the main entry point for saving a project.
//
// @param projectProp The project properties as a JSON string.
// @param filePath The path to save the project file.
// @param compressionLevel The compression level to use.
// @return 0 on success, -1 on error.
DYCORE_API double DyCore_save_project(const char* projectProp,
                                      const char* filePath,
                                      double compressionLevel) {
    namespace fs = std::filesystem;

    if (!filePath || strlen(filePath) == 0) {
        throw_error_event("File path is empty.");
        return -1;
    }

    fs::path path = fs::path((char8_t*)filePath);
    fs::path parentDir = path.parent_path();
    if (!parentDir.empty() && !fs::exists(parentDir)) {
        throw_error_event("Parent directory does not exist: " +
                          parentDir.string());
        return -1;
    }

    save_project(projectProp, filePath, compressionLevel);
    return 0;
}

DYCORE_API const char* DyCore_get_notes_array_string() {
    static string notesArrayString;
    notesArrayString = get_notes_array_string();
    return notesArrayString.c_str();
}

DYCORE_API double DyCore_project_import_xml(const char* filePath,
                                            double importInfo,
                                            double importTiming) {
    if (!filePath || strlen(filePath) == 0) {
        throw_error_event("File path is empty.");
        return -1;
    }

    return project_import_xml(filePath, importInfo > 0, importTiming > 0);
}

DYCORE_API const char* DyCore_get_chart_metadata() {
    static string chartMetadata;
    chartMetadata = project_get_metadata().dump();
    return chartMetadata.c_str();
}