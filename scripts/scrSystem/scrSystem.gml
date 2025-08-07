
#region Macros

#macro MAX_STAT_TYPE 5
#macro MAX_SHADOW_COUNT 300
#macro MAX_PARTICLE_COUNT 500

#endregion

#region MAP FUNCTIONS

function map_close() {
	with(objMain) {
		safe_video_free();
		kawase_destroy(kawaseArr);
		
		note_delete_all();
		instance_destroy(objScoreBoard);
		instance_destroy(objPerfectIndc);
		instance_destroy(objEditor);
		instance_destroy(objShadow);
		instance_destroy(objShadowMIX);
		instance_destroy(objTopBar);
		
		time_source_destroy(timesourceResumeDelay);
		time_source_destroy(timesourceSyncVideo);
		part_emitter_destroy_all(partSysNote);
		part_system_destroy(partSysNote);
		part_type_destroy(partTypeNoteDL);
		part_type_destroy(partTypeNoteDR);
		part_type_destroy(partTypeHold);
		
		if(bgImageSpr != -1)
		    sprite_delete(bgImageSpr);
		
		if(!is_undefined(music)) {
			FMODGMS_Snd_Unload(music);
			FMODGMS_Chan_RemoveChannel(channel);
		}

		DyCore_clear_notes();
		global.noteIDMan.clear();
		global.activationMan.clear();
	}
	
	instance_destroy(objMain);

	call_later(1, time_source_units_seconds, function() { gc_collect(); });
	call_later(2, time_source_units_seconds, function() { gc_collect(); });
	call_later(3, time_source_units_seconds, function() { gc_collect(); });
}

function map_reset() {
	map_close();
	instance_create_depth(0, 0, 0, objMain);
}

function map_load(_file = "") {

	if(is_struct(_file)) {
		map_load_struct(_file);
		
		return;
	}
	var _direct = _file != "";
	if(_file == "")
	    _file = get_open_filename_ext(i18n_get("fileformat_chart") + " (*.xml;*.dyn;*dy;*.osu)|*.xml;*.dyn;*dy;*.osu", "", 
	        program_directory, "Load Dynamix Chart File 加载谱面文件");
        
    if(_file == "") return;
    
    if(!file_exists(_file)) {
        announcement_error(i18n_get("anno_chart_not_exists"));
        return;
    }
    
    var _confirm = _direct? true:show_question_i18n("box_q_import_confirm");
    if(!_confirm)
    	return;
    
    var _clear = _direct? true:show_question_i18n("box_q_import_clear");
    if(_clear)
    	note_delete_all();
    
    var _timing_reset = _direct? true:show_question_i18n("box_q_import_timing_reset");
    if(_timing_reset)
    	timing_point_reset();
    
	try {
		switch filename_ext(_file) {
			case ".xml":
			case ".dy":
				map_import_dym(_file, _direct);
				break;
			case ".osu":
				map_import_osu(_file);
				break;
			case ".dyn":
				map_import_dyn(_file);
				break;
		}
	} catch (e) {
		announcement_error("谱面解析错误。请确认谱面的格式受 DyNode 支持。\n错误信息：[scale,0.7]"+string(e));
		return;
	}
    
    // Notes information init & Remove extra sub notes.
    note_sort_all(true);
    
    announcement_play("anno_import_chart_complete");
}

function map_import_dym(_file, _direct = false) {
    var _buf = buffer_load(_file);
    var _str;
	var _dy_format = false;
	var _import_info, _import_tp;
	if(!_direct) {
		_import_info = show_question_i18n("box_q_import_info");
		_import_tp = show_question_i18n("box_q_import_bpm");
	} else {
		_import_info = true;
		_import_tp = true;
	}

	if(filename_ext(_file) == ".xml") {
		dyc_chart_import_xml(_file, _import_info, _import_tp);
		return;
	}
	else {
		var result = dyc_chart_import_dy(_file, _import_info, _import_tp);

		// Read background & image from .dy format.
		if(_import_info && result == 0) {
			var _remix = dyc_chart_import_dy_get_remix();
			var _music = convert_mime_base64_to_file("audio", _remix.music, objMain.chartTitle);
			if(_music != "")
				music_load(_music);
			var _image = convert_mime_base64_to_file("image", _remix.bg, objMain.chartTitle);
			if(_image != "")
				image_load(_image);
			var _video = convert_mime_base64_to_file("video", _remix.bg, objMain.chartTitle);
			if(_video != "")
				video_load(_video);
		}
	}
    
}

