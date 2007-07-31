# Microsoft Developer Studio Project File - Name="clist_modern" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=clist_modern - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "clist_modern.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "clist_modern.mak" CFG="clist_modern - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "clist_modern - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "clist_modern - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "clist_modern - Win32 Debug Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "clist_modern - Win32 Release Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "SAK"
# PROP Scc_LocalPath "SAK"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "clist_modern - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "$(SolutionDir)$(ConfigurationName)/Plugins"
# PROP BASE Intermediate_Dir "$(SolutionDir)$(ConfigurationName)/Obj/$(ProjectName)"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "$(SolutionDir)$(ConfigurationName)/Plugins"
# PROP Intermediate_Dir "$(SolutionDir)$(ConfigurationName)/Obj/$(ProjectName)"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /I "../../include" /Zi /W3 /Ox /Og /Ob1 /Oi /Os /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "_MBCS" /GF /EHsc /J /YX /FAcs /Fa"" /c /GX 
# ADD CPP /nologo /MD /I "../../include" /Zi /W3 /Ox /Og /Ob1 /Oi /Os /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "_MBCS" /GF /EHsc /J /YX /FAcs /Fa"" /c /GX 
# ADD BASE MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\mwclist.tlb" /win32 
# ADD MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\mwclist.tlb" /win32 
# ADD BASE RSC /l 2057 /d "NDEBUG" 
# ADD RSC /l 2057 /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib MSImg32.Lib comctl32.lib shlwapi.lib  gdiplus.lib Delayimp.lib /nologo /dll /out:"$(SolutionDir)$(ConfigurationName)\Plugins\$(ProjectName).dll" /incremental:no /delayload:"gdiplus.dll" /debug /pdbtype:sept /map /mapinfo:lines /subsystem:windows /opt:ref /opt:icf /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib MSImg32.Lib comctl32.lib shlwapi.lib  gdiplus.lib Delayimp.lib /nologo /dll /out:"$(SolutionDir)$(ConfigurationName)\Plugins\$(ProjectName).dll" /incremental:no /delayload:"gdiplus.dll" /debug /pdbtype:sept /map /mapinfo:lines /subsystem:windows /opt:ref /opt:icf /MACHINE:I386

!ELSEIF  "$(CFG)" == "clist_modern - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "$(SolutionDir)$(ConfigurationName)/Plugins"
# PROP BASE Intermediate_Dir "$(SolutionDir)$(ConfigurationName)/Obj/$(ProjectName)"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "$(SolutionDir)$(ConfigurationName)/Plugins"
# PROP Intermediate_Dir "$(SolutionDir)$(ConfigurationName)/Obj/$(ProjectName)"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /I "../../include" /ZI /W3 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "_MBCS" /EHsc /J /YX /FR /GZ /c /GX 
# ADD CPP /nologo /MDd /I "../../include" /ZI /W3 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "_MBCS" /EHsc /J /YX /FR /GZ /c /GX 
# ADD BASE MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\mwclist.tlb" /win32 
# ADD MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\mwclist.tlb" /win32 
# ADD BASE RSC /l 1033 /d "_DEBUG" 
# ADD RSC /l 1033 /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib MSImg32.Lib comctl32.lib shlwapi.lib gdiplus.lib Delayimp.lib /nologo /dll /out:"$(SolutionDir)$(ConfigurationName)\Plugins\$(ProjectName).dll" /incremental:yes /delayload:"gdiplus.dll" /debug /pdbtype:sept /subsystem:windows /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib MSImg32.Lib comctl32.lib shlwapi.lib gdiplus.lib Delayimp.lib /nologo /dll /out:"$(SolutionDir)$(ConfigurationName)\Plugins\$(ProjectName).dll" /incremental:yes /delayload:"gdiplus.dll" /debug /pdbtype:sept /subsystem:windows /MACHINE:I386

