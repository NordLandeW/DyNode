
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

    var offsetDelta = keycheck_down(vk_up) - keycheck_down(vk_down);
    offsetDelta += (keycheck_down(vk_pageup) - keycheck_down(vk_pagedown)) * 5;

    historyOffset = clamp(historyOffset + offsetDelta, 0, max(0, array_length(global.console.messages) - visibleLines));
    input_group_reset();
}