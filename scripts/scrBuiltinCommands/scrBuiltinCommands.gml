
global.BUILTIN_COMMANDS = [
    CommandWidth,
    CommandPosition,
    CommandTime,
    CommandSide,
    CommandHelp,
    CommandClear,
    CommandEcho,
    CommandExpr,
    CommandQuit,
    CommandRandomize,
    CommandSnap,
    CommandLinear,
    CommandCosine,
    CommandCatmullRom,
    CommandCubic,
    CommandCenter,
    CommandPurge,
    CommandFix,
    CommandDuplicate,
    CommandDeduplicate
]

function CommandWidth():CommandSignature("width", ["w", "wid"]) constructor {
    add_variant(1, 0, "Sets the width of selected notes to the specified value.");

    static execute = function(args, matchedVariant) {
        command_arg_check_real(args[0]);        
        command_check_in_editor();
        
        var setWidth = real(args[0]);
        if(setWidth <= 0) {
            console_echo("Width must be a positive real number.");
            return;
        }

        if(editor_select_count() == 0) {
            console_echo("No notes selected.");
            return;
        }

        with(objNote) {
            if(stateType == NOTE_STATES.SELECTED) {
                var prop = get_prop();
                prop.width = setWidth;
                set_prop(prop, true);
            }
        }

        console_echo($"Set width of {string(editor_select_count())} selected notes to {string(setWidth)}.");
    }
}

function CommandPosition():CommandSignature("position", ["pos", "p"]) constructor {
    add_variant(1, 0, "Sets the position of selected notes to the specified value.");

    static execute = function(args, matchedVariant) {
        command_arg_check_real(args[0]);        
        command_check_in_editor();
        
        var setPosition = real(args[0]);

        if(editor_select_count() == 0) {
            console_echo("No notes selected.");
            return;
        }

        with(objNote) {
            if(stateType == NOTE_STATES.SELECTED) {
                var prop = get_prop();
                prop.position = setPosition;
                set_prop(prop, true);
            }
        }

        console_echo($"Set position of {string(editor_select_count())} selected notes to {string(setPosition)}.");
    }
}

function CommandTime():CommandSignature("time", ["t"]) constructor {
    add_variant(1, 0, "Sets the time of selected notes to the specified value.");

    static execute = function(args, matchedVariant) {
        command_arg_check_real(args[0]);
        command_check_in_editor();

        var setTime = real(args[0]);

        if(editor_select_count() == 0) {
            console_echo("No notes selected.");
            return;
        }

        with(objNote) {
            if(stateType == NOTE_STATES.SELECTED) {
                var prop = get_prop();
                prop.time = setTime;
                set_prop(prop, true);
            }
        }

        console_echo($"Set time of {string(editor_select_count())} selected notes to {string(setTime)}.");
    }
}

function CommandSide():CommandSignature("side", ["s"]) constructor {
    add_variant(1, 0, "Sets the side of selected notes to the specified value.");
    add_variant(2, 0, "Sets the side of selected notes to the specified value. Overwriting the visual consitent mode with the specified boolean.");

    static execute = function(args, matchedVariant) {
        command_check_in_editor();

        var argCount = matchedVariant.get_args_count();
        command_arg_check_integer(args[0]);
        if(argCount > 1)
            command_arg_check_boolean(args[1]);

        var setSide = int64(args[0]);
        setSide = (setSide % 3 + 3) % 3;

        if(editor_select_count() == 0) {
            console_echo("No notes selected.");
            return;
        }

        with(objNote) {
            if(stateType == NOTE_STATES.SELECTED) {
                var origProp = get_prop();
                if(argCount == 1)
                    change_side(setSide);
                else
                    change_side(setSide, command_arg_to_boolean(args[1]));
                operation_step_add(OPERATION_TYPE.MOVE, origProp, get_prop());
            }
        }

        console_echo($"Set side of {string(editor_select_count())} selected notes to {string(setSide)}.");
    }
}


