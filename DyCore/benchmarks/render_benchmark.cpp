#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "activation.h"
#include "format/dyn.h"
#include "layout.h"
#include "note.h"
#include "notePoolManager.h"
#include "project.h"
#include "render.h"

namespace {

struct BenchmarkOptions {
    size_t noteCount = 20000;
    size_t iterations = 100;
    size_t warmupIterations = 10;
    std::string scenario = "mixed";
    std::string chartPath;
    double noteSpeed = 0.0;
};

struct BenchmarkContext {
    double nowTime = 100.0;
    double noteSpeed = 1000.0;
    size_t sourceNoteCount = 0;
};

struct TimingStats {
    double meanMs = 0.0;
    double medianMs = 0.0;
    double p95Ms = 0.0;
    double minMs = 0.0;
    double maxMs = 0.0;
};

struct NotePoolCleanup {
    ~NotePoolCleanup() {
        get_note_pool_manager().clear_notes();
    }
};

uint64_t fnv1a64(std::span<const char> bytes) {
    uint64_t hash = 14695981039346656037ull;
    for (const auto value : bytes) {
        hash ^= static_cast<unsigned char>(value);
        hash *= 1099511628211ull;
    }
    return hash;
}

TimingStats calculate_stats(std::vector<double> samples) {
    if (samples.empty()) {
        return {};
    }

    std::sort(samples.begin(), samples.end());
    const auto percentile = [&](double value) {
        const size_t index = std::min(
            samples.size() - 1,
            static_cast<size_t>(std::ceil(value * samples.size()) - 1));
        return samples[index];
    };

    return {
        .meanMs = std::accumulate(samples.begin(), samples.end(), 0.0) /
                  static_cast<double>(samples.size()),
        .medianMs = percentile(0.50),
        .p95Ms = percentile(0.95),
        .minMs = samples.front(),
        .maxMs = samples.back(),
    };
}

void print_stats(std::string_view name, const TimingStats& stats) {
    std::cout << std::fixed << std::setprecision(4) << name
              << ".mean_ms=" << stats.meanMs << " median_ms=" << stats.medianMs
              << " p95_ms=" << stats.p95Ms << " min_ms=" << stats.minMs
              << " max_ms=" << stats.maxMs << '\n';
}

SpriteData make_sprite(std::string name, glm::vec2 size, SPRITE_DRAW_TYPE type,
                       std::initializer_list<int> data = {}, int paddingLR = 0,
                       int paddingTop = 0, int paddingBottom = 0) {
    SpriteData sprite{
        .name = std::move(name),
        .size = size,
        .uv0 = {0.0f, 0.0f},
        .uv1 = {1.0f, 1.0f},
        .paddingLR = paddingLR,
        .paddingTop = paddingTop,
        .paddingBottom = paddingBottom,
        .drawSetting = {.type = type, .data = {}},
    };
    std::copy(data.begin(), data.end(), sprite.drawSetting.data);
    sprite.caculate_uv_values();
    return sprite;
}

void initialize_sprites() {
    auto& sprites = get_sprite_manager();
    sprites.add_sprite(make_sprite("sprNote", {45.0f, 28.0f},
                                   SPRITE_DRAW_TYPE::SEG_3, {22, 22}, 30));
    sprites.add_sprite(make_sprite("sprChain", {120.0f, 77.0f},
                                   SPRITE_DRAW_TYPE::SEG_5, {21, 78, 19}, 30));
    sprites.add_sprite(make_sprite("sprHoldEdge", {67.0f, 106.0f},
                                   SPRITE_DRAW_TYPE::SLICE_9, {32, 33, 53, 52},
                                   30, 13, 26));
    sprites.add_sprite(make_sprite("sprHold", {512.0f, 256.0f},
                                   SPRITE_DRAW_TYPE::REPEAT_VERT));
    sprites.add_sprite(
        make_sprite("sprHoldGrey", {512.0f, 256.0f}, SPRITE_DRAW_TYPE::NORMAL));
}

NOTE_TYPE note_type_for(size_t index, size_t count,
                        const std::string& scenario) {
    if (scenario == "normal") {
        return NOTE_TYPE::NORMAL;
    }
    if (scenario == "holds") {
        return NOTE_TYPE::HOLD;
    }
    if (scenario == "clustered") {
        if (index < count * 3 / 5) {
            return NOTE_TYPE::NORMAL;
        }
        if (index < count * 4 / 5) {
            return NOTE_TYPE::CHAIN;
        }
        return NOTE_TYPE::HOLD;
    }

    switch (index % 5) {
        case 0:
            return NOTE_TYPE::HOLD;
        case 1:
            return NOTE_TYPE::CHAIN;
        default:
            return NOTE_TYPE::NORMAL;
    }
}

BenchmarkContext initialize_synthetic_notes(const BenchmarkOptions& options) {
    BenchmarkContext context{
        .nowTime = 100.0,
        .noteSpeed = options.noteSpeed > 0.0 ? options.noteSpeed : 1000.0,
        .sourceNoteCount = options.noteCount,
    };
    auto& notes = get_note_pool_manager();
    for (size_t index = 0; index < options.noteCount; ++index) {
        const NOTE_TYPE type =
            note_type_for(index, options.noteCount, options.scenario);
        const double progress =
            options.noteCount > 1
                ? static_cast<double>(index) /
                      static_cast<double>(options.noteCount - 1)
                : 0.0;

        Note note{
            .side = static_cast<int>(index % 3),
            .type = static_cast<int>(type),
            .time = context.nowTime + progress * 0.70,
            .width = 1.0 + static_cast<double>(index % 5) * 0.25,
            .position = static_cast<double>(index % 6),
            .lastTime = type == NOTE_TYPE::HOLD
                            ? 2.5 + static_cast<double>(index % 7) * 0.25
                            : 0.0,
            .beginTime = 0.0,
            .noteID = std::format("{:09}", index),
            .subNoteID = {},
        };
        if (!notes.create_note(note)) {
            throw std::runtime_error("Failed to create benchmark note");
        }
    }

    auto& activation = get_note_activation_manager();
    activation.set_range(context.nowTime, context.noteSpeed);
    activation.recalculate();
    return context;
}

BenchmarkContext initialize_chart_notes(const BenchmarkOptions& options) {
    Project project;
    if (project_import_dyn(options.chartPath.c_str(), project) != 0 ||
        project.charts.empty()) {
        throw std::runtime_error("Failed to load benchmark chart");
    }

    auto chartNotes = project.charts.front().notes;
    if (chartNotes.empty()) {
        throw std::runtime_error("Benchmark chart does not contain notes");
    }
    std::sort(chartNotes.begin(), chartNotes.end(),
              [](const Note& left, const Note& right) {
                  return left.time < right.time;
              });

    std::array<size_t, 4> typeCounts{};
    for (const auto& note : chartNotes) {
        if (note.type < 0 || note.type >= static_cast<int>(typeCounts.size())) {
            throw std::runtime_error(
                "Benchmark chart contains invalid note type " +
                std::to_string(note.type));
        }
        ++typeCounts[static_cast<size_t>(note.type)];
    }
    std::cout << "chart_notes=" << chartNotes.size()
              << " normal=" << typeCounts[0] << " chain=" << typeCounts[1]
              << " hold=" << typeCounts[2] << " sub=" << typeCounts[3]
              << std::endl;
    if (typeCounts[3] != 0) {
        throw std::runtime_error(
            "Benchmark chart unexpectedly contains serialized sub notes");
    }

    BenchmarkContext context{
        .noteSpeed = options.noteSpeed > 0.0 ? options.noteSpeed : 1.6,
        .sourceNoteCount = chartNotes.size(),
    };

    const double guaranteedActiveWindow =
        std::min(BASE_RES_H - JUDGE_LINE_BELOW_FROM_BOTTOM,
                 BASE_RES_W / 2 - JUDGE_LINE_SIDE_FROM_EDGE) /
        context.noteSpeed;
    size_t bestBegin = 0;
    size_t bestEnd = 0;
    for (size_t begin = 0, end = 0; begin < chartNotes.size(); ++begin) {
        end = std::max(end, begin);
        while (end < chartNotes.size() &&
               chartNotes[end].time <=
                   chartNotes[begin].time + guaranteedActiveWindow) {
            ++end;
        }
        if (end - begin > bestEnd - bestBegin) {
            bestBegin = begin;
            bestEnd = end;
        }
    }
    context.nowTime = chartNotes[bestBegin].time;

    auto& notes = get_note_pool_manager();
    for (size_t index = 0; index < chartNotes.size(); ++index) {
        Note note = chartNotes[index];
        note.noteID = std::format("B{:08}", index);
        note.subNoteID.clear();
        note.beginTime = 0.0;
        if (!notes.create_note(note)) {
            throw std::runtime_error("Failed to create chart benchmark note");
        }
    }

    auto& activation = get_note_activation_manager();
    activation.set_range(context.nowTime, context.noteSpeed);
    activation.recalculate();
    std::cout << "chart_active_notes=" << activation.get_active_notes().size()
              << " active_holds=" << activation.get_active_holds().size()
              << " lasting_holds=" << activation.get_lasting_holds().size()
              << std::endl;
    return context;
}

BenchmarkOptions parse_options(int argc, char** argv) {
    BenchmarkOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        const auto require_value = [&]() -> std::string_view {
            if (++index >= argc) {
                throw std::invalid_argument(std::string(argument) +
                                            " requires a value");
            }
            return argv[index];
        };

        if (argument == "--notes") {
            options.noteCount = std::stoull(std::string(require_value()));
        } else if (argument == "--iterations") {
            options.iterations = std::stoull(std::string(require_value()));
        } else if (argument == "--warmup") {
            options.warmupIterations =
                std::stoull(std::string(require_value()));
        } else if (argument == "--scenario") {
            options.scenario = require_value();
        } else if (argument == "--chart") {
            options.chartPath = require_value();
        } else if (argument == "--speed") {
            options.noteSpeed = std::stod(std::string(require_value()));
        } else {
            throw std::invalid_argument("Unknown argument: " +
                                        std::string(argument));
        }
    }

