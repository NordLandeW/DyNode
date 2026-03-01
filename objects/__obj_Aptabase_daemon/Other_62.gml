
if(!variable_global_exists("__aptabaseClient"))
    return;

var aptabaseCli = global.__aptabaseClient;
if(!is_struct(aptabaseCli))
    return;

var isDebug = variable_struct_exists(aptabaseCli, "isDebug") && aptabaseCli.isDebug;

var requestID = string(async_load[? "id"]);

if(aptabaseCli.is_request_existed(requestID)) {
    if(async_load[? "status"] == 1) {
        return;
    }
    
    var isSuccess = false;
    var isClientError = false;
    if(ds_map_exists(async_load, "http_status")) {
        var httpStatus = real(async_load[? "http_status"]);
        isSuccess = (httpStatus >= 200) && (httpStatus < 300);
        isClientError = (httpStatus >= 400) && (httpStatus < 500) && (httpStatus != 429) && (httpStatus != 408);
    }
    else if(ds_map_exists(async_load, "status")) {
        var status = real(async_load[? "status"]);
        isSuccess = (status >= 0);
    }
    
    if(isSuccess || isClientError) {
        if(isDebug && isClientError) {
            show_debug_message("Aptabase request failed with client error " + string(httpStatus) + ", dropping events.");
        } else if(isDebug) {
            show_debug_message("Aptabase HTTP request " + requestID + " succeeded and acknowledged.");
        }

        aptabaseCli.acknowledge_request(requestID);
    } else {
        if(isDebug) {
            var debugStatus = ds_map_exists(async_load, "status") ? string(async_load[? "status"]) : "n/a";
            var debugHttpStatus = ds_map_exists(async_load, "http_status") ? string(async_load[? "http_status"]) : "n/a";
            show_debug_message("Aptabase HTTP request " + requestID + " failed (status=" + debugStatus + ", http_status=" + debugHttpStatus + "), repushing events.");
            if(ds_map_exists(async_load, "result")) {
                show_debug_message("Respond result: " + async_load[? "result"]);
            } else {
                show_debug_message("No response data received.");
            }
        }

        aptabaseCli.repush_request(requestID);
    }
}

