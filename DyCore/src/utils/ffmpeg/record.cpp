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

FILE* ffmpeg_pipe = nullptr;

std::string Recorder::build_ffmpeg_cmd_utf8(std::string_view pixel_fmt,
                                            std::string_view filename_utf8,
                                            int width, int height, int fps) {
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
    cmd.append(" -i - -y ");
    cmd.append(" -c:v " + usingEncoder + " ");
    cmd.append(ffmpegEncoderOptions.at(usingEncoder));
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

int Recorder::start_recording(const std::string& filename, int width,
                              int height, int fps) {
    std::string cmd =
        build_ffmpeg_cmd_utf8(pixelFormat, filename, width, height, fps);
    if (!open_ffmpeg_pipe_utf8(ffmpeg_pipe, cmd)) {
        print_debug_message("Error: Failed to open pipe to FFmpeg.");
        return -1;
    }
    print_debug_message("FFmpeg instantiated successfully. Start recording.");
    return 0;
}

int Recorder::start_recording(const std::wstring& filename, int width,
                              int height, int fps) {
    std::string utf8name = wstringToUtf8(filename);
    std::string cmd =
        build_ffmpeg_cmd_utf8(pixelFormat, utf8name, width, height, fps);
    if (!open_ffmpeg_pipe_utf8(ffmpeg_pipe, cmd)) {
        print_debug_message("Error: Failed to open pipe to FFmpeg.");
        return -1;
    }
    print_debug_message("FFmpeg instantiated successfully. Start recording.");
    return 0;
}

void Recorder::push_frame(const void* frameData, int frameSize) {
    if (!ffmpeg_pipe || !frameData || frameSize <= 0) {
        print_debug_message("Warning: push_frame called with invalid state.");
        return;
    }
    size_t written =
        fwrite(frameData, 1, static_cast<size_t>(frameSize), ffmpeg_pipe);
    if (written != static_cast<size_t>(frameSize)) {
        print_debug_message("Warning: Incomplete frame write to FFmpeg.");
    }
}

void Recorder::finish_recording() {
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
    // ! This instance is not safe. A bug in main gamemaker runner seems will
    // overwrite random memory in this instance and is hard to fix.
    static Recorder recorder;
    return recorder;
}