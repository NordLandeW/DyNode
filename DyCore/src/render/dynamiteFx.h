#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vector>

// Playback reconstructs judgments from the chart. Editor judgments are
// one-shots owned by actual hit events, independent of the source note.
class DynamiteFx {
   public:
    struct Hit {
        int side;
        double position, width, time;
        bool mixer = false;
    };
    void trigger_hit(Hit hit, double noteTime, bool light);
    void reset();
    size_t prepare(double chartTime, double editorClock, int setting,
                   int mixers, bool playback, bool playing);
    size_t render(char* output, int pass) const;
    double mixer_age(int side) const {
        return mixerAge.at(side - 1);
    }
    size_t capacity_bytes() const;

   private:
    struct Quad {
        int side, region;
        glm::vec2 origin, size;
        double travel;
        glm::u8vec4 color;
    };
    void hit_sample(Hit hit, double age, bool frame, bool light);
    void holding_sample(Hit hit, double age);
    void add_visible(int pass, Quad quad);
    std::array<double, 2> mixerAge{};
    struct EditorHit {
        Hit hit;          // time is the independent clock at emission
        double noteTime;  // only for budget ordering with continuous holds
        bool frame, light;
    };
    std::vector<EditorHit> editorHits;
    // 0: filtered hit sheet; 1: holding sprite above notes; 2: point-sampled
    // light.
    std::array<std::vector<Quad>, 3> passes;
};

DynamiteFx& dynamite_fx();
