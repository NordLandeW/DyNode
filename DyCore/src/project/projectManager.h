#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <shared_mutex>

#include "json.hpp"
#include "project.h"

class ProjectManager {
   public:
    static ProjectManager &inst() {
        static ProjectManager instance;
        return instance;
    }

   private:
    mutable std::shared_mutex mtx;
    mutable std::mutex audioMtx;

    Project project;
    fs::path projectFilePath;
    fs::path projectDirPath;
    std::atomic<uint64_t> chartMusicLoadRequestId = 0;

    int currentChartIndex = 0;
    uint64_t chartMetadataLastModifiedTime = 0;
    bool is_current_chart_set();
    void check_current_chart_set();
    Chart &get_current_chart();

    void load_all_audio_data();

   public:
    ProjectManager() {
        // Create a default chart.
        project.charts.push_back(Chart{.metadata = {
                                           .title = "Last Train at 25 O'clock",
                                           .sideType = {"MIXER", "PAD"},
                                           .difficulty = 3,
                                       }});
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
    uint64_t get_chart_metadata_last_modified_time() const;
    void set_chart_path(const ChartPath &path);
    ChartPath get_chart_path();
    void set_project_metadata(const nlohmann::json &meta);
    nlohmann::json get_project_metadata() const;
    void set_version(const string &ver);
    string get_version() const;

    string get_project_path() const {
        return projectFilePath.string();
    }
    string get_project_dir_path() const {
        return projectDirPath.string();
    }

    string get_full_path(const char *relativePath) const;

    // Loads audio data for the current chart.
    int load_chart_audio(const char *filePath);
    void unload_chart_audio();

    std::string dump() const;
};
