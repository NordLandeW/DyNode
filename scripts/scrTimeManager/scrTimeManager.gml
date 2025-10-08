
function TimeManager() constructor {

    global.timeManager = self;

    enum TIME_MODE {
        DEFAULT,
        FIXED
    };

    mode = TIME_MODE.DEFAULT;
    fixedSpeed = 60; // in fps

    static get_delta = function() {
        if(mode == TIME_MODE.DEFAULT) {
            return delta_time;
        } else {
            return 1000000 / fixedSpeed;
        }
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