#include "audio.h"

#include <string>

#include "miniaudio.h"
#include "timer.h"
#include "utils.h"

int load_audio(const char* filePath, AudioData& audioData) {
    // TODO: Not implemented yet, just a placeholder using miniaudio to load the
    // audio data.
    return -1;

    TIMER_SCOPE("audio.cpp::load_audio");

    ma_decoder decoder;
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);

    if (ma_decoder_init_file(filePath, &config, &decoder) != MA_SUCCESS) {
        print_debug_message("Failed to open audio file.");
        print_debug_message("filePath: " + std::string(filePath));
        return -1;
    }

    audioData.sampleRate = decoder.outputSampleRate;
    audioData.channels = decoder.outputChannels;

    ma_uint64 totalFrames = 0;
    ma_result lengthResult =
        ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);
    if (lengthResult != MA_SUCCESS) {
        print_debug_message("Failed to get audio length.");
        print_debug_message("filePath: " + std::string(filePath));
        ma_decoder_uninit(&decoder);
        return -2;
    }

    audioData.pcmData.resize(totalFrames * audioData.channels);

    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&decoder, audioData.pcmData.data(), totalFrames,
                               &framesRead);

    if (framesRead < totalFrames) {
        audioData.pcmData.resize(framesRead * audioData.channels);
    }

    ma_decoder_uninit(&decoder);
    return 0;
}
