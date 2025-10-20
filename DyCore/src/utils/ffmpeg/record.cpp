#include "record.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_set>

#include "utils.h"

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#endif

// FFmpeg Encoders Availability Detection and process helpers
namespace {

#ifdef _WIN32
bool run_command_capture_output_utf8(const std::string& cmd,
                                     std::string& output) {
    SECURITY_ATTRIBUTES saAttr{};
    saAttr.nLength = sizeof(saAttr);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hRead = NULL, hWrite = NULL;
    if (!CreatePipe(&hRead, &hWrite, &saAttr, 0)) {
        print_debug_message(
            "run_command_capture_output_utf8: CreatePipe failed.");
        return false;
    }
    if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0)) {
        print_debug_message(
            "run_command_capture_output_utf8: SetHandleInformation failed.");
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return false;
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    PROCESS_INFORMATION pi{};
    std::wstring wcmd = s2ws(cmd);
    std::wstring cmdline = wcmd;

    BOOL ok = CreateProcessW(NULL, cmdline.data(), NULL, NULL, TRUE,
                             CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    CloseHandle(hWrite);

    if (!ok) {
        print_debug_message(
            "run_command_capture_output_utf8: CreateProcessW failed.");
        CloseHandle(hRead);
        return false;
    }

    const DWORD BUF_SIZE = 4096;
    char buffer[BUF_SIZE];
    DWORD bytes = 0;
    for (;;) {
        BOOL success = ReadFile(hRead, buffer, BUF_SIZE, &bytes, NULL);
        if (!success || bytes == 0)
            break;
        output.append(buffer, buffer + bytes);
    }
    CloseHandle(hRead);

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}
#else
bool run_command_capture_output_utf8(const std::string& cmd,
                                     std::string& output) {
    (void)cmd;
    (void)output;
    print_debug_message(
        "Not Implemented: POSIX process APIs are not supported in this build.");
    return false;
}
#endif

bool has_token_word(const std::string& haystack, const std::string& needle) {
    size_t pos = haystack.find(needle);
    while (pos != std::string::npos) {
        bool left_ok =
            (pos == 0) ||
            std::isspace(static_cast<unsigned char>(haystack[pos - 1]));
        size_t end = pos + needle.size();
        bool right_ok = (end >= haystack.size()) ||
                        std::isspace(static_cast<unsigned char>(haystack[end]));
        if (left_ok && right_ok)
            return true;
        pos = haystack.find(needle, pos + 1);
    }
    return false;
}

}  // namespace

