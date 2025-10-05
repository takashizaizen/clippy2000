#include "HistoryWindow.h"
#include <windowsx.h>
#include <sstream>
#include <algorithm>

#define ID_SEARCH_EDIT 1001
#define ID_LISTVIEW 1002
#define ID_QUIT_BUTTON 1003
#define ID_DIVIDER 1004

HistoryWindow::HistoryWindow()
    : m_hwnd(nullptr)
    , m_searchEdit(nullptr)
    , m_listView(nullptr)
    , m_quitButton(nullptr)
    , m_divider(nullptr)
    , m_hInstance(nullptr)
    , m_boldFont(nullptr)
    , m_bgBrush(nullptr)
    , m_isVisible(false)
    , m_restoreCallback(nullptr)
    , m_oldEditProc(nullptr)
    , m_oldListViewProc(nullptr)
    , m_selectedIndex(0)
{
}

HistoryWindow::~HistoryWindow() {
    if (m_boldFont) {
        DeleteObject(m_boldFont);
    }
    if (m_bgBrush) {
        DeleteObject(m_bgBrush);
    }
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
}

LRESULT CALLBACK HistoryWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HistoryWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<HistoryWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<HistoryWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        switch (uMsg) {
            case WM_CREATE:
                pThis->OnCreate();
                return 0;

            case WM_CLOSE:
                pThis->OnClose();
                return 0;

            case WM_SIZE:
                pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
                return 0;

            case WM_COMMAND:
                if (LOWORD(wParam) == ID_SEARCH_EDIT && HIWORD(wParam) == EN_CHANGE) {
                    pThis->OnSearchTextChanged();
                } else if (LOWORD(wParam) == ID_QUIT_BUTTON && HIWORD(wParam) == STN_CLICKED) {
                    PostQuitMessage(0);
                }
                return 0;

            case WM_NOTIFY: {
                NMHDR* pNmhdr = reinterpret_cast<NMHDR*>(lParam);
                if (pNmhdr->idFrom == ID_LISTVIEW) {
                    if (pNmhdr->code == NM_DBLCLK) {
                        // Get which item was double-clicked
                        NMITEMACTIVATE* pItemActivate = reinterpret_cast<NMITEMACTIVATE*>(lParam);
                        if (pItemActivate->iItem >= 0) {
                            pThis->m_selectedIndex = pItemActivate->iItem;
                        }
                        pThis->OnListDoubleClick();
                    } else if (pNmhdr->code == NM_CLICK || pNmhdr->code == NM_RCLICK) {
                        // Get which item was clicked
                        NMITEMACTIVATE* pItemActivate = reinterpret_cast<NMITEMACTIVATE*>(lParam);
                        if (pItemActivate->iItem >= 0) {
                            pThis->m_selectedIndex = pItemActivate->iItem;
                            InvalidateRect(pThis->m_listView, NULL, TRUE);
                        }
                        // Prevent ListView from changing its own selection
                        ListView_SetItemState(pThis->m_listView, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
                        return TRUE;  // Return TRUE to prevent default processing
                    } else if (pNmhdr->code == LVN_ITEMCHANGING) {
                        // Block all ListView selection changes
                        return TRUE;
                    } else if (pNmhdr->code == NM_CUSTOMDRAW) {
                        NMLVCUSTOMDRAW* pCD = reinterpret_cast<NMLVCUSTOMDRAW*>(lParam);
                        if (pCD->nmcd.dwDrawStage == CDDS_PREPAINT) {
                            return CDRF_NOTIFYITEMDRAW;
                        } else if (pCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                            if ((int)pCD->nmcd.dwItemSpec == pThis->m_selectedIndex) {
                                pCD->clrTextBk = RGB(100, 150, 255);  // Light blue background for selection
                                pCD->clrText = RGB(255, 255, 255);    // White text for selection
                            } else {
                                pCD->clrTextBk = RGB(255, 229, 217);  // Peach background for non-selected
                                pCD->clrText = RGB(0, 0, 0);          // Black text for non-selected
                            }
                            return CDRF_NEWFONT;
                        }
                    }
                }
                return 0;
            }

            case WM_KEYDOWN:
                if (pThis->OnKeyDown(wParam)) {
                    return 0; // Key was handled
                }
                break;

            case WM_CTLCOLOREDIT: {
                // Set search field background to lighter peach
                if (pThis) {
                    HDC hdcEdit = (HDC)wParam;
                    SetBkColor(hdcEdit, RGB(255, 242, 235));  // Lighter peach background
                    SetTextColor(hdcEdit, RGB(0, 0, 0));      // Black text
                    static HBRUSH hbrLightPeach = CreateSolidBrush(RGB(255, 242, 235));
                    return (LRESULT)hbrLightPeach;
                }
                break;
            }

            case WM_CTLCOLORSTATIC: {
                // Set quit button background to match window background
                if (pThis && (HWND)lParam == pThis->m_quitButton) {
                    HDC hdcStatic = (HDC)wParam;
                    SetBkColor(hdcStatic, RGB(255, 229, 217));  // Peach background
                    SetTextColor(hdcStatic, RGB(0, 0, 0));      // Black text
                    return (LRESULT)pThis->m_bgBrush;
                }
                break;
            }

            case WM_NCHITTEST: {
                LRESULT hit = DefWindowProc(hwnd, uMsg, wParam, lParam);
                if (hit == HTCLIENT && pThis) {
                    // Get mouse position relative to window
                    POINT pt;
                    pt.x = GET_X_LPARAM(lParam);
                    pt.y = GET_Y_LPARAM(lParam);
                    ScreenToClient(hwnd, &pt);

                    // Check if mouse is over interactive controls
                    HWND childUnderMouse = ChildWindowFromPoint(hwnd, pt);
                    if (childUnderMouse == hwnd || childUnderMouse == pThis->m_divider) {
                        // Not over any child control, allow dragging
                        return HTCAPTION;
                    }
                }
                return hit;
            }
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK HistoryWindow::EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HistoryWindow* pThis = reinterpret_cast<HistoryWindow*>(GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA));

    if (pThis && uMsg == WM_KEYDOWN) {
        // Check for Shift+Up/Down before other keys
        if (GetKeyState(VK_SHIFT) & 0x8000) {
            if (wParam == VK_UP || wParam == VK_DOWN) {
                if (pThis->OnKeyDown(wParam)) {
                    return 0; // Key was handled
                }
            }
        }
        // Handle other keys (Ctrl+number, ESC, Enter)
        if (pThis->OnKeyDown(wParam)) {
            return 0; // Key was handled
        }
    }

    // Call original edit control procedure
    if (pThis && pThis->m_oldEditProc) {
        return CallWindowProc(pThis->m_oldEditProc, hwnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK HistoryWindow::ListViewSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HistoryWindow* pThis = reinterpret_cast<HistoryWindow*>(GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA));

    if (pThis && uMsg == WM_KEYDOWN) {
        // Handle all keyboard navigation
        if (pThis->OnKeyDown(wParam)) {
            return 0; // Key was handled
        }
    }

    // Call original ListView procedure
    if (pThis && pThis->m_oldListViewProc) {
        return CallWindowProc(pThis->m_oldListViewProc, hwnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool HistoryWindow::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    // Create peach background brush
    m_bgBrush = CreateSolidBrush(RGB(255, 229, 217));

    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = m_bgBrush;
    wc.lpszClassName = L"Clippy2000HistoryWindow";

    if (!RegisterClassEx(&wc)) {
        // Class might already be registered
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }

    // Calculate centered position
    int windowWidth = 600;
    int windowHeight = 400;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    // Create window without title bar, centered on screen
    m_hwnd = CreateWindowEx(
        WS_EX_TOPMOST,
        L"Clippy2000HistoryWindow",
        L"Clippy2000 - Clipboard History",
        WS_POPUP | WS_BORDER,
        x, y,
        windowWidth, windowHeight,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    return m_hwnd != nullptr;
}

void HistoryWindow::OnCreate() {
    CreateControls();
}

void HistoryWindow::CreateControls() {
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // Create search edit box
    m_searchEdit = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        10, 10, 560, 25,
        m_hwnd,
        (HMENU)ID_SEARCH_EDIT,
        m_hInstance,
        nullptr
    );

    // Set placeholder text
    SendMessage(m_searchEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"Search clipboard history...");

    // Subclass the edit control to intercept key presses
    m_oldEditProc = (WNDPROC)SetWindowLongPtr(m_searchEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

    // Create list view without column headers and border
    m_listView = CreateWindowEx(
        0,
        WC_LISTVIEW,
        L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_NOCOLUMNHEADER,
        10, 45, 560, 300,
        m_hwnd,
        (HMENU)ID_LISTVIEW,
        m_hInstance,
        nullptr
    );

    // Set up list view columns (still needed for layout, but headers won't show)
    LVCOLUMN lvc;
    lvc.mask = LVCF_WIDTH;

    lvc.cx = 40;
    ListView_InsertColumn(m_listView, 0, &lvc);

    lvc.cx = 60;
    ListView_InsertColumn(m_listView, 1, &lvc);

    lvc.cx = 440;
    ListView_InsertColumn(m_listView, 2, &lvc);

    // Enable full row select
    ListView_SetExtendedListViewStyle(m_listView, LVS_EX_FULLROWSELECT);

    // Subclass the ListView to intercept key presses
    m_oldListViewProc = (WNDPROC)SetWindowLongPtr(m_listView, GWLP_WNDPROC, (LONG_PTR)ListViewSubclassProc);

    // Set pastel peach background color for ListView
    ListView_SetBkColor(m_listView, RGB(255, 229, 217));  // Soft peach/pastel color
    ListView_SetTextBkColor(m_listView, RGB(255, 229, 217));

    // Create horizontal divider line
    m_divider = CreateWindowEx(
        0,
        L"STATIC",
        L"",
        WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        10, 350, 560, 2,
        m_hwnd,
        (HMENU)ID_DIVIDER,
        m_hInstance,
        nullptr
    );

    // Create bold font for quit text
    m_boldFont = CreateFont(
        16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );

    // Create quit text (styled as a clickable label, left-aligned)
    m_quitButton = CreateWindowEx(
        0,
        L"STATIC",
        L"Quit Application",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOTIFY,
        10, 360, 560, 25,
        m_hwnd,
        (HMENU)ID_QUIT_BUTTON,
        m_hInstance,
        nullptr
    );

    // Apply bold font to quit text
    SendMessage(m_quitButton, WM_SETFONT, (WPARAM)m_boldFont, TRUE);
}

void HistoryWindow::Show() {
    if (m_hwnd) {
        // Clear search filter and display all entries
        SetWindowText(m_searchEdit, L"");
        m_currentFilter = L"";
        m_selectedIndex = 0;
        FilterAndDisplay(m_currentFilter);

        ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
        SetFocus(m_searchEdit);
        m_isVisible = true;
    }
}

void HistoryWindow::Hide() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
        m_isVisible = false;
    }
}

bool HistoryWindow::IsVisible() const {
    return m_isVisible;
}

void HistoryWindow::OnClose() {
    Hide();
}

void HistoryWindow::OnSize(int width, int height) {
    if (m_searchEdit) {
        SetWindowPos(m_searchEdit, nullptr, 10, 10, width - 20, 25, SWP_NOZORDER);
    }
    if (m_listView) {
        SetWindowPos(m_listView, nullptr, 10, 45, width - 20, height - 100, SWP_NOZORDER);
    }
    if (m_divider) {
        SetWindowPos(m_divider, nullptr, 10, height - 50, width - 20, 2, SWP_NOZORDER);
    }
    if (m_quitButton) {
        SetWindowPos(m_quitButton, nullptr, 10, height - 40, width - 20, 25, SWP_NOZORDER);
    }
}

void HistoryWindow::UpdateHistory(const std::vector<ClipboardEntry>& entries) {
    m_allEntries = entries;

    // Get current search text
    wchar_t searchText[256] = {0};
    GetWindowText(m_searchEdit, searchText, 256);
    m_currentFilter = searchText;

    FilterAndDisplay(m_currentFilter);
}

void HistoryWindow::RefreshIfVisible(const std::vector<ClipboardEntry>& entries) {
    if (m_isVisible) {
        UpdateHistory(entries);
    } else {
        // Just update the entries without refreshing display
        m_allEntries = entries;
    }
}

void HistoryWindow::OnSearchTextChanged() {
    wchar_t searchText[256] = {0};
    GetWindowText(m_searchEdit, searchText, 256);
    m_currentFilter = searchText;

    m_selectedIndex = 0; // Reset selection when filter changes
    FilterAndDisplay(m_currentFilter);
}

void HistoryWindow::FilterAndDisplay(const std::wstring& filter) {
    ListView_DeleteAllItems(m_listView);

    std::wstring lowerFilter = filter;
    std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), towlower);

    int displayIndex = 1;
    const int maxItems = 10; // Limit to 10 items

    for (size_t i = 0; i < m_allEntries.size() && displayIndex <= maxItems; i++) {
        const auto& entry = m_allEntries[i];

        // Filter by search text
        if (!lowerFilter.empty()) {
            std::wstring lowerText = entry.text;
            std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), towlower);

            if (lowerText.find(lowerFilter) == std::wstring::npos) {
                continue; // Skip entries that don't match
            }
        }

        // Add item to list view
        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = displayIndex - 1;
        lvi.lParam = i; // Store original index

        // Column 0: Index (1-9, 0 for 10th item)
        wchar_t indexStr[16];
        wsprintf(indexStr, L"%d", displayIndex % 10);
        lvi.pszText = indexStr;
        ListView_InsertItem(m_listView, &lvi);

        // Column 1: Type
        const wchar_t* typeStr = L"TEXT";
        if (entry.type == ClipboardDataType::Files) typeStr = L"FILES";
        else if (entry.type == ClipboardDataType::Image) typeStr = L"IMAGE";
        ListView_SetItemText(m_listView, displayIndex - 1, 1, (LPWSTR)typeStr);

        // Column 2: Content
        std::wstring content = entry.text;
        if (content.length() > 80) {
            content = content.substr(0, 80) + L"...";
        }
        // Replace newlines with spaces
        std::replace(content.begin(), content.end(), L'\n', L' ');
        std::replace(content.begin(), content.end(), L'\r', L' ');

        ListView_SetItemText(m_listView, displayIndex - 1, 2, (LPWSTR)content.c_str());

        displayIndex++;
    }
}