function CommandHelp():CommandSignature("help", ["h", "?"]) constructor {
    add_variant(0, 0, "Lists all commands.");
    add_variant(1, 0, "Shows detailed help for the specified command.");

    static execute = function(args, matchedVariant) {
        // Force long command.
        with(objConsole)
            quickCommand = false;

        var join_string_array = function(arr) {
            var out = "";
            for(var i = 0, l = array_length(arr); i < l; i++) {
                out += (i > 0 ? ", " : "") + string(arr[i]);
            }
            return out;
        };

        var build_usage = function(cmdName, variant) {
            var usage = string(cmdName);

            for(var i = 0; i < variant.requiredArgs; i++) {
                usage += " <arg" + string(i + 1) + ">";
            }

            if(variant.optionalArgs < 0) {
                usage += " <args...>";
            }
            else {
                for(var i = 0; i < variant.optionalArgs; i++) {
                    usage += " [arg" + string(variant.requiredArgs + i + 1) + "]";
                }
            }

            return usage;
        };

        var commands = [];
        array_copy(commands, 0, global.console.commands, 0, array_length(global.console.commands));

        // Keep output stable/predictable for users by sorting commands alphabetically.
        for(var i = 0, l = array_length(commands); i < l - 1; i++) {
            for(var j = i + 1; j < l; j++) {
                if(string_lower(commands[i].command) > string_lower(commands[j].command)) {
                    var tmp = commands[i];
                    commands[i] = commands[j];
                    commands[j] = tmp;
                }
            }
        }

        if(array_length(args) == 0) {
            console_echo($"Available commands ({string(array_length(commands))}):");

            for(var i = 0, l = array_length(commands); i < l; i++) {
                var cmdSig = commands[i];

                var aliasText = "";
                if(array_length(cmdSig.aliases) > 0) {
                    aliasText = " (" + join_string_array(cmdSig.aliases) + ")";
                }

                console_echo("- " + string(cmdSig.command) + aliasText);

                for(var v = 0, vl = array_length(cmdSig.variants); v < vl; v++) {
                    var variant = cmdSig.variants[v];
                    var usage = build_usage(cmdSig.command, variant);

                    var desc = string_trim(string(variant.description));
                    if(desc != "") {
                        console_echo("    " + usage + " - " + desc);
                    }
                    else {
                        console_echo("    " + usage);
                    }
                }
            }

            console_echo("Tip: help <command> for details.");
            return;
        }

        var target = args[0];
        var targetSig = global.console.find_command(target);
        if(targetSig == undefined) {
            console_echo_warning("Command '" + string(target) + "' not found.");
            return;
        }

        console_echo("Help for: " + string(targetSig.command));

        if(array_length(targetSig.aliases) > 0) {
            console_echo("Aliases: " + join_string_array(targetSig.aliases));
        }

        for(var v = 0, vl = array_length(targetSig.variants); v < vl; v++) {
            var variant = targetSig.variants[v];
            var usage = build_usage(targetSig.command, variant);

            var desc = string_trim(string(variant.description));
            if(desc != "") {
                console_echo("- " + usage + " - " + desc);
            }
            else {
                console_echo("- " + usage);
            }
        }
    }
}

function CommandClear():CommandSignature("clear", ["cls"]) constructor {
    add_variant(0, 0, "Clears console output history.");

    static execute = function(args, matchedVariant) {
        global.console.messages = [];

        // Leaving one line after clearing makes the action visible to the user.
        console_echo("Console cleared.");
    }
}

function CommandEcho():CommandSignature("echo", ["print"]) constructor {
    add_variant(0, 0, "Prints an empty line.");
    add_variant(1, 0, "Prints the specified message.");

    static execute = function(args, matchedVariant) {
        if(array_length(args) == 0) {
            console_echo("");
        }
        else {
            console_echo(args[0]);
        }
    }
}

function CommandExpr():CommandSignature("expr", ["e"]) constructor {
    add_variant(1, 0, "Execute the expression on selected notes / all notes");

    static execute = function(args, matchedVariant) {
        command_check_in_editor();
        var expr = args[0];
        if(expr == "") {
            console_echo_warning("Expression cannot be empty.");
            return;
        }
        try {
            advanced_expr(expr);
        }
        catch(e) {
            console_echo_error("Error evaluating expression: " + string(e));
        }
    }
}

function CommandQuit():CommandSignature("quit", ["exit", "q"]) constructor {
    add_variant(0, 0, "Quits the console.");

    static execute = function(args, matchedVariant) {
        with(objConsole)
            unfocus();
    }
}

function CommandRandomize():CommandSignature("randomize", ["rand"]) constructor {
    add_variant(0, 0, "Randomizes properties of selected notes.");

    static execute = function(args, matchedVariant) {
        command_check_in_editor();

        chart_randomize();
    }
}

