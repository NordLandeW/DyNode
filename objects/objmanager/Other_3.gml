
FMODGMS_Sys_Close();
save_config();

if(global.recordManager.is_recording())
    global.recordManager.finish_recording();