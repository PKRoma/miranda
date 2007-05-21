# Microsoft Developer Studio Project File - Name="icqoscar8" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=icqoscar8 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "icqoscar8.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "icqoscar8.mak" CFG="icqoscar8 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "icqoscar8 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "icqoscar8 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Miranda/miranda/protocols/icqoscarj", QLIAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "icqoscar8 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "icqoscar8_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "icqoscar8_EXPORTS" /FR /Yu"icqoscar.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x417 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /map /debug /machine:I386 /out:"../../bin/release/plugins/ICQ.dll" /ALIGN:4096 /ignore:4108
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "icqoscar8 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "icqoscar8_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "icqoscar8_EXPORTS" /FR /Yu"icqoscar.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x25000000" /dll /map /debug /debugtype:both /machine:I386 /out:"../../bin/debug/plugins/ICQ.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /incremental:no

!ENDIF 

# Begin Target

# Name "icqoscar8 - Win32 Release"
# Name "icqoscar8 - Win32 Debug"
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\UI\askauthentication.h
# End Source File
# Begin Source File

SOURCE=.\capabilities.h
# End Source File
# Begin Source File

SOURCE=.\channels.h
# End Source File
# Begin Source File

SOURCE=.\cookies.h
# End Source File
# Begin Source File

SOURCE=.\directpackets.h
# End Source File
# Begin Source File

SOURCE=.\families.h
# End Source File
# Begin Source File

SOURCE=.\forkthread.h
# End Source File
# Begin Source File

SOURCE=.\globals.h
# End Source File
# Begin Source File

SOURCE=.\guids.h
# End Source File
# Begin Source File

SOURCE=.\i18n.h
# End Source File
# Begin Source File

SOURCE=.\iconlib.h
# End Source File
# Begin Source File

SOURCE=.\icq_advsearch.h
# End Source File
# Begin Source File

SOURCE=.\icq_avatar.h
# End Source File
# Begin Source File

SOURCE=.\icq_constants.h
# End Source File
# Begin Source File

SOURCE=.\icq_db.h
# End Source File
# Begin Source File

SOURCE=.\icq_direct.h
# End Source File
# Begin Source File

SOURCE=.\icq_fieldnames.h
# End Source File
# Begin Source File

SOURCE=.\icq_http.h
# End Source File
# Begin Source File

SOURCE=.\icq_infoupdate.h
# End Source File
# Begin Source File

SOURCE=.\icq_opts.h
# End Source File
# Begin Source File

SOURCE=.\icq_packet.h
# End Source File
# Begin Source File

SOURCE=.\icq_popups.h
# End Source File
# Begin Source File

SOURCE=.\icq_rates.h
# End Source File
# Begin Source File

SOURCE=.\icq_server.h
# End Source File
# Begin Source File

SOURCE=.\icq_servlist.h
# End Source File
# Begin Source File

SOURCE=.\icq_uploadui.h
# End Source File
# Begin Source File

SOURCE=.\icq_xtraz.h
# End Source File
# Begin Source File

SOURCE=.\icqosc_svcs.h
# End Source File
# Begin Source File

SOURCE=.\icqoscar.h
# End Source File
# Begin Source File

SOURCE=.\init.h
# End Source File
# Begin Source File

SOURCE=.\log.h
# End Source File
# Begin Source File

SOURCE=.\UI\loginpassword.h
# End Source File
# Begin Source File

SOURCE=.\m_icq.h
# End Source File
# Begin Source File

SOURCE=.\md5.h
# End Source File
# Begin Source File

SOURCE=.\oscar_filetransfer.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\stdpackets.h
# End Source File
# Begin Source File

SOURCE=.\tlv.h
# End Source File
# Begin Source File

SOURCE=.\UI\userinfotab.h
# End Source File
# Begin Source File

SOURCE=.\utilities.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\icos\auth_ask.ico
# End Source File
# Begin Source File

SOURCE=.\icos\auth_grant.ico
# End Source File
# Begin Source File

SOURCE=.\icos\auth_revoke.ico
# End Source File
# Begin Source File

SOURCE=.\changeinfo\expandst.ico
# End Source File
# Begin Source File

SOURCE=.\icos\icq.ico
# End Source File
# Begin Source File

SOURCE=.\resources.rc
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus.bmp
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus01.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus02.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus03.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus04.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus05.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus06.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus07.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus08.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus09.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus10.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus11.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus12.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus13.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus14.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus15.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus16.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus17.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus18.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus19.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus20.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus21.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus22.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus23.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus24.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus25.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus26.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus27.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus28.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus29.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus30.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus31.ico
# End Source File
# Begin Source File

SOURCE=.\icos\xstatus32.ico
# End Source File
# End Group
# Begin Group "FLAP Channels"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\chan_01login.c
# End Source File
# Begin Source File

SOURCE=.\chan_02data.c
# End Source File
# Begin Source File

SOURCE=.\chan_03error.c
# End Source File
# Begin Source File

SOURCE=.\chan_04close.c
# End Source File
# Begin Source File

