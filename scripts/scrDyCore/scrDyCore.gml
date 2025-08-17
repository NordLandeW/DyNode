/// DyCore Interface.

enum DYCORE_ASYNC_EVENT_TYPE { PROJECT_SAVING, GENERAL_ERROR, GM_ANNOUNCEMENT };
function DyCoreManager() constructor {
    // DyCore Step function.
    static step = function() {
        // Manage async events.
        if(DyCore_has_async_event()) {
            var _async_event = DyCore_get_async_event();
            if(_async_event == "") {
                show_debug_message("!Warning: DyCore async event is empty.");
                announcement_error("DyCore async events parsed failed.");
                return;
            }
            else {
                do_async_events(json_parse(_async_event));
            }
        }
    }

    static do_async_events = function(event) {
        show_debug_message(event);
        switch(event[$ "type"]) {
            case DYCORE_ASYNC_EVENT_TYPE.PROJECT_SAVING:
                project_save_callback(event);
                break;
            case DYCORE_ASYNC_EVENT_TYPE.GENERAL_ERROR:
                announcement_error(i18n_get("anno_dycore_error", event[$ "content"]));
                break;
            case DYCORE_ASYNC_EVENT_TYPE.GM_ANNOUNCEMENT:
                var _anno_content = json_parse(event[$ "content"]);
                var _msg = _anno_content[$ "msg"];
                var _args = _anno_content[$ "args"];
                switch(event[$ "status"]) {
                    case 0:
                        announcement_play(i18n_get(_msg, _args));
                        break;
                    case 1:
                        announcement_warning(i18n_get(_msg, _args));
                        break;
                    case 2:
                        announcement_error(i18n_get(_msg, _args));
                        break;
                    default:
                        announcement_error("Unknown GM announcement type from DyCore.");
                        break;
                }
                break;
            default:
                show_debug_message("!Warning: Unknown dycore async event type.");
                break;
        }
    }
}

function dyc_init() {
    if(DyCore_init(window_handle()) != "success") {
        show_error("DyCore Initialized Failed.", false);
    }
    /// Add sprites data.
    // Add sprNote2.
    var uv = sprite_get_uvs(sprNote2, 0);
    dyc_add_sprite_data({
        name: "sprNote",
        uv00: uv[0],
        uv01: uv[1],
        uv10: uv[2],
        uv11: uv[3],
        width: sprite_get_width(sprNote2),
        height: sprite_get_height(sprNote2),
        type: 1, // SEG-3
        data: [22, 22],
        paddingLR: _note_get_lrpadding_total(0),
        paddingTop: 0,
        paddingBottom: 0
    });

    // Add sprChain.
    var uv = sprite_get_uvs(sprChain, 0);
    dyc_add_sprite_data({
        name: "sprChain",
        uv00: uv[0],
        uv01: uv[1],
        uv10: uv[2],
        uv11: uv[3],
        width: sprite_get_width(sprChain),
        height: sprite_get_height(sprChain),
        type: 2, // SEG-5
        data: [21, 78, 19],
        paddingLR: _note_get_lrpadding_total(1),
        paddingTop: 0,
        paddingBottom: 0
    });

    // Add sprHoldEdge
    var uv = sprite_get_uvs(sprHoldEdge, 0);
    dyc_add_sprite_data({
        name: "sprHoldEdge",
        uv00: uv[0],
        uv01: uv[1],
        uv10: uv[2],
        uv11: uv[3],
        width: sprite_get_width(sprHoldEdge),
        height: sprite_get_height(sprHoldEdge),
        type: 3, // Slice-9
        data: [32, 33, 53, 52],
        paddingLR: _note_get_lrpadding_total(2),
        paddingTop: 13,
        paddingBottom: 26,
    });

    // Add sprHold
    var uv = sprite_get_uvs(sprHold, 0);
    dyc_add_sprite_data({
        name: "sprHold",
        uv00: uv[0],
        uv01: uv[1],
        uv10: uv[2],
        uv11: uv[3],
        width: sprite_get_width(sprHold),
        height: sprite_get_height(sprHold),
        type: 4, // Repeat-vert
        data: [],
        paddingLR: 0,
        paddingTop: 0,
        paddingBottom: 0,
    });

    // Add sprHoldGrey
    var uv = sprite_get_uvs(sprHoldGrey, 0);
    dyc_add_sprite_data({
        name: "sprHoldGrey",
        uv00: uv[0],
        uv01: uv[1],
        uv10: uv[2],
        uv11: uv[3],
        width: sprite_get_width(sprHoldGrey),
        height: sprite_get_height(sprHoldGrey),
        type: 0, // Normal
        data: [],
        paddingLR: 0,
        paddingTop: 0,
        paddingBottom: 0,
    });
}

