#include "ClipboardMonitor.h"
#include <iostream>

ClipboardMonitor::ClipboardMonitor()
    : m_hwnd(nullptr)
    , m_isMonitoring(false)
    , m_callback(nullptr)
{
}

ClipboardMonitor::~ClipboardMonitor() {
    Stop();
}

bool ClipboardMonitor::Initialize(HWND hwnd) {
    if (!hwnd) {
        std::cerr << "Invalid window handle" << std::endl;
        return false;
    }
    m_hwnd = hwnd;
    return true;
}

bool ClipboardMonitor::Start() {
    if (m_isMonitoring) {
        return true;
    }

    if (!m_hwnd) {
        std::cerr << "Monitor not initialized" << std::endl;
        return false;
    }

    // Use AddClipboardFormatListener (Windows Vista+)
    if (AddClipboardFormatListener(m_hwnd)) {
        m_isMonitoring = true;
        std::cout << "Clipboard monitoring started" << std::endl;
        return true;
    }

    std::cerr << "Failed to add clipboard listener. Error: " << GetLastError() << std::endl;
    return false;
}

void ClipboardMonitor::Stop() {
    if (m_isMonitoring && m_hwnd) {
        RemoveClipboardFormatListener(m_hwnd);
        m_isMonitoring = false;
        std::cout << "Clipboard monitoring stopped" << std::endl;
    }
}

void ClipboardMonitor::SetCallback(ClipboardChangeCallback callback) {
    m_callback = callback;
}

void ClipboardMonitor::OnClipboardUpdate() {
    if (m_callback) {
        m_callback();
    }
}
