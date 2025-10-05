#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <mutex>

enum class ClipboardDataType {
    Text,
    Image,
    Files
};

struct ClipboardEntry {
    ClipboardDataType type;
    std::wstring text;  // For text or file paths (semicolon-separated)
    std::chrono::system_clock::time_point timestamp;

    ClipboardEntry()
        : type(ClipboardDataType::Text), timestamp(std::chrono::system_clock::now()) {}

    ClipboardEntry(const std::wstring& t, ClipboardDataType dataType = ClipboardDataType::Text)
        : type(dataType), text(t), timestamp(std::chrono::system_clock::now()) {}
};

class ClipboardHistory {
public:
    ClipboardHistory(size_t maxEntries = 100);

    // Add a new clipboard entry
    void AddEntry(const std::wstring& text, ClipboardDataType type = ClipboardDataType::Text);

    // Get all entries (newest first)
    std::vector<ClipboardEntry> GetEntries() const;

    // Get entry count
    size_t GetCount() const;

    // Clear all entries
    void Clear();

    // Set maximum number of entries
    void SetMaxEntries(size_t maxEntries);

    // Search entries by text (case-insensitive)
    std::vector<ClipboardEntry> Search(const std::wstring& query) const;

private:
    std::vector<ClipboardEntry> m_entries;
    size_t m_maxEntries;
    mutable std::mutex m_mutex;

    // Check if entry already exists (avoid duplicates)
    bool IsDuplicate(const std::wstring& text) const;
};
