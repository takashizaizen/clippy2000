#include "ClipboardHistory.h"
#include <algorithm>
#include <cctype>

ClipboardHistory::ClipboardHistory(size_t maxEntries)
    : m_maxEntries(maxEntries)
{
    m_entries.reserve(maxEntries);
}

void ClipboardHistory::AddEntry(const std::wstring& text, ClipboardDataType type) {
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
    m_entries.insert(m_entries.begin(), ClipboardEntry(text, type));

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

std::vector<ClipboardEntry> ClipboardHistory::Search(const std::wstring& query) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ClipboardEntry> results;

    if (query.empty()) {
        return m_entries;
    }

    // Convert query to lowercase for case-insensitive search
    std::wstring lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(),
        [](wchar_t c) { return towlower(c); });

    // Search through entries
    for (const auto& entry : m_entries) {
        // Convert entry text to lowercase
        std::wstring lowerText = entry.text;
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(),
            [](wchar_t c) { return towlower(c); });

        // Check if query is found in the text
        if (lowerText.find(lowerQuery) != std::wstring::npos) {
            results.push_back(entry);
        }
    }

    return results;
}
