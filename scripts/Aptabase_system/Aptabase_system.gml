
#macro __APTABASE_SDK_VERSION "1.0.0"
#macro __APTABASE_MAX_BATCH_SIZE_LIMIT 25

function __AptabaseEvent(eventName, props) constructor {
    static pad2 = function(value) {
        var n = floor(value);
        return n < 10 ? "0" + string(n) : string(n);
    }

    static pad3 = function(value) {
        var n = floor(value);
        if(n < 10) return "00" + string(n);
        if(n < 100) return "0" + string(n);
        return string(n);
    }

    static utc_timestamp_iso8601 = function() {
        var nowLocal = date_current_datetime();
        var nowUTC = date_inc_minute(nowLocal, -date_get_timezone());

        var year = date_get_year(nowUTC);
        var month = date_get_month(nowUTC);
        var day = date_get_day(nowUTC);
        var hour = date_get_hour(nowUTC);
        var minute = date_get_minute(nowUTC);

        var secondWithFraction = date_get_second(nowUTC);
        var second = floor(secondWithFraction);
        var millisecond = floor(frac(secondWithFraction) * 1000);

        return string(year) + "-" + pad2(month) + "-" + pad2(day)
            + "T" + pad2(hour) + ":" + pad2(minute) + ":" + pad2(second)
            + "." + pad3(millisecond) + "Z";
    }

    static is_valid_prop_value = function(value) {
        return is_string(value) || is_real(value) || is_bool(value);
    }

    static sanitize_props = function(rawProps, isDebugMode) {
        var cleanedProps = {};
        if(is_undefined(rawProps) || !is_struct(rawProps)) {
            return cleanedProps;
        }

        var keys = variable_struct_get_names(rawProps);
        var keyCount = array_length(keys);
        for(var i = 0; i < keyCount; i++) {
            var key = keys[i];
            var value = rawProps[$ key];

            if(is_valid_prop_value(value)) {
                cleanedProps[$ key] = value;
            } else if(isDebugMode) {
                show_debug_message("Aptabase ignored invalid prop '" + string(key) + "'. Only flat string/number/bool values are allowed.");
            }
        }

        return cleanedProps;
    }

    static get_locale_code = function() {
        var language = string_lower(string(os_get_language()));
        var region = string_upper(string(os_get_region()));

        if(string_length(language) <= 0) language = "en";
        if(string_length(region) > 0) return language + "-" + region;
        return language;
    }

    static os_type_to_name = function() {
        switch(os_type) {
            case os_windows: return "Windows";
            case os_gxgames: return "GX.games";
            case os_linux: return "Linux";
            case os_macosx: return "macOS";
            case os_ios: return "iOS";
            case os_tvos: return "tvOS";
            case os_android: return "Android";
            case os_ps4: return "PlayStation 4";
            case os_ps5: return "PlayStation 5";
            case os_switch: return "Nintendo Switch";
            case os_unknown: return "Unknown";
            default: return "Unknown";
        }
    }

    static map_get_first_string = function(map, keys, fallback) {
        var count = array_length(keys);
        for(var i = 0; i < count; i++) {
            var key = keys[i];
            if(ds_map_exists(map, key)) {
                var value = map[? key];
                if(!is_undefined(value)) {
                    var text = string(value);
                    if(string_length(text) > 0) return text;
                }
            }
        }

        return fallback;
    }

    static decode_os_version = function() {
        var versionValue = floor(real(os_version));

        switch(os_type) {
            case os_windows:
                var majorWin = floor(versionValue / 65536);
                var minorWin = versionValue - (majorWin * 65536);
                return string(majorWin) + "." + string(minorWin);

            case os_macosx:
            case os_ios:
            case os_tvos:
                var majorApple = floor(versionValue / 16777216);
                var remainApple = versionValue - (majorApple * 16777216);
                var minorApple = floor(remainApple / 4096);
                var buildApple = remainApple - (minorApple * 4096);
                return string(majorApple) + "." + string(minorApple) + "." + string(buildApple);

            case os_android:
                return "API " + string(versionValue);

            default:
                return string(versionValue);
        }
    }

    var resolvedSessionID = "";
    var resolvedIsDebug = false;
    var resolvedAppVersion = APTABASE_APP_VERSION;
    var aptabaseCli = global.__aptabaseClient;
    if(is_struct(aptabaseCli)) {
        resolvedSessionID = aptabaseCli.sessionID;
        resolvedIsDebug = aptabaseCli.isDebug;
        resolvedAppVersion = aptabaseCli.appVersion;
    }

    var resolvedProps = sanitize_props(props, resolvedIsDebug);

    var readableLocale = get_locale_code();
    var readableOsName = os_type_to_name();
    var readableOsVersion = "";
    var readableDeviceModel = "";

    var osInfo = os_get_info();
    if(ds_exists(osInfo, ds_type_map)) {
        readableOsName = map_get_first_string(osInfo, ["os_name", "system_name", "platform", "name"], readableOsName);
        readableOsVersion = map_get_first_string(osInfo, ["os_version_string", "version_string", "system_version", "os_version", "version", "release", "build"], readableOsVersion);
        readableDeviceModel = map_get_first_string(osInfo, ["device_model", "model", "hardware_model", "machine", "hardware", "device", "product", "manufacturer"], readableDeviceModel);

        ds_map_destroy(osInfo);
    }

    if(string_length(readableOsVersion) <= 0) readableOsVersion = decode_os_version();
    if(string_length(readableDeviceModel) <= 0) readableDeviceModel = "unknown";

    self.timestamp = utc_timestamp_iso8601();
    self.sessionId = resolvedSessionID;
    self.eventName = eventName;
    self.systemProps = {
        locale: readableLocale,
        osName: readableOsName,
        osVersion: readableOsVersion,
        deviceModel: readableDeviceModel,
        isDebug: resolvedIsDebug,
        appVersion: resolvedAppVersion,
        sdkVersion: $"gm-aptabase@{__APTABASE_SDK_VERSION}"
    };
    self.props = resolvedProps;
}

