# Microsoft Developer Studio Project File - Name="tabSRMM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=tabSRMM - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "tabsrmm.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "tabsrmm.mak" CFG="tabSRMM - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "tabSRMM - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabSRMM - Win32 Release Unicode" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabSRMM - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabSRMM - Win32 Debug Unicode" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp".\Debug/srmm.pch" /YX /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp".\Debug/srmm.pch" /Yu"commonheaders.h" /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:".\Debug\srmm.pdb" /debug /machine:IX86 /out:"..\..\Bin\Debug\Plugins\tabsrmm.dll" /implib:".\Debug/srmm.lib" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib shlwapi.lib /nologo /subsystem:windows /dll /pdb:".\Debug\srmm.pdb" /debug /machine:IX86 /out:"..\..\Bin\Debug\Plugins\tabsrmm.dll" /implib:".\Debug/srmm.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /incremental:no

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release_Unicode"
# PROP BASE Intermediate_Dir ".\Release_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release_Unicode"
# PROP Intermediate_Dir ".\Release_Unicode"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /Fp".\Release_Unicode/srmm.pch" /YX /GF /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp".\Release_Unicode/srmm.pch" /Yu"commonheaders.h" /GF /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG" /d "UNICODE"
# ADD RSC /l 0x809 /fo".\tabsrmm_private.res" /d "NDEBUG" /d "UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /base:"0x6a540000" /subsystem:windows /dll /machine:IX86 /out:"..\..\Bin\Release\Plugins\tabsrmm_unicode.dll" /implib:".\Release_Unicode/srmm.lib" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib shlwapi.lib /nologo /base:"0x6a540000" /subsystem:windows /dll /map /debug /machine:IX86 /out:"..\..\Bin\Release Unicode\Plugins\tabsrmm.dll" /implib:".\Release_Unicode/srmm.lib" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp".\Release/srmm.pch" /YX /GF /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp".\Release/srmm.pch" /Yu"commonheaders.h" /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /base:"0x6a540000" /subsystem:windows /dll /machine:IX86 /out:"..\..\Bin\Release\Plugins\tabsrmm.dll" /implib:".\Release/srmm.lib" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib shlwapi.lib /nologo /base:"0x6a540000" /subsystem:windows /dll /map /debug /machine:IX86 /out:"..\..\Bin\Release\Plugins\tabsrmm.dll" /implib:".\Release/srmm.lib" /pdbtype:sept /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug_Unicode"
# PROP BASE Intermediate_Dir ".\Debug_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug_Unicode"
# PROP Intermediate_Dir ".\Debug_Unicode"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR /Fp".\Debug_Unicode/srmm.pch" /YX /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR /Fp".\Debug_Unicode/srmm.pch" /Yu"commonheaders.h" /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG" /d "UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:".\Debug_Unicode\srmm.pdb" /debug /machine:IX86 /out:"..\..\Bin\Debug\Plugins\tabsrmm_unicode.dll" /implib:".\Debug_Unicode/srmm.lib" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib shlwapi.lib /nologo /subsystem:windows /dll /pdb:".\Debug_Unicode\srmm.pdb" /debug /machine:IX86 /out:"..\..\Bin\Debug Unicode\Plugins\tabsrmm.dll" /implib:".\Debug_Unicode/srmm.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /incremental:no

!ENDIF 

# Begin Target

# Name "tabSRMM - Win32 Debug"
# Name "tabSRMM - Win32 Release Unicode"
# Name "tabSRMM - Win32 Release"
# Name "tabSRMM - Win32 Debug Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Chat"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\chat\chat.h
# End Source File
# Begin Source File

SOURCE=.\chat\chat_resource.h
# End Source File
# Begin Source File

SOURCE=.\chat\chatprototypes.h
# End Source File
# Begin Source File

