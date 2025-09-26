
if(mouse_y < y - sideButtonWidth / 2)
    choosing = 3;
else if(mouse_x < x - sideButtonWidth / 2) choosing = 0;
else if(mouse_x > x + sideButtonWidth / 2) choosing = 2;
else choosing = 1;

if(mouse_check_button_released(mb_right)) {
    editor_set_editside(side[choosing]);
    instance_destroy();
}

animTargetGradAlpha = [0, 0, 0, 0];
animTargetGradAlpha[choosing] = 1;