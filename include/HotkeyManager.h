#pragma once

#include <windows.h>
#include <functional>
#include <unordered_map>

class HotkeyManager {
public:
    using HotkeyCallback = std::function<void()>;

    HotkeyManager();
    ~HotkeyManager();

    // Initialize with window handle
    bool Initialize(HWND hwnd);

    // Register a hotkey (returns hotkey ID on success, 0 on failure)
    // modifiers: MOD_ALT, MOD_CONTROL, MOD_SHIFT, MOD_WIN
    int RegisterHotkey(UINT modifiers, UINT vk, HotkeyCallback callback);

    // Unregister a hotkey by ID
    bool UnregisterHotkey(int hotkeyId);

    // Unregister all hotkeys
    void UnregisterAll();

    // Handle WM_HOTKEY message
    void OnHotkey(int hotkeyId);

private:
    HWND m_hwnd;
    int m_nextId;
    std::unordered_map<int, HotkeyCallback> m_callbacks;
};
