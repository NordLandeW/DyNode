
#macro RECORDING_FPS 60
#macro RECORDING_FPS_MAX 600
#macro RECORDING_RESOLUTION_W 1920
#macro RECORDING_RESOLUTION_H 1080

function RecordManager() constructor {

    prepareRecording = false;
    recording = false;
    frameBuffer = -1;
    originalFPS = game_get_speed(gamespeed_fps);
    targetFilePath = "";

    static _get_surface_buffer_size = function(w, h) {
        return w * h * 4;
    }

    static start_recording = function(filename) {

        prepareRecording = false;

        if(recording) {
            show_debug_message("-- Already recording!");
            return;
        }

        if(!DyCore_ffmpeg_is_available()) {
            show_debug_message("-- FFMPEG not available!");
            // TODO: Show announcement.
            return;
        }

        recording = true;
        global.timeManager.set_mode_fixed(RECORDING_FPS);
        originalFPS = game_get_speed(gamespeed_fps);
        game_set_speed(RECORDING_FPS_MAX, gamespeed_fps);
        resolution_set(RECORDING_RESOLUTION_W, RECORDING_RESOLUTION_H, false);
        display_reset(max(global.graphics.VSync, 2), false);
        targetFilePath = filename;
        
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

        frameBuffer = buffer_create(_get_surface_buffer_size(w, h), buffer_fast, 1);
        show_debug_message($"-- Frame buffer created. Size: {_get_surface_buffer_size(w, h)} bytes");
        show_debug_message($"-- Recording at {w}x{h} @ {_fps} FPS");
        show_debug_message("-- Music path: " + musicPath);

        global.__InputManager.freeze();
    }

    static push_frame = function() {
        if(!recording) return;

        buffer_get_surface(frameBuffer, application_surface, 0);
        
        var err = DyCore_ffmpeg_push_frame(buffer_get_address(frameBuffer), buffer_get_size(frameBuffer));

        if(err < 0) {
            show_debug_message("-- Failed to push frame. Error code: " + string(err));
            announcement_task(i18n_get("recording_failed", [err]), 5000, "recording", ANNO_STATE.ERROR);
            global.recordManager.abort_recording();
            return;
        }

        announcement_task(
            i18n_get("recording_processing", 
                [RECORDING_RESOLUTION_W, RECORDING_RESOLUTION_H, RECORDING_FPS, 100 * (max(objMain.nowTime, 0) / objMain.musicLength),
                DyCore_ffmpeg_get_using_decoder()]
            ), 5000, "recording");
    }

    static abort_recording = function() {
        if(!recording) {
            show_debug_message("-- Not recording!");
            return;
        }
        // No need to call `DyCore_ffmpeg_finish_recording`.
        recording = false;
        post_recording();
        show_debug_message("-- Recording aborted.");
    }

    static finish_recording = function() {
        if(!recording) {
            show_debug_message("-- Not recording!");
            return;
        }

        DyCore_ffmpeg_finish_recording();
        recording = false;
        post_recording();
        show_debug_message("-- Recording finished.");
        announcement_task(i18n_get("recording_complete", [targetFilePath]), 5000, "recording", ANNO_STATE.COMPLETE);
    }

    static post_recording = function() {
        buffer_delete(frameBuffer);
        frameBuffer = -1;
        global.timeManager.set_mode_default();
        game_set_speed(originalFPS, gamespeed_fps);
        display_reset(global.graphics.AA, global.graphics.VSync);

        global.__InputManager.unfreeze();
    }

    static is_recording = function() {
        return recording || prepareRecording;
    }

}

function recording_default_filename() {
    return $"{dyc_chart_get_title()}_{difficulty_num_to_name(dyc_chart_get_difficulty())}_recording.mp4";
}

function recording_start(filename = "") {
    if(!DyCore_ffmpeg_is_available()) {
        announcement_warning("recording_no_ffmpeg");
        return;
    }


    if(filename == "") {
        var defaultFilename = recording_default_filename();
        filename = dyc_get_save_filename("Video File (*.mp4)|*.mp4", defaultFilename, objManager.projectPath, i18n_get("recording_savefile_dlg_title"));
    }
    if(filename == "") {
        show_debug_message("-- Recording cancelled (no filename).");
        return;
    }

    global.recordManager.prepareRecording = true;
    playview_start_replay(method({
        filename: filename
    }, function() {
        global.recordManager.start_recording(filename);
    }));
}

function _debug_start_record() {
    playview_start_replay(function() {
        global.recordManager.start_recording("test114514.mp4");
    });
}

function _debug_stop_record() {
    global.recordManager.finish_recording();
    show_debug_message("Recording stopped.");
}