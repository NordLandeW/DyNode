
if(_update_status == UPDATE_STATUS.READY) {
    show_debug_message("Last update step initiate.");
    var _status = DyCore_update(program_directory);
    if(_status < 0)
        show_error("DyCore update failed. "+string(_status), true);
}