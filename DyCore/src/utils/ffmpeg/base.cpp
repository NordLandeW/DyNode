#ifdef _WIN32
#include <windows.h>
#else
#include <stdio.h>     // For popen, pclose
#include <sys/wait.h>  // For WEXITSTATUS
#endif

#include "base.h"

/**
 * @brief Check if FFmpeg is available in the system's PATH.
 *
 * @return Returns true if FFmpeg is available, otherwise false.
 */
bool is_FFmpeg_available() {
#ifdef _WIN32
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = NULL;
    si.hStdOutput = NULL;
    si.hStdError = NULL;

    ZeroMemory(&pi, sizeof(pi));

    char cmd[] = "ffmpeg -version";

    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL,
                        NULL, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exit_code == 0;

#else
    // TODO: Implement a safer method
    // FILE* pipe = popen("ffmpeg -version > /dev/null 2>&1", "r");
    // if (!pipe) {
    //     return false;
    // }

    // int status = pclose(pipe);
    // if (WIFEXITED(status)) {
    //     return WEXITSTATUS(status) == 0;
    // }
    return false;
#endif
}