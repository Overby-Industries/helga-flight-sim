@echo off
setlocal
set "TARGET=%~dp0HelgaFlightSim.exe"
set "LINK=%USERPROFILE%\Desktop\Project Helga.lnk"

if not exist "%TARGET%" (
    echo Could not find HelgaFlightSim.exe next to this script.
    echo Make sure you run this from the folder you extracted the game into.
    pause
    exit /b 1
)

powershell -NoProfile -Command "$s = (New-Object -ComObject WScript.Shell).CreateShortcut('%LINK%'); $s.TargetPath = '%TARGET%'; $s.WorkingDirectory = '%~dp0'; $s.IconLocation = '%TARGET%'; $s.Save()"

echo.
echo Desktop shortcut created: %LINK%
echo.
pause
