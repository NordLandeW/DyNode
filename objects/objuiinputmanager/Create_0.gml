
inputsMap = {};

// In-Functions

register_input = function (nid, inst) {
    if(variable_struct_exists(inputsMap, nid)) {
        show_error("Duplicated input id: "+nid, true);
    }
    inputsMap[$ nid] = inst;
}

get_input = function (nid) {
    if(!variable_struct_exists(inputsMap, nid)) {
        show_error("Input id not found: "+nid, true);
    }
    return inputsMap[$ nid].input;
}

get_list_pointer = function (nid) {
    if(!variable_struct_exists(inputsMap, nid)) {
        show_error("Input id not found: "+nid, true);
    }
    return inputsMap[$ nid].listPointer;
}