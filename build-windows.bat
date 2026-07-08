@echo off
REM XFB Windows Build and Package Script
REM Requires: Qt6 (with MSVC), CMake, Visual Studio Build Tools, NSIS (optional)
REM
REM Usage:
REM   build-windows.bat [--arch x64|arm64] [--clean]
REM
REM   --arch x64     Build for 64-bit Intel/AMD (default)
REM   --arch arm64   Build for 64-bit ARM (Windows on ARM, e.g. Snapdragon)
REM   --clean        Remove previous build/ and install/ folders first
REM
REM Environment overrides:
REM   QT_DIR        Path to the Qt build matching the target arch, e.g.
REM                   x64:   C:\Qt\6.8.3\msvc2022_64
REM                   arm64: C:\Qt\6.8.3\msvc2022_arm64
REM   QT_HOST_DIR   (cross-compiling only) Path to a Qt build for the HOST arch.
REM                 Needed when building arm64 on an x64 machine, because
REM                 windeployqt is a native tool and the arm64 one can't run on
REM                 x64. When set, the host windeployqt deploys the target DLLs.

setlocal enabledelayedexpansion

echo.
echo ========================================
echo   XFB Windows Build Script
echo ========================================
echo.

set PROJECT_NAME=XFB
set VERSION=3.14159
set BUILD_DIR=build
set INSTALL_DIR=install

