

function CommandVariant(reqArgs, optArgs, description = "") constructor {
    /// @type {Real} The number of required arguments.
    requiredArgs = reqArgs;
    /// @type {Real} Whether has optional args. Should be 0 or 1.
    optionalArgs = optArgs;
    self.description = description;

    if(optionalArgs < 0 || optionalArgs > 1) {
        throw "Optional arguments must be either 0 or 1.";
    }
}

/// @description The command signature class.
/// @param {String} fullCommand The full name of the command.
/// @param {Function} parser The parser function for the command. It should accept (args, matchedVariant).
/// @param {Array<String>} aliases The aliases of the command.
function CommandSignature(fullCommand, parser, aliases = []) constructor {

    command = fullCommand;
    self.aliases = aliases;
    
    /// @type {Array<Struct.CommandVariant>} The variants of the command.
    variants = [];
    maxRequiredArgsCount = 0;
    hasOptionalArgs = false;
    self._commandParser = parser;

    /// @description Parses the command with the given arguments and matched variant.
    /// @param {Array<String>} args The arguments provided.
    /// @param {Struct.CommandVariant} matchedVariant The matched variant based on the arguments.
    static parse = function(args, matchedVariant) {
        return self._commandParser(args, matchedVariant);
    }

    /// @description Adds a new variant to the command signature.
    /// @param {Real} reqArgs The number of required arguments.
    /// @param {Real} optArgs The number of optional arguments.
    /// @param {String} description The description of the variant.
    static add_variant = function(reqArgs, optArgs, description = "") {
        if(optArgs > 0 && hasOptionalArgs && reqArgs < maxRequiredArgsCount) {
            throw "Cannot add variant with fewer required arguments after variants with optional arguments have been added.";
        }

        // Check duplication.
        for(var i=0, l=array_length(variants); i<l; i++) {
            var variant = variants[i];
            if(variant.requiredArgs == reqArgs && variant.optionalArgs == optArgs) {
                throw $"Variant with {reqArgs} required and {optArgs} optional arguments is already registered for command '{command}'.";
            }
        }

        var variant = new CommandVariant(reqArgs, optArgs, description);
        maxRequiredArgsCount = max(maxRequiredArgsCount, reqArgs);
        hasOptionalArgs = hasOptionalArgs || (optArgs > 0);

        array_push(variants, variant);
    }

    static match_variant = function(argCount) {
        for(var i=0, l=array_length(variants); i<l; i++) {
            var variant = variants[i];
            if(argCount == variant.requiredArgs || 
                (argCount > variant.requiredArgs && argCount == variant.requiredArgs + variant.optionalArgs)) {
                return variant;
            }
        }
        return undefined;
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

    static run_command_line = function(commandLine) {
        commandLine = string_trim(commandLine);
        var parts = string_split(commandLine, " ", true);
        var cmd = string_delete(parts[0], 1, 1);
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

        var args = [];
        for(var i = 1; i < array_length(parts); i++) {
            array_push(args, parts[i]);
        }

        try {
            cmdSig.parse(args, cmdVariant);
        }
        catch(e) {
            echo_error("Error executing command '" + cmd + "': " + string(e));
        }
    }

    static echo = function(message) {
        array_push(messages, string(message));
        show_debug_message("[Console] " + string(message));
    }

    static echo_error = function(message) {
        array_push(messages, "[c_red]" + string(message));
        show_debug_message("[Console][Error] " + string(message));
    }

    static echo_warning = function(message) {
        array_push(messages, "[c_yellow]" + string(message));
        show_debug_message("[Console][Warning] " + string(message));
    }

    static get_messages = function() {
        return messages;
    }

    static get_last_messages = function(count = 10) {
        var startIndex = max(0, array_length(messages) - count);
        var result = [];
        array_copy(result, 0, messages, startIndex, array_length(messages) - startIndex);
        return result;
    }
}

new Console();

#region Builtin Commands

function CommandWidth():CommandSignature("width", undefined, ["w", "wid"]) constructor {
    add_variant(1, 0, "Sets the width of selected notes to the specified value.");

    static parse = function(args, matchedVariant) {
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

function CommandPosition():CommandSignature("position", undefined, ["pos", "p"]) constructor {
    add_variant(1, 0, "Sets the position of selected notes to the specified value.");

    static parse = function(args, matchedVariant) {
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

function CommandTime():CommandSignature("time", undefined, ["t"]) constructor {
    add_variant(1, 0, "Sets the time of selected notes to the specified value.");

    static parse = function(args, matchedVariant) {
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