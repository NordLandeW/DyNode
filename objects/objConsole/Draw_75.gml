
if(!focusing) return;

// Draw input bar.
draw_set_color_alpha(colorScheme.background, 0.75);
draw_rectangle(0, BASE_RES_H - inputBarHeight, BASE_RES_W, BASE_RES_H, false);

scribble(inputBuffer)
    .starting_format("mSpaceMono", colorScheme.text)
    .align(fa_left, fa_middle)
    .draw(inputBarPadding, BASE_RES_H - inputBarHeight / 2);

// Draw message bar.

var currentVisibleLines = get_current_visible_lines();

var messageAnchorY = BASE_RES_H - inputBarHeight - messageBarHeight / 2;
var messages = global.console.get_last_messages(historyOffset, currentVisibleLines);


for(var i = 0; i < array_length(messages); i++) {
    var msgY = messageAnchorY - i * messageBarHeight;
    draw_set_color_alpha(colorScheme.background, 0.75);
    draw_rectangle(0, msgY - messageBarHeight / 2, BASE_RES_W, msgY + messageBarHeight / 2, false);
    
    scribble(messages[array_length(messages) - 1 - i])
        .starting_format("mSpaceMono", colorScheme.text)
        .align(fa_left, fa_middle)
        .draw(messageBarPadding, msgY);
}