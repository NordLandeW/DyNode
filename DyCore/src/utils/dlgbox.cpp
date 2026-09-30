
#include "dlgbox.h"

#include <Windows.h>
#include <atlbase.h>
#include <shobjidl.h>

#include <filesystem>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "DyCore.h"
#include "utils.h"

namespace {

void trim_filename_suffix(std::wstring& filename) {
    const auto end = filename.find_last_not_of(L" .");
    filename.erase(end == std::wstring::npos ? 0 : end + 1);
}

bool is_reserved_filename(std::wstring_view filename) {
    // Device names remain reserved with any extension, including .tar.gz.
    auto stem = filename.substr(0, filename.find(L'.'));
    while (!stem.empty() && stem.back() == L' ') {
        stem.remove_suffix(1);
    }
    std::wstring upper(stem);
    for (auto& ch : upper) {
        if (ch >= L'a' && ch <= L'z') {
            ch -= L'a' - L'A';
        }
    }
    if (upper == L"CON" || upper == L"PRN" || upper == L"AUX" ||
        upper == L"NUL" || upper == L"CONIN$" || upper == L"CONOUT$") {
        return true;
    }
    if (upper.size() != 4 ||
        (upper.substr(0, 3) != L"COM" && upper.substr(0, 3) != L"LPT")) {
        return false;
    }
    const auto digit = upper.back();
    return (digit >= L'1' && digit <= L'9') || digit == L'\u00b9' ||
           digit == L'\u00b2' || digit == L'\u00b3';
}

void truncate_filename(std::wstring& filename) {
    // NTFS limits one component to 255 UTF-16 code units. Keep the final
    // extension and avoid cutting a supplementary Unicode character in half.
    constexpr size_t max_length = 255;
    if (filename.size() <= max_length) {
        return;
    }
    const auto dot = filename.find_last_of(L'.');
    std::wstring extension;
    if (dot != std::wstring::npos && dot > 0 &&
        filename.size() - dot < max_length) {
        extension = filename.substr(dot);
    }
    filename.resize(max_length - extension.size());
    if (!filename.empty() && filename.back() >= 0xd800 &&
        filename.back() <= 0xdbff) {
        filename.pop_back();
    }
    filename += extension;
}

}  // namespace

std::wstring sanitize_save_filename(std::wstring_view filename) {
    std::wstring result(filename);
    for (auto& ch : result) {
        if (ch < 32 || std::wstring_view(L"<>:\"/\\|?*").find(ch) !=
                           std::wstring_view::npos) {
            ch = L'_';
        }
    }
    // Win32 also strips leading ASCII spaces, which can hide a device name.
    const auto begin = result.find_first_not_of(L' ');
    result.erase(0, begin == std::wstring::npos ? result.size() : begin);
    trim_filename_suffix(result);
    truncate_filename(result);
    trim_filename_suffix(result);
    if (result.empty()) {
        return L"example";
    }
    if (is_reserved_filename(result)) {
        result.insert(result.begin(), L'_');
        truncate_filename(result);
    }
    return result;
}

// Helper class for RAII-based COM initialization
class ComInitializer {
   public:
    ComInitializer() {
        m_hr = CoInitializeEx(
            NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    }
    ~ComInitializer() {
        if (SUCCEEDED(m_hr)) {
            CoUninitialize();
        }
    }
    operator HRESULT() const {
        return m_hr;
    }

   private:
    HRESULT m_hr;
};

// Helper to convert UTF-8 string_view to wstring
static std::wstring to_wstring(std::string_view str) {
    if (str.empty()) {
        return std::wstring();
    }
    // Determine the required buffer size for the wide string
    int size_needed =
        MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
    if (size_needed == 0) {
        // This can happen if the input string is invalid UTF-8 or if flags are
        // invalid. For simplicity, returning an empty string. A more robust
        // solution might throw.
        return std::wstring();
    }
    // Allocate the wstring and perform the conversion
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0],
                        size_needed);
    return wstr;
}

/**
 * @brief Opens a modern 'Save File' dialog.
 * @param filter "Description|*.ext1;*.ext2|..."
 * @param default_filename Suggested filename.
 * @param initial_directory Initial directory.
 * @param caption Window title.
 * @return Full path if user clicks 'Save', otherwise std::nullopt.
 * @throws std::runtime_error on system-level errors.
 */
