# Microsoft Developer Studio Project File - Name="miranda32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=miranda32 - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "miranda32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "miranda32.mak" CFG="miranda32 - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "miranda32 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "miranda32 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "miranda32 - Win32 Release Unicode" (based on "Win32 (x86) Application")
!MESSAGE "miranda32 - Win32 Debug Unicode" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "miranda32 - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fr /Yu"commonheaders.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 wsock32.lib comctl32.lib winmm.lib version.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"../bin/release/miranda32.exe" /fixed /ALIGN:4096 /ignore:4108
# SUBTRACT LINK32 /pdb:none /incremental:yes

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fr /Yu"commonheaders.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib version.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"../bin/debug/miranda32.exe"
# SUBTRACT LINK32 /profile /pdb:none /incremental:no

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "miranda32___Win32_Release_Unicode"
# PROP BASE Intermediate_Dir "miranda32___Win32_Release_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release_Unicode"
# PROP Intermediate_Dir ".\Release_Unicode"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /Fr /Yu"../../core/commonheaders.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Yu"commonheaders.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wsock32.lib comctl32.lib winmm.lib version.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /machine:I386 /out:"../bin/release/miranda32.exe" /fixed /ALIGN:4096 /ignore:4108
# SUBTRACT BASE LINK32 /pdb:none /debug
# ADD LINK32 wsock32.lib comctl32.lib winmm.lib version.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"../bin/Release Unicode/miranda32.exe" /fixed /ALIGN:4096 /ignore:4108
# SUBTRACT LINK32 /pdb:none /incremental:yes

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "miranda32___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "miranda32___Win32_Debug_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug_Unicode"
# PROP Intermediate_Dir ".\Debug_Unicode"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /Fr /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fr /Yu"commonheaders.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib version.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"../bin/debug/miranda32.exe"
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib version.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"../bin/Debug Unicode/miranda32.exe"
# SUBTRACT LINK32 /profile /pdb:none /incremental:no

!ENDIF 

# Begin Target

# Name "miranda32 - Win32 Release"
# Name "miranda32 - Win32 Debug"
# Name "miranda32 - Win32 Release Unicode"
# Name "miranda32 - Win32 Debug Unicode"
# Begin Group "SDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\include\m_addcontact.h
# End Source File
# Begin Source File

SOURCE=..\include\m_awaymsg.h
# End Source File
# Begin Source File

SOURCE=..\include\m_button.h
# End Source File
# Begin Source File

SOURCE=..\include\m_clc.h
# End Source File
# Begin Source File

SOURCE=..\include\m_clist.h
# End Source File
# Begin Source File

SOURCE=..\include\m_clui.h
# End Source File
# Begin Source File

SOURCE=..\include\m_contacts.h
# End Source File
# Begin Source File

SOURCE=..\include\m_database.h
# End Source File
# Begin Source File

SOURCE=..\include\m_email.h
# End Source File
# Begin Source File

SOURCE=..\include\m_file.h
# End Source File
# Begin Source File

SOURCE=..\include\m_findadd.h
# End Source File
# Begin Source File

SOURCE=..\include\m_fuse.h
# End Source File
# Begin Source File

SOURCE=..\include\m_history.h
# End Source File
# Begin Source File

SOURCE=..\include\m_icq.h
# End Source File
# Begin Source File

SOURCE=..\include\m_idle.h
# End Source File
# Begin Source File

SOURCE=..\include\m_ignore.h
# End Source File
# Begin Source File

SOURCE=..\include\m_langpack.h
# End Source File
# Begin Source File

SOURCE=..\include\m_message.h
# End Source File
# Begin Source File

SOURCE=..\include\m_netlib.h
# End Source File
# Begin Source File

SOURCE=..\include\m_options.h
# End Source File
# Begin Source File

SOURCE=..\include\m_plugins.h
# End Source File
# Begin Source File

SOURCE=..\include\m_popup.h
# End Source File
# Begin Source File

SOURCE=..\include\m_protocols.h
# End Source File
# Begin Source File

SOURCE=..\include\m_protomod.h
# End Source File
# Begin Source File

SOURCE=..\include\m_protosvc.h
# End Source File
# Begin Source File

SOURCE=..\include\m_sessions.h
# End Source File
# Begin Source File

SOURCE=..\include\m_skin.h
# End Source File
# Begin Source File

SOURCE=..\include\m_system.h
# End Source File
# Begin Source File

SOURCE=..\include\m_url.h
# End Source File
# Begin Source File

SOURCE=..\include\m_userinfo.h
# End Source File
# Begin Source File

SOURCE=..\include\m_utils.h
# End Source File
# Begin Source File

SOURCE=..\include\newpluginapi.h
# End Source File
# Begin Source File