/// @param {Id.Buffer} buffer 
/// @param {Struct.sNoteProp} noteProp 
function dyc_note_serialization(buffer, noteProp) {
    noteProp.bitwrite(buffer);
}

function dyc_note_deserialization(buffer) {
    return new sNoteProp().bitread(buffer);
}

/// @param {Struct.sNoteProp} noteProp 
function dyc_update_note(noteProp, record = false, recursive = false) {
    // Check if noteProp is sNoteProp type.
    if(!variable_struct_exists(noteProp, "copy"))
        noteProp = new sNoteProp(noteProp);

    var noteID = noteProp.noteID;
    if(!dyc_note_exists(noteID)) {
        show_debug_message("!Warning: dyc_update_note called with non-existing noteID: " + noteID);
        return;
    }

    var origProp = -1;
    if(record) origProp = dyc_get_note(noteID);

    static buffer = buffer_create(1024, buffer_grow, 1);
    dyc_note_serialization(buffer, noteProp);
    var result = DyCore_modify_note_bitwise(noteID, buffer_get_address(buffer));

    if(result == 0 && record) {
        operation_step_add(OPERATION_TYPE.MOVE, origProp, noteProp);
    }

    if(!recursive) {
        if(noteProp.noteType == NOTE_TYPE.HOLD) {
            // Sync the subnote's data.
            var subNote = dyc_get_note(noteProp.subNoteID);
            subNote.position = noteProp.position;
            subNote.side = noteProp.side;
            subNote.width = noteProp.width;
            subNote.beginTime = noteProp.time;
            subNote.time = noteProp.time + noteProp.lastTime;
            
            dyc_update_note(subNote, false, true);
        }
        else if(noteProp.noteType == NOTE_TYPE.SUB) {
            // Sync the father's data.
            var parentNote = dyc_get_note(noteProp.subNoteID);
            parentNote.lastTime = noteProp.time - parentNote.time;
    
            dyc_update_note(parentNote, false, true);
        }
    }

    if(result < 0)
        throw "Unknown error in dyc_update_note.";
}

/// @param {Struct.sNoteProp} noteProp 
function dyc_create_note(noteProp, record = false) {
    static buffer = buffer_create(1024, buffer_grow, 1);
    noteProp.bitwrite(buffer);
    var result = DyCore_insert_note(buffer_get_address(buffer));
    if(result < 0) {
        throw "Unknown error in dyc_create_note.";
    }
    if(record) {
        operation_step_add(OPERATION_TYPE.ADD, noteProp.copy(), -1);
    }
}

/// @description Get note by noteID.
/// @param {String} noteID The note ID.
/// @returns {Struct.sNoteProp} The note struct or undefined if not found.
function dyc_get_note(noteID) {
    static propBuffer = buffer_create(1024, buffer_grow, 1);
    var result = DyCore_get_note(noteID, buffer_get_address(propBuffer));
    if (result == 0) {
        return dyc_note_deserialization(propBuffer);
    }
    if (result == -1) {
        show_debug_message("!Warning: dyc_get_note failed to find noteID: " + noteID);
    }
    return undefined;
}

