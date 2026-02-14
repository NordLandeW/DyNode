function __test_quick_sort(_size) {
    show_debug_message("=====TEST QUICK SORT: " + string(_size) + "======");

    // Test quick sort speed
    var st, et;
    var _test_array = array_create(_size, 0), _temp_array;
    for(var i=0; i<array_length(_test_array); i++)
        _test_array[i] = dyc_irandom(_size);
    
    // array_sort
    _temp_array = SnapDeepCopy(_test_array);
    st = get_timer();
    array_sort(_temp_array, true);
    et = get_timer();
    show_debug_message("Array sort time: " + string((et - st)/1000) + "ms");

    // quick_sort
    _temp_array = SnapDeepCopy(_test_array);
    st = get_timer();
    quick_sort(_temp_array, true);
    et = get_timer();
    show_debug_message("Quick sort time: " + string((et - st)/1000) + "ms");

    // extern_quick_sort
    _temp_array = SnapDeepCopy(_test_array);
    st = get_timer();
    extern_quick_sort(_temp_array, true);
    et = get_timer();
    show_debug_message("Extern quick sort time: " + string((et - st)/1000) + "ms");

    // array_sort with compare function
    _temp_array = SnapDeepCopy(_test_array);
    st = get_timer();
    array_sort(_temp_array, function(_a, _b) {
        return sign(_b - _a);
    });
    et = get_timer();
    show_debug_message("Array sort with compare function time: " + string((et - st)/1000) + "ms");

    // quick_sort with compare function
    _temp_array = SnapDeepCopy(_test_array);
    st = get_timer();
    quick_sort(_temp_array, function(_a, _b) {
        return sign(_b - _a);
    });
    et = get_timer();
    show_debug_message("Quick sort with compare function time: " + string((et - st)/1000) + "ms");

    // extern_index_sort
    _temp_array = SnapDeepCopy(_test_array);
    st = get_timer();
    var _end_array = extern_index_sort(_temp_array, function(_x) {
        return -_x;
    });
    et = get_timer();
    show_debug_message("Extern index sort with compare function time: " + string((et - st)/1000) + "ms");
}
function __test_vertex_construction(_size) {
    show_debug_message($"=====TEST VERTEX CONSTRUCTION: {_size}======");
    vertex_format_begin();
    vertex_format_add_position();
    vertex_format_add_texcoord();
    vertex_format_add_color();
    var vFormat = vertex_format_end();
    var vBuff = vertex_create_buffer();
    var st, et;
    st = get_timer();
    vertex_begin(vBuff, vFormat);
    for(var i=0; i<_size; i++) {
        vertex_position(vBuff, i, i);
        vertex_texcoord(vBuff, 0, 0);
        vertex_color(vBuff, c_white, 1);
    }
    vertex_end(vBuff);
    et = get_timer();
    show_debug_message("Vertex construction time: " + string((et - st)/1000) + "ms");

    var buff = buffer_create(_size * (4 * 2 + 4 * 2 + 4), buffer_fixed, 1);
    st = get_timer();
    DyCore__test_vbuff_construct(buffer_get_address(buff), _size);
    buffer_set_used_size(buff, _size * (4 * 2 + 4 * 2 + 4));
    vertex_update_buffer_from_buffer(vBuff, 0, buff);
    et = get_timer();
    show_debug_message("Vertex extern construction time: " + string((et - st)/1000) + "ms");

    vertex_delete_buffer(vBuff);
    vertex_format_delete(vFormat);
    buffer_delete(buff);
}

