@echo off
REM XFB Windows Uninstaller (Portable)
REM Run this script to remove XFB portable installation

echo.
echo ========================================
echo   XFB Uninstaller for Windows
echo ========================================
echo.
echo This will remove XFB from the current directory.
echo Your music library and playlists will NOT be deleted.
echo.

set /p CONFIRM="Continue with uninstallation? (Y/N): "
if /i not "%CONFIRM%"=="Y" (
    echo Uninstallation cancelled.
    pause
    exit /b 0
)

echo.
echo Stopping XFB if running...
taskkill /F /IM XFB.exe >nul 2>&1
timeout /t 2 /nobreak >nul

REM Remove Desktop shortcut if it exists
if exist "%USERPROFILE%\Desktop\XFB.lnk" (
    del "%USERPROFILE%\Desktop\XFB.lnk"
    echo   Removed Desktop shortcut
)

REM Remove Start Menu shortcut if it exists
if exist "%APPDATA%\Microsoft\Windows\Start Menu\Programs\XFB" (
    rmdir /s /q "%APPDATA%\Microsoft\Windows\Start Menu\Programs\XFB"
    echo   Removed Start Menu shortcuts
)

REM Remove Tor data
set TOR_DATA=%LOCALAPPDATA%\XFB\tor_data
if exist "%TOR_DATA%" (
    rmdir /s /q "%TOR_DATA%"
    echo   Removed Tor data
)

REM Remove application cache
set APP_CACHE=%LOCALAPPDATA%\XFB
if exist "%APP_CACHE%" (
    rmdir /s /q "%APP_CACHE%"
    echo   Removed application cache
)

echo.
echo ========================================
echo   XFB has been uninstalled
echo ========================================
echo.
echo User configuration in %%APPDATA%%\Netpack - Online Solutions\XFB
echo was preserved. Remove it manually if desired.
echo.
echo You can now delete this folder to complete removal.
echo.
pause
