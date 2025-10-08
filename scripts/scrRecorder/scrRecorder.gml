
#macro RECORDING_FPS 60
#macro RECORDING_FPS_MAX 600
#macro RECORDING_RESOLUTION_W 1920
#macro RECORDING_RESOLUTION_H 1080

function RecordManager() constructor {

    recording = false;
    frameBuffer = -1;
    originalFPS = game_get_speed(gamespeed_fps);

    static _get_surface_buffer_size = function(w, h) {
        return w * h * 4;
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
        resolution_set(RECORDING_RESOLUTION_W, RECORDING_RESOLUTION_H, false);
        
        var w = RECORDING_RESOLUTION_W;
        var h = RECORDING_RESOLUTION_H;
        var _fps = RECORDING_FPS;
        var musicPath = "";

        // Get music path.
        if(instance_exists(objMain)) {
            musicPath = get_absolute_path(filename_path(objManager.projectPath), objManager.musicPath);
        }

        var err = dyc_ffmpeg_start_recording(filename, musicPath, w, h, _fps, PLAYBACK_EMPTY_TIME / 1000);
        if(err != 0) {
            show_debug_message("-- Failed to start recording. Error code: " + string(err));
        } else {
            show_debug_message("-- Recording started: " + filename);
        }

        frameBuffer = buffer_create(_get_surface_buffer_size(w, h), buffer_grow, 1);
        show_debug_message($"-- Frame buffer created. Size: {_get_surface_buffer_size(w, h)} bytes");
        show_debug_message($"-- Recording at {w}x{h} @ {_fps} FPS");
        show_debug_message("-- Music path: " + musicPath);
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

    static is_recording = function() {
        return recording;
    }

}

function _debug_start_record() {
    global.recordManager.start_recording("test114514.mp4");
    playview_start_replay();
    playview_pause_and_resume(true);
}

function _debug_stop_record() {
    global.recordManager.finish_recording();
    show_debug_message("Recording stopped.");
}