SOURCE=..\include\statusmodes.h
# End Source File
# Begin Source File

SOURCE=..\include\win2k.h
# End Source File
# End Group
# Begin Group "Core"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\core\commonheaders.c
# ADD CPP /Yc"commonheaders.h"
# End Source File
# Begin Source File

SOURCE=.\core\commonheaders.h
# End Source File
# Begin Source File

SOURCE=.\core\forkthread.h
# End Source File
# Begin Source File

SOURCE=.\core\memory.c
# End Source File
# Begin Source File

SOURCE=.\core\miranda.c

!IF  "$(CFG)" == "miranda32 - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\core\miranda.h
# End Source File
# Begin Source File

SOURCE=.\core\modules.c

!IF  "$(CFG)" == "miranda32 - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

# ADD BASE CPP /Yu"commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\core\modules.h
# End Source File
# End Group
# Begin Group "Modules"

# PROP Default_Filter ""
# Begin Group "addcontact"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\addcontact\addcontact.c

!IF  "$(CFG)" == "miranda32 - Win32 Release"

# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

# ADD BASE CPP /Yu"../../core/commonheaders.h"
# ADD CPP /Yu"commonheaders.h"

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

!ENDIF 

# End Source File
# End Group
# Begin Group "autoaway"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\autoaway\autoaway.c
# End Source File
# End Group
# Begin Group "button"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\button\button.c
# End Source File
# End Group
# Begin Group "contacts"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\contacts\contacts.c
# End Source File
# End Group
# Begin Group "database"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\database\database.c
# End Source File
# Begin Source File

SOURCE=.\modules\database\dbini.c
# ADD CPP /Yu"../../core/commonheaders.h"
# End Source File
# Begin Source File

SOURCE=.\modules\database\dblists.c
# End Source File
# Begin Source File

SOURCE=.\modules\database\dblists.h
# End Source File
# Begin Source File

SOURCE=.\modules\database\dbtime.c
# End Source File
# Begin Source File

SOURCE=.\modules\database\dbutils.c
# End Source File
# Begin Source File

SOURCE=.\modules\database\profilemanager.c
# End Source File
# Begin Source File

SOURCE=.\modules\database\profilemanager.h
# End Source File
# End Group
# Begin Group "findadd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\findadd\findadd.c
# End Source File
# Begin Source File

SOURCE=.\modules\findadd\findadd.h
# End Source File
# Begin Source File

SOURCE=.\modules\findadd\searchresults.c
# End Source File
# End Group
# Begin Group "help"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\help\about.c
# End Source File
# Begin Source File

SOURCE=.\modules\help\help.c
# End Source File
# End Group
# Begin Group "history"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\history\history.c
# End Source File
# End Group
# Begin Group "idle"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\idle\idle.c
# End Source File
# End Group
# Begin Group "ignore"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\ignore\ignore.c
# End Source File
# End Group
# Begin Group "langpack"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\langpack\langpack.c
# End Source File
# Begin Source File

SOURCE=.\modules\langpack\lpservices.c
# End Source File
# End Group
# Begin Group "netlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\netlib\netlib.c
# End Source File
# Begin Source File

SOURCE=.\modules\netlib\netlib.h
# End Source File
# Begin Source File

SOURCE=.\modules\netlib\netlibbind.c
# End Source File
# Begin Source File

SOURCE=.\modules\netlib\netlibhttp.c
# End Source File
# Begin Source File

SOURCE=.\modules\netlib\netlibhttpproxy.c
# End Source File
# Begin Source File

SOURCE=.\modules\netlib\netliblog.c
# End Source File
# Begin Source File

SOURCE=.\modules\netlib\netlibopenconn.c
# End Source File
# Begin Source File

SOURCE=.\modules\netlib\netlibopts.c
# End Source File
# Begin Source File

SOURCE=.\modules\netlib\netlibpktrecver.c
# End Source File
# Begin Source File

SOURCE=.\modules\netlib\netlibsecurity.c
# End Source File
# Begin Source File

SOURCE=.\modules\netlib\netlibsock.c
# End Source File
# Begin Source File

SOURCE=.\modules\netlib\netlibupnp.c
# End Source File
# End Group
# Begin Group "options"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\options\options.c
# End Source File
# End Group
# Begin Group "plugins"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\plugins\newplugins.c
# End Source File
# End Group
# Begin Group "protocols"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\protocols\protoaccs.c
# End Source File
# Begin Source File

SOURCE=.\modules\protocols\protochains.c
# End Source File
# Begin Source File

SOURCE=.\modules\protocols\protocols.c
# End Source File
# Begin Source File

SOURCE=.\modules\protocols\protoint.c
# End Source File
# Begin Source File

