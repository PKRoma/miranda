# Microsoft Developer Studio Project File - Name="miranda32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=miranda32 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "miranda32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "miranda32.mak" CFG="miranda32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "miranda32 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "miranda32 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/miranda32", UBAAAAAA"
# PROP Scc_LocalPath "."
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
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib ../icqlib/release/icqlib.lib wsock32.lib libc.lib /nologo /subsystem:windows /pdb:none /machine:I386 /out:"c:/miranda/miranda32.exe"
# SUBTRACT LINK32 /debug /nodefaultlib

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
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib ../icqlib/debug/icqlib.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../debug/miranda32.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "miranda32 - Win32 Release"
# Name "miranda32 - Win32 Debug"
# Begin Group "resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Icos\checkupgrade.ico
# End Source File
# Begin Source File

SOURCE=.\icos\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\Icos\delete.ico
# End Source File
# Begin Source File

SOURCE=.\bmp\DLG_Contact_Add.bmp
# End Source File
# Begin Source File

SOURCE=.\bmp\DLG_Details.bmp
# End Source File
# Begin Source File

SOURCE=.\bmp\DLG_Message_rec.bmp
# End Source File
# Begin Source File

SOURCE=.\bmp\DLG_message_send.bmp
# End Source File
# Begin Source File

SOURCE=.\bmp\DLG_Options_Gen.bmp
# End Source File
# Begin Source File

SOURCE=.\bmp\DLG_Options_password.bmp
# End Source File
# Begin Source File

SOURCE=.\bmp\DLG_url_send.bmp
# End Source File
# Begin Source File

SOURCE=.\dnd.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\finduser.ico
# End Source File
# Begin Source File

SOURCE=.\geturl.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\help.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\history.ico
# End Source File
# Begin Source File

SOURCE=.\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\icos\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\icos\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\ico00003.ico
# End Source File
# Begin Source File

SOURCE=.\ico00004.ico
# End Source File
# Begin Source File

SOURCE=.\ico00005.ico
# End Source File
# Begin Source File

SOURCE=.\ico00006.ico
# End Source File
# Begin Source File

SOURCE=.\ico00007.ico
# End Source File
# Begin Source File

SOURCE=.\ico00008.ico
# End Source File
# Begin Source File

SOURCE=.\ico00009.ico
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\ignore.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\import.ico
# End Source File
# Begin Source File

SOURCE=.\message.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\mirandaw.ico
# End Source File
# Begin Source File

SOURCE=.\NA2.ico
# End Source File
# Begin Source File

SOURCE=.\occupied.ico
# End Source File
# Begin Source File

SOURCE=.\offline2.ico
# End Source File
# Begin Source File

SOURCE=.\online2.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\ops_icqnPass.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\ops_plugins.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\ops_proxy.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\ops_sound.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\options.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\rename.ico
# End Source File
# Begin Source File

SOURCE=.\reply.ico
# End Source File
# Begin Source File

SOURCE=.\sendurl.ico
# End Source File
# Begin Source File

SOURCE=.\bmp\Title.bmp
# End Source File
# Begin Source File

SOURCE=.\tray.ICO
# End Source File
# Begin Source File

SOURCE=.\trayonli.ico
# End Source File
# Begin Source File

SOURCE=.\unread.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\unread2.ico
# End Source File
# Begin Source File

SOURCE=.\Icos\userdetails.ico
# End Source File
# End Group
# Begin Group "headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\contact.h
# End Source File
# Begin Source File

SOURCE=.\docking.h
# End Source File
# Begin Source File

SOURCE=.\encryption.h
# End Source File
# Begin Source File

SOURCE=.\global.h
# End Source File
# Begin Source File

SOURCE=.\icqdll.h
# End Source File
# Begin Source File

SOURCE=.\import.h
# End Source File
# Begin Source File

SOURCE=.\internal.h
# End Source File
# Begin Source File

SOURCE=.\miranda.h
# End Source File
# Begin Source File

SOURCE=.\msgque.h
# End Source File
# Begin Source File

SOURCE=.\pluginapi.h
# End Source File
# Begin Source File

SOURCE=.\Plugins.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# End Group
# Begin Group "source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\callback.c
# End Source File
# Begin Source File

SOURCE=.\contact.c
# End Source File
# Begin Source File

SOURCE=.\Docking.c
# End Source File
# Begin Source File

SOURCE=.\Encrypt.c
# End Source File
# Begin Source File

SOURCE=.\globals.c
# End Source File
# Begin Source File

SOURCE=.\import.c
# End Source File
# Begin Source File

SOURCE=.\miranda.c
# End Source File
# Begin Source File

SOURCE=.\misc.c
# End Source File
# Begin Source File

SOURCE=.\msgque.c
# End Source File
# Begin Source File

SOURCE=.\options.c
# End Source File
# Begin Source File

SOURCE=.\Plugins.c
# End Source File
# Begin Source File

SOURCE=.\wndprocs.c
# End Source File
# End Group
# Begin Group "extra"

# PROP Default_Filter ""
# End Group
# Begin Group "MSN"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\MSN_Contact.c
# End Source File
# Begin Source File

SOURCE=.\MSN_global.c
# End Source File
# Begin Source File

SOURCE=.\MSN_global.h
# End Source File
# Begin Source File

SOURCE=.\MSN_Login.c
# End Source File
# Begin Source File

SOURCE=.\MSN_md5.h
# End Source File
# Begin Source File

SOURCE=.\MSN_md5c.c
# End Source File
# Begin Source File

SOURCE=.\MSN_MIME.c
# End Source File
# Begin Source File

SOURCE=.\MSN_Misc.c
# End Source File
# Begin Source File

SOURCE=.\MSN_packet.c
# End Source File
# Begin Source File

SOURCE=.\MSN_SS.c
# End Source File
# Begin Source File

SOURCE=.\MSN_ws.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\icos\DRAGCOPY.CUR
# End Source File
# End Target
# End Project
