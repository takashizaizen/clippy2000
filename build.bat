@echo off
REM Build script for Clippy2000

echo =====================================
echo  Clippy2000 Build Script
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

REM Create build directory if it doesn't exist
if not exist build (
    echo Creating build directory...
    mkdir build
    echo.
)

REM Run CMake configure if needed
if not exist build\CMakeCache.txt (
    echo Configuring CMake...
    cd build
    "C:\Program Files\CMake\bin\cmake.exe" -G "Visual Studio 17 2022" ..
    cd ..
    echo.
)

REM Build the project
echo Building Clippy2000 (Release)...
cd build
"C:\Program Files\CMake\bin\cmake.exe" --build . --config Release
cd ..

if %ERRORLEVEL% EQU 0 (
    echo.
    echo =====================================
    echo  Build Successful!
    echo =====================================
    echo Executable: build\Release\clippy2000.exe
    echo.
) else (
    echo.
    echo =====================================
    echo  Build Failed!
    echo =====================================
    echo.
    exit /b 1
)
