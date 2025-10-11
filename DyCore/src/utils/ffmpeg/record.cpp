#include "record.h"

#include <cctype>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>

#include "utils.h"

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#ifndef DYCORE_POSIX_AVAILABLE
#define DYCORE_POSIX_AVAILABLE 0
#endif

#else

#if defined(__has_include)
#  if __has_include(<unistd.h>) && __has_include(<sys/types.h>) && __has_include(<sys/wait.h>) && __has_include(<fcntl.h>)
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef DYCORE_POSIX_AVAILABLE
#define DYCORE_POSIX_AVAILABLE 1
#endif
#else
#ifndef DYCORE_POSIX_AVAILABLE
#define DYCORE_POSIX_AVAILABLE 0
#endif
#endif
#else
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef DYCORE_POSIX_AVAILABLE
#define DYCORE_POSIX_AVAILABLE 1
#endif
#endif

#endif

// FFmpeg Encoders Availability Detection
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
#if DYCORE_POSIX_AVAILABLE
// Minimal shell-agnostic tokenizer that respects single/double quotes and
// backslash escapes.
static std::vector<std::string> parse_command_to_argv(const std::string& cmd) {
    std::vector<std::string> argv;
    std::string cur;
    enum class State { Normal, InSingle, InDouble };
    State st = State::Normal;
    auto push_cur = [&]() {
        if (!cur.empty()) {
            argv.push_back(cur);
            cur.clear();
        }
    };
    const size_t N = cmd.size();
    for (size_t i = 0; i < N; ++i) {
        char ch = cmd[i];
        if (st == State::Normal) {
            if (std::isspace(static_cast<unsigned char>(ch))) {
                push_cur();
                continue;
            }
            if (ch == '\'') {
                st = State::InSingle;
                continue;
            }
            if (ch == '"') {
                st = State::InDouble;
                continue;
            }
            if (ch == '\\' && i + 1 < N) {
                cur.push_back(cmd[++i]);
                continue;
            }
            cur.push_back(ch);
        } else if (st == State::InSingle) {
            if (ch == '\'') {
                st = State::Normal;
            } else {
                cur.push_back(ch);
            }
        } else {  // InDouble
            if (ch == '"') {
                st = State::Normal;
            } else if (ch == '\\' && i + 1 < N) {
                // Preserve common escapes inside double quotes
                cur.push_back(cmd[++i]);
            } else {
                cur.push_back(ch);
            }
        }
    }
    push_cur();
    return argv;
}

bool run_command_capture_output_utf8(const std::string& cmd,
                                     std::string& output) {
    // Build argv safely without invoking a shell.
    std::vector<std::string> args = parse_command_to_argv(cmd);
    if (args.empty()) {
        return false;
    }

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        return false;
    }

    pid_t pid = fork();
    if (pid == -1) {
        // fork failed
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }

    if (pid == 0) {
        // child: redirect stdout/stderr to pipe write-end
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        // Build argv array
        std::vector<char*> cargv;
        cargv.reserve(args.size() + 1);
        for (auto& s : args) {
            cargv.push_back(const_cast<char*>(s.c_str()));
        }
        cargv.push_back(nullptr);

        execvp(cargv[0], cargv.data());
        _exit(127);
    }

    // parent
    close(pipefd[1]);
    char buffer[4096];
    ssize_t n;
    while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, static_cast<size_t>(n));
    }
    close(pipefd[0]);

    int status = 0;
    (void)waitpid(pid, &status, 0);

    return true;
}
#else
bool run_command_capture_output_utf8(const std::string& cmd,
                                     std::string& output) {
    (void)cmd;
    (void)output;
    print_debug_message(
        "Error: POSIX process APIs are unavailable; cannot execute command "
        "safely.");
    return false;
}
#endif
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
    if (!run_command_capture_output_utf8("ffmpeg -hide_banner -encoders",
                                         out)) {
        print_debug_message(
            "ffmpeg_detect_available_encoders: failed to run ffmpeg -encoders");
        return result;
    }

    std::string lower = out;
    for (char& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    bool has_h264 = false;
    bool has_h265 = false;
    bool has_nvenc = false;
    bool has_intel = false;
    bool has_amd = false;

    // CPU
    if (has_token_word(lower, "libx264"))
        has_h264 = true;
    if (has_token_word(lower, "libx265"))
        has_h265 = true;

    // NVIDIA NVENC
    if (has_token_word(lower, "h264_nvenc")) {
        has_h264 = true;
        has_nvenc = true;
    }
    if (has_token_word(lower, "hevc_nvenc") ||
        has_token_word(lower, "h265_nvenc")) {
        has_h265 = true;
        has_nvenc = true;
    }

    // Intel QSV
    if (has_token_word(lower, "h264_qsv")) {
        has_h264 = true;
        has_intel = true;
    }
    if (has_token_word(lower, "hevc_qsv") ||
        has_token_word(lower, "h265_qsv")) {
        has_h265 = true;
        has_intel = true;
    }

    // AMD AMF
    if (has_token_word(lower, "h264_amf")) {
        has_h264 = true;
        has_amd = true;
    }
    if (has_token_word(lower, "hevc_amf") ||
        has_token_word(lower, "h265_amf")) {
        has_h265 = true;
        has_amd = true;
    }

    if (has_h264)
        result.insert("h264");
    if (has_h265)
        result.insert("h265");
    if (has_nvenc)
        result.insert("nvenc");
    if (has_intel)
        result.insert("intel");
    if (has_amd)
        result.insert("amd");

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
#else
    // POSIX shell (popen uses /bin/sh -c): use single quotes and escape single
    // quotes. 'abc'def'  -> 'abc'\''def'
    auto quote_arg = [](std::string_view s) {
        std::string out;
        out.reserve(s.size() + 2 + (s.size() / 8));
        out.push_back('\'');
        for (char ch : s) {
            if (ch == '\'') {
                out.append("'\\''");
            } else {
                out.push_back(ch);
            }
        }
        out.push_back('\'');
        return out;
    };
#endif

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
}
#ifdef _WIN32
PROCESS_INFORMATION g_ffmpeg_pi{};
#elif DYCORE_POSIX_AVAILABLE
static pid_t g_ffmpeg_pid = -1;
#endif

