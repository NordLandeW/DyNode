/// @description 
// visible = false;

depth = -10000;

// Editors
editorMode = 5;                             // 1 note 2 chain 3 hold 4 view 5 play 0 copy
editorModeBeforeCopy = editorMode;
editorSide = 0;                             // 0 down 1 left 2 right
editorLRSide = false;                       // Special mode
editorLRSideLock = false;                   // For special mode lock
editorLastSide = 0;
/// @type {Array<Id.Instance.objNote>} instances that attached to cursor now
editorNoteAttaching = -1;
editorNoteAttachingCenter = 0;           // attached instances' center

// Expressions
editorLastExpr = "";

// Width for different modes
editorDefaultWidth = [
	1.0,
	1.0,
	[1.0, 1.0],
	[1.0, 1.0, 1.0]
	];
editorDefaultWidthModeName = [
	"default_width_mode_name_1",
	"default_width_mode_name_2",
	"default_width_mode_name_3",
	"default_width_mode_name_4"
	];
editorDefaultWidthMode = 0;

editorSelectMultiSidesBinding = true;       // multiple sides selected notes' properties' binding
/// @type {Id.Instance.objNote} 
editorSelectSingleTarget = -999;            // instance for single selection target
/// @type {Id.Instance.objNote} 
editorSelectSingleTargetInbound = -999;
editorSelectOccupied = false;               // selecting note
editorSelectCount = 0;  
editorSelectMultiple = false;               // selecting multiple notes
editorSelectDragOccupied = false;           // dragging notes 
editorSelectArea = false;                   // selecting area
/// @type {Any}
editorSelectAreaPosition = undefined;
editorSelectInbound = false;                // if at least mouse is inbound one selected note
editorSelectResetRequest = false;
/// @type {Id.Instance.objNote} 
editorSelectedSingleInbound = -999;         // Selected single note mouse inbound checker
/// @type {Id.Instance.objNote} 
editorSelectedSingleInboundLast = -999;         // Selected single note mouse inbound checker
editorNoteSortRequest = false;

editorHighlightLine = false;
editorHighlightLineFix = 1;                 // Fix the flickering issue by last at least 2 frames.
editorHighlightLineEnabled = true;
editorHighlightTime = 0;
editorHighlightPosition = 0;
editorHighlightSide = 0;
editorHighlightWidth = 0;

editorWidthAdjustTime = 9999;
editorWidthAdjustTimeThreshold = 300;

    // Grid Settings
    editorGridXEnabled = true;
    editorGridYEnabled = true;
    editorGridWidthEnabled = true;

// Highlight lines
highlightLineColorDownA = scribble_rgb_to_bgr(0xb6ffff);
highlightLineColorDownB = scribble_rgb_to_bgr(0x81d4fa);
highlightLineColorSideA = scribble_rgb_to_bgr(0xffc1e3);
highlightLineColorSideB = scribble_rgb_to_bgr(0xce93d8);

// Beatlines
enum BeatlineStyles {
	BS_DEFAULT,
	BS_MONOLONG,
	BS_MONO,
	BS_LONG,
};

#macro BEATLINE_STYLES_COUNT 4

beatlineStylesName = [];
beatlineStylesName[BeatlineStyles.BS_DEFAULT] = "beatline_style_default";
beatlineStylesName[BeatlineStyles.BS_MONOLONG] = "beatline_style_monolong";
beatlineStylesName[BeatlineStyles.BS_MONO] = "beatline_style_mono";
beatlineStylesName[BeatlineStyles.BS_LONG] = "beatline_style_long";

beatlineStyleCurrent = global.beatlineStyle;

beatlineSurf = -1;
beatlineColors = [0, 0xffffff, 0x3643f4, 0xb0279c, 0xf39621,
    0x6abb66, 0x889600, 0x757575, 0x3bebff,
    0, 0x33cac0, 0, 0x37405d,
    0, 0x414c6d, 0, 0x921b31,
    0, 0, 0, 0x00b3ff,
    0, 0, 0, 0,
    0, 0, 0, 0x7a6e54]; // 1/1, 1/2, 1/3 ...
