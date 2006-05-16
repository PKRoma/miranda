!include "MUI.nsh"
!include "Sections.nsh"

Name "Miranda IM 0.3.3.1"
OutFile "miranda-im-v0.3.3.1.exe"

InstallDir "$PROGRAMFILES\Miranda IM"
InstallDirRegKey HKLM "Software\Miranda" "Install_Dir"
SetCompressor lzma
SetOverWrite on

!packhdr "temp_installer.dat" "upx -9 temp_installer.dat"

!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install-blue.ico"
!define MUI_COMPONENTSPAGE_NODESC
!define MUI_HEADERIMAGE
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
BrandingText "www.miranda-im.org"

VAR INST_MODE
VAR INST_UPGRADE
VAR INST_ININEW

Section "Miranda IM (core)"
  SectionIn RO
  ; Remove Old Install Files
  Delete "$INSTDIR\Docs\Contributors.txt"
  Delete "$INSTDIR\Docs\Readme.txt"
  Delete "$INSTDIR\Docs\License.txt"
  Delete "$INSTDIR\Docs\Contrib\AIM_*.*"
  Delete "$INSTDIR\Docs\Contrib\IRC_*.*"
  Delete "$INSTDIR\Docs\Contrib\ICQ_*.*"
  Delete "$INSTDIR\Docs\Contrib\Jabber_*.*"
  Delete "$INSTDIR\Docs\Contrib\MSN_*.*"
  RMDir "$INSTDIR\Docs\Contrib"
  Delete "$INSTDIR\Docs\mirandaboot.ini"
  Delete "$INSTDIR\gpl.txt"
  Delete "$INSTDIR\Uninstall\unins000.dat"
  Delete "$INSTDIR\Uninstall\unins000.exe"
  RMDir  "$INSTDIR\Uninstall"
  
  SetOutPath "$INSTDIR"
  File "..\..\Bin\Release\miranda32.exe"
  File "..\..\Bin\Release\dbtool.exe"
  SetOutPath "$INSTDIR\Plugins"
  File "..\..\Bin\Release\Plugins\srmm.dll"
  SetOutPath "$INSTDIR\Docs"
  File /oname="SRMM-License.txt"  "..\..\bin\release\docs\SRMM-license.txt"
  File /oname="SRMM-Readme.txt"  "..\..\bin\release\docs\SRMM-readme.txt"

  SetOutPath "$INSTDIR"
  StrCmp $INST_ININEW "0" noiniu
  File "..\..\Miranda-IM\Docs\mirandaboot.ini"
  noiniu:
  File /oname=Contributors.txt "..\..\Miranda-IM\docs\credits.txt"
  File /oname=Readme.txt "..\..\Miranda-IM\docs\releasenotes.txt"
  File /oname=License.txt "..\..\Miranda-IM\docs\license.txt"
SectionEnd

SubSection /e "Protocols" p0
  
  Section "AIM" p1
    SetOutPath "$INSTDIR\Plugins"
    File "..\..\Bin\Release\Plugins\AIM.dll"
    SetOutPath "$INSTDIR\Docs"
    File /oname="AIM-License.txt"  "..\..\bin\release\docs\aim-license.txt"
    File /oname="AIM-Readme.txt"  "..\..\bin\release\docs\aim-readme.txt"
  SectionEnd

  Section "ICQ" p2
    SetOutPath "$INSTDIR\Plugins"
    File "..\..\Bin\Release\Plugins\ICQ.dll"
    SetOutPath "$INSTDIR\Docs"
    File /oname="ICQ-License.txt"  "..\..\bin\release\docs\icq-license.txt"
    File /oname="ICQ-Readme.txt"  "..\..\bin\release\docs\icq-readme.txt"
  SectionEnd

  Section "IRC" p3
    SetOutPath "$INSTDIR\Plugins"
    File "..\..\Bin\Release\Plugins\IRC.dll"
    SetOverWrite off
    File "..\..\Bin\Release\Plugins\IRC_Servers.ini"
    SetOverWrite on
    SetOutPath "$INSTDIR\Docs"
    File /oname="IRC-License.txt"  "..\..\Bin\Release\Docs\IRC_license.txt"
    File /oname="IRC-Readme.txt"  "..\..\Bin\Release\Docs\IRC_Readme.txt"
  SectionEnd

  Section "Jabber" p4
    SetOutPath "$INSTDIR\Plugins"
    File "..\..\Bin\Release\Plugins\Jabber.dll"
    SetOutPath "$INSTDIR\Docs"
    File /oname="Jabber-License.txt"  "..\..\Protocols\Jabber\docs\gpl.txt"
    File /oname="Jabber-Readme.txt"  "..\..\Bin\Release\Docs\readme_jabber.txt"
  SectionEnd

  Section "MSN" p5
    SetOutPath "$INSTDIR\Plugins"
    File "..\..\Bin\Release\Plugins\MSN.dll"
    SetOutPath "$INSTDIR\Docs"
    File /oname="MSN-License.txt"  "..\..\bin\release\docs\gpl.txt"
    File /oname="MSN-Readme.txt"  "..\..\bin\release\docs\readme-msn.txt"
  SectionEnd  

  ;Section "Yahoo" p6
  ;  SetOutPath "$INSTDIR\Plugins"
  ;  File "..\..\Bin\Release\Plugins\Yahoo.dll"
  ;  SetOutPath "$INSTDIR\Docs"
  ;  File /oname="Yahoo-License.txt"  "..\..\Protocols\Yahoo\Docs\License.txt"
  ;  File /oname="Yahoo-Readme.txt"  "..\..\Protocols\Yahoo\Docs\ReadMe.txt"
  ;SectionEnd

