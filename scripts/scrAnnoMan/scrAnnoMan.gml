

function AnnouncementManager() constructor {

	annoThresholdNumber = 7;
	annoThresholdTime = 400;
	annosY = [];

    /// @type {Array<Id.Instance.objAnnouncement>} 
    announcements = [];

    static is_existed = function(annoID) {
        var arr = announcements;
        for(var i=0, l=array_length(arr); i<l; i++)
            if(instance_exists(arr[i]) && arr[i].uniqueID == annoID)
                return true;
        return false;
    }

    static update_announcement = function(annoID, msg, duration, type, state) {
        var arr = announcements;
        for(var i=0, l=array_length(arr); i<l; i++)
            if(instance_exists(arr[i]) && arr[i].uniqueID == annoID && arr[i].annoType == type) {
                with(arr[i]) {
                    str = msg;
                    lastTime = timer + duration;
                    set_state(state);
                    _generate_element();
                }
            }
    }

    static create_announcement = function(msg, duration, annoID, type, state) {
        msg = i18n_get(msg);
	
        var _below = 10;
        var _beside = 10;
        var _nx = BASE_RES_W - _beside;
        var _ny = BASE_RES_H - _below;
        
        if(annoID == "null")
            annoID = random_id(8);

        if(is_existed(annoID)) {
            update_announcement(annoID, msg, duration, type, state);
            return;
        }
        
        /// @type {Id.Instance.objAnnouncement}
        var _inst = instance_create_depth(_nx, _ny, 0, objAnnouncement);
        _inst.init(msg, duration, annoID, type);
        _inst.set_state(state);
        
        array_push(announcements, _inst);
    }

    static step = function() {
        // Clear removed annos
        for(var i=0, l=array_length(announcements); i<l; i++) {
            if(!instance_exists(announcements[i])) {
                array_delete(announcements, i, 1);
                i--;
                l--;
            }
        }

        // Caculate annos' Y
        var _h = 0;
        var _margin = 15;
        var _l = array_length(announcements);
        for(var i=array_length(announcements)-1; i>=0; i--) {
            /// @self Id.Instance.objAnnouncement
            with(announcements[i]) {
                targetY = _h;
                _h += element.get_bbox().height + _margin;
                
                if(_l - i > other.annoThresholdNumber)
                    lastTime = min(lastTime, timer + other.annoThresholdTime);
            }
        }
    }
}


function announcement_play(_str, time = 3000, _uniqueID = "null") {
	global.announcementMan.create_announcement(_str, time, _uniqueID, ANNO_TYPE.NORMAL, ANNO_STATE.IDLE);
}

function announcement_task(_str, time = 3000, _uniqueID = "null", state = ANNO_STATE.PROCESSING) {
    global.announcementMan.create_announcement(_str, time, _uniqueID, ANNO_TYPE.TASK, state);
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