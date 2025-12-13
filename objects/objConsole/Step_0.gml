
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

    var offsetDelta = (keycheck_down(vk_pageup) - keycheck_down(vk_pagedown)) * visibleLines * 0.5;

    historyOffset = clamp(historyOffset + offsetDelta, 0, max(0, array_length(global.console.messages) - visibleLines));

    // Handle mouse wheel scrolling.
    var consoleRange = inputBarHeight + messageBarHeight * min(visibleLines, array_length(global.console.messages));
    if(mouse_y > BASE_RES_H - consoleRange) {
        var wheelDelta = wheelcheck_up() - wheelcheck_down();
        historyOffset = clamp(historyOffset + wheelDelta, 0, max(0, array_length(global.console.messages) - visibleLines));
        wheel_clear();
    }

    input_group_reset();
}