!ELSEIF  "$(CFG)" == "clist_modern - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "$(SolutionDir)$(ConfigurationName)/Plugins"
# PROP BASE Intermediate_Dir "$(SolutionDir)$(ConfigurationName)/Obj/$(ProjectName)"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "$(SolutionDir)$(ConfigurationName)/Plugins"
# PROP Intermediate_Dir "$(SolutionDir)$(ConfigurationName)/Obj/$(ProjectName)"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /I "../../include" /ZI /W3 /Od /D "_DEBUG_LOG" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "_UNICODE" /EHsc /J /YX /FR /GZ /c /GX 
# ADD CPP /nologo /MDd /I "../../include" /ZI /W3 /Od /D "_DEBUG_LOG" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "_UNICODE" /EHsc /J /YX /FR /GZ /c /GX 
# ADD BASE MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\mwclist.tlb" /win32 
# ADD MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\mwclist.tlb" /win32 
# ADD BASE RSC /l 1033 /d "_DEBUG" 
# ADD RSC /l 1033 /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib MSImg32.Lib comctl32.lib shlwapi.lib gdiplus.lib Delayimp.lib /nologo /dll /out:"$(SolutionDir)$(ConfigurationName)\Plugins\$(ProjectName).dll" /incremental:yes /delayload:"gdiplus.dll" /debug /pdbtype:sept /subsystem:windows /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib MSImg32.Lib comctl32.lib shlwapi.lib gdiplus.lib Delayimp.lib /nologo /dll /out:"$(SolutionDir)$(ConfigurationName)\Plugins\$(ProjectName).dll" /incremental:yes /delayload:"gdiplus.dll" /debug /pdbtype:sept /subsystem:windows /MACHINE:I386

!ELSEIF  "$(CFG)" == "clist_modern - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "$(SolutionDir)$(ConfigurationName)/Plugins"
# PROP BASE Intermediate_Dir "$(SolutionDir)$(ConfigurationName)/Obj/$(ProjectName)"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "$(SolutionDir)$(ConfigurationName)/Plugins"
# PROP Intermediate_Dir "$(SolutionDir)$(ConfigurationName)/Obj/$(ProjectName)"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /I "../../include" /Zi /W3 /Ox /Og /Os /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "_UNICODE" /GF /EHsc /J /YX /FAcs /Fa"" /c /GX 
# ADD CPP /nologo /MD /I "../../include" /Zi /W3 /Ox /Og /Os /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "_UNICODE" /GF /EHsc /J /YX /FAcs /Fa"" /c /GX 
# ADD BASE MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\mwclist.tlb" /win32 
# ADD MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\mwclist.tlb" /win32 
# ADD BASE RSC /l 2057 /d "NDEBUG" 
# ADD RSC /l 2057 /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib MSImg32.Lib comctl32.lib shlwapi.lib  gdiplus.lib Delayimp.lib /nologo /dll /out:"$(SolutionDir)$(ConfigurationName)\Plugins\$(ProjectName).dll" /delayload:"gdiplus.dll" /debug /pdbtype:sept /map /mapinfo:lines /subsystem:windows /opt:ref /opt:icf /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib MSImg32.Lib comctl32.lib shlwapi.lib  gdiplus.lib Delayimp.lib /nologo /dll /out:"$(SolutionDir)$(ConfigurationName)\Plugins\$(ProjectName).dll" /delayload:"gdiplus.dll" /debug /pdbtype:sept /map /mapinfo:lines /subsystem:windows /opt:ref /opt:icf /MACHINE:I386

!ENDIF

# Begin Target

# Name "clist_modern - Win32 Release"
# Name "clist_modern - Win32 Debug"
# Name "clist_modern - Win32 Debug Unicode"
# Name "clist_modern - Win32 Release Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\button.c
# End Source File
# Begin Source File

SOURCE=.\cache_funcs.c
# End Source File
# Begin Source File

SOURCE=.\clc.c
# End Source File
# Begin Source File

SOURCE=.\clcidents.c
# End Source File
# Begin Source File

SOURCE=.\clcitems.c
# End Source File
# Begin Source File

SOURCE=.\clcmsgs.c
# End Source File
# Begin Source File

SOURCE=.\clcopts.c
# End Source File
# Begin Source File

SOURCE=.\clcpaint.c
# End Source File
# Begin Source File

SOURCE=.\clcutils.c
# End Source File
# Begin Source File

SOURCE=.\clistevents.c
# End Source File
# Begin Source File

SOURCE=.\clistmenus.c
# End Source File
# Begin Source File

SOURCE=.\clistmod.c
# End Source File
# Begin Source File

SOURCE=.\clistopts.c
# End Source File
# Begin Source File

SOURCE=.\clistsettings.c
# End Source File
# Begin Source File

SOURCE=.\clisttray.c
# End Source File
# Begin Source File

SOURCE=.\clui.c
# End Source File
# Begin Source File

SOURCE=.\cluiframes.c
# End Source File
# Begin Source File

SOURCE=.\cluiservices.c
# End Source File
# Begin Source File

