// DyCore.cpp: Core of DyNode.

#include "pch.h"
#include "framework.h"
#include "DyCore.h"

ChartInstance chart;

// For DyNode
vector<int> notePointers;

DYCORE_API double DyCore_note_iterator_new() {
	notePointers.push_back(0);
	return notePointers.size() - 1;
}

DYCORE_API double DyCore_note_iterator_clear() {
	notePointers.clear();
	return true;
}

DYCORE_API double DyCore_note_iterator_read_real(double id, char* type) {
	string nType(type);
	NoteInstance inst = chart.get_note_at(id);

	if (nType == "side") {
		return (int)inst.side;
	}
	else if (nType == "width") {
		return inst.width;
	}
	else if (nType == "position") {
		return inst.position;
	}
	else if (nType == "time") {
		return inst.time;
	}
	else if (nType == "type") {
		return (int)inst.noteType;
	}
}

DYCORE_API const char * DyCore_note_iterator_read_string(double id, char* type) {
	string nType(type);
	NoteInstance inst = chart.get_note_at(id);

	if (nType == "nid") {
		return inst.noteId.c_str();
	}
	else if (nType == "sid") {
		return inst.subId.c_str();
	}
}

DYCORE_API double DyCore_note_iterator_add(double id) {
	notePointers[id] ++;
	return true;
}

DYCORE_API const char* DyCore_init() {

	return "success";
}

DYCORE_API double DyCore_chart_load(char* filePath) {

}