// Return available encoders: "h264", "h265", "nvenc", "intel", "amd"
std::unordered_set<std::string> ffmpeg_detect_available_encoders() {
    std::unordered_set<std::string> result;

    std::string out;
    if (!run_command_capture_output_utf8("ffmpeg -hide_banner -encoders", out)) {
        print_debug_message(
            "ffmpeg_detect_available_encoders: failed to run ffmpeg -encoders");
        return result;
    }

    std::string lower = out;
    for (char& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    enum Flags {
        F_H264  = 1 << 0,
        F_H265  = 1 << 1,
        F_NVENC = 1 << 2,
        F_INTEL = 1 << 3,
        F_AMD   = 1 << 4
    };
    int flags = 0;

    struct Tok { const char* token; int set; };
    const Tok toks[] = {
        {"libx264",    F_H264},
        {"libx265",    F_H265},
        {"h264_nvenc", F_H264 | F_NVENC},
        {"hevc_nvenc", F_H265 | F_NVENC},
        {"h265_nvenc", F_H265 | F_NVENC},
        {"h264_qsv",   F_H264 | F_INTEL},
        {"hevc_qsv",   F_H265 | F_INTEL},
        {"h265_qsv",   F_H265 | F_INTEL},
        {"h264_amf",   F_H264 | F_AMD},
        {"hevc_amf",   F_H265 | F_AMD},
        {"h265_amf",   F_H265 | F_AMD},
    };

    for (const auto& t : toks) {
        if (has_token_word(lower, t.token)) {
            flags |= t.set;
        }
    }

    if (flags & F_H264)  result.insert("h264");
    if (flags & F_H265)  result.insert("h265");
    if (flags & F_NVENC) result.insert("nvenc");
    if (flags & F_INTEL) result.insert("intel");
    if (flags & F_AMD)   result.insert("amd");

    return result;
}

std::string Recorder::build_ffmpeg_cmd_utf8(std::string_view pixel_fmt,
                                            std::string_view filename_utf8,
                                            std::string_view musicPath,
                                            int width, int height, int fps,
                                            double musicOffset) {
#ifdef _WIN32
    // Windows: surround with double quotes and escape any embedded quotes.
    auto quote_arg = [](std::string_view s) {
        std::string out;
        out.reserve(s.size() + 2);
        out.push_back('"');
        for (char ch : s) {
            if (ch == '"') {
                out.push_back('\\');
                out.push_back('"');
            } else {
                out.push_back(ch);
            }
        }
        out.push_back('"');
        return out;
    };

    const std::string quoted_filename = quote_arg(filename_utf8);

    std::string cmd, usingEncoder = get_default_encoder();

    print_debug_message("Using Encoder: " + usingEncoder);

    cmd.reserve(256 + quoted_filename.size());
    cmd.append("ffmpeg -f rawvideo -pix_fmt ");
    cmd.append(pixel_fmt);
    cmd.append(" -s ");
    cmd.append(std::to_string(width));
    cmd.append("x");
    cmd.append(std::to_string(height));
    cmd.append(" -r ");
    cmd.append(std::to_string(fps));
    cmd.append(" -i - ");
    if (!musicPath.empty()) {
        cmd.append("-itsoffset ");
        cmd.append(std::to_string(musicOffset) + " ");
        cmd.append("-i ");
        cmd.append(quote_arg(musicPath));
        cmd.append(" -map 0:v -map 1:a -c:a aac -b:a 192k ");
    }
    cmd.append(" -c:v " + usingEncoder + " ");
    cmd.append(ffmpegEncoderOptions.at(usingEncoder));
    cmd.append("-y ");
    cmd.append(quoted_filename);
    return cmd;
#else
    (void)pixel_fmt;
    (void)filename_utf8;
    (void)musicPath;
    (void)width;
    (void)height;
    (void)fps;
    (void)musicOffset;
    print_debug_message("Not Implemented: recording command construction is only supported on Windows.");
    return std::string();
#endif
}

#ifdef _WIN32
PROCESS_INFORMATION g_ffmpeg_pi{};
#endif

static bool open_ffmpeg_pipe_utf8(FILE*& out, const std::string& cmdUtf8) {
#ifdef _WIN32
    // Create a hidden FFmpeg process with its stdin connected to a pipe.
    SECURITY_ATTRIBUTES saAttr{};
    saAttr.nLength = sizeof(saAttr);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hChildStdinRd = NULL;
    HANDLE hChildStdinWr = NULL;

    if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) {
        print_debug_message("Error: CreatePipe failed.");
        return false;
    }

    // Ensure the write end is NOT inherited by the child process.
    if (!SetHandleInformation(hChildStdinWr, HANDLE_FLAG_INHERIT, 0)) {
        print_debug_message("Error: SetHandleInformation failed.");
        CloseHandle(hChildStdinRd);
        CloseHandle(hChildStdinWr);
        return false;
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = hChildStdinRd;
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    PROCESS_INFORMATION pi{};
    std::wstring wcmd = s2ws(cmdUtf8);
    std::wstring cmdline = wcmd;  // CreateProcessW may modify the buffer.

    BOOL ok =
        CreateProcessW(NULL, cmdline.data(), NULL, NULL,
                       TRUE,              // inherit stdin/stdout/stderr handles
                       CREATE_NO_WINDOW,  // do not create a console window
                       NULL, NULL, &si, &pi);

    // Parent is done with these.
    CloseHandle(hChildStdinRd);

    if (!ok) {
        print_debug_message("Error: CreateProcessW for FFmpeg failed.");
        CloseHandle(hChildStdinWr);
        return false;
    }

    // Convert the write-end HANDLE to a FILE* for fwrite.
    int fd =
        _open_osfhandle(reinterpret_cast<intptr_t>(hChildStdinWr), _O_BINARY);
    if (fd == -1) {
        print_debug_message("Error: _open_osfhandle failed.");
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(hChildStdinWr);
        return false;
    }

    FILE* f = _fdopen(fd, "wb");
    if (!f) {
        print_debug_message("Error: _fdopen failed.");
        _close(fd);  // closes underlying handle
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return false;
    }

    out = f;

    // Save process info for graceful teardown in finish_recording.
    g_ffmpeg_pi = pi;

    return true;
#else
    (void)cmdUtf8;
    out = nullptr;
    print_debug_message(
        "Not Implemented: starting FFmpeg process is only supported on Windows.");
    return false;
#endif
}

void Recorder::writer_worker() {
    while (recording_active || !frame_queue.empty()) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cond.wait(
            lock, [this] { return !frame_queue.empty() || !recording_active; });

        if (!frame_queue.empty()) {
            std::vector<char> frame_data = std::move(frame_queue.front());
            frame_queue.pop();
            lock.unlock();

            if (ffmpeg_pipe) {
                size_t written = fwrite(frame_data.data(), 1, frame_data.size(),
                                        ffmpeg_pipe);
                if (written != frame_data.size()) {
                    print_debug_message(
                        "Warning: Incomplete frame write to FFmpeg.");
                }
            }
        }
    }
}

