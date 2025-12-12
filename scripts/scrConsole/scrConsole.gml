

function CommandVariant(reqArgs, optArgs, description = "") constructor {
    requiredArgs = reqArgs;
    optionalArgs = optArgs;
    self.description = description;
}

/// @description The command signature class.
/// @param {String} fullCommand The full name of the command.
/// @param {Function} parser The parser function for the command.
/// @param {Array<String>} aliases The aliases of the command.
function CommandSignature(fullCommand, parser, aliases = []) constructor {

    command = fullCommand;
    self.aliases = aliases;
    
    variants = [];
    maxRequiredArgsCount = 0;
    hasOptionalArgs = false;
    commandParser = parser;

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

    static parse = function(args) {
        return commandParser(args);
    }
}

function Console() constructor {

    global.console = self;

    /// @type {Array<Struct.CommandSignature>} Registered commands.
    commands = [];

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

    /// @param {Struct.CommandSignature} commandSignature The command to register.
    static register_command = function(commandSignature) {
        if(!is_struct(commandSignature)) {
            throw "Invalid command signature provided.";
        }
        if(has_command(commandSignature.command)) {
            throw "Command '" + commandSignature.command + "' is already registered.";
        }
        array_push(commands, commandSignature);
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
        throw "Not Implemented";
    }
}

new Console();

#region Builtin Commands

function CommandWidth():CommandSignature("width", undefined, ["w", "wid"]) constructor {

    static parser = function(args) {

    }
}

#endregion

function command_init() {



}