    if (options.noteCount == 0 || options.iterations == 0) {
        throw std::invalid_argument("notes and iterations must be positive");
    }
    if (options.chartPath.empty() && options.scenario != "normal" &&
        options.scenario != "holds" && options.scenario != "mixed" &&
        options.scenario != "clustered") {
        throw std::invalid_argument(
            "scenario must be normal, holds, mixed, or clustered");
    }
    return options;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        NotePoolCleanup cleanup;
        const BenchmarkOptions options = parse_options(argc, argv);
        initialize_sprites();
        const BenchmarkContext context =
            options.chartPath.empty() ? initialize_synthetic_notes(options)
                                      : initialize_chart_notes(options);

        const size_t vertexBufferBound = get_vertex_buffer_bound();
        std::cout << "vertex_buffer_bound=" << vertexBufferBound << std::endl;
        constexpr size_t guardSize = 4096;
        constexpr char guardValue = static_cast<char>(0xA5);
        std::vector<char> vertexBuffer(vertexBufferBound + guardSize,
                                       guardValue);
        std::array<size_t, 3> outputSizes{};
        std::array<uint64_t, 3> outputHashes{};
        for (const int state : {1, 0, 2}) {
            std::fill(vertexBuffer.begin(), vertexBuffer.end(), guardValue);
            outputSizes[state] = render_active_notes(
                vertexBuffer.data(), context.nowTime, context.noteSpeed, state);
            if (outputSizes[state] > vertexBufferBound) {
                throw std::runtime_error(
                    "Rendered output exceeds reported vertex buffer bound");
            }
            if (std::any_of(vertexBuffer.begin() + vertexBufferBound,
                            vertexBuffer.end(),
                            [](char value) { return value != guardValue; })) {
                throw std::runtime_error(
                    "Rendering wrote beyond reported vertex buffer bound");
            }
            outputHashes[state] = fnv1a64(std::span(
                vertexBuffer.data(), static_cast<size_t>(outputSizes[state])));
        }

