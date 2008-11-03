!include "MUI.nsh"
!include "Sections.nsh"
!include "WinVer.nsh"
!include "LogicLib.nsh"

!define MIM_NAME                "Miranda IM"
!define MIM_VERSION             "0.7.11"
!define MIM_PREVIEW             "0" ; 0 for final build

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


!if  ${MIM_PREVIEW} != 0
Name                            "${MIM_NAME} ${MIM_VERSION} Preview Release ${MIM_PREVIEW}"
OutFile                         "..\..\miranda\bin\miranda-im-v${MIM_VERSION}-pr${MIM_PREVIEW}-${MIM_BUILD_TYPE}.exe"
!else
Name                            "${MIM_NAME} ${MIM_VERSION}"
OutFile                         "..\..\miranda\bin\miranda-im-v${MIM_VERSION}-${MIM_BUILD_TYPE}.exe"
!endif

InstallDir                      "$PROGRAMFILES\Miranda IM"
InstallDirRegKey                HKLM "Software\Miranda" "Install_Dir"
SetCompressor                   lzma
SetOverWrite                    on
BrandingText                    "www.miranda-im.org"

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

Section "Miranda IM"
  SectionIn RO
  !insertmacro PrintInstallerDetails "Installing Miranda IM Core Files..."

  SetOutPath "$INSTDIR"
  File "${MIM_BUILD_DIR}\miranda32.exe"
  File "${MIM_BUILD_DIR}\dbtool.exe"
  File "${MIM_BUILD_DIR}\zlib.dll"
  File "${MIM_BUILD_DIRANSI}\winssl.dll"
  File "${MIM_BUILD_SRC}\docs\contributors.txt"
  File "${MIM_BUILD_SRC}\docs\readme.txt"
  File "${MIM_BUILD_SRC}\docs\license.txt"
  
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
  File "${MIM_BUILD_DIRANSI}\plugins\dbx_mmap.dll"
  !else
  File "${MIM_BUILD_DIRANSI}\plugins\dbx_3x.dll"
  !endif
  File "${MIM_BUILD_DIR}\plugins\chat.dll"
  
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM" "DisplayName" "Miranda IM ${MIM_VERSION}" 
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Miranda IM" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

SubSection /e "Protocols"
  Section "AIM"
    !insertmacro PrintInstallerDetails "Installing AIM Protocol..."
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIRANSI}\plugins\Aim.dll"
    !insertmacro InstallMirandaProtoIcon "AIM"
  SectionEnd
  
  Section "Gadu-Gadu"
    !insertmacro PrintInstallerDetails "Installing Gadu-Gadu Protocol..."
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIRANSI}\plugins\GG.dll"
    ; GG uses embedded icons
  SectionEnd
  
  Section "ICQ"
    !insertmacro PrintInstallerDetails "Installing ICQ Protocol..."
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIRANSI}\plugins\icq.dll"
    SetOutPath "$INSTDIR\Icons"
    File "${MIM_BUILD_DIRANSI}\Icons\xstatus_ICQ.dll"
    !insertmacro InstallMirandaProtoIcon "ICQ"
  SectionEnd

  Section "IRC"
    !insertmacro PrintInstallerDetails "Installing IRC Protocol..."
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIR}\plugins\irc.dll"
    ${If} $INST_UPGRADE = 0
      SetOverWrite off
      File "${MIM_BUILD_SRC}\protocols\IRCG\Docs\irc_servers.ini"
      SetOverWrite on
    ${EndIf}
    !insertmacro InstallMirandaProtoIcon "IRC"
  SectionEnd

  Section "Jabber" JABBER
    !insertmacro PrintInstallerDetails "Installing Jabber Protocol..."
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIR}\plugins\jabber.dll"
    SetOutPath "$INSTDIR\Icons"
    !insertmacro InstallMirandaProtoIcon "Jabber"
  SectionEnd

  Section "MSN"
    !insertmacro PrintInstallerDetails "Installing MSN Protocol..."
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIR}\plugins\msn.dll"
    !insertmacro InstallMirandaProtoIcon "MSN"
  SectionEnd

  Section "Yahoo"
    !insertmacro PrintInstallerDetails "Installing Yahoo Protocol..."
    SetOutPath "$INSTDIR\Plugins"
    File "${MIM_BUILD_DIRANSI}\plugins\yahoo.dll"
    !insertmacro InstallMirandaProtoIcon "Yahoo"
  SectionEnd
SubSectionEnd

Section "Import Plugin"
  !insertmacro PrintInstallerDetails "Installing Import Plugin..."
  SetOutPath "$INSTDIR\Plugins"
  File "${MIM_BUILD_DIR}\plugins\import.dll"
SectionEnd

SubSection /e "Options" pOptions
  Section "Install Start Menu Shortcuts"
    !insertmacro PrintInstallerDetails "Installing Start Menu Shortcuts..."
    SetOutPath "$INSTDIR"
    RMDir /r "$SMPROGRAMS\Miranda IM"
    CreateDirectory "$SMPROGRAMS\Miranda IM"
    CreateShortCut  "$SMPROGRAMS\Miranda IM\Miranda IM.lnk" "$INSTDIR\miranda32.exe"
    CreateShortCut  "$SMPROGRAMS\Miranda IM\Database Tool.lnk" "$INSTDIR\dbtool.exe"
    WriteINIStr     "$SMPROGRAMS\Miranda IM\Homepage.url" "InternetShortcut" "URL" "http://www.miranda-im.org/"
  SectionEnd

  Section "Install Desktop Shortcut"
    !insertmacro PrintInstallerDetails "Installing Desktop Shortcut..."
    SetOutPath "$INSTDIR"
    CreateShortCut  "$DESKTOP\Miranda IM.lnk" "$INSTDIR\miranda32.exe"
  SectionEnd

  Section "Install Quicklaunch Shortcut"
    !insertmacro PrintInstallerDetails "Installing Quicklaunch Shortcut..."
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
  Delete "$INSTDIR\winssl.dll"
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
FunctionEnd

