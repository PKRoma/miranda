!include "MUI.nsh"
!include "Sections.nsh"

!define MIM_NAME                "Miranda IM"
!define MIM_VERSION             "0.5"
!define MIM_PREVIEW             "3" ; comment out for final build

!define MIM_BUILD_UNICODE

!ifdef MIM_BUILD_UNICODE
!define MIM_BUILD_TYPE          "unicode"
!define MIM_BUILD_DIR           "..\..\miranda\bin\Release Unicode"
!define MIM_BUILD_ICONS         "icons\bin\hicolor"
!else
!define MIM_BUILD_TYPE          "ansi"
!define MIM_BUILD_DIR           "..\..\miranda\bin\Release"
!define MIM_BUILD_ICONS         "icons\bin\locolor"
!endif
!define MIM_BUILD_DIRANSI       "..\..\miranda\bin\Release"
!define MIM_BUILD_SRC           "..\..\miranda"

!ifdef MIM_PREVIEW
Name                            "${MIM_NAME} ${MIM_VERSION} Preview Release ${MIM_PREVIEW}"
OutFile                         "miranda-v${MIM_VERSION}-pr${MIM_PREVIEW}-${MIM_BUILD_TYPE}.exe"
!else
Name                            "${MIM_NAME} ${MIM_VERSION}"
OutFile                         "miranda-v${MIM_VERSION}-${MIM_BUILD_TYPE}.exe"
!endif

InstallDir                      "$PROGRAMFILES\Miranda IM"
InstallDirRegKey                HKLM "Software\Miranda" "Install_Dir"
SetCompressor                   lzma
SetOverWrite                    on
BrandingText                    "www.miranda-im.org"

VAR INST_UPGRADE

!packhdr "temp_installer.dat" "upx -9 temp_installer.dat"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\orange.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "${NSISDIR}\Contrib\Graphics\Header\orange-uninstall.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange-uninstall.bmp"
!define MUI_ABORTWARNING
!define MUI_COMPONENTSPAGE_NODESC
!define MUI_LICENSEPAGE_BGCOLOR /grey

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${MIM_BUILD_SRC}\docs\license.txt"
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE VerifyInstallDir
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "!Miranda IM"
  SectionIn RO

  Delete "$INSTDIR\Uninstall\unins000.dat"
  Delete "$INSTDIR\Uninstall\unins000.exe"
  Delete "$INSTDIR\Uninstall\unins001.dat"
  Delete "$INSTDIR\Uninstall\unins001.exe"
  RMDir  "$INSTDIR\Uninstall"

  SetOutPath "$INSTDIR"
  File "${MIM_BUILD_DIR}\miranda32.exe"
  File "${MIM_BUILD_DIR}\dbtool.exe"
  File /oname=contributors.txt "${MIM_BUILD_SRC}\docs\credits.txt"
  File /oname=readme.txt "${MIM_BUILD_SRC}\docs\releasenotes.txt"
  File /oname=license.txt "${MIM_BUILD_SRC}\docs\license.txt"

  SetOverWrite off
  File "${MIM_BUILD_SRC}\docs\mirandaboot.ini"
  SetOverWrite on

  SetOutPath "$INSTDIR\Plugins"
  File "${MIM_BUILD_DIR}\plugins\clist_classic.dll"
  File "${MIM_BUILD_DIR}\plugins\srmm.dll"
  File "${MIM_BUILD_DIRANSI}\plugins\png2dib.dll"
  File "${MIM_BUILD_DIRANSI}\plugins\dbx_3x.dll"
  File "${MIM_BUILD_DIR}\plugins\chat.dll"

  SetOutPath "$INSTDIR\Icons"
  File "${MIM_BUILD_ICONS}\proto_AIM.dll"
  File "${MIM_BUILD_ICONS}\proto_ICQ.dll"
  File "${MIM_BUILD_ICONS}\proto_IRC.dll"
  File "${MIM_BUILD_ICONS}\proto_JABBER.dll"
  File "${MIM_BUILD_ICONS}\proto_MSN.dll"
  File "${MIM_BUILD_ICONS}\proto_YAHOO.dll"
  
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM_is1" ; remove old uninstaller key
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM" "DisplayName" "Miranda IM" 
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