void HistoryWindow::OnListDoubleClick() {
    int itemCount = ListView_GetItemCount(m_listView);
    if (m_selectedIndex >= 0 && m_selectedIndex < itemCount) {
        LVITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = m_selectedIndex;
        ListView_GetItem(m_listView, &lvi);

        int originalIndex = (int)lvi.lParam;
        if (originalIndex >= 0 && originalIndex < (int)m_allEntries.size()) {
            if (m_restoreCallback) {
                m_restoreCallback(m_allEntries[originalIndex]);
            }
            Hide();
        }
    }
}

bool HistoryWindow::OnKeyDown(WPARAM key) {
    if (key == VK_ESCAPE) {
        Hide();
        return true;
    }

    // Check for Shift+Up/Down for navigation
    if (GetKeyState(VK_SHIFT) & 0x8000) {
        if (key == VK_DOWN) {
            int itemCount = ListView_GetItemCount(m_listView);
            if (itemCount > 0) {
                m_selectedIndex = (m_selectedIndex + 1) % itemCount;
                InvalidateRect(m_listView, NULL, TRUE);
                ListView_EnsureVisible(m_listView, m_selectedIndex, FALSE);
            }
            return true;
        } else if (key == VK_UP) {
            int itemCount = ListView_GetItemCount(m_listView);
            if (itemCount > 0) {
                m_selectedIndex = (m_selectedIndex - 1 + itemCount) % itemCount;
                InvalidateRect(m_listView, NULL, TRUE);
                ListView_EnsureVisible(m_listView, m_selectedIndex, FALSE);
            }
            return true;
        }
    }

    // Check for Ctrl+1 through Ctrl+0
    if (GetKeyState(VK_CONTROL) & 0x8000) {
        int itemIndex = -1;

        // Ctrl+1 = item 0, Ctrl+2 = item 1, ..., Ctrl+0 = item 9
        if (key >= '1' && key <= '9') {
            itemIndex = key - '1'; // 1->0, 2->1, ..., 9->8
        } else if (key == '0') {
            itemIndex = 9; // 0 = 10th item
        }

        if (itemIndex != -1 && itemIndex < ListView_GetItemCount(m_listView)) {
            // Get the original entry index from the list item
            LVITEM lvi = {0};
            lvi.mask = LVIF_PARAM;
            lvi.iItem = itemIndex;
            ListView_GetItem(m_listView, &lvi);

            int originalIndex = (int)lvi.lParam;
            if (originalIndex >= 0 && originalIndex < (int)m_allEntries.size()) {
                if (m_restoreCallback) {
                    m_restoreCallback(m_allEntries[originalIndex]);
                }
                Hide();
                return true;
            }
        }
    }

    // Check for Enter to select highlighted item
    if (key == VK_RETURN) {
        int itemCount = ListView_GetItemCount(m_listView);
        if (m_selectedIndex >= 0 && m_selectedIndex < itemCount) {
            LVITEM lvi = {0};
            lvi.mask = LVIF_PARAM;
            lvi.iItem = m_selectedIndex;
            ListView_GetItem(m_listView, &lvi);

            int originalIndex = (int)lvi.lParam;
            if (originalIndex >= 0 && originalIndex < (int)m_allEntries.size()) {
                if (m_restoreCallback) {
                    m_restoreCallback(m_allEntries[originalIndex]);
                }
                Hide();
                return true;
            }
        }
    }

    return false; // Key not handled
}

void HistoryWindow::SetRestoreCallback(RestoreCallback callback) {
    m_restoreCallback = callback;
}

std::wstring HistoryWindow::GetEntryDisplayText(const ClipboardEntry& entry, int index) {
    std::wostringstream oss;
    oss << index << L". ";

    switch (entry.type) {
        case ClipboardDataType::Text:
            oss << L"[TEXT] " << entry.text.substr(0, 60);
            if (entry.text.length() > 60) oss << L"...";
            break;
        case ClipboardDataType::Files:
            oss << L"[FILES] " << entry.text;
            break;
        case ClipboardDataType::Image:
            oss << L"[IMAGE]";
            break;
    }

    return oss.str();
}
