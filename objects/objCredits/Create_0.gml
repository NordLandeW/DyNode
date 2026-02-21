
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
renewAcceleration = 20000;
currentRenewAcceleration = renewAcceleration;
accelerationDamping = 0.998;
renewAccelerationDamping = 0.9;

nowY = height + 50;
nowX = 10;

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
    result += "[sprMsdfNotoSans][scale, 0.8]Yet another Dynamix Charting tool.\n";
    result += "[sprMsdfNotoSans][scale, 0.75]made by [scale, 1.2]NordLandeW\n";
    result += "\n\n\n";

    // Localization
    result += "[scale,1.6]" + i18n_get("credits_localization_title") + "\n";
    for(var i = 0; i < array_length(localization); i++) {
        var name = localization[i][0];
        var contribution = localization[i][1];
        result += $"[scale,1.2]{name}   [scale,0.8]{i18n_get(contribution)}[/scale]\n";
    }
    result += "\n\n\n";

    // Special thanks
    result += "[scale,1.6]" + i18n_get("credits_special_thanks_title") + "\n";
    for(var i = 0; i < array_length(special_thanks); i++) {
        var name = special_thanks[i][0];
        var contribution = special_thanks[i][1];
        result += $"[scale,1.2]{name}";
        if(contribution != "")
            result += $"   [scale,0.8]{i18n_get(contribution)}[/scale]";
        result += "\n";
    }
    result += "\n\n\n";

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