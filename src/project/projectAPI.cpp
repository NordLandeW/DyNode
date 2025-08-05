#include "api.h"
#include "format/xml.h"
#include "gm.h"
#include "project.h"
#include "utils.h"

// Saves the project to a file.
// This is the main entry point for saving a project.
//
// @param filePath The path to save the project file.
// @param compressionLevel The compression level to use.
// @return 0 on success, -1 on error.
DYCORE_API double DyCore_save_project(const char* filePath,
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

    save_project(filePath, compressionLevel);
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

DYCORE_API double DyCore_project_load(const char* filePath) {
    try {
        load_project(filePath);
    } catch (const std::exception& e) {
        gamemaker_announcement(ANNO_ERROR, "anno_project_load_failed",
                               {e.what()});
        print_debug_message("Project load failed: " + string(e.what()));
        return -1;
    }
    return 0;
}

DYCORE_API const char* DyCore_get_chart_metadata() {
    static string chartMetadata;
    chartMetadata = nlohmann::json(chart_get_metadata()).dump();
    return chartMetadata.c_str();
}

DYCORE_API const char* DyCore_get_chart_path() {
    static string chartPath;
    chartPath = nlohmann::json(chart_get_path()).dump();
    return chartPath.c_str();
}

DYCORE_API const char* DyCore_get_project_metadata() {
    static string projectMetadata;
    projectMetadata = project_get_metadata().dump();
    return projectMetadata.c_str();
}

DYCORE_API const char* DyCore_get_project_version() {
    static string projectVersion;
    projectVersion = project_get_version();
    return projectVersion.c_str();
}

DYCORE_API double DyCore_set_chart_metadata(const char* chartMetadataJson) {
    chart_set_metadata(nlohmann::json::parse(chartMetadataJson));
    return 0;
}

DYCORE_API double DyCore_set_chart_path(const char* chartPathJson) {
    chart_set_path(nlohmann::json::parse(chartPathJson));
    return 0;
}

DYCORE_API double DyCore_set_project_metadata(const char* projectMetadataJson) {
    project_set_metadata(nlohmann::json::parse(projectMetadataJson));
    return 0;
}

DYCORE_API double DyCore_set_project_version(const char* projectVersion) {
    project_set_version(projectVersion);
    return 0;
}
