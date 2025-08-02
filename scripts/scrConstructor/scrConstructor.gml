
function sTimingPoint(_time, _beatlength, _beats) constructor {
    time = _time;
    beatLength = _beatlength;
    meter = _beats;             // x/4
}

enum OPERATION_TYPE {
    MOVE,
    ADD,
    REMOVE,
    TPADD,
    TPREMOVE,
    TPCHANGE,
    OFFSET,
    CUT, // special
    ATTACH, // special
    MIRROR, // special
    ROTATE, // special
    PASTE, // special
    SETWIDTH, // special
    SETTYPE, // special
    RANDOMIZE, // special
    DUPLICATE, // special
    EXPR, // special
}

function sOperation(_type, _from, _to) constructor {
    /// @type {Enum.OPERATION_TYPE}
    opType = _type;
    fromProp = _from;
    toProp = _to;
}

enum NOTE_SIDE {
    FRONT,
    LEFT,
    RIGHT
}

enum NOTE_TYPE {
    NORMAL,
    CHAIN,
    HOLD,
    SUB
}

#macro NOTE_NORMAL_PADDING_PIXELS_LEFT 5
#macro NOTE_NORMAL_PADDING_PIXELS_RIGHT 5
#macro NOTE_CHAIN_PADDING_PIXELS_LEFT 7
#macro NOTE_CHAIN_PADDING_PIXELS_RIGHT 7
#macro NOTE_HOLD_PADDING_PIXELS_LEFT 12
#macro NOTE_HOLD_PADDING_PIXELS_RIGHT 12

function _note_get_lrpadding_total(noteType) {
    switch(noteType) {
        case NOTE_TYPE.NORMAL:
            return NOTE_NORMAL_PADDING_PIXELS_LEFT + NOTE_NORMAL_PADDING_PIXELS_RIGHT;
        case NOTE_TYPE.CHAIN:
            return NOTE_CHAIN_PADDING_PIXELS_LEFT + NOTE_CHAIN_PADDING_PIXELS_RIGHT;
        case NOTE_TYPE.HOLD:
        case NOTE_TYPE.SUB:
            return NOTE_HOLD_PADDING_PIXELS_LEFT + NOTE_HOLD_PADDING_PIXELS_RIGHT;
    }
}

function _note_get_sprite_orig_width(noteType) {
    switch(noteType) {
        case NOTE_TYPE.NORMAL:
            return 45;  // sprNote2
        case NOTE_TYPE.CHAIN:
            return 120;  // sprChain
        case NOTE_TYPE.HOLD:
            return 67; // sprHoldEdge2
    }
}

function sNoteProp(_initProp = undefined) constructor {
    time = 0;
    side = NOTE_SIDE.FRONT;
    width = 0;
    position = 0;
    lastTime = 0;
    noteType = NOTE_TYPE.NORMAL;
    noteID = "";
    subNoteID = "";
    beginTime = 0;

    if(is_struct(_initProp)) {
        time = _initProp[$ "time"];
        side = _initProp[$ "side"];
        width = _initProp[$ "width"];
        position = _initProp[$ "position"];
        lastTime = _initProp[$ "lastTime"];
        noteType = _initProp[$ "noteType"];
        noteID = _initProp[$ "noteID"];
        subNoteID = _initProp[$ "subNoteID"];
        beginTime = _initProp[$ "beginTime"];
    }

    static get_pixel_width = function() {
        var pWidth = width * 300 / (side == 0 ? 1:2) - 30 + _note_get_lrpadding_total(noteType);
        pWidth = max(pWidth, _note_get_sprite_orig_width(noteType));
        return pWidth;
    }

    static is_outscreen = function() {
        var pWidth = get_pixel_width();
        var _nx = note_pos_to_x(position, side);
        if(side == 0) {
            var _xl = _nx - pWidth / 2, _xr = _nx + pWidth / 2;
            if(_xr <= 0 || _xl >= BASE_RES_W)
                _outbound = true;
        }
        else {
            var _yl = _nx - pWidth / 2, _yr = _nx + pWidth / 2;
            if(_yr <= 0 || _yl >= BASE_RES_H)
                _outbound = true;
        }
    }
}