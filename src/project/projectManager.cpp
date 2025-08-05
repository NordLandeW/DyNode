#include "projectManager.h"

#include "format/dyn.h"
#include "note.h"
#include "notePoolManager.h"
#include "timing.h"
#include "utils.h"

ProjectManager &get_project_manager() {
    static ProjectManager instance;
    return instance;
}

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
    std::lock_guard<std::mutex> lock(mtx);
    project = Project();
    currentChartIndex = 0;
}

void ProjectManager::load_project_from_file(const char *filePath) {
    clear_project();
    project_import_dyn(filePath, project);

    // Todo: (Future feature) Manually choose chart to start editing.
    set_current_chart(0);
}

void ProjectManager::update_current_chart() {
    std::lock_guard<std::mutex> lock(mtx);
    check_current_chart_set();
    auto &chart = get_current_chart();
    // Update timing points.
    get_timing_manager().get_timing_points(chart.timingPoints);
    // Update notes.
    get_note_pool_manager().get_notes(chart.notes);
}

void ProjectManager::set_chart_metadata(const ChartMetadata &meta) {
    std::lock_guard<std::mutex> lock(mtx);
    check_current_chart_set();
    get_current_chart().metadata = meta;
}

ChartMetadata ProjectManager::get_chart_metadata() {
    std::lock_guard<std::mutex> lock(mtx);
    check_current_chart_set();
    return get_current_chart().metadata;
}

void ProjectManager::set_chart_path(const ChartPath &path) {
    std::lock_guard<std::mutex> lock(mtx);
    check_current_chart_set();
    get_current_chart().path = path;
}

ChartPath ProjectManager::get_chart_path() {
    std::lock_guard<std::mutex> lock(mtx);
    check_current_chart_set();
    return get_current_chart().path;
}

void ProjectManager::set_version(const string &ver) {
    std::lock_guard<std::mutex> lock(mtx);
    project.version = ver;
}

string ProjectManager::get_version() const {
    std::lock_guard<std::mutex> lock(mtx);
    return project.version;
}

void ProjectManager::set_project_metadata(const nlohmann::json &meta) {
    std::lock_guard<std::mutex> lock(mtx);
    project.metadata = meta;
}

nlohmann::json ProjectManager::get_project_metadata() const {
    std::lock_guard<std::mutex> lock(mtx);
    return project.metadata;
}

std::string ProjectManager::dump() const {
    std::lock_guard<std::mutex> lock(mtx);
    return nlohmann::json(project).dump();
}
