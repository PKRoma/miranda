# Microsoft Developer Studio Project File - Name="icqlib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=icqlib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "icqlib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "icqlib.mak" CFG="icqlib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "icqlib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "icqlib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/icqlib", XAAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "icqlib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Gf /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MSVC_" /YX /FD /c
# ADD BASE RSC /l 0x1009 /d "NDEBUG"
# ADD RSC /l 0x1009 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "icqlib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_MSVC_" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x1009 /d "_DEBUG"
# ADD RSC /l 0x1009 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "icqlib - Win32 Release"
# Name "icqlib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\chatsession.c
# End Source File
# Begin Source File

SOURCE=.\contacts.c
# End Source File
# Begin Source File

SOURCE=.\cyrillic.c
# End Source File
# Begin Source File

SOURCE=.\filesession.c
# End Source File
# Begin Source File

SOURCE=.\icqbyteorder.c
# End Source File
# Begin Source File

SOURCE=.\icqlib.c
# End Source File
# Begin Source File

SOURCE=.\icqpacket.c
# End Source File
# Begin Source File

SOURCE=.\list.c
# End Source File
# Begin Source File

SOURCE=.\proxy.c
# End Source File
# Begin Source File

SOURCE=.\queue.c
# End Source File
# Begin Source File

SOURCE=.\stdpackets.c
# End Source File
# Begin Source File

SOURCE=.\tcp.c
# End Source File
# Begin Source File

SOURCE=.\tcpchathandle.c
# End Source File
# Begin Source File

SOURCE=.\tcpfilehandle.c
# End Source File
# Begin Source File

SOURCE=.\tcphandle.c
# End Source File
# Begin Source File

SOURCE=.\tcplink.c
# End Source File
# Begin Source File

SOURCE=.\udp.c
# End Source File
# Begin Source File

SOURCE=.\udphandle.c

!IF  "$(CFG)" == "icqlib - Win32 Release"

# SUBTRACT CPP /FA<none>

!ELSEIF  "$(CFG)" == "icqlib - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\chatsession.h
# End Source File
# Begin Source File

SOURCE=.\contacts.h
# End Source File
# Begin Source File

SOURCE=.\filesession.h
# End Source File
# Begin Source File

SOURCE=.\icq.h
# End Source File
# Begin Source File

SOURCE=.\icqbyteorder.h
# End Source File
# Begin Source File

SOURCE=.\icqlib.h
# End Source File
# Begin Source File

SOURCE=.\icqpacket.h
# End Source File
# Begin Source File

SOURCE=.\icqtypes.h
# End Source File
# Begin Source File

SOURCE=.\list.h
# End Source File
# Begin Source File

SOURCE=.\queue.h
# End Source File
# Begin Source File

SOURCE=.\stdpackets.h
# End Source File
# Begin Source File

SOURCE=.\tcp.h
# End Source File
# Begin Source File

SOURCE=.\tcplink.h
# End Source File
# Begin Source File

SOURCE=.\udp.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# End Group
# End Target
# End Project