SOURCE=.\chat\clist.c
DEP_CPP_CLIST=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /Yu"commonheaders.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chat\colorchooser.c
DEP_CPP_COLOR=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /Yu"commonheaders.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chat\log.c
DEP_CPP_LOG_C=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_MathModule.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /Yu"commonheaders.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chat\m_chat.h
# End Source File
# Begin Source File

SOURCE=.\chat\main.c
DEP_CPP_MAIN_=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /Yu"commonheaders.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chat\manager.c
DEP_CPP_MANAG=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /Yu"commonheaders.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chat\message.c
DEP_CPP_MESSA=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /Yu"commonheaders.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chat\options.c
DEP_CPP_OPTIO=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_MathModule.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /Yu"commonheaders.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chat\services.c
DEP_CPP_SERVI=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_MathModule.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /Yu"commonheaders.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chat\tools.c
DEP_CPP_TOOLS=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /Yu"commonheaders.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chat\window.c
DEP_CPP_WINDO=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_MathModule.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /Yu"../commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /Yu"commonheaders.h"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\container.c
DEP_CPP_CONTA=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_MathModule.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\sendqueue.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\containeroptions.c
DEP_CPP_CONTAI=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=eventpopups.c
DEP_CPP_EVENT=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_icq.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\formatting.cpp
DEP_CPP_FORMA=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_MathModule.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /nologo /GX /Zi /EHsc
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /nologo /GX /EHsc
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\generic_msghandlers.c
DEP_CPP_GENER=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hotkeyhandler.c
DEP_CPP_HOTKE=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\sendqueue.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=ImageDataObject.cpp
DEP_CPP_IMAGE=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\ImageDataObject.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=msgdialog.c
DEP_CPP_MSGDI=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_MathModule.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\sendqueue.h"\
	".\templates.h"\
	{$(INCLUDE)}"uxtheme.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /nologo /GX /Od /GZ

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /nologo /GX /Zi /O1

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /nologo /GX /O1

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /nologo /GX /Od /FR /GZ

!ENDIF 

# End Source File
# Begin Source File

SOURCE=msgdlgutils.c
DEP_CPP_MSGDL=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_MathModule.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=msglog.c
DEP_CPP_MSGLO=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_MathModule.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /nologo /GX /Od /GZ

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /nologo /GX /Zi /O1

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /nologo /GX /O1

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /nologo /GX /Od /FR /GZ

!ENDIF 

# End Source File
# Begin Source File

SOURCE=msgoptions.c
DEP_CPP_MSGOP=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_MathModule.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	{$(INCLUDE)}"uxtheme.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /nologo /GX /Od /GZ

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /nologo /GX /Zi /O1

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /nologo /GX /O1

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /nologo /GX /Od /FR /GZ

!ENDIF 

# End Source File
# Begin Source File

SOURCE=msgs.c
DEP_CPP_MSGS_=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_MathModule.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\m_toptoolbar.h"\
	".\m_updater.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\sendqueue.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /nologo /GX /Od /GZ

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /nologo /GX /Zi /O1

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /nologo /GX /O1

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /nologo /GX /Od /FR /GZ

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\selectcontainer.c
DEP_CPP_SELEC=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=sendqueue.c
DEP_CPP_SENDQ=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\sendqueue.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=srmm.c
DEP_CPP_SRMM_=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

# ADD CPP /nologo /GX /Od /Yc"commonheaders.h" /GZ

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /nologo /GX /Zi /O1 /Yc"commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

# ADD CPP /nologo /GX /O1 /Yc"commonheaders.h"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