REM --- Parse arguments ---
set ARCH=x64
set DO_CLEAN=0
:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--clean" (
    set DO_CLEAN=1
    shift
    goto :parse_args
)
if /i "%~1"=="--arch" (
    set ARCH=%~2
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="x64"   ( set ARCH=x64   & shift & goto :parse_args )
if /i "%~1"=="arm64" ( set ARCH=arm64 & shift & goto :parse_args )
if /i "%~1"=="--arm64" ( set ARCH=arm64 & shift & goto :parse_args )
echo [WARNING] Unknown argument: %~1
shift
goto :parse_args
:args_done

REM --- Normalize and validate the target architecture ---
if /i "%ARCH%"=="x86_64" set ARCH=x64
if /i "%ARCH%"=="amd64"  set ARCH=x64
if /i "%ARCH%"=="aarch64" set ARCH=arm64
if /i "%ARCH%"=="x64" (
    set CMAKE_ARCH=x64
    set QT_SUBDIR=msvc2022_64
) else if /i "%ARCH%"=="arm64" (
    set CMAKE_ARCH=ARM64
    set QT_SUBDIR=msvc2022_arm64
) else (
    echo [ERROR] Unsupported --arch "%ARCH%". Use x64 or arm64.
    exit /b 1
)
echo [INFO] Target architecture: %ARCH%  ^(CMake -A %CMAKE_ARCH%, Qt %QT_SUBDIR%^)

REM Use a Windows- and arch-specific build directory so it never collides with
REM a build/ tree generated on another OS when the project folder is shared
REM (e.g. this repo is visible as Z:\ on Windows and /Users/... on macOS).
set "BUILD_DIR=build-windows-%ARCH%"
REM Arch-specific distribution folder so x64 and arm64 outputs don't overwrite
REM each other when built from the same (possibly shared) source tree.
set "DIST_DIR=dist-windows-%ARCH%"

REM --- Determine host architecture (for picking the right MSVC toolchain) ---
set HOST_ARCH=%PROCESSOR_ARCHITECTURE%
if defined PROCESSOR_ARCHITEW6432 set HOST_ARCH=%PROCESSOR_ARCHITEW6432%
REM Normalize the host arch to the same x64/arm64 tokens used for the target.
if /i "%HOST_ARCH%"=="ARM64" ( set "HOST_TOKEN=arm64" ) else ( set "HOST_TOKEN=x64" )
REM Candidate vcvarsall arguments to try, in preference order. We try the native
REM host toolset first, then the cross variant, so the build works whether you
REM installed the ARM64-hosted or the x64-hosted ARM64 build tools component.
if /i "%ARCH%"=="arm64" (
    if /i "%HOST_TOKEN%"=="arm64" ( set "VCVARS_CANDIDATES=arm64 amd64_arm64" ) else ( set "VCVARS_CANDIDATES=amd64_arm64" )
) else (
    if /i "%HOST_TOKEN%"=="arm64" ( set "VCVARS_CANDIDATES=amd64 arm64_amd64" ) else ( set "VCVARS_CANDIDATES=x64" )
)
echo [INFO] Host architecture: %HOST_ARCH%  ^(vcvarsall candidates: %VCVARS_CANDIDATES%^)

REM --- Detect Qt installation (must match the target arch) ---
REM A directory is a usable Qt kit for a CMake build if it contains
REM lib\cmake\Qt6\Qt6Config.cmake (what -DCMAKE_PREFIX_PATH needs). We accept
REM qmake.exe / qtpaths.exe as alternative evidence, since exact tool naming
REM has varied across Qt versions.
if defined QT_DIR (
    REM Trim surrounding quotes and a trailing backslash, if any.
    set QT_DIR=%QT_DIR:"=%
    if "!QT_DIR:~-1!"=="\" set "QT_DIR=!QT_DIR:~0,-1!"
    echo [INFO] QT_DIR is set to: !QT_DIR!
    if exist "!QT_DIR!\lib\cmake\Qt6\Qt6Config.cmake" ( set "QT_DIR=!QT_DIR!" & goto :qt_found )
    if exist "!QT_DIR!\bin\qmake.exe"   ( set "QT_DIR=!QT_DIR!" & goto :qt_found )
    if exist "!QT_DIR!\bin\qtpaths.exe" ( set "QT_DIR=!QT_DIR!" & goto :qt_found )
    echo [WARNING] QT_DIR does not look like a Qt kit ^(no lib\cmake\Qt6\Qt6Config.cmake,
    echo           bin\qmake.exe or bin\qtpaths.exe under it^). Trying auto-detection...
)

REM Scan every Qt version installed under the standard roots for a build that
REM matches the target arch's subdir (e.g. msvc2022_64 / msvc2022_arm64).
echo [INFO] Auto-detecting a "!QT_SUBDIR!" Qt kit under C:\Qt and %USERPROFILE%\Qt ...
for %%R in ("C:\Qt" "%USERPROFILE%\Qt") do (
    if exist "%%~R" (
        for /d %%V in ("%%~R\*") do (
            if exist "%%~V\!QT_SUBDIR!\lib\cmake\Qt6\Qt6Config.cmake" ( set "QT_DIR=%%~V\!QT_SUBDIR!" & goto :qt_found )
            if exist "%%~V\!QT_SUBDIR!\bin\qmake.exe"   ( set "QT_DIR=%%~V\!QT_SUBDIR!" & goto :qt_found )
            if exist "%%~V\!QT_SUBDIR!\bin\qtpaths.exe" ( set "QT_DIR=%%~V\!QT_SUBDIR!" & goto :qt_found )
        )
    )
)

echo [ERROR] Qt installation for %ARCH% not found ^(looked for "!QT_SUBDIR!" kits
echo         under C:\Qt and %USERPROFILE%\Qt^).
echo.
echo         Make sure the "%QT_SUBDIR%" component is installed, then point QT_DIR
echo         at that kit. In PowerShell use $env: ^(plain "set" does NOT work there^):
echo           $env:QT_DIR = "C:\Qt\6.11.1\!QT_SUBDIR!"
echo         In cmd.exe:
echo           set QT_DIR=C:\Qt\6.11.1\!QT_SUBDIR!
exit /b 1

:qt_found
echo [INFO] Qt directory: %QT_DIR%

REM --- Set up the Visual Studio environment for the target arch ---
REM Locate Visual Studio with vswhere, try to initialize the toolset, and if the
REM required (e.g. ARM64) MSVC build tools are missing, install them
REM automatically. Set XFB_NO_AUTO_INSTALL=1 to disable auto-installation.
call :locate_vcvarsall
call :try_vcvars
if defined MSVC_READY goto :msvc_ready

if /i "%XFB_NO_AUTO_INSTALL%"=="1" (
    echo [ERROR] MSVC toolset for %ARCH% not available and auto-install is disabled.
    goto :msvc_missing
)

echo [WARNING] The MSVC build tools for %ARCH% are not available yet.
echo [INFO] Attempting to download and install them ^(this can take several minutes
echo        and may prompt for administrator permission^)...
call :install_msvc

REM Re-locate (in case Build Tools were freshly installed) and retry.
call :locate_vcvarsall
call :try_vcvars
if defined MSVC_READY goto :msvc_ready

:msvc_missing
echo [ERROR] Could not obtain the MSVC toolset for %ARCH% ^(tried: %VCVARS_CANDIDATES%^).
echo         Open the Visual Studio Installer -^> Modify -^> Individual components
echo         and add:
echo           - MSVC v143 - VS 2022 C++ ARM64/ARM64EC build tools ^(for arm64 targets^)
echo           - MSVC v143 - VS 2022 C++ x64/x86 build tools
echo         then re-run this script.
exit /b 1

:msvc_ready
echo [INFO] MSVC environment ready ^(toolset %MSVC_READY%^).

REM --- Check CMake (auto-install via winget if missing) ---
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    if /i "%XFB_NO_AUTO_INSTALL%"=="1" (
        echo [ERROR] CMake not found and auto-install is disabled.
        exit /b 1
    )
    echo [WARNING] CMake not found. Attempting to install via winget...
    where winget >nul 2>&1 && winget install --id Kitware.CMake -e --accept-package-agreements --accept-source-agreements
    REM winget's PATH update isn't visible to this session; add the default location.
    where cmake >nul 2>&1 || set "PATH=%ProgramFiles%\CMake\bin;%PATH%"
    where cmake >nul 2>&1
    if !errorlevel! neq 0 (
        echo [ERROR] CMake still not found. Install it from https://cmake.org/download and re-run.
        exit /b 1
    )
)
echo [INFO] CMake found.

