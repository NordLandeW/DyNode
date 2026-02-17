
#include "xml.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <pugixml.hpp>
#include <vector>

#include "gm.h"
#include "import_common.h"
#include "note.h"
#include "project.h"
#include "timing.h"
#include "utils.h"
#include "xml.h"

using std::string;

namespace {

struct ExportNote {
    int id;
    int subId = -1;
    double time;
    int type;
    double position;
    double width;
    int side;
};

std::vector<ExportNote> collect_export_notes() {
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

        if (note.get_note_type() == NOTE_TYPE::HOLD) {
            ExportNote subNote;
            subNote.id = noteIndex++;
            en.subId = subNote.id;
            subNote.time = note.time + note.lastTime;
            subNote.type = 3;
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

    return exportNotes;
}

void apply_note_time_fix(std::vector<ExportNote>& exportNotes, double fixError) {
    if (fixError <= 0 || exportNotes.empty()) {
        return;
    }

    size_t group_start_index = 0;
    for (size_t i = 1; i < exportNotes.size(); ++i) {
        if (exportNotes[i].time - exportNotes[group_start_index].time >
            fixError) {
            if (i - group_start_index > 1) {
                double first_note_time = exportNotes[group_start_index].time;
                for (size_t j = group_start_index + 1; j < i; ++j) {
                    exportNotes[j].time = first_note_time;
                }
            }
            group_start_index = i;
        }
    }

    if (exportNotes.size() - group_start_index > 1) {
        double first_note_time = exportNotes[group_start_index].time;
        for (size_t j = group_start_index + 1; j < exportNotes.size(); ++j) {
            exportNotes[j].time = first_note_time;
        }
    }
}

class TimeToBarConverter {
public:
    TimeToBarConverter(bool isDym,
                       double timeOffset,
                       double barPerMin,
                       const std::vector<TimingPoint>& timingPoints)
        : isDym_(isDym),
          timeOffset_(timeOffset),
          barPerMin_(barPerMin),
          timingPoints_(timingPoints) {}

    double operator()(double time) const {
        if (!isDym_) {
            return (time + timeOffset_) * barPerMin_ / 60000.0;
        }

        double current_bar = 0;
        double last_time = timingPoints_[0].time;
        double last_bpm = timingPoints_[0].get_bpm();

        for (const auto& tp : timingPoints_) {
            if (time < tp.time) {
                break;
            }
            current_bar += (tp.time - last_time) * (last_bpm / 4.0) / 60000.0;
            last_time = tp.time;
            last_bpm = tp.get_bpm();
        }

        current_bar += (time - last_time) * (last_bpm / 4.0) / 60000.0;
        return current_bar;
    }

private:
    bool isDym_;
    double timeOffset_;
    double barPerMin_;
    const std::vector<TimingPoint>& timingPoints_;
};

const char* note_type_to_string(int type) {
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
}

pugi::xml_node get_side_root(int side,
                             pugi::xml_node notes_middle_root,
                             pugi::xml_node notes_left_root,
                             pugi::xml_node notes_right_root) {
    switch (side) {
        case 0:
            return notes_middle_root;
        case 1:
            return notes_left_root;
        case 2:
            return notes_right_root;
    }
    return {};
}

void append_note_nodes(pugi::xml_node root,
                       const std::vector<ExportNote>& exportNotes,
                       const TimeToBarConverter& timeToBar) {
    auto notes_middle_root =
        root.append_child("m_notes").append_child("m_notes");
    auto notes_left_root =
        root.append_child("m_notesLeft").append_child("m_notes");
    auto notes_right_root =
        root.append_child("m_notesRight").append_child("m_notes");

    for (const auto& note : exportNotes) {
        auto side_root = get_side_root(note.side, notes_middle_root,
                                       notes_left_root, notes_right_root);

        auto note_node = side_root.append_child("CMapNoteAsset");
        note_node.append_child("m_id").text().set(note.id);
        note_node.append_child("m_type").text().set(note_type_to_string(note.type));
        note_node.append_child("m_time").text().set(
            format_double_with_precision(timeToBar(note.time), XML_EXPORT_EPS)
                .c_str());
        note_node.append_child("m_position")
            .text()
            .set(format_double_with_precision(note.position - note.width / 2.0,
                                              XML_EXPORT_EPS)
                     .c_str());
        note_node.append_child("m_width").text().set(
            format_double_with_precision(note.width, XML_EXPORT_EPS).c_str());
        note_node.append_child("m_subId").text().set(note.subId);
    }
}

void append_timing_nodes_for_dym(pugi::xml_node root,
                                 bool isDym,
                                 const std::vector<TimingPoint>& timingPoints) {
    if (!isDym) {
        return;
    }

    auto arg_root = root.append_child("m_argument");
    auto bpm_root = arg_root.append_child("m_bpmchange");

    double current_bar = 0;
    double last_time = timingPoints[0].time;
    double last_bpm = timingPoints[0].get_bpm();

    for (const auto& tp : timingPoints) {
        current_bar += (tp.time - last_time) * (last_bpm / 4.0) / 60000.0;
        last_time = tp.time;
        last_bpm = tp.get_bpm();

        auto bpm_node = bpm_root.append_child("CBpmchange");
        bpm_node.append_child("m_time").text().set(
            format_double_with_precision(current_bar, XML_EXPORT_EPS).c_str());
        bpm_node.append_child("m_value").text().set(
            format_double_with_precision(tp.get_bpm() / 4.0, XML_EXPORT_EPS)
                .c_str());
    }
}

}  // namespace

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
        std::vector<ImportedNoteData> notes;

        auto import_side_notes = [&](pugi::xml_node side_root, int side) {
            auto arrNode = side_root.child("m_notes");
            if (!arrNode)
                return;
            for (auto noteNode : arrNode.children("CMapNoteAsset")) {
                ImportedNoteData noteData;
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
        std::vector<ImportedTimingData> xmlTimings;
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
                      [](const ImportedTimingData& a,
                         const ImportedTimingData& b) {
                          return a.time < b.time;
                      });
        }

        import_timing_points(importTiming, hasTimingData, xmlTimings, offset,
                             barPerMin);
        fix_imported_note_times(notes, xmlTimings, offset, barPerMin);
        const auto noteIDTimeMap = build_note_id_time_map(notes);
        add_imported_notes_to_project(notes, noteIDTimeMap);
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

    auto exportNotes = collect_export_notes();
    apply_note_time_fix(exportNotes, fixError);
    TimeToBarConverter timeToBar(isDym, timeOffset, barPerMin, timingPoints);
    append_note_nodes(root, exportNotes, timeToBar);
    append_timing_nodes_for_dym(root, isDym, timingPoints);

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