SOURCE=.\commonheaders.c
# End Source File
# Begin Source File

SOURCE=.\contact.c
# End Source File
# Begin Source File

SOURCE=.\Docking.c
# End Source File
# Begin Source File

SOURCE=.\extraimage.c
# End Source File
# Begin Source File

SOURCE=.\framesmenu.c
# End Source File
# Begin Source File

SOURCE=.\gdiplus.cpp
# End Source File
# Begin Source File

SOURCE=.\groupmenu.c
# End Source File
# Begin Source File

SOURCE=.\image_array.c
# End Source File
# Begin Source File

SOURCE=.\init.c
# End Source File
# Begin Source File

SOURCE=.\keyboard.c
# End Source File
# Begin Source File

SOURCE=.\log.c
# End Source File
# Begin Source File

SOURCE=.\modern_animated_avatars.c
# End Source File
# Begin Source File

SOURCE=.\modern_awaymsg.c
# End Source File
# Begin Source File

SOURCE=.\modern_button.c
# End Source File
# Begin Source File

SOURCE=.\modern_gettextasync.c
# End Source File
# Begin Source File

SOURCE=.\modern_row.cpp
# End Source File
# Begin Source File

SOURCE=.\modern_skinselector.c
# End Source File
# Begin Source File

SOURCE=.\modern_statusbar.c
# End Source File
# Begin Source File

SOURCE=.\modern_toolbar.c
# End Source File
# Begin Source File

SOURCE=.\newrowopts.c
# End Source File
# Begin Source File

SOURCE=.\popup.c
# End Source File
# Begin Source File

SOURCE=.\rowheight_funcs.c
# End Source File
# Begin Source File

SOURCE=.\rowtemplateopt.c
# End Source File
# Begin Source File

SOURCE=.\SkinEditor.c
# End Source File
# Begin Source File

SOURCE=.\SkinEngine.c
# End Source File
# Begin Source File

SOURCE=.\SkinOpt.c
# End Source File
# Begin Source File

SOURCE=.\viewmodes.c
# End Source File
# Begin Source File

SOURCE=.\xpTheme.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cache_funcs.h
# End Source File
# Begin Source File

SOURCE=.\clc.h
# End Source File
# Begin Source File

SOURCE=.\clist.h
# End Source File
# Begin Source File

SOURCE=.\clui.h
# End Source File
# Begin Source File

SOURCE=.\commonheaders.h
# End Source File
# Begin Source File

SOURCE=.\commonprototypes.h
# End Source File
# Begin Source File

SOURCE=.\defsettings.h
# End Source File
# Begin Source File

SOURCE=.\effectenum.h
# End Source File
# Begin Source File

SOURCE=.\image_array.h
# End Source File
# Begin Source File

SOURCE=.\log.h
# End Source File
# Begin Source File

SOURCE=.\menuSubsystem.h
# End Source File
# Begin Source File

SOURCE=.\modern_row.h
# End Source File
# Begin Source File

SOURCE=.\modern_statusbar.h
# End Source File
# Begin Source File

SOURCE=.\popup.h
# End Source File
# Begin Source File

SOURCE=.\rowheight_funcs.h
# End Source File
# Begin Source File

SOURCE=.\SkinEngine.h
# End Source File
# Begin Source File

SOURCE=.\statusmodes.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Group "res"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\res\1.ico
# End Source File
# Begin Source File

SOURCE=.\res\2.ico
# End Source File
# Begin Source File

SOURCE=.\res\3.ico
# End Source File
# Begin Source File

SOURCE=.\res\4.ico
# End Source File
# Begin Source File

SOURCE=.\res\5.ico
# End Source File
# Begin Source File

SOURCE=.\res\6.ico
# End Source File
# Begin Source File

SOURCE=.\res\7.ico
# End Source File
# Begin Source File

SOURCE=.\res\8.ico
# End Source File
# Begin Source File

SOURCE=.\res\addgoupp.ico
# End Source File
# Begin Source File

SOURCE=.\res\AlwaysVis.ico
# End Source File
# Begin Source File

SOURCE=.\res\away.ico
# End Source File
# Begin Source File

SOURCE=.\res\back.tga
# End Source File
# Begin Source File

SOURCE=.\res\chat.ico
# End Source File
# Begin Source File

SOURCE=.\res\Chatchannel.ico
# End Source File
# Begin Source File

SOURCE=.\res\dnd.ico
# End Source File
# Begin Source File

SOURCE=.\res\hide_avatar.ico
# End Source File
# Begin Source File