std::optional<std::filesystem::path> get_save_filename(
    std::string_view filter, std::string_view default_filename,
    const std::filesystem::path& initial_directory, std::string_view caption) {
    // Initialize COM
    ComInitializer com_init;
    if (FAILED(com_init)) {
        throw std::runtime_error("Failed to initialize COM library.");
    }

    // Create FileSaveDialog
    CComPtr<IFileSaveDialog> pFileSave;
    HRESULT hr = pFileSave.CoCreateInstance(__uuidof(FileSaveDialog));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create FileSaveDialog instance.");
    }

    // Set options
    if (!caption.empty()) {
        pFileSave->SetTitle(to_wstring(caption).c_str());
    }
    if (!default_filename.empty()) {
        const auto safe_filename =
            sanitize_save_filename(to_wstring(default_filename));
        hr = pFileSave->SetFileName(safe_filename.c_str());
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to set the suggested filename.");
        }
    }
    if (!initial_directory.empty()) {
        CComPtr<IShellItem> pFolderItem;
        hr = SHCreateItemFromParsingName(initial_directory.c_str(), NULL,
                                         IID_PPV_ARGS(&pFolderItem));
        if (SUCCEEDED(hr)) {
            pFileSave->SetDefaultFolder(pFolderItem);
        }
    }

    // Set file filters
    std::vector<COMDLG_FILTERSPEC> filter_specs;
    std::vector<std::wstring> w_filter_parts;  // Keep wstrings alive
    if (!filter.empty()) {
        std::stringstream ss(filter.data());
        std::string part;
        while (std::getline(ss, part, '|')) {
            w_filter_parts.push_back(to_wstring(part));
        }

        if (w_filter_parts.size() % 2 == 0) {
            for (size_t i = 0; i < w_filter_parts.size(); i += 2) {
                filter_specs.push_back(
                    {w_filter_parts[i].c_str(), w_filter_parts[i + 1].c_str()});
            }
            pFileSave->SetFileTypes(static_cast<UINT>(filter_specs.size()),
                                    filter_specs.data());
        }
    }

    // Show dialog
    hr = pFileSave->Show(get_hwnd_handle());

    // Process result
    if (SUCCEEDED(hr)) {
        CComPtr<IShellItem> pItem;
        hr = pFileSave->GetResult(&pItem);
        if (SUCCEEDED(hr)) {
            PWSTR pszFilePath = NULL;
            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

            if (SUCCEEDED(hr)) {
                std::filesystem::path result_path(pszFilePath);
                CoTaskMemFree(pszFilePath);
                return result_path;
            }
        }
    } else if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        return std::nullopt;
    }

    throw std::runtime_error("File dialog failed with an unexpected error.");
}

/**
 * @brief Opens a modern 'Open File' dialog.
 * @param filter "Description|*.ext1;*.ext2|..."
 * @param default_filename Suggested filename.
 * @param initial_directory Initial directory.
 * @param caption Window title.
 * @return Full path if user clicks 'Open', otherwise std::nullopt.
 * @throws std::runtime_error on system-level errors.
 */
