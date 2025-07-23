
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