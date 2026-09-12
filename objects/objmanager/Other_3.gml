
app_cleanup_step("config", function() { save_config(); });
app_cleanup_step("AppClose", function() {
    if(global.analytics) aptabase_track("AppClose");
});
app_cleanup_step("Aptabase", function() { aptabase_shutdown(global.analytics); });
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