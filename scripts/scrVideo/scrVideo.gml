
function safe_video_init() {
    with(objMain) {
        bgVideoSurf = surface_create(BASE_RES_W, BASE_RES_H);
        surface_clear(bgVideoSurf);
        if(room_speed > VIDEO_FREQUENCY) {
            timesourceUpdateVideo =
        		time_source_create(time_source_game, 1/VIDEO_FREQUENCY, 
        		time_source_units_seconds,
        		function() {
        			if(bgVideoAlpha > EPS && nowPlaying && bgVideoDisplay)
        				safe_video_update();
        		}, [], -1);
        }
        else {
            timesourceUpdateVideo =
        		time_source_create(time_source_game, 1, 
        		time_source_units_frames,
        		function() {
        			if(bgVideoAlpha > EPS && nowPlaying && bgVideoDisplay)
        				safe_video_update();
        		}, [], -1);
        }
    	time_source_start(timesourceUpdateVideo);
    }
}

#macro VIDEO_PAUSE_AHEAD_ENDING 150

function safe_video_update() {
    with(objMain) {
        bgVideoSurf = surface_checkate(bgVideoSurf, BASE_RES_W, BASE_RES_H);
        
        if(bgVideoAlpha < EPS || !bgVideoLoaded)
            return false;
        if(video_get_status() <= 1)
            return false;

        if(nowPlaying && nowTime > bgVideoLength - VIDEO_PAUSE_AHEAD_ENDING)
            safe_video_pause();
        else if(nowPlaying && editor_get_editmode() == 5 && bgVideoPaused)
            safe_video_seek_to(nowTime);
        
        surface_set_target(bgVideoSurf);
            draw_clear_alpha(c_black, 0);
            var _status = video_draw();
            if(_status[0] == -1) {
                // announcement_error("video_playback_error");
                //! Bug in runtime 2023.11
                //! Occasional error message will occur.
                show_debug_message_safe("VIDEO PLAYBACK ERROR.");
                // safe_video_free();
                // bgVideoLoaded = false;
            }
            else if(_status[0] == 0 && surface_exists(_status[1])) {
                var _w = surface_get_width(_status[1]), _h = surface_get_height(_status[1]);
                var _wscl = BASE_RES_W / _w;
                var _hscl = BASE_RES_H / _h;
                var _scl = max(_wscl, _hscl); // Centre & keep ratios
                var _nx = BASE_RES_W/2 - _scl * _w / 2;
                var _ny = BASE_RES_H/2 - _scl * _h / 2;
                draw_surface_ext(_status[1], _nx, _ny, _scl, _scl, 0, c_white, 1);
            }
        surface_reset_target();
    }
    return true;
}

function safe_video_draw(x, y, alp) {

    with(objMain) {
        if(!surface_exists(bgVideoSurf))
            safe_video_update();
		if(bgVideoLoaded)
			draw_surface_ext(bgVideoSurf, 0, 0, 1, 1, 0, c_white, alp);
    }
}

function safe_video_free() {
    with(objMain) {
        bgVideoPaused = false;
        bgVideoLoaded = false;
        bgVideoReloading = false;
        bgVideoDisplay = false;
        
        surface_free_f(bgVideoSurf);
        bgVideoDestroying = true;

        //! Bug in runtime 2024.2: sometimes video_close will not work as expected.
        video_close();
        
        show_debug_message("VIDEO FREE!!");
    }
    
}

function safe_video_seek_to(time) {
    if(editor_get_editmode() != 5)
        return;
    video_seek_to(time);
    safe_video_resume();
    show_debug_message("VIDEO SEEKING TO: "+ string(time));
}

// If the video is loaded (or being reloading)
function safe_video_check_loaded() {
    return objMain.bgVideoLoaded || objMain.bgVideoReloading;
}

function safe_video_pause() {
    if(objMain.bgVideoPaused) return;
    objMain.bgVideoPaused = true;
    video_pause();
    show_debug_message("VIDEO PAUSED.");
}

function safe_video_resume() {
    if(!objMain.bgVideoPaused) return;
    objMain.bgVideoPaused = false;
    video_resume();
    show_debug_message("VIDEO RESUMED.");
}