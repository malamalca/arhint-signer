; ArhintSigner Installer
; NSIS Script

!define APP_NAME "ArhintSigner"
!define APP_VERSION "1.0.0"
!define PUBLISHER "Arhint"
!define EXTENSION_ID "knihbpgabbekmjmchbdmmaiecddokeln"

; Include Modern UI
!include "MUI2.nsh"

; General Settings
Name "${APP_NAME}"
OutFile "ArhintSigner-Setup.exe"
InstallDir "$PROGRAMFILES\${APP_NAME}"
InstallDirRegKey HKLM "Software\${APP_NAME}" "Install_Dir"
RequestExecutionLevel admin

; Interface Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Pages
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

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

;--------------------------------
; Installer Section

Section "Install"
  SetOutPath "$INSTDIR"
  
  ; Copy files
  File "dist\arhint-signer.exe"
  
  ; Create manifest file with correct path
  StrCpy $0 "$INSTDIR\arhint-signer.exe"
  StrCpy $1 $0 "" "" ; Copy string
  ${StrRep} $2 $1 "\" "\\" ; Replace \ with \\
  
  FileOpen $3 "$INSTDIR\arhint.signer.json" w
  FileWrite $3 '{$\r$\n'
  FileWrite $3 '  "name": "arhint.signer",$\r$\n'
  FileWrite $3 '  "description": "ArhintSigner - Digital Signature Helper",$\r$\n'
  FileWrite $3 '  "path": "$2",$\r$\n'
  FileWrite $3 '  "type": "stdio",$\r$\n'
  FileWrite $3 '  "allowed_origins": [$\r$\n'
  FileWrite $3 '    "chrome-extension://${EXTENSION_ID}/"$\r$\n'
  FileWrite $3 '  ]$\r$\n'
  FileWrite $3 '}$\r$\n'
  FileClose $3
  
  ; Register native messaging host with Chrome
  WriteRegStr HKCU "Software\Google\Chrome\NativeMessagingHosts\arhint.signer" "" "$INSTDIR\arhint.signer.json"
  
  ; Register native messaging host with Edge
  WriteRegStr HKCU "Software\Microsoft\Edge\NativeMessagingHosts\arhint.signer" "" "$INSTDIR\arhint.signer.json"
  
  ; Copy Chrome extension files
  SetOutPath "$INSTDIR\extension"
  File "extension\manifest.json"
  File "extension\background.js"
  
  ; Write uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ; Create Start Menu shortcuts
  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  
  ; Write registry keys for Add/Remove Programs
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon" "$INSTDIR\arhint-signer.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "${PUBLISHER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1
  
  ; Store installation directory
  WriteRegStr HKLM "Software\${APP_NAME}" "Install_Dir" "$INSTDIR"
  
  ; Show installation complete message with extension installation instructions
  MessageBox MB_OK "Installation complete!$\r$\n$\r$\nTo install the Chrome extension:$\r$\n1. Open Chrome and go to chrome://extensions/$\r$\n2. Enable 'Developer mode'$\r$\n3. Click 'Load unpacked'$\r$\n4. Select: $INSTDIR\extension$\r$\n$\r$\nRestart your browser for changes to take effect."
  
SectionEnd

;--------------------------------
; Uninstaller Section

Section "Uninstall"
  ; Remove registry keys
  DeleteRegKey HKCU "Software\Google\Chrome\NativeMessagingHosts\arhint.signer"
  DeleteRegKey HKCU "Software\Microsoft\Edge\NativeMessagingHosts\arhint.signer"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
  DeleteRegKey HKLM "Software\${APP_NAME}"
  
  ; Remove files and directories
  Delete "$INSTDIR\arhint-signer.exe"
  Delete "$INSTDIR\arhint.signer.json"
  Delete "$INSTDIR\extension\manifest.json"
  Delete "$INSTDIR\extension\background.js"
  RMDir "$INSTDIR\extension"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  
  ; Remove Start Menu shortcuts
  Delete "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk"
  RMDir "$SMPROGRAMS\${APP_NAME}"
  
  MessageBox MB_OK "Uninstallation complete!$\r$\n$\r$\nPlease manually remove the Chrome extension:$\r$\n1. Go to chrome://extensions/$\r$\n2. Remove 'ArhintSigner'$\r$\n$\r$\nRestart your browser."
  
SectionEnd

; String replacement function
Function StrRep
  Exch $R4 ; $R4 = Replacement String
  Exch
  Exch $R3 ; $R3 = String to replace (needle)
  Exch 2
  Exch $R1 ; $R1 = String to do replacement in (haystack)
  Push $R2 ; Temp
  Push $R5 ; Len (needle)
  Push $R6 ; len (haystack)
  Push $R7 ; Scratch reg
  StrCpy $R2 -1
  StrLen $R5 $R3
  StrLen $R6 $R1
loop:
  IntOp $R2 $R2 + 1
  StrCpy $R7 $R1 $R5 $R2
  StrCmp $R7 "" done
  StrCmp $R7 $R3 found
  Goto loop
found:
  StrCpy $R7 $R1 $R2
  IntOp $R2 $R2 + $R5
  StrCpy $R1 $R1 "" $R2
  StrCpy $R1 $R7$R4$R1
  IntOp $R2 $R2 + -1
  IntOp $R6 $R6 + -1
  Goto loop
done:
  StrCpy $R3 $R1
  Pop $R7
  Pop $R6
  Pop $R5
  Pop $R2
  Pop $R1
  Pop $R4
  Exch $R3
FunctionEnd

!macro StrRep output string old new
  Push "${string}"
  Push "${old}"
  Push "${new}"
  Call StrRep
  Pop ${output}
!macroend
!define StrRep "!insertmacro StrRep"
