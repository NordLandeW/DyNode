/// @description Check for latest update

if (async_load[? "id"] == _update_get_event_handle) {
	if (async_load[? "status"] == 0) {
		try {
			var _result = json_parse(async_load[? "result"]);
		
			if(is_struct(_result)) {
				show_debug_message("Got update information.");
				if(variable_struct_exists(_result, "message") && _result.message == "Not Found") {
					announcement_warning("autoupdate_warn");
				}
				else if(DEBUG_MODE || version_cmp(_result.tag_name, VERSION)>0 && _result.tag_name != global.lastCheckedVersion) {
					_update_version = _result.tag_name;
					_update_url = _result.html_url;
					_update_filename = _result.assets[0].name;
					_update_github_url = _result.assets[0].browser_download_url;

					show_debug_message("Filename: "+_update_filename+"\nURL: "+_update_github_url);
					announcement_play("[scale, 1.5]"+i18n_get("autoupdate_found_1")+"[#aed581]" + _result.tag_name + 
						"[/c][scale,1.2]\n[region,update_2][cycle,0,30]" + i18n_get("autoupdate_found_3") + "[/cycle][/region]\n" +
						"[/c][scale,1]\n[region,update][cycle,130,150]" + i18n_get("autoupdate_found_2") + "[/cycle][/region]" +
						"[/c][scale,0.8]\n[region,update_skip][c_white]" + i18n_get("autoupdate_found_4") + "[/cycle][/region]    " +
						"[/c][scale,0.8][region,update_off][c_ltgray]" + i18n_get("autoupdate_found_5") + "[/cycle][/region]\n\n" +
						"[c_ltgrey][scale, 1]"+format_markdown(_result.body)+"\n", 5000);
				}
			}
		} catch (e) {
			announcement_warning("autoupdate_err")
		}
	}
	else if (async_load[? "status"] < 0) {
		announcement_warning("autoupdate_err")
	}
}

if (async_load[? "id"] == _update_download_event_handle) {
	if(_update_status == UPDATE_STATUS.FAILED)
		return;
	var status = async_load[? "status"];
	var _err = false;
	if (status == 1) {
		if(_update_status != UPDATE_STATUS.DOWNLOADING) {
			announcement_play("autoupdate_process_1");
			_update_status = UPDATE_STATUS.DOWNLOADING;
		}
		_update_size = async_load[? "contentLength"];
		_update_received = async_load[? "sizeDownloaded"];
	}
	else if(status == 0) {
		var _hstatus = string(async_load[? "http_status"]);
		show_debug_message("Get https status:"+_hstatus);
		if(string_char_at(_hstatus, 1) != "2")
			_err = true;
		else {
			var _bfile = file_bin_open(UPDATE_TARGET_FILE, 0);
			var _filesize = file_bin_size(_bfile);
			// Check the file integrity to prevent corrupted file.
			if(_filesize < 100)
				_err = true;
			else
				start_update_unzip();
			file_bin_close(_bfile);
		}
	}
	else if(status < 0) {
		_err = true;
	}

	if(_err) {
		if(_update_status == UPDATE_STATUS.CHECKING_I)
			fallback_update();
		else {
			announcement_error("autoupdate_process_err_1");
			_update_status = UPDATE_STATUS.FAILED;
		}
	}
}