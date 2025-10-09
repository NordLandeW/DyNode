#include "record.h"

#include <cstdio>
#include <string>
#include <string_view>

#include "utils.h"

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <fcntl.h>
#include <io.h>

#endif

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

    std::string cmd;
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
    // TODO: Implement a safer method
    // out = popen(cmdUtf8.c_str(), "w");
    return out != nullptr;
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
    pclose(ffmpeg_pipe);
    ffmpeg_pipe = nullptr;
#endif
}

Recorder& get_recorder() {
    static Recorder recorder;
    return recorder;
}