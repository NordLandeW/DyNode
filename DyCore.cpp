// DyCore.cpp: 定义应用程序的入口点。
//

#include "DyCore.h"

const int RETURN_BUFFER_SIZE = 10 * 1024 * 1024;
char _return_buffer[RETURN_BUFFER_SIZE];

DYCORE_API const char* DyCore_init() {

	return "success";
}

DYCORE_API const char* DyCore_delaunator(char* in_struct) {
	json j = json::parse(in_struct);

	if (!j.is_array())
		return "Error!in_struct must be an array.";

	auto coords = j.get<std::vector<double>>();

	delaunator::Delaunator d(coords);

	json r = json::array();

	for (std::size_t i = 0; i < d.triangles.size(); i += 3) {
		r.push_back(
			{
				{"p1", {d.coords[2 * d.triangles[i]], d.coords[2 * d.triangles[i] + 1]}},
				{"p2", {d.coords[2 * d.triangles[i + 1]], d.coords[2 * d.triangles[i + 1] + 1]}},
				{"p3", {d.coords[2 * d.triangles[i + 2]], d.coords[2 * d.triangles[i + 2] + 1]}}
			}
		);
	}

	strcpy(_return_buffer, r.dump().c_str());


	return _return_buffer;
}