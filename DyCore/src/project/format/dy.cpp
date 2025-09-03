
#include "dy.h"

#include <exception>
#include <fstream>
#include <json.hpp>
#include <string>

#include "gm.h"
#include "note.h"
#include "project.h"
#include "timing.h"
#include "utils.h"

using nlohmann::json;
using std::string;

double json_to_double(const json& j) {
    if (j.is_string()) {
        return std::stod(j.get<string>());
    }
    if (j.is_number()) {
        return j.get<double>();
    }
    throw std::runtime_error("Invalid type for double conversion");
}

string json_to_string(const json& j) {
    if (j.is_string()) {
        return j.get<string>();
    }
    if (j.is_number_integer()) {
        return std::to_string(j.get<long long>());
    }
    if (j.is_number_float()) {
        return std::to_string(j.get<double>());
    }
    if (j.is_null()) {
        return "";
    }
    throw std::runtime_error("Invalid type for string conversion: " + j.dump());
}

json lastRemix;

string get_dy_remix() {
    return lastRemix.dump();
}

IMPORT_DY_RESULT_STATES chart_import_dy(const char* filePath,
                                        bool importMetadata,
                                        bool importTiming) {
    // Read file into buffer.
    std::ifstream stream(convert_char_to_path(filePath));
    if (!stream.is_open()) {
        print_debug_message("Failed to open DY file: " + string(filePath));
        return IMPORT_DY_RESULT_STATES::FAILURE;
    }

    auto start = std::chrono::high_resolution_clock::now();

    try {
        json doc = json::parse(stream);
        stream.close();

        // Check for old Dynamaker format.
        if (doc.contains("DynamixMap")) {
            return IMPORT_DY_RESULT_STATES::OLD_FORMAT;
        }

        auto map_root = doc["CMap"];
        if (map_root.is_null()) {
            throw std::runtime_error(
                "Invalid DY structure: Missing <CMap> root element");
        }

        auto noteType_conversion = [](const string& type) -> int {
            if (type == "NORMAL")
                return 0;
            if (type == "CHAIN")
                return 1;
            if (type == "HOLD")
                return 2;
            if (type == "SUB")
                return 3;
            return -1;
        };

        // Read chart metadata.
        ChartMetadata metaData;
        metaData.sideType[0] = map_root["m_leftRegion"];
        metaData.sideType[1] = map_root["m_rightRegion"];
        string chartID = map_root["m_mapID"];
        metaData.difficulty = difficulty_char_to_int(chartID.back());
        metaData.title = map_root["m_path"];

        if (importMetadata)
            chart_set_metadata(metaData);

        double barPerMin = json_to_double(map_root["m_barPerMin"]);
        double offset = json_to_double(map_root["m_timeOffset"]);

        // Read note data.
        struct DYNoteData {
            string id, subid;
            int type, side;
            double bar, position, width, time;
        };
        struct DYTimingData {
            double time;
            double barPerMinute;
        };
        std::vector<DYNoteData> notes;

        auto import_side_notes = [&](const json& side_root, int side) {
            auto arrNode = side_root["m_notes"]["CMapNoteAsset"];
            if (!arrNode.is_array())
                return;
            for (auto noteNode : arrNode) {
                DYNoteData noteData;
                // Read
                noteData.id = json_to_string(noteNode["m_id"]);
                noteData.subid = json_to_string(noteNode["m_subId"]);
                noteData.type = noteType_conversion(noteNode["m_type"]);
                noteData.bar = json_to_double(noteNode["m_time"]);
                noteData.position = json_to_double(noteNode["m_position"]);
                noteData.width = json_to_double(noteNode["m_width"]);

                // Convert DY's position to DyNode's position.
                noteData.position = noteData.position + noteData.width / 2;
                noteData.id += "_" + std::to_string(side);
                noteData.subid += "_" + std::to_string(side);
                noteData.side = side;
                notes.push_back(noteData);
            }
        };

        import_side_notes(map_root["m_notes"], 0);
        import_side_notes(map_root["m_notesLeft"], 1);
        import_side_notes(map_root["m_notesRight"], 2);

        // Read timing data.
        bool hasTimingData = false;
        std::vector<DYTimingData> xmlTimings;
        if (map_root.contains("m_argument") &&
            map_root["m_argument"].contains("m_bpmchange")) {
            auto timingRootNode = map_root["m_argument"]["m_bpmchange"];
            for (auto timingNode : timingRootNode["CBpmchange"]) {
                xmlTimings.push_back({
                    json_to_double(timingNode["m_time"]),
                    json_to_double(timingNode["m_value"]),
                });
                hasTimingData = true;
            }

            std::sort(xmlTimings.begin(), xmlTimings.end(),
                      [](const DYTimingData& a, const DYTimingData& b) {
                          return a.time < b.time;
                      });
        }

        static auto bar_to_time = [](double offset, double barPerMinute) {
            return (offset * 60000.0) / barPerMinute;
        };

        if (importTiming) {
            auto& timingMan = get_timing_manager();
            timingMan.clear();
            if (hasTimingData) {
                double rTime = bar_to_time(-offset, barPerMin);
                for (int i = 0; i < xmlTimings.size(); i++) {
                    double nTime = xmlTimings[i].time;
                    if (i > 0) {
                        nTime = bar_to_time(nTime - xmlTimings[i - 1].time,
                                            xmlTimings[i - 1].barPerMinute) +
                                rTime;
                    } else {
                        nTime = rTime;
                    }
                    rTime = nTime;

                    TimingPoint tp;
                    tp.meter = 4;
                    tp.time = nTime;
                    tp.set_bpm(xmlTimings[i].barPerMinute * 4);
                    timingMan.add_timing_point(tp);
                }
            } else {
                timingMan.add_timing_point({bar_to_time(-offset, barPerMin),
                                            60000.0 / (barPerMin * 4), 4});
            }
            timingMan.sort();
        }

        // Fix every note's time.
        double fixedOffset = bar_to_time(offset, barPerMin);
        for (auto& note : notes) {
            if (xmlTimings.size() > 1) {
                double nTime = note.bar;
                double rTime = 0;
                for (int i = 1, l = xmlTimings.size(); i <= l; i++) {
                    if (i == l || xmlTimings[i].time > nTime) {
                        rTime += bar_to_time(nTime - xmlTimings[i - 1].time,
                                             xmlTimings[i - 1].barPerMinute);
                        break;
                    }
                    rTime +=
                        bar_to_time(xmlTimings[i].time - xmlTimings[i - 1].time,
                                    xmlTimings[i - 1].barPerMinute);
                }
                note.time = rTime;
            } else {
                note.time = bar_to_time(note.bar, barPerMin);
            }
            note.time -= fixedOffset;
        }

        // Build temporary noteID to time map.
        std::unordered_map<string, double> noteIDTimeMap;
        for (auto& note : notes) {
            noteIDTimeMap[note.id] = note.time;
        }

        // Add notes to the project.
        for (const auto& note : notes)
            if (note.type != 3) {
                Note newNote;
                newNote.time = note.time;
                newNote.side = note.side;
                newNote.lastTime = 0;
                newNote.width = note.width;
                newNote.position = note.position;
                newNote.type = note.type;
                newNote.beginTime = note.time;

                if (note.type == 2) {
                    newNote.lastTime = noteIDTimeMap[note.subid] - note.time;
                }
                create_note(newNote);
            }

        // Save remix part.
        lastRemix = doc["remix"];
    } catch (const std::exception& e) {
        print_debug_message("Exception occurred while importing DY: " +
                            string(e.what()));
        throw_error_event("DY import error: " + string(e.what()));
        return IMPORT_DY_RESULT_STATES::FAILURE;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    print_debug_message("DY import took: " + std::to_string(duration.count()) +
                        " seconds");

    return IMPORT_DY_RESULT_STATES::SUCCESS;
}