function map_import_osu(_file = "") {
    if(_file == "")
	    _file = get_open_filename_ext("OSU Files (*.osu)|*.osu", "", 
	        program_directory, "Load osu! Chart File 加载 osu! 谱面文件");
        
    if(_file == "") return;
    
    var _import_hitobj = show_question_i18n(i18n_get("box_q_osu_import_objects"));
    var _delay_time = 0;
    
    var _buf = buffer_load(_file);
    var _grid = SnapBufferReadCSV(_buf, 0);
    buffer_delete(_buf);
	
    var _type = "";
    var _h = array_length(_grid);
    var _mode = 0;				// Osu Game Mode
    
    for(var i=0; i<_h; i++) {
        if(string_last_pos("[", _grid[i][0]) != 0) {
        	_type = _grid[i][0];
        }
            
        else if(_grid[i][0] != ""){
            switch _type {
            	case "[General]":
            		if(string_last_pos("Mode", _grid[i][0]) != 0)
            			_mode = real(string_digits(_grid[i][0]));
            		break;
                case "[TimingPoints]":
					if(array_length(_grid[i]) < 3) break;
                    var _time = real(_grid[i][0]) + _delay_time;
                    var _mspb = string_letters(_grid[i][1]) != ""?-1:real(_grid[i][1]);
                    var _meter = real(_grid[i][2]);
                    if(_mspb > 0)
                        timing_point_add(_time, _mspb, _meter);
                    break;
                case "[HitObjects]":
                	if(_import_hitobj) {
						if(array_length(_grid[i]) < 6) break;
                		var _ntime = real(_grid[i][2]) + _delay_time;
                		var _ntype = real(_grid[i][3]);
                		if(_ntime > 0) {
	                		switch _mode {
	                			case 0:
	                			case 1:
	                			case 2:
	                				var _x = real(_grid[i][0]);
	                				var _y = real(_grid[i][1]);
									build_note({
										noteType: NOTE_TYPE.NORMAL,
										time: _ntime,
										position: _x / 512 * 5,
										width: 1.0,
										side: NOTE_SIDE.FRONT,
									});
	                				break;
	                			case 3: // Mania Mode
	                				var _x = real(_grid[i][0]);
	                				if(_ntype & 128) { // If is a Mania Hold
	                					var _subtim = real(string_copy(_grid[i][5], 1, string_pos(":", _grid[i][5])-1)) + _delay_time;
										build_note({
											noteType: NOTE_TYPE.HOLD,
											position: _x / 512 * 5,
											width: 1.0,
											lastTime: _subtim,
											side: NOTE_SIDE.FRONT,
										})
	                				} 
	                				else
	                					build_note({
	                						noteType: NOTE_TYPE.NORMAL,
	                						time: _ntime,
	                						position: _x / 512 * 5,
	                						width: 1.0,
	                						side: NOTE_SIDE.FRONT,
	                					});
	                				break;
	                		}
                		}
                	}
                	break;
				default:
					break;
            }
        }
    }
    
    timing_point_sort();
    note_sort_all(true);
    
    announcement_play("anno_import_info_complete", 1000);
}

function map_import_dyn(_file) {
	var _import_info = show_question_i18n("box_q_import_info");
    var _import_tp = show_question_i18n("box_q_import_bpm");

	dyc_chart_import_dyn(_file, _import_info, _import_tp);
}

function map_set_title() {
	var _title = get_string_i18n(i18n_get("box_set_chart_title") + ": ", map_get_title());
	
	if(_title == "") return;
	
	var _scribble_tag = string_last_pos("[_scribble]", _title) != 0
	
	if (!_scribble_tag)
		_title = string_replace_all(_title, "[", "[[")
	
	objMain.chartTitle = _title;
	dyc_chart_set_title(_title);
}

function music_load(_file = "") {
    if(_file == "")
	    _file = get_open_filename_ext("Music Files (*.mp3;*.flac;*.wav;*.ogg;*.aiff;*.mid)|*.mp3;*.flac;*.wav;*.ogg;*.aiff;*.mid", "", 
	        program_directory, "Load Music File 加载音乐文件");
        
    if(_file == "") return;
    
    if(!file_exists(_file)) {
        announcement_error(i18n_get("anno_music_not_exists") + _file);
        return;
    }
    
    with(objMain) {
        if(!is_undefined(music))
            FMODGMS_Snd_Unload(music);
        
        chartMusicFile = _file;
        music = FMODGMS_Snd_LoadSound_Ext2(_file, 0x00004200);
        // music = FMODGMS_Snd_LoadSound(_file);
        if(music < 0) {
        	announcement_error(i18n_get("anno_music_load_err")+FMODGMS_Util_GetErrorMessage());
        	music = undefined;
        	return;
        }
        FMODGMS_Snd_PlaySound(music, channel);
        if(!nowPlaying) FMODGMS_Chan_PauseChannel(channel);
        else {
            nowTime = 0;
        }
        sampleRate = FMODGMS_Chan_Get_Frequency(channel);
        musicLength = FMODGMS_Snd_Get_Length(music);
        usingMP3 = string_lower(filename_ext(_file)) == ".mp3";
        if(usingMP3)
        	show_debug_message_safe("The music file is using the mp3 format")
        
    }
    objManager.musicPath = _file;
    show_debug_message_safe("Load sucessfully.");
    
    announcement_play("anno_music_load_complete", 1000);
}