/// @description Get note at notes array's index.
/// @param {Real} index The index of the note in the notes array.
/// @returns {Struct.sNoteProp} The note struct or undefined if not found.
function dyc_get_note_at_index(index) {
    DyCore_sort_notes();
    static propBuffer = buffer_create(1024, buffer_grow, 1);
    var result = DyCore_get_note_at_index(index, buffer_get_address(propBuffer));
    if (result == 0) {
        return dyc_note_deserialization(propBuffer);
    }
    return undefined;
}

/// @description Get note at notes array's index directly.
/// This will bypass the OutOfOrder flag.
/// @param {Real} index The index of the note in the notes array.
/// @returns {Struct.sNoteProp} The note struct or undefined if not found.
function dyc_get_note_at_index_direct(index) {
    static propBuffer = buffer_create(1024, buffer_grow, 1);
    var result = DyCore_get_note_at_index_direct(index, buffer_get_address(propBuffer));
    if (result == 0) {
        return dyc_note_deserialization(propBuffer);
    }
    return undefined;
}

/// @description Get note time at notes array's index.
/// @param {Real} index The index of the note in the notes array.
/// @returns {Real} The time of the note or undefined if not found.
function dyc_get_note_time_at_index(index) {
    DyCore_sort_notes();
    var result = DyCore_get_note_time_at_index(index);
    if (result != -999999) {
        return result;
    }
    return undefined;
}

/// @description Get note time at notes array's index.
/// @param {Real} index The index of the note in the notes array.
/// @returns {Real} The time of the note or undefined if not found.
function dyc_get_note_id_at_index(index) {
    DyCore_sort_notes();
    var result = DyCore_get_note_id_at_index(index);
    if (result != "") {
        return result;
    }
    return undefined;
}

/// @description Get the index of the note in the notes array by noteID.
/// @param {String} noteID The note ID.
/// @returns {Real} The index of the note in the notes array or -1 if not found.
function dyc_get_note_array_index(noteID) {
    return DyCore_get_note_array_index(noteID);
}

function dyc_note_exists(noteID) {
    return DyCore_note_exists(noteID) >= 0;
}

function dyc_get_note_count() {
    return DyCore_get_note_count();
}

function dyc_chart_import_xml(filePath, importInfo, importTiming) {
    var _result = DyCore_chart_import_xml(filePath, importInfo, importTiming);
    if (_result == 1) {
        announcement_error("dym_import_failed");
        return -1;
    } else if (_result == 2) {
        announcement_warning("bad_dym_chart_format", 10000);
        return -1;
    }

    show_debug_message("Load XML file completed.");
    return 0;
}

function dyc_chart_import_dy(filePath, importInfo, importTiming) {
    var _result = DyCore_chart_import_dy(filePath, importInfo, importTiming);
    if (_result == 1) {
        announcement_error("dym_import_failed");
        return -1;
    } else if (_result == 2) {
        announcement_warning("bad_dym_chart_format", 10000);
        return -1;
    }

    show_debug_message("Load DY file completed.");
    return 0;
}

function dyc_chart_import_dy_get_remix() {
    try {
        return json_parse(DyCore_chart_import_dy_get_remix());
    } catch (e) {
        show_debug_message("Error parsing DY remix: " + string(e));
        return undefined;
    }
}

/// @param {Real} fixError The fix-error offset.
function dyc_chart_export_xml(filePath, isDym, fixError) {
    return DyCore_chart_export_xml(filePath, isDym, fixError);
}

function dyc_project_load(filePath) {
    return DyCore_project_load(filePath);
}

function dyc_chart_import_dyn(filePath, importInfo, importTiming) {
    return DyCore_chart_import_dyn(filePath, importInfo, importTiming);
}

/// @returns {Any} The chart metadata struct.
function dyc_chart_get_metadata() {
    static _metadata = {};
    static _lastModified = -1;
    try {
        var _modified = DyCore_get_chart_metadata_last_modified_time();
        if(_modified != _lastModified) {
            _lastModified = _modified;
            _metadata = json_parse(DyCore_get_chart_metadata());
        }
        return _metadata;
    } catch (e) {
        show_debug_message("Error parsing chart metadata: " + string(e));
        return undefined;
    }
}

