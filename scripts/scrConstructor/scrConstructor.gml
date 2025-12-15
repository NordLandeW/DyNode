
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
        case NOTE_TYPE.SUB:
            return 67; // sprHoldEdge2
    }
}

/// @type {Asset.GMObject.objNote} 
function _note_get_object_asset(noteType) {
    switch(noteType) {
        case NOTE_TYPE.NORMAL:
            return objNote;
        case NOTE_TYPE.CHAIN:
            return objChain;
        case NOTE_TYPE.HOLD:
            return objHold;
        case NOTE_TYPE.SUB:
            return objHoldSub;
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
        var _var_default = function(str, key, val) {
            if(!variable_struct_exists(str, key)) return val;
            return str[$ key];
        }
        time = _var_default(_initProp, "time", time);
        side = _var_default(_initProp, "side", side);
        width = _var_default(_initProp, "width", width);
        position = _var_default(_initProp, "position", position);
        lastTime = _var_default(_initProp, "lastTime", lastTime);
        noteType = _var_default(_initProp, "noteType", noteType);
        noteID = _var_default(_initProp, "noteID", noteID);
        subNoteID = _var_default(_initProp, "subNoteID", subNoteID);
        beginTime = _var_default(_initProp, "beginTime", beginTime);
    }

    static get_pixel_width = function() {
        var pWidth = width * 300 / (side == 0 ? 1:2) - 30 + _note_get_lrpadding_total(noteType);
        pWidth = max(pWidth, _note_get_sprite_orig_width(noteType));
        return pWidth;
    }

    static is_outscreen = function() {
        var pWidth = get_pixel_width();
        var _nx = note_pos_to_x(position, side);
        var _outbound = false;
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
        return _outbound;
    }

    static bitread = function(buffer) {
        buffer_seek(buffer, buffer_seek_start, 0);
        side = buffer_read(buffer, buffer_u32);
        noteType = buffer_read(buffer, buffer_u32);
        time = buffer_read(buffer, buffer_f64);
        width = buffer_read(buffer, buffer_f64);
        position = buffer_read(buffer, buffer_f64);
        lastTime = buffer_read(buffer, buffer_f64);
        beginTime = buffer_read(buffer, buffer_f64);
        noteID = buffer_read(buffer, buffer_string);
        subNoteID = buffer_read(buffer, buffer_string);

        return self;
    }

    static bitwrite = function(buffer) {
        buffer_seek(buffer, buffer_seek_start, 0);
        buffer_write(buffer, buffer_u32, side);
        buffer_write(buffer, buffer_u32, noteType);
        buffer_write(buffer, buffer_f64, time);
        buffer_write(buffer, buffer_f64, width);
        buffer_write(buffer, buffer_f64, position);
        buffer_write(buffer, buffer_f64, lastTime);
        buffer_write(buffer, buffer_f64, beginTime);
        buffer_write(buffer, buffer_string, noteID);
        buffer_write(buffer, buffer_string, subNoteID);

        return self;
    }

    /// @returns {Struct.sNoteProp} 
    static copy = function() {
        return SnapDeepCopy(self);
    }
}