function background_load(_file = "") {
	if(_file == "")
	    _file = get_open_filename_ext("Background Files (*.jpg;*.jpeg;*.png;*.mp4;*.avi;*.mkv)|*.jpg;*.jpeg;*.png;*.mp4;*.avi;*.mkv|JPG Files (*.jpg)|*.jpg|PNG Files (*.png)|*.png", "",
	        program_directory, "Load Background File 加载背景文件");
        
    if(_file == "") return;

	var _ext = filename_ext(_file);
	_ext = string_lower(_ext);
    
    switch(_ext) {
    	case ".jpg":
    	case ".jpeg":
    	case ".png":
    		image_load(_file);
    		break;
    	case ".mp4":
    	case ".avi":
    	case ".mkv":
    		video_load(_file);
    		break;
		default:
			announcement_error("Unsupported background file format: " + filename_ext(_file));
			break;
    }
}

function background_reset() {
	with(objManager) {
		backgroundPath = "";
		videoPath = "";
		with(objMain) {
			if(sprite_exists(bgImageSpr))
				sprite_delete(bgImageSpr);
			bgImageSpr = -1;
		}
		safe_video_free();
		announcement_play("anno_background_reset");
	}
}

function video_load(_file, _safe = true) {
	if(!file_exists(_file)) {
	        announcement_error("video_playback_file_not_exists"+_file);
        return;
    }
    if(_safe)
		safe_video_free();
	else
		video_close();
    
    if(variable_global_exists("__tmp_handlevo") && global.__tmp_handlevo != undefined)
    	call_cancel(global.__tmp_handlevo);
    global.__tmp_handlevo_time = 0;
    global.__tmp_handlevo = call_later(1, time_source_units_frames, function () {
    	if(delta_time <= 1000000)
    		global.__tmp_handlevo_time += delta_time / 1000;
    	if(video_get_status() == video_status_closed && global.__tmp_handlevo_time < 3000) {
    		video_open(objManager.videoPath);
			video_enable_loop(true);	// Prevent weird bugs
			video_set_volume(0);
    	}
    	else if(video_get_status() != video_status_preparing) {
    		if(global.__tmp_handlevo_time >= 10000)
    			announcement_error("video_playback_open_timeout"); 
    		call_cancel(global.__tmp_handlevo);
    		global.__tmp_handlevo = undefined;
    	}
    }, true);
	
	objManager.videoPath = _file;
}

function image_load(_file) {
	if(!file_exists(_file)) {
        announcement_error(i18n_get("anno_graph_not_exists")+_file);
        return;
    }
    
    var _spr = sprite_add(_file, 1, 0, 0, 0, 0);

    if(_spr < 0) {
        announcement_error("anno_graph_load_err");
        return;
    }
    
    with(objMain) {
        if(bgImageSpr != -1)
            sprite_delete(bgImageSpr);
        
        bgImageSpr = _spr;
    }
    objManager.backgroundPath = _file;
}

function map_export_xml(_export_to_dym) {
	if(dyc_get_timingpoints_count() == 0) {
		announcement_error("export_timing_error");
		return;
	}
	
    var _file = "";
    var _mapid = (_export_to_dym?"_dym_":"_map_") + map_get_alt_title() + "_" + difficulty_num_to_char(objMain.chartDifficulty);
	var _default_file_name = $"{_mapid}-{current_year}-{current_month}-{current_day}-{current_hour}-{current_minute}-{current_second}";

	var _file_title = _export_to_dym ? "Export Dynamaker-modified Chart as XML File 导出 DyM 谱面文件" : "Export Dynamix Chart as XML File 导出实机 XML 谱面文件";
    _file = get_save_filename_ext("XML File (*.xml)|*.xml", _default_file_name + ".xml", program_directory, _file_title);
    
    if(_file == "") return;

	// Init the xml export variables.
	_setup_xml_compability_variables();

    var _fix_error = _export_to_dym? false:show_question(i18n_get("export_fix_error_question", global.offsetCorrection));

	var _result = dyc_chart_export_xml(_file, _export_to_dym, _fix_error? global.offsetCorrection:0);
	if(_result < 0)
		announcement_error("anno_export_failed");
	else
		announcement_play("anno_export_complete");

	show_debug_message("Export done.");
}

