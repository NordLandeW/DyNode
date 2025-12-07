
function dyc_video_get_frame() {
    static frameBuffer = -1;
    static frameSurface = -999;
    var buffSize = DyCore_video_get_buffer_size();

    if(!buffer_exists(frameBuffer) || buffer_get_size(frameBuffer) != buffSize) {
        if(buffSize <= 0)
            return -1;

        if(buffer_exists(frameBuffer))
            buffer_resize(frameBuffer, buffSize);
        else {
            frameBuffer = buffer_create(buffSize, buffer_fixed, 1);
        }
    }

    var updated = DyCore_video_get_frame(buffer_get_address(frameBuffer), buffSize);
    if(updated || !surface_exists(frameSurface)) {
        buffer_set_used_size(frameBuffer, buffSize);
        var vw = DyCore_video_get_width();
        var vh = DyCore_video_get_height();
        if(!surface_exists(frameSurface) || vw != surface_get_width(frameSurface) || vh != surface_get_height(frameSurface)) {
            if(surface_exists(frameSurface))
                surface_free(frameSurface);
            frameSurface = surface_create(vw, vh);
        }

        buffer_set_surface(frameBuffer, frameSurface, 0);
    }

    return frameSurface;
}

function dyc_video_draw(x, y, alp) {
    if(objMain.nowTime < 0) {
        draw_sprite_ext(sprBlack, 0, 0, 0, BASE_RES_W / 32, BASE_RES_H / 32, 0, c_black, alp);
        if(dyc_video_is_playing())
            dyc_video_pause();
        return;
    }

    if(!dyc_video_is_playing() && objMain.nowPlaying && editor_get_editmode() == 5) {
        dyc_video_play();
        dyc_video_seek_to(objMain.nowTime / 1000);
    }

    var _surf = dyc_video_get_frame();

    if(_surf < 0) {
        draw_sprite_ext(sprBlack, 0, 0, 0, BASE_RES_W / 32, BASE_RES_H / 32, 0, c_black, alp);
        return;
    }
    var _sw = surface_get_width(_surf), _sh = surface_get_height(_surf);
    var _nw = BASE_RES_W, _nh = BASE_RES_H;
    var _scl = max(_nw / _sw, _nh / _sh); // Centre & keep ratios
    var _sx = (_nw - _sw * _scl) / 2, _sy = (_nh - _sh * _scl) / 2;

    shader_set(shd_video);
    draw_surface_ext(_surf, _sx, _sy, _scl, _scl, 0, c_white, alp);
    shader_reset();
}

function dyc_video_free() {
    DyCore_video_close();
}

function dyc_video_seek_to(time) {
    if(editor_get_editmode() != 5)
        return;
    DyCore_video_seek(time);
}

// If the video is loaded
function dyc_video_is_loaded() {
    return DyCore_video_is_loaded();
}

function dyc_video_is_playing() {
    return DyCore_video_is_playing();
}

function dyc_video_pause() {
    DyCore_video_pause();
}

function dyc_video_play() {
    DyCore_video_play();
}