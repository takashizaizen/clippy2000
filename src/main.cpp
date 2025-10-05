#include <windows.h>
#include <shellapi.h>
#include "ClipboardMonitor.h"
#include "ClipboardHistory.h"
#include "HotkeyManager.h"
#include "Storage.h"
#include "ClipboardUtils.h"
#include "HistoryWindow.h"
#include "SystemTray.h"

#define WM_TRAYICON (WM_USER + 1)

// Global pointers for access in window procedure
ClipboardMonitor* g_monitor = nullptr;
ClipboardHistory* g_history = nullptr;
HotkeyManager* g_hotkeyMgr = nullptr;
Storage* g_storage = nullptr;
HistoryWindow* g_historyWindow = nullptr;
SystemTray* g_systemTray = nullptr;

// Flag to ignore clipboard changes we caused ourselves
bool g_ignoreNextClipboardChange = false;

// Window procedure to handle messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLIPBOARDUPDATE:
            if (g_monitor) {
                g_monitor->OnClipboardUpdate();
            }
            return 0;
        case WM_HOTKEY:
            if (g_hotkeyMgr) {
                g_hotkeyMgr->OnHotkey(static_cast<int>(wParam));
            }
            return 0;
        case WM_TRAYICON:
            if (g_systemTray) {
                g_systemTray->OnTrayMessage(wParam, lParam);
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // No console output needed for GUI app

    // Create a message-only window for receiving clipboard notifications
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"Clippy2000";

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Failed to register window class", L"Error", MB_OK | MB_ICONERROR);
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
        MessageBox(NULL, L"Failed to create window", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Initialize storage
    Storage storage(L"clippy2000.db");
    g_storage = &storage;

    if (!storage.Initialize()) {
        MessageBox(NULL, L"Failed to initialize storage", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Initialize clipboard history
    ClipboardHistory history(100);
    g_history = &history;

    // Load persisted entries
    auto savedEntries = storage.LoadEntries(100);
    for (const auto& entry : savedEntries) {
        history.AddEntry(entry.text, entry.type);
    }

    // Initialize clipboard monitor
    ClipboardMonitor monitor;
    g_monitor = &monitor;

    if (!monitor.Initialize(hwnd)) {
        MessageBox(NULL, L"Failed to initialize clipboard monitor", L"Error", MB_OK | MB_ICONERROR);
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
                }
                break;

            case ClipboardDataType::Files:
                data = ClipboardUtils::GetClipboardFiles();
                if (!data.empty()) {
                    entry = ClipboardEntry(data, ClipboardDataType::Files);
                    g_history->AddEntry(data, ClipboardDataType::Files);
                    g_storage->SaveEntry(entry);
                }
                break;

            case ClipboardDataType::Image:
                data = L"[Image]";
                entry = ClipboardEntry(data, ClipboardDataType::Image);
                g_history->AddEntry(data, ClipboardDataType::Image);
                g_storage->SaveEntry(entry);
                break;
        }

        // Update GUI if visible
        if (!data.empty() && g_historyWindow) {
            auto entries = g_history->GetEntries();
            g_historyWindow->RefreshIfVisible(entries);
        }
    });

    if (!monitor.Start()) {
        MessageBox(NULL, L"Failed to start clipboard monitoring", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Initialize hotkey manager
    HotkeyManager hotkeyMgr;
    g_hotkeyMgr = &hotkeyMgr;

    if (!hotkeyMgr.Initialize(hwnd)) {
        MessageBox(NULL, L"Failed to initialize hotkey manager", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Initialize GUI history window
    HistoryWindow historyWindow;
    g_historyWindow = &historyWindow;

    if (!historyWindow.Initialize(GetModuleHandle(NULL))) {
        MessageBox(NULL, L"Failed to initialize history window", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Set callback for when user restores an entry from GUI
    historyWindow.SetRestoreCallback([](const ClipboardEntry& entry) {
        g_ignoreNextClipboardChange = true;
        if (!ClipboardUtils::RestoreEntry(entry)) {
            g_ignoreNextClipboardChange = false;
        }
    });

    // Register Ctrl+Shift+V to toggle GUI history window
    hotkeyMgr.RegisterHotkey(MOD_CONTROL | MOD_SHIFT, 'V', []() {
        if (g_historyWindow->IsVisible()) {
            g_historyWindow->Hide();
        } else {
            auto entries = g_history->GetEntries();
            g_historyWindow->UpdateHistory(entries);
            g_historyWindow->Show();
        }
    });

    // Initialize system tray
    SystemTray systemTray;
    g_systemTray = &systemTray;

    if (!systemTray.Initialize(hwnd, WM_TRAYICON)) {
        MessageBox(NULL, L"Failed to initialize system tray", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Set system tray menu callback
    systemTray.SetMenuCallback([](UINT menuId) {
        switch (menuId) {
            case 1001: // ID_SHOW_HISTORY
                if (g_historyWindow->IsVisible()) {
                    g_historyWindow->Hide();
                } else {
                    auto entries = g_history->GetEntries();
                    g_historyWindow->UpdateHistory(entries);
                    g_historyWindow->Show();
                }
                break;
            case 1002: // ID_CLEAR_HISTORY
                g_history->Clear();
                g_storage->ClearAll();
                if (g_historyWindow->IsVisible()) {
                    g_historyWindow->UpdateHistory({});
                }
                break;
            case 1003: // ID_EXIT
                PostQuitMessage(0);
                break;
        }
    });

    systemTray.Show();

    // Show the history window on startup
    auto entries = history.GetEntries();
    historyWindow.UpdateHistory(entries);
    historyWindow.Show();

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    g_monitor = nullptr;
    g_history = nullptr;
    g_hotkeyMgr = nullptr;
    g_storage = nullptr;
    g_historyWindow = nullptr;
    g_systemTray = nullptr;
    return 0;
}
