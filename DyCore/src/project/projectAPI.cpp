#include <cwctype>

#include "api.h"
#include "format/dy.h"
#include "format/dyn.h"
#include "format/osu.h"
#include "format/xml.h"
#include "gm.h"
#include "project.h"
#include "projectManager.h"
#include "utils.h"

// Invalidate unread saves and finish any active read before GML clears its
// pools.
DYCORE_API double DyCore_project_save_invalidate() {
    ProjectManager::inst().invalidate_pending_saves();
    return 0;
}

// Returns the accepted request ID, or -1 if no worker was dispatched.
DYCORE_API double DyCore_save_project_request(const char* filePath,
                                              double compressionLevel) {
    namespace fs = std::filesystem;
    try {
        if (!filePath || strlen(filePath) == 0) {
            throw std::runtime_error("File path is empty.");
        }
        const fs::path path = convert_char_to_path(filePath);
        const fs::path parentDir = path.parent_path();
        if (!parentDir.empty() && !fs::exists(parentDir)) {
            throw std::runtime_error("Parent directory does not exist.");
        }
        return static_cast<double>(save_project(filePath, compressionLevel));
    } catch (const std::exception& e) {
        throw_error_event(e.what());
        return -1;
    }
}

// Preserve the legacy return convention for existing native callers.
DYCORE_API double DyCore_save_project(const char* filePath,
                                      double compressionLevel) {
    return DyCore_save_project_request(filePath, compressionLevel) < 0 ? -1 : 0;
}

DYCORE_API const char* DyCore_get_notes_array_string() {
    static string notesArrayString;
    notesArrayString = get_notes_array_string();
    return notesArrayString.c_str();
}

// Parse the selected file completely before publishing the requested editor
// changes.
DYCORE_API double DyCore_import_chart_file(const char* path, double info,
                                           double timing, double flags) {
    try {
        if (!path || !*path)
            throw std::invalid_argument("File path is empty");
        auto extension = convert_char_to_path(path).extension().wstring();
        std::transform(
            extension.begin(), extension.end(), extension.begin(),
            [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        const int mode = static_cast<int>(flags);
        ChartImportOptions options{(mode & 1) != 0, (mode & 2) != 0,
                                   (mode & 4) != 0};
        if (extension == L".xml")
            return static_cast<double>(
                chart_import_xml(path, info > 0, timing > 0, options));
        if (extension == L".dy")
            return static_cast<double>(
                chart_import_dy(path, info > 0, timing > 0, options));
        if (extension == L".dyn")
            return chart_import_dyn(path, info > 0, timing > 0, options);
        if (extension != L".osu")
            throw std::invalid_argument("Unsupported chart format");
        chart_import_osu(path, info > 0, options);
        return 0;
    } catch (const std::exception& e) {
        gamemaker_announcement(GM_ANNOUNCEMENT_TYPE::ANNO_ERROR,
                               "anno_chart_parse_error", {e.what()});
        return -1;
    }
}

DYCORE_API double DyCore_chart_import_xml(const char* filePath,
                                          double importInfo,
                                          double importTiming) {
    if (!filePath || strlen(filePath) == 0) {
        throw_error_event("File path is empty.");
        return -1;
    }

    return (double)chart_import_xml(filePath, importInfo > 0, importTiming > 0);
}

DYCORE_API double DyCore_chart_import_dy(const char* filePath,
                                         double importInfo,
                                         double importTiming) {
    if (!filePath || strlen(filePath) == 0) {
        throw_error_event("File path is empty.");
        return -1;
    }

    return (double)chart_import_dy(filePath, importInfo > 0, importTiming > 0);
}

DYCORE_API const char* DyCore_chart_import_dy_get_remix() {
    static string remix;
    remix = get_dy_remix();
    return remix.c_str();
}

DYCORE_API double DyCore_project_load(const char* filePath) {
    try {
        load_project(filePath);
    } catch (const std::exception& e) {
        print_debug_message("Project load failed: " + string(e.what()));
        gamemaker_announcement(GM_ANNOUNCEMENT_TYPE::ANNO_ERROR,
                               "anno_project_load_failed", {e.what()});
        return -1;
    }
    return 0;
}

DYCORE_API double DyCore_chart_import_dyn(const char* filePath,
                                          double importInfo,
                                          double importTiming) {
    if (!filePath || strlen(filePath) == 0) {
        throw_error_event("File path is empty.");
        return -1;
    }

    return chart_import_dyn(filePath, importInfo > 0, importTiming > 0);
}

DYCORE_API double DyCore_chart_export_xml(const char* filePath, double isDym,
                                          double fixError) {
    if (!filePath || strlen(filePath) == 0) {
        throw_error_event("File path is empty.");
        return -1;
    }

    try {
        chart_export_xml(filePath, isDym > 0, fixError);
    } catch (const std::exception& e) {
        print_debug_message("Failed to export XML file to " + string(filePath) +
                            ": " + e.what());
        return -1;
    }

    return 0;
}

DYCORE_API const char* DyCore_get_chart_metadata() {
    static string chartMetadata;
    chartMetadata = nlohmann::json(chart_get_metadata()).dump();
    return chartMetadata.c_str();
}

DYCORE_API double DyCore_get_chart_metadata_last_modified_time() {
    return ProjectManager::inst().get_chart_metadata_last_modified_time();
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

DYCORE_API double DyCore_load_chart_audio(const char* filePath) {
    return chart_load_audio(filePath);
}
