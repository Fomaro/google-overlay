@echo off
echo ==============================================
echo Building native Google Overlay (C++ / Win32)
echo ==============================================

set GCC_PATH=C:\Users\vinog\scoop\apps\gcc\current\bin

if exist "%GCC_PATH%\g++.exe" (
    set CXX="%GCC_PATH%\g++.exe"
    set WINDRES="%GCC_PATH%\windres.exe"
) else (
    set CXX=g++
    set WINDRES=windres
)

echo Compiling resources...
%WINDRES% src/resource.rc -O coff -o src/resource.res

echo Compiling C++ binary...
%CXX% -O2 -std=c++17 -municode -mwindows -static -static-libgcc -static-libstdc++ ^
    src/main.cpp src/resource.res ^
    -o google-overlay.exe ^
    -I include ^
    -lole32 -loleaut32 -luuid -ldwmapi -luser32 -lshell32 -lgdi32 -ladvapi32

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Successfully compiled google-overlay.exe!
) else (
    echo [ERROR] Build failed!
    exit /b %ERRORLEVEL%
)
