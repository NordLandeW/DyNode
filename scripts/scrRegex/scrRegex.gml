
/// @description Checks if the given string matches the specified regex pattern.
/// @param {String} str The string to be tested.
/// @param {String} pattern The regex pattern to match against.
/// @returns {Bool} True if the string matches the pattern, false otherwise.
function regex_match(str, pattern) {
    return DyCore_regex_match(str, pattern);
}

/// @description Matches the given string against the specified regex pattern and returns all matches.
/// @param {String} str The string to be tested.
/// @param {String} pattern The regex pattern to match against.
/// @returns {Array} An array of all matches found in the string.
function regex_match_ext(str, pattern) {
    var result = DyCore_regex_match_ext(str, pattern);
    if(result == "") {
        return [];
    } else {
        return json_parse(result);
    }
}


#region Regex Simple Tests

/// @description Checks if the given string is a valid real number (GML-style: decimal + optional exponent).
/// @param {String} str The string to be tested.
/// @returns {Bool} True if the string can be parsed as a real without relying on trimming/locale.
function regex_is_real(str) {
    // Keeps validation deterministic and independent from runtime parsing quirks (e.g., whitespace trimming).
    return regex_match(str, "^[+-]?(\\d+(\\.\\d*)?|\\.\\d+)([eE][+-]?\\d+)?$");
}

/// @description Checks if the given string is a valid base-10 integer.
/// @param {String} str The string to be tested.
/// @returns {Bool} True if the string is a signed integer with at least one digit.
function regex_is_integer(str) {
    return regex_match(str, "^[+-]?\\d+$");
}

#endregion