# ADD CPP /nologo /GX /Od /FR /Yc"commonheaders.h" /GZ

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tabctrl.c
DEP_CPP_TABCT=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	{$(INCLUDE)}"uxtheme.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=templates.c
DEP_CPP_TEMPL=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\themes.c
DEP_CPP_THEME=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=trayicon.c
DEP_CPP_TRAYI=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\m_toptoolbar.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=TSButton.c
DEP_CPP_TSBUT=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=userprefs.c
DEP_CPP_USERP=\
	"..\..\include\m_addcontact.h"\
	"..\..\include\m_avatars.h"\
	"..\..\include\m_button.h"\
	"..\..\include\m_chat.h"\
	"..\..\include\m_clc.h"\
	"..\..\include\m_clist.h"\
	"..\..\include\m_clui.h"\
	"..\..\include\m_contacts.h"\
	"..\..\include\m_database.h"\
	"..\..\include\m_file.h"\
	"..\..\include\m_history.h"\
	"..\..\include\m_icolib.h"\
	"..\..\include\m_langpack.h"\
	"..\..\include\m_message.h"\
	"..\..\include\m_options.h"\
	"..\..\include\m_plugins.h"\
	"..\..\include\m_popup.h"\
	"..\..\include\m_protocols.h"\
	"..\..\include\m_protomod.h"\
	"..\..\include\m_protosvc.h"\
	"..\..\include\m_skin.h"\
	"..\..\include\m_stdhdr.h"\
	"..\..\include\m_system.h"\
	"..\..\include\m_userinfo.h"\
	"..\..\include\m_utils.h"\
	"..\..\include\newpluginapi.h"\
	"..\..\include\statusmodes.h"\
	"..\..\include\win2k.h"\
	".\chat\chat.h"\
	".\chat\chatprototypes.h"\
	".\commonheaders.h"\
	".\functions.h"\
	".\generic_msghandlers.h"\
	".\m_cln_skinedit.h"\
	".\m_fingerprint.h"\
	".\m_flash.h"\
	".\m_ieview.h"\
	".\m_metacontacts.h"\
	".\m_nudge.h"\
	".\m_smileyadd.h"\
	".\m_spellchecker.h"\
	".\msgdlgutils.h"\
	".\msgs.h"\
	".\nen.h"\
	".\templates.h"\
	

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

# ADD CPP /Zi

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=commonheaders.h
# End Source File
# Begin Source File

SOURCE=functions.h
# End Source File
# Begin Source File

SOURCE=.\generic_msghandlers.h
# End Source File
# Begin Source File

SOURCE=IcoLib.h
# End Source File
# Begin Source File

SOURCE=ImageDataObject.h
# End Source File
# Begin Source File

SOURCE=m_fontservice.h
# End Source File
# Begin Source File

SOURCE=m_ieview.h
# End Source File
# Begin Source File

SOURCE=m_MathModule.h
# End Source File
# Begin Source File

SOURCE=m_message.h
# End Source File
# Begin Source File

SOURCE=m_metacontacts.h
# End Source File
# Begin Source File

SOURCE=.\m_popup.h
# End Source File
# Begin Source File

SOURCE=m_smileyadd.h
# End Source File
# Begin Source File

SOURCE=m_Snapping_windows.h
# End Source File
# Begin Source File

SOURCE=.\m_spellchecker.h
# End Source File
# Begin Source File

SOURCE=msgdlgutils.h
# End Source File
# Begin Source File

SOURCE=msgs.h
# End Source File
# Begin Source File

SOURCE=nen.h
# End Source File
# Begin Source File

SOURCE=resource.h
# End Source File
# Begin Source File

SOURCE=sendqueue.h
# End Source File
# Begin Source File

SOURCE=templates.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=".\res\angeli-icons\Add.ico"
# End Source File
# Begin Source File

SOURCE="..\..\Miranda-IM\res\addcontact.ico"
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\AddXP.ico"
# End Source File
# Begin Source File

SOURCE=".\res\arrow-down.ico"
# End Source File
# Begin Source File

SOURCE=.\res\check.ico
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\Close.ico"
# End Source File
# Begin Source File

SOURCE=.\res\delete.ico
# End Source File
# Begin Source File

SOURCE=res\Details32.ico
# End Source File
# Begin Source File

SOURCE=res\Details8.ico
# End Source File
# Begin Source File

