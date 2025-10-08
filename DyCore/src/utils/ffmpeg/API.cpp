#include "api.h"

#include "record.h"
#include "tools.h"

DYCORE_API double DyCore_ffmpeg_is_available() {
    return is_FFmpeg_available() ? 1.0 : 0.0;
}

DYCORE_API double DyCore_ffmpeg_start_recording(const char* filename,
                                                double width, double height,
                                                double fps) {
    auto recorder = get_recorder();

    return recorder.start_recording(filename, (int)width, (int)height,
                                    (int)fps);
}

DYCORE_API double DyCore_ffmpeg_push_frame(const char* data, double size) {
    auto recorder = get_recorder();

    recorder.push_frame(data, (int)size);
    return 0;
}

DYCORE_API double DyCore_ffmpeg_finish_recording() {
    auto recorder = get_recorder();

    recorder.finish_recording();
    return 0;
}