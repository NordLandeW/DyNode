/// DyCore Interface.

enum DYCORE_ASYNC_EVENT_TYPE { PROJECT_SAVING, GENERAL_ERROR };
function DyCoreManager() constructor {
    // DyCore Step function.
    static step = function() {
        // Manage async events.
        if(DyCore_has_async_event()) {
            var _async_event = DyCore_get_async_event();
            if(_async_event == "") {
                show_debug_message("!Warning: DyCore async event is empty.");
                announcement_error("DyCore async events prased failed.");
                return;
            }
            else {
                do_async_events(json_parse(_async_event));
            }
        }
    }

    static do_async_events = function(event) {
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
                    announcement_error(i18n_get("anno_project_save_failed", event[$ "message"]));
                }
                break;
            case DYCORE_ASYNC_EVENT_TYPE.GENERAL_ERROR:
                announcement_error(i18n_get("anno_dycore_error", event[$ "message"]));
                break;
            default:
                show_debug_message("!Warning: Unknown dycore async event type.");
                break;
        }
    }
}

function dyc_note_serialization(buffer, noteProp) {
    buffer_seek(buffer, buffer_seek_start, 0);
    buffer_write(buffer, buffer_u32, noteProp[$ "side"]);
    buffer_write(buffer, buffer_u32, noteProp[$ "noteType"]);
    buffer_write(buffer, buffer_f64, noteProp[$ "time"]);
    buffer_write(buffer, buffer_f64, noteProp[$ "width"]);
    buffer_write(buffer, buffer_f64, noteProp[$ "position"]);
    buffer_write(buffer, buffer_f64, noteProp[$ "lastTime"]);
    buffer_write(buffer, buffer_f64, noteProp[$ "beginTime"]);
    buffer_write(buffer, buffer_string, noteProp[$ "noteID"]);
    buffer_write(buffer, buffer_string, noteProp[$ "subNoteID"]);
}

function dyc_note_deserialization(buffer) {
    var noteProp = {};
    buffer_seek(buffer, buffer_seek_start, 0);
    noteProp[$ "side"] = buffer_read(buffer, buffer_u32);
    noteProp[$ "noteType"] = buffer_read(buffer, buffer_u32);
    noteProp[$ "time"] = buffer_read(buffer, buffer_f64);
    noteProp[$ "width"] = buffer_read(buffer, buffer_f64);
    noteProp[$ "position"] = buffer_read(buffer, buffer_f64);
    noteProp[$ "lastTime"] = buffer_read(buffer, buffer_f64);
    noteProp[$ "beginTime"] = buffer_read(buffer, buffer_f64);
    noteProp[$ "noteID"] = buffer_read(buffer, buffer_string);
    noteProp[$ "subNoteID"] = buffer_read(buffer, buffer_string);
    return noteProp;
}

function dyc_update_note(noteProp) {
    var noteID = noteProp[$ "noteID"];
    static buffer = -1;
    if(buffer < 0) {
        buffer = buffer_create(1024, buffer_grow, 1);
    }

    dyc_note_serialization(buffer, noteProp);
    DyCore_modify_note_bitwise(noteID, buffer_get_address(buffer));
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