SubSectionEnd

Section "Import Plugin"
    SetOutPath "$INSTDIR\Plugins"
    File "..\..\Bin\Release\Plugins\import.dll"
    SetOutPath "$INSTDIR\Docs"
    File /oname="Import-License.txt"  "..\..\Bin\Release\Docs\import-license.txt"
    File /oname="Import-Readme.txt"  "..\..\Bin\Release\Docs\import-readme.txt"
SectionEnd

SubSection /e "Options"

  Section "Install Icons"
    SetOutPath "$INSTDIR\Icons"
    ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
    StrCmp $R0 '5.1' xpicons
    StrCmp $R0 '5.2' xpicons
    
    File "icons\bin\locolor\proto_AIM.dll"
    File "icons\bin\locolor\proto_ICQ.dll"
    File "icons\bin\locolor\proto_IRC.dll"
    File "icons\bin\locolor\proto_JABBER.dll"
    File "icons\bin\locolor\proto_MSN.dll"
    ;File "icons\bin\locolor\proto_YAHOO.dll"
    Goto endicons
    xpicons:
    File "icons\bin\hicolor\proto_AIM.dll"
    File "icons\bin\hicolor\proto_ICQ.dll"
    File "icons\bin\hicolor\proto_IRC.dll"
    File "icons\bin\hicolor\proto_JABBER.dll"
    File "icons\bin\hicolor\proto_MSN.dll"
    ;File "icons\bin\hicolor\proto_YAHOO.dll"
    endicons:
  SectionEnd

  Section "Install for All Users"
    StrCpy $INST_MODE "1"
    SetShellVarContext "all"
    StrCmp $INST_ININEW "0" nowriteappdata
    StrCmp $INST_UPGRADE "1" nowriteappdata
    ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
    StrCmp $R0 '5.0' writeappdata
    StrCmp $R0 '5.1' writeappdata
    StrCmp $R0 '5.2' writeappdata
    Goto nowriteappdata
    writeappdata:
    WriteINIStr "$INSTDIR\mirandaboot.ini" "Database" "ProfileDir" "%APPDATA%\Miranda"
    nowriteappdata:
  SectionEnd

  Section "Install Start Menu Shortcuts"
      SetOutPath "$INSTDIR"
      RMDir /r "$SMPROGRAMS\Miranda IM"
      CreateDirectory "$SMPROGRAMS\Miranda IM"
      CreateShortCut  "$SMPROGRAMS\Miranda IM\Miranda IM.lnk" "$INSTDIR\miranda32.exe"
      CreateShortCut  "$SMPROGRAMS\Miranda IM\License.lnk" "$INSTDIR\License.txt"
      CreateShortCut  "$SMPROGRAMS\Miranda IM\Database Repair Tool.lnk" "$INSTDIR\dbtool.exe"
      WriteINIStr     "$SMPROGRAMS\Miranda IM\Homepage.url" "InternetShortcut" "URL" "http://www.miranda-im.org/"
      CreateShortCut  "$SMPROGRAMS\Miranda IM\Plugins.lnk" "$INSTDIR\Plugins\"
  SectionEnd

  Section "Install Desktop Shortcuts"
    SetOutPath "$INSTDIR"
    CreateShortCut  "$DESKTOP\Miranda IM.lnk" "$INSTDIR\miranda32.exe"
    CreateShortCut  "$QUICKLAUNCH\Miranda IM.lnk" "$INSTDIR\miranda32.exe"
  SectionEnd

SubSectionEnd

