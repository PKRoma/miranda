# Microsoft Developer Studio Project File - Name="jabber" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=jabber - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "jabber.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "jabber.mak" CFG="jabber - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "jabber - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "jabber - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "jabber - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib version.lib /nologo /base:"0x32500000" /dll /map /machine:I386 /out:"../../bin/release/plugins/jabber.dll" /ALIGN:4096 /ignore:4108
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "jabber - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib version.lib /nologo /base:"0x32500000" /dll /map /debug /machine:I386 /out:"../../bin/debug/plugins/jabber.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "jabber - Win32 Release"
# Name "jabber - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\jabber.c
# End Source File
# Begin Source File

SOURCE=.\jabber_agent.c
# End Source File
# Begin Source File

SOURCE=.\jabber_byte.c
# End Source File
# Begin Source File

SOURCE=.\jabber_file.c
# End Source File
# Begin Source File

SOURCE=.\jabber_form.c
# End Source File
# Begin Source File

SOURCE=.\jabber_ft.c
# End Source File
# Begin Source File

SOURCE=.\jabber_groupchat.c
# End Source File
# Begin Source File

SOURCE=.\jabber_groupchatlog.c
# End Source File
# Begin Source File

SOURCE=.\jabber_iq.c
# End Source File
# Begin Source File

SOURCE=.\jabber_iqid.c
# End Source File
# Begin Source File

SOURCE=.\jabber_iqid_muc.c
# End Source File
# Begin Source File

SOURCE=.\jabber_list.c
# End Source File
# Begin Source File

SOURCE=.\jabber_menu.c
# End Source File
# Begin Source File

SOURCE=.\jabber_misc.c
# End Source File
# Begin Source File

SOURCE=.\jabber_opt.c
# End Source File
# Begin Source File

SOURCE=.\jabber_password.c
# End Source File
# Begin Source File

SOURCE=.\jabber_proxy.c
# End Source File
# Begin Source File

SOURCE=.\jabber_ssl.c
# End Source File
# Begin Source File

SOURCE=.\jabber_svc.c
# End Source File
# Begin Source File

SOURCE=.\jabber_thread.c
# End Source File
# Begin Source File

SOURCE=.\jabber_userinfo.c
# End Source File
# Begin Source File

SOURCE=.\jabber_util.c
# End Source File
# Begin Source File

SOURCE=.\jabber_vcard.c
# End Source File
# Begin Source File

SOURCE=.\jabber_ws.c
# End Source File
# Begin Source File

SOURCE=.\jabber_xml.c
# End Source File
# Begin Source File

SOURCE=.\jabber_xmlns.c
# End Source File
# Begin Source File

SOURCE=.\sha1.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\jabber.h
# End Source File
# Begin Source File

SOURCE=.\jabber_byte.h
# End Source File
# Begin Source File

SOURCE=.\jabber_iq.h
# End Source File
# Begin Source File

SOURCE=.\jabber_list.h
# End Source File
# Begin Source File

SOURCE=.\jabber_proxy.h
# End Source File
# Begin Source File

SOURCE=.\jabber_ssl.h
# End Source File
# Begin Source File

SOURCE=.\jabber_xml.h
# End Source File
# Begin Source File

SOURCE=.\jabber_xmlns.h
# End Source File
# Begin Source File

SOURCE=.\sha1.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\icos\addcontact.ico
# End Source File
# Begin Source File

SOURCE=.\icos\block.ico
# End Source File
# Begin Source File

SOURCE=.\icos\delete.ico
# End Source File
# Begin Source File

SOURCE=.\icos\gcadmin.ico
# End Source File
# Begin Source File

SOURCE=.\icos\gcmodera.ico
# End Source File
# Begin Source File

SOURCE=.\icos\gcowner.ico
# End Source File
# Begin Source File

SOURCE=.\icos\gcvoice.ico
# End Source File
# Begin Source File

SOURCE=.\icos\grant.ico
# End Source File
# Begin Source File

SOURCE=.\icos\group.ico
# End Source File
# Begin Source File

SOURCE=.\icos\jabber.ico
# End Source File
# Begin Source File

SOURCE=.\jabber.rc
# End Source File
# Begin Source File

SOURCE=.\icos\key.ico
# End Source File
# Begin Source File

SOURCE=.\icos\message.ico
# End Source File
# Begin Source File

SOURCE=.\icos\open.ico
# End Source File
# Begin Source File

SOURCE=.\icos\pages.ico
# End Source File
# Begin Source File

SOURCE=.\icos\rename.ico
# End Source File
# Begin Source File

SOURCE=.\icos\request.ico
# End Source File
# Begin Source File

SOURCE=.\icos\save.ico
# End Source File
# Begin Source File

SOURCE=.\icos\write.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\docs\changelog_jabber.txt
# End Source File
# Begin Source File

SOURCE=.\docs\readme_jabber.txt
# End Source File
# Begin Source File

SOURCE=.\docs\translation_jabber.txt
# End Source File
# End Target
# End Project
