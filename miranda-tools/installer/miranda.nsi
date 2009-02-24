!include "MUI.nsh"
!include "Sections.nsh"
!include "WinVer.nsh"
!include "LogicLib.nsh"

!define MIM_NAME                "Miranda IM"
!define MIM_VERSION             "0.8"
!define MIM_PREVIEW             "1" ; 0 for final build

!define MIM_BUILD_ICONS_LOW     "icons\bin\locolor"
!define MIM_BUILD_ICONS_HI      "icons\bin\hicolor"
!define MIM_BUILD_OPTIONS_FILE  "miranda32.lst"
!define MIM_BUILD_OPTIONS_SECT  "InstalledSections"

!ifdef MIM_BUILD_UNICODE
!define MIM_BUILD_TYPE          "unicode"
!define MIM_BUILD_DIR           "..\..\miranda\bin\Release Unicode"
!else
!define MIM_BUILD_TYPE          "ansi"
!define MIM_BUILD_DIR           "..\..\miranda\bin\Release"
!endif
!define MIM_BUILD_DIRANSI       "..\..\miranda\bin\Release"
!define MIM_BUILD_SRC           "..\..\miranda"


!if  ${MIM_PREVIEW} != 0
Name                            "${MIM_NAME} ${MIM_VERSION} Preview Release ${MIM_PREVIEW}"
OutFile                         "..\..\miranda\bin\miranda-im-v${MIM_VERSION}-pr${MIM_PREVIEW}-${MIM_BUILD_TYPE}.exe"
!else
Name                            "${MIM_NAME} ${MIM_VERSION}"
OutFile                         "..\..\miranda\bin\miranda-im-v${MIM_VERSION}-${MIM_BUILD_TYPE}.exe"
!endif

InstallDir                      "$PROGRAMFILES\Miranda IM"
InstallDirRegKey                HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\miranda32.exe" "Path"
SetCompressor                   lzma
SetOverWrite                    on
BrandingText                    "miranda-im.org"

VAR INST_UPGRADE
var INST_SUCCESS

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

!macro PrintInstallerDetails Details
  SetDetailsPrint textonly
  DetailPrint "${Details}"
  SetDetailsPrint listonly
!macroend

!macro InstallMirandaProtoIcon IconFile
  SetOutPath "$INSTDIR\Icons"
  SetOverWrite off
  !ifdef MIM_BUILD_UNICODE
  ${If} ${AtLeastWinXP}
    File "${MIM_BUILD_ICONS_HI}\proto_${IconFile}.dll"
  ${Else}
    File "${MIM_BUILD_ICONS_LOW}\proto_${IconFile}.dll"
  ${EndIf}
  !else
  File "${MIM_BUILD_ICONS_LOW}\proto_${IconFile}.dll"
  !endif
  SetOverWrite on
!macroend

!macro WriteInstallerOption IniOption IniValue
  SetOutPath "$INSTDIR"
  WriteINIStr "$INSTDIR\${MIM_BUILD_OPTIONS_FILE}" "${MIM_BUILD_OPTIONS_SECT}" "${IniValue}" "${IniOption}"
!macroend

