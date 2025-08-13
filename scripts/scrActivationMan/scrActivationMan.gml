

function Activationmanager() constructor {
    
    activatedPool = {};
    deactivatedPool = {};

    static activate = function(inst) {
        var instr = string(inst);
        if(variable_struct_exists(deactivatedPool, instr)) {
            variable_struct_remove(deactivatedPool, instr);
            variable_struct_set(activatedPool, instr, 0);
        }
    }

    static deactivate = function(inst) {
        var instr = string(inst);
        if(variable_struct_exists(activatedPool, instr)) {
            variable_struct_set(deactivatedPool, instr, 0);
            variable_struct_remove(activatedPool, instr);
        }
    }

    static is_activated = function(inst) {
        if(inst < 0) return false;
        var instr = string(inst);
        return variable_struct_exists(activatedPool, instr);
    }

    static is_tracked = function(inst) {
        var instr = string(inst);
        return variable_struct_exists(activatedPool, instr) || 
               variable_struct_exists(deactivatedPool, instr);
    }

    static track = function(inst, activated = true) {
        var instr = string(inst);
        if(activated) {
            variable_struct_set(activatedPool, instr, 0);
        }
        else {
            variable_struct_set(deactivatedPool, instr, 0);
        }
    }

    static detrack = function(inst) {
        var instr = string(inst);
        if(variable_struct_exists(activatedPool, instr)) {
            variable_struct_remove(activatedPool, instr);
        }
        else if(variable_struct_exists(deactivatedPool, instr)) {
            variable_struct_remove(deactivatedPool, instr);
        }
        else {
            show_debug_message("Warning: trying to detrack an instance that is not tracked: " + instr);
        }
    }

    static activate_all = function() {
        var _keys = variable_struct_get_names(deactivatedPool);
        for(var i=0; i<array_length(_keys); i++) {
            variable_struct_set(activatedPool, _keys[i], 0);
        }
        delete deactivatedPool;
        deactivatedPool = {};
    }

    static deactivate_all = function() {
        var _keys = variable_struct_get_names(activatedPool);
        for(var i=0; i<array_length(_keys); i++) {
            variable_struct_set(deactivatedPool, _keys[i], 0);
        }
        delete activatedPool;
        activatedPool = {};
    }

    static clear = function() {
        delete deactivatedPool;
        delete activatedPool;
        deactivatedPool = {};
        activatedPool = {};
    }

    static free = function() {
        delete deactivatedPool;
        delete activatedPool;
    }
}