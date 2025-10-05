#include "Storage.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

// Simple file-based storage implementation
// TODO: Replace with SQLite for better performance and querying

Storage::Storage(const std::wstring& dbPath)
    : m_dbPath(dbPath)
    , m_db(nullptr)
{
}

Storage::~Storage() {
}

bool Storage::Initialize() {
    // For now, just check if we can create/access the file
    std::wofstream test(m_dbPath, std::ios::app);
    if (!test) {
        std::wcerr << L"Failed to initialize storage at: " << m_dbPath << std::endl;
        return false;
    }
    test.close();

    std::wcout << L"Storage initialized at: " << m_dbPath << std::endl;
    return true;
}

bool Storage::SaveEntry(const ClipboardEntry& entry) {
    std::wofstream file(m_dbPath, std::ios::app);
    if (!file) {
        return false;
    }

    // Get timestamp as epoch
    auto epoch = std::chrono::system_clock::to_time_t(entry.timestamp);

    // Format: timestamp|type|text
    // Escape newlines and pipe characters in text
    std::wstring escapedText = entry.text;
    size_t pos = 0;
    while ((pos = escapedText.find(L'\n', pos)) != std::wstring::npos) {
        escapedText.replace(pos, 1, L"\\n");
        pos += 2;
    }
    pos = 0;
    while ((pos = escapedText.find(L'|', pos)) != std::wstring::npos) {
        escapedText.replace(pos, 1, L"\\p");
        pos += 2;
    }

    file << epoch << L"|" << static_cast<int>(entry.type) << L"|" << escapedText << L"\n";
    file.close();
    return true;
}

std::vector<ClipboardEntry> Storage::LoadEntries(size_t limit) {
    std::vector<ClipboardEntry> entries;
    std::wifstream file(m_dbPath);

    if (!file) {
        return entries;
    }

    std::wstring line;
    while (std::getline(file, line) && entries.size() < limit) {
        // Parse: timestamp|type|text
        size_t firstPipe = line.find(L'|');
        if (firstPipe == std::wstring::npos) {
            continue;
        }

        size_t secondPipe = line.find(L'|', firstPipe + 1);

        ClipboardEntry entry;
        std::wstring timestampStr;
        std::wstring typeStr;
        std::wstring text;

        if (secondPipe == std::wstring::npos) {
            // Old format: timestamp|text (no type)
            timestampStr = line.substr(0, firstPipe);
            text = line.substr(firstPipe + 1);
            entry.type = ClipboardDataType::Text; // Default to text for old entries
        } else {
            // New format: timestamp|type|text
            timestampStr = line.substr(0, firstPipe);
            typeStr = line.substr(firstPipe + 1, secondPipe - firstPipe - 1);
            text = line.substr(secondPipe + 1);

            // Parse type
            try {
                int typeInt = std::stoi(typeStr);
                entry.type = static_cast<ClipboardDataType>(typeInt);
            } catch (...) {
                entry.type = ClipboardDataType::Text;
            }
        }

        // Unescape text
        size_t pos = 0;
        while ((pos = text.find(L"\\n", pos)) != std::wstring::npos) {
            text.replace(pos, 2, L"\n");
            pos += 1;
        }
        pos = 0;
        while ((pos = text.find(L"\\p", pos)) != std::wstring::npos) {
            text.replace(pos, 2, L"|");
            pos += 1;
        }

        entry.text = text;

        // Convert timestamp string to time_point
        try {
            std::time_t epoch = std::stoll(timestampStr);
            entry.timestamp = std::chrono::system_clock::from_time_t(epoch);
        } catch (...) {
            entry.timestamp = std::chrono::system_clock::now();
        }

        entries.push_back(entry);
    }

    file.close();

    // Reverse to get newest first (file has oldest first)
    std::reverse(entries.begin(), entries.end());

    return entries;
}

bool Storage::ClearAll() {
    std::wofstream file(m_dbPath, std::ios::trunc);
    if (!file) {
        return false;
    }
    file.close();
    std::wcout << L"Storage cleared" << std::endl;
    return true;
}

size_t Storage::GetCount() {
    std::wifstream file(m_dbPath);
    if (!file) {
        return 0;
    }

    size_t count = 0;
    std::wstring line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            count++;
        }
    }

    file.close();
    return count;
}

bool Storage::CreateTables() {
    // Not needed for file-based storage
    return true;
}
