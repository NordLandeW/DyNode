
event_inherited();

// The credits screen size.
width = BASE_RES_W;
height = BASE_RES_H;

// Speed controllers.
baseSpeed = 100;
speedDamping = 0.99999;
currentSpeed = baseSpeed;
currentAcceleration = 0;
scrollAcceleration = 8000;
loopAcceleration = 20000;
currentLoopAcceleration = loopAcceleration;
accelerationDamping = 0.998;
loopAccelerationDamping = 0.9;

// Layout parameters
nowY = height + 50;
nowX = 10;

creditsMidPadding = 20;
creditsRowPadding = 20;
creditsPartPadding = 200;
creditsRowHeight = 30;
creditsNameYOffset = 10;

// Credits.

localization = [
    ["Jmakxd", "credits_localization_zh_en"]
]

special_thanks = [
    ["Algax", ""],
    ["AXIS5", ""],
    ["iam6668", ""],
    ["Jmakxd", ""]
]

licenseText = "";

function read_license_text() {
    var creditFile = buffer_load("credits_license.txt");
    licenseText = buffer_read(creditFile, buffer_text);
    buffer_delete(creditFile);
    if(i18n_get_lang() != "en-us")
        licenseText = string_replace_all(licenseText, "\n", "     \n");
}

read_license_text();

function get_about_text() {
    var result = "";

    // Head part.
    result += "[scale,0.75][sprIcon]\n\n";
    result += "[mDynamix][scale, 2.5]DyNode[/scale]\n\n";
    result += "[sprMsdfNotoSans][scale, 0.8]Yet another Dynamix charting tool.\n";
    result += "[sprMsdfNotoSans][scale, 0.8]... by [scale, 1.2]NordLandeW\n";

    return result;
}

function get_license_text() {
    var result = "";
    // License
    result += "[sprMsdfNotoSans][scale,0.8]" + i18n_get("credits_license_info") + "\n\n";
    result += "[scale,0.65]" + licenseText + "\n";

    result += "[scale,0.8]" + i18n_get("credits_thanks") + "\n";
    return result;
}

middleText = get_about_text();
rightText = get_license_text();

function draw_credits(credits_array, X, offsetY) {
    var curY = offsetY;
    for(var i = 0; i < array_length(credits_array); i++) {
        var _cur = credits_array[i];
        var _name = _cur[0];
        var _contribution = i18n_get(_cur[1]);
        curY += creditsRowHeight;

        var _ele = scribble(_name)
            .starting_format("sprMsdfNotoSans", c_white)
            .scale(1.2)
            .align(fa_right, fa_bottom)
            .msdf_shadow(c_black, 0.8, 0, 3, 6)
            .blend(image_blend, image_alpha);
        if(_contribution == "") {
            _ele.align(fa_center, fa_bottom)
                .draw(X, curY + creditsNameYOffset);
        }
        else {
            _ele.draw(X - creditsMidPadding / 2, curY + creditsNameYOffset);
        }
        
        if(DEBUG_MODE) {
            draw_set_color(c_red);
            draw_line(0, curY, width, curY);
        }

        if(_contribution != "") {
            var _con_ele = scribble(_contribution)
                .starting_format("sprMsdfNotoSans", c_white)
                .scale(0.7)
                .align(fa_left, fa_bottom)
                .msdf_shadow(c_black, 0.8, 0, 3, 6)
                .blend(image_blend, image_alpha);
            _con_ele.draw(X + creditsMidPadding / 2, curY);
        }

        curY += creditsRowPadding;
        
    }

    return curY - offsetY;
}