function map_load_struct(_str, _import_info = true, _import_tp = true) {
	with(objMain) {
		if(_import_info) {
			chartTitle = _str.title;
			chartDifficulty = _str.difficulty;
			chartSideType = _str.sidetype;
		}
	}
	
	var _arr = _str.notes;
	for(var i=0, l=array_length(_arr); i<l; i++) 
		build_note(_arr[i]);
	
	show_debug_message_safe("Load map from struct sucessfully.");
}

function map_get_title() {
	var _title = objMain.chartTitle;
	var _scribble_tag = string_last_pos("[_scribble]", _title) != 0;
	
	if (!_scribble_tag)
		_title = string_replace_all(_title, "[[", "[")
	else {
		var _new_title = "";
		var _in_bracket = false;
		for(var i=1; i<=string_length(_title); i++) {
			if (string_char_at(_title, i) == "[") _in_bracket = true;
			if (!_in_bracket) {
				_new_title += string_char_at(_title, i);
			}
			if (string_char_at(_title, i) == "]") _in_bracket = false;
		}
		_title = _new_title;
	}
	
	return _title;
}

function map_get_alt_title() {
	if(!instance_exists(objMain)) return "example";
	var _forbidden_chars = "?*:\"<>\\/|"
	var _title = map_get_title();
	for(var i=1, l=string_length(_forbidden_chars); i<l; i++)
		_title = string_replace_all(_title, string_char_at(_forbidden_chars, i), "_");
	
	return _title;
}

function map_add_offset(_offset = "", record = false) {
	var _record = false;
	if(_offset == "") {
		var _nega = 1;
		_offset = get_string_i18n(i18n_get("box_add_offset"), "");
		if(_offset == "") return;
		if(string_char_at(_offset, 1) == "-")
			_nega = -1;
		_offset = real(string_real(_offset))*_nega;
		_record = true;
	}
	
	dyc_timingpoints_add_offset(_offset);
	
	DyCore_note_add_offset(_offset);
	
	announcement_play(i18n_get("anno_add_offset", _offset));
	
	if(record)
		operation_step_add(OPERATION_TYPE.OFFSET, _offset, -1);
}

#endregion

#region PROJECT FUNCTIONS

function project_load(_file = "") {
	if(_file == "") 
		_file = get_open_filename_ext("DyNode File (*.dyn)|*.dyn", map_get_alt_title() + ".dyn", program_directory, 
        "Load Project 打开项目");
    
    if(_file == "") return 0;
    
    map_reset();
    
	var result = dyc_project_load(_file);
	if(result < 0) {
		show_debug_message("Project load failed. Terminated.");
		return;
	}
	show_debug_message("Project loaded.");
	var chartMetadata = dyc_chart_get_metadata();
	var projectMetadata = dyc_project_get_metadata();
	var path = dyc_chart_get_path();
	var version = dyc_project_get_version();
	var _propath = filename_path(_file);
    
    var _path_deal = function(_pth, _propath) {
    	if(filename_path(_pth)=="") return _propath+_pth;
    	return _pth;
    }

	objMain.chartTitle = chartMetadata[$ "title"];
	objMain.chartDifficulty = chartMetadata[$ "difficulty"];
	objMain.chartSideType = chartMetadata[$ "sideType"];
    
    with(objManager) {
    	musicPath = path[$ "music"];
    	backgroundPath = path[$ "image"];
    	if(variable_struct_exists(path, "video"))
    		videoPath = path[$ "video"];
    	else
    		videoPath = "";

		objMain.animTargetTime = 0;
		
		if(variable_struct_exists(projectMetadata[$"stats"], "projectTime"))
			projectTime = projectMetadata[$"stats"][$ "projectTime"];
		else
			projectTime = 0;

	    music_load(_path_deal(musicPath, _propath));
	    if(backgroundPath != "")
	    	background_load(_path_deal(backgroundPath, _propath));
	    if(videoPath != "")
	    	background_load(_path_deal(videoPath, _propath));
	    	
	    timing_point_reset();
	    timing_point_sort();
	    
	    projectPath = _file;
	    
	    if(variable_struct_exists(projectMetadata, "settings"))
	    	project_set_settings(projectMetadata[$ "settings"]);
    }
    
    /// Old version workaround
    
	    if(version_cmp(version, "v0.1.5") < 0) {
	    	var _question = show_question_i18n(i18n_get("old_version_warn_1"));
			if(_question)
				map_add_offset(-64, true);
	    }
		
	///

	// Version update backup
	if(version != VERSION) {
		project_backup(objManager.projectPath);
	}
    
    announcement_play("anno_project_load_complete");
    
    return 1;
}

