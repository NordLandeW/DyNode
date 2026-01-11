
#macro CONSOLE_MAX_LINE_LENGTH 200

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
        var newMessages = string_split(string(message), "\n");
        for(var i = 0; i < array_length(newMessages); i++) {
            var line = newMessages[i];
            // Split long lines.
            while(string_length(line) > CONSOLE_MAX_LINE_LENGTH) {
                var part = string_copy(line, 1, CONSOLE_MAX_LINE_LENGTH);
                array_push(messages, part);
                line = string_copy(line, CONSOLE_MAX_LINE_LENGTH + 1, string_length(line) - CONSOLE_MAX_LINE_LENGTH);
            }
            array_push(messages, line);
        }

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
    command_register_builtin_commands();
}

function console_init() {
    command_init();

    instance_create_depth(0, 0, 0, objConsole);
}

function console_run(commandLine) {
    return global.console.run_command_line(commandLine);
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