REM --- Clean if requested ---
if "%DO_CLEAN%"=="1" (
    echo [INFO] Cleaning previous build...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    if exist "%INSTALL_DIR%" rmdir /s /q "%INSTALL_DIR%"
)

REM --- Guard against a stale/incompatible CMake cache ---
REM If a CMakeCache.txt exists but was generated for a different source path
REM (e.g. on macOS via a shared drive), CMake refuses to configure. Detect the
REM mismatch and clear the build directory so it can be regenerated cleanly.
if exist "%BUILD_DIR%\CMakeCache.txt" (
    set "CACHE_HOME="
    for /f "usebackq tokens=1* delims==" %%H in (`findstr /b /c:"CMAKE_HOME_DIRECTORY:INTERNAL=" "%BUILD_DIR%\CMakeCache.txt"`) do set "CACHE_HOME=%%I"
    set "CURDIR_FWD=%CD:\=/%"
    if /i not "!CACHE_HOME!"=="!CURDIR_FWD!" (
        echo [WARNING] The CMake cache in "%BUILD_DIR%" was generated for a different
        echo           location ^(!CACHE_HOME!^). Clearing it to reconfigure for this machine...
        rmdir /s /q "%BUILD_DIR%"
    )
)

REM --- Create build directory ---
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM --- Configure ---
echo [INFO] Configuring with CMake...
cmake .. ^
    -G "Visual Studio 17 2022" ^
    -A %CMAKE_ARCH% ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DBUILD_TESTING=OFF

if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    cd ..
    exit /b 1
)
echo [SUCCESS] CMake configuration completed.

REM --- Build ---
echo [INFO] Building XFB...
cmake --build . --config Release --target XFB -- /m

if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    cd ..
    exit /b 1
)
echo [SUCCESS] Build completed.

REM --- Find the executable ---
set EXE_PATH=bin\Release\XFB.exe
if not exist "%EXE_PATH%" (
    set EXE_PATH=bin\XFB.exe
    if not exist "!EXE_PATH!" (
        echo [ERROR] XFB.exe not found in build output.
        cd ..
        exit /b 1
    )
)
echo [INFO] Executable: %EXE_PATH%

REM --- Deploy Qt libraries ---
cd ..
echo [INFO] Preparing distribution folder: %DIST_DIR%
if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%"
copy "%BUILD_DIR%\%EXE_PATH%" "%DIST_DIR%\XFB.exe"

