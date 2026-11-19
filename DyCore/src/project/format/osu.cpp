#include "osu.h"

#include <charconv>
#include <cmath>
#include <fstream>
#include <set>
#include <string_view>

#include "gm.h"
#include "projectManager.h"
#include "utils.h"

namespace {
using std::string_view;
string_view trim(string_view text) {
    const auto first = text.find_first_not_of(" \t\r");
    return first == text.npos
               ? string_view{}
               : text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
}
string_view field(string_view& line) {
    auto end = line.find(',');
    auto value = trim(line.substr(0, end));
    line = end == line.npos ? string_view{} : line.substr(end + 1);
    return value;
}
template <typename T>
T number(string_view text) {
    T value{};
    if (text.empty())
        throw std::runtime_error("Missing osu! numeric field");
    if (text.front() == '+')
        text.remove_prefix(1);
    auto [end, error] =
        std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() ||
        !std::isfinite(value))
        throw std::runtime_error("Invalid osu! numeric field");
    return value;
}
}  // namespace

void chart_import_osu(const char* filePath, bool importHitObjects,
                      ChartImportOptions options) {
    std::ifstream stream(convert_char_to_path(filePath));
    if (!stream)
        throw std::runtime_error("Cannot open osu! beatmap");
    Chart chart{};
    std::string storage, section;
    bool header = false, chartSection = false;
    std::vector<double> duplicates;
    int mode = 0;
    std::set<double> offsets;
    while (std::getline(stream, storage)) {
        auto row = trim(storage);
        if (row.starts_with("\xef\xbb\xbf"))
            row.remove_prefix(3);
        if (row.empty() || row.starts_with("//"))
            continue;
        if (!header) {
            constexpr string_view prefix = "osu file format v";
            if (!row.starts_with(prefix) ||
                number<int>(trim(row.substr(prefix.size()))) < 1)
                throw std::runtime_error("Not an osu! beatmap");
            header = true;
        } else if (row.front() == '[' && row.back() == ']') {
            section = row;
            chartSection |= row == "[TimingPoints]" || row == "[HitObjects]";
        } else if (section == "[General]") {
            auto sep = row.find(':');
            if (sep != row.npos && trim(row.substr(0, sep)) == "Mode")
                mode = number<int>(trim(row.substr(sep + 1)));
        } else if (section == "[TimingPoints]") {
            const double time = number<double>(field(row));
            auto length = field(row);
            if (length == "NaN")
                continue;  // osu! also uses NaN for inherited speed points.
            const double beat = number<double>(length);
            if (beat <= 0)
                continue;  // Inherited speed changes are not beat lengths.
            const int meter = row.empty() ? 4 : number<int>(field(row));
            if (meter <= 0)
                throw std::runtime_error("Invalid osu! timing meter");
            auto next = offsets.lower_bound(time);
            if ((next != offsets.end() &&
                 *next - time < TIMING_POINT_EPSILON) ||
                (next != offsets.begin() &&
                 time - *std::prev(next) < TIMING_POINT_EPSILON) ||
                (!options.resetTiming &&
                 get_timing_manager().has_timing_point_at(time))) {
                duplicates.push_back(time);
                continue;
            }
            offsets.insert(next, time);
            chart.timingPoints.push_back({time, beat, meter});
        } else if (section == "[HitObjects]" && importHitObjects) {
            const double x = number<double>(field(row));
            field(row);  // Vertical position is not used on the front lane.
            const double time = number<double>(field(row));
            const int type = number<int>(field(row));
            field(row);  // Hit sound.
            if (time <= 0 || mode < 0 || mode > 3)
                continue;
            Note note{};
            note.beginTime = note.time = time;
            note.position = x / 512 * 5;
            note.width = 1;
            if (mode == 3 && (type & 128)) {
                auto tail = field(row);
                note.type = static_cast<int>(NOTE_TYPE::HOLD);
                note.lastTime =
                    number<double>(tail.substr(0, tail.find(':'))) - time;
            }
            chart.notes.push_back(std::move(note));
        }
    }
    if (!header || !chartSection || stream.bad())
        throw std::runtime_error("Incomplete osu! beatmap");
    ProjectManager::inst().import_chart(std::move(chart), false, true, options);
    for (double time : duplicates)
        gamemaker_announcement(GM_ANNOUNCEMENT_TYPE::ANNO_WARNING,
                               "timing_duplicate_error",
                               {std::to_string(time)});
}
