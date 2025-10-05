#pragma once

#include <windows.h>
#include <string>
#include "ClipboardHistory.h"

class ClipboardUtils {
public:
    // Set clipboard text
    static bool SetClipboardText(const std::wstring& text);

    // Get clipboard text
    static std::wstring GetClipboardText();

    // Get clipboard file paths
    static std::wstring GetClipboardFiles();

    // Detect clipboard data type
    static ClipboardDataType GetClipboardDataType();

    // Restore entry to clipboard
    static bool RestoreEntry(const ClipboardEntry& entry);

private:
    // Restore files to clipboard in CF_HDROP format
    static bool RestoreFilesToClipboard(const std::wstring& filePaths);
};