function project_save() {
	return project_save_as(objManager.projectPath);
}

function project_save_as(_file = "") {
	
	if(_file == "")
		_file = get_save_filename_ext("DyNode File (*.dyn)|*.dyn", map_get_alt_title() + ".dyn", program_directory, 
	        "Project save as 项目另存为");
	
	if(_file == "") return 0;
	
	var _contents;
	var _corruption = false;
	var _corruption_file_id = random_id(6);

	DyCore_set_project_version(VERSION);
	DyCore_set_project_metadata(json_stringify({
		stats : {
			projectTime: objManager.projectTime,
		},
		settings: project_get_settings(),
	}));
	DyCore_set_chart_metadata(json_stringify({
		title: objMain.chartTitle,
		difficulty: objMain.chartDifficulty,
		sideType: objMain.chartSideType,
		charter: "",
		artist: ""
	}));
	DyCore_set_chart_path(json_stringify({
		music: objManager.musicPath,
		image: objManager.backgroundPath,
		video: objManager.videoPath
	}));
	
	try {
		project_file_duplicate(
			{
				backgroundPath: objManager.backgroundPath,
				videoPath: objManager.videoPath,
				musicPath: objManager.musicPath,
			}
		);
	} catch (e) {
		announcement_warning("复制音乐/背景/视频文件时出现错误。[scale, 0.7]\n"+string(e));
	}

	// Trigger an async saving project event.
	DyCore_save_project(_file, DYCORE_COMPRESSION_LEVEL);

	return 1;
}

function project_file_duplicate(_project) {
	var _bg = _project.backgroundPath;
	var _vd = _project.videoPath;
	var _mu = _project.musicPath;
	var _propath = filename_path(objManager.projectPath);
	var _new_file_path = function (_old_path, _propath) {
		return _propath + filename_name(_old_path);
	}
	var _nbg = _new_file_path(_bg, _propath);
	var _nvd = _new_file_path(_vd, _propath);
	var _nmu = _new_file_path(_mu, _propath);
	
	var _process = function(_pro, _varname, _file, _nfile) {
		if(filename_path(_file)=="") return;	// If already relative path
		if(file_exists(_file)) {
			if(!file_exists(_nfile))
				file_copy(_file, _nfile);
			else { // Compare file's binary size is more efficent.
				var _f = file_bin_open(_file, 0);
				var _nf = file_bin_open(_nfile, 0);
				var _fs = file_bin_size(_f);
				var _nfs = file_bin_size(_nf);
				file_bin_close(_f);
				file_bin_close(_nf);
				if(_fs != _nfs) {
					_nfile = filename_path(_nfile)+filename_name_no_ext(_nfile)+"_"+random_id(4)+filename_ext(_nfile);
					file_copy(_file, _nfile);
				}
			}
			_nfile = filename_name(_nfile);
			variable_struct_set(_pro, _varname, _nfile);
			variable_instance_set(objManager, _varname, _nfile);
		}
	}
	_process(_project, "backgroundPath", _bg, _nbg);
	_process(_project, "videoPath", _vd, _nvd);
	_process(_project, "musicPath", _mu, _nmu);
	
	return;
}

function project_get_settings() {
	return {
		editside: editor_get_editside(),
		editmode: editor_get_editmode(),
		defaultWidth: objEditor.editorDefaultWidth,
		defaultWidthMode: objEditor.editorDefaultWidthMode,
		ntime: objMain.nowTime,
		fade: objMain.fadeOtherNotes,
		bgdim: objMain.bgDim,
		pbspd: objMain.playbackSpeed,
		hitvol: objMain.volume_get_hitsound(),
		mainvol: objMain.volume_get_main(),
		pitchshift: objMain.usingPitchShift,
		beatlineAlpha: objEditor.animBeatlineTargetAlpha,
		editorSelectMultiSidesBinding: objEditor.editorSelectMultiSidesBinding
	};
}