        for (size_t iteration = 0; iteration < options.warmupIterations;
             ++iteration) {
            for (const int state : {1, 0, 2}) {
                render_active_notes(vertexBuffer.data(), context.nowTime,
                                    context.noteSpeed, state);
            }
        }

        std::array<std::vector<double>, 3> samples;
        std::vector<double> totalSamples;
        for (auto& stateSamples : samples) {
            stateSamples.reserve(options.iterations);
        }
        totalSamples.reserve(options.iterations);

        using Clock = std::chrono::steady_clock;
        for (size_t iteration = 0; iteration < options.iterations;
             ++iteration) {
            double totalMs = 0.0;
            for (const int state : {1, 0, 2}) {
                const auto begin = Clock::now();
                const size_t outputSize =
                    render_active_notes(vertexBuffer.data(), context.nowTime,
                                        context.noteSpeed, state);
                const auto end = Clock::now();
                if (outputSize != outputSizes[state]) {
                    throw std::runtime_error(
                        "Rendering output size changed during benchmark");
                }
                const double elapsedMs =
                    std::chrono::duration<double, std::milli>(end - begin)
                        .count();
                samples[state].push_back(elapsedMs);
                totalMs += elapsedMs;
            }
            totalSamples.push_back(totalMs);
        }

        const auto& activation = get_note_activation_manager();
        std::cout << "scenario="
                  << (options.chartPath.empty() ? options.scenario : "chart")
                  << " source_notes=" << context.sourceNoteCount
                  << " active_notes=" << activation.get_active_notes().size()
                  << " active_holds=" << activation.get_active_holds().size()
                  << " now_time=" << context.nowTime
                  << " note_speed=" << context.noteSpeed
                  << " iterations=" << options.iterations << '\n';
        for (const int state : {0, 1, 2}) {
            std::cout << "state" << state << ".bytes=" << outputSizes[state]
                      << " hash=0x" << std::hex << outputHashes[state]
                      << std::dec << '\n';
        }
        print_stats("state0", calculate_stats(samples[0]));
        print_stats("state1", calculate_stats(samples[1]));
        print_stats("state2", calculate_stats(samples[2]));
        print_stats("total", calculate_stats(totalSamples));
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "render benchmark failed: " << exception.what() << '\n';
        return 1;
    }
}
