
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

visibleLines = 20;
historyOffset = 0;

bgAlpha = 0.75;

function focus() {
    focusing = true;
    keyboard_string = "";

    input_check_group_set("console");
    show_debug_message("Console focused.");
}

function unfocus() {
    focusing = false;
    inputBuffer = "";
    call_later(1, time_source_units_frames, function() {
        input_check_group_reset();
    });
    show_debug_message("Console unfocused.");
}