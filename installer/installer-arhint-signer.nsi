; ArhintSigner Web Service Installer
; NSIS Script

!define APP_NAME "ArhintSigner Web Service"
!define APP_SHORT_NAME "ArhintSignerWS"
!define APP_VERSION "1.0.0"
!define PUBLISHER "Arhint"
!define SERVICE_PORT "8082"

; Include Modern UI
!include "MUI2.nsh"
!include "FileFunc.nsh"

; General Settings
Name "${APP_NAME}"
OutFile "ArhintSigner-WebService-Setup.exe"
InstallDir "$PROGRAMFILES\${APP_SHORT_NAME}"
InstallDirRegKey HKLM "Software\${APP_SHORT_NAME}" "Install_Dir"
RequestExecutionLevel admin

; Interface Settings
!define MUI_ABORTWARNING
!define MUI_ICON "..\resources\icon\app-icon-48.ico"
!define MUI_UNICON "..\resources\icon\app-icon-48.ico"

; Component page description
!define MUI_COMPONENTSPAGE_TEXT_TOP "Choose installation options for ${APP_NAME}."

; Pages
!insertmacro MUI_PAGE_LICENSE "..\LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Languages
!insertmacro MUI_LANGUAGE "English"

; Version Information
VIProductVersion "${APP_VERSION}.0"
VIAddVersionKey "ProductName" "${APP_NAME}"
VIAddVersionKey "CompanyName" "${PUBLISHER}"
VIAddVersionKey "FileDescription" "${APP_NAME} Installer"
VIAddVersionKey "FileVersion" "${APP_VERSION}"
VIAddVersionKey "LegalCopyright" "Copyright (C) 2025 ${PUBLISHER}"

;--------------------------------
; Helper Functions

Function ReserveURL
  ; Reserve HTTP URL using netsh
  nsExec::ExecToLog 'netsh http add urlacl url=http://+:${SERVICE_PORT}/ user=Everyone'
  Pop $0
  ${If} $0 == 0
    MessageBox MB_OK "HTTP URL reserved successfully.$\nThe web service can now run without administrator privileges."
  ${Else}
    MessageBox MB_OK "Failed to reserve HTTP URL.$\nYou may need to run as Administrator."
  ${EndIf}
FunctionEnd

;--------------------------------
; Installer Sections

Section "Program Files" SecProgram
  SectionIn RO  ; Read-only, always installed
  
  SetOutPath "$INSTDIR"
  
  ; Copy main executable
  File "..\release\arhint-signer.exe"
  
  ; Copy icon files
  File "..\resources\icon\app-icon-16.ico"
  File "..\resources\icon\app-icon-32.ico"
  File "..\resources\icon\app-icon-48.ico"
  File "..\resources\icon\app-icon-64.ico"
  File "..\resources\icon\app-icon-128.ico"
  File "..\resources\icon\app-icon-256.ico"
  
  ; Copy example HTML file
  File "..\examples\example-arhint-signer.html"
  
  ; Copy README
  File "..\README.md"
  
  ; Write uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ; Write registry keys for Add/Remove Programs
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_SHORT_NAME}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_SHORT_NAME}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_SHORT_NAME}" "DisplayIcon" "$INSTDIR\app-icon-48.ico"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_SHORT_NAME}" "Publisher" "${PUBLISHER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_SHORT_NAME}" "DisplayVersion" "${APP_VERSION}"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_SHORT_NAME}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_SHORT_NAME}" "NoRepair" 1
  
  ; Get installed size
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_SHORT_NAME}" "EstimatedSize" "$0"
  
  ; Store installation directory
  WriteRegStr HKLM "Software\${APP_SHORT_NAME}" "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM "Software\${APP_SHORT_NAME}" "Version" "${APP_VERSION}"
SectionEnd

Section "Reserve HTTP URL" SecReserveURL
  ; Reserve HTTP URL for non-admin users
  Call ReserveURL
SectionEnd

Section "Auto-start on Windows Startup" SecAutoStart
  ; Add registry entry to auto-start the service on Windows boot
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "${APP_SHORT_NAME}" "$INSTDIR\arhint-signer.exe"
SectionEnd

;--------------------------------
; Section Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecProgram} "Install the ${APP_NAME} executable and related files."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecReserveURL} "Reserve HTTP URL to allow the service to run without administrator privileges."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecAutoStart} "Automatically start the web service when Windows starts."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Uninstaller Section

Section "Uninstall"
  
  ; Remove HTTP URL reservation
  nsExec::ExecToLog 'netsh http delete urlacl url=http://+:${SERVICE_PORT}/'
  
  ; Remove auto-start registry entry
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "${APP_SHORT_NAME}"
  
  ; Remove files
  Delete "$INSTDIR\arhint-signer.exe"
  Delete "$INSTDIR\example-arhint-signer.html"
  Delete "$INSTDIR\README.md"
  Delete "$INSTDIR\Uninstall.exe"
  
  ; Remove directories
  RMDir "$INSTDIR"
  
  ; Remove Start Menu shortcuts
  Delete "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk"
  RMDir "$SMPROGRAMS\${APP_NAME}"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_SHORT_NAME}"
  DeleteRegKey HKLM "Software\${APP_SHORT_NAME}"
  
SectionEnd
