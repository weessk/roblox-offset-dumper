@echo off
title Offset Dumper 
echo.

if not exist "output" mkdir output
if not exist "src\utils" mkdir src\utils

echo [+] Checkin source files...
if not exist "src\main.cpp" (
    echo [-] Error: src\main.cpp MIA!
    pause
    exit /b 1
)

echo [+] All source files locked n loaded
echo [+] Compilin dumper w gcc...

g++ -std=c++20 -O3 -march=native -static -s ^
    src/main.cpp ^
    src/memory/mem.cpp ^
    src/utils/OutputGenerator.cpp ^
    -o Dumper.exe ^
    -I. ^
    -Iinclude ^
    -Isrc ^
    -lkernel32 -luser32 -lpsapi

if %ERRORLEVEL% == 0 (
    echo.
    echo [+] ====================================
    echo [+] BUILD SUCCESSFUL! LETS GOOO
    echo [+] ====================================
    echo [+] Executable: Dumper.exe
    echo [+] Features: 50+ offsets
    echo [+] Output: HPP/TXT/JSON
    echo [+] ====================================
    echo.
) else (
    echo.
    echo [-] BUILD FAILED!
    echo [-] Check errors above
    echo.
    pause
    exit /b 1
)

echo Press any key to dip...
pause >nul