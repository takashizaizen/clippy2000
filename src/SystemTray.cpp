#include "SystemTray.h"

SystemTray::SystemTray()
    : m_hwnd(nullptr)
    , m_isVisible(false)
    , m_callbackMessage(0)
    , m_menuCallback(nullptr)
{
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(m_nid);
}

SystemTray::~SystemTray() {
    Hide();
}

bool SystemTray::Initialize(HWND hwnd, UINT callbackMessage) {
    m_hwnd = hwnd;
    m_callbackMessage = callbackMessage;

    m_nid.hWnd = hwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = callbackMessage;

    // Load default application icon
    m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    wcscpy_s(m_nid.szTip, L"Clippy2000 - Clipboard Manager");

    return true;
}

bool SystemTray::Show() {
    if (m_isVisible) {
        return true;
    }

    if (Shell_NotifyIcon(NIM_ADD, &m_nid)) {
        m_isVisible = true;
        return true;
    }

    return false;
}

bool SystemTray::Hide() {
    if (!m_isVisible) {
        return true;
    }

    if (Shell_NotifyIcon(NIM_DELETE, &m_nid)) {
        m_isVisible = false;
        return true;
    }

    return false;
}

bool SystemTray::SetTooltip(const wchar_t* tooltip) {
    wcscpy_s(m_nid.szTip, tooltip);

    if (m_isVisible) {
        return Shell_NotifyIcon(NIM_MODIFY, &m_nid);
    }

    return true;
}

void SystemTray::OnTrayMessage(WPARAM wParam, LPARAM lParam) {
    if (lParam == WM_RBUTTONUP) {
        ShowContextMenu();
    } else if (lParam == WM_LBUTTONDBLCLK) {
        // Double-click to show history
        if (m_menuCallback) {
            m_menuCallback(ID_SHOW_HISTORY);
        }
    }
}

void SystemTray::SetMenuCallback(MenuCallback callback) {
    m_menuCallback = callback;
}

void SystemTray::ShowContextMenu() {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) {
        return;
    }

    AppendMenu(hMenu, MF_STRING, ID_SHOW_HISTORY, L"Show History");
    AppendMenu(hMenu, MF_STRING, ID_CLEAR_HISTORY, L"Clear History");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, ID_EXIT, L"Exit");

    // Required for the menu to close when clicking outside
    SetForegroundWindow(m_hwnd);

    UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, m_hwnd, NULL);

    if (cmd && m_menuCallback) {
        m_menuCallback(cmd);
    }

    DestroyMenu(hMenu);
}
