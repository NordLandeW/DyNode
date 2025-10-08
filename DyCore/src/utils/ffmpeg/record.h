#pragma once

#include <string>

// Handle FFmpeg recording functionalities.
class Recorder {
    int recWidth = 0;
    int recHeight = 0;
    int recFPS = 0;
    const std::string pixelFormat = "rgba";
    const std::string FFmpegCommand =
        "ffmpeg -f rawvideo -pix_fmt %s -s %dx%d -r %d -i - "
        "-c:v libx264 -preset fast -crf 22 -pix_fmt yuv420p "
        "-y %s";

   public:
    // Returns 0 on success, negative value on failure.
    int start_recording(const std::string& filename, int width, int height,
                        int fps);
    // Wide-character filename support.
    int start_recording(const std::wstring& filename, int width, int height,
                        int fps);
    // Push a frame to the recorder.
    void push_frame(const void* frameData, int frameSize);
    // Finish recording.
    void finish_recording();
};

Recorder& get_recorder();