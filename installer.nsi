; XFB NSIS Installer Script
; Requires NSIS 3.x

!define APPNAME "XFB"
!define COMPANYNAME "Netpack - Online Solutions"
!define DESCRIPTION "XFB Radio Automation Software"
!ifndef VERSION
  !define VERSION "3.1415926"
!endif
; Target architecture: "x64" (default) or "arm64". Passed by build-windows.bat
; via /DARCH=...; affects the output filename and the displayed name.
!ifndef ARCH
  !define ARCH "x64"
!endif
; Folder holding the built application to package. build-windows.bat passes an
; arch-specific folder (e.g. dist-windows-x64) via /DDISTDIR=...
!ifndef DISTDIR
  !define DISTDIR "dist"
!endif
!define HELPURL "https://netpack.pt/"
!define UPDATEURL "https://github.com/netpack/XFB/releases"
!define ABOUTURL "https://netpack.pt/"

; Installer attributes
Name "${APPNAME} ${VERSION} (${ARCH})"
; Keep the historical name for x64 (referenced in README/releases); add an
; arch suffix for other targets so artifacts don't collide.
!if "${ARCH}" == "x64"
  OutFile "XFB-${VERSION}-Setup.exe"
!else
  OutFile "XFB-${VERSION}-${ARCH}-Setup.exe"
!endif
InstallDir "$PROGRAMFILES64\${APPNAME}"
InstallDirRegKey HKLM "Software\${COMPANYNAME}\${APPNAME}" "InstallDir"
RequestExecutionLevel admin

; Modern UI
!include "MUI2.nsh"
; FileFunc.nsh provides ${GetSize}; it must be included BEFORE the Install
; section uses it (NSIS processes the script top-to-bottom).
!include "FileFunc.nsh"
; nsDialogs + LogicLib power the custom "Preferences" page (language/theme).
!include "nsDialogs.nsh"
!include "LogicLib.nsh"

; ─── Preferences page state ──────────────────────────────────────────────────
; Selections captured on the custom page and written to xfb-defaults.conf, which
; XFB uses to seed a first-run user config (see main.cpp setupConfiguration).
Var Dialog
Var LangCombo
Var ThemeLightRadio
Var ThemeDarkRadio
Var StartMenuCheck ; HWND of the checkbox control
Var SelLanguage    ; "en" | "pt" | "fr"
Var SelDarkMode    ; "true" | "false"
Var SelStartMenu   ; ${BST_CHECKED} / ${BST_UNCHECKED}

; Pages
!insertmacro MUI_PAGE_WELCOME
; Show a plain-text overview + license. README.md is Markdown/HTML and rendered
; as raw tags on this page, so we point it at a dedicated plain-text file.
!insertmacro MUI_PAGE_LICENSE "installer-license.txt"
!insertmacro MUI_PAGE_DIRECTORY
; Custom page: let the user pick language and light/dark appearance up front.
Page custom PrefsPageCreate PrefsPageLeave
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Language
!insertmacro MUI_LANGUAGE "English"

; ─── Preferences page ────────────────────────────────────────────────────────

Function .onInit
    ; Sensible defaults if the user just clicks through.
    StrCpy $SelLanguage "en"
    StrCpy $SelDarkMode "false"
    StrCpy $SelStartMenu ${BST_CHECKED}
FunctionEnd

Function PrefsPageCreate
    !insertmacro MUI_HEADER_TEXT "Preferences" "Choose your language and appearance. You can change these later in Options."

    nsDialogs::Create 1018
    Pop $Dialog
    ${If} $Dialog == error
        Abort
    ${EndIf}

    ; Language
    ${NSD_CreateLabel} 0 0 100% 12u "Language:"
    Pop $0
    ${NSD_CreateDropList} 0 13u 60% 60u ""
    Pop $LangCombo
    ${NSD_CB_AddString} $LangCombo "English"
    ${NSD_CB_AddString} $LangCombo "Portugues"
    ${NSD_CB_AddString} $LangCombo "Francais"
    ${NSD_CB_SelectString} $LangCombo "English"

    ; Appearance
    ${NSD_CreateLabel} 0 40u 100% 12u "Appearance:"
    Pop $0
    ${NSD_CreateRadioButton} 0 53u 45% 12u "Light"
    Pop $ThemeLightRadio
    ${NSD_CreateRadioButton} 45% 53u 45% 12u "Dark"
    Pop $ThemeDarkRadio
    ${NSD_Check} $ThemeLightRadio

    ; Shortcuts
    ${NSD_CreateCheckbox} 0 78u 100% 12u "Create a Start Menu shortcut"
    Pop $StartMenuCheck
    ${NSD_Check} $StartMenuCheck

    nsDialogs::Show
