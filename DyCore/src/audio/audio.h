#include <cstdint>
#include <vector>

struct AudioData {
    uint32_t sampleRate = 0;
    uint32_t channels = 0;
    std::vector<float> pcmData;

    size_t get_frame_count() const {
        if (channels == 0)
            return 0;
        return pcmData.size() / channels;
    }
};

// Loads audio data from a file and fills the AudioData structure.
// Returns 0 on success, non-zero on failure.
int load_audio(const char* filePath, AudioData& audioData);
