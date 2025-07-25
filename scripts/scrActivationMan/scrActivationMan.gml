

function Activationmanager() constructor {
    
    deactivatedPool = {};

    static activate = function(inst) {
        var instr = string(inst);
        if(variable_struct_exists(deactivatedPool, instr)) {
            variable_struct_remove(deactivatedPool, instr);
        }
    }

    static deactivate = function(inst) {
        var instr = string(inst);
        if(!variable_struct_exists(deactivatedPool, instr)) {
            variable_struct_set(deactivatedPool, instr, 0);
        }
    }

    static is_activated = function(inst) {
        if(inst < 0) return false;
        var instr = string(inst);
        return !variable_struct_exists(deactivatedPool, instr);
    }

    static detrack = function(inst) {
        activate(inst);
    }

    static activate_all = function() {
        delete deactivatedPool;
        deactivatedPool = {};
    }

    static free = function() {
        delete deactivatedPool;
    }
}