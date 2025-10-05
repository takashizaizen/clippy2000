#pragma once

#include <windows.h>
#include <functional>

class ClipboardMonitor {
public:
    using ClipboardChangeCallback = std::function<void()>;

    ClipboardMonitor();
    ~ClipboardMonitor();

    // Initialize the monitor with a window handle
    bool Initialize(HWND hwnd);

    // Start/Stop monitoring
    bool Start();
    void Stop();

    // Set callback for clipboard changes
    void SetCallback(ClipboardChangeCallback callback);

    // Handle clipboard update message
    void OnClipboardUpdate();

private:
    HWND m_hwnd;
    bool m_isMonitoring;
    ClipboardChangeCallback m_callback;
};
