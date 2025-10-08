
#macro RECORDING_FPS 60
#macro RECORDING_FPS_MAX 600

function RecordManager() constructor {

    recording = false;
    frameBuffer = -1;
    originalFPS = game_get_speed(gamespeed_fps);

    static _get_surface_buffer_size = function(surf) {
        return surface_get_width(surf) * surface_get_height(surf) * 4;
    }

    static start_recording = function(filename) {

        if(recording) {
            show_debug_message("-- Already recording!");
            return;
        }

        recording = true;
        global.timeManager.set_mode_fixed(RECORDING_FPS);
        originalFPS = game_get_speed(gamespeed_fps);
        game_set_speed(RECORDING_FPS_MAX, gamespeed_fps);
        var w = surface_get_width(application_surface);
        var h = surface_get_height(application_surface);
        var _fps = RECORDING_FPS;

        var err = DyCore_ffmpeg_start_recording(filename, w, h, _fps);
        if(err != 0) {
            show_debug_message("-- Failed to start recording. Error code: " + string(err));
        } else {
            show_debug_message("-- Recording started: " + filename);
        }

        frameBuffer = buffer_create(_get_surface_buffer_size(application_surface), buffer_grow, 1);
    }

    static push_frame = function() {
        if(!recording) return;

        buffer_get_surface(frameBuffer, application_surface, 0);
        
        DyCore_ffmpeg_push_frame(buffer_get_address(frameBuffer), buffer_get_size(frameBuffer));
    }

    static finish_recording = function() {
        if(!recording) {
            show_debug_message("-- Not recording!");
            return;
        }

        DyCore_ffmpeg_finish_recording();
        recording = false;
        buffer_delete(frameBuffer);
        frameBuffer = -1;
        global.timeManager.set_mode_default();
        game_set_speed(originalFPS, gamespeed_fps);
        show_debug_message("-- Recording finished.");
    }

}

function _debug_start_record() {
    global.recordManager.start_recording("test114514.mp4");
}

function _debug_stop_record() {
    global.recordManager.finish_recording();
    show_debug_message("Recording stopped.");
}