function dyc_chart_set_metadata(metadata) {
    try {
        var _metadataStr = json_stringify(metadata);
        return DyCore_set_chart_metadata(_metadataStr);
    } catch (e) {
        show_debug_message("Error setting chart metadata: " + string(e));
        return false;
    }
}

function dyc_chart_set_title(title) {
    try {
        var _metadata = dyc_chart_get_metadata();
        _metadata.title = title;
        return dyc_chart_set_metadata(_metadata);
    } catch (e) {
        show_debug_message("Error setting chart title: " + string(e));
        return false;
    }
}

function dyc_chart_set_difficulty(difficulty) {
    try {
        var _metadata = dyc_chart_get_metadata();
        _metadata.difficulty = difficulty;
        return dyc_chart_set_metadata(_metadata);
    } catch (e) {
        show_debug_message("Error setting chart difficulty: " + string(e));
        return false;
    }
}

function dyc_chart_set_sidetype(sideType) {
    try {
        var _metadata = dyc_chart_get_metadata();
        _metadata.sideType = sideType;
        return dyc_chart_set_metadata(_metadata);
    } catch (e) {
        show_debug_message("Error setting chart side type: " + string(e));
        return false;
    }
}

function dyc_chart_get_title() {
    try {
        var _metadata = dyc_chart_get_metadata();
        return _metadata.title;
    } catch (e) {
        show_debug_message("Error getting chart title: " + string(e));
        return undefined;
    }
}

function dyc_chart_get_difficulty() {
    try {
        var _metadata = dyc_chart_get_metadata();
        return _metadata.difficulty;
    } catch (e) {
        show_debug_message("Error getting chart difficulty: " + string(e));
        return undefined;
    }
}

function dyc_chart_get_sidetype() {
    try {
        var _metadata = dyc_chart_get_metadata();
        return _metadata.sideType;
    } catch (e) {
        show_debug_message("Error getting chart side type: " + string(e));
        return undefined;
    }
}

function dyc_chart_get_path() {
    try {
        var _path = DyCore_get_chart_path();
        return json_parse(_path);
    } catch (e) {
        show_debug_message("Error parsing chart path: " + string(e));
        return undefined;
    }
}

function dyc_project_get_metadata() {
    try {
        var _metadata = DyCore_get_project_metadata();
        return json_parse(_metadata);
    } catch (e) {
        show_debug_message("Error parsing project metadata: " + string(e));
        return undefined;
    }
}

/// @returns {Array<Struct.sTimingPoint>} 
function dyc_get_timingpoints() {
    static _timingpoints = [];
    static _lastModifiedTime = -1;
    var _lastModified = DyCore_get_timing_points_last_modified_time();
    if (_lastModified != _lastModifiedTime) {
        _lastModifiedTime = _lastModified;
        try {
            _timingpoints = json_parse(DyCore_get_timing_array_string());
        } catch (e) {
            show_debug_message("Error parsing timing points: " + string(e));
        }
    }
    return _timingpoints;
}

function dyc_get_timingpoints_count() {
    return DyCore_get_timing_points_count();
}

/// @param {Struct.sTimingPoint} timingPoint 
function dyc_insert_timingpoint(timingPoint) {
    try {
        return DyCore_insert_timing_point(json_stringify(timingPoint));
    } catch (e) {
        show_debug_message("Error inserting timing point: " + string(e));
        return -1;
    }
}

function dyc_timingpoints_sort() {
    return DyCore_timing_points_sort();
}

function dyc_timingpoints_reset() {
    return DyCore_timing_points_reset();
}

function dyc_timingpoints_delete_at(time) {
    return DyCore_delete_timing_point_at_time(time);
}

function dyc_timingpoints_change(time, timingPoint) {
    return DyCore_timing_points_change(time, json_stringify(timingPoint));
}

function dyc_timingpoints_add_offset(offset) {
    return DyCore_timing_points_add_offset(offset);
}

