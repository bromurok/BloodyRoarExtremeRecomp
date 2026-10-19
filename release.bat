@echo off
set "GAME_EXE=BloodyRoarExtreme"
cd /d "%~dp0"
set "OUT=out\build\release"
if not exist "%OUT%\%GAME_EXE%.exe" call build.bat || exit /b 1
del /q "%GAME_EXE%.zip" 2>nul
rem an empty assets folder beside the exe, where the game (files or .iso) goes
rd /s /q "%OUT%\release" 2>nul
mkdir "%OUT%\release\assets"
copy /y "sdk\dxc\bin\x64\dxcompiler.dll" "%OUT%\dxcompiler.dll" >nul 2>nul
copy /y "sdk\dxc\bin\x64\dxil.dll" "%OUT%\dxil.dll" >nul 2>nul
set "DLL_BUNDLE="
if exist "%OUT%\dxcompiler.dll" set DLL_BUNDLE=%DLL_BUNDLE% ".\%OUT%\dxcompiler.dll"
if exist "%OUT%\dxil.dll" set DLL_BUNDLE=%DLL_BUNDLE% ".\%OUT%\dxil.dll"
if exist "libc++.dll" set DLL_BUNDLE=%DLL_BUNDLE% .\libc++.dll
if exist "libunwind.dll" set DLL_BUNDLE=%DLL_BUNDLE% .\libunwind.dll
7z a "%GAME_EXE%.zip" ".\%OUT%\%GAME_EXE%.exe" ".\%OUT%\release\assets" .\README.md .\LICENSE .\THIRD_PARTY.txt %DLL_BUNDLE%
echo.
echo   Release zip created: %GAME_EXE%.zip
echo.
