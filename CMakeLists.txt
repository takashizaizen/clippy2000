cmake_minimum_required(VERSION 3.15)
project(clippy2000 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/ClipboardMonitor.cpp
    src/ClipboardHistory.cpp
    src/SystemTray.cpp
    src/HotkeyManager.cpp
    src/Storage.cpp
    src/ClipboardUtils.cpp
    src/HistoryWindow.cpp
)

# Create executable
add_executable(clippy2000 ${SOURCES})

# Windows specific settings
if(WIN32)
    target_link_libraries(clippy2000 PRIVATE user32 shell32 comctl32)

    # Force Unicode
    target_compile_definitions(clippy2000 PRIVATE UNICODE _UNICODE)

    # Create Windows GUI application (no console)
    set_target_properties(clippy2000 PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Include directories
target_include_directories(clippy2000 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