function dyc_project_get_version() {
    return DyCore_get_project_version();
}

function dyc_get_active_notes(nowTime, noteSpeed) {
    static _activeNotes = [];
    static lastUpdateTime = -1;
    static buffer = buffer_create(1024 * 1024, buffer_fixed, 1);

    if(lastUpdateTime != global.frameCurrentTime) {
        lastUpdateTime = global.frameCurrentTime;
        delete _activeNotes;
        _activeNotes = [];
    }
    else return _activeNotes;

    var boundSize = DyCore_cac_active_notes(nowTime, noteSpeed);
    if(boundSize > buffer_get_size(buffer)) {
        buffer_resize(buffer, boundSize);
        buffer_set_used_size(buffer, boundSize);
    }

    DyCore_get_active_notes(buffer_get_address(buffer));
    buffer_seek(buffer, buffer_seek_start, 0);
    var count = buffer_read(buffer, buffer_u32);

    array_resize(_activeNotes, count);
    for(var i = 0; i < count; i++) {
        _activeNotes[i] = buffer_read(buffer, buffer_string);
    }

    return _activeNotes;
}

function dyc_update_active_notes() {
    DyCore_cac_active_notes(objMain.nowTime, objMain.playbackSpeed);
}

/// @description This function will not update active notes.
function dyc_get_lasting_holds() {
    static _lastingHolds = [];
    static lastUpdateTime = -1;
    static buffer = buffer_create(1024 * 1024, buffer_fixed, 1);

    if(lastUpdateTime != global.frameCurrentTime) {
        lastUpdateTime = global.frameCurrentTime;
        delete _lastingHolds;
        _lastingHolds = [];
    }
    else return _lastingHolds;

    var boundSize = DyCore_get_active_notes_bound();
    if(boundSize > buffer_get_size(buffer)) {
        buffer_resize(buffer, boundSize);
        buffer_set_used_size(buffer, boundSize);
    }

    DyCore_get_lasting_holds(buffer_get_address(buffer));
    buffer_seek(buffer, buffer_seek_start, 0);
    var count = buffer_read(buffer, buffer_u32);

    array_resize(_lastingHolds, count);
    for(var i = 0; i < count; i++) {
        _lastingHolds[i] = buffer_read(buffer, buffer_string);
    }

    return _lastingHolds;
}

function dyc_add_sprite_data(data) {
    try {
        return DyCore_add_sprite_data(json_stringify(data));
    } catch (e) {
        show_debug_message("Error adding sprite data: " + string(e));
        return -1;
    }
}

function dyc_get_note_index_upperbound(time) {
    return DyCore_get_note_index_upper_bound(time);
}

function dyc_get_open_filename(filter, filename, dir, title) {
    var result = DyCore_get_open_filename(filter, filename, dir, title);

    if(result == "terminated") return "";
    else if(result == "") {
        announcement_error("Opening file save dialogue failed.");
        return result;
    }

    return result;
}

function dyc_get_save_filename(filter, filename, dir, title) {
    var result = DyCore_get_save_filename(filter, filename, dir, title);

    if(result == "terminated") return "";
    else if(result == "") {
        announcement_error("Opening file save dialogue failed.");
        return result;
    }

    return result;
}

function dyc_show_question(text) {
    var result = DyCore_show_question(text);
    return result > 0 ? true : false;
}

function dyc_get_string(prompt, default_text) {
    var result = DyCore_get_string(prompt, default_text);
    if(result == "<%$><.3>TERMINATED<!#><##>") return "";
    else if(result == "") {
        announcement_error("Opening input dialog failed.");
        return result;
    }
    return result;
}

function dyc_random_range(min, max) {
    return DyCore_random_range(min, max);
}

function dyc_random(r) {
    return dyc_random_range(0, r);
}

function dyc_irandom_range(min, max) {
    return DyCore_irandom_range(min, max);
}

function dyc_irandom(r) {
    return dyc_irandom_range(0, r);
}