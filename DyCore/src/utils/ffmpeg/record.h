#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// Handle FFmpeg recording functionalities.
class Recorder {
    FILE* ffmpeg_pipe = nullptr;

    int recWidth = 0;
    int recHeight = 0;
    int recFPS = 0;
    const std::string pixelFormat = "rgba";
    // Encoder options tuned for medium-to-high quality at reasonable speed.
    const std::unordered_map<std::string, std::string> ffmpegEncoderOptions = {
        // CPU
        {"libx264",    "-preset fast -crf 20 "},  // H.264
        {"libx265",    "-preset fast -crf 22 "},  // HEVC

        // NVIDIA NVENC
        {"hevc_nvenc", "-preset p6 -cq 22 "},     // HEVC preferred
        {"h264_nvenc", "-preset p6 -cq 20 "},     // H.264 fallback

        // Intel Quick Sync Video (QSV)
        {"hevc_qsv",   "-preset medium -rc icq -global_quality 23 "},
        {"h264_qsv",   "-preset medium -rc icq -global_quality 21 "},

        // AMD AMF
        {"hevc_amf",   "-rc cqp -qp_i 23 -qp_p 23 -qp_b 23 "},
        {"h264_amf",   "-rc cqp -qp_i 21 -qp_p 21 -qp_b 21 "},

        // Essentials/other CPU fallbacks
        {"libopenh264", "-b:v 5M -g 120 "},       // H.264 via Cisco OpenH264
        {"mpeg4",       "-q:v 5 "},               // MPEG-4 Part 2 (widely available)
    };

    // For async writing
    std::thread writer_thread;
    std::queue<std::vector<char>> frame_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cond;
    bool recording_active = false;

    void writer_worker();

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
    std::string get_default_encoder();
};

Recorder& get_recorder();