beatlineLengthOffset = [
    0,
    0,
    -30,    // 2
    -30,    // 3
    -60,    // 4
    -30,    // 5
    -60,    // 6
    -30,
    -90,
    0,
    -60,
    0,
    -120,
    0,
    -60,
    0,
    -120,   // 16
    0, 0, 0, -120, // 20
    0, 0, 0, 0, 0, 0, 0, -120];
beatlineModes = [
    [
        [1],
        [1, 2],
        [1, 3],
        [1, 2, 4],
        [1, 3, 6],
        [1, 2, 4, 8],
        [1, 2, 4, 8, 16],
        [1, 3, 6, 12], 
        [1, 2, 4, 8, 16],
        [1, 2, 4, 8, 16],
        [1, 2, 4, 8, 16]
    ],
    [
        [1],
        [1, 5],
        [1, 7],
        [1, 5, 10],
        [1, 7, 14],
        [1, 5, 10, 20],
        [1, 7, 14, 28]
    ]
];
beatlineDivs = [[1, 2, 3, 4, 6, 8, 16, 12, 32, 64, 128], [1, 5, 7, 10, 14, 20, 28]];
beatlineNowGroup = 0;
beatlineNowMode = 3;
beatlineDiv = -1;
beatlineEnabled = array_create(28, 0);

beatlineHardWidth = 6;
beatlineWidth = 3;
beatlineHardLength = BASE_RES_W * 0.9;
beatlineLength = BASE_RES_W * 0.75;
beatlineLengthLong = BASE_RES_W - objMain.targetLineBeside * 2;
beatlineHardHeight = (BASE_RES_H - objMain.targetLineBeside) * 0.95;
beatlineHeight = (BASE_RES_H - objMain.targetLineBeside) * 0.90;
beatlineAlpha = [0, 0, 0];
beatlineAlphaMul = 0;
beatlineSideInfoX = resor_to_x(0.5)+beatlineHardLength/2;
beatlineSideInfoDivWidth = 100;

animSpeed = 0.4;
animBeatlineTargetAlpha = [0.7, 0, 0];
animBeatlineTargetAlphaM = 0;

// Copy & Paste

copyStack = [];
copyRequest = false;
cutRequest = false;
attachRequest = false;
attachRequestCenterID = undefined;
copyMultipleSides = false;
singlePaste = false;		// Only paste for one time then return (for attach mode)

// Undo & Redo

operationStack = [];
operationStackStep = [];
operationPointer = -1;
operationCount = 0;
operationSyncTime = [INF, -INF];		// The earliest operated instance's time on last step
operationMergeLastRequest = 0;
operationMergeLastRequestCount = 0;
/// @type {Enum.OPERATION_TYPE} 
operationMergeLastRequestType = undefined;  // Used for special operation type.

// Methods

/// Get editor's divisor number
function get_div() {
	return beatlineDiv;
}

/// Set editor's divisor to [_div]
/// If [_match], then set the beatlineNowGroup and beatlineNowMode to match the divisor.
function set_div(_div, _match = true) {
	beatlineDiv = _div;
	if(_match)
	for(var i=0; i<2; i++)
	for(var j=0; j<array_length(beatlineDivs[i]); j++) {
		if(beatlineDivs[i][j] == _div) {
			beatlineNowGroup = i;
			beatlineNowMode = j;
			return;
		}
	}
}

/// Send copy request.
function copy() {
	copyRequest = true;
}

/// Send cut request.
function cut() {
	cutRequest = true;
}

/// Send attach request.
/// @param {Id.Instance.objNote} inst - The note instance to attach to.
function attach(inst) {
	attachRequest = true;
	singlePaste = true;
    attachRequestCenterID = inst.noteID;
}

set_div(4);			// Default divisor set to 4