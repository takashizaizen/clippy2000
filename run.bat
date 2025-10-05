@echo off
REM Run script for Clippy2000

echo Starting Clippy2000...

if not exist build\Release\clippy2000.exe (
    echo Error: Executable not found!
    echo Please run build.bat first.
    exit /b 1
)

start "" "build\Release\clippy2000.exe"
echo Clippy2000 started in background.
echo Press Ctrl+Shift+V to open the clipboard history window.
