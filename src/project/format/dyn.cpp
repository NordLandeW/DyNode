#include "dyn.h"

#include <fstream>
#include <json.hpp>
#include <string>

#include "compress.h"
#include "gm.h"
#include "note.h"
#include "project.h"
#include "timing.h"
#include "utils.h"

using std::string;
using json = nlohmann::json;

json conversion_v0_to_v1(const json& input) {
    print_debug_message("Converting DYN v0 to v1 format...");
    const json& origProjectObject = input;
    json output = json::object();
    // Deal with projectObject.
    {
        print_debug_message("  Converting project object...");
        output["version"] = origProjectObject["version"];

        // Add formatVersion.
        output["formatVersion"] = 1;

        // Add metadata. Specifically for DyNode pre v0.1.19.
        output["metadata"] = json::object();
        output["metadata"]["settings"] = origProjectObject["settings"];
        output["metadata"]["stats"] = json::object();
        output["metadata"]["stats"]["projectTime"] =
            origProjectObject["projectTime"];
    }
    // Deal with chartObject.
    print_debug_message("  Converting chart object...");
    json origChartObject = origProjectObject["charts"];
    json chartObject = json::object();
    {
        // Deal with chartMetadataObject.
        json chartMetadataObject = json::object();
        chartMetadataObject["title"] = origChartObject["title"];
        chartMetadataObject["sideType"] = origChartObject["sidetype"];
        chartMetadataObject["difficulty"] = origChartObject["difficulty"];
        chartMetadataObject["artist"] = "";
        chartMetadataObject["charter"] = "";

        // Deal with chartPathObject.
        json chartPathObject = json::object();
        chartPathObject["music"] = origProjectObject["musicPath"];
        chartPathObject["image"] = origProjectObject["backgroundPath"];
        chartPathObject["video"] = origProjectObject["videoPath"];

        // Deal with noteObjects list.
        json noteObjects = json::array();
        for (const auto& note : origChartObject["notes"]) {
            json noteObject = note;
            noteObject["type"] = note["noteType"];
            noteObject["length"] = note["lastTime"];
            noteObject.erase("lastTime");
            noteObject.erase("noteType");
            noteObjects.push_back(noteObject);
        }

        // Deal with timingPointObjects list.
        json timingPointObjects = json::array();
        for (const auto& timingPoint : origProjectObject["timingPoints"]) {
            json timingPointObject = json::object();
            timingPointObject["offset"] = timingPoint["time"];
            timingPointObject["bpm"] =
                60000 / timingPoint["beatLength"].get<double>();
            timingPointObject["meter"] = timingPoint["meter"];
            timingPointObjects.push_back(timingPointObject);
        }

        chartObject["metadata"] = chartMetadataObject;
        chartObject["path"] = chartPathObject;
        chartObject["notes"] = noteObjects;
        chartObject["timingPoints"] = timingPointObjects;
    }

    output["charts"] = json::array();
    output["charts"].push_back(chartObject);

    print_debug_message("v0 to v1 conversion complete.");

    return output;
}

json dyn_old_format_conversion(json input) {
    int currentVersion = -1;
    if (!input.contains("formatVersion"))
        currentVersion = 0;
    else
        currentVersion = input["formatVersion"].get<int>();

    while (true) {
        switch (currentVersion) {
            case 0:
                input = conversion_v0_to_v1(input);
                currentVersion = 1;
                break;
            case DYN_FILE_FORMAT_VERSION:
                print_debug_message("All conversion complete.");
                // print_debug_message("Conversion result: \n" + input.dump());
                return input;
            default:
                throw std::runtime_error("Unknown format version");
        }
    }
}

bool dyn_old_format_check(const json& input) {
    // v0 version
    if (!input.contains("formatVersion"))
        return true;
    return false;
}

// This function is for reading the entire project.
int project_import_dyn(const char* filePath, Project& project) {
    json projectJson;

    std::ifstream stream(convert_char_to_path(filePath), std::ios::binary);
    if (!stream.is_open()) {
        print_debug_message("Failed to open DYN file: " + string(filePath));
        return -1;
    }
    print_debug_message("Opened DYN file: " + string(filePath));

    std::string content;
    content.assign((std::istreambuf_iterator<char>(stream)),
                   std::istreambuf_iterator<char>());
    stream.close();

    if (check_compressed(content.c_str(), content.size())) {
        print_debug_message("Decompressing DYN file...");
        string decompressed =
            decompress_string(content.c_str(), content.size());
        print_debug_message("Decompression complete. Parsing...");
        projectJson = json::parse(decompressed);
    } else {
        print_debug_message("Not a compressed DYN file. Directly parsing...");
        projectJson = json::parse(content);
    }

    print_debug_message("Successfully parsed DYN file.");

    // Check old version.
    try {
        if (dyn_old_format_check(projectJson)) {
            print_debug_message("Detected old DYN format, converting...");
            projectJson = dyn_old_format_conversion(projectJson);
        }
    } catch (const std::exception& e) {
        print_debug_message("Error importing DYN file: " + string(e.what()));
        gamemaker_announcement(GM_ANNOUNCEMENT_TYPE::ANNO_ERROR,
                               "dyn_old_format_conversion_failed", {e.what()});
    }

    projectJson.get_to<Project>(project);
    return 0;
}

// This function is for importing charts.
int chart_import_dyn(const char* filePath, bool importInfo, bool importTiming) {
    try {
        Project project;
        project_import_dyn(filePath, project);

        // Import notes from the first chart.
        const Chart& chart = project.charts[0];
        for (const Note& note : chart.notes) {
            create_note(note);
        }

        // Import chart information if requested.
        if (importInfo) {
            chart_set_metadata(chart.metadata);
        }

        // Import timing information if requested.
        if (importTiming) {
            get_timing_manager().append_timing_points(chart.timingPoints);
        }
    } catch (const std::exception& e) {
        gamemaker_announcement(GM_ANNOUNCEMENT_TYPE::ANNO_ERROR,
                               "dyn_chart_import_failed", {e.what()});
    }

    return 0;
}