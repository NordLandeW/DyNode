

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

    static update_announcement = function(annoID, msg, duration) {
        var arr = announcements;
        for(var i=0, l=array_length(arr); i<l; i++)
            if(instance_exists(arr[i]) && arr[i].uniqueID == annoID) {
                with(arr[i]) {
                    str = msg;
                    lastTime = timer + duration;
                }
            }
    }

    static create_announcement = function(msg, duration, annoID) {
        msg = i18n_get(msg);
	
        var _below = 10;
        var _beside = 10;
        var _nx = BASE_RES_W - _beside;
        var _ny = BASE_RES_H - _below;
        
        if(annoID == "null")
            annoID = random_id(8);

        if(is_existed(annoID)) {
            update_announcement(annoID, msg, duration);
            return;
        }
        
        /// @type {Id.Instance.objAnnouncement}
        var _inst = instance_create_depth(_nx, _ny, 0, objAnnouncement);
        _inst.init(msg, duration, annoID);
        
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