function project_set_settings(str) {
	editor_set_editmode(str.editmode == 0 ? 3: str.editmode);
	if(str.editmode < 5)
		editor_set_editside(str.editside);
	with(objEditor) {
		editorDefaultWidth = str.defaultWidth;
		editorDefaultWidthMode = str.defaultWidthMode;
	}
	if(variable_struct_exists(str, "ntime")) {
		objMain.nowTime = str.ntime;
		objMain.animTargetTime = str.ntime;
	}
	if(variable_struct_exists(str, "fade")) {
		objMain.fadeOtherNotes = str.fade;
	}
	if(variable_struct_exists(str, "bgdim")) {
		objMain.bgDim = str.bgdim;
	}
	if(variable_struct_exists(str, "pbspd")) {
		objMain.playbackSpeed = str.pbspd;
		objMain.animTargetPlaybackSpeed = str.pbspd;
	}
	if(variable_struct_exists(str, "hitvol")) {
		objMain.volume_set_hitsound(str.hitvol);
	}
	if(variable_struct_exists(str, "mainvol")) {
		objMain.volume_set_main(str.mainvol);
	}
	if(variable_struct_exists(str, "pitchshift")) {
		objMain.music_pitchshift_switch(str.pitchshift);
	}
	if(variable_struct_exists(str, "beatlineAlpha")) {
		objEditor.animBeatlineTargetAlpha = str.beatlineAlpha;
	}
	if(variable_struct_exists(str, "editorSelectMultiSidesBinding")) {
		objEditor.editorSelectMultiSidesBinding = str.editorSelectMultiSidesBinding;
	}
}

function project_new() {
	
	var _confirm = show_question_i18n(i18n_get("new_project_warn"));
	if(!_confirm) return;
	
	with(objManager) {
		musicPath = "";
		backgroundPath = "";
		projectPath = "";
	}
	
	map_close();
	
	room_goto(rProjectInit);
}

function project_auto_save() {
	if(!instance_exists(objMain)) return;
	if(editor_get_editmode() == 0) return;		// If in copy mode, skip this autosave.
	with(objManager) {
		if(projectPath != "") {
			autosaving = true;
			try {
				project_backup(projectPath);
			} catch(e) {
				announcement_error("Project backup failed. Info: "+string(e));
			};
			project_save();
		}
		else {
			announcement_warning("autosave_ineffective");
		}
	}
}

function project_backup_get_name(project_path) {
	var _ret = filename_name_no_ext(project_path)
		 + "_" + DyCore_get_file_modification_time(project_path)
		 + filename_ext(project_path);

	return _ret;
}

function project_backup(project_path) {
	var proDir = filename_dir(project_path);
	var bckDir = proDir + "\\backups\\";

	if(!directory_exists(bckDir))
		directory_create(bckDir);

	var newPth = bckDir + project_backup_get_name(project_path);
	file_copy(project_path, newPth);

	// Also copy the current related files.
	var _fn_copy = function(fr, bckDir) {
		if(fr == "") return;
		var to = bckDir + filename_name(fr);
		if(file_exists(to)) return;
		file_copy(fr, to);
	}
	_fn_copy(objManager.musicPath, bckDir);
	_fn_copy(objManager.videoPath, bckDir);
	_fn_copy(objManager.backgroundPath, bckDir);

	// Check if successfully copied.
	if(!file_exists(newPth))
		show_debug_message("[ERROR] Backup failed.");
	else
		show_debug_message("Backup successfully: " + newPth);
}

#endregion

#region THEME FUNCTIONS

#macro c_neon_pink 0x9A01FE
#macro c_neon_yellow 0x00fffc
function theme_init() {
	
	global.themes = [];
	global.themeAt = 0;
	
	/// Theme Configuration
	
	array_push(global.themes, {
		title: "[c_aqua]Dynamix[/c]",
		color: c_aqua,
		partSpr: sprParticleW,		// Particle Sprite
		partColA: c_neon_pink, 		// Note's Particle Color
		partColB: c_neon_pink,
		partColHA: c_neon_yellow,		// Hold's Particle Color
		partColHB: c_neon_yellow,
		partBlend: false
	});
	
	array_push(global.themes, {
		title: "[c_sakura]Sakura[/c]",
		color: 0xc5b7ff,
		partSpr: sprParticleW,		// Particle Sprite
		partColA: 0xc5b7ff, 		// Note's Particle Color
		partColB: 0xc5b7ff,
		partColHA: c_neon_yellow,		// Hold's Particle Color
		partColHB: c_neon_yellow,
		partBlend: false
	});
	
	array_push(global.themes, {
		title: "Piano",
		color: c_black,
		partSpr: sprParticleW,		// Particle Sprite
		partColA: c_white, 		// Note's Particle Color
		partColB: c_ltgrey,
		partColHA: c_white,		// Hold's Particle Color
		partColHB: c_black,
		partBlend: false
	});
	
	/// End of Configuration
	
	global.themeCount = array_length(global.themes);
	
}

function theme_next() {
	global.themeAt ++;
	global.themeAt %= global.themeCount;
	
	if(instance_exists(objMain))
		objMain.themeColor = global.themes[global.themeAt].color;
	
	announcement_play(i18n_get("anno_switch_theme_to") + " [[" + global.themes[global.themeAt].title + "]", 1000);
}

