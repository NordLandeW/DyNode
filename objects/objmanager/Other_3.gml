
app_cleanup_step("window", function() { window_set_visible_w(false); });
app_cleanup_step("config", function() { save_config(); });
app_cleanup_step("AppClose", function() {
    if(global.analytics) aptabase_track("AppClose");
});
app_cleanup_step("project saves", function() {
    if(DyCore_shutdown_project_saves() < 0) show_debug_message("Project saves drained with errors.");
});
app_cleanup_step("recorder", function() { global.recordManager.cleanup(); });
app_cleanup_step("chart", function() { map_close(true); });
app_cleanup_step("video frame", function() { dyc_video_clear_frame(); });
app_cleanup_step("note renderer", function() { global.noteRenderer.cleanup(); });
app_cleanup_step("DyCore", function() {
    if(DyCore_shutdown() < 0) show_debug_message("DyCore shutdown completed with errors.");
});
app_cleanup_step("FMOD", function() { FMODGMS_Sys_Close(); });
app_cleanup_step("telemetry", function() {
    var request = aptabase_shutdown(global.analytics);
    var result = json_parse(DyCore_shutdown_telemetry(request.endpoint, request.appKey, request.events, request.batchSize));
    show_debug_message("Telemetry shutdown: " + json_stringify(aptabase_apply_shutdown_result(result)));
});
