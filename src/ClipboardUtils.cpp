#include "ClipboardUtils.h"
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <iostream>
#include <vector>

bool ClipboardUtils::SetClipboardText(const std::wstring& text) {
    if (!OpenClipboard(nullptr)) {
        return false;
    }

    EmptyClipboard();

    size_t size = (text.length() + 1) * sizeof(wchar_t);
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hGlobal) {
        CloseClipboard();
        return false;
    }

    void* pData = GlobalLock(hGlobal);
    if (pData) {
        memcpy(pData, text.c_str(), size);
        GlobalUnlock(hGlobal);
    }

    SetClipboardData(CF_UNICODETEXT, hGlobal);
    CloseClipboard();

    return true;
}

std::wstring ClipboardUtils::GetClipboardText() {
    if (!OpenClipboard(nullptr)) {
        return L"";
    }

    std::wstring text;
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData != nullptr) {
        wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
        if (pszText != nullptr) {
            text = pszText;
            GlobalUnlock(hData);
        }
    }

    CloseClipboard();
    return text;
}

std::wstring ClipboardUtils::GetClipboardFiles() {
    if (!OpenClipboard(nullptr)) {
        return L"";
    }

    std::wstring result;
    HANDLE hData = GetClipboardData(CF_HDROP);
    if (hData != nullptr) {
        HDROP hDrop = static_cast<HDROP>(hData);
        UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);

        for (UINT i = 0; i < fileCount; i++) {
            wchar_t filePath[MAX_PATH];
            if (DragQueryFile(hDrop, i, filePath, MAX_PATH)) {
                if (!result.empty()) {
                    result += L";";
                }
                result += filePath;
            }
        }
    }

    CloseClipboard();
    return result;
}

ClipboardDataType ClipboardUtils::GetClipboardDataType() {
    if (!OpenClipboard(nullptr)) {
        return ClipboardDataType::Text;
    }

    ClipboardDataType type = ClipboardDataType::Text;

    if (IsClipboardFormatAvailable(CF_HDROP)) {
        type = ClipboardDataType::Files;
    } else if (IsClipboardFormatAvailable(CF_BITMAP) || IsClipboardFormatAvailable(CF_DIB)) {
        type = ClipboardDataType::Image;
    } else if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        type = ClipboardDataType::Text;
    }

    CloseClipboard();
    return type;
}

bool ClipboardUtils::RestoreEntry(const ClipboardEntry& entry) {
    switch (entry.type) {
        case ClipboardDataType::Text:
        case ClipboardDataType::Image:
            // Restore as text (images as placeholder text)
            return SetClipboardText(entry.text);

        case ClipboardDataType::Files:
            // Restore files as CF_HDROP format
            return RestoreFilesToClipboard(entry.text);

        default:
            return false;
    }
}

bool ClipboardUtils::RestoreFilesToClipboard(const std::wstring& filePaths) {
    if (!OpenClipboard(nullptr)) {
        std::wcerr << L"Failed to open clipboard for file restore" << std::endl;
        return false;
    }

    EmptyClipboard();

    // Parse semicolon-separated file paths
    std::vector<std::wstring> files;
    std::wstring current;
    for (wchar_t c : filePaths) {
        if (c == L';') {
            if (!current.empty()) {
                files.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        files.push_back(current);
    }

    if (files.empty()) {
        CloseClipboard();
        std::wcerr << L"No files to restore" << std::endl;
        return false;
    }

    std::wcout << L"Restoring " << files.size() << L" file(s) to clipboard" << std::endl;

    // Calculate size needed for DROPFILES structure
    size_t totalSize = sizeof(DROPFILES);
    for (const auto& file : files) {
        totalSize += (file.length() + 1) * sizeof(wchar_t);
    }
    totalSize += sizeof(wchar_t); // Final null terminator

    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, totalSize);
    if (!hGlobal) {
        CloseClipboard();
        return false;
    }

    DROPFILES* pDropFiles = static_cast<DROPFILES*>(GlobalLock(hGlobal));
    if (!pDropFiles) {
        GlobalFree(hGlobal);
        CloseClipboard();
        return false;
    }

    // Fill DROPFILES structure
    pDropFiles->pFiles = sizeof(DROPFILES);
    pDropFiles->pt.x = 0;
    pDropFiles->pt.y = 0;
    pDropFiles->fNC = FALSE;
    pDropFiles->fWide = TRUE; // Unicode

    // Copy file paths
    wchar_t* pFiles = reinterpret_cast<wchar_t*>(reinterpret_cast<BYTE*>(pDropFiles) + sizeof(DROPFILES));
    for (const auto& file : files) {
        wcscpy(pFiles, file.c_str());
        pFiles += file.length() + 1;
    }
    *pFiles = L'\0'; // Double null terminator

    GlobalUnlock(hGlobal);
    SetClipboardData(CF_HDROP, hGlobal);
    CloseClipboard();

    return true;
}