function __test_misc() {

    var _a = { bruh: "bruh" };
    var _b = new sNoteProp();

    if(variable_struct_exists(_a, "copy"))
        show_debug_message("What?");
    if(!variable_struct_exists(_b, "copy"))
        show_debug_message("Bad judging statements.");
    
    // Test regex

    // Basic test helper (regex_match does full-string match)
    var __assert_match = function(_str, _pattern, _expected, _label) {
        var _ok = regex_match(_str, _pattern);
        if(_ok == _expected)
            show_debug_message($"[REGEX][PASS] {_label} | str='{_str}' pattern='{_pattern}' => {string(_ok)}");
        else
            show_debug_message($"[REGEX][FAIL] {_label} | str='{_str}' pattern='{_pattern}' => {string(_ok)} (expected {string(_expected)})");
    };

    // regex_match_ext returns capture array: [0] is full match, then capture groups in order.
    var __assert_match_ext = function(_str, _pattern, _expected, _label) {
        // Basic array equality for debug-style tests.
        // Uses strict length+element checks to keep failures actionable.
        var __arrays_equal = function(_a, _b) {
            if(!is_array(_a) || !is_array(_b)) return false;

            var _len = array_length(_a);
            if(_len != array_length(_b)) return false;

            for(var i = 0; i < _len; i++) {
                if(_a[i] != _b[i]) return false;
            }

            return true;
        };

        var _got = regex_match_ext(_str, _pattern);
        var _ok = __arrays_equal(_got, _expected);
        if(_ok)
            show_debug_message($"[REGEX_EXT][PASS] {_label} | str='{_str}' pattern='{_pattern}' => {json_stringify(_got)}");
        else
            show_debug_message($"[REGEX_EXT][FAIL] {_label} | str='{_str}' pattern='{_pattern}' => {json_stringify(_got)} (expected {json_stringify(_expected)})");
    };

    show_debug_message("=====TEST REGEX_MATCH======");

    // 1) Digits (must match whole string)
    __assert_match("abc123", "^\\d+$", false, "digits only (reject mixed)");
    __assert_match("123", "^\\d+$", true, "digits only");
    __assert_match("abcdef", "^\\d+$", false, "digits only (reject non-digits)");

    // 2) Exact match
    __assert_match("hello", "hello", true, "exact match");
    __assert_match(" hello ", "hello", false, "exact match (whitespace)");

    // 3) Email-ish
    __assert_match("test.user_01@example.com", "^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$", true, "email-ish");
    __assert_match("not-an-email", "^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$", false, "email-ish invalid");

    // 4) Alternation / groups
    __assert_match("cat", "^(cat|dog)$", true, "alternation hit");
    __assert_match("cow", "^(cat|dog)$", false, "alternation miss");

    // 5) Escapes: literal dot
    __assert_match("v1.2.3", "v\\d+\\.\\d+\\.\\d+", true, "semantic version");

    show_debug_message("=====TEST REGEX_MATCH_EXT======");

    // 1) No capture groups => only full match at index 0
    __assert_match_ext("123", "^\\d+$", ["123"], "digits only => full match only");

    // 2) Multiple capture groups
    __assert_match_ext("abc123", "^(abc)(\\d+)$", ["abc123", "abc", "123"], "captures: prefix + digits");

    // 3) Optional capture group (unmatched groups become empty strings)
    __assert_match_ext("abc", "^(abc)(\\d+)?$", ["abc", "abc", ""], "optional capture group");

    // 4) Alternation capture
    __assert_match_ext("dog", "^(cat|dog)$", ["dog", "dog"], "alternation capture");

    // 5) Escapes + 3 capture groups
    __assert_match_ext("v1.2.3", "^(v\\d+)\\.(\\d+)\\.(\\d+)$", ["v1.2.3", "v1", "2", "3"], "semantic version captures");

    // 6) Full-string match behavior (regex_search would match here, but regex_match_ext should not)
    __assert_match_ext("xx123yy", "\\d+", [], "reject partial matches");
    __assert_match_ext("abc", "^\\d+$", [], "reject non-match");
}

