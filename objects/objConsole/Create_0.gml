
persistent = true;

colorScheme = {
    background: c_black,
    text: c_white,
};

inputBarHeight = 30;
inputBarPadding = 5;

messageBarHeight = 25;
messageBarPadding = 10;

focusing = false;

inputBuffer = "";
inputCursorPos = 0;

visibleLines = 20;
historyOffset = 0;

bgAlpha = 0.75;

quickCommand = false;

cursorX = inputBarPadding;
cursorHeight = inputBarHeight - 12;
cursorTimer = 0;

// Temporary implementation.
commandHistory = [];
commandHistoryPointer = -1;

function focus() {
    focusing = true;
    keyboard_string = "";
    inputBuffer = "";
    inputCursorPos = 0;

    input_check_group_set("console");
    show_debug_message("Console focused.");
}

function unfocus() {
    quickCommand = false;
    focusing = false;
    inputBuffer = "";
    inputCursorPos = 0;
    call_later(1, time_source_units_frames, function() {
        input_check_group_reset();
    });
    show_debug_message("Console unfocused.");
}

function get_current_visible_lines() {
    return quickCommand ? min(visibleLines, 3) : visibleLines;
}

function insert_text_at_cursor(_text) {
    if(_text == "") return;
    inputBuffer = string_insert(_text, inputBuffer, inputCursorPos + 1);
    inputCursorPos += string_length(_text);
    inputCursorPos = clamp(inputCursorPos, 0, string_length(inputBuffer));
    cursorTimer = 0;
}

function delete_left_at_cursor() {
    if(inputCursorPos <= 0) return;
    inputBuffer = string_delete(inputBuffer, inputCursorPos, 1);
    inputCursorPos -= 1;
    inputCursorPos = clamp(inputCursorPos, 0, string_length(inputBuffer));
    cursorTimer = 0;
}

function delete_right_at_cursor() {
    if(inputCursorPos >= string_length(inputBuffer)) return;
    inputBuffer = string_delete(inputBuffer, inputCursorPos + 1, 1);
    inputCursorPos = clamp(inputCursorPos, 0, string_length(inputBuffer));
    cursorTimer = 0;
}