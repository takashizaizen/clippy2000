#include <windows.h>
#include <iostream>
#include <string>
#include "ClipboardMonitor.h"

// Global pointer to monitor for access in window procedure
ClipboardMonitor* g_monitor = nullptr;

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

    // Initialize clipboard monitor
    ClipboardMonitor monitor;
    g_monitor = &monitor;

    if (!monitor.Initialize(hwnd)) {
        std::cerr << "Failed to initialize clipboard monitor" << std::endl;
        return 1;
    }

    // Set callback to print clipboard content
    monitor.SetCallback([]() {
        std::wstring clipboardText = GetClipboardText();
        if (!clipboardText.empty()) {
            std::wcout << L"[CLIPBOARD CHANGE] Text: " << clipboardText << std::endl;
        } else {
            std::cout << "[CLIPBOARD CHANGE] Non-text data or empty" << std::endl;
        }
    });

    if (!monitor.Start()) {
        std::cerr << "Failed to start clipboard monitoring" << std::endl;
        return 1;
    }

    std::cout << "\nClipboard monitoring active!" << std::endl;
    std::cout << "Copy some text to test the monitor..." << std::endl;
    std::cout << "Press Ctrl+C to exit.\n" << std::endl;

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    g_monitor = nullptr;
    return 0;
}
