# Microsoft Developer Studio Project File - Name="dbsqlite" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=dbsqlite - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dbsqlite.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dbsqlite.mak" CFG="dbsqlite - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dbsqlite - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dbsqlite - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Miranda/miranda/plugins/dbsqlite", XKPAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DBSQLITE_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Zi /O1 /I "../../include/" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DBSQLITE_EXPORTS" /D "NO_TCL" /D TEMP_STORE=2 /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /debug /machine:I386 /out:"../../bin/release/plugins/dbx_sqlite.dll"
# SUBTRACT LINK32 /pdb:none /incremental:yes

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DBSQLITE_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include/" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DBSQLITE_EXPORTS" /D "NO_TCL" /D TEMP_STORE=2 /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../../bin/debug/plugins/dbx_sqlite.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /incremental:no

!ENDIF 

# Begin Target

# Name "dbsqlite - Win32 Release"
# Name "dbsqlite - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "sqlite3"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sqlite3\attach.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\auth.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\btree.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\btree.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\build.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\config.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\date.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\delete.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\expr.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\func.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\hash.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\hash.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\insert.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\legacy.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\main.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\opcodes.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\opcodes.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\os.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\os_common.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\os_mac.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\os_mac.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\os_unix.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\os_unix.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\os_win.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\os_win.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\pager.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\pager.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\parse.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\parse.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\pragma.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\printf.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\random.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\select.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\sqlite3.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\sqliteInt.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\table.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\tokenize.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\trigger.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\update.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\utf.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\util.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\vacuum.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbe.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbe.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbeapi.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbeaux.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbeInt.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbemem.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlite3\where.c

!IF  "$(CFG)" == "dbsqlite - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "dbsqlite - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\db.c
# End Source File
# Begin Source File

SOURCE=.\dbcache.c
# End Source File
# Begin Source File

SOURCE=.\init.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\commonheaders.h
# End Source File
# Begin Source File

SOURCE=.\db.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
