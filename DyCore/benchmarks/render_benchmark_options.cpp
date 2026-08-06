#include "render_benchmark_options.h"

#include <array>
#include <stdexcept>
#include <string>
#include <string_view>

namespace render_benchmark {
namespace {

using OptionParser = void (*)(BenchmarkOptions&, std::string_view);

struct OptionSpec {
    std::string_view name;
    OptionParser parser;
};

std::size_t parse_size_value(std::string_view value) {
    return std::stoull(std::string(value));
}

double parse_double_value(std::string_view value) {
    return std::stod(std::string(value));
}

void set_note_count(BenchmarkOptions& options, std::string_view value) {
    options.noteCount = parse_size_value(value);
}

void set_iterations(BenchmarkOptions& options, std::string_view value) {
    options.iterations = parse_size_value(value);
}

void set_warmup_iterations(BenchmarkOptions& options, std::string_view value) {
    options.warmupIterations = parse_size_value(value);
}

void set_scenario(BenchmarkOptions& options, std::string_view value) {
    options.scenario = value;
}

void set_chart_path(BenchmarkOptions& options, std::string_view value) {
    options.chartPath = value;
}

void set_note_speed(BenchmarkOptions& options, std::string_view value) {
    options.noteSpeed = parse_double_value(value);
}

void set_worker_count(BenchmarkOptions& options, std::string_view value) {
    options.workerCount = parse_size_value(value);
}

constexpr std::array<OptionSpec, 7> OPTION_SPECS{{
    {"--notes", set_note_count},
    {"--iterations", set_iterations},
    {"--warmup", set_warmup_iterations},
    {"--scenario", set_scenario},
    {"--chart", set_chart_path},
    {"--speed", set_note_speed},
    {"--workers", set_worker_count},
}};

const OptionSpec* find_option(std::string_view argument) {
    for (const auto& option : OPTION_SPECS) {
        if (option.name == argument) {
            return &option;
        }
    }
    return nullptr;
}

bool is_supported_scenario(std::string_view scenario) {
    return scenario == "normal" || scenario == "holds" || scenario == "mixed" ||
           scenario == "clustered";
}

void validate_options(const BenchmarkOptions& options) {
    if (options.noteCount == 0 || options.iterations == 0) {
        throw std::invalid_argument("notes and iterations must be positive");
    }
    if (options.chartPath.empty() && !is_supported_scenario(options.scenario)) {
        throw std::invalid_argument(
            "scenario must be normal, holds, mixed, or clustered");
    }
}

}  // namespace

BenchmarkOptions parse_options(int argc, char** argv) {
    BenchmarkOptions options;
    for (int index = 1; index < argc;) {
        const std::string_view argument = argv[index];
        const OptionSpec* option = find_option(argument);
        if (option == nullptr) {
            throw std::invalid_argument("Unknown argument: " +
                                        std::string(argument));
        }
        if (++index >= argc) {
            throw std::invalid_argument(std::string(argument) +
                                        " requires a value");
        }
        option->parser(options, argv[index]);
        ++index;
    }
    validate_options(options);
    return options;
}

}  // namespace render_benchmark
