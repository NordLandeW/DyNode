
enum STYLE {
    DYNAMIX,
    DYNAMITE
}

#macro STYLE_COUNT (2)
#macro DYNAMITE_SIDE_SPEED_SCALE (0.9)
global.STYLE_NAMES = ["Dynamix", "Dynamite"];

function style_init() {
    global.style = STYLE.DYNAMIX;
}

function style_next() {
    global.style = (global.style + 1) % STYLE_COUNT;

    announcement_set("Style", global.STYLE_NAMES[global.style]);
}

/// @description Inline Tap, Chain and Hold artwork at the chart stats' text size.
function style_note_stat_icons() {
    static _dynamix = ["[sprNote]", "[scale,0.4][sprChain][/s]", "[scale,0.4][sprHoldEdge][/s]"];
    static _dynamite = ["[scale,0.17][sprDynamiteTap][/s]", "[scale,0.16][sprDynamiteChainIcon][/s]", "[scale,0.17][sprDynamiteHoldIcon][/s]"];
    return global.style == STYLE.DYNAMITE ? _dynamite : _dynamix;
}
