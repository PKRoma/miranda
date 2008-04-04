!include "MUI.nsh"
!include "Sections.nsh"
!include "WinVer.nsh"

!define MIM_NAME                "Miranda IM"
!define MIM_VERSION             "0.7.4"
!define MIM_PREVIEW             "0" ; make 0 for final build

!define MIM_BUILD_UNICODE

!define MIM_BUILD_ICONS_LOW     "icons\bin\locolor"
!define MIM_BUILD_ICONS_HI      "icons\bin\hicolor"

!ifdef MIM_BUILD_UNICODE
!define MIM_BUILD_TYPE          "unicode"
!define MIM_BUILD_DIR           "..\..\miranda\bin\Release Unicode"
!else
!define MIM_BUILD_TYPE          "ansi"
!define MIM_BUILD_DIR           "..\..\miranda\bin\Release"
!endif
!define MIM_BUILD_DIRANSI       "..\..\miranda\bin\Release"
!define MIM_BUILD_SRC           "..\..\miranda"

!if MIM_PREVIEW>0
Name                            "${MIM_NAME} ${MIM_VERSION} Preview Release ${MIM_PREVIEW}"
OutFile                         "miranda-im-v${MIM_VERSION}-pr${MIM_PREVIEW}-${MIM_BUILD_TYPE}.exe"
!else
Name                            "${MIM_NAME} ${MIM_VERSION}"
OutFile                         "miranda-im-v${MIM_VERSION}-${MIM_BUILD_TYPE}.exe"
!endif

InstallDir                      "$PROGRAMFILES\Miranda IM"
InstallDirRegKey                HKLM "Software\Miranda" "Install_Dir"
SetCompressor                   lzma
SetOverWrite                    on
BrandingText                    "www.miranda-im.org"

VAR INST_UPGRADE

!packhdr "temp_installer.dat" "upx -9 temp_installer.dat"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "Graphics\header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "Graphics\header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "Graphics\welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "Graphics\welcome.bmp"
!define MUI_ICON "Graphics\install.ico"
!define MUI_UNICON "Graphics\uninstall.ico"
!define MUI_ABORTWARNING
!define MUI_COMPONENTSPAGE_NODESC
!define MUI_LICENSEPAGE_BGCOLOR /grey
!define MUI_FINISHPAGE_RUN $INSTDIR\miranda32.exe
!define MUI_FINISHPAGE_RUN_TEXT "Start Miranda IM"
!define MUI_FINISHPAGE_SHOWREADME $INSTDIR\readme.txt
!define MUI_FINISHPAGE_SHOWREADME_TEXT "View Readme"
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!define MUI_FINISHPAGE_LINK "Donate to Miranda IM"
!define MUI_FINISHPAGE_LINK_LOCATION "http://www.miranda-im.org/donate/"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${MIM_BUILD_SRC}\docs\license.txt"
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE VerifyInstallDir
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

!macro InstallMirandaIcon IconFile
  SetOutPath "$INSTDIR\Icons"
  SetOverWrite off
  !ifdef MIM_BUILD_UNICODE
  ${If} ${AtLeastWinXP}
  File "${MIM_BUILD_ICONS_HI}\${IconFile}"
  ${Else}
  File "${MIM_BUILD_ICONS_LOW}\${IconFile}"
  ${EndIf}
  !else
  File "${MIM_BUILD_ICONS_LOW}\${IconFile}"
  !endif
  SetOverWrite on
!macroend

Section "Miranda IM"
  SectionIn RO
  SetDetailsPrint textonly
  DetailPrint "Installing Miranda IM Core Files..."
  SetDetailsPrint listonly

  SetOutPath "$INSTDIR"
  File "${MIM_BUILD_DIR}\miranda32.exe"
  File "${MIM_BUILD_DIR}\dbtool.exe"
  File "${MIM_BUILD_DIR}\zlib.dll"
  File "${MIM_BUILD_SRC}\docs\contributors.txt"
  File "${MIM_BUILD_SRC}\docs\readme.txt"
  File "${MIM_BUILD_SRC}\docs\license.txt"

  StrCmp $INST_UPGRADE "1" nosavemirandaboot
  SetOverWrite off
  File "${MIM_BUILD_SRC}\docs\mirandaboot.ini"
  SetOverWrite on
  nosavemirandaboot:
    
  SetOutPath "$INSTDIR\Plugins"
  File "${MIM_BUILD_DIR}\plugins\clist_classic.dll"
  File "${MIM_BUILD_DIR}\plugins\srmm.dll"
  File "${MIM_BUILD_DIR}\plugins\avs.dll"
  File "${MIM_BUILD_DIRANSI}\plugins\advaimg.dll"
  !ifdef MIM_BUILD_UNICODE
  File "${MIM_BUILD_DIRANSI}\plugins\dbx_mmap.dll"
  Delete "$INSTDIR\Plugins\dbx_3x.dll" ; remove old dbx_3x.dll
  !else
  File "${MIM_BUILD_DIRANSI}\plugins\dbx_3x.dll"
  !endif
  File "${MIM_BUILD_DIR}\plugins\chat.dll"
  
  Delete "$INSTDIR\Plugins\png2dib.dll" ; remove old png2dib.dll
  
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM_is1" ; remove old uninstaller key
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM" "DisplayName" "Miranda IM ${MIM_VERSION}" 
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