// Snap all selected notes to the timing grid.
//
// Modes:
// - pre:     prefer snapping to the previous grid line (SNAP_MODE.SNAP_PRE)
// - post:    prefer snapping to the next grid line (SNAP_MODE.SNAP_POST)
// - nearest: prefer snapping to the nearest grid line (SNAP_MODE.SNAP_AROUND)
//
// Usage:
//   .snap
//   .snap <pre|post|nearest>
function CommandSnap():CommandSignature("snap", []) constructor {
    add_variant(0, 0, "Snaps all selected notes to the timing grid (default mode: pre).");
    add_variant(1, 0, "Snaps all selected notes to the timing grid. Mode: pre|post|nearest.");

    static execute = function(args, matchedVariant) {
        command_check_in_editor();

        var snapMode = SNAP_MODE.SNAP_PRE;
        if(array_length(args) > 0) {
            var modeArg = string_lower(string_trim(args[0]));
            switch(modeArg) {
                case "pre":
                case "prev":
                case "previous":
                    snapMode = SNAP_MODE.SNAP_PRE;
                    break;

                case "post":
                case "next":
                case "after":
                    snapMode = SNAP_MODE.SNAP_POST;
                    break;

                case "nearest":
                case "near":
                case "around":
                    snapMode = SNAP_MODE.SNAP_AROUND;
                    break;

                default:
                    console_echo_warning("Invalid mode. Expected: pre | post | nearest.");
                    return;
            }
        }

        /// @type {Array<Struct.sNoteProp>} The note properties to process.
        var noteProps = command_get_target_notes();

        var processedCount = 0;
        var skippedSubCount = 0;

        for(var i = 0, l = array_length(noteProps); i < l; i++) {
            var curProp = noteProps[i];
            if(curProp.noteType == NOTE_TYPE.SUB) {
                skippedSubCount++;
            }
            else {
                var originalStartTime = curProp.time;
                var originalEndTime = curProp.time + curProp.lastTime;

                var snappedStart = editor_snap_to_grid_time(originalStartTime, curProp.side, true, true, snapMode).time;
                curProp.time = snappedStart;

                if(curProp.noteType == NOTE_TYPE.HOLD) {
                    var snappedEnd = editor_snap_to_grid_time(originalEndTime, curProp.side, true, true, snapMode).time;
                    curProp.lastTime = max(1, snappedEnd - snappedStart);
                }

                dyc_update_note(curProp, true);

                processedCount++;
            }
        }

        if(processedCount > 0) {
            note_sort_request();
        }

        var modeName = "pre";
        if(snapMode == SNAP_MODE.SNAP_POST) modeName = "post";
        else if(snapMode == SNAP_MODE.SNAP_AROUND) modeName = "nearest";

        console_echo($"Snapped {string(processedCount)} selected notes to grid ({modeName}).");
        if(skippedSubCount > 0) {
            console_echo_warning($"Skipped {string(skippedSubCount)} sub note(s). Select the hold head instead.");
        }
    }
}

function CommandCurve(fullCommand, alias):CommandSignature(fullCommand, alias) constructor {
    
    static curve_add_variant = function(name) {
        add_variant(0, 0, $"Applies {name} curve sampling on selected notes.");
        add_variant(0, -1, $"Applies {name} curve sampling on selected notes. Overwriting noteType / beat division / other settings with extra arguments.");
    }

    /// @type {Function} The sampling function to use.
    sampling_function = undefined;

    static execute = function(args, matchedVariant) {
        command_check_in_editor();
        var typeOverwrite = -1;
        var beatDivOverwrite = -1;
        if(matchedVariant.get_args_count() < 0) {
            for(var i = 0, l = array_length(args); i < l; i++) {
                if(command_arg_check_note_type(args[i], false)) {
                    typeOverwrite = command_arg_to_note_type(args[i]);
                }
                else if(command_arg_check_integer(args[i], false)) {
                    beatDivOverwrite = int64(args[i]);
                    if(beatDivOverwrite <= 0) {
                        console_echo_warning("Beat division must be a positive integer.");
                        return;
                    }
                }
                else {
                    console_echo_warning("Invalid argument. Expected note type or beat division integer. Note type options: tap|note|normal, hold, slide|chain.");
                    return;
                }
            }
        }

        if(editor_select_count() < 2) {
            console_echo_warning("At least two notes must be selected to apply linear sampling.");
            return;
        }

        sampling_function(typeOverwrite, beatDivOverwrite);
    }
}

function CommandLinear():CommandCurve("linear", ["lin"]) constructor {
    curve_add_variant("linear");

    sampling_function = editor_linear_sampling;
}

function CommandCosine():CommandCurve("cosine", ["cos"]) constructor {
    curve_add_variant("cosine");

    sampling_function = editor_cosine_sampling;
}

function CommandCatmullRom():CommandCurve("catrom", ["crom"]) constructor {
    curve_add_variant("Catmull-Rom");

    sampling_function = editor_catmull_rom_sampling;
}

function CommandCubic():CommandCurve("cubic", ["cub"]) constructor {
    curve_add_variant("natural cubic");

    sampling_function = editor_cubic_sampling;
}

