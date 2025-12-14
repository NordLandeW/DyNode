

function CommandVariant(reqArgs, optArgs, description = "") constructor {
    /// @type {Real} The number of required arguments.
    requiredArgs = reqArgs;
    /// @type {Real} Optional arguments count. Use -1 to indicate a variadic tail.
    optionalArgs = optArgs;
    self.description = description;

    if(optionalArgs != -1 && (optionalArgs < 0 || optionalArgs > 1)) {
        throw "Optional arguments must be 0, 1, or -1 (variadic).";
    }

    static get_args_count = function() {
        if(optionalArgs < 0) {
            return -1;
        }
        return requiredArgs + optionalArgs;
    }
}

/// @description The command signature class.
/// @param {String} fullCommand The full name of the command.
/// @param {Array<String>} aliases The aliases of the command.
function CommandSignature(fullCommand, aliases = []) constructor {

    command = fullCommand;
    self.aliases = aliases;
    
    /// @type {Array<Struct.CommandVariant>} The variants of the command.
    variants = [];
    maxRequiredArgsCount = 0;
    hasOptionalArgs = false;

    /// @description Execute the command with the given arguments and matched variant.
    /// @param {Array<String>} args The arguments provided.
    /// @param {Struct.CommandVariant} matchedVariant The matched variant based on the arguments.
    static execute = function(args, matchedVariant) {
        throw "Execute function not implemented for command '" + command + "'.";
    }

    /// @description Adds a new variant to the command signature.
    /// @param {Real} reqArgs The number of required arguments.
    /// @param {Real} optArgs The number of optional arguments. Use -1 to indicate a variadic tail.
    /// @param {String} description The description of the variant.
    static add_variant = function(reqArgs, optArgs, description = "") {
        if(optArgs != 0 && hasOptionalArgs && reqArgs < maxRequiredArgsCount) {
            throw "Cannot add variant with fewer required arguments after variants with optional arguments have been added.";
        }

        // Check duplication.
        for(var i=0, l=array_length(variants); i<l; i++) {
            var variant = variants[i];
            if(variant.requiredArgs == reqArgs && variant.optionalArgs == optArgs) {
                throw $"Variant with {reqArgs} required and {optArgs} optional arguments is already registered for command '{command}'.";
            }
            if(variant.optionalArgs > 0 && optArgs < 0) {
                throw $"Cannot add variadic variant when a variant with optional arguments is already registered for command '{command}'.";
            }
        }

        var variant = new CommandVariant(reqArgs, optArgs, description);
        maxRequiredArgsCount = max(maxRequiredArgsCount, reqArgs);
        hasOptionalArgs = hasOptionalArgs || (optArgs > 0);

        array_push(variants, variant);
    }

    static match_variant = function(argCount) {
        /// @type {Struct.CommandVariant|Real} The best matching variant found.
        var maximumVariant = -1;
        for(var i=0, l=array_length(variants); i<l; i++) {
            var variant = variants[i];

            if(variant.optionalArgs < 0) {
                if(argCount >= variant.requiredArgs) {
                    return variant;
                }
            }
            else if(argCount == variant.requiredArgs ||
                (argCount > variant.requiredArgs && argCount == variant.requiredArgs + variant.optionalArgs)) {
                return variant;
            }

            if(variant.requiredArgs + variant.optionalArgs <= argCount) {
                if(maximumVariant == -1 || maximumVariant.requiredArgs + maximumVariant.optionalArgs < variant.requiredArgs + variant.optionalArgs) {
                    maximumVariant = variant;
                }
            }
        }
        return maximumVariant == -1 ? undefined : maximumVariant;
    }

    static clear_variants = function() {
        variants = [];
        maxRequiredArgsCount = 0;
        hasOptionalArgs = false;
    }

    static add_alias = function(alias) {
        if(!array_contains(self.aliases, alias)) {
            array_push(self.aliases, alias);
        }
        else {
            throw "Alias '" + alias + "' is already registered for command '" + command + "'.";
        }
    }
}

function Console() constructor {

    /// @type {Struct.Console} The global console instance.
    global.console = self;

    /// @type {Array<Struct.CommandSignature>} Registered commands.
    commands = [];

    /// @type {Array<String>} Console output messages.
    messages = [];
    maxMessages = 1000;

    /// @param {String} command The command or alias to check.
    /// @returns {Boolean} Whether the command is registered.
    static has_command = function(command) {
        for(var i = 0; i < array_length(commands); i++) {
            var cmdSig = commands[i];
            if(cmdSig.command == command || array_contains(cmdSig.aliases, command)) {
                return true;
            }
        }
        return false;
    }

    /// @param {String} command The command or alias to find.
    /// @returns {Struct.CommandSignature|undefined} The found command signature or undefined if not found.
    static find_command = function(command) {
        for(var i = 0; i < array_length(commands); i++) {
            var cmdSig = commands[i];
            if(cmdSig.command == command || array_contains(cmdSig.aliases, command)) {
                return cmdSig;
            }
        }
        return undefined;
    }

    /// @param {Struct.CommandSignature} commandSignature The command to register.
    static register_command = function(commandSignature) {
        if(!is_struct(commandSignature)) {
            throw "Invalid command signature provided.";
        }
        if(has_command(commandSignature.command)) {
            throw "Command '" + commandSignature.command + "' is already registered.";
        }

        // Check aliases duplication.
        for(var i = 0; i < array_length(commandSignature.aliases); i++) {
            var alias = commandSignature.aliases[i];
            if(has_command(alias)) {
                throw "Alias '" + alias + "' is already registered by another command.";
            }
        }

        array_push(commands, commandSignature);
        show_debug_message($"Registered command '{commandSignature.command}'.");
    }

    /// @param {Struct.CommandSignature|String} command The command to unregister.
    static unregister_command = function(command) {
        var commandName = command;
        if(is_struct(command)) {
            commandName = command.command;
        }
        for(var i = 0; i < array_length(commands); i++) {
            var cmdSig = commands[i];
            // Only recognize by full command name for unregistration.
            if(cmdSig.command == commandName) {
                array_delete(commands, i, 1);
                return;
            }
        }
        throw "Command '" + commandName + "' is not registered.";
    }

    /// @description Splits a command line into tokens.
    ///
    /// Tokenization rules:
    /// - Whitespace (ASCII <= 32) separates tokens, except inside double quotes.
    /// - Double quotes are not included in output tokens.
    /// - Backslash starts an escape sequence.
    ///
    /// Supported escape sequences:
    /// - \" => "
    /// - \\ => \
    /// - \n  => newline
    /// - \t  => tab
    /// - \r  => carriage return
    /// - \  (space) => a literal space
    ///
    /// @param {String} commandLine The full command line.
    /// @returns {Array<String>} Tokens, including the command name at index 0.
    static command_line_split = function(commandLine) {
        if(!is_string(commandLine)) {
            throw "command_line_split: commandLine must be a string.";
        }

        commandLine = string_trim(commandLine);
        if(commandLine == "") {
            return [];
        }

        var tokens = [];
        var current = "";
        var tokenActive = false;
        var inQuotes = false;
        var escaped = false;

        var len = string_length(commandLine);
        for(var i = 1; i <= len; i++) {
            var ch = string_char_at(commandLine, i);

            if(escaped) {
                tokenActive = true;
                switch(ch) {
                    case "n": current += "\n"; break;
                    case "t": current += "\t"; break;
                    case "r": current += "\r"; break;
                    case "\\": current += "\\"; break;
                    case "\"": current += "\""; break;
                    case " ": current += " "; break;
                    default:
                        throw $"command_line_split: unknown escape sequence '\\{ch}' at position {string(i)}.";
                }
                escaped = false;
                continue;
            }

            if(ch == "\\") {
                escaped = true;
                tokenActive = true;
                continue;
            }

            if(ch == "\"") {
                inQuotes = !inQuotes;
                tokenActive = true;
                continue;
            }

            if(!inQuotes && ord(ch) <= 32) {
                if(tokenActive) {
                    array_push(tokens, current);
                    current = "";
                    tokenActive = false;
                }
                continue;
            }

            current += ch;
            tokenActive = true;
        }

        if(escaped) {
            throw "command_line_split: command line ends with a trailing escape character '\\'.";
        }
        if(inQuotes) {
            throw "command_line_split: unterminated double quote in command line.";
        }
        if(tokenActive) {
            array_push(tokens, current);
        }

        return tokens;
    }

    /// @description Tokenizes arguments from an already-split token array.
    ///
    /// Note: If more arguments are provided than `argCount`, the exceeding part will be
    /// merged into the last argument, separated by spaces.
    ///
    /// @param {Array<String>} tokens Tokens returned by `command_line_split()`.
    /// @param {Real} argCount The maximum number of arguments expected (excluding the command itself).
    /// @returns {Array<String>} Parsed arguments.
    static command_line_tokenize_from_tokens = function(tokens, argCount) {
        if(!is_array(tokens)) {
            throw "command_line_tokenize_from_tokens: tokens must be an array.";
        }
        if(!is_real(argCount)) {
            throw "command_line_tokenize_from_tokens: argCount must be a real number.";
        }

        argCount = floor(argCount);
        if(argCount < 0) {
            throw "command_line_tokenize_from_tokens: argCount must be >= 0.";
        }

        if(array_length(tokens) == 0) {
            return [];
        }

        var actualArgsCount = max(0, array_length(tokens) - 1);
        if(argCount == 0) {
            return [];
        }

        var args = [];

        if(actualArgsCount <= argCount) {
            for(var a = 1; a <= actualArgsCount; a++) {
                array_push(args, tokens[a]);
            }
            return args;
        }

        for(var a = 1; a <= argCount - 1; a++) {
            array_push(args, tokens[a]);
        }

        var mergedArg = tokens[argCount];
        for(var a = argCount + 1; a <= actualArgsCount; a++) {
            mergedArg += " " + tokens[a];
        }
        array_push(args, mergedArg);

        return args;
    }

    /// @description Tokenizes a command line into arguments.
    /// Supports:
    /// - Double-quoted strings (quotes are not included in output tokens)
    /// - Escape sequences (\" \\ \n \t \r and escaping a space)
    ///
    /// Note: If more arguments are provided than `argCount`, the exceeding part will be
    /// merged into the last argument, separated by spaces.
    ///
    /// @param {String} commandLine The full command line.
    /// @param {Real} argCount The maximum number of arguments expected (excluding the command itself).
    /// @returns {Array<String>} Parsed arguments.
    static command_line_tokenize = function(commandLine, argCount) {
        var tokens = command_line_split(commandLine);
        return command_line_tokenize_from_tokens(tokens, argCount);
    }

    /// @description Runs a command line.
    /// @param {String} commandLine The full command line to run.
    static run_command_line = function(commandLine) {
        try {
            var tokens = command_line_split(commandLine);
            if(array_length(tokens) == 0) {
                return -1;
            }

            var cmd = tokens[0];
            if(string_char_at(cmd, 1) == "." || string_char_at(cmd, 1) == "/") {
                cmd = string_copy(cmd, 2, string_length(cmd) - 1);
            }

            var cmdSig = find_command(cmd);
            if(cmdSig == undefined) {
                var found = false;
                // Single argument variation.
                // Example: w2.5 or width2.5 to set width to 2.5 directly.
                if(array_length(tokens) == 1) {
                    var matches = regex_match_ext(cmd, "^([a-zA-Z]+)([-+]?[0-9]*\\.?[0-9]+)$");
                    if(array_length(matches) == 3) {
                        cmdSig = find_command(matches[1]);
                        if(cmdSig != undefined) {
                            found = true;
                            tokens[0] = matches[1];
                            array_push(tokens, matches[2]);
                        }
                    }
                }
                
                if(!found) {
                    echo_warning("Command '" + cmd + "' not found.");
                    return -1;
                }
            }

            var actualArgsCount = max(0, array_length(tokens) - 1);
            var cmdVariant = cmdSig.match_variant(actualArgsCount);
            if(cmdVariant == undefined) {
                echo_warning("No matching variant found for command '" + cmd + "' with " + string(actualArgsCount) + " arguments.");
                return -1;
            }

            // For variadic variants, accept all provided args (no forced merging).
            // For non-variadic variants, keep the feature: merge exceeding args into the last one.
            var argCountExpected = cmdVariant.optionalArgs < 0
                ? actualArgsCount
                : (cmdVariant.requiredArgs + cmdVariant.optionalArgs);

            var args = command_line_tokenize_from_tokens(tokens, argCountExpected);
            var result = cmdSig.execute(args, cmdVariant);

            return 0;
        }
        catch(e) {
            // Includes both parsing/tokenization errors and execution errors.
            echo_error("Error executing command line: " + string(e));
            return -1;
        }
    }

    static echo = function(message) {
        array_push(messages, string(message));
        show_debug_message("[Console] " + string(message));

        if(array_length(messages) > maxMessages) {
            array_delete(messages, 0, array_length(messages) - maxMessages);
        }
    }

    static echo_error = function(message) {
        echo("[c_red]" + string(message));
    }

    static echo_warning = function(message) {
        echo("[c_yellow]" + string(message));
    }

    static get_messages = function() {
        return messages;
    }

    static get_last_messages = function(offset, count) {
        var startIndex = max(0, array_length(messages) - count - offset);
        var result = [];
        array_copy(result, 0, messages, startIndex, min(count, array_length(messages) - startIndex));
        return result;
    }
}

new Console();

#region Builtin Commands

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

        var selectedCount = editor_select_count();
        if(selectedCount == 0) {
            console_echo("No notes selected.");
            return;
        }

        var processedCount = 0;
        var skippedSubCount = 0;

        with(objNote) {
            if(stateType == NOTE_STATES.SELECTED) {
                // Sub notes are derived from holds; moving them directly would desync undo history.
                // Users should snap the hold head instead.
                if(noteType == NOTE_TYPE.SUB) {
                    skippedSubCount++;
                }
                else {
                    var prop = get_prop();

                    // For holds we must snap both endpoints independently.
                    // If we derived endTime from the snapped start, we would unintentionally shift the tail.
                    var originalStartTime = prop.time;
                    var originalEndTime = prop.time + prop.lastTime;

                    var snappedStart = editor_snap_to_grid_time(originalStartTime, prop.side, true, true, snapMode).time;
                    prop.time = snappedStart;

                    // Holds are defined by (startTime, endTime). Snap both ends so the segment aligns.
                    if(noteType == NOTE_TYPE.HOLD) {
                        var snappedEnd = editor_snap_to_grid_time(originalEndTime, prop.side, true, true, snapMode).time;
                        prop.lastTime = max(1, snappedEnd - snappedStart);
                    }

                    set_prop(prop, true);

                    processedCount++;
                }
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

function CommandLinear():CommandSignature("linear", ["lin"]) constructor {
    add_variant(0, 0, "Applies linear sampling on selected notes.");
    add_variant(0, -1, "Applies linear sampling on selected notes. Overwriting noteType and beat division with extra arguments.");

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

        editor_linear_sampling(typeOverwrite, beatDivOverwrite);
    }
}
#endregion

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

/// @param {Struct.CommandSignature} commandSignature The command to register.
function command_register(commandSignature) {
    global.console.register_command(commandSignature);
}

/// @param {Struct.CommandSignature|String} command The command to unregister.
function command_unregister(command) {
    global.console.unregister_command(command);
}

function command_init() {
    command_register(new CommandHelp());
    command_register(new CommandClear());
    command_register(new CommandEcho());
    command_register(new CommandWidth());
    command_register(new CommandPosition());
    command_register(new CommandTime());
    command_register(new CommandSide());
    command_register(new CommandRandomize());
    command_register(new CommandExpr());
    command_register(new CommandQuit());
    command_register(new CommandSnap());
    command_register(new CommandLinear());
}

function console_init() {
    command_init();

    instance_create_depth(0, 0, 0, objConsole);
}

function console_run(commandLine) {
    global.console.run_command_line(commandLine);
}

function console_echo(message) {
    global.console.echo(message);
}

function console_echo_error(message) {
    global.console.echo_error(message);
}

function console_echo_warning(message) {
    global.console.echo_warning(message);
}