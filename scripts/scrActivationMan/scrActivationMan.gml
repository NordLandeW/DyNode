

function Activationmanager() constructor {
    
    activatedPool = DyCore_ds_unordered_set_get();
    deactivatedPool = DyCore_ds_unordered_set_get();

    static activate = function(inst) {
        var instr = string(inst);
        if(DyCore_ds_unordered_set_has(deactivatedPool, instr)) {
            DyCore_ds_unordered_set_remove(deactivatedPool, instr);
            DyCore_ds_unordered_set_add(activatedPool, instr);
        }
    }

    static deactivate = function(inst) {
        var instr = string(inst);
        if(DyCore_ds_unordered_set_has(activatedPool, instr)) {
            DyCore_ds_unordered_set_add(deactivatedPool, instr);
            DyCore_ds_unordered_set_remove(activatedPool, instr);
        }
    }

    static is_activated = function(inst) {
        if(inst < 0) return false;
        var instr = string(inst);
        return DyCore_ds_unordered_set_has(activatedPool, instr);
    }

    static is_tracked = function(inst) {
        var instr = string(inst);
        return DyCore_ds_unordered_set_has(activatedPool, instr) || 
               DyCore_ds_unordered_set_has(deactivatedPool, instr);
    }

    static track = function(inst, activated = true) {
        var instr = string(inst);
        if(activated) {
            DyCore_ds_unordered_set_add(activatedPool, instr);
        }
        else {
            DyCore_ds_unordered_set_add(deactivatedPool, instr);
        }
    }

    static detrack = function(inst) {
        var instr = string(inst);
        if(DyCore_ds_unordered_set_has(activatedPool, instr)) {
            DyCore_ds_unordered_set_remove(activatedPool, instr);
        }
        else if(DyCore_ds_unordered_set_has(deactivatedPool, instr)) {
            DyCore_ds_unordered_set_remove(deactivatedPool, instr);
        }
        else {
            show_debug_message("Warning: trying to detrack an instance that is not tracked: " + instr);
        }
    }

    static activate_all = function() {
        DyCore_ds_unordered_set_merge(activatedPool, deactivatedPool);
        DyCore_ds_unordered_set_clear(deactivatedPool);
    }

    static deactivate_all = function() {
        DyCore_ds_unordered_set_merge(deactivatedPool, activatedPool);
        DyCore_ds_unordered_set_clear(activatedPool);
    }

    static clear = function() {
        DyCore_ds_unordered_set_clear(deactivatedPool);
        DyCore_ds_unordered_set_clear(activatedPool);
    }

    static free = function() {
        DyCore_ds_unordered_set_free(deactivatedPool);
        DyCore_ds_unordered_set_free(activatedPool);
    }
}