function CommandCenter():CommandSignature("center", ["centre", "cen"]) constructor {
    add_variant(0, 0, "Centers the position of selected notes.");

    static execute = function(args, matchedVariant) {
        command_check_in_editor();

        if(editor_select_count() == 0) {
            console_echo("No notes selected.");
            return;
        }

        editor_selected_centrialize();

        console_echo($"Centered position of {editor_select_count()} selected notes.");
    }
}

function CommandPurge():CommandSignature("purge") constructor {
    add_variant(0, 0, "Remove all notes from the chart.");

    static execute = function(args, matchedVariant) {
        command_check_in_editor();

        note_delete_all(true);

        console_echo($"Purged all notes from the chart.");
    }
}

function CommandFix():CommandSignature("fix") constructor {
    add_variant(0, 0, "A general fixing tool.");

    static execute = function(args, matchedVariant) {
        command_check_in_editor();

        editor_fix_notes();

        console_echo($"Fixed note properties in the chart.");
    }
}

function CommandDuplicate():CommandSignature("duplicate", ["dup"]) constructor {
    add_variant(0, 1, "Quick duplicate selected notes.");

    static execute = function(args, matchedVariant) {
        command_check_in_editor();
        var count = 1;
        if(array_length(args) > 0) {
            command_arg_check_integer(args[0]);
            count = int64(args[0]);
        }

        if(editor_select_count() == 0) {
            console_echo("No notes selected.");
            return;
        }

        for(var i=0; i<count; i++) {
            editor_note_duplicate_quick();
        }

        console_echo($"Duplicated {string(editor_select_count())} selected notes {string(count)} time(s).");
    }
}

function CommandDeduplicate():CommandSignature("deduplicate", ["dedup"]) constructor {
    add_variant(0, 0, "Removes duplicate notes from the chart.");

    static execute = function(args, matchedVariant) {
        command_check_in_editor();

        var noteProps = command_get_target_notes();
        var removedCount = editor_deduplicate_notes(noteProps);

        console_echo($"Removed {string(removedCount)} duplicate notes from the chart.");
    }
}

function command_register_builtin_commands() {
    for(var i = 0, l = array_length(global.BUILTIN_COMMANDS); i < l; i++) {
        var cmdSig = new global.BUILTIN_COMMANDS[i]();
        global.console.register_command(cmdSig);
    }
}

function command_arg_check_real(arg, abort = true) {
    if(!regex_is_real(arg)) {
        if(abort)
            throw "Argument '" + string(arg) + "' is not a valid real number.";
        return false;
    }
    return true;
}

function command_arg_check_integer(arg, abort = true) {
    if(!regex_is_integer(arg)) {
        if(abort)
            throw "Argument '" + string(arg) + "' is not a valid integer.";
        return false;
    }
    return true;
}

function command_arg_check_boolean(arg, abort = true) {
    var lowerArg = string_lower(string_trim(arg));
    if(lowerArg != "true" && lowerArg != "false" && lowerArg != "1" && lowerArg != "0") {
        if(abort)
            throw "Argument '" + string(arg) + "' is not a valid boolean.";
        return false;
    }
    return true;
}
 
function command_arg_to_boolean(arg) {
    var lowerArg = string_lower(string_trim(arg));
    return (lowerArg == "true" || lowerArg == "1");
}

function command_arg_check_note_type(arg, abort = true) {
    arg = string_lower(string_trim(arg));
    static availableOptions = ["tap", "note", "hold", "slide", "chain", "normal"];
    if(!array_contains(availableOptions, arg)) {
        if(abort)
            throw "Argument '" + string(arg) + "' is not a valid note type. Available types: " + string_join(availableOptions, ", ") + ".";
        return false;
    }
    return true;
}

function command_arg_to_note_type(arg) {
    arg = string_lower(string_trim(arg));
    switch(arg) {
        case "tap":
        case "note":
        case "normal":
            return NOTE_TYPE.NORMAL;
        case "hold":
            return NOTE_TYPE.HOLD;
        case "slide":
        case "chain":
            return NOTE_TYPE.CHAIN;
        default:
            throw "Invalid note type: '" + string(arg) + "'.";
    }
}

function command_check_in_editor(abort = true) {
    if(!instance_exists(objEditor)) {
        if(abort)
            throw "This command can only be used in the editor.";
        return false;
    }
}

/// @description Get note properties to process based on selection.
/// @returns {Array<Struct.sNoteProp>} The note properties to process.
function command_get_target_notes() {
    command_check_in_editor();

    /// @type {Array<Struct.sNoteProp>} The note properties to process.
    var noteProps = [];
    var selectedCount = editor_select_count();
    if(selectedCount == 0)
        noteProps = note_get_all_props();
    else noteProps = editor_get_selected_notes();

    return noteProps;
}