# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Clippy2000 is a Windows clipboard manager utility written in C++ using the Windows API. It provides clipboard history tracking, search functionality, data persistence, and customizable hotkeys.

## Architecture

The project follows a modular architecture designed for extensibility and testability:

- **Core Modules**: Separate concerns into distinct, loosely-coupled modules
  - `ClipboardMonitor`: Monitors Windows clipboard changes via clipboard chain or format listeners
  - `ClipboardHistory`: Manages in-memory clipboard history with configurable size limits
  - `Storage`: Handles persistence of clipboard data to disk (likely using SQLite or flat files)
  - `SearchEngine`: Provides fast text search across clipboard history
  - `HotkeyManager`: Registers and handles global Windows hotkeys
  - `UI`: User interface layer (system tray, popup window, or main window)

- **Design Patterns**:
  - Use dependency injection to enable unit testing with mock implementations
  - Observer pattern for clipboard change notifications
  - Command pattern for hotkey actions
  - Repository pattern for storage abstraction

- **Platform Integration**:
  - Use Windows API functions: `SetClipboardViewer`, `AddClipboardFormatListener`, or `RegisterHotKey`
  - Handle multiple clipboard formats (CF_TEXT, CF_UNICODETEXT, CF_BITMAP, CF_HDROP for files)
  - Ensure proper COM initialization for clipboard operations

## Build System

Expected build configuration:
- **Compiler**: MSVC (Visual Studio) or MinGW-w64
- **Build Tool**: CMake or Visual Studio solution files
- **Target**: Windows 10+ (64-bit)

Common commands (to be added once build system is configured):
```bash
# Build the project
cmake --build build --config Release

# Run tests
ctest --test-dir build

# Run the application
build/Release/clippy2000.exe
```

## Testing Strategy

- **Unit Tests**: Test individual modules in isolation using mocked dependencies
- **Integration Tests**: Test module interactions (e.g., clipboard monitor → history → storage)
- **Manual Tests**: Test Windows API integration and hotkey registration
- Consider using Google Test (gtest) or Catch2 for C++ testing framework

## Key Implementation Considerations

- **Thread Safety**: Clipboard operations may require marshaling to the main UI thread
- **Memory Management**: Use RAII principles and smart pointers (std::unique_ptr, std::shared_ptr)
- **Error Handling**: Handle Windows API errors gracefully (GetLastError, HRESULT codes)
- **Performance**: Limit clipboard history size and implement efficient search indexing
- **Security**: Sanitize clipboard data, especially when persisting sensitive information
- **Resource Cleanup**: Properly unregister clipboard listeners and hotkeys on shutdown
