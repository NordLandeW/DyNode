
function aptabase_init(appKey, config = undefined) {
    global.__aptabaseClient.appKey = appKey;
    global.__aptabaseClient.apply_config(config);
    global.__aptabaseClient.start();

    show_debug_message($"Welcome to use gm-aptabase@{__APTABASE_SDK_VERSION}! Aptabase initialized.");
}

function aptabase_track(eventName, props = undefined) {
    var event = new __AptabaseEvent(eventName, props);
    global.__aptabaseClient.push_event(event);
}

function aptabase_flush() {
    global.__aptabaseClient.flush();
}