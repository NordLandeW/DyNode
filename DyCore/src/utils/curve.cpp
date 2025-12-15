
#include "curve.h"

#include <cstddef>
#include <cubinterpp_header.hpp>

#include "api.h"
#include "json.hpp"
#include "utils.h"

/// Solve a natural spline interpolation problem given points (x, f).
/// Returns 0 on success, -1 on failure.
int solve_natural_spline(std::vector<double> x, std::vector<double> f,
                         std::vector<double> xQuery,
                         std::vector<double>& fQuery) {
    try {
        cip::NaturalSpline1D<double> spline(x, f);
        fQuery = spline.evaln(xQuery);
    } catch (...) {
        return -1;
    }

    return 0;
}

DYCORE_API double DyCore_solve_natural_spline(const void* _xf,
                                              const void* _x_query,
                                              void* _f_query,
                                              const char* params) {
    try {
        nlohmann::json j = nlohmann::json::parse(params);

        size_t n_points = j["n_points"].get<size_t>();
        size_t n_query = j["n_query"].get<size_t>();

        const double* x = static_cast<const double*>(_xf);
        const double* f = static_cast<const double*>(_xf) + n_points;
        const double* x_query = static_cast<const double*>(_x_query);
        double* f_query = static_cast<double*>(_f_query);

        std::vector<double> x_vec(x, x + static_cast<size_t>(n_points));
        std::vector<double> f_vec(f, f + static_cast<size_t>(n_points));
        std::vector<double> x_query_vec(x_query,
                                        x_query + static_cast<size_t>(n_query));
        std::vector<double> f_query_vec;

        int result =
            solve_natural_spline(x_vec, f_vec, x_query_vec, f_query_vec);

        if (result == 0) {
            print_debug_message("Natural Spline solving finished.");
            for (size_t i = 0; i < n_query; ++i) {
                f_query[i] = f_query_vec[i];
            }
            return 0.0;
        }
    } catch (const std::exception& e) {
        print_debug_message(
            "Error in DyCore_solve_natural_spline. Exception: " +
            std::string(e.what()));
        return -1.0;
    }
    return -1.0;
}