function __AptabaseSendingEvent(requestID, events) constructor {
    self.requestID = requestID;
    
    /// @type {Array<Struct.__AptabaseEvent>} 
    self.events = events;
}

function __AptabaseClient() constructor {
    global.__aptabaseClient = self;

    // Event management.
    eventQueue = [];

    // Map of flush ID to event for events that have been sent but not yet acknowledged.
    sendingEvents = {};

    // Config.
    appKey = "";
    appVersion = string(APTABASE_APP_VERSION);
    baseURL = APTABASE_BASE_URL;
    maxBatchSize = min(APTABASE_MAX_BATCH_SIZE, __APTABASE_MAX_BATCH_SIZE_LIMIT);
    flushInterval = APTABASE_FLUSH_INTERVAL;
    isDebug = bool(APTABASE_IS_DEBUG);

    // Timesource handle.
    flushEventHandle = undefined;

    static new_session_id = function() {
        randomize();
        var nowUTC = date_inc_minute(date_current_datetime(), -date_get_timezone());
        var unixTimestamp = floor((nowUTC - 25569) * 86400);
        var randomNumber = irandom_range(0, 99999999);
        return string(unixTimestamp) + string(randomNumber);
    }

    sessionID = new_session_id();

    static get_host_from_app_key = function(appKey) {
        if(string_pos("SH", appKey) > 0) {
            return APTABASE_SH_HOST;
        }
        if(string_pos("EU", appKey) > 0) {
            return APTABASE_EU_HOST;
        }
        return APTABASE_US_HOST;
    }

    static send_request = function(payload) {
        var resolvedBaseURL = baseURL;
        if(string_length(resolvedBaseURL) <= 0) {
            resolvedBaseURL = get_host_from_app_key(appKey);
        }

        var endpoint = resolvedBaseURL;
        if(string_pos("/api/v0/events", endpoint) <= 0) {
            if(string_copy(endpoint, string_length(endpoint), 1) == "/") {
                endpoint += "api/v0/events";
            } else {
                endpoint += "/api/v0/events";
            }
        }

        var headers = ds_map_create();
        headers[? "Content-Type"] = "application/json";
        headers[? "App-Key"] = appKey;

        var requestID = http_request(endpoint, "POST", headers, payload);
        ds_map_destroy(headers);

        if(isDebug) {
            if(requestID < 0) {
                show_debug_message("Aptabase request failed to send: " + endpoint + " (requestID=" + string(requestID) + ")");
            } else {
                show_debug_message("Aptabase request sent: " + endpoint + " (requestID=" + string(requestID) + ")");
            }
            show_debug_message("Payload: \n" + payload);
        }

        return requestID;
    }

    /// @param {Struct.__AptabaseEvent} event 
    static push_event = function(event) {
        array_push(eventQueue, event);
    }

    static flush = function() {
        var eventsToSend = min(array_length(eventQueue), min(maxBatchSize, __APTABASE_MAX_BATCH_SIZE_LIMIT));
        if(eventsToSend <= 0) return;

        var sentEvents = [];
        for(var i = 0; i < eventsToSend; i++) {
            var event = eventQueue[i];
            array_push(sentEvents, event);
        }

        var requestID = send_request(json_stringify(sentEvents));
        if(requestID >= 0) {
            sendingEvents[$ requestID] = sentEvents;
            array_delete(eventQueue, 0, eventsToSend);
        } else if(isDebug) {
            show_debug_message("Aptabase flush aborted: requestID < 0, events kept in queue.");
        }
    }

    static apply_config = function(config) {
        if(!is_struct(config)) return;

        if(variable_struct_exists(config, "app_key"))
            appKey = config[? "app_key"];
        if(variable_struct_exists(config, "app_version"))
            appVersion = string(config[? "app_version"]);
        if(variable_struct_exists(config, "base_url"))
            baseURL = config[? "base_url"];
        if(variable_struct_exists(config, "max_batch_size"))
            maxBatchSize = min(config[? "max_batch_size"], __APTABASE_MAX_BATCH_SIZE_LIMIT);
        if(variable_struct_exists(config, "flush_interval")) {
            flushInterval = config[? "flush_interval"];
            start();
        }
        if(variable_struct_exists(config, "is_debug"))
            isDebug = bool(config[? "is_debug"]);
    }

    static is_request_existed = function(id) {
        return variable_struct_exists(sendingEvents, id);
    }

    static repush_request = function(id) {
        if(is_request_existed(id)) {
            var events = sendingEvents[$ id];
            eventQueue = array_concat(events, eventQueue);
            variable_struct_remove(sendingEvents, id);

            if(isDebug)
                show_debug_message("Re-pushed events from request " + string(id) + " back to the queue.");
        }
    }

    static acknowledge_request = function(id) {
        if(is_request_existed(id)) {
            variable_struct_remove(sendingEvents, id);

            if(isDebug)
                show_debug_message("Acknowledged request " + string(id) + " and removed it from pending list.");
        }
    }

    static start = function() {
        if(!is_undefined(flushEventHandle)) stop();
        flushEventHandle = call_later(
            flushInterval, time_source_units_seconds, function() {
                global.__aptabaseClient.flush();
            }, true
        );
    }

    static stop = function() {
        if(!is_undefined(flushEventHandle)) {
            call_cancel(flushEventHandle);
            flushEventHandle = undefined;
        }
    }

    // Create daemon object.
    call_later(1, time_source_units_frames, function() {
        instance_create_depth(0, 0, 10000, __obj_Aptabase_daemon);
    });
}


new __AptabaseClient();