bool open_ffmpeg_pipe_utf8(FILE*& out, const std::string& cmdUtf8) {
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
#if DYCORE_POSIX_AVAILABLE
    // POSIX: spawn FFmpeg without a shell and connect its stdin via a pipe.
    std::vector<std::string> args = parse_command_to_argv(cmdUtf8);
    if (args.empty()) {
        print_debug_message("Error: Failed to build argv for FFmpeg.");
        return false;
    }

    int pipe_stdin[2];
    if (pipe(pipe_stdin) != 0) {
        print_debug_message("Error: pipe() failed for FFmpeg stdin.");
        return false;
    }

    pid_t pid = fork();
    if (pid == -1) {
        print_debug_message("Error: fork() failed for FFmpeg.");
        close(pipe_stdin[0]);
        close(pipe_stdin[1]);
        return false;
    }

    if (pid == 0) {
        // child
        // Connect read-end of pipe to stdin
        dup2(pipe_stdin[0], STDIN_FILENO);
        close(pipe_stdin[0]);
        close(pipe_stdin[1]);

        // Build argv
        std::vector<char*> cargv;
        cargv.reserve(args.size() + 1);
        for (auto& s : args)
            cargv.push_back(const_cast<char*>(s.c_str()));
        cargv.push_back(nullptr);

        execvp(cargv[0], cargv.data());
        _exit(127);
    }

    // parent
    close(pipe_stdin[0]);
    FILE* f = fdopen(pipe_stdin[1], "wb");
    if (!f) {
        print_debug_message("Error: fdopen failed for FFmpeg stdin.");
        close(pipe_stdin[1]);
        // Ensure child is reaped
        int status = 0;
        (void)waitpid(pid, &status, 0);
        return false;
    }
    // Disable buffering to reduce latency on frame writes.
    setvbuf(f, nullptr, _IONBF, 0);
    out = f;
    g_ffmpeg_pid = pid;
    return true;
#else
    // No POSIX process API available; cannot open FFmpeg pipe safely.
    (void)cmdUtf8;
    out = nullptr;
    print_debug_message(
        "Error: POSIX process APIs are unavailable; cannot start FFmpeg.");
    return false;
#endif
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
}

int Recorder::start_recording(const std::wstring& filename,
                              const std::wstring& musicPath, int width,
                              int height, int fps, double musicOffset) {
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
    if (recording_active) {
        recording_active = false;
        queue_cond.notify_one();
        if (writer_thread.joinable()) {
            writer_thread.join();
        }
    }

    if (!ffmpeg_pipe) {
#ifdef _WIN32
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
#endif
        return;
    }

    fflush(ffmpeg_pipe);
#ifdef _WIN32
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
    fclose(ffmpeg_pipe);
    ffmpeg_pipe = nullptr;
#if DYCORE_POSIX_AVAILABLE
    if (g_ffmpeg_pid > 0) {
        int status = 0;
        (void)waitpid(g_ffmpeg_pid, &status, 0);
        g_ffmpeg_pid = -1;
    }
#endif
#endif
}

std::string Recorder::get_default_encoder() {
    // Selection order: GPU HEVC > GPU H.264 > CPU HEVC > CPU H.264
    std::string enc_out;
    if (!run_command_capture_output_utf8("ffmpeg -hide_banner -encoders",
                                         enc_out)) {
        print_debug_message(
            "Warning: Could not query FFmpeg encoders. Defaulting to libx264.");
        return "libx264";
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

    print_debug_message(
        "Warning: No suitable FFmpeg encoder found. Defaulting to libx264.");
    return "libx264";
}

Recorder& get_recorder() {
    static Recorder recorder;
    return recorder;
}