SOURCE="..\..\Miranda-IM\res\dragcopy.cur"
# End Source File
# Begin Source File

SOURCE=..\..\src\res\dragcopy.cur
# End Source File
# Begin Source File

SOURCE="..\..\Miranda-IM\res\dropuser.cur"
# End Source File
# Begin Source File

SOURCE=..\..\src\res\dropuser.cur
# End Source File
# Begin Source File

SOURCE=.\res\error.ico
# End Source File
# Begin Source File

SOURCE="..\..\Miranda-IM\res\history.ico"
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\History.ico"
# End Source File
# Begin Source File

SOURCE=res\History32.ico
# End Source File
# Begin Source File

SOURCE=res\History8.ico
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\HistoryXP.ico"
# End Source File
# Begin Source File

SOURCE="..\..\Miranda-IM\res\hyperlin.cur"
# End Source File
# Begin Source File

SOURCE=..\..\src\res\hyperlin.cur
# End Source File
# Begin Source File

SOURCE=..\..\src\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\in.ico
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\Incom.ico"
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\Info.ico"
# End Source File
# Begin Source File

SOURCE=.\res\leftarrow.ico
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\Multi.ico"
# End Source File
# Begin Source File

SOURCE="..\..\Miranda-IM\res\multisend.ico"
# End Source File
# Begin Source File

SOURCE=res\Multisend32.ico
# End Source File
# Begin Source File

SOURCE=res\Multisend8.ico
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\MultiXP.ico"
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\Opt.ico"
# End Source File
# Begin Source File

SOURCE=.\res\options.ico
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\OptXP.ico"
# End Source File
# Begin Source File

SOURCE=.\res\out.ico
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\Outg.ico"
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\Pencil.ico"
# End Source File
# Begin Source File

SOURCE=.\res\Photo32.ico
# End Source File
# Begin Source File

SOURCE=.\res\Photo8.ico
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\Pic.ico"
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\PicXP.ico"
# End Source File
# Begin Source File

SOURCE=.\res\pulldown.ico
# End Source File
# Begin Source File

SOURCE=.\res\pulldown1.ico
# End Source File
# Begin Source File

SOURCE=.\res\pullup.ico
# End Source File
# Begin Source File

SOURCE=.\res\quote.ico
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\Quote02.ico"
# End Source File
# Begin Source File

SOURCE="..\..\Miranda-IM\res\rename.ico"
# End Source File
# Begin Source File

SOURCE=.\res\rightarrow.ico
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\Save.ico"
# End Source File
# Begin Source File

SOURCE=.\res\save.ico
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\SaveXP.ico"
# End Source File
# Begin Source File

SOURCE=".\res\angeli-icons\Send.ico"
# End Source File
# Begin Source File

SOURCE=.\res\smbutton.ico
# End Source File
# Begin Source File

SOURCE=.\res\smiley_xp.ico
# End Source File
# Begin Source File

SOURCE=.\tabsrmm_private.rc
# End Source File
# Begin Source File

SOURCE=res\Typing32.ico
# End Source File
# Begin Source File

SOURCE=res\Typing8.ico
# End Source File
# Begin Source File

SOURCE="..\..\Miranda-IM\res\viewdetails.ico"
# End Source File
# End Group
# Begin Group "Misc Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\docs\changelog.txt
# End Source File
# Begin Source File

SOURCE=langpacks\langpack_tabsrmm_german.txt
# End Source File
# Begin Source File

SOURCE=.\Makefile.ansi
# End Source File
# Begin Source File

SOURCE=MAKEFILE.W32
# End Source File
# Begin Source File

SOURCE=.\docs\MetaContacts.TXT
# End Source File
# Begin Source File

SOURCE=.\docs\Popups.txt
# End Source File
# Begin Source File

SOURCE=.\docs\Readme.icons
# End Source File
# Begin Source File

SOURCE=.\docs\readme.txt
# End Source File
# End Group
# End Target
# End Project