REM Pick the windeployqt to run. windeployqt is a native tool, so when
REM cross-compiling (e.g. arm64 target on an x64 host) the target Qt's
REM windeployqt can't execute. In that case use the HOST Qt's windeployqt and
REM point it at the target Qt via --qtpaths so it copies the right DLLs.
set WINDEPLOYQT=%QT_DIR%\bin\windeployqt.exe
set WINDEPLOY_EXTRA=
if defined QT_HOST_DIR (
    if not "%HOST_TOKEN%"=="%ARCH%" (
        echo [INFO] Cross-deploy: using host windeployqt from QT_HOST_DIR.
        set WINDEPLOYQT=%QT_HOST_DIR%\bin\windeployqt.exe
        set WINDEPLOY_EXTRA=--qtpaths "%QT_DIR%\bin\qtpaths.exe"
    )
)

echo [INFO] Running windeployqt...
REM Note: no --qml-dir. XFB does not load any .qml at runtime (the ad-banner
REM QML is disabled), and passing --qml-dir at a path without QML content can
REM make windeployqt abort before copying anything.
"%WINDEPLOYQT%" ^
    --release ^
    --no-translations ^
    --no-system-d3d-compiler ^
    --no-opengl-sw ^
    %WINDEPLOY_EXTRA% ^
    "%DIST_DIR%\XFB.exe"
set "WINDEPLOY_RC=%errorlevel%"
if not "%WINDEPLOY_RC%"=="0" (
    echo [WARNING] windeployqt returned %WINDEPLOY_RC%; verifying core DLLs manually.
)

REM Safety net: some windeployqt versions/targets miss module DLLs (we've seen
REM Qt6Sql.dll left out on arm64). Copy any Qt6 module the app links that isn't
REM already in dist so the app doesn't fail at startup with "Qt6*.dll not found".
echo [INFO] Verifying core Qt runtime DLLs...
for %%D in (Core Gui Widgets Concurrent Multimedia MultimediaWidgets Sql Network Qml QmlMeta QmlModels QmlWorkerScript Quick QuickWidgets OpenGL OpenGLWidgets Svg) do (
    if not exist "%DIST_DIR%\Qt6%%D.dll" if exist "%QT_DIR%\bin\Qt6%%D.dll" (
        echo [INFO]   + Qt6%%D.dll
        copy /y "%QT_DIR%\bin\Qt6%%D.dll" "%DIST_DIR%\" >nul
    )
)

REM The Windows platform plugin is required for the app to start at all.
if not exist "%DIST_DIR%\platforms\qwindows.dll" if exist "%QT_DIR%\plugins\platforms\qwindows.dll" (
    echo [INFO] Deploying platform plugin qwindows.dll
    if not exist "%DIST_DIR%\platforms" mkdir "%DIST_DIR%\platforms"
    copy /y "%QT_DIR%\plugins\platforms\qwindows.dll" "%DIST_DIR%\platforms\" >nul
)

REM SQL driver plugins (e.g. qsqlite) are needed by QSqlDatabase at runtime.
if not exist "%DIST_DIR%\sqldrivers" if exist "%QT_DIR%\plugins\sqldrivers" (
    echo [INFO] Deploying SQL driver plugins
    xcopy /e /i /y /q "%QT_DIR%\plugins\sqldrivers" "%DIST_DIR%\sqldrivers" >nul
)

echo [SUCCESS] Qt libraries deployed.

REM --- Bundle ffmpeg so no separate install is ever needed ---
REM Set XFB_NO_FFMPEG_BUNDLE=1 to skip (XFB would then install it on demand).
if not "%XFB_NO_FFMPEG_BUNDLE%"=="1" (
    if exist "%DIST_DIR%\ffmpeg.exe" (
        echo [INFO] ffmpeg already present in dist; skipping download.
    ) else (
        call :bundle_ffmpeg
    )
)

REM --- Bundle Tor (tor.exe) so the Tor feature works out of the box ---
REM Set XFB_NO_TOR_BUNDLE=1 to skip. TorNetworkService looks for
REM <app>\tor\tor.exe first, which is exactly where this places it.
if not "%XFB_NO_TOR_BUNDLE%"=="1" (
    if exist "%DIST_DIR%\tor\tor.exe" (
        echo [INFO] Tor already present in dist; skipping download.
    ) else (
        call :bundle_tor
    )
)