/// @returns {Any} 
function theme_get() {
	return global.themes[global.themeAt];
}

function theme_get_color_hsv() {
	var col = global.themes[global.themeAt].color;
	return color_rgb_to_hsv(col);
}

#endregion

#region ANNOUNCEMENT FUNCTIONS

function announcement_play(_str, time = 3000, _uniqueID = "null") {
	_str = i18n_get(_str);
	
	var _below = 10;
	var _beside = 10;
	var _nx = BASE_RES_W - _beside;
	var _ny = BASE_RES_H - _below;
	
	if(_uniqueID == "null")
		_uniqueID = random_id(8);
	
	var _found = false;
	with(objManager) {
		var arr = announcements;
		for(var i=0, l=array_length(arr); i<l; i++)
			if(instance_exists(arr[i]) && arr[i].uniqueID == _uniqueID) {
				_found = true;
				with(arr[i]) {
					str = _str;
					lastTime = timer + time;
					_generate_element();
				}
			}
	}
	if(_found) return;
	
	var _inst = instance_create_depth(_nx, _ny, 0, objAnnouncement, {
		str: _str,
		lastTime: time,
		uniqueID: _uniqueID
	});
	
	array_push(objManager.announcements, _inst);
	// show_debug_message_safe("NEW MD5 ANNO: " + _uniqueID);
}

function announcement_warning(str, time = 5000, uid = "null") {
	str = i18n_get(str);
	announcement_play("[c_warning][[" + i18n_get("anno_prefix_warn") + "] [/c]" + str, time, uid);
}

function announcement_error(str, time = 8000, uid = "null") {
	str = i18n_get(str);
	announcement_play("[#f44336][[" + i18n_get("anno_prefix_err") + "] " + str, time, uid);
	show_debug_message_safe(str);
}

function announcement_adjust(str, val) {
	str = i18n_get(str);
	announcement_play(str + ": " + i18n_get(val?"anno_adjust_enabled":"anno_adjust_disabled"), 3000, md5_string_unicode(str));
}

function announcement_set(str, val) {
	str = i18n_get(str);
	if(is_real(val))
		val = string_format(val, 1, 2);
	announcement_play(str + ": " + i18n_get(string(val)), 3000, md5_string_unicode(str));
}

#endregion

#region SYSTEM FUNCTIONS

function get_config_path() {
	if(os_type == os_linux) {
		return "config.json";
	}
	else
		return SYSFIX + program_directory + "config.json";
}

function load_config() {
	var pth = get_config_path();
	if(!file_exists(pth) || DEBUG_MODE)
		save_config();
	
	if(!file_exists(pth))
		show_error("Config file creating failed.", true)
	
	var _buf = buffer_load(pth);
	var _con = SnapBufferReadLooseJSON(_buf, 0);
	buffer_delete(_buf);
	
	// If config file is corrupted
	if(!is_struct(_con)) {
		show_error_i18n("error_config_file_corrupted", false);
		file_delete(pth);
		load_config();
		return -1;
	}
	
	var _check_set = function (struct, struct_name, global_name = "") {
		if(global_name == "") global_name = struct_name;
		if(variable_struct_exists(struct, struct_name))
			variable_global_set(global_name, variable_struct_get(struct, struct_name));
	}
	
	_check_set(_con, "theme", "themeAt");
	_check_set(_con, "FPS", "fps");
	_check_set(_con, "autosave");
	_check_set(_con, "autoupdate");
	_check_set(_con, "FMOD_MP3_DELAY");
	_check_set(_con, "ANNOUNCEMENT_MAX_LIMIT");
	_check_set(_con, "fullscreen");
	if(variable_struct_exists(_con, "language"))
		i18n_set_lang(variable_struct_get(_con, "language"));
	_check_set(_con, "simplify");
	_check_set(_con, "updatechannel");
	_check_set(_con, "graphics");
	_check_set(_con, "beatlineStyle");
	_check_set(_con, "musicDelay");
	_check_set(_con, "dropAdjustError");
	_check_set(_con, "lastCheckedVersion");
	_check_set(_con, "offsetCorrection");
	_check_set(_con, "VIDEO_UPDATE_FREQUENCY");
	_check_set(_con, "autoSaveTime");
	_check_set(_con, "analytics");
	_check_set(_con, "particleEffects");
	_check_set(_con, "PROJECT_COMPRESSION_LEVEL");
	// Clamp the offset correction.
	global.offsetCorrection = max(0, global.offsetCorrection)
	global.autoSaveTime = max(1, global.autoSaveTime);
	vars_init();

	// Version check.
	if(_con[$ "version"] != VERSION) {
		if(version_cmp(VERSION, _con[$ "version"]) > 0)
			announcement_play(i18n_get("version_higher", VERSION));
		else
			announcement_warning(i18n_get("version_lower", VERSION));
	}
	
	return md5_file(pth);
}

