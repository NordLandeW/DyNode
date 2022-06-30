
#define DYCORE_API extern "C" __declspec(dllexport)
#include "pch.h"

using json = nlohmann::json;
using std::string;
using std::vector;
using std::map;

enum CHART_DIFFICULTY {
	CASUAL, NORMAL, HARD, MEGA, GIGA, TERA
};
const string DIFF_STRING = "CNMHGT";



class NoteInstance {

};

class ChartInstance {
private:
	enum CHART_SIDE_TYPE {
		MIXER, MULTI
	};
	enum CHART_SIDE {
		DOWN, LEFT, RIGHT
	};

	string chartTitle;

	int chartSideType[2];

	double chartBeatPerMinute, chartBarPerMinute, chartTimeOffset;

	int chartDifficulty;

	string chartID, chartMusicFilePath, chartFilePath;

	vector<NoteInstance> chartNotesList;
	map<string, NoteInstance> chartNotesMap[3];

public:

	ChartInstance() {
		chartID = chartMusicFilePath = chartFilePath = chartTitle = "";
		chartDifficulty = CASUAL;
		chartSideType[0] = MIXER, chartSideType[1] = MULTI;
		chartBeatPerMinute = 180.0;
		chartBarPerMinute = chartBeatPerMinute / 4;
		chartTimeOffset = 0;
	}
};

class ProjectInstance {

};