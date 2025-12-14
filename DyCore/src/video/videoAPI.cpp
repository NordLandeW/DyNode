#include "api.h"
#include "decoder.h"
#include "utils.h"

DYCORE_API double DyCore_video_open(const char *filePath) {
    auto &decoder = VideoDecoder::get_instance();

    if (!filePath || strlen(filePath) == 0) {
        print_debug_message("File path is empty.");
        return -1;
    }

    bool success = decoder.open(s2ws(std::string(filePath)).c_str());
    return success ? 0 : -1;
}

DYCORE_API double DyCore_video_close() {
    auto &decoder = VideoDecoder::get_instance();

    decoder.close();
    return 0;
}

DYCORE_API double DyCore_video_is_loaded() {
    auto &decoder = VideoDecoder::get_instance();
    return decoder.is_loaded() ? 1 : 0;
}

DYCORE_API double DyCore_video_is_playing() {
    auto &decoder = VideoDecoder::get_instance();
    return decoder.is_playing() ? 1 : 0;
}

DYCORE_API double DyCore_video_is_finished() {
    auto &decoder = VideoDecoder::get_instance();
    return decoder.is_finished() ? 1 : 0;
}

DYCORE_API double DyCore_video_pause() {
    auto &decoder = VideoDecoder::get_instance();
    decoder.set_pause(true);
    return 0;
}

DYCORE_API double DyCore_video_seek(double seconds) {
    auto &decoder = VideoDecoder::get_instance();
    decoder.seek(seconds);
    return 0;
}

DYCORE_API double DyCore_video_play() {
    auto &decoder = VideoDecoder::get_instance();
    decoder.set_pause(false);
    return 0;
}

DYCORE_API double DyCore_video_get_frame(void *gm_buffer_ptr,
                                         double buffer_size) {
    auto &decoder = VideoDecoder::get_instance();
    return decoder.get_frame(gm_buffer_ptr, buffer_size);
}

DYCORE_API double DyCore_video_get_frame_sync(void *gm_buffer_ptr,
                                              double buffer_size,
                                              double delta_time) {
    auto &decoder = VideoDecoder::get_instance();
    return decoder.get_frame_sync(gm_buffer_ptr, buffer_size, delta_time);
}

DYCORE_API double DyCore_video_get_width() {
    auto &decoder = VideoDecoder::get_instance();
    return decoder.get_width();
}

DYCORE_API double DyCore_video_get_height() {
    auto &decoder = VideoDecoder::get_instance();
    return decoder.get_height();
}

DYCORE_API double DyCore_video_get_duration() {
    auto &decoder = VideoDecoder::get_instance();
    return decoder.get_duration();
}

DYCORE_API double DyCore_video_get_buffer_size() {
    auto &decoder = VideoDecoder::get_instance();
    return static_cast<double>(decoder.get_width() * decoder.get_height() * 4);
}

DYCORE_API double DyCore_video_set_speed(double speed) {
    auto &decoder = VideoDecoder::get_instance();
    return decoder.set_speed(speed);
}

DYCORE_API double DyCore_video_get_speed() {
    auto &decoder = VideoDecoder::get_instance();
    return decoder.get_speed();
}

DYCORE_API double DyCore_video_set_sync_mode(double enable) {
    auto &decoder = VideoDecoder::get_instance();
    decoder.set_sync_mode(enable != 0.0);
    return 0;
}

DYCORE_API double DyCore_video_get_sync_mode() {
    auto &decoder = VideoDecoder::get_instance();
    return decoder.is_sync_mode() ? 1.0 : 0.0;
}