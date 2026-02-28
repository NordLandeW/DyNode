
function window_on_files_dropped(fileList) {
    var files;
    if(is_string(fileList)) {
        files = json_parse(fileList);
    } else {
        files = fileList;
    }

    if(!is_array(files)) {
        show_debug_log("!Error: Invalid file list in window_on_files_dropped().");
        return;
    }

    for(var i = 0; i < array_length(files); i++) {
        var file = files[i];
        var filext = filename_ext(file);

        switch (filext) {
            case ".dyn":
            case ".dy":
            case ".xml":
                project_load(file);
                break;
            case ".jpg":
            case ".jpeg":
            case ".png":
            case ".mp4":
            case ".avi":
            case ".mkv":
                background_load(file);
                break;
            case ".mp3":
            case ".flac":
            case ".wav":
            case ".ogg":
            case ".aiff":
            case ".mid":
                music_load(file);
                break;
            default:
                show_debug_message("!Warning: Unsupported file type dropped: " + filext);
                announcement_warning("drop_unsupported_file");
                break;
        }
    }
}



function window_reset() {
	var _ratio = min(display_get_width() / BASE_RES_W, display_get_height() / BASE_RES_H) * objManager.windowDisplayRatio;
	var w = BASE_RES_W * _ratio, h = BASE_RES_H * _ratio
    window_set_rectangle(
		(display_get_width() - w) * 0.5,
		(display_get_height() - h) * 0.5,
		w,
		h
	);
}

function resolution_set(width, height, updateViewport = true) {
	surface_resize(application_surface, width, height);

	if(updateViewport) {
		view_wport[0] = width;
		view_hport[0] = height;
		view_xport[0] = 0;
		view_yport[0] = 0;
	}
}


function window_toggle_fullscreen() {
    global.fullscreen = !global.fullscreen;
    window_set_fullscreen(global.fullscreen);

    if(!global.fullscreen) {
        // When exiting fullscreen, reset the window size to the default ratio of the display size.
        window_reset();
    }
}

function window_check_fullscreen() {
    if(window_get_fullscreen() != global.fullscreen) {
        global.fullscreen = window_get_fullscreen();
        if(!global.fullscreen) {
            window_reset();
        }
    }
}

function window_enable_ime() {
    return dyc_enable_ime();
}

function window_disable_ime() {
    return dyc_disable_ime();
}