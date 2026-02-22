#include "projectManager.h"

#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

#include "format/dyn.h"
#include "note.h"
#include "notePoolManager.h"
#include "timer.h"
#include "timing.h"
#include "utils.h"

bool ProjectManager::is_current_chart_set() {
    return currentChartIndex != -1;
}

void ProjectManager::check_current_chart_set() {
    if (!is_current_chart_set()) {
        throw std::out_of_range("No current chart set");
    }
}

int ProjectManager::get_chart_count() const {
    return project.charts.size();
}

void ProjectManager::set_current_chart(int index) {
    if (index < 0 || index >= get_chart_count()) {
        throw std::out_of_range("Chart index out of range");
    }
    currentChartIndex = index;

    auto &currentChart = get_current_chart();
    // Set notes.
    get_note_pool_manager().clear_notes();
    for (auto &note : currentChart.notes) {
        create_note(note);
    }

    // Set timing points.
    get_timing_manager().clear();
    get_timing_manager().append_timing_points(currentChart.timingPoints);

    print_debug_message("Current chart set to: " + currentChart.metadata.title);
}

Chart &ProjectManager::get_current_chart() {
    check_current_chart_set();
    return project.charts[currentChartIndex];
}

void ProjectManager::clear_project() {
    std::lock_guard<std::shared_mutex> lock(mtx);
    project = Project();
    currentChartIndex = 0;
    chartMetadataLastModifiedTime = get_current_time();
}

// Loads audio data for all charts in the project.
void load_all_audio_data(Project &project) {
    TIMER_SCOPE("load_all_audio_data");
    std::unordered_map<std::string, AudioData> loadedAudioCache;
    std::unordered_set<std::string> failedAudioPaths;

    for (auto &chart : project.charts) {
        const auto &musicPath = project_get_full_path(chart.path.music.c_str());
        if (musicPath.empty()) {
            continue;
        }

        if (auto it = loadedAudioCache.find(musicPath);
            it != loadedAudioCache.end()) {
            chart.audioData = it->second;
            continue;
        }

        if (failedAudioPaths.find(musicPath) != failedAudioPaths.end()) {
            continue;
        }

        AudioData audioData;
        if (load_audio(musicPath.c_str(), audioData) == 0) {
            print_debug_message(
                "Loaded audio path: " + musicPath +
                ", totalSamples=" + std::to_string(audioData.pcmData.size()) +
                ", sampleRate=" + std::to_string(audioData.sampleRate) +
                ", channels=" + std::to_string(audioData.channels));
            chart.audioData = audioData;
            loadedAudioCache.emplace(musicPath, std::move(audioData));
        } else {
            failedAudioPaths.insert(musicPath);
            print_debug_message("Failed to load audio for chart: " +
                                chart.metadata.title);
        }
    }
}

void ProjectManager::load_project_from_file(const char *filePath) {
    clear_project();
    project_import_dyn(filePath, project);

    projectFilePath = convert_char_to_path(filePath);
    projectDirPath = projectFilePath.parent_path();

    load_all_audio_data(project);

    // Todo: (Future feature) Manually choose chart to start editing.
    set_current_chart(0);
}

void ProjectManager::update_current_chart() {
    std::lock_guard<std::shared_mutex> lock(mtx);
    check_current_chart_set();
    auto &chart = get_current_chart();
    // Update timing points.
    get_timing_manager().get_timing_points(chart.timingPoints);
    // Update notes.
    get_note_pool_manager().get_notes(chart.notes, true);
}

void ProjectManager::set_chart_metadata(const ChartMetadata &meta) {
    std::lock_guard<std::shared_mutex> lock(mtx);
    check_current_chart_set();
    get_current_chart().metadata = meta;

    chartMetadataLastModifiedTime = get_current_time();
}

ChartMetadata ProjectManager::get_chart_metadata() {
    std::shared_lock<std::shared_mutex> lock(mtx);
    check_current_chart_set();
    return get_current_chart().metadata;
}

uint64_t ProjectManager::get_chart_metadata_last_modified_time() const {
    return chartMetadataLastModifiedTime;
}

void ProjectManager::set_chart_path(const ChartPath &path) {
    std::lock_guard<std::shared_mutex> lock(mtx);
    check_current_chart_set();
    get_current_chart().path = path;
}

ChartPath ProjectManager::get_chart_path() {
    std::shared_lock<std::shared_mutex> lock(mtx);
    check_current_chart_set();
    return get_current_chart().path;
}

void ProjectManager::set_version(const string &ver) {
    std::lock_guard<std::shared_mutex> lock(mtx);
    project.version = ver;
}

string ProjectManager::get_version() const {
    std::shared_lock<std::shared_mutex> lock(mtx);
    return project.version;
}

string ProjectManager::get_full_path(const char *relativePath) const {
    // Judge if the path is already absolute.
    fs::path p = convert_char_to_path(relativePath);
    if (p.is_absolute()) {
        return p.string();
    }

    return (projectDirPath / fs::path(relativePath)).string();
}

int ProjectManager::load_chart_music(const char *filePath) {
    fs::path musicPath = get_full_path(filePath);
    AudioData &audioData = get_current_chart().audioData;
    if (load_audio(musicPath.string().c_str(), audioData) == 0) {
        print_debug_message(
            "Loaded audio path: " + musicPath.string() +
            ", totalSamples=" + std::to_string(audioData.pcmData.size()) +
            ", sampleRate=" + std::to_string(audioData.sampleRate) +
            ", channels=" + std::to_string(audioData.channels));
    } else {
        print_debug_message("Failed to load audio for chart: " +
                            get_current_chart().metadata.title);
        return -1;
    }
    return 0;
}

void ProjectManager::set_project_metadata(const nlohmann::json &meta) {
    std::lock_guard<std::shared_mutex> lock(mtx);
    project.metadata = meta;
}

nlohmann::json ProjectManager::get_project_metadata() const {
    std::shared_lock<std::shared_mutex> lock(mtx);
    return project.metadata;
}

std::string ProjectManager::dump() const {
    std::shared_lock<std::shared_mutex> lock(mtx);
    return nlohmann::json(project).dump();
}
