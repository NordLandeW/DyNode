#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "activation.h"
#include "capacity.h"
#include "format/dyn.h"
#include "layout.h"
#include "note.h"
#include "notePoolManager.h"
#include "project.h"
#include "render.h"
#include "render_benchmark_options.h"
#include "utils.h"

namespace {

using render_benchmark::BenchmarkOptions;
using render_benchmark::parse_options;

struct BenchmarkContext {
    double nowTime = 100.0;
    double noteSpeed = 1000.0;
    size_t sourceNoteCount = 0;
    double firstTime = 100.0;
    double lastTime = 100.0;
};

struct TimingStats {
    double meanMs = 0.0;
    double medianMs = 0.0;
    double p95Ms = 0.0;
    double p99Ms = 0.0;
    double minMs = 0.0;
    double maxMs = 0.0;
};

struct NotePoolCleanup {
    ~NotePoolCleanup() {
        shutdown_note_rendering();
        get_note_pool_manager().shutdown_executor();
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
        .p99Ms = percentile(0.99),
        .minMs = samples.front(),
        .maxMs = samples.back(),
    };
}

void print_stats(std::string_view name, const TimingStats& stats) {
    std::cout << std::fixed << std::setprecision(4) << name
              << ".mean_ms=" << stats.meanMs << " median_ms=" << stats.medianMs
              << " p95_ms=" << stats.p95Ms << " p99_ms=" << stats.p99Ms
              << " min_ms=" << stats.minMs << " max_ms=" << stats.maxMs << '\n';
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
        context.lastTime = std::max(
            context.lastTime,
            note.time + (type == NOTE_TYPE::HOLD ? note.lastTime : 0.0));
        if (!notes.create_note(note)) {
            throw std::runtime_error("Failed to create benchmark note");
        }
    }

    auto& activation = get_note_activation_manager();
    activation.set_range(context.nowTime, context.noteSpeed);
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
    context.firstTime = chartNotes.front().time;
    context.lastTime = context.firstTime;
    for (const auto& note : chartNotes) {
        context.lastTime = std::max(
            context.lastTime,
            note.time + (note.get_note_type() == NOTE_TYPE::HOLD ? note.lastTime
                                                                 : 0.0));
    }

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
    return context;
}

using Clock = std::chrono::steady_clock;
constexpr size_t GUARD_SIZE = 4096;
constexpr char GUARD_VALUE = static_cast<char>(0xA5);

double elapsed_ms(Clock::time_point begin) {
    return std::chrono::duration<double, std::milli>(Clock::now() - begin)
        .count();
}

struct FrameBuffer {
    std::vector<char> bytes;
    size_t growths = 0;

    void prepare(size_t bound) {
        if (bound > bytes.max_size() - GUARD_SIZE) {
            throw std::length_error("Vertex buffer bound is too large");
        }
        if (reserve_with_headroom(bytes, bound + GUARD_SIZE))
            ++growths;
        bytes.resize(bytes.capacity());
    }
};

struct FrameSample {
    std::array<double, 3> renderMs{};
    std::array<size_t, 3> outputSizes{};
    std::array<uint64_t, 3> outputHashes{};
    double activationMs = 0.0;
    double capacityMs = 0.0;
    size_t activeNotes = 0;
    size_t bound = 0;

    double render_ms() const {
        return std::accumulate(renderMs.begin(), renderMs.end(), 0.0);
    }
    double frame_ms() const {
        return activationMs + capacityMs + render_ms();
    }
};

double time_for_frame(const BenchmarkOptions& options,
                      const BenchmarkContext& context, size_t iteration) {
    const double window = static_cast<double>(BASE_RES_H) / context.noteSpeed;
    if (options.mode == "seek") {
        return iteration % 2 == 0 ? context.firstTime - window * 2
                                  : context.nowTime;
    }
    if (options.mode == "timeline") {
        const double progress =
            options.iterations > 1
                ? static_cast<double>(iteration) /
                      static_cast<double>(options.iterations - 1)
                : 0.0;
        return context.firstTime - window +
               progress * (context.lastTime - context.firstTime + window * 2);
    }
    return context.nowTime;
}