REM --- Copy additional files ---
if exist "README.md" copy "README.md" "%DIST_DIR%\"
if exist "xfb_icon.png" copy "xfb_icon.png" "%DIST_DIR%\"

REM --- Create NSIS installer (auto-install NSIS via winget if missing) ---
where makensis >nul 2>&1
if %errorlevel% neq 0 (
    if not "%XFB_NO_AUTO_INSTALL%"=="1" (
        where winget >nul 2>&1 && (
            echo [INFO] NSIS not found. Installing via winget...
            winget install --id NSIS.NSIS -e --accept-package-agreements --accept-source-agreements
        )
        REM winget PATH update isn't visible this session; add the default location.
        where makensis >nul 2>&1 || set "PATH=%ProgramFiles(x86)%\NSIS;%PATH%"
    )
)
REM Name of the installer this build should produce (matches OutFile in installer.nsi).
if /i "%ARCH%"=="arm64" (
    set "INSTALLER_NAME=XFB-%VERSION%-arm64-Setup.exe"
) else (
    set "INSTALLER_NAME=XFB-%VERSION%-Setup.exe"
)

where makensis >nul 2>&1
if %errorlevel% equ 0 (
    echo [INFO] NSIS found, creating installer...
    REM Delete any installer left over from a previous build FIRST. Otherwise a
    REM failed makensis run would leave the old installer in place and it could
    REM be run by mistake — which is exactly how an out-of-date license page /
    REM missing options page slips through after a rebuild.
    if exist "%INSTALLER_NAME%" del /f /q "%INSTALLER_NAME%"
    makensis /DVERSION=%VERSION% /DARCH=%ARCH% /DDISTDIR=%DIST_DIR% installer.nsi
    if !errorlevel! equ 0 if exist "%INSTALLER_NAME%" (
        echo [SUCCESS] Installer created: %INSTALLER_NAME%
    ) else (
        echo [WARNING] NSIS installer creation failed. No installer was produced; run the portable %DIST_DIR%\XFB.exe instead.
    )
) else (
    echo [INFO] NSIS not available. Skipping installer creation ^(portable %DIST_DIR%\ still produced^).
)

REM --- Summary ---
echo.
echo ========================================
echo   Build completed successfully!
echo ========================================
echo.
echo   Portable: %DIST_DIR%\XFB.exe
echo   Version:  %VERSION%
echo   Arch:     %ARCH%
if /i "%ARCH%"=="x64"   echo   Installer: XFB-%VERSION%-Setup.exe
if /i "%ARCH%"=="arm64" echo   Installer: XFB-%VERSION%-arm64-Setup.exe
echo.
echo   To run: %DIST_DIR%\XFB.exe
echo.

REM End of main flow — do not fall through into subroutines below.
exit /b 0

REM ============================ Subroutines ============================

REM --- Locate vcvarsall.bat and the VS install path.
REM IMPORTANT: we must NOT run vswhere inside a  for /f (`...`)  block, because
REM its path is under "C:\Program Files (x86)\..." and the ")" in "(x86)" breaks
REM cmd's parenthesis matching (the infamous 'C:\Program' is not recognized).
REM Instead we run vswhere at top level, capture output to a temp file, and
REM derive vcvarsall from the install path.
:locate_vcvarsall
set "VCVARSALL="
set "VSINSTALLPATH="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :locate_fallback

"%VSWHERE%" -latest -prerelease -products * -property installationPath >"%TEMP%\xfb_vspath.txt" 2>nul
if exist "%TEMP%\xfb_vspath.txt" set /p VSINSTALLPATH=<"%TEMP%\xfb_vspath.txt"
del "%TEMP%\xfb_vspath.txt" >nul 2>&1
if defined VSINSTALLPATH if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat"
if defined VCVARSALL goto :eof

:locate_fallback
if defined VCVARSALL goto :eof
call :probe_vcvars "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools"
call :probe_vcvars "%ProgramFiles%\Microsoft Visual Studio\2022\Community"
call :probe_vcvars "%ProgramFiles%\Microsoft Visual Studio\2022\Professional"
call :probe_vcvars "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise"
call :probe_vcvars "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"
call :probe_vcvars "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community"
goto :eof

