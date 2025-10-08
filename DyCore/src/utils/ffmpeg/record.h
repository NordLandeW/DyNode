#pragma once

#include <string>
#include <unordered_map>

// Handle FFmpeg recording functionalities.
class Recorder {
    int recWidth = 0;
    int recHeight = 0;
    int recFPS = 0;
    const std::string pixelFormat = "rgba";
    const std::unordered_map<std::string, std::string> ffmpegEncoderOptions = {
        {"libx264", "-preset fast -crf 20 "},
        {"libx265", "-preset fast -crf 22 "},
        {"hevc_nvenc", "-preset p6 -cq 22 "},
        {"h264_nvenc", "-preset p6 -cq 20 "},
    };

    std::string usingEncoder = "hevc_nvenc";

    std::string build_ffmpeg_cmd_utf8(std::string_view pixel_fmt,
                                      std::string_view filename_utf8,
                                      std::string_view musicPath, int width,
                                      int height, int fps, double musicOffset);

   public:
    // Returns 0 on success, negative value on failure.
    int start_recording(const std::string& filename,
                        const std::string& musicPath, const int width,
                        int height, int fps, double musicOffset);
    // Wide-character filename support.
    int start_recording(const std::wstring& filename,
                        const std::wstring& musicPath, int width, int height,
                        int fps, double musicOffset);
    // Push a frame to the recorder.
    void push_frame(const void* frameData, int frameSize);
    // Finish recording.
    void finish_recording();
};

Recorder& get_recorder();