SOURCE=.\chan_05ping.c
# End Source File
# End Group
# Begin Group "SNAC Families"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\fam_01service.c
# End Source File
# Begin Source File

SOURCE=.\fam_02location.c
# End Source File
# Begin Source File

SOURCE=.\fam_03buddy.c
# End Source File
# Begin Source File

SOURCE=.\fam_04message.c
# End Source File
# Begin Source File

SOURCE=.\fam_09bos.c
# End Source File
# Begin Source File

SOURCE=.\fam_0alookup.c
# End Source File
# Begin Source File

SOURCE=.\fam_0bstatus.c
# End Source File
# Begin Source File

SOURCE=.\fam_13servclist.c
# End Source File
# Begin Source File

SOURCE=.\fam_15icqserver.c
# End Source File
# Begin Source File

SOURCE=.\fam_17signon.c
# End Source File
# End Group
# Begin Group "Direct Connection"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\directpackets.c
# End Source File
# Begin Source File

SOURCE=.\icq_direct.c
# End Source File
# Begin Source File

SOURCE=.\icq_directmsg.c
# End Source File
# Begin Source File

SOURCE=.\icq_filerequests.c
# End Source File
# Begin Source File

SOURCE=.\icq_filetransfer.c
# End Source File
# End Group
# Begin Group "Miranda Bits"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\icq_advsearch.c
# End Source File
# Begin Source File

SOURCE=.\icq_db.c
# End Source File
# Begin Source File

SOURCE=.\icqosc_svcs.c
# End Source File
# Begin Source File

SOURCE=.\init.c
# End Source File
# Begin Source File

SOURCE=.\log.c
# End Source File
# End Group
# Begin Group "Documentation"

# PROP Default_Filter ".txt"
# Begin Source File

SOURCE=".\docs\icq-readme.txt"
# End Source File
# Begin Source File

SOURCE=".\docs\IcqOscarJ-db settings.txt"
# End Source File
# Begin Source File

SOURCE=".\docs\IcqOscarJ-translation.txt"
# End Source File
# End Group
# Begin Group "UI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UI\askauthentication.c
# End Source File
# Begin Source File

SOURCE=.\icq_firstrun.c
# End Source File
# Begin Source File

SOURCE=.\icq_opts.c
# End Source File
# Begin Source File

SOURCE=.\icq_popups.c
# End Source File
# Begin Source File

SOURCE=.\icq_uploadui.c
# End Source File
# Begin Source File

SOURCE=.\UI\loginpassword.c
# End Source File
# Begin Source File

SOURCE=.\UI\userinfotab.c
# End Source File
# End Group
# Begin Group "Change Info"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\changeinfo\changeinfo.h
# End Source File
# Begin Source File

SOURCE=.\changeinfo\constants.c
# End Source File
# Begin Source File

SOURCE=.\changeinfo\db.c
# End Source File
# Begin Source File

SOURCE=.\changeinfo\dlgproc.c
# End Source File
# Begin Source File

SOURCE=.\changeinfo\editlist.c
# End Source File
# Begin Source File

SOURCE=.\changeinfo\editstring.c
# End Source File
# Begin Source File

SOURCE=.\changeinfo\main.c
# End Source File
# Begin Source File

SOURCE=.\changeinfo\upload.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\capabilities.c
# End Source File
# Begin Source File

SOURCE=.\cookies.c
# End Source File
# Begin Source File

SOURCE=.\forkthread.c
# End Source File
# Begin Source File

SOURCE=.\i18n.c
# End Source File
# Begin Source File

SOURCE=.\iconlib.c
# End Source File
# Begin Source File

SOURCE=.\icq_avatar.c
# End Source File
# Begin Source File

SOURCE=.\icq_clients.c
# End Source File
# Begin Source File

SOURCE=.\icq_fieldnames.c
# End Source File
# Begin Source File

SOURCE=.\icq_http.c
# End Source File
# Begin Source File

SOURCE=.\icq_infoupdate.c
# End Source File
# Begin Source File

SOURCE=.\icq_packet.c
# End Source File
# Begin Source File

SOURCE=.\icq_rates.c
# End Source File
# Begin Source File

SOURCE=.\icq_server.c
# End Source File
# Begin Source File

SOURCE=.\icq_servlist.c
# End Source File
# Begin Source File

SOURCE=.\icq_xstatus.c
# End Source File
# Begin Source File

SOURCE=.\icq_xtraz.c
# End Source File
# Begin Source File

SOURCE=.\icqoscar.c
# ADD CPP /Yc"icqoscar.h"
# End Source File
# Begin Source File

SOURCE=.\md5.c
# End Source File
# Begin Source File

SOURCE=.\oscar_filetransfer.c
# End Source File
# Begin Source File

SOURCE=.\stdpackets.c
# End Source File
# Begin Source File

SOURCE=.\tlv.c
# End Source File
# Begin Source File

SOURCE=.\utilities.c
# End Source File
# End Target
# End Project