int Recorder::start_recording(const std::string& filename,
                              const std::string& musicPath, int width,
                              int height, int fps, double musicOffset) {
#ifdef _WIN32
    std::string cmd = build_ffmpeg_cmd_utf8(pixelFormat, filename, musicPath,
                                            width, height, fps, musicOffset);
    if (!open_ffmpeg_pipe_utf8(ffmpeg_pipe, cmd)) {
        print_debug_message("Error: Failed to open pipe to FFmpeg.");
        return -1;
    }
    recording_active = true;
    writer_thread = std::thread(&Recorder::writer_worker, this);
    print_debug_message("FFmpeg instantiated successfully. Start recording.");
    return 0;
#else
    (void)filename;
    (void)musicPath;
    (void)width;
    (void)height;
    (void)fps;
    (void)musicOffset;
    print_debug_message("Not Implemented: recording is only supported on Windows.");
    return -1;
#endif
}

int Recorder::start_recording(const std::wstring& filename,
                              const std::wstring& musicPath, int width,
                              int height, int fps, double musicOffset) {
#ifdef _WIN32
    std::string utf8name = wstringToUtf8(filename);
    std::string utf8musicPath = wstringToUtf8(musicPath);
    std::string cmd = build_ffmpeg_cmd_utf8(
        pixelFormat, utf8name, utf8musicPath, width, height, fps, musicOffset);
    if (!open_ffmpeg_pipe_utf8(ffmpeg_pipe, cmd)) {
        print_debug_message("Error: Failed to open pipe to FFmpeg.");
        return -1;
    }
    recording_active = true;
    writer_thread = std::thread(&Recorder::writer_worker, this);
    print_debug_message("FFmpeg instantiated successfully. Start recording.");
    return 0;
#else
    (void)filename;
    (void)musicPath;
    (void)width;
    (void)height;
    (void)fps;
    (void)musicOffset;
    print_debug_message("Not Implemented: recording is only supported on Windows.");
    return -1;
#endif
}