SOURCE=.\res\invisible.ico
# End Source File
# Begin Source File

SOURCE=.\res\listening_to.ico
# End Source File
# Begin Source File

SOURCE=.\res\lunch.ico
# End Source File
# Begin Source File

SOURCE=.\res\MicroAway.ico
# End Source File
# Begin Source File

SOURCE=.\res\MicroChat.ico
# End Source File
# Begin Source File

SOURCE=.\res\MicroDnd.ico
# End Source File
# Begin Source File

SOURCE=.\res\MicroInvisible.ico
# End Source File
# Begin Source File

SOURCE=.\res\MicroLunch.ico
# End Source File
# Begin Source File

SOURCE=.\res\MicroNa.ico
# End Source File
# Begin Source File

SOURCE=.\res\MicroOccupied.ico
# End Source File
# Begin Source File

SOURCE=.\res\MicroOffline.ico
# End Source File
# Begin Source File

SOURCE=.\res\MicroPhone.ico
# End Source File
# Begin Source File

SOURCE=.\res\NA.ico
# End Source File
# Begin Source File

SOURCE=.\res\neo_meta_create.cur
# End Source File
# Begin Source File

SOURCE=.\res\neo_meta_default.cur
# End Source File
# Begin Source File

SOURCE=.\res\neo_meta_move.cur
# End Source File
# Begin Source File

SOURCE=.\res\NeverVis.ico
# End Source File
# Begin Source File

SOURCE=.\res\occupied.ico
# End Source File
# Begin Source File

SOURCE=.\res\offline.ico
# End Source File
# Begin Source File

SOURCE=.\res\phone.ico
# End Source File
# Begin Source File

SOURCE=.\res\rate_high.ico
# End Source File
# Begin Source File

SOURCE=.\res\rate_low.ico
# End Source File
# Begin Source File

SOURCE=.\res\rate_med.ico
# End Source File
# Begin Source File

SOURCE=.\res\rate_none.ico
# End Source File
# Begin Source File

SOURCE=.\res\reset_view.ico
# End Source File
# Begin Source File

SOURCE=.\res\set_view.ico
# End Source File
# Begin Source File

SOURCE=.\res\show_avatar.ico
# End Source File
# Begin Source File

SOURCE=.\res\skin.msf
# End Source File
# End Group
# End Group
# Begin Group "m_api"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\m_addcontact.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_avatars.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_awaymsg.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_button.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_chat.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_clc.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_clist.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_clistint.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_clui.h
# End Source File
# Begin Source File

SOURCE=.\m_api\m_clui.h
# End Source File
# Begin Source File

SOURCE=.\m_api\m_cluiframes.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_contactdir.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_contacts.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_database.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_email.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_file.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_findadd.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_fontservice.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_fuse.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_genmenu.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_history.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_icolib.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_icq.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_idle.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_ignore.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_imgsrvc.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_langpack.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_message.h
# End Source File
# Begin Source File

SOURCE=.\m_api\m_metacontacts.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_netlib.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_options.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_plugins.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_png.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_popup.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_protocols.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_protomod.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_protosvc.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_skin.h
# End Source File
# Begin Source File

SOURCE=.\m_api\m_skin_eng.h
# End Source File
# Begin Source File

SOURCE=.\m_api\m_skinbutton.h
# End Source File
# Begin Source File

SOURCE=.\m_api\m_smileyadd.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_system.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_system_cpp.h
# End Source File
# Begin Source File

SOURCE=.\m_api\m_toolbar.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_url.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_userinfo.h
# End Source File
# Begin Source File

SOURCE=..\..\include\m_utils.h
# End Source File
# Begin Source File

SOURCE=.\m_api\m_variables.h
# End Source File
# Begin Source File

SOURCE=.\m_api\m_xpTheme.h
# End Source File
# Begin Source File

SOURCE=..\..\include\newpluginapi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\statusmodes.h
# End Source File
# Begin Source File

SOURCE=..\..\include\win2k.h
# End Source File
# End Group
# Begin Group "hdr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\hdr\cluiframes.h
# End Source File
# Begin Source File

SOURCE=.\hdr\modern_awaymsg.h
# End Source File
# Begin Source File

SOURCE=.\hdr\modern_gettextasync.h
# End Source File
# Begin Source File

SOURCE=.\hdr\modern_global_structure.h
# End Source File
# Begin Source File

SOURCE=.\hdr\modern_skinselector.h
# End Source File
# End Group
# End Target
# End Project

