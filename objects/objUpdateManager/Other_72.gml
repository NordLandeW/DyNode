/// @description Update unzip event

if(_update_unzip_event_handle == async_load[? "id"]) {
    var _status = async_load[? "status"];
    if (_status < 0) {
        show_debug_message("ZIP file extraction failed.");
        announcement_error("autoupdate_process_err_2");
    }
    else if(_status == 0) {
        update_ready();
    }
}