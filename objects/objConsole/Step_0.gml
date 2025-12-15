
if(!focusing) {
    if(keycheck_down(190)) {    // .
        focus();
        keyboard_string = ".";
        quickCommand = true;
    }
    if(keycheck_down(191)) {    // /
        focus();
        keyboard_string = "/";
        quickCommand = false;
    }
}
if(focusing) {
    if(keyboard_string != inputBuffer) {
        inputBuffer = keyboard_string;
        cursorTimer = 0;
    }

    input_group_set("console");
    if(keycheck_down(vk_enter) || keycheck_down_ctrl(vk_enter)) {
        // Check if is quick command.
        if(string_char_at(inputBuffer, 1) == ".") {
            quickCommand = true;
        }
        else if(string_char_at(inputBuffer, 1) == "/") {
            quickCommand = false;
        }

        // Execute command.
        array_push(commandHistory, inputBuffer);
        commandHistoryPointer = -1;
        var result = console_run(inputBuffer);
        if(ctrl_ishold() || (quickCommand && result == 0)) {
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
    var consoleRange = inputBarHeight + messageBarHeight * min(get_current_visible_lines(), array_length(global.console.messages));
    if(mouse_y > BASE_RES_H - consoleRange) {
        var wheelDelta = wheelcheck_up() - wheelcheck_down();
        historyOffset = clamp(historyOffset + wheelDelta, 0, max(0, array_length(global.console.messages) - visibleLines));
        wheel_clear();
    }

    // Handle command history navigation.
    var pointerDelta = (keycheck_down(vk_up) - keycheck_down(vk_down));
    if(pointerDelta != 0) {
        commandHistoryPointer += pointerDelta;
        commandHistoryPointer = clamp(commandHistoryPointer, -1, array_length(commandHistory) - 1);
        if(commandHistoryPointer == -1) {
            inputBuffer = "";
        } else {
            inputBuffer = commandHistory[array_length(commandHistory) - 1 - commandHistoryPointer];
        }
        keyboard_string = inputBuffer;
        cursorTimer = 0;
    }

    input_group_reset();
}

// Update cursor position.

var targetX = inputBarPadding + scribble(inputBuffer).starting_format("mSpaceMono", colorScheme.text).get_width();
cursorX = lerp_lim_a(cursorX, targetX, 0.4, 200);