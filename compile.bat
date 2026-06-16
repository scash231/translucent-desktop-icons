@echo off
echo Compiling resource file...
windres resource.rc -O coff -o resource.res
if %ERRORLEVEL% neq 0 (
    echo Resource compilation failed.
    exit /b %ERRORLEVEL%
)

echo Compiling main.cpp...
g++ -O3 -o desktop_icons.exe main.cpp resource.res -luser32 -lgdi32
if %ERRORLEVEL% equ 0 (
    echo Compilation successful! Created desktop_icons.exe
) else (
    echo Compilation failed.
)
