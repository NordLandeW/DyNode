
function TimeManager() constructor {

    global.timeManager = self;

    enum TIME_MODE {
        DEFAULT,
        FIXED
    };

    mode = TIME_MODE.DEFAULT;
    fixedSpeed = 60; // in fps

    static _clamped_delta = function() {
        return min(100000, delta_time);
    }

    static get_delta = function(overwriteMode = -1, clamped = true) {
        var _curMode = mode;
        if(overwriteMode != -1) _curMode = overwriteMode;
        
        if(_curMode == TIME_MODE.DEFAULT) {
            return clamped ? _clamped_delta() : delta_time;
        } else {
            return 1000000 / fixedSpeed;
        }
    }

    static get_delta_default = function(clamped = true) {
        return get_delta(TIME_MODE.DEFAULT, clamped);
    }
    static get_delta_fixed = function(clamped = true) {
        return get_delta(TIME_MODE.FIXED, clamped);
    }

    static set_mode_default = function() {
        mode = TIME_MODE.DEFAULT;
    }

    static set_mode_fixed = function(fps) {
        mode = TIME_MODE.FIXED;
        fixedSpeed = fps;
    }

    static get_fps = function() {
        if(mode == TIME_MODE.DEFAULT) {
            return game_get_speed(gamespeed_fps);
        } else {
            return fixedSpeed;
        }
    }

    static get_fps_scale = function() {
        return BASE_FPS / get_fps();
    }
}

new TimeManager();