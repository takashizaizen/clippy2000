#include "HotkeyManager.h"
#include <iostream>

HotkeyManager::HotkeyManager()
    : m_hwnd(nullptr)
    , m_nextId(1)
{
}

HotkeyManager::~HotkeyManager() {
    UnregisterAll();
}

bool HotkeyManager::Initialize(HWND hwnd) {
    if (!hwnd) {
        std::cerr << "Invalid window handle" << std::endl;
        return false;
    }
    m_hwnd = hwnd;
    return true;
}

int HotkeyManager::RegisterHotkey(UINT modifiers, UINT vk, HotkeyCallback callback) {
    if (!m_hwnd) {
        std::cerr << "HotkeyManager not initialized" << std::endl;
        return 0;
    }

    int id = m_nextId++;

    if (::RegisterHotKey(m_hwnd, id, modifiers, vk)) {
        m_callbacks[id] = callback;
        std::cout << "Hotkey registered with ID: " << id << std::endl;
        return id;
    }

    std::cerr << "Failed to register hotkey. Error: " << GetLastError() << std::endl;
    return 0;
}

bool HotkeyManager::UnregisterHotkey(int hotkeyId) {
    if (!m_hwnd) {
        return false;
    }

    auto it = m_callbacks.find(hotkeyId);
    if (it != m_callbacks.end()) {
        if (::UnregisterHotKey(m_hwnd, hotkeyId)) {
            m_callbacks.erase(it);
            std::cout << "Hotkey unregistered: " << hotkeyId << std::endl;
            return true;
        }
    }

    return false;
}

void HotkeyManager::UnregisterAll() {
    if (!m_hwnd) {
        return;
    }

    for (const auto& pair : m_callbacks) {
        ::UnregisterHotKey(m_hwnd, pair.first);
    }

    m_callbacks.clear();
    std::cout << "All hotkeys unregistered" << std::endl;
}

void HotkeyManager::OnHotkey(int hotkeyId) {
    auto it = m_callbacks.find(hotkeyId);
    if (it != m_callbacks.end() && it->second) {
        it->second();
    }
}
