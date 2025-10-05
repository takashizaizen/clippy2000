#include "ClipboardHistory.h"
#include <algorithm>

ClipboardHistory::ClipboardHistory(size_t maxEntries)
    : m_maxEntries(maxEntries)
{
    m_entries.reserve(maxEntries);
}

void ClipboardHistory::AddEntry(const std::wstring& text) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Ignore empty text
    if (text.empty()) {
        return;
    }

    // Check for duplicate (don't add if same as most recent)
    if (IsDuplicate(text)) {
        return;
    }

    // Add new entry at the beginning (newest first)
    m_entries.insert(m_entries.begin(), ClipboardEntry(text));

    // Remove oldest entry if we exceed max size
    if (m_entries.size() > m_maxEntries) {
        m_entries.pop_back();
    }
}

std::vector<ClipboardEntry> ClipboardHistory::GetEntries() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entries;
}

size_t ClipboardHistory::GetCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entries.size();
}

void ClipboardHistory::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}

void ClipboardHistory::SetMaxEntries(size_t maxEntries) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxEntries = maxEntries;

    // Trim if current size exceeds new max
    if (m_entries.size() > m_maxEntries) {
        m_entries.resize(m_maxEntries);
    }
}

bool ClipboardHistory::IsDuplicate(const std::wstring& text) const {
    // Check if the text matches the most recent entry
    if (!m_entries.empty() && m_entries.front().text == text) {
        return true;
    }
    return false;
}