function save_config() {
	
	fast_file_save(get_config_path(), SnapToJSON({
		theme: global.themeAt,
		FPS: global.fps,
		version: VERSION,
		autosave: global.autosave,
		autoupdate: global.autoupdate,
		FMOD_MP3_DELAY: global.FMOD_MP3_DELAY,
		ANNOUNCEMENT_MAX_LIMIT: global.ANNOUNCEMENT_MAX_LIMIT,
		fullscreen: global.fullscreen,
		language: i18n_get_lang(),
		simplify: global.simplify,
		updatechannel: global.updatechannel,
		graphics: global.graphics,
		beatlineStyle: global.beatlineStyle,
		musicDelay: global.musicDelay,
		dropAdjustError: global.dropAdjustError,
		lastCheckedVersion: global.lastCheckedVersion,
		offsetCorrection: global.offsetCorrection,
		VIDEO_UPDATE_FREQUENCY: global.VIDEO_UPDATE_FREQUENCY,
		autoSaveTime: global.autoSaveTime,
		analytics: global.analytics,
		particleEffects: global.particleEffects,
		PROJECT_COMPRESSION_LEVEL: global.PROJECT_COMPRESSION_LEVEL
	}, true));
	
}

function md5_config() {
	if(!file_exists(get_config_path()))
		save_config();
	
	return md5_file(get_config_path());
}

function vars_init() {
	// Some variables that will take changes immediately
	
	if(DEBUG_MODE) global.fps = 165;
	game_set_speed(global.fps, gamespeed_fps);
	global.fpsAdjust = BASE_FPS / global.fps;
	
	if(instance_exists(objMain))
		with(objMain) _partsys_init();
}

function switch_debug_info() {
	with(objMain) {
		showDebugInfo = !showDebugInfo;
		announcement_adjust("anno_debug_info", showDebugInfo);
	}
}

function switch_autosave(state = !global.autosave) {
	with(objManager) {
		if(state) {
			announcement_play("autosave_enable", 2000, "autosave_switch");
			time_source_start(tsAutosave);
		}
		else {
			announcement_play("autosave_disable", 2000, "autosave_switch");
			time_source_stop(tsAutosave);
		}
		global.autosave = state;
	}
}

#endregion

#region Stat Functions

function stat_reset() {
	objMain.statCount = [
		[0, 0, 0, 0],
		[0, 0, 0, 0],
		[0, 0, 0, 0],
		[0, 0, 0, 0]
		];
}

function stat_note_string(stype, ntype) {
	with(objMain)
		if(stype == 1) {
			return string(statCount[3][ntype]);
		}
		else if(stype == 2) {
			return string_concat(statCount[1][ntype],"/",
								 statCount[0][ntype],"/",
								 statCount[2][ntype]);
		}
}

function stat_next() {
	objMain.showStats ++;
	objMain.showStats %= MAX_STAT_TYPE;

	note_recac_stats();
}

function stat_visible() {
	if(!instance_exists(objMain))
		return false;
	return objMain.showStats > 0;
}

/// @description Caculate the avg notes' count between [_time-_range, _time] (in ms)
function stat_kps(_time, _range) {
	return DyCore_kps_count(_time, _range);
}

#endregion

#region FMOD Functions

function sfmod_channel_get_position(channel, spr) {
    var _ret = FMODGMS_Chan_Get_Position(channel);
    _ret = _ret - global.FMOD_MP3_DELAY * objMain.usingMP3 - global.musicDelay;
    return _ret;
}

function sfmod_channel_set_position(pos, channel, spr) {
    pos = pos + global.FMOD_MP3_DELAY * objMain.usingMP3 + global.musicDelay;
    FMODGMS_Chan_Set_Position(channel, pos);
}

#endregion

#region Misc Functions

function game_end_confirm() {
	var _confirm_exit = instance_exists(objMain) ? show_question_i18n("confirm_close") : true;
	if(_confirm_exit)
		game_end();
}

function reset_scoreboard() {
	with(objScoreBoard) {
		nowScore = 0;
		animTargetScore = 0;
	}
	with(objPerfectIndc) {
		nowTime = 99999;
	}
}

function global_add_delay(delay) {
	global.musicDelay += delay;
	with(objMain)
		if(nowPlaying)
			nowTime -= delay;
	save_config();
	announcement_set("global_music_delay", global.musicDelay);
}

#endregion
