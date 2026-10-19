@echo off
set "GAME_EXE=BloodyRoarExtreme"
cd /d "%~dp0"
set "OUT=out\build\release"
if not exist "%OUT%\%GAME_EXE%.exe" call build.bat || exit /b 1
del /q "%GAME_EXE%.zip" 2>nul
rem an empty assets folder beside the exe, where the game (files or .iso) goes
rd /s /q "%OUT%\release" 2>nul
copy /y "sdk\dxc\bin\x64\dxcompiler.dll" "%OUT%\dxcompiler.dll" >nul
copy /y "sdk\dxc\bin\x64\dxil.dll" "%OUT%\dxil.dll" >nul
7z a "%GAME_EXE%.zip" ".\%OUT%\%GAME_EXE%.exe" ".\%OUT%\dxcompiler.dll" ".\%OUT%\dxil.dll" ".\%OUT%\release\assets" .\README.md .\LICENSE .\THIRD_PARTY.txt
echo.
echo   Release zip created: %GAME_EXE%.zip
echo.
