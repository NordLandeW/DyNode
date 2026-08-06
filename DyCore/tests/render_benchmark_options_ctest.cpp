#include <doctest/doctest.h>

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

#include "render_benchmark_options.h"

namespace {

render_benchmark::BenchmarkOptions parse(
    std::initializer_list<std::string> arguments) {
    std::vector<std::string> mutableArguments(arguments);
    std::vector<char*> argv;
    argv.reserve(mutableArguments.size());
    for (auto& argument : mutableArguments) {
        argv.push_back(argument.data());
    }
    return render_benchmark::parse_options(static_cast<int>(argv.size()),
                                           argv.data());
}

}  // namespace

TEST_CASE("RenderBenchmarkOptionsParseArguments") {
    const auto options =
        parse({"render_benchmark", "--notes", "42", "--iterations", "7",
               "--warmup", "3", "--scenario", "normal", "--chart", "chart.dyn",
               "--speed", "1.75", "--workers", "4"});

    CHECK(options.noteCount == 42);
    CHECK(options.iterations == 7);
    CHECK(options.warmupIterations == 3);
    CHECK(options.scenario == "normal");
    CHECK(options.chartPath == "chart.dyn");
    CHECK(options.noteSpeed == doctest::Approx(1.75));
    CHECK(options.workerCount == 4);
}

TEST_CASE("RenderBenchmarkOptionsValidateArguments") {
    const auto zeroNotes = [] {
        (void)parse({"render_benchmark", "--notes", "0"});
    };
    CHECK_THROWS_WITH_AS(zeroNotes(), "notes and iterations must be positive",
                         std::invalid_argument);

    const auto invalidScenario = [] {
        (void)parse({"render_benchmark", "--scenario", "invalid"});
    };
    CHECK_THROWS_WITH_AS(invalidScenario(),
                         "scenario must be normal, holds, mixed, or clustered",
                         std::invalid_argument);

    const auto chartScenario = parse(
        {"render_benchmark", "--chart", "chart.dyn", "--scenario", "custom"});
    CHECK(chartScenario.scenario == "custom");
}

TEST_CASE("RenderBenchmarkOptionsRejectUnknownAndMissingValues") {
    const auto unknownArgument = [] {
        (void)parse({"render_benchmark", "--unknown", "value"});
    };
    CHECK_THROWS_WITH_AS(unknownArgument(), "Unknown argument: --unknown",
                         std::invalid_argument);

    const auto missingValue = [] {
        (void)parse({"render_benchmark", "--workers"});
    };
    CHECK_THROWS_WITH_AS(missingValue(), "--workers requires a value",
                         std::invalid_argument);
}
