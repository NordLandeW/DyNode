
if(!focusing) {
    if(keycheck_down(190)) {
        focus();
    }
}
if(focusing) {
    inputBuffer = keyboard_string;

    input_group_set("console");
    if(keycheck_down(vk_enter) || keycheck_down_ctrl(vk_enter)) {
        // Execute command.
        global.console.run_command_line(inputBuffer);
        if(ctrl_ishold()) {
            unfocus();
        }
        else {
            inputBuffer = "";
            keyboard_string = inputBuffer;
        }
    }

    if(keycheck_down(vk_escape)) {
        unfocus();
    }
    input_group_reset();
}