# Microsoft Developer Studio Project File - Name="clist_nicer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=clist_nicer - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "clist.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "clist.mak" CFG="clist_nicer - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "clist_nicer - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "clist_nicer - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "clist_nicer - Win32 Release Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "clist_nicer - Win32 Debug Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Miranda/miranda/plugins/clist_nicer", JBOAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Yu"commonheaders.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /i "../../include/" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib msimg32.lib advapi32.lib shlwapi.lib /nologo /base:"0x6590000" /dll /map /debug /machine:I386 /out:"../../bin/release/plugins/clist_nicer.dll" /OPT:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Yu"commonheaders.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /i "../../include/" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib msimg32.lib advapi32.lib gdiplus.lib delayimp.lib shlwapi.lib /nologo /base:"0x6590000" /dll /debug /machine:I386 /out:"../../bin/debug/plugins/clist_nicer.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /incremental:no

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "clist___Win32_Release_Unicode"
# PROP BASE Intermediate_Dir "clist___Win32_Release_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_Unicode"
# PROP Intermediate_Dir "Release_Unicode"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Yu"commonheaders.h" /FD /c
# ADD CPP /nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Yu"commonheaders.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /i "../../include/" /d "NDEBUG"
# ADD RSC /l 0x809 /i "../../include/" /d "NDEBUG" /d "UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib msimg32.lib advapi32.lib /nologo /base:"0x6590000" /dll /machine:I386 /out:"../../bin/release/plugins/clist_nicer.dll"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib msimg32.lib advapi32.lib shlwapi.lib /nologo /base:"0x6590000" /dll /map /debug /machine:I386 /out:"../../bin/Release Unicode/plugins/clist_nicer.dll" /OPT:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "clist___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "clist___Win32_Debug_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_Unicode"
# PROP Intermediate_Dir "Debug_Unicode"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Yu"commonheaders.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /i "../../include/" /d "_DEBUG"
# ADD RSC /l 0x809 /i "../../include/" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib msimg32.lib advapi32.lib /nologo /base:"0x6590000" /dll /debug /machine:I386 /out:"../../bin/debug/plugins/clist_nicer.dll" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib msimg32.lib advapi32.lib gdiplus.lib delayimp.lib shlwapi.lib /nologo /base:"0x6590000" /dll /debug /machine:I386 /out:"../../bin/Debug Unicode/plugins/clist_nicer.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /incremental:no

!ENDIF 

# Begin Target

# Name "clist_nicer - Win32 Release"
# Name "clist_nicer - Win32 Debug"
# Name "clist_nicer - Win32 Release Unicode"
# Name "clist_nicer - Win32 Debug Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "CLUIFrames"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CLUIFrames\cluiframes.c
# ADD CPP /Yu"../commonheaders.h"
# End Source File
# Begin Source File

SOURCE=.\CLUIFrames\cluiframes.h
# End Source File
# Begin Source File

SOURCE=.\CLUIFrames\framesmenu.c
# ADD CPP /Yu"../commonheaders.h"
# End Source File
# Begin Source File

SOURCE=.\CLUIFrames\groupmenu.c
# ADD CPP /Yu"../commonheaders.h"
# End Source File
# End Group
# Begin Source File

SOURCE=.\alphablend.c
# End Source File
# Begin Source File

SOURCE=.\clc.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CLCButton.c
# End Source File
# Begin Source File

SOURCE=.\clcidents.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clcitems.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clcmsgs.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clcopts.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clcpaint.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clcutils.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clistevents.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clistmenus.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clistmod.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clistopts.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clistsettings.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clisttray.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clnplus.cpp

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /GX
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD CPP /GR- /GX
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clui.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cluiopts.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cluiservices.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\commonheaders.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yc"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

# ADD CPP /Yc

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yc"commonheaders.h"
# ADD CPP /Yc"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

# ADD CPP /Yc

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\contact.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\coolsb\coolsblib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\coolsb\coolscroll.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Docking.c
# End Source File
# Begin Source File

SOURCE=.\extBackg.c
# End Source File
# Begin Source File

SOURCE=.\forkthread.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\init.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CLUIFrames\movetogroup.c
# ADD CPP /Yu"../commonheaders.h"
# End Source File
# Begin Source File

SOURCE=.\rowheight_funcs.c
# End Source File
# Begin Source File

SOURCE=.\statusbar.c
# End Source File
# Begin Source File

SOURCE=.\statusfloater.c
# End Source File
# Begin Source File

SOURCE=.\viewmodes.c
# End Source File
# Begin Source File

SOURCE=.\wallpaper.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\clc.h
# End Source File
# Begin Source File

SOURCE=.\clist.h
# End Source File
# Begin Source File

SOURCE=.\commonheaders.h
# End Source File
# Begin Source File

SOURCE=.\forkthread.h
# End Source File
# Begin Source File

SOURCE=.\m_cln_skinedit.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\addgoupp.ico
# End Source File
# Begin Source File

SOURCE=".\res\arrow-down.ico"
# End Source File
# Begin Source File

SOURCE=.\res\overlay\away.ico
# End Source File
# Begin Source File

SOURCE=.\res\blank.ico
# End Source File
# Begin Source File

SOURCE=.\res\overlay\chat.ico
# End Source File
# Begin Source File

SOURCE=.\res\connecting.ico
# End Source File
# Begin Source File

SOURCE=.\res\delete.ico
# End Source File
# Begin Source File

SOURCE=.\res\overlay\dnd.ico
# End Source File
# Begin Source File

SOURCE=.\res\dragcopy.cur
# End Source File
# Begin Source File

SOURCE=.\res\dropuser.cur
# End Source File
# Begin Source File

SOURCE=.\res\find.ico
# End Source File
# Begin Source File

SOURCE=.\res\groups.ico
# End Source File
# Begin Source File

SOURCE=.\res\hyperlin.cur
# End Source File
# Begin Source File

SOURCE=.\res\invisible.ico
# End Source File
# Begin Source File

SOURCE=.\res\overlay\invisible.ico
# End Source File
# Begin Source File

SOURCE=.\res\overlay\lunch.ico
# End Source File
# Begin Source File

SOURCE=.\res\menu.ico
# End Source File
# Begin Source File

SOURCE=.\res\minimize.ico
# End Source File
# Begin Source File

SOURCE=.\res\overlay\NA.ico
# End Source File
# Begin Source File

SOURCE=.\res\notick.ico
# End Source File
# Begin Source File

SOURCE=.\res\notick1.ico
# End Source File
# Begin Source File

SOURCE=.\res\overlay\occupied.ico
# End Source File
# Begin Source File

SOURCE=.\res\overlay\offline.ico
# End Source File
# Begin Source File

SOURCE=.\res\online.ico
# End Source File
# Begin Source File

SOURCE=.\res\overlay\online.ico
# End Source File
# Begin Source File

SOURCE=.\res\options.ico
# End Source File
# Begin Source File

SOURCE=.\res\options_clvm.ico
# End Source File
# Begin Source File

SOURCE=.\res\overlay\phone.ico
# End Source File
# Begin Source File

SOURCE=.\res\rename.ico
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=.\res\slist.ico
# End Source File
# Begin Source File

SOURCE=.\res\sounds_off.ico
# End Source File
# Begin Source File

SOURCE=.\res\sounds_on.ico
# End Source File
# Begin Source File

SOURCE=.\res\tabsrmm_menu.ico
# End Source File
# Begin Source File

SOURCE=.\res\visible.ico
# End Source File
# End Group
# End Target
# End Project
