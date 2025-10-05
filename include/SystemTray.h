#pragma once

#include <windows.h>
#include <shellapi.h>
#include <functional>

class SystemTray {
public:
    using MenuCallback = std::function<void(UINT)>;

    SystemTray();
    ~SystemTray();

    // Initialize the system tray icon
    bool Initialize(HWND hwnd, UINT callbackMessage);

    // Show/hide the tray icon
    bool Show();
    bool Hide();

    // Update tooltip text
    bool SetTooltip(const wchar_t* tooltip);

    // Handle tray icon messages
    void OnTrayMessage(WPARAM wParam, LPARAM lParam);

    // Set callback for menu item selections
    void SetMenuCallback(MenuCallback callback);

private:
    HWND m_hwnd;
    NOTIFYICONDATA m_nid;
    bool m_isVisible;
    UINT m_callbackMessage;
    MenuCallback m_menuCallback;

    // Menu IDs
    enum MenuItems {
        ID_SHOW_HISTORY = 1001,
        ID_CLEAR_HISTORY = 1002,
        ID_EXIT = 1003
    };

    void ShowContextMenu();
};