Section "Miranda IM"
  SectionIn RO
  !insertmacro PrintInstallerDetails "Installing Miranda IM Core Files..."
  
  !insertmacro WriteInstallerOption "0" "AIM"
  !insertmacro WriteInstallerOption "0" "Gadu-Gadu"
  !insertmacro WriteInstallerOption "0" "ICQ"
  !insertmacro WriteInstallerOption "0" "IRC"
  !insertmacro WriteInstallerOption "0" "Jabber"
  !insertmacro WriteInstallerOption "0" "MSN"
  !insertmacro WriteInstallerOption "0" "Yahoo"
  !insertmacro WriteInstallerOption "0" "Import"
  !insertmacro WriteInstallerOption "0" "StartMenuShortCut"
  !insertmacro WriteInstallerOption "0" "DesktopShortCut"
  !insertmacro WriteInstallerOption "0" "QuickLaunchShortCut"

  SetOutPath "$INSTDIR"
  File "${MIM_BUILD_DIR}\miranda32.exe"
  File "${MIM_BUILD_DIR}\dbtool.exe"
  File "${MIM_BUILD_DIR}\zlib.dll"
  File "${MIM_BUILD_SRC}\docs\contributors.txt"
  File "${MIM_BUILD_SRC}\docs\readme.txt"
  File "${MIM_BUILD_SRC}\docs\license.txt"
  ; winssl.dll only needed in 0.7.x
  Delete "$INSTDIR\winssl.dll"
  
  ${If} $INST_UPGRADE = 0
    SetOverWrite off
    File "${MIM_BUILD_SRC}\docs\mirandaboot.ini"
    SetOverWrite on
  ${EndIf}
    
  SetOutPath "$INSTDIR\Plugins"
  File "${MIM_BUILD_DIR}\plugins\clist_classic.dll"
  File "${MIM_BUILD_DIR}\plugins\srmm.dll"
  File "${MIM_BUILD_DIR}\plugins\avs.dll"
  File "${MIM_BUILD_DIRANSI}\plugins\advaimg.dll"
  !ifdef MIM_BUILD_UNICODE
  File "${MIM_BUILD_DIR}\plugins\dbx_mmap.dll"
  !else
  File "${MIM_BUILD_DIR}\plugins\dbx_3x.dll"
  !endif
  File "${MIM_BUILD_DIR}\plugins\chat.dll"
  
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM" "DisplayName" "Miranda IM ${MIM_VERSION}" 
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\miranda32.exe" "" "$INSTDIR\miranda32.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\miranda32.exe" "Path" "$INSTDIR"

  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

SubSection /e "Protocols"
  Section "AIM" pProtoAim
    !insertmacro PrintInstallerDetails "Installing AIM Protocol..."
    !insertmacro WriteInstallerOption "1" "AIM"
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIR}\plugins\Aim.dll"
    !insertmacro InstallMirandaProtoIcon "AIM"
  SectionEnd
  
  Section "Gadu-Gadu" pProtoGaduGadu
    !insertmacro PrintInstallerDetails "Installing Gadu-Gadu Protocol..."
    !insertmacro WriteInstallerOption "1" "Gadu-Gadu"
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIRANSI}\plugins\GG.dll"
    ; GG uses embedded icons
  SectionEnd
  
  Section "ICQ" pProtoICQ
    !insertmacro PrintInstallerDetails "Installing ICQ Protocol..."
    !insertmacro WriteInstallerOption "1" "ICQ"
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIR}\plugins\icq.dll"
    SetOutPath "$INSTDIR\Icons"
    File "${MIM_BUILD_DIRANSI}\Icons\xstatus_ICQ.dll"
    !insertmacro InstallMirandaProtoIcon "ICQ"
  SectionEnd

  Section "IRC" pProtoIRC
    !insertmacro PrintInstallerDetails "Installing IRC Protocol..."
    !insertmacro WriteInstallerOption "1" "IRC"
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIR}\plugins\irc.dll"
    ${If} $INST_UPGRADE = 0
      SetOverWrite off
      File "${MIM_BUILD_SRC}\protocols\IRCG\Docs\irc_servers.ini"
      SetOverWrite on
    ${EndIf}
    !insertmacro InstallMirandaProtoIcon "IRC"
  SectionEnd

  Section "Jabber" pProtoJabber
    !insertmacro PrintInstallerDetails "Installing Jabber Protocol..."
    !insertmacro WriteInstallerOption "1" "Jabber"
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIR}\plugins\jabber.dll"
    SetOutPath "$INSTDIR\Icons"
    !insertmacro InstallMirandaProtoIcon "Jabber"
  SectionEnd

  Section "MSN" pProtoMSN
    !insertmacro PrintInstallerDetails "Installing MSN Protocol..."
    !insertmacro WriteInstallerOption "1" "MSN"
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIR}\plugins\msn.dll"
    !insertmacro InstallMirandaProtoIcon "MSN"
  SectionEnd

  Section "Yahoo" pProtoYahoo
    !insertmacro PrintInstallerDetails "Installing Yahoo Protocol..."
    !insertmacro WriteInstallerOption "1" "Yahoo"
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIRANSI}\plugins\yahoo.dll"
    !insertmacro InstallMirandaProtoIcon "Yahoo"
  SectionEnd
SubSectionEnd

Section "Import Plugin" pImport
  !insertmacro PrintInstallerDetails "Installing Import Plugin..."
  !insertmacro WriteInstallerOption "1" "Import"
  SetOutPath "$INSTDIR\Plugins"
  File "${MIM_BUILD_DIR}\plugins\import.dll"
