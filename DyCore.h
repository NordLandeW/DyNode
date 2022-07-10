
#define DYCORE_API extern "C" __declspec(dllexport)
#include "pch.h"

using json = nlohmann::json;
using std::string;
using std::vector;
using std::map;

class NoteInstance;
class ChartInstance;

enum CHART_DIFFICULTY {
	CASUAL, NORMAL, HARD, MEGA, GIGA, TERA
};
enum CHART_SIDE_TYPE {
	MIXER, MULTI
};
enum CHART_SIDE {
	DOWN, LEFT, RIGHT
};
enum NOTE_TYPE {
	NOTE, CHAIN, HOLD, SUB
};
const string DIFF_STRING = "CNMHGT";
const string CHART_SIDE_TYPE_STR[2] = { "MULTI", "MIXER" };

CHART_SIDE_TYPE chart_sidetype_stoi(string stype) {
	if (stype == "MULTI") return MULTI;
	else if (stype == "MIXER") return MIXER;
	else return CHART_SIDE_TYPE(-1);
}



class NoteInstance {
public:
	NOTE_TYPE noteType;
	double width, position, time;
	CHART_SIDE side;

	string noteId = "", subId = "";

	NoteInstance(NOTE_TYPE type, int wid, int pos, int tim, CHART_SIDE nside, string id, string sid) :
		noteType(type), width(wid), position(pos), time(tim), side(nside), noteId(id), subId(sid)
	{
		if (type != HOLD) subId = "-1";
	}
};

class ChartInstance {
private:
	string chartTitle;

	CHART_SIDE_TYPE chartSideType[2];

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

	NoteInstance get_note_at(int id) {
		return chartNotesList[id];
	}

	bool load_from_xml(string filePath) {
		using namespace pugi;
		
		xml_document cxml;
		cxml.load_file(filePath.c_str());

		xml_node cmap = cxml.child("CMap");
		chartTitle = cmap.child_value("m_path");
		chartBarPerMinute = std::stof(cmap.child_value("m_barPerMin"));
		chartTimeOffset = std::stof(cmap.child_value("m_timeOffset"));
		chartSideType[0] = chart_sidetype_stoi(cmap.child_value("m_leftRegion"));
		chartSideType[1] = chart_sidetype_stoi(cmap.child_value("m_rightRegion"));
		chartID = cmap.child_value("m_mapID");

		xml_node downside = cmap.child("m_notes").child("m_notes");

		for (xml_node note : downside.children("CMapNoteAsset")) {

		}
	}
};

class ProjectInstance {

};