#include <windows.h>
#include <shellapi.h>
#include <iostream>
#include <string>
#include <sstream>
#include <limits>
#include "ClipboardMonitor.h"
#include "ClipboardHistory.h"
#include "SystemTray.h"
#include "HotkeyManager.h"
#include "Storage.h"
#include "ClipboardUtils.h"

// Custom message for system tray
#define WM_TRAYICON (WM_USER + 1)

// Global pointers for access in window procedure
ClipboardMonitor* g_monitor = nullptr;
ClipboardHistory* g_history = nullptr;
SystemTray* g_tray = nullptr;
HotkeyManager* g_hotkeyMgr = nullptr;
Storage* g_storage = nullptr;

// Flag to ignore clipboard changes we caused ourselves
bool g_ignoreNextClipboardChange = false;

// Helper to display an entry
void DisplayEntry(const ClipboardEntry& entry, int index) {
    std::wcout << L"  " << index << L". ";
    switch (entry.type) {
        case ClipboardDataType::Text:
            std::wcout << L"[TEXT] " << entry.text.substr(0, 60);
            if (entry.text.length() > 60) std::wcout << L"...";
            break;
        case ClipboardDataType::Files:
            std::wcout << L"[FILES] " << entry.text;
            break;
        case ClipboardDataType::Image:
            std::wcout << L"[IMAGE]";
            break;
    }
    std::wcout << std::endl;
}

