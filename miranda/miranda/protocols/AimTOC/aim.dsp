# Microsoft Developer Studio Project File - Name="aim" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=aim - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "aim.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aim.mak" CFG="aim - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aim - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "aim - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "aim - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AIM_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AIM_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /machine:I386 /out:"../../bin/release/plugins/AIM.dll" /ALIGN:4096 /ignore:4108
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "aim - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AIM_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AIM_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../../bin/debug/plugins/AIM.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "aim - Win32 Release"
# Name "aim - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\aim.c
# End Source File
# Begin Source File

SOURCE=.\buddies.c
# End Source File
# Begin Source File

SOURCE=.\evilmode.c
# End Source File
# Begin Source File

SOURCE=.\filerecv.c
# End Source File
# Begin Source File

SOURCE=.\firstrun.c
# End Source File
# Begin Source File

SOURCE=.\groupchat.c
# End Source File
# Begin Source File

SOURCE=.\groups.c
# End Source File
# Begin Source File

SOURCE=.\idle.c
# End Source File
# Begin Source File

SOURCE=.\im.c
# End Source File
# Begin Source File

SOURCE=.\keepalive.c
# End Source File
# Begin Source File

SOURCE=.\links.c
# End Source File
# Begin Source File

SOURCE=.\list.c
# End Source File
# Begin Source File

SOURCE=.\log.c
# End Source File
# Begin Source File

SOURCE=.\options.c
# End Source File
# Begin Source File

SOURCE=.\password.c
# End Source File
# Begin Source File

SOURCE=.\pthread.c
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=.\search.c
# End Source File
# Begin Source File

SOURCE=.\server.c
# End Source File
# Begin Source File

SOURCE=.\services.c
# End Source File
# Begin Source File

SOURCE=.\toc.c
# End Source File
# Begin Source File

SOURCE=.\userinfo.c
# End Source File
# Begin Source File

SOURCE=.\utils.c
# End Source File
# Begin Source File

SOURCE=.\version.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\aim.h
# End Source File
# Begin Source File

SOURCE=.\buddies.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\evilmode.h
# End Source File
# Begin Source File

SOURCE=.\filerecv.h
# End Source File
# Begin Source File

SOURCE=.\firstrun.h
# End Source File
# Begin Source File

SOURCE=.\groupchat.h
# End Source File
# Begin Source File

SOURCE=.\groups.h
# End Source File
# Begin Source File

SOURCE=.\idle.h
# End Source File
# Begin Source File

SOURCE=.\im.h
# End Source File
# Begin Source File

SOURCE=.\keepalive.h
# End Source File
# Begin Source File

SOURCE=.\links.h
# End Source File
# Begin Source File

SOURCE=.\list.h
# End Source File
# Begin Source File

SOURCE=.\log.h
# End Source File
# Begin Source File

SOURCE=.\options.h
# End Source File
# Begin Source File

SOURCE=.\password.h
# End Source File
# Begin Source File

SOURCE=.\pthread.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\search.h
# End Source File
# Begin Source File

SOURCE=.\server.h
# End Source File
# Begin Source File

SOURCE=.\services.h
# End Source File
# Begin Source File

SOURCE=.\toc.h
# End Source File
# Begin Source File

SOURCE=.\userinfo.h
# End Source File
# Begin Source File

SOURCE=.\utils.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\icos\aim.ico
# End Source File
# Begin Source File

SOURCE=.\icos\aimxp.ico
# End Source File
# Begin Source File

SOURCE=.\icos\blank.ico
# End Source File
# Begin Source File

SOURCE=.\icos\delete.ico
# End Source File
# Begin Source File

SOURCE=.\icos\groupchat.ico
# End Source File
# Begin Source File

SOURCE=.\icos\groupchata.ico
# End Source File
# Begin Source File

SOURCE=.\icos\password.ico
# End Source File
# Begin Source File

SOURCE=.\icos\show.ico
# End Source File
# Begin Source File

SOURCE=.\icos\warn.ico
# End Source File
# End Group
# End Target
# End Project