std::optional<std::filesystem::path> get_open_filename(
    std::string_view filter, std::string_view default_filename,
    const std::filesystem::path& initial_directory, std::string_view caption) {
    // Initialize COM
    ComInitializer com_init;
    if (FAILED(com_init)) {
        throw std::runtime_error("Failed to initialize COM library.");
    }

    // Create FileOpenDialog
    CComPtr<IFileOpenDialog> pFileOpen;
    HRESULT hr = pFileOpen.CoCreateInstance(__uuidof(FileOpenDialog));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create FileOpenDialog instance.");
    }

    // Set options
    if (!caption.empty()) {
        pFileOpen->SetTitle(to_wstring(caption).c_str());
    }
    if (!default_filename.empty()) {
        pFileOpen->SetFileName(to_wstring(default_filename).c_str());
    }
    if (!initial_directory.empty()) {
        CComPtr<IShellItem> pFolderItem;
        hr = SHCreateItemFromParsingName(initial_directory.c_str(), NULL,
                                         IID_PPV_ARGS(&pFolderItem));
        if (SUCCEEDED(hr)) {
            pFileOpen->SetDefaultFolder(pFolderItem);
        }
    }

    // Set file filters
    std::vector<COMDLG_FILTERSPEC> filter_specs;
    std::vector<std::wstring> w_filter_parts;  // Keep wstrings alive
    if (!filter.empty()) {
        std::stringstream ss(filter.data());
        std::string part;
        while (std::getline(ss, part, '|')) {
            w_filter_parts.push_back(to_wstring(part));
        }

        if (w_filter_parts.size() % 2 == 0) {
            for (size_t i = 0; i < w_filter_parts.size(); i += 2) {
                filter_specs.push_back(
                    {w_filter_parts[i].c_str(), w_filter_parts[i + 1].c_str()});
            }
            pFileOpen->SetFileTypes(static_cast<UINT>(filter_specs.size()),
                                    filter_specs.data());
        }
    }

    // Show dialog
    hr = pFileOpen->Show(get_hwnd_handle());

    // Process result
    if (SUCCEEDED(hr)) {
        CComPtr<IShellItem> pItem;
        hr = pFileOpen->GetResult(&pItem);
        if (SUCCEEDED(hr)) {
            PWSTR pszFilePath = NULL;
            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

            if (SUCCEEDED(hr)) {
                std::filesystem::path result_path(pszFilePath);
                CoTaskMemFree(pszFilePath);
                return result_path;
            }
        }
    } else if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        return std::nullopt;
    }

    throw std::runtime_error("File dialog failed with an unexpected error.");
}

/**
 * @brief Displays a modal "Yes/No" dialog.
 * @param question_text The UTF-8 encoded question to display.
 * @return True for "Yes", false for "No".
 */
bool show_question(std::string_view question_text) {
    std::wstring w_question_text = to_wstring(question_text);

    const wchar_t* caption = L"Question";
    UINT style = MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL;

    int result =
        MessageBoxW(get_hwnd_handle(), w_question_text.c_str(), caption, style);

    return (result == IDYES);
}

#include "resource.h"

struct InputDialogData {
    std::wstring prompt;
    std::wstring default_text;
    std::wstring result;
};

INT_PTR CALLBACK InputDialogProc(HWND hDlg, UINT message, WPARAM wParam,
                                 LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            InputDialogData* pData = reinterpret_cast<InputDialogData*>(lParam);
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pData);

            SetDlgItemTextW(hDlg, IDC_PROMPT_TEXT, pData->prompt.c_str());
            SetDlgItemTextW(hDlg, IDC_EDIT_INPUT, pData->default_text.c_str());

            SHSTOCKICONINFO sii = {};
            sii.cbSize = sizeof(sii);

            HRESULT hr = SHGetStockIconInfo(SIID_INFO, SHGSI_ICON, &sii);

            if (SUCCEEDED(hr)) {
                SendDlgItemMessage(hDlg, IDC_INFO_ICON, STM_SETICON,
                                   (WPARAM)sii.hIcon, 0);
            }

            return (INT_PTR)TRUE;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDOK: {
                    InputDialogData* pData = reinterpret_cast<InputDialogData*>(
                        GetWindowLongPtr(hDlg, DWLP_USER));
                    if (pData) {
                        wchar_t buffer[4096];
                        GetDlgItemTextW(hDlg, IDC_EDIT_INPUT, buffer, 4096);
                        pData->result = buffer;
                    }
                    EndDialog(hDlg, IDOK);
                    return (INT_PTR)TRUE;
                }
                case IDCANCEL: {
                    EndDialog(hDlg, IDCANCEL);
                    return (INT_PTR)TRUE;
                }
            }
            break;
        }
    }
    return (INT_PTR)FALSE;
}

/**
 * @brief Displays a dialog to get a string from the user.
 * @param prompt The UTF-8 message to display.
 * @param default_text The UTF-8 default text for the input box.
 * @return The entered string if OK is pressed, otherwise std::nullopt.
 */
std::optional<std::string> get_string(std::string_view prompt,
                                      std::string_view default_text) {
    InputDialogData data;
    data.prompt = to_wstring(prompt);
    data.default_text = to_wstring(default_text);

    INT_PTR dialogResult = DialogBoxParamW(
        get_hmodule(), MAKEINTRESOURCEW(IDD_INPUT_DIALOG), get_hwnd_handle(),
        InputDialogProc, reinterpret_cast<LPARAM>(&data));

    if (dialogResult == IDOK) {
        return wstringToUtf8(data.result);
    }

    return std::nullopt;
}