void Recorder::push_frame(const void* frameData, int frameSize) {
    if (!recording_active || !frameData || frameSize <= 0) {
        print_debug_message("Warning: push_frame called with invalid state.");
        return;
    }
    std::vector<char> frame_copy(static_cast<size_t>(frameSize));
    memcpy(frame_copy.data(), frameData, static_cast<size_t>(frameSize));
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        frame_queue.push(std::move(frame_copy));
    }
    queue_cond.notify_one();
}

void Recorder::finish_recording() {
#ifdef _WIN32
    if (recording_active) {
        recording_active = false;
        queue_cond.notify_one();
        if (writer_thread.joinable()) {
            writer_thread.join();
        }
    }

    if (!ffmpeg_pipe) {
        // Even if ffmpeg_pipe is null, ensure process handles are cleaned up.
        print_debug_message(
            "Warning: finish_recording called without active recording.");
        if (g_ffmpeg_pi.hProcess) {
            WaitForSingleObject(g_ffmpeg_pi.hProcess, INFINITE);
            CloseHandle(g_ffmpeg_pi.hThread);
            CloseHandle(g_ffmpeg_pi.hProcess);
            g_ffmpeg_pi.hThread = NULL;
            g_ffmpeg_pi.hProcess = NULL;
        }
        return;
    }

    fflush(ffmpeg_pipe);
    fclose(ffmpeg_pipe);
    ffmpeg_pipe = nullptr;

    if (g_ffmpeg_pi.hProcess) {
        WaitForSingleObject(g_ffmpeg_pi.hProcess, INFINITE);
        CloseHandle(g_ffmpeg_pi.hThread);
        CloseHandle(g_ffmpeg_pi.hProcess);
        g_ffmpeg_pi.hThread = NULL;
        g_ffmpeg_pi.hProcess = NULL;
    }
#else
    // Gracefully stop writer thread if running; no external process management.
    if (recording_active) {
        recording_active = false;
        queue_cond.notify_one();
        if (writer_thread.joinable()) {
            writer_thread.join();
        }
    }
    if (ffmpeg_pipe) {
        fflush(ffmpeg_pipe);
        fclose(ffmpeg_pipe);
        ffmpeg_pipe = nullptr;
    }
    print_debug_message("Not Implemented: finish_recording on non-Windows does not manage FFmpeg process.");
#endif
}

std::string Recorder::get_default_encoder() {
    // Selection order:
    // 1) GPU HEVC  -> hevc_nvenc / hevc_qsv / hevc_amf
    // 2) GPU H.264 -> h264_nvenc / h264_qsv / h264_amf
    // 3) CPU HEVC  -> libx265
    // 4) CPU H.264 -> libx264
    // 5) Essentials/minimal builds -> libopenh264 -> mpeg4
    std::string enc_out;
    if (!run_command_capture_output_utf8("ffmpeg -hide_banner -encoders",
                                         enc_out)) {
        print_debug_message(
            "Warning: Could not query FFmpeg encoders. Defaulting to mpeg4.");
        return "mpeg4";
    }

    std::string lower = enc_out;
    for (char& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    auto has = [&](const char* token) { return has_token_word(lower, token); };

    // Prefer GPU HEVC first
    if (has("hevc_nvenc"))
        return "hevc_nvenc";
    if (has("hevc_qsv"))
        return "hevc_qsv";
    if (has("hevc_amf"))
        return "hevc_amf";

    // GPU H.264 fallbacks
    if (has("h264_nvenc"))
        return "h264_nvenc";
    if (has("h264_qsv"))
        return "h264_qsv";
    if (has("h264_amf"))
        return "h264_amf";

    // CPU HEVC
    if (has("libx265"))
        return "libx265";

    // CPU H.264 fallback
    if (has("libx264"))
        return "libx264";

    // Essentials/other minimal builds
    if (has("libopenh264"))
        return "libopenh264";
    if (has("mpeg4"))
        return "mpeg4";

    print_debug_message(
        "Warning: No suitable FFmpeg encoder found. Defaulting to mpeg4.");
    return "mpeg4";
}

Recorder& get_recorder() {
    static Recorder recorder;
    return recorder;
}