SubSection /e "Protocols"
  Section "AIM"
    SetDetailsPrint textonly
    DetailPrint "Installing AIM Protocol..."
    SetDetailsPrint listonly
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIRANSI}\plugins\Aim.dll"
    !insertmacro InstallMirandaIcon "proto_AIM.dll"
  SectionEnd
  
  Section "Gadu-Gadu"
    SetDetailsPrint textonly
    DetailPrint "Installing Gadu-Gadu Protocol..."
    SetDetailsPrint listonly
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIRANSI}\plugins\GG.dll"
    ;!insertmacro InstallMirandaIcon "proto_GG.dll"
    ;GG icons are embedded int the protocol dll
  SectionEnd
  
  Section "ICQ"
    SetDetailsPrint textonly
    DetailPrint "Installing ICQ Protocol..."
    SetDetailsPrint listonly
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIRANSI}\plugins\icq.dll"
    SetOutPath "$INSTDIR\Icons"
    File "${MIM_BUILD_DIRANSI}\Icons\xstatus_ICQ.dll"
    !insertmacro InstallMirandaIcon "proto_ICQ.dll"
  SectionEnd

  Section "IRC"
    SetDetailsPrint textonly
    DetailPrint "Installing IRC Protocol..."
    SetDetailsPrint listonly
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIR}\plugins\irc.dll"
    StrCmp $INST_UPGRADE "1" nosaveircservers
    SetOverWrite off
    File "${MIM_BUILD_SRC}\protocols\IRCG\Docs\irc_servers.ini"
    SetOverWrite on
    nosaveircservers:
    !insertmacro InstallMirandaIcon "proto_IRC.dll"
  SectionEnd

  Section "Jabber"
    SetDetailsPrint textonly
    DetailPrint "Installing Jabber Protocol..."
    SetDetailsPrint listonly
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIR}\plugins\jabber.dll"
    SetOutPath "$INSTDIR\Icons"
    !insertmacro InstallMirandaIcon "proto_Jabber.dll"
  SectionEnd

  Section "MSN"
    SetDetailsPrint textonly
    DetailPrint "Installing MSN Protocol..."
    SetDetailsPrint listonly
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIR}\plugins\msn.dll"
    !insertmacro InstallMirandaIcon "proto_MSN.dll"
  SectionEnd

  Section "Yahoo"
    SetDetailsPrint textonly
    DetailPrint "Installing Yahoo Protocol..."
    SetDetailsPrint listonly
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIRANSI}\plugins\yahoo.dll"
    !insertmacro InstallMirandaIcon "proto_Yahoo.dll"
  SectionEnd
SubSectionEnd

Section "Import Plugin"
  SetDetailsPrint textonly
  DetailPrint "Installing Import Plugin..."
  SetDetailsPrint listonly
  SetOutPath "$INSTDIR\Plugins"
  File "${MIM_BUILD_DIR}\plugins\import.dll"
SectionEnd

SubSection /e "Options" pOptions
  Section "Install Start Menu Shortcuts"
    SetDetailsPrint textonly
    DetailPrint "Installing Start Menu Shortcuts..."
    SetDetailsPrint listonly
    SetOutPath "$INSTDIR"
    RMDir /r "$SMPROGRAMS\Miranda IM"
    CreateDirectory "$SMPROGRAMS\Miranda IM"
    CreateShortCut  "$SMPROGRAMS\Miranda IM\Miranda IM.lnk" "$INSTDIR\miranda32.exe"
    CreateShortCut  "$SMPROGRAMS\Miranda IM\Database Tool.lnk" "$INSTDIR\dbtool.exe"
    WriteINIStr     "$SMPROGRAMS\Miranda IM\Homepage.url" "InternetShortcut" "URL" "http://www.miranda-im.org/"
  SectionEnd

  Section "Install Desktop Shortcut"
    SetDetailsPrint textonly
    DetailPrint "Installing Desktop Shortcut..."
    SetDetailsPrint listonly
    SetOutPath "$INSTDIR"
    CreateShortCut  "$DESKTOP\Miranda IM.lnk" "$INSTDIR\miranda32.exe"
  SectionEnd

  Section "Install Quicklaunch Shortcut"
    SetDetailsPrint textonly
    DetailPrint "Installing Quicklaunch Shortcut..."
    SetDetailsPrint listonly
    SetOutPath "$INSTDIR"
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
  !ifdef MIM_BUILD_UNICODE
  StrCmp $INST_UPGRADE "1" "" noupgrade
  !insertmacro ClearSectionFlag ${pStoreData} ${SF_SELECTED}
  SectionSetText ${pStoreData} ""
  !insertmacro SetSectionFlag ${pOptions} ${SF_EXPAND}
  Goto noupgradeend
  noupgrade:
  SectionSetText ${pStoreData} "Store profile data in user home directory"
  !insertmacro SetSectionFlag ${pStoreData} ${SF_SELECTED}
  noupgradeend:
  !endif
FunctionEnd

