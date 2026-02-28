
function aptabase_init(appKey, config = undefined) {
    global.__aptabaseSystem.appKey = appKey;
    global.__aptabaseSystem.apply_config(config);
    global.__aptabaseSystem.start();

    show_debug_message($"Welcome to use gm-aptabase@{__APTABASE_SDK_VERSION}! Aptabase initialized.");
}

function aptabase_track(eventName, props = undefined) {
    var event = new __AptabaseEvent(eventName, props);
    global.__aptabaseSystem.push_event(event);
}

function aptabase_flush() {
    global.__aptabaseSystem.flush();
}