
#include "xml.h"

#include <exception>
#include <fstream>
#include <pugixml.hpp>
#include <vector>

#include "gm.h"
#include "note.h"
#include "project.h"
#include "timing.h"
#include "utils.h"
#include "xml.h"

using std::string;

IMPORT_XML_RESULT_STATES chart_import_xml(const char* filePath,
                                          bool importMetadata,
                                          bool importTiming) {
    // Read file into buffer.
    std::ifstream stream(convert_char_to_path(filePath));
    if (!stream.is_open()) {
        print_debug_message("Failed to open XML file: " + string(filePath));
        return IMPORT_XML_RESULT_STATES::FAILURE;
    }

    auto start = std::chrono::high_resolution_clock::now();

    try {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load(stream);
        if (!result) {
            throw std::runtime_error("Failed to parse XML file: " +
                                     string(filePath));
        }

        // Check for old Dynamaker format.
        if (doc.child("DynamixMap")) {
            return IMPORT_XML_RESULT_STATES::OLD_FORMAT;
        }

        auto map_root = doc.child("CMap");
        if (!map_root) {
            throw std::runtime_error(
                "Invalid XML structure: Missing <CMap> root element");
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
        metaData.sideType[0] = map_root.child_value("m_leftRegion");
        metaData.sideType[1] = map_root.child_value("m_rightRegion");
        string chartID = map_root.child_value("m_mapID");
        metaData.difficulty = difficulty_char_to_int(chartID.back());
        metaData.title = map_root.child_value("m_path");

        if (importMetadata)
            chart_set_metadata(metaData);

        double barPerMin = std::stod(map_root.child_value("m_barPerMin"));
        double offset = std::stod(map_root.child_value("m_timeOffset"));

        // Read note data.
        struct XMLNoteData {
            string id, subid;
            int type, side;
            double bar, position, width, time;
        };
        struct XMLTimingData {
            double time;
            double barPerMinute;
        };
        std::vector<XMLNoteData> notes;

        auto import_side_notes = [&](pugi::xml_node side_root, int side) {
            auto arrNode = side_root.child("m_notes");
            if (!arrNode)
                return;
            for (auto noteNode : arrNode.children("CMapNoteAsset")) {
                XMLNoteData noteData;
                // Read
                noteData.id = noteNode.child_value("m_id");
                noteData.subid = noteNode.child_value("m_subId");
                noteData.type =
                    noteType_conversion(noteNode.child_value("m_type"));
                noteData.bar = std::stod(noteNode.child_value("m_time"));
                noteData.position =
                    std::stod(noteNode.child_value("m_position"));
                noteData.width = std::stod(noteNode.child_value("m_width"));

                // Convert XML's position to DyNode's position.
                noteData.position = noteData.position + noteData.width / 2;
                noteData.id += "_" + std::to_string(side);
                noteData.subid += "_" + std::to_string(side);
                noteData.side = side;
                notes.push_back(noteData);
            }
        };

        import_side_notes(map_root.child("m_notes"), 0);
        import_side_notes(map_root.child("m_notesLeft"), 1);
        import_side_notes(map_root.child("m_notesRight"), 2);

        // Read timing data.
        bool hasTimingData = false;
        std::vector<XMLTimingData> xmlTimings;
        if (auto timingRootNode =
                map_root.child("m_argument").child("m_bpmchange")) {
            for (auto timingNode : timingRootNode.children("CBpmchange")) {
                xmlTimings.push_back({
                    std::stod(timingNode.child_value("m_time")),
                    std::stod(timingNode.child_value("m_value")),
                });
                hasTimingData = true;
            }

            std::sort(xmlTimings.begin(), xmlTimings.end(),
                      [](const XMLTimingData& a, const XMLTimingData& b) {
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
    } catch (const std::exception& e) {
        print_debug_message("Exception occurred while importing XML: " +
                            string(e.what()));
        throw_error_event("XML import error: " + string(e.what()));
        return IMPORT_XML_RESULT_STATES::FAILURE;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    print_debug_message("XML import took: " + std::to_string(duration.count()) +
                        " seconds");

    return IMPORT_XML_RESULT_STATES::SUCCESS;
}

void chart_export_xml(const char* filePath, bool isDym, double fixError) {
    const auto& fdwp = format_double_with_precision;
    pugi::xml_document doc;

    // Prolog
    auto prolog = doc.append_child(pugi::node_declaration);
    prolog.append_attribute("version") = "1.0";

    // Root
    auto root = doc.append_child("CMap");
    root.append_attribute("xmlns:xsi") =
        "http://www.w3.org/2001/XMLSchema-instance";
    root.append_attribute("xmlns:xsd") = "http://www.w3.org/2001/XMLSchema";

    // Metadata
    auto chartMetadata = chart_get_metadata();
    auto& timingMan = get_timing_manager();

    std::vector<TimingPoint> timingPoints;
    timingMan.get_timing_points(timingPoints);
    auto firstTp = timingPoints[0];
    double barPerMin = firstTp.get_bpm() / 4.0;
    double timeOffset = -firstTp.time;
    double barOffset = timeOffset * barPerMin / 60000;

    root.append_child("m_path").text().set(chartMetadata.title.c_str());
    root.append_child("m_barPerMin")
        .text()
        .set(fdwp(barPerMin, XML_EXPORT_EPS).c_str());
    root.append_child("m_timeOffset")
        .text()
        .set(fdwp(barOffset, XML_EXPORT_EPS).c_str());
    root.append_child("m_leftRegion")
        .text()
        .set(chartMetadata.sideType[0].c_str());
    root.append_child("m_rightRegion")
        .text()
        .set(chartMetadata.sideType[1].c_str());
    root.append_child("m_mapID").text().set(
        ("_map_" + chartMetadata.title + "_" +
         difficulty_int_to_char(chartMetadata.difficulty))
            .c_str());

    // Note processing
    struct ExportNote {
        int id;
        int subId = -1;
        double time;
        int type;
        double position;
        double width;
        int side;
    };

    std::vector<ExportNote> exportNotes;
    int noteIndex = 0;

    std::vector<Note> notes;
    get_notes_array(notes);
    std::sort(notes.begin(), notes.end(),
              [](const Note& a, const Note& b) { return a.time < b.time; });

    for (const auto& note : notes) {
        ExportNote en;
        en.id = noteIndex++;
        en.time = note.time;
        en.type = note.type;
        en.position = note.position;
        en.width = note.width;
        en.side = note.side;

        if (note.type == 2) {  // HOLD
            ExportNote subNote;
            subNote.id = noteIndex++;
            en.subId = subNote.id;
            subNote.time = note.time + note.lastTime;
            subNote.type = 3;  // SUB
            subNote.position = note.position;
            subNote.width = note.width;
            subNote.side = note.side;
            exportNotes.push_back(subNote);
        }
        exportNotes.push_back(en);
    }

    std::sort(exportNotes.begin(), exportNotes.end(),
              [](const ExportNote& a, const ExportNote& b) {
                  return a.time < b.time;
              });

    if (fixError > 0 && !exportNotes.empty()) {
        size_t group_start_index = 0;
        for (size_t i = 1; i < exportNotes.size(); ++i) {
            if (exportNotes[i].time - exportNotes[group_start_index].time >
                fixError) {
                if (i - group_start_index > 1) {
                    double first_note_time =
                        exportNotes[group_start_index].time;
                    for (size_t j = group_start_index + 1; j < i; ++j) {
                        exportNotes[j].time = first_note_time;
                    }
                }
                group_start_index = i;
            }
        }

        if (exportNotes.size() - group_start_index > 1) {
            double first_note_time = exportNotes[group_start_index].time;
            for (size_t j = group_start_index + 1; j < exportNotes.size();
                 ++j) {
                exportNotes[j].time = first_note_time;
            }
        }
    }

    // Time to bar conversion
    auto time_to_bar = [&](double time) {
        if (!isDym) {
            // Simple conversion for non-DyM formats
            return (time + timeOffset) * barPerMin / 60000.0;
        }

        // DyM format with multiple timing points
        double current_bar = 0;
        double last_time = 0;
        double last_bpm = timingPoints[0].get_bpm();

        for (const auto& tp : timingPoints) {
            if (time < tp.time) {
                break;
            }
            current_bar += (tp.time - last_time) * (last_bpm / 4.0) / 60000.0;
            last_time = tp.time;
            last_bpm = tp.get_bpm();
        }
        current_bar += (time - last_time) * (last_bpm / 4.0) / 60000.0;
        return current_bar;
    };

    auto noteType_to_string = [](int type) {
        switch (type) {
            case 0:
                return "NORMAL";
            case 1:
                return "CHAIN";
            case 2:
                return "HOLD";
            case 3:
                return "SUB";
        }
        return "";
    };

    // Create note nodes
    auto notes_middle_root =
        root.append_child("m_notes").append_child("m_notes");
    auto notes_left_root =
        root.append_child("m_notesLeft").append_child("m_notes");
    auto notes_right_root =
        root.append_child("m_notesRight").append_child("m_notes");

    for (const auto& note : exportNotes) {
        pugi::xml_node side_root;
        switch (note.side) {
            case 0:
                side_root = notes_middle_root;
                break;
            case 1:
                side_root = notes_left_root;
                break;
            case 2:
                side_root = notes_right_root;
                break;
        }

        auto note_node = side_root.append_child("CMapNoteAsset");
        note_node.append_child("m_id").text().set(note.id);
        note_node.append_child("m_type").text().set(
            noteType_to_string(note.type));
        note_node.append_child("m_time").text().set(
            fdwp(time_to_bar(note.time), XML_EXPORT_EPS).c_str());
        note_node.append_child("m_position")
            .text()
            .set(
                fdwp(note.position - note.width / 2.0, XML_EXPORT_EPS).c_str());
        note_node.append_child("m_width").text().set(
            fdwp(note.width, XML_EXPORT_EPS).c_str());
        note_node.append_child("m_subId").text().set(note.subId);
    }

    // Timing points for DyM
    if (isDym) {
        auto arg_root = root.append_child("m_argument");
        auto bpm_root = arg_root.append_child("m_bpmchange");

        double current_bar = 0;
        double last_time = 0;
        double last_bpm = timingPoints[0].get_bpm();

        for (const auto& tp : timingPoints) {
            current_bar += (tp.time - last_time) * (last_bpm / 4.0) / 60000.0;
            last_time = tp.time;
            last_bpm = tp.get_bpm();

            auto bpm_node = bpm_root.append_child("CBpmchange");
            bpm_node.append_child("m_time").text().set(
                fdwp(current_bar, XML_EXPORT_EPS).c_str());
            bpm_node.append_child("m_value").text().set(
                fdwp(tp.get_bpm() / 4.0, XML_EXPORT_EPS).c_str());
        }
    }

    // Save file
    std::ofstream stream(convert_char_to_path(filePath));
    if (!stream.is_open()) {
        throw_error_event("Failed to open XML file for writing: " +
                          string(filePath));
    }
    try {
        doc.save(stream);
    } catch (const std::exception& e) {
        throw_error_event("Failed to save XML file to " + string(filePath) +
                          ": " + e.what());
    }

    stream.close();
}
