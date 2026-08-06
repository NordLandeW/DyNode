#pragma once

#include <cstddef>
#include <string>

namespace render_benchmark {

struct BenchmarkOptions {
    std::size_t noteCount = 20000;
    std::size_t iterations = 100;
    std::size_t warmupIterations = 10;
    std::string scenario = "mixed";
    std::string chartPath;
    double noteSpeed = 0.0;
    std::size_t workerCount = 0;
};

BenchmarkOptions parse_options(int argc, char** argv);

}  // namespace render_benchmark
