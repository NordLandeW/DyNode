#include "api.h"

#include <json.hpp>

#include "ffmpeg/base.h"
#include "record.h"

DYCORE_API double DyCore_ffmpeg_is_available() {
    return is_FFmpeg_available() ? 1.0 : 0.0;
}

DYCORE_API double DyCore_ffmpeg_start_recording(const char *parameter) {
    auto &recorder = get_recorder();
    nlohmann::json j = nlohmann::json::parse(parameter);

    std::string filename = j.value("filename", "output.mp4");
    std::string musicPath = j.value("musicPath", "");
    int width = j.value("width", 1920);
    int height = j.value("height", 1080);
    int fps = j.value("fps", 60);
    double musicOffset = j.value("musicOffset", 0.0);

    return recorder.start_recording(filename, musicPath, width, height, fps,
                                    musicOffset);
}

DYCORE_API double DyCore_ffmpeg_push_frame(const char *data, double size) {
    auto &recorder = get_recorder();

    int rc = recorder.push_frame(data, (int)size);
    return static_cast<double>(rc);
}

DYCORE_API double DyCore_ffmpeg_finish_recording() {
    auto &recorder = get_recorder();

    recorder.finish_recording();
    return 0;
}