Section -
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM" "DisplayName" "Miranda IM" 
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section Uninstall
  SetShellVarContext "all"
  RMDir /r "$SMPROGRAMS\Miranda IM"
  Delete "$DESKTOP\Miranda IM.lnk"
  Delete "$QUICKLAUNCH\Miranda IM.lnk"
  SetShellVarContext "current"
  RMDir /r "$SMPROGRAMS\Miranda IM"
  Delete "$DESKTOP\Miranda IM.lnk"
  Delete "$QUICKLAUNCH\Miranda IM.lnk"

  RMDir /r "$INSTDIR\Docs"
  RMDir /r "$INSTDIR\Icons"
  RMDir /r "$INSTDIR\Plugins"
  Delete "$INSTDIR\dbtool.exe"
  Delete "$INSTDIR\miranda32.exe"
  Delete "$INSTDIR\mirandaboot.ini"
  Delete "$INSTDIR\License.txt"
  Delete "$INSTDIR\Contributors.txt"
  Delete "$INSTDIR\Readme.txt"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$INSTDIR\config.exe"
  RMDir "$INSTDIR"
  DeleteRegKey HKLM "SOFTWARE\Miranda"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM"
SectionEnd

Function .onInit
  StrCpy $INST_MODE "0"
  StrCpy $INST_UPGRADE "0"
  StrCpy $INST_ININEW "1"
  SetShellVarContext "current"
  StrCpy $0 "0"
  FindWindow $R0 "Miranda"
  IsWindow $R0 showwarn
  FindWindow $R0 "Miranda IM"
  IsWindow $R0 0 norun
  showwarn:
  MessageBox MB_OK "Miranda IM is currently running.  It is recommended that you close Miranda IM so the installation can complete successfully."
  Sleep 1000
  norun:
FunctionEnd

Function .onVerifyInstDir
  IfFileExists "$INSTDIR\miranda32.exe" "" endupgrade
  StrCpy $INST_UPGRADE "1"
  Goto endupgradex
  endupgrade:
  StrCpy $INST_UPGRADE "0"
  endupgradex:
  IfFileExists "$INSTDIR\mirandaboot.ini" "" endini
  StrCpy $INST_ININEW "0"
  Goto endinix
  endini:
  StrCpy $INST_ININEW "1"
  endinix:
  StrCmp $INST_UPGRADE "1" vupgrade
  !insertmacro SetSectionFlag ${p1} ${SF_RO}
  !insertmacro SetSectionFlag ${p2} ${SF_RO}
  !insertmacro SetSectionFlag ${p3} ${SF_RO}
  !insertmacro SetSectionFlag ${p4} ${SF_RO}
  !insertmacro SetSectionFlag ${p5} ${SF_RO}
  ;!insertmacro SetSectionFlag ${p6} ${SF_RO}
  !insertmacro SetSectionFlag ${p1} ${SF_SELECTED}
  !insertmacro SetSectionFlag ${p2} ${SF_SELECTED}
  !insertmacro SetSectionFlag ${p3} ${SF_SELECTED}
  !insertmacro SetSectionFlag ${p4} ${SF_SELECTED}
  !insertmacro SetSectionFlag ${p5} ${SF_SELECTED}
  ;!insertmacro SetSectionFlag ${p6} ${SF_SELECTED}
  !insertmacro SetSectionFlag ${p0} ${SF_RO}
  !insertmacro ClearSectionFlag ${p0} ${SF_EXPAND}
  SectionSetText ${p1} ""
  SectionSetText ${p2} ""
  SectionSetText ${p3} ""
  SectionSetText ${p4} ""
  SectionSetText ${p5} ""
  ;SectionSetText ${p6} ""
  SectionSetText ${p0} ""
  goto vupgradeend
  vupgrade:
  !insertmacro ClearSectionFlag ${p1} ${SF_RO}
  !insertmacro ClearSectionFlag ${p2} ${SF_RO}
  !insertmacro ClearSectionFlag ${p3} ${SF_RO}
  !insertmacro ClearSectionFlag ${p4} ${SF_RO}
  !insertmacro ClearSectionFlag ${p5} ${SF_RO}
  ;!insertmacro ClearSectionFlag ${p6} ${SF_RO}
  !insertmacro ClearSectionFlag ${p0} ${SF_RO}
  !insertmacro SetSectionFlag ${p0} ${SF_EXPAND}
  SectionSetText ${p1} "AIM"
  SectionSetText ${p2} "ICQ"
  SectionSetText ${p3} "IRC"
  SectionSetText ${p4} "Jabber"
  SectionSetText ${p5} "MSN"
  ;SectionSetText ${p6} "Yahoo"
  SectionSetText ${p0} "Protocols"
  vupgradeend:
FunctionEnd

Function .onInstSuccess
  StrCmp $INST_UPGRADE "1" noconfig
  SetOutPath "$INSTDIR"
  File config.exe
  Exec "$INSTDIR\config.exe"
  Delete /REBOOTOK "$INSTDIR\config.exe"
  Goto endinstall
  noconfig:
  SetOutPath "$INSTDIR"
  Exec "$INSTDIR\miranda32.exe"
  endinstall:
FunctionEnd
