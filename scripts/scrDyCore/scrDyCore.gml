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
                if(event[$ "status"] == 0) {
                    if(objManager.autosaving) {
                        if(editor_get_editmode() != 5)  // Ignore announcement when edit mode is playback.
                            announcement_play("autosave_complete");
                        objManager.autosaving = false;
                    }
                    else
                        announcement_play("anno_project_save_complete");
                }
                else {
                    announcement_error(i18n_get("anno_project_save_failed", event[$ "content"]));
                }
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

/// @param {Id.Buffer} buffer 
/// @param {Struct.sNoteProp} noteProp 
function dyc_note_serialization(buffer, noteProp) {
    noteProp.bitwrite(buffer);
}

function dyc_note_deserialization(buffer) {
    return new sNoteProp().bitread(buffer);
}

/// @param {Struct.sNoteProp} noteProp 
function dyc_update_note(noteProp, record = false) {
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

    if(noteProp.noteType == NOTE_TYPE.HOLD) {
        // Sync the subnote's data.
        var subNote = dyc_get_note(noteProp.subNoteID);
        subNote.position = noteProp.position;
        subNote.side = noteProp.side;
        subNote.width = noteProp.width;
        subNote.beginTime = noteProp.time;
        
        dyc_update_note(subNote, false);
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

/// @description To check if the buffer is compressed.
function dyc_is_compressed_buffer(buffer) {
	return DyCore_is_compressed(buffer_get_address(buffer), buffer_get_size(buffer)) >= 0;
}

/// @description Convert compressed/uncompressed buffer to project struct.
/// @param {Id.Buffer} buffer The given buffer.
/// @returns {Struct} The final struct.
function dyc_read_project_buffer(buffer) {
	var _json = "";
	if(!dyc_is_compressed_buffer(buffer)) {
		// Fallback to text format.
		buffer_seek(buffer, buffer_seek_start, 0);
		_json = buffer_read(buffer, buffer_text);
	}
	else {
		_json = DyCore_decompress_string(buffer_get_address(buffer), buffer_get_size(buffer));
		if(_json == "failed")
			throw "Decompress failed.";
	}

	var _str = json_parse(_json);

	if(!is_struct(_str))
		throw "Parse failed.";
	return _str;
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

function dyc_project_import_xml(filePath, importInfo, importTiming) {
    var _result = DyCore_project_import_xml(filePath, importInfo, importTiming);
    if (_result == 1) {
        announcement_error("dym_import_failed");
        return;
    } else if (_result == 2) {
        announcement_warning("bad_xml_chart_format", 10000);
        return;
    }

    try {
        if(importTiming) {
            var _timingArray = json_parse(DyCore_get_timing_array_string());
            objEditor.timingPoints = SnapDeepCopy(
                array_concat(objEditor.timingPoints, _timingArray));
            timing_point_sort();
        }
        if(importInfo) {
            /// @type {Any}
            var _info = dyc_chart_get_metadata();
            with(objMain) {
                chartTitle = _info.title;
                chartDifficulty = _info.difficulty;
                chartSideType = _info.sideType;
            }
        }
    } catch (e) {
        announcement_error("dym_import_info_timing_failed");
        return;
    }

    show_debug_message("Load XML file completed.");
}

function dyc_project_load(filePath) {
    return DyCore_project_load(filePath);
}

function dyc_chart_get_metadata() {
    try {
        var _metadata = DyCore_get_chart_metadata();
        return json_parse(_metadata);
    } catch (e) {
        show_debug_message("Error parsing chart metadata: " + string(e));
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
    try {
        var _timingpoints = DyCore_get_timing_array_string();
        return json_parse(_timingpoints);
    } catch (e) {
        show_debug_message("Error parsing timing points: " + string(e));
        return undefined;
    }
}

function dyc_project_get_version() {
    try {
        return DyCore_get_project_version();
    } catch (e) {
        show_debug_message("Error getting project version: " + string(e));
        return "unknown";
    }
}