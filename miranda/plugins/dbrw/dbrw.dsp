# Microsoft Developer Studio Project File - Name="dbrw" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=dbrw - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dbrw.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dbrw.mak" CFG="dbrw - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dbrw - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dbrw - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dbrw - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DBRW_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../include" /D _WIN32_WINNT=0x0500 /D "DBRW_OLD_PLUGINAPI_SUPPORT" /D "NO_TCL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DBRW_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../../bin/release/plugins/dbx_rw.dll"

!ELSEIF  "$(CFG)" == "dbrw - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DBRW_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D _WIN32_WINNT=0x0500 /D "DBRW_OLD_PLUGINAPI_SUPPORT" /D "DBRW_DEBUG" /D "DBRW_LOGGING" /D "NO_TCL" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DBRW_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../../bin/debug/plugins/dbx_rw.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "dbrw - Win32 Release"
# Name "dbrw - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\contacts.c
# End Source File
# Begin Source File

SOURCE=.\dbrw.c
# End Source File
# Begin Source File

SOURCE=.\events.c
# End Source File
# Begin Source File

SOURCE=.\settings.c
# End Source File
# Begin Source File

SOURCE=.\sql.c
# End Source File
# Begin Source File

SOURCE=.\utf8.c
# End Source File
# Begin Source File

SOURCE=.\utils.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\dbrw.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "SQLite"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sqlite3\alter.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\analyze.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\attach.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\auth.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\btree.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\btree.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\btreeInt.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\build.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\callback.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\complete.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\date.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\delete.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\expr.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\func.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\hash.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\hash.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\insert.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\keywordhash.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\legacy.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\limits.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\loadext.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\main.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\malloc.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\opcodes.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\opcodes.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\os.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\os.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\os_common.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\os_os2.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\os_os2.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\os_unix.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\os_win.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\pager.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\pager.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\parse.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\parse.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\pragma.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\prepare.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\printf.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\random.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\select.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\sqlite3.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\sqlite3ext.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\sqliteInt.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\table.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\tokenize.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\trigger.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\update.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\utf.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\util.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\vacuum.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbe.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbe.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbeapi.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbeaux.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbeblob.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbefifo.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbeInt.h
# End Source File
# Begin Source File

SOURCE=.\sqlite3\vdbemem.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\vtab.c
# End Source File
# Begin Source File

SOURCE=.\sqlite3\where.c
# End Source File
# End Group
# End Target
# End Project