// Window procedure to handle messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLIPBOARDUPDATE:
            if (g_monitor) {
                g_monitor->OnClipboardUpdate();
            }
            return 0;
        case WM_TRAYICON:
            if (g_tray) {
                g_tray->OnTrayMessage(wParam, lParam);
            }
            return 0;
        case WM_HOTKEY:
            if (g_hotkeyMgr) {
                g_hotkeyMgr->OnHotkey(static_cast<int>(wParam));
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main() {
    std::cout << "Clippy2000 - Windows Clipboard Manager" << std::endl;
    std::cout << "Initializing..." << std::endl;

    // Create a message-only window for receiving clipboard notifications
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"Clippy2000";

    if (!RegisterClassEx(&wc)) {
        std::cerr << "Failed to register window class. Error: " << GetLastError() << std::endl;
        return 1;
    }

    // Create message-only window (HWND_MESSAGE parent)
    HWND hwnd = CreateWindowEx(
        0,
        L"Clippy2000",
        L"Clippy2000",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (!hwnd) {
        std::cerr << "Failed to create window. Error: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Window created successfully!" << std::endl;

    // Initialize storage
    Storage storage(L"clippy2000.db");
    g_storage = &storage;

    if (!storage.Initialize()) {
        std::cerr << "Failed to initialize storage" << std::endl;
        return 1;
    }

    // Initialize clipboard history
    ClipboardHistory history(100);
    g_history = &history;

    // Load persisted entries
    auto savedEntries = storage.LoadEntries(100);
    std::cout << "Loaded " << savedEntries.size() << " entries from storage" << std::endl;
    for (const auto& entry : savedEntries) {
        history.AddEntry(entry.text);
    }

    // Initialize clipboard monitor
    ClipboardMonitor monitor;
    g_monitor = &monitor;

    if (!monitor.Initialize(hwnd)) {
        std::cerr << "Failed to initialize clipboard monitor" << std::endl;
        return 1;
    }

    // Set callback to add to history, save to storage, and print
    monitor.SetCallback([]() {
        // Check if we should ignore this change (we caused it ourselves)
        if (g_ignoreNextClipboardChange) {
            g_ignoreNextClipboardChange = false;
            return;
        }

        ClipboardDataType dataType = ClipboardUtils::GetClipboardDataType();
        std::wstring data;
        ClipboardEntry entry;

        switch (dataType) {
            case ClipboardDataType::Text:
                data = ClipboardUtils::GetClipboardText();
                if (!data.empty()) {
                    entry = ClipboardEntry(data, ClipboardDataType::Text);
                    g_history->AddEntry(data, ClipboardDataType::Text);
                    g_storage->SaveEntry(entry);
                    std::wcout << L"[CLIPBOARD CHANGE] Text: " << data.substr(0, 100);
                    if (data.length() > 100) std::wcout << L"...";
                    std::wcout << std::endl;
                }
                break;

            case ClipboardDataType::Files:
                data = ClipboardUtils::GetClipboardFiles();
                if (!data.empty()) {
                    entry = ClipboardEntry(data, ClipboardDataType::Files);
                    g_history->AddEntry(data, ClipboardDataType::Files);
                    g_storage->SaveEntry(entry);
                    std::wcout << L"[CLIPBOARD CHANGE] Files: " << data << std::endl;
                }
                break;

            case ClipboardDataType::Image:
                data = L"[Image]";
                entry = ClipboardEntry(data, ClipboardDataType::Image);
                g_history->AddEntry(data, ClipboardDataType::Image);
                g_storage->SaveEntry(entry);
                std::cout << "[CLIPBOARD CHANGE] Image copied" << std::endl;
                break;
        }

        if (!data.empty()) {
            std::wcout << L"[HISTORY] Total entries: " << g_history->GetCount() << std::endl;
        }
    });

    if (!monitor.Start()) {
        std::cerr << "Failed to start clipboard monitoring" << std::endl;
        return 1;
    }

    // Initialize system tray
    SystemTray tray;
    g_tray = &tray;

    if (!tray.Initialize(hwnd, WM_TRAYICON)) {
        std::cerr << "Failed to initialize system tray" << std::endl;
        return 1;
    }

    // Set tray menu callback
    tray.SetMenuCallback([&](UINT menuId) {
        switch (menuId) {
            case 1001: // ID_SHOW_HISTORY
                std::cout << "Show History clicked" << std::endl;
                std::cout << "Current history entries: " << history.GetCount() << std::endl;
                break;
            case 1002: // ID_CLEAR_HISTORY
                std::cout << "Clear History clicked" << std::endl;
                history.Clear();
                std::cout << "History cleared!" << std::endl;
                break;
            case 1003: // ID_EXIT
                std::cout << "Exit clicked" << std::endl;
                PostQuitMessage(0);
                break;
        }
    });

    if (!tray.Show()) {
        std::cerr << "Failed to show system tray icon" << std::endl;
        return 1;
    }

    // Initialize hotkey manager
    HotkeyManager hotkeyMgr;
    g_hotkeyMgr = &hotkeyMgr;

    if (!hotkeyMgr.Initialize(hwnd)) {
        std::cerr << "Failed to initialize hotkey manager" << std::endl;
        return 1;
    }

    // Register Ctrl+Shift+V to show history
    hotkeyMgr.RegisterHotkey(MOD_CONTROL | MOD_SHIFT, 'V', []() {
        std::cout << "\n[HOTKEY] Ctrl+Shift+V pressed - Show History" << std::endl;
        std::cout << "Current history entries: " << g_history->GetCount() << std::endl;
        std::cout << "To restore an entry, use Ctrl+Shift+R and enter the number" << std::endl;
        std::cout << std::endl;

        auto entries = g_history->GetEntries();
        int index = 1;
        for (const auto& entry : entries) {
            if (index > 10) break; // Show only last 10
            DisplayEntry(entry, index);
            index++;
        }
    });

    // Register Ctrl+Shift+F to search history
    hotkeyMgr.RegisterHotkey(MOD_CONTROL | MOD_SHIFT, 'F', []() {
        std::cout << "\n[HOTKEY] Ctrl+Shift+F pressed - Search History" << std::endl;
        std::cout << "Enter search term (or press Enter to cancel): " << std::flush;

        // Clear input buffer
        std::cin.clear();
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

        std::string queryStr;
        if (!std::getline(std::cin, queryStr) || queryStr.empty()) {
            std::cout << "Search cancelled" << std::endl;
            return;
        }

        // Convert to wstring
        std::wstring query(queryStr.begin(), queryStr.end());

        auto results = g_history->Search(query);
        std::cout << "\nFound " << results.size() << " matching entries:" << std::endl;

        int index = 1;
        for (const auto& entry : results) {
            if (index > 10) break;
            DisplayEntry(entry, index);
            index++;
        }
    });

    // Register Ctrl+Shift+R to restore an entry
    hotkeyMgr.RegisterHotkey(MOD_CONTROL | MOD_SHIFT, 'R', []() {
        std::cout << "\n[HOTKEY] Ctrl+Shift+R pressed - Restore Entry" << std::endl;
        std::cout << "Enter entry number to restore (1-" << g_history->GetCount() << "): " << std::flush;

        // Clear input buffer
        std::cin.clear();
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << "Input cancelled" << std::endl;
            return;
        }

        try {
            int index = std::stoi(input);
            if (index < 1 || index > static_cast<int>(g_history->GetCount())) {
                std::cout << "Invalid entry number" << std::endl;
                return;
            }

            auto entries = g_history->GetEntries();
            const ClipboardEntry& entry = entries[index - 1];

            // Set flag to ignore the clipboard change we're about to make
            g_ignoreNextClipboardChange = true;

            if (ClipboardUtils::RestoreEntry(entry)) {
                std::wcout << L"✓ Restored to clipboard: ";
                DisplayEntry(entry, index);
            } else {
                g_ignoreNextClipboardChange = false; // Reset flag on failure
                std::cout << "Failed to restore entry to clipboard" << std::endl;
            }
        } catch (...) {
            std::cout << "Invalid input" << std::endl;
        }
    });

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Clippy2000 - Clipboard Manager" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nMonitoring: Text, Images, and Files" << std::endl;
    std::cout << "\nHotkeys:" << std::endl;
    std::cout << "  Ctrl+Shift+V - Show clipboard history (last 10 entries)" << std::endl;
    std::cout << "  Ctrl+Shift+F - Search clipboard history" << std::endl;
    std::cout << "  Ctrl+Shift+R - Restore entry by number" << std::endl;
    std::cout << "\nSystem tray icon created - right-click for menu" << std::endl;
    std::cout << "Press Ctrl+C to exit or use tray menu.\n" << std::endl;

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    g_monitor = nullptr;
    g_history = nullptr;
    g_tray = nullptr;
    g_hotkeyMgr = nullptr;
    g_storage = nullptr;
    return 0;
}
