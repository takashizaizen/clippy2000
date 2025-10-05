#pragma once

#include "ClipboardHistory.h"
#include <string>
#include <vector>

class Storage {
public:
    Storage(const std::wstring& dbPath = L"clippy2000.db");
    ~Storage();

    // Initialize storage (create tables if needed)
    bool Initialize();

    // Save a clipboard entry
    bool SaveEntry(const ClipboardEntry& entry);

    // Load all entries from storage
    std::vector<ClipboardEntry> LoadEntries(size_t limit = 100);

    // Clear all entries
    bool ClearAll();

    // Get entry count
    size_t GetCount();

private:
    std::wstring m_dbPath;
    void* m_db; // sqlite3* - using void* to avoid including sqlite3.h here

    bool CreateTables();
};
