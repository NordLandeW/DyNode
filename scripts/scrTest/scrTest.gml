function __test_quick_sort(_size) {
    show_debug_message("=====TEST QUICK SORT: " + string(_size) + "======");

    // Test quick sort speed
    var st, et;
    var _test_array = array_create(_size, 0), _temp_array;
    for(var i=0; i<array_length(_test_array); i++)
        _test_array[i] = irandom(_size);
    
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

function __test_compression() {
    var _str_bef = "This is a string.\nthe string."

    var _cSize = DyCore_compress_string(_str_bef, buffer_get_address(global.__DyCore_Buffer), 11);
    
    var _str_aft = DyCore_decompress_string(buffer_get_address(global.__DyCore_Buffer), _cSize);

    if(_str_bef == _str_aft)
        show_debug_message("Validation pass.");
}

function test_at_start() {
    show_debug_message("=====DEBUG======")
    var TEST_QUICK_SORT = true;
    var TEST_COMPRESSION = false;

    if(TEST_QUICK_SORT) {
        __test_quick_sort(10);
        __test_quick_sort(100);
        __test_quick_sort(1000);
        __test_quick_sort(10000);
        __test_quick_sort(100000);
        __test_quick_sort(500000);
        __test_quick_sort(2000000);
    }
    if(TEST_COMPRESSION)
        __test_compression();
}