
#include "delaunator.hpp"

#include "api.h"
#include "json.hpp"

using nlohmann::json;

// Performs Delaunay triangulation on a set of 2D points.
//
// @param in_struct A JSON string representing an array of coordinates.
//        Example: "[x1, y1, x2, y2, ...]"
// @return A JSON string representing an array of triangles.
//         Returns an error message if the input is not a valid array.
DYCORE_API const char* DyCore_delaunator(char* in_struct) {
    static std::string returnBuffer;

    json j = json::parse(in_struct);

    if (!j.is_array())
        return "Error!in_struct must be an array.";

    auto coords = j.get<std::vector<double>>();

    delaunator::Delaunator d(coords);

    json r = json::array();

    for (std::size_t i = 0; i < d.triangles.size(); i += 3) {
        r.push_back(
            {{"p1",
              {d.coords[2 * d.triangles[i]], d.coords[2 * d.triangles[i] + 1]}},
             {"p2",
              {d.coords[2 * d.triangles[i + 1]],
               d.coords[2 * d.triangles[i + 1] + 1]}},
             {"p3",
              {d.coords[2 * d.triangles[i + 2]],
               d.coords[2 * d.triangles[i + 2] + 1]}}});
    }

    returnBuffer = r.dump();

    return returnBuffer.c_str();
}
