#pragma once
#include <mutex>

#include "json.hpp"
#include "project.h"

class ProjectManager {
   private:
    mutable std::mutex mtx;
    Project project;
    int currentChartIndex = 0;
    bool is_current_chart_set();
    void check_current_chart_set();
    Chart &get_current_chart();

   public:
    ProjectManager() {
        // Create a default chart.
        project.charts.push_back(Chart());
    }

    void clear_project();
    void load_project(const Project &proj);
    void load_project_from_file(const char *filePath);
    int get_chart_count() const;
    void set_current_chart(int index);
    // Update timing points and notes to the current chart.
    void update_current_chart();

    /// Getters & Setters

    void set_chart_metadata(const ChartMetadata &meta);
    ChartMetadata get_chart_metadata();
    void set_chart_path(const ChartPath &path);
    ChartPath get_chart_path();
    void set_project_metadata(const nlohmann::json &meta);
    nlohmann::json get_project_metadata() const;
    void set_version(const string &ver);
    string get_version() const;

    std::string dump() const;
};

ProjectManager &get_project_manager();
