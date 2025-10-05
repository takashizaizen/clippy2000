@echo off
REM Clean script for Clippy2000

echo =====================================
echo  Clippy2000 Clean Script
echo =====================================
echo.

REM Kill any running instances
echo Stopping any running instances...
taskkill /F /IM clippy2000.exe 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Stopped running instance.
) else (
    echo No running instance found.
)
echo.

REM Remove build directory
if exist build (
    echo Removing build directory...
    rmdir /S /Q build
    echo Build directory removed.
) else (
    echo Build directory not found.
)

REM Remove database file
if exist clippy2000.db (
    echo Removing database file...
    del clippy2000.db
    echo Database file removed.
)

echo.
echo =====================================
echo  Clean Complete!
echo =====================================
