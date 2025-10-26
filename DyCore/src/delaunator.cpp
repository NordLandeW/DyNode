
#include "delaunator.hpp"

#include "api.h"
#include "json.hpp"
#include "utils/bitio.h"
#include <cstdint>
#include <vector>

using nlohmann::json;

// ! Deprecated.
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

// Binary buffer interface (preferred). Input: [u32 point_count][x f64][y f64]*N
// Output: [u32 tri_count][x1 f64][y1 f64][x2 f64][y2 f64][x3 f64][y3 f64]*tri
// coordFormat: 0 = f32, 1 = f64 (both for input). Output uses f64 by design.
DYCORE_API double DyCore_delaunator_buffer(char* inBuf, char* outBuf, double outCapacityBytes, double coordFormat) {
    if (!inBuf || !outBuf) return -1.0;

    try {
        const bool inputIsF64 = static_cast<int>(coordFormat) == 1;

        // Read input buffer
        const char* r = inBuf;
        uint32_t pointCount = 0;
        bitread(r, pointCount);

        std::vector<double> coords;
        coords.reserve(static_cast<size_t>(pointCount) * 2);

        if (inputIsF64) {
            for (uint32_t i = 0; i < pointCount; ++i) {
                double x, y;
                bitread(r, x);
                bitread(r, y);
                coords.push_back(x);
                coords.push_back(y);
            }
        } else {
            for (uint32_t i = 0; i < pointCount; ++i) {
                float x, y;
                bitread(r, x);
                bitread(r, y);
                coords.push_back(static_cast<double>(x));
                coords.push_back(static_cast<double>(y));
            }
        }

        if (coords.size() < 6) {
            // Not enough points to form any triangle: write header (0) and return 0
            const size_t required = sizeof(uint32_t);
            if (static_cast<size_t>(outCapacityBytes) < required)
                return -static_cast<double>(required);
            char* w = outBuf;
            uint32_t triCount = 0;
            bitwrite(w, triCount);
            return 0.0;
        }

        // Perform triangulation
        delaunator::Delaunator d(coords);
        const std::size_t triCount = d.triangles.size() / 3;

        // Calculate required bytes: header + 6 doubles per triangle
        const size_t required = sizeof(uint32_t) + triCount * 6 * sizeof(double);
        if (static_cast<size_t>(outCapacityBytes) < required) {
            return -static_cast<double>(required);
        }

        // Write output buffer
        char* w = outBuf;
        uint32_t triCountU32 = static_cast<uint32_t>(triCount);
        bitwrite(w, triCountU32);

        for (std::size_t i = 0; i < d.triangles.size(); i += 3) {
            std::size_t i0 = d.triangles[i];
            std::size_t i1 = d.triangles[i + 1];
            std::size_t i2 = d.triangles[i + 2];

            double x0 = d.coords[2 * i0];
            double y0 = d.coords[2 * i0 + 1];
            double x1 = d.coords[2 * i1];
            double y1 = d.coords[2 * i1 + 1];
            double x2 = d.coords[2 * i2];
            double y2 = d.coords[2 * i2 + 1];

            bitwrite(w, x0); bitwrite(w, y0);
            bitwrite(w, x1); bitwrite(w, y1);
            bitwrite(w, x2); bitwrite(w, y2);
        }

        return static_cast<double>(triCount);
    } catch (...) {
        return -1.0;
    }
}