FunctionEnd

Function PrefsPageLeave
    ; Language selection -> language code XFB understands.
    ${NSD_GetText} $LangCombo $0
    ${If} $0 == "Portugues"
        StrCpy $SelLanguage "pt"
    ${ElseIf} $0 == "Francais"
        StrCpy $SelLanguage "fr"
    ${Else}
        StrCpy $SelLanguage "en"
    ${EndIf}

    ; Theme radio -> DarkMode boolean.
    ${NSD_GetState} $ThemeDarkRadio $1
    ${If} $1 == ${BST_CHECKED}
        StrCpy $SelDarkMode "true"
    ${Else}
        StrCpy $SelDarkMode "false"
    ${EndIf}

    ; Start Menu shortcut checkbox -> store its checked state for the Section.
    ${NSD_GetState} $StartMenuCheck $SelStartMenu
FunctionEnd

; ─── Install Section ─────────────────────────────────────────────────────────

Section "Install"
    SetOutPath $INSTDIR
    
    ; Copy all files from the (arch-specific) distribution directory
    File /r "${DISTDIR}\*.*"

    ; Write the chosen preferences to a defaults file next to XFB.exe. On its
    ; first run (when no per-user xfb.conf exists yet) XFB seeds the user config
    ; from this file — see setupConfiguration() in main.cpp. Writing it here,
    ; rather than into the user's %APPDATA% from this elevated installer, keeps
    ; it reliable regardless of which account UAC elevated to. QSettings' INI
    ; format keeps these keys in the [General] section.
    FileOpen $9 "$INSTDIR\xfb-defaults.conf" w
    FileWrite $9 "[General]$\r$\n"
    FileWrite $9 "Language=$SelLanguage$\r$\n"
    FileWrite $9 "DarkMode=$SelDarkMode$\r$\n"
    FileClose $9

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    
    ; Create Start Menu shortcuts (optional, per the Preferences page)
    ${If} $SelStartMenu == ${BST_CHECKED}
        CreateDirectory "$SMPROGRAMS\${APPNAME}"
        CreateShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\XFB.exe" "" "$INSTDIR\XFB.exe" 0
        CreateShortCut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
    ${EndIf}
    
    ; Create Desktop shortcut
    CreateShortCut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\XFB.exe" "" "$INSTDIR\XFB.exe" 0
    
    ; Write registry keys for Add/Remove Programs
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME} (${ARCH}) - ${DESCRIPTION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayIcon" "$INSTDIR\XFB.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "Publisher" "${COMPANYNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "HelpLink" "${HELPURL}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "URLUpdateInfo" "${UPDATEURL}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "URLInfoAbout" "${ABOUTURL}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayVersion" "${VERSION}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoRepair" 1
    
    ; Store install directory
    WriteRegStr HKLM "Software\${COMPANYNAME}\${APPNAME}" "InstallDir" "$INSTDIR"
    
    ; Estimate installed size
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "EstimatedSize" "$0"
SectionEnd

; ─── Uninstall Section ───────────────────────────────────────────────────────

Section "Uninstall"
    ; Kill running XFB processes
    nsExec::ExecToLog 'taskkill /F /IM XFB.exe'
    
    ; Wait for process to exit
    Sleep 1000
    
    ; Remove Start Menu shortcuts
    Delete "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"
    Delete "$SMPROGRAMS\${APPNAME}\Uninstall.lnk"
    RMDir "$SMPROGRAMS\${APPNAME}"
    
    ; Remove Desktop shortcut
    Delete "$DESKTOP\${APPNAME}.lnk"
    
    ; Remove installed files (entire install directory)
    RMDir /r "$INSTDIR"
    
    ; Remove registry keys
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
    DeleteRegKey HKLM "Software\${COMPANYNAME}\${APPNAME}"
    
    ; Note: User config in %APPDATA%\Netpack - Online Solutions\XFB is preserved
    ; Users can manually remove it if desired
SectionEnd
