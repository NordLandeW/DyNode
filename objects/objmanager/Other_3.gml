
FMODGMS_Sys_Close();
save_config();

if(global.recordManager.is_recording())
    global.recordManager.finish_recording();

if(global.analytics) {
    aptabase_track("AppClose");
    aptabase_flush();
}