REM --- Set VCVARSALL if %1 contains a VC\Auxiliary\Build\vcvarsall.bat.
:probe_vcvars
if defined VCVARSALL goto :eof
if exist "%~1\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%~1\VC\Auxiliary\Build\vcvarsall.bat"
if not defined VSINSTALLPATH if defined VCVARSALL set "VSINSTALLPATH=%~1"
goto :eof

REM --- Try each candidate toolset until "cl" is on PATH. Sets MSVC_READY.
REM Each candidate is handled by :vc_one at top level (NOT inside this for
REM block), because vcvarsall lives under "C:\Program Files (x86)\..." and the
REM ")" in "(x86)" would prematurely close a parenthesized block if the path
REM were expanded here, producing a bogus 'C:\Program' is not recognized error.
:try_vcvars
set "MSVC_READY="
if not defined VCVARSALL goto :eof
for %%A in (%VCVARS_CANDIDATES%) do call :vc_one %%A
goto :eof

REM --- Initialize one candidate toolset (%1). Sets MSVC_READY on success.
:vc_one
if defined MSVC_READY goto :eof
echo [INFO] Trying MSVC toolset: %1
call "%VCVARSALL%" %1 >nul 2>&1
where cl >nul 2>&1 && set "MSVC_READY=%1"
goto :eof

REM --- Install the C++ build tools (ARM64 + x64). Modifies an existing VS
REM     install if present, otherwise installs Build Tools via winget.
REM A silent "modify" MUST run elevated, so we launch setup.exe through
REM PowerShell's Start-Process -Verb RunAs (this raises a single UAC prompt).
:install_msvc
set "VSINSTALLER=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\setup.exe"
if not exist "%VSINSTALLER%" set "VSINSTALLER=%ProgramFiles%\Microsoft Visual Studio\Installer\setup.exe"

if not defined VSINSTALLPATH goto :install_via_winget
if not exist "%VSINSTALLER%" goto :install_via_winget

echo [INFO] Adding the C++ ARM64/x64 build tools to your existing Visual Studio.
echo [INFO] A Windows UAC prompt will appear (administrator rights are required
echo        for a silent install) - please approve it. This can take a while.
REM Values are passed to PowerShell via environment variables to avoid quoting
REM problems with the spaces and parentheses in the paths.
set "XFB_VSINSTALLER=%VSINSTALLER%"
set "XFB_VSINSTALLPATH=%VSINSTALLPATH%"
powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath $env:XFB_VSINSTALLER -ArgumentList 'modify','--installPath',$env:XFB_VSINSTALLPATH,'--add','Microsoft.VisualStudio.Component.VC.Tools.ARM64','--add','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','--quiet','--norestart' -Verb RunAs -Wait"
goto :eof

:install_via_winget
where winget >nul 2>&1 || goto :install_none
echo [INFO] No existing Visual Studio C++ install found. Installing
echo        "Build Tools for Visual Studio 2022" with the ARM64 + x64 toolsets
echo        via winget (approve any UAC prompt)...
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --accept-package-agreements --accept-source-agreements --override "--quiet --norestart --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.Tools.ARM64 --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
goto :eof

:install_none
echo [WARNING] Could not auto-install the build tools: no existing Visual Studio
echo           install to modify, and winget is not available.
goto :eof