SectionEnd

SubSection /e "Options" pOptions
  Section "Install Start Menu Shortcuts" pSCStartMenu
    !insertmacro PrintInstallerDetails "Installing Start Menu Shortcuts..."
    !insertmacro WriteInstallerOption "1" "StartMenuShortCut"
    SetOutPath "$INSTDIR"
    RMDir /r "$SMPROGRAMS\Miranda IM"
    CreateDirectory "$SMPROGRAMS\Miranda IM"
    CreateShortCut  "$SMPROGRAMS\Miranda IM\Miranda IM.lnk" "$INSTDIR\miranda32.exe"
    CreateShortCut  "$SMPROGRAMS\Miranda IM\Database Tool.lnk" "$INSTDIR\dbtool.exe"
    WriteINIStr     "$SMPROGRAMS\Miranda IM\Homepage.url" "InternetShortcut" "URL" "http://www.miranda-im.org/"
  SectionEnd

  Section "Install Desktop Shortcut" pSCDesktop
    !insertmacro PrintInstallerDetails "Installing Desktop Shortcut..."
    !insertmacro WriteInstallerOption "1" "DesktopShortCut"
    SetOutPath "$INSTDIR"
    CreateShortCut  "$DESKTOP\Miranda IM.lnk" "$INSTDIR\miranda32.exe"
  SectionEnd

  Section "Install Quicklaunch Shortcut" pSCQuickLaunch
    !insertmacro PrintInstallerDetails "Installing Quicklaunch Shortcut..."
    !insertmacro WriteInstallerOption "1" "QuickLaunchShortCut"
    SetOutPath "$INSTDIR"
    CreateShortCut  "$QUICKLAUNCH\Miranda IM.lnk" "$INSTDIR\miranda32.exe"
  SectionEnd

  !ifdef MIM_BUILD_UNICODE
  Section /o "Store profile data in user home directory" pStoreData
    !insertmacro PrintInstallerDetails "Configuring profile directory..."
    ${If} $INST_UPGRADE = 0
      WriteINIStr "$INSTDIR\mirandaboot.ini" "Database" "ProfileDir" "%APPDATA%\Miranda"
    ${EndIf}
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
  Delete "$INSTDIR\zlib.dll"
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
  FindWindow $R0 "Miranda"
  IsWindow $R0 showwarn
  FindWindow $R0 "Miranda IM"
  IsWindow $R0 0 norun
  showwarn:
  MessageBox MB_OK "Miranda IM is currently running.  It is recommended that you close Miranda IM so the installation can complete successfully."
  Sleep 1000
  norun:
  StrCpy $INST_SUCCESS 0
FunctionEnd

Function .onInstSuccess
  StrCpy $INST_SUCCESS 1
FunctionEnd