FrameSample render_frame(const BenchmarkContext& context, double time,
                         FrameBuffer& buffer, bool updateActivation,
                         bool hashOutput = false,
                         const std::string& reorderId = {},
                         double reorderedTime = 0.0) {
    FrameSample sample;
    if (updateActivation) {
        const auto begin = Clock::now();
        if (!reorderId.empty()) {
            get_note_pool_manager().access_note(
                reorderId, [&](Note& note) { note.time = reorderedTime; });
        }
        auto& activation = get_note_activation_manager();
        activation.set_range(time, context.noteSpeed);
        activation.recalculate();
        sample.activationMs = elapsed_ms(begin);
    }
    sample.activeNotes =
        get_note_activation_manager().get_active_notes().size();
    const auto prepareBegin = Clock::now();
    sample.bound = prepare_note_rendering();
    buffer.prepare(sample.bound);
    sample.capacityMs = elapsed_ms(prepareBegin);
    for (const int state : {1, 0, 2}) {
        auto guard = std::span(buffer.bytes).subspan(sample.bound, GUARD_SIZE);
        std::fill(guard.begin(), guard.end(), GUARD_VALUE);
        const auto begin = Clock::now();
        sample.outputSizes[state] = render_active_notes(
            buffer.bytes.data(), time, context.noteSpeed, state);
        sample.renderMs[state] = elapsed_ms(begin);
        if (sample.outputSizes[state] > sample.bound ||
            std::any_of(guard.begin(), guard.end(),
                        [](char value) { return value != GUARD_VALUE; })) {
            throw std::runtime_error(
                "Rendering exceeded the reported vertex buffer bound");
        }
        if (hashOutput) {
            sample.outputHashes[state] = fnv1a64(
                std::span(buffer.bytes.data(), sample.outputSizes[state]));
        }
    }
    return sample;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const BenchmarkOptions options = parse_options(argc, argv);
        NotePoolCleanup cleanup;
        const auto setupBegin = Clock::now();
        set_render_worker_count_override(options.workerCount);
        initialize_sprites();
        get_note_pool_manager().initialize_executor();
        initialize_note_rendering();
        const BenchmarkContext context =
            options.chartPath.empty() ? initialize_synthetic_notes(options)
                                      : initialize_chart_notes(options);
        const double setupMs = elapsed_ms(setupBegin);
        FrameBuffer vertexBuffer;
        const double firstTime = time_for_frame(options, context, 0);
        const auto cold =
            render_frame(context, firstTime, vertexBuffer, true, true);
        const auto outputSizes = cold.outputSizes;
        const auto outputHashes = cold.outputHashes;

        std::string reorderId;
        double reorderTime = 0.0;
        if (options.mode == "reorder") {
            auto& notes = get_note_pool_manager();
            for (int i = 0; i < notes.get_note_count(); ++i) {
                const auto& note = notes.get_note_direct(i);
                if (note.get_note_type() == NOTE_TYPE::NORMAL ||
                    note.get_note_type() == NOTE_TYPE::CHAIN) {
                    reorderId = note.noteID;
                    reorderTime = note.time;
                    break;
                }
            }
            if (reorderId.empty()) {
                throw std::invalid_argument(
                    "reorder mode requires a NORMAL or CHAIN note");
            }
        }
        // Dynamic modes warm only their sparse starting point. Pre-warming
        // the dense interval would hide the high-water growth being measured.
        for (size_t iteration = 0; iteration < options.warmupIterations;
             ++iteration) {
            render_frame(context, firstTime, vertexBuffer,
                         options.mode != "steady");
        }
        const auto before = get_note_rendering_stats();
        const size_t bufferGrowthsBefore = vertexBuffer.growths;
        const size_t noteExecutorsBefore =
            get_note_pool_manager().executor_creation_count();
        std::array<std::vector<double>, 3> samples;
        std::vector<double> totalSamples, activationSamples, capacitySamples,
            frameSamples, growingFrameSamples;
        for (auto& stateSamples : samples)
            stateSamples.reserve(options.iterations);
        for (auto* values :
             {&totalSamples, &activationSamples, &capacitySamples,
              &frameSamples, &growingFrameSamples}) {
            values->reserve(options.iterations);
        }
        size_t minActive = (std::numeric_limits<size_t>::max)(), maxActive = 0;
        for (size_t iteration = 0; iteration < options.iterations;
             ++iteration) {
            const auto growths = get_note_rendering_stats().capacityGrowths +
                                 vertexBuffer.growths;
            const double mutationTime =
                reorderTime +
                (iteration % 2 == 0 ? 1.0 : -1.0) / context.noteSpeed;
            const auto sample = render_frame(
                context, time_for_frame(options, context, iteration),
                vertexBuffer, options.mode != "steady", false, reorderId,
                mutationTime);
            if (options.mode == "steady" && sample.outputSizes != outputSizes) {
                throw std::runtime_error(
                    "Rendering output size changed during steady benchmark");
            }
            for (const int state : {0, 1, 2})
                samples[state].push_back(sample.renderMs[state]);
            totalSamples.push_back(sample.render_ms());
            activationSamples.push_back(sample.activationMs);
            capacitySamples.push_back(sample.capacityMs);
            frameSamples.push_back(sample.frame_ms());
            if (get_note_rendering_stats().capacityGrowths +
                    vertexBuffer.growths !=
                growths) {
                growingFrameSamples.push_back(sample.frame_ms());
            }
            minActive = std::min(minActive, sample.activeNotes);
            maxActive = std::max(maxActive, sample.activeNotes);
        }

        const auto& activation = get_note_activation_manager();
        const size_t availableWorkerCount =
            static_cast<size_t>(std::max(1, hardware_concurrency()));
        const size_t configuredWorkerCount =
            options.workerCount == 0
                ? availableWorkerCount
                : std::clamp(options.workerCount, size_t{1},
                             availableWorkerCount);
        std::cout << "scenario="
                  << (options.chartPath.empty() ? options.scenario : "chart")
                  << " source_notes=" << context.sourceNoteCount
                  << " active_notes=" << activation.get_active_notes().size()
                  << " active_holds=" << activation.get_active_holds().size()
                  << " now_time=" << context.nowTime
                  << " note_speed=" << context.noteSpeed
                  << " workers=" << configuredWorkerCount
                  << " iterations=" << options.iterations << '\n';
        const auto after = get_note_rendering_stats();
        std::cout << "mode=" << options.mode
                  << " timing_scope=cpu_stages_only_no_GML_or_GPU"
                  << " growth_counters=workspace_and_caller_buffer_not_all_"
                     "allocations\n"
                  << "setup_ms=" << setupMs
                  << " cold.activation_ms=" << cold.activationMs
                  << " cold.capacity_ms=" << cold.capacityMs
                  << " cold.render_ms=" << cold.render_ms()
                  << " cold.frame_cpu_ms=" << cold.frame_ms() << '\n'
                  << "active_notes.min=" << minActive << " max=" << maxActive
                  << '\n'
                  << "workspace.growths.measured="
                  << after.capacityGrowths - before.capacityGrowths
                  << " capacity_bytes=" << after.workspaceCapacityBytes << '\n'
                  << "vertex_buffer.growths.measured="
                  << vertexBuffer.growths - bufferGrowthsBefore
                  << " capacity_bytes=" << vertexBuffer.bytes.capacity() << '\n'
                  << "render_executor.creations.measured="
                  << after.executorCreations - before.executorCreations
                  << " total=" << after.executorCreations << '\n'
                  << "note_executor.creations.measured="
                  << get_note_pool_manager().executor_creation_count() -
                         noteExecutorsBefore
                  << " total="
                  << get_note_pool_manager().executor_creation_count() << '\n'
                  << "task_submissions.measured="
                  << after.taskSubmissions - before.taskSubmissions << '\n'
                  << "growth_frames=" << growingFrameSamples.size()
                  << " peak_frame="
                  << std::distance(frameSamples.begin(),
                                   std::max_element(frameSamples.begin(),
                                                    frameSamples.end()))
                  << '\n';
        for (const int state : {0, 1, 2}) {
            std::cout << "state" << state << ".bytes=" << outputSizes[state]
                      << " hash=0x" << std::hex << outputHashes[state]
                      << std::dec << '\n';
        }
        print_stats("state0", calculate_stats(samples[0]));
        print_stats("state1", calculate_stats(samples[1]));
        print_stats("state2", calculate_stats(samples[2]));
        print_stats("total", calculate_stats(totalSamples));
        print_stats("activation", calculate_stats(activationSamples));
        print_stats("capacity", calculate_stats(capacitySamples));
        print_stats("frame_cpu", calculate_stats(frameSamples));
        if (!growingFrameSamples.empty())
            print_stats("growing_frame_cpu",
                        calculate_stats(growingFrameSamples));
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "render benchmark failed: " << exception.what() << '\n';
        return 1;
    }
}
