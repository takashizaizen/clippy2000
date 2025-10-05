#include <windows.h>
#include <iostream>
#include <string>
#include "ClipboardMonitor.h"
#include "ClipboardHistory.h"
#include "SystemTray.h"
#include "HotkeyManager.h"
#include "Storage.h"

// Custom message for system tray
#define WM_TRAYICON (WM_USER + 1)

// Global pointers for access in window procedure
ClipboardMonitor* g_monitor = nullptr;
ClipboardHistory* g_history = nullptr;
SystemTray* g_tray = nullptr;
HotkeyManager* g_hotkeyMgr = nullptr;
Storage* g_storage = nullptr;

// Function to get clipboard text
std::wstring GetClipboardText() {
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
        std::wstring clipboardText = GetClipboardText();
        if (!clipboardText.empty()) {
            ClipboardEntry entry(clipboardText);
            g_history->AddEntry(clipboardText);
            g_storage->SaveEntry(entry);
            std::wcout << L"[CLIPBOARD CHANGE] Text: " << clipboardText << std::endl;
            std::wcout << L"[HISTORY] Total entries: " << g_history->GetCount() << std::endl;
        } else {
            std::cout << "[CLIPBOARD CHANGE] Non-text data or empty" << std::endl;
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

        auto entries = g_history->GetEntries();
        int count = 0;
        for (const auto& entry : entries) {
            if (count >= 5) break; // Show only last 5 for now
            std::wcout << L"  " << (count + 1) << L". " << entry.text.substr(0, 50);
            if (entry.text.length() > 50) std::wcout << L"...";
            std::wcout << std::endl;
            count++;
        }
    });

    std::cout << "\nClipboard monitoring active!" << std::endl;
    std::cout << "System tray icon created - right-click for menu" << std::endl;
    std::cout << "Hotkey registered: Ctrl+Shift+V to show history" << std::endl;
    std::cout << "Copy some text to test the monitor..." << std::endl;
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
