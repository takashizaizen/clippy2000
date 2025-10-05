#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <functional>
#include "ClipboardHistory.h"

class HistoryWindow {
public:
    using RestoreCallback = std::function<void(const ClipboardEntry&)>;

    HistoryWindow();
    ~HistoryWindow();

    // Initialize and create the window
    bool Initialize(HINSTANCE hInstance);

    // Show/hide the window
    void Show();
    void Hide();
    bool IsVisible() const;

    // Update the history list
    void UpdateHistory(const std::vector<ClipboardEntry>& entries);

    // Refresh the display if window is visible
    void RefreshIfVisible(const std::vector<ClipboardEntry>& entries);

    // Set callback for when user wants to restore an entry
    void SetRestoreCallback(RestoreCallback callback);

private:
    HWND m_hwnd;
    HWND m_searchEdit;
    HWND m_listView;
    HINSTANCE m_hInstance;
    bool m_isVisible;
    RestoreCallback m_restoreCallback;
    std::vector<ClipboardEntry> m_allEntries;
    std::wstring m_currentFilter;
    WNDPROC m_oldEditProc;

    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Handle messages
    void OnCreate();
    void OnClose();
    void OnSize(int width, int height);
    void OnSearchTextChanged();
    void OnListDoubleClick();
    bool OnKeyDown(WPARAM key);

    // Helper functions
    void CreateControls();
    void UpdateListView();
    void FilterAndDisplay(const std::wstring& filter);
    std::wstring GetEntryDisplayText(const ClipboardEntry& entry, int index);
};
