@echo off
set "GAME_EXE=BloodyRoarExtreme"
cd /d "%~dp0"
set "OUT=out\build\release"
if not exist "%OUT%\%GAME_EXE%.exe" call build.bat || exit /b 1
del /q "%GAME_EXE%.zip" 2>nul
rem an empty assets folder beside the exe, where the game (files or .iso) goes
rd /s /q "%OUT%\release" 2>nul
mkdir "%OUT%\release\assets"
7z a "%GAME_EXE%.zip" ".\%OUT%\%GAME_EXE%.exe" ".\%OUT%\release\assets" .\README.md .\LICENSE .\THIRD_PARTY.txt
echo.
echo   Release zip created: %GAME_EXE%.zip
echo.
