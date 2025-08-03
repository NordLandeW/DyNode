
#include "xml.h"

#include <exception>
#include <fstream>
#include <pugixml.hpp>

#include "gm.h"
#include "note.h"
#include "project.h"
#include "timing.h"
#include "utils.h"

using std::string;

IMPORT_XML_RESULT_STATES project_import_xml(const char* filePath,
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
        ChartMetaData metaData;
        metaData.sideType[0] = map_root.child_value("m_leftRegion");
        metaData.sideType[1] = map_root.child_value("m_rightRegion");
        string chartID = map_root.child_value("m_mapID");
        metaData.difficulty = difficulty_char_to_int(chartID.back());
        metaData.title = map_root.child_value("m_path");

        if (importMetadata)
            project_set_metadata(metaData);

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
                string newID = generate_note_id();

                newNote.noteID = newID;
                newNote.time = note.time;
                newNote.side = note.side;
                newNote.lastTime = 0;
                newNote.width = note.width;
                newNote.position = note.position;
                newNote.noteType = note.type;
                newNote.beginTime = note.time;

                if (note.type == 2) {
                    string newSubID = generate_note_id();
                    Note newSubNote(newNote);
                    newNote.lastTime = noteIDTimeMap[note.subid] - note.time;
                    newNote.subNoteID = newSubID;

                    newSubNote.noteType = 3;  // Sub note type
                    newSubNote.noteID = newSubID;
                    newSubNote.subNoteID = newID;
                    newSubNote.time = noteIDTimeMap[note.subid];
                    newSubNote.lastTime = 0;
                    newSubNote.beginTime = newNote.time;
                    insert_note(newSubNote);
                }
                insert_note(newNote);
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