SOURCE=.\modules\protocols\protoopts.c
# End Source File
# End Group
# Begin Group "skin"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\skin\hotkeys.c
# End Source File
# Begin Source File

SOURCE=.\modules\skin\skinicons.c
# End Source File
# Begin Source File

SOURCE=.\modules\skin\sounds.c
# End Source File
# End Group
# Begin Group "srauth"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\srauth\auth.c
# End Source File
# Begin Source File

SOURCE=.\modules\srauth\authdialogs.c
# End Source File
# End Group
# Begin Group "srawaymsg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\srawaymsg\awaymsg.c
# End Source File
# Begin Source File

SOURCE=.\modules\srawaymsg\sendmsg.c
# End Source File
# End Group
# Begin Group "sremail"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\sremail\email.c
# End Source File
# End Group
# Begin Group "srfile"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\srfile\file.c
# End Source File
# Begin Source File

SOURCE=.\modules\srfile\file.h
# End Source File
# Begin Source File

SOURCE=.\modules\srfile\fileexistsdlg.c
# End Source File
# Begin Source File

SOURCE=.\modules\srfile\fileopts.c
# End Source File
# Begin Source File

SOURCE=.\modules\srfile\filerecvdlg.c
# End Source File
# Begin Source File

SOURCE=.\modules\srfile\filesenddlg.c
# End Source File
# Begin Source File

SOURCE=.\modules\srfile\filexferdlg.c
# End Source File
# End Group
# Begin Group "srurl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\srurl\url.c
# End Source File
# Begin Source File

SOURCE=.\modules\srurl\url.h
# End Source File
# Begin Source File

SOURCE=.\modules\srurl\urldialogs.c
# End Source File
# End Group
# Begin Group "userinfo"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\userinfo\contactinfo.c
# End Source File
# Begin Source File

SOURCE=.\modules\userinfo\stdinfo.c
# End Source File
# Begin Source File

SOURCE=.\modules\userinfo\userinfo.c
# End Source File
# End Group
# Begin Group "useronline"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\useronline\useronline.c
# End Source File
# End Group
# Begin Group "utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\utils\bmpfilter.c
# End Source File
# Begin Source File

SOURCE=.\modules\utils\colourpicker.c
# End Source File
# Begin Source File

SOURCE=.\modules\utils\hyperlink.c
# End Source File
# Begin Source File

SOURCE=.\modules\utils\md5.c
# End Source File
# Begin Source File

SOURCE=.\modules\utils\openurl.c
# End Source File
# Begin Source File

SOURCE=.\modules\utils\path.c
# End Source File
# Begin Source File

SOURCE=.\modules\utils\resizer.c
# End Source File
# Begin Source File

SOURCE=.\modules\utils\sha1.c
# End Source File
# Begin Source File

SOURCE=.\modules\utils\utf.c
# End Source File
# Begin Source File

SOURCE=.\modules\utils\utils.c
# End Source File
# Begin Source File

SOURCE=.\modules\utils\windowlist.c
# End Source File
# End Group
# Begin Group "visibility"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\visibility\visibility.c
# End Source File
# End Group
# Begin Group "clist"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\clist\clc.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\clc.h
# End Source File
# Begin Source File

SOURCE=.\modules\clist\clcfiledrop.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\clcidents.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\clcitems.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\clcmsgs.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\clcutils.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\clistcore.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\clistevents.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\clistmenus.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\clistmod.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\clistsettings.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\clisttray.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\clui.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\cluiservices.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\contact.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\Docking.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\genmenu.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\genmenu.h
# End Source File
# Begin Source File

SOURCE=.\modules\clist\genmenuopt.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\groups.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\keyboard.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\movetogroup.c
# End Source File
# Begin Source File

SOURCE=.\modules\clist\protocolorder.c
# End Source File
# End Group
# Begin Group "fontservice"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\fonts\FontOptions.c
# End Source File
# Begin Source File

SOURCE=.\modules\fonts\FontService.c
# End Source File
# Begin Source File

SOURCE=.\modules\fonts\FontService.h
# End Source File
# Begin Source File

SOURCE=.\modules\fonts\module_fonts.c
# End Source File
# Begin Source File

SOURCE=.\modules\fonts\module_fonts.h
# End Source File
# Begin Source File

SOURCE=.\modules\fonts\services.c
# End Source File
# End Group
# Begin Group "icolib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\icolib\extracticon.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\modules\icolib\IcoLib.h
# End Source File
# Begin Source File

SOURCE=.\modules\icolib\skin2icons.c
# End Source File
# End Group
# Begin Group "updatenotify"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modules\updatenotify\updatenotify.c
# End Source File
# End Group
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\miranda32.exe.manifest
# End Source File
# Begin Source File

SOURCE=.\vc6.rc
# End Source File
# End Group
# End Target
# End Project