SubSection /e "Protocols"
  Section "AIM"
    SetOutPath "$INSTDIR\Plugins"
    File /oname=aim.dll "${MIM_BUILD_DIRANSI}\plugins\AimOscar.dll"
  SectionEnd

  Section "ICQ"
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIRANSI}\plugins\icq.dll"
  SectionEnd

  Section "IRC"
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIRANSI}\plugins\irc.dll"
    SetOverWrite off
    File "${MIM_BUILD_SRC}\protocols\IRC\Docs\IRC_Servers.ini"
    SetOverWrite on
  SectionEnd

  Section "Jabber"
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIR}\plugins\jabber.dll"
  SectionEnd

  Section "MSN"
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIR}\plugins\msn.dll"
  SectionEnd

  Section "Yahoo"
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIRANSI}\plugins\yahoo.dll"
  SectionEnd
SubSectionEnd

Section "Import Plugin"
  SetOutPath "$INSTDIR\Plugins"
  File "${MIM_BUILD_DIRANSI}\plugins\import.dll"
SectionEnd

SubSection /e "Options" pOptions
  Section "Install Start Menu Shortcuts"
      SetOutPath "$INSTDIR"
      RMDir /r "$SMPROGRAMS\Miranda IM"
      CreateDirectory "$SMPROGRAMS\Miranda IM"
      CreateShortCut  "$SMPROGRAMS\Miranda IM\Miranda IM.lnk" "$INSTDIR\miranda32.exe"
      CreateShortCut  "$SMPROGRAMS\Miranda IM\Database Repair Tool.lnk" "$INSTDIR\dbtool.exe"
      WriteINIStr     "$SMPROGRAMS\Miranda IM\Homepage.url" "InternetShortcut" "URL" "http://www.miranda-im.org/"
  SectionEnd

  Section "Install Desktop Shortcuts"
    SetOutPath "$INSTDIR"
    CreateShortCut  "$DESKTOP\Miranda IM.lnk" "$INSTDIR\miranda32.exe"
    CreateShortCut  "$QUICKLAUNCH\Miranda IM.lnk" "$INSTDIR\miranda32.exe"
  SectionEnd

  !ifdef MIM_BUILD_UNICODE
  Section /o "Store profile data in user home directory" pStoreData
    StrCmp $INST_UPGRADE "1" nowriteappdata
    WriteINIStr "$INSTDIR\mirandaboot.ini" "Database" "ProfileDir" "%APPDATA%\Miranda"
    nowriteappdata:
  SectionEnd
  !endif
SubSectionEnd

Section Uninstall
  SetShellVarContext "all"
  RMDir /r "$SMPROGRAMS\Miranda IM"
  Delete "$DESKTOP\Miranda IM.lnk"
  Delete "$QUICKLAUNCH\Miranda IM.lnk"
  SetShellVarContext "current"
  RMDir /r "$SMPROGRAMS\Miranda IM"
  Delete "$DESKTOP\Miranda IM.lnk"
  Delete "$QUICKLAUNCH\Miranda IM.lnk"

  RMDir /r "$INSTDIR\Icons"
  RMDir /r "$INSTDIR\Plugins"
  Delete "$INSTDIR\dbtool.exe"
  Delete "$INSTDIR\miranda32.exe"
  Delete "$INSTDIR\mirandaboot.ini"
  Delete "$INSTDIR\license.txt"
  Delete "$INSTDIR\contributors.txt"
  Delete "$INSTDIR\readme.txt"
  Delete "$INSTDIR\uninstall.exe"
  RMDir "$INSTDIR"

  DeleteRegKey HKLM "SOFTWARE\Miranda"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM"
SectionEnd

Function .onInit
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

Function VerifyInstallDir
  IfFileExists "$INSTDIR\miranda32.exe" "" endupgrade
  StrCpy $INST_UPGRADE "1"
  Goto endupgradex
  endupgrade:
  StrCpy $INST_UPGRADE "0"
  endupgradex:
  StrCmp $INST_UPGRADE "1" "" noupgrade
  !insertmacro ClearSectionFlag ${pStoreData} ${SF_SELECTED}
  SectionSetText ${pStoreData} ""
  !insertmacro SetSectionFlag ${pOptions} ${SF_EXPAND}
  Goto noupgradeend
  noupgrade:
  SectionSetText ${pStoreData} "Store profile data in user home directory"
  !insertmacro ClearSectionFlag ${pStoreData} ${SF_SELECTED}
  noupgradeend:
FunctionEnd

Function .onInstSuccess
  SetOutPath "$INSTDIR"
  Exec "$INSTDIR\miranda32.exe"
FunctionEnd