Function VerifyInstallDir
  ${If} ${FileExists} "$INSTDIR\miranda32.exe"
    StrCpy $INST_UPGRADE 1
  ${Else}
    StrCpy $INST_UPGRADE 0
  ${EndIf}
  !ifdef MIM_BUILD_UNICODE
  ${If} $INST_UPGRADE = 1
    !insertmacro ClearSectionFlag ${pStoreData} ${SF_SELECTED}
    SectionSetText ${pStoreData} ""
    !insertmacro SetSectionFlag ${pOptions} ${SF_EXPAND}
  ${Else}
    SectionSetText ${pStoreData} "Store profile data in user home directory"
    !insertmacro SetSectionFlag ${pStoreData} ${SF_SELECTED}
  ${EndIf}
  !endif
  
  ReadINIStr $0 "$INSTDIR\${MIM_BUILD_OPTIONS_FILE}" ${MIM_BUILD_OPTIONS_SECT} "AIM"
  ${If} $0 == "0"
    !insertmacro ClearSectionFlag ${pProtoAim} ${SF_SELECTED}
  ${Else}
    !insertmacro SetSectionFlag ${pProtoAim} ${SF_SELECTED}
  ${EndIf}
  ReadINIStr $0 "$INSTDIR\${MIM_BUILD_OPTIONS_FILE}" ${MIM_BUILD_OPTIONS_SECT} "Gadu-Gadu"
  ${If} $0 == "0"
    !insertmacro ClearSectionFlag ${pProtoGaduGadu} ${SF_SELECTED}
  ${Else}
    !insertmacro SetSectionFlag ${pProtoGaduGadu} ${SF_SELECTED}
  ${EndIf}
  ReadINIStr $0 "$INSTDIR\${MIM_BUILD_OPTIONS_FILE}" ${MIM_BUILD_OPTIONS_SECT} "ICQ"
  ${If} $0 == "0"
    !insertmacro ClearSectionFlag ${pProtoICQ} ${SF_SELECTED}
  ${Else}
    !insertmacro SetSectionFlag ${pProtoICQ} ${SF_SELECTED}
  ${EndIf}
  ReadINIStr $0 "$INSTDIR\${MIM_BUILD_OPTIONS_FILE}" ${MIM_BUILD_OPTIONS_SECT} "IRC"
  ${If} $0 == "0"
    !insertmacro ClearSectionFlag ${pProtoIRC} ${SF_SELECTED}
  ${Else}
    !insertmacro SetSectionFlag ${pProtoIRC} ${SF_SELECTED}
  ${EndIf}
  ReadINIStr $0 "$INSTDIR\${MIM_BUILD_OPTIONS_FILE}" ${MIM_BUILD_OPTIONS_SECT} "Jabber"
  ${If} $0 == "0"
    !insertmacro ClearSectionFlag ${pProtoJabber} ${SF_SELECTED}
  ${Else}
    !insertmacro SetSectionFlag ${pProtoJabber} ${SF_SELECTED}
  ${EndIf}
  ReadINIStr $0 "$INSTDIR\${MIM_BUILD_OPTIONS_FILE}" ${MIM_BUILD_OPTIONS_SECT} "MSN"
  ${If} $0 == "0"
    !insertmacro ClearSectionFlag ${pProtoMSN} ${SF_SELECTED}
  ${Else}
    !insertmacro SetSectionFlag ${pProtoMSN} ${SF_SELECTED}
  ${EndIf}
  ReadINIStr $0 "$INSTDIR\${MIM_BUILD_OPTIONS_FILE}" ${MIM_BUILD_OPTIONS_SECT} "Yahoo"
  ${If} $0 == "0"
    !insertmacro ClearSectionFlag ${pProtoYahoo} ${SF_SELECTED}
  ${Else}
    !insertmacro SetSectionFlag ${pProtoYahoo} ${SF_SELECTED}
  ${EndIf}
  ReadINIStr $0 "$INSTDIR\${MIM_BUILD_OPTIONS_FILE}" ${MIM_BUILD_OPTIONS_SECT} "Import"
  ${If} $0 == "0"
    !insertmacro ClearSectionFlag ${pImport} ${SF_SELECTED}
  ${Else}
    !insertmacro SetSectionFlag ${pImport} ${SF_SELECTED}
  ${EndIf}
  ReadINIStr $0 "$INSTDIR\${MIM_BUILD_OPTIONS_FILE}" ${MIM_BUILD_OPTIONS_SECT} "StartMenuShortCut"
  ${If} $0 == "0"
    !insertmacro ClearSectionFlag ${pSCStartMenu} ${SF_SELECTED}
  ${Else}
    !insertmacro SetSectionFlag ${pSCStartMenu} ${SF_SELECTED}
  ${EndIf}
  ReadINIStr $0 "$INSTDIR\${MIM_BUILD_OPTIONS_FILE}" ${MIM_BUILD_OPTIONS_SECT} "DesktopShortCut"
  ${If} $0 == "0"
    !insertmacro ClearSectionFlag ${pSCDesktop} ${SF_SELECTED}
  ${Else}
    !insertmacro SetSectionFlag ${pSCDesktop} ${SF_SELECTED}
  ${EndIf}
  ReadINIStr $0 "$INSTDIR\${MIM_BUILD_OPTIONS_FILE}" ${MIM_BUILD_OPTIONS_SECT} "QuickLaunchShortCut"
  ${If} $0 == "0"
    !insertmacro ClearSectionFlag ${pSCQuickLaunch} ${SF_SELECTED}
  ${Else}
    !insertmacro SetSectionFlag ${pSCQuickLaunch} ${SF_SELECTED}
  ${EndIf}
FunctionEnd