REM --- Download a static ffmpeg build for the target arch and place
REM     ffmpeg.exe + ffprobe.exe in dist\. Best-effort: on failure the app still
REM     works (it offers to install ffmpeg on demand). BtbN provides native
REM     win64 and winarm64 builds.
:bundle_ffmpeg
if /i "%ARCH%"=="arm64" (
    set "FFMPEG_URL=https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-winarm64-gpl.zip"
) else (
    set "FFMPEG_URL=https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip"
)
where powershell >nul 2>&1
if errorlevel 1 (
    echo [WARNING] PowerShell not found; skipping ffmpeg bundle ^(will install on demand^).
    goto :eof
)
echo [INFO] Downloading ffmpeg ^(%ARCH%^) for bundling ^(this may take a minute^)...
set "XFB_FFMPEG_URL=%FFMPEG_URL%"
set "XFB_DIST_DIR=%CD%\%DIST_DIR%"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ProgressPreference='SilentlyContinue'; $ErrorActionPreference='Stop'; try { $u=$env:XFB_FFMPEG_URL; $z=Join-Path $env:TEMP 'xfb_ffmpeg.zip'; $d=Join-Path $env:TEMP 'xfb_ffmpeg'; Invoke-WebRequest -Uri $u -OutFile $z; if(Test-Path $d){Remove-Item -Recurse -Force $d}; Expand-Archive -Path $z -DestinationPath $d -Force; $f=Get-ChildItem -Path $d -Recurse -Filter ffmpeg.exe | Select-Object -First 1; $p=Get-ChildItem -Path $d -Recurse -Filter ffprobe.exe | Select-Object -First 1; if($f){Copy-Item $f.FullName -Destination (Join-Path $env:XFB_DIST_DIR 'ffmpeg.exe') -Force}; if($p){Copy-Item $p.FullName -Destination (Join-Path $env:XFB_DIST_DIR 'ffprobe.exe') -Force}; Remove-Item -Recurse -Force $d,$z -ErrorAction SilentlyContinue } catch { Write-Host ('ffmpeg bundle failed: ' + $_.Exception.Message); exit 1 }"
if exist "%DIST_DIR%\ffmpeg.exe" (
    echo [SUCCESS] Bundled ffmpeg.exe ^(and ffprobe.exe^) into %DIST_DIR%.
) else (
    echo [WARNING] Could not bundle ffmpeg; XFB will install it on demand instead.
)
goto :eof

REM --- Download the Tor Expert Bundle and place tor.exe (+ its DLLs) in
REM     dist\tor\. Best-effort: on failure the Tor feature is simply
REM     unavailable in this build. Uses the x86_64 build (runs under emulation
REM     on Windows ARM64, as there is no official Windows-ARM64 Tor build).
:bundle_tor
where powershell >nul 2>&1 || (echo [WARNING] PowerShell not found; skipping Tor bundle. & goto :eof)
where tar >nul 2>&1 || (echo [WARNING] tar.exe not found; skipping Tor bundle. & goto :eof)
echo [INFO] Downloading the Tor Expert Bundle for bundling ^(this may take a minute^)...
set "XFB_DIST_DIR=%CD%\%DIST_DIR%"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ProgressPreference='SilentlyContinue'; $ErrorActionPreference='Stop'; try { $info = Invoke-RestMethod 'https://aus1.torproject.org/torbrowser/update_3/release/downloads.json'; $ver = $info.version; $url = 'https://archive.torproject.org/tor-package-archive/torbrowser/' + $ver + '/tor-expert-bundle-windows-x86_64-' + $ver + '.tar.gz'; $tgz = Join-Path $env:TEMP 'xfb_tor.tar.gz'; $ext = Join-Path $env:TEMP 'xfb_tor'; Invoke-WebRequest -Uri $url -OutFile $tgz; if(Test-Path $ext){Remove-Item -Recurse -Force $ext}; New-Item -ItemType Directory -Force -Path $ext | Out-Null; tar -xzf $tgz -C $ext; $dest = Join-Path $env:XFB_DIST_DIR 'tor'; if(Test-Path $dest){Remove-Item -Recurse -Force $dest}; New-Item -ItemType Directory -Force -Path $dest | Out-Null; Copy-Item (Join-Path $ext 'tor\*') -Destination $dest -Recurse -Force; $data = Join-Path $ext 'data'; if(Test-Path $data){ Copy-Item $data -Destination (Join-Path $dest 'data') -Recurse -Force }; Remove-Item -Recurse -Force $tgz,$ext -ErrorAction SilentlyContinue } catch { Write-Host ('Tor bundle failed: ' + $_.Exception.Message); exit 1 }"
if exist "%DIST_DIR%\tor\tor.exe" (
    echo [SUCCESS] Bundled Tor into %DIST_DIR%\tor.
) else (
    echo [WARNING] Could not bundle Tor; the Tor feature will be unavailable in this build.
)
goto :eof