function __test_expr() {
    show_debug_message("=====TEST EXPR======");

    var __assert_expr_real = function(_expr, _expected, _label, _eps = 0.000001) {
        try {
            expr_init();
            var _res = expr_eval(_expr);
            var _got = is_struct(_res) ? variable_struct_get(_res, "value") : _res;
            var _ok = abs(_got - _expected) <= _eps;
            if(_ok)
                show_debug_message($"[EXPR][PASS] {_label} | expr='{_expr}' => {string(_got)}");
            else
                show_debug_message($"[EXPR][FAIL] {_label} | expr='{_expr}' => {string(_got)} (expected {string(_expected)})");
        } catch(e) {
            show_debug_message($"[EXPR][FAIL] {_label} | expr='{_expr}' threw: {string(e)}");
        }
    };

    var __assert_expr_with_var = function(_expr, _var_name, _var_value, _expected, _label, _eps = 0.000001) {
        try {
            expr_init();
            expr_set_var(_var_name, _var_value);
            var _res = expr_eval(_expr);
            var _got = is_struct(_res) ? variable_struct_get(_res, "value") : _res;
            var _ok = abs(_got - _expected) <= _eps;
            if(_ok)
                show_debug_message($"[EXPR][PASS] {_label} | expr='{_expr}' => {string(_got)}");
            else
                show_debug_message($"[EXPR][FAIL] {_label} | expr='{_expr}' => {string(_got)} (expected {string(_expected)})");
        } catch(e) {
            show_debug_message($"[EXPR][FAIL] {_label} | expr='{_expr}' threw: {string(e)}");
        }
    };

    var __assert_expr_in_range = function(_expr, _min, _max, _label, _samples = 8) {
        var _ok = true;
        var _last = 0;
        try {
            expr_init();
            for(var _i = 0; _i < _samples; _i++) {
                var _res = expr_eval(_expr);
                _last = is_struct(_res) ? variable_struct_get(_res, "value") : _res;
                if(_last < _min || _last > _max) {
                    _ok = false;
                    break;
                }
            }
            if(_ok)
                show_debug_message($"[EXPR][PASS] {_label} | expr='{_expr}' in [{string(_min)}, {string(_max)}]");
            else
                show_debug_message($"[EXPR][FAIL] {_label} | expr='{_expr}' => {string(_last)} out of [{string(_min)}, {string(_max)}]");
        } catch(e) {
            show_debug_message($"[EXPR][FAIL] {_label} | expr='{_expr}' threw: {string(e)}");
        }
    };

    var __assert_expr_in_range_with_var = function(_expr, _var_name, _var_value, _min, _max, _label, _samples = 8) {
        var _ok = true;
        var _last = 0;
        try {
            expr_init();
            expr_set_var(_var_name, _var_value);
            for(var _i = 0; _i < _samples; _i++) {
                var _res = expr_eval(_expr);
                _last = is_struct(_res) ? variable_struct_get(_res, "value") : _res;
                if(_last < _min || _last > _max) {
                    _ok = false;
                    break;
                }
            }
            if(_ok)
                show_debug_message($"[EXPR][PASS] {_label} | expr='{_expr}' in [{string(_min)}, {string(_max)}]");
            else
                show_debug_message($"[EXPR][FAIL] {_label} | expr='{_expr}' => {string(_last)} out of [{string(_min)}, {string(_max)}]");
        } catch(e) {
            show_debug_message($"[EXPR][FAIL] {_label} | expr='{_expr}' threw: {string(e)}");
        }
    };

    // Basic function calls
    __assert_expr_real("pow(2, 3)", 8, "pow basic");
    __assert_expr_real("sin(0)", 0, "sin basic");
    __assert_expr_real("cos(0)", 1, "cos basic");
    __assert_expr_real("exp(0)", 1, "exp basic");
    __assert_expr_real("step(0.5, 0.4)", 0, "step below edge");
    __assert_expr_real("step(0.5, 0.6)", 1, "step above edge");
    __assert_expr_real("clamp(5, 0, 3)", 3, "clamp upper bound");
    __assert_expr_in_range("rand(10)", 0, 10, "rand range [0, x]");
    __assert_expr_in_range("rand_range(-2, 3)", -2, 3, "rand_range range [l, r]");

    // Function calls combined with operators
    __assert_expr_real("pow(2,3)+cos(0)", 9, "function + operator");
    __assert_expr_real("pow(2, sin(0)+3)", 8, "nested function in args");
    __assert_expr_real("clamp(pow(2,5)-10, 0, 10)", 10, "nested + clamp");
    __assert_expr_real("step(0.5, sin(0)+0.6)", 1, "step with expression input");
    __assert_expr_real("exp(1) * exp(1)", exp(2), "exp multiplication identity", 0.00001);

    // Function calls with variables
    __assert_expr_with_var("pow(time, 2)", "time", 4, 16, "pow with variable");
    __assert_expr_with_var("sin(time)+cos(0)", "time", 0, 1, "sin with variable");
    __assert_expr_with_var("clamp(pow(time, 2)-10, 0, 5)", "time", 4, 5, "clamp+pow with variable");
    __assert_expr_with_var("step(0.5, cos(time))", "time", 0, 1, "step+cos with variable");
    __assert_expr_with_var("exp(time) + pow(time, 2)", "time", 1, exp(1) + 1, "exp+pow with variable", 0.00001);
    __assert_expr_in_range_with_var("rand_range(-time, time)", "time", 2, -2, 2, "rand_range with variable");
}

function test_at_start() {
    show_debug_message("=====DEBUG======")
    
    __test_misc();
    __test_expr();

    var TEST_QUICK_SORT = false;
    var TEST_VERTEX_CONSTRUCTION = false;

    if(TEST_QUICK_SORT) {
        __test_quick_sort(10);
        __test_quick_sort(100);
        __test_quick_sort(1000);
        __test_quick_sort(10000);
        __test_quick_sort(100000);
        __test_quick_sort(500000);
        __test_quick_sort(2000000);
    }
    if(TEST_VERTEX_CONSTRUCTION) {
        __test_vertex_construction(100);
        __test_vertex_construction(1000);
        __test_vertex_construction(10000);
        __test_vertex_construction(100000);
        __test_vertex_construction(500000);
        __test_vertex_construction(2000000);
        __test_vertex_construction(10000000);
    }
}