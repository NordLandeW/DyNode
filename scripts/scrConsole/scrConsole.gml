

function CommandVariant(reqArgs, optArgs, description = "") constructor {
    /// @type {Real} The number of required arguments.
    requiredArgs = reqArgs;
    /// @type {Real} Optional arguments count. Use -1 to indicate a variadic tail.
    optionalArgs = optArgs;
    self.description = description;

    if(optionalArgs != -1 && (optionalArgs < 0 || optionalArgs > 1)) {
        throw "Optional arguments must be 0, 1, or -1 (variadic).";
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

    /// @description Runs a command line.
    /// @param {String} commandLine The full command line to run.
    static run_command_line = function(commandLine) {
        commandLine = string_trim(commandLine);
        if(commandLine == "") {
            return;
        }
        var parts = string_split(commandLine, " ", true);

        var cmd = parts[0];
        if(string_char_at(cmd, 1) == ".")
            cmd = string_copy(cmd, 2, string_length(cmd) - 1);

        var cmdSig = find_command(cmd);
        if(cmdSig == undefined) {
            echo_warning("Command '" + cmd + "' not found.");
            return;
        }

        var cmdVariant = cmdSig.match_variant(array_length(parts) - 1);
        if(cmdVariant == undefined) {
            echo_warning("No matching variant found for command '" + cmd + "' with " + string(array_length(parts) - 1) + " arguments.");
            return;
        }

        var argCount = cmdVariant.requiredArgs + cmdVariant.optionalArgs;
        var mergeArgsCount = max(0, array_length(parts) - 1 - argCount);

        var args = [];
        for(var i = 1; i <= argCount - (mergeArgsCount > 0 ? 1 : 0); i++) {
            array_push(args, parts[i]);
        }
        var mergedArg = parts[argCount];
        for(var i = 0; i < mergeArgsCount; i++) {
            mergedArg += " " + parts[argCount + 1 + i];
        }
        if(mergeArgsCount > 0) {
            array_push(args, mergedArg);
        }

        try {
            cmdSig.execute(args, cmdVariant);
        }
        catch(e) {
            echo_error("Error executing command '" + cmd + "': " + string(e));
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
                var origProp = get_prop();
                width = setWidth;
                update_prop();
                operation_step_add(OPERATION_TYPE.MOVE, origProp, get_prop());
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
        if(setPosition <= 0) {
            console_echo("Position must be a positive real number.");
            return;
        }

        if(editor_select_count() == 0) {
            console_echo("No notes selected.");
            return;
        }

        with(objNote) {
            if(stateType == NOTE_STATES.SELECTED) {
                var origProp = get_prop();
                position = setPosition;
                update_prop();
                operation_step_add(OPERATION_TYPE.MOVE, origProp, get_prop());
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
        if(setTime <= 0) {
            console_echo("Time must be a positive real number.");
            return;
        }

        if(editor_select_count() == 0) {
            console_echo("No notes selected.");
            return;
        }

        with(objNote) {
            if(stateType == NOTE_STATES.SELECTED) {
                var origProp = get_prop();
                time = setTime;
                update_prop();
                operation_step_add(OPERATION_TYPE.MOVE, origProp, get_prop());
            }
        }

        console_echo($"Set time of {string(editor_select_count())} selected notes to {string(setTime)}.");
    }
}

function CommandHelp():CommandSignature("help", ["h", "?"]) constructor {
    add_variant(0, 0, "Lists all commands.");
    add_variant(1, 0, "Shows detailed help for the specified command.");

    static execute = function(args, matchedVariant) {
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