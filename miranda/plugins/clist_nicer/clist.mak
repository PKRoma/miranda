# Microsoft Developer Studio Generated NMAKE File, Based on clist.dsp
!IF "$(CFG)" == ""
CFG=clist_nicer - Win32 Release Unicode
!MESSAGE No configuration specified. Defaulting to clist_nicer - Win32 Release Unicode.
!ENDIF

!IF "$(CFG)" != "clist_nicer - Win32 Release" && "$(CFG)" != "clist_nicer - Win32 Debug" && "$(CFG)" != "clist_nicer - Win32 Release Unicode" && "$(CFG)" != "clist_nicer - Win32 Debug Unicode"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE
NULL=nul
!ENDIF

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\release\plugins\clist_nicer.dll"


CLEAN :
	-@erase "$(INTDIR)\alphablend.obj"
	-@erase "$(INTDIR)\clc.obj"
	-@erase "$(INTDIR)\CLCButton.obj"
	-@erase "$(INTDIR)\clcidents.obj"
	-@erase "$(INTDIR)\clcitems.obj"
	-@erase "$(INTDIR)\clcmsgs.obj"
	-@erase "$(INTDIR)\clcopts.obj"
	-@erase "$(INTDIR)\clcpaint.obj"
	-@erase "$(INTDIR)\clcutils.obj"
	-@erase "$(INTDIR)\clist.pch"
	-@erase "$(INTDIR)\clistevents.obj"
	-@erase "$(INTDIR)\clistmenus.obj"
	-@erase "$(INTDIR)\clistmod.obj"
	-@erase "$(INTDIR)\clistopts.obj"
	-@erase "$(INTDIR)\clistsettings.obj"
	-@erase "$(INTDIR)\clisttray.obj"
	-@erase "$(INTDIR)\clnplus.obj"
	-@erase "$(INTDIR)\clui.obj"
	-@erase "$(INTDIR)\cluiframes.obj"
	-@erase "$(INTDIR)\cluiopts.obj"
	-@erase "$(INTDIR)\cluiservices.obj"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\contact.obj"
	-@erase "$(INTDIR)\coolsblib.obj"
	-@erase "$(INTDIR)\coolscroll.obj"
	-@erase "$(INTDIR)\Docking.obj"
	-@erase "$(INTDIR)\extBackg.obj"
	-@erase "$(INTDIR)\forkthread.obj"
	-@erase "$(INTDIR)\framesmenu.obj"
	-@erase "$(INTDIR)\groupmenu.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\movetogroup.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\rowheight_funcs.obj"
	-@erase "$(INTDIR)\statusbar.obj"
	-@erase "$(INTDIR)\statusfloater.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\viewmodes.obj"
	-@erase "$(INTDIR)\wallpaper.obj"
	-@erase "$(OUTDIR)\clist_nicer.exp"
	-@erase "$(OUTDIR)\clist_nicer.map"
	-@erase "$(OUTDIR)\clist_nicer.pdb"
	-@erase "..\..\bin\release\plugins\clist_nicer.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /i "../../include/" /d "NDEBUG"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\clist.bsc"
BSC32_SBRS= \

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib msimg32.lib advapi32.lib shlwapi.lib /nologo /base:"0x6590000" /dll /incremental:no /pdb:"$(OUTDIR)\clist_nicer.pdb" /map:"$(INTDIR)\clist_nicer.map" /debug /machine:I386 /out:"../../bin/release/plugins/clist_nicer.dll" /implib:"$(OUTDIR)\clist_nicer.lib" /OPT:NOWIN98
LINK32_OBJS= \
	"$(INTDIR)\cluiframes.obj" \
	"$(INTDIR)\framesmenu.obj" \
	"$(INTDIR)\groupmenu.obj" \
	"$(INTDIR)\alphablend.obj" \
	"$(INTDIR)\clc.obj" \
	"$(INTDIR)\CLCButton.obj" \
	"$(INTDIR)\clcidents.obj" \
	"$(INTDIR)\clcitems.obj" \
	"$(INTDIR)\clcmsgs.obj" \
	"$(INTDIR)\clcopts.obj" \
	"$(INTDIR)\clcpaint.obj" \
	"$(INTDIR)\clcutils.obj" \
	"$(INTDIR)\clistevents.obj" \
	"$(INTDIR)\clistmenus.obj" \
	"$(INTDIR)\clistmod.obj" \
	"$(INTDIR)\clistopts.obj" \
	"$(INTDIR)\clistsettings.obj" \
	"$(INTDIR)\clisttray.obj" \
	"$(INTDIR)\clnplus.obj" \
	"$(INTDIR)\clui.obj" \
	"$(INTDIR)\cluiopts.obj" \
	"$(INTDIR)\cluiservices.obj" \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\contact.obj" \
	"$(INTDIR)\coolsblib.obj" \
	"$(INTDIR)\coolscroll.obj" \
	"$(INTDIR)\Docking.obj" \
	"$(INTDIR)\extBackg.obj" \
	"$(INTDIR)\forkthread.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\movetogroup.obj" \
	"$(INTDIR)\rowheight_funcs.obj" \
	"$(INTDIR)\statusbar.obj" \
	"$(INTDIR)\statusfloater.obj" \
	"$(INTDIR)\viewmodes.obj" \
	"$(INTDIR)\wallpaper.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\release\plugins\clist_nicer.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\bin\debug\plugins\clist_nicer.dll"


CLEAN :
	-@erase "$(INTDIR)\alphablend.obj"
	-@erase "$(INTDIR)\clc.obj"
	-@erase "$(INTDIR)\CLCButton.obj"
	-@erase "$(INTDIR)\clcidents.obj"
	-@erase "$(INTDIR)\clcitems.obj"
	-@erase "$(INTDIR)\clcmsgs.obj"
	-@erase "$(INTDIR)\clcopts.obj"
	-@erase "$(INTDIR)\clcpaint.obj"
	-@erase "$(INTDIR)\clcutils.obj"
	-@erase "$(INTDIR)\clist.pch"
	-@erase "$(INTDIR)\clistevents.obj"
	-@erase "$(INTDIR)\clistmenus.obj"
	-@erase "$(INTDIR)\clistmod.obj"
	-@erase "$(INTDIR)\clistopts.obj"
	-@erase "$(INTDIR)\clistsettings.obj"
	-@erase "$(INTDIR)\clisttray.obj"
	-@erase "$(INTDIR)\clnplus.obj"
	-@erase "$(INTDIR)\clui.obj"
	-@erase "$(INTDIR)\cluiframes.obj"
	-@erase "$(INTDIR)\cluiopts.obj"
	-@erase "$(INTDIR)\cluiservices.obj"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\contact.obj"
	-@erase "$(INTDIR)\coolsblib.obj"
	-@erase "$(INTDIR)\coolscroll.obj"
	-@erase "$(INTDIR)\Docking.obj"
	-@erase "$(INTDIR)\extBackg.obj"
	-@erase "$(INTDIR)\forkthread.obj"
	-@erase "$(INTDIR)\framesmenu.obj"
	-@erase "$(INTDIR)\groupmenu.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\movetogroup.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\rowheight_funcs.obj"
	-@erase "$(INTDIR)\statusbar.obj"
	-@erase "$(INTDIR)\statusfloater.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\viewmodes.obj"
	-@erase "$(INTDIR)\wallpaper.obj"
	-@erase "$(OUTDIR)\clist_nicer.exp"
	-@erase "$(OUTDIR)\clist_nicer.pdb"
	-@erase "..\..\bin\debug\plugins\clist_nicer.dll"
	-@erase "..\..\bin\debug\plugins\clist_nicer.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /i "../../include/" /d "_DEBUG"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\clist.bsc"
BSC32_SBRS= \

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib msimg32.lib advapi32.lib gdiplus.lib delayimp.lib shlwapi.lib /nologo /base:"0x6590000" /dll /incremental:yes /pdb:"$(OUTDIR)\clist_nicer.pdb" /debug /machine:I386 /out:"../../bin/debug/plugins/clist_nicer.dll" /implib:"$(OUTDIR)\clist_nicer.lib" /pdbtype:sept
LINK32_OBJS= \
	"$(INTDIR)\cluiframes.obj" \
	"$(INTDIR)\framesmenu.obj" \
	"$(INTDIR)\groupmenu.obj" \
	"$(INTDIR)\alphablend.obj" \
	"$(INTDIR)\clc.obj" \
	"$(INTDIR)\CLCButton.obj" \
	"$(INTDIR)\clcidents.obj" \
	"$(INTDIR)\clcitems.obj" \
	"$(INTDIR)\clcmsgs.obj" \
	"$(INTDIR)\clcopts.obj" \
	"$(INTDIR)\clcpaint.obj" \
	"$(INTDIR)\clcutils.obj" \
	"$(INTDIR)\clistevents.obj" \
	"$(INTDIR)\clistmenus.obj" \
	"$(INTDIR)\clistmod.obj" \
	"$(INTDIR)\clistopts.obj" \
	"$(INTDIR)\clistsettings.obj" \
	"$(INTDIR)\clisttray.obj" \
	"$(INTDIR)\clnplus.obj" \
	"$(INTDIR)\clui.obj" \
	"$(INTDIR)\cluiopts.obj" \
	"$(INTDIR)\cluiservices.obj" \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\contact.obj" \
	"$(INTDIR)\coolsblib.obj" \
	"$(INTDIR)\coolscroll.obj" \
	"$(INTDIR)\Docking.obj" \
	"$(INTDIR)\extBackg.obj" \
	"$(INTDIR)\forkthread.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\movetogroup.obj" \
	"$(INTDIR)\rowheight_funcs.obj" \
	"$(INTDIR)\statusbar.obj" \
	"$(INTDIR)\statusfloater.obj" \
	"$(INTDIR)\viewmodes.obj" \
	"$(INTDIR)\wallpaper.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\debug\plugins\clist_nicer.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

OUTDIR=.\Release_Unicode
INTDIR=.\Release_Unicode

ALL : "..\..\bin\Release Unicode\plugins\clist_nicer.dll"


CLEAN :
	-@erase "$(INTDIR)\alphablend.obj"
	-@erase "$(INTDIR)\clc.obj"
	-@erase "$(INTDIR)\CLCButton.obj"
	-@erase "$(INTDIR)\clcidents.obj"
	-@erase "$(INTDIR)\clcitems.obj"
	-@erase "$(INTDIR)\clcmsgs.obj"
	-@erase "$(INTDIR)\clcopts.obj"
	-@erase "$(INTDIR)\clcpaint.obj"
	-@erase "$(INTDIR)\clcutils.obj"
	-@erase "$(INTDIR)\clist.pch"
	-@erase "$(INTDIR)\clistevents.obj"
	-@erase "$(INTDIR)\clistmenus.obj"
	-@erase "$(INTDIR)\clistmod.obj"
	-@erase "$(INTDIR)\clistopts.obj"
	-@erase "$(INTDIR)\clistsettings.obj"
	-@erase "$(INTDIR)\clisttray.obj"
	-@erase "$(INTDIR)\clnplus.obj"
	-@erase "$(INTDIR)\clui.obj"
	-@erase "$(INTDIR)\cluiframes.obj"
	-@erase "$(INTDIR)\cluiopts.obj"
	-@erase "$(INTDIR)\cluiservices.obj"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\contact.obj"
	-@erase "$(INTDIR)\coolsblib.obj"
	-@erase "$(INTDIR)\coolscroll.obj"
	-@erase "$(INTDIR)\Docking.obj"
	-@erase "$(INTDIR)\extBackg.obj"
	-@erase "$(INTDIR)\forkthread.obj"
	-@erase "$(INTDIR)\framesmenu.obj"
	-@erase "$(INTDIR)\groupmenu.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\movetogroup.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\rowheight_funcs.obj"
	-@erase "$(INTDIR)\statusbar.obj"
	-@erase "$(INTDIR)\statusfloater.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\viewmodes.obj"
	-@erase "$(INTDIR)\wallpaper.obj"
	-@erase "$(OUTDIR)\clist_nicer.exp"
	-@erase "$(OUTDIR)\clist_nicer.map"
	-@erase "$(OUTDIR)\clist_nicer.pdb"
	-@erase "..\..\bin\Release Unicode\plugins\clist_nicer.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /i "../../include/" /d "NDEBUG" /d "UNICODE"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\clist.bsc"
BSC32_SBRS= \

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib msimg32.lib advapi32.lib shlwapi.lib /nologo /base:"0x6590000" /dll /incremental:no /pdb:"$(OUTDIR)\clist_nicer.pdb" /map:"$(INTDIR)\clist_nicer.map" /debug /machine:I386 /out:"../../bin/Release Unicode/plugins/clist_nicer.dll" /implib:"$(OUTDIR)\clist_nicer.lib" /OPT:NOWIN98
LINK32_OBJS= \
	"$(INTDIR)\cluiframes.obj" \
	"$(INTDIR)\framesmenu.obj" \
	"$(INTDIR)\groupmenu.obj" \
	"$(INTDIR)\alphablend.obj" \
	"$(INTDIR)\clc.obj" \
	"$(INTDIR)\CLCButton.obj" \
	"$(INTDIR)\clcidents.obj" \
	"$(INTDIR)\clcitems.obj" \
	"$(INTDIR)\clcmsgs.obj" \
	"$(INTDIR)\clcopts.obj" \
	"$(INTDIR)\clcpaint.obj" \
	"$(INTDIR)\clcutils.obj" \
	"$(INTDIR)\clistevents.obj" \
	"$(INTDIR)\clistmenus.obj" \
	"$(INTDIR)\clistmod.obj" \
	"$(INTDIR)\clistopts.obj" \
	"$(INTDIR)\clistsettings.obj" \
	"$(INTDIR)\clisttray.obj" \
	"$(INTDIR)\clnplus.obj" \
	"$(INTDIR)\clui.obj" \
	"$(INTDIR)\cluiopts.obj" \
	"$(INTDIR)\cluiservices.obj" \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\contact.obj" \
	"$(INTDIR)\coolsblib.obj" \
	"$(INTDIR)\coolscroll.obj" \
	"$(INTDIR)\Docking.obj" \
	"$(INTDIR)\extBackg.obj" \
	"$(INTDIR)\forkthread.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\movetogroup.obj" \
	"$(INTDIR)\rowheight_funcs.obj" \
	"$(INTDIR)\statusbar.obj" \
	"$(INTDIR)\statusfloater.obj" \
	"$(INTDIR)\viewmodes.obj" \
	"$(INTDIR)\wallpaper.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\Release Unicode\plugins\clist_nicer.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

OUTDIR=.\Debug_Unicode
INTDIR=.\Debug_Unicode

ALL : "..\..\bin\Debug Unicode\plugins\clist_nicer.dll"


CLEAN :
	-@erase "$(INTDIR)\alphablend.obj"
	-@erase "$(INTDIR)\clc.obj"
	-@erase "$(INTDIR)\CLCButton.obj"
	-@erase "$(INTDIR)\clcidents.obj"
	-@erase "$(INTDIR)\clcitems.obj"
	-@erase "$(INTDIR)\clcmsgs.obj"
	-@erase "$(INTDIR)\clcopts.obj"
	-@erase "$(INTDIR)\clcpaint.obj"
	-@erase "$(INTDIR)\clcutils.obj"
	-@erase "$(INTDIR)\clist.pch"
	-@erase "$(INTDIR)\clistevents.obj"
	-@erase "$(INTDIR)\clistmenus.obj"
	-@erase "$(INTDIR)\clistmod.obj"
	-@erase "$(INTDIR)\clistopts.obj"
	-@erase "$(INTDIR)\clistsettings.obj"
	-@erase "$(INTDIR)\clisttray.obj"
	-@erase "$(INTDIR)\clnplus.obj"
	-@erase "$(INTDIR)\clui.obj"
	-@erase "$(INTDIR)\cluiframes.obj"
	-@erase "$(INTDIR)\cluiopts.obj"
	-@erase "$(INTDIR)\cluiservices.obj"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\contact.obj"
	-@erase "$(INTDIR)\coolsblib.obj"
	-@erase "$(INTDIR)\coolscroll.obj"
	-@erase "$(INTDIR)\Docking.obj"
	-@erase "$(INTDIR)\extBackg.obj"
	-@erase "$(INTDIR)\forkthread.obj"
	-@erase "$(INTDIR)\framesmenu.obj"
	-@erase "$(INTDIR)\groupmenu.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\movetogroup.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\rowheight_funcs.obj"
	-@erase "$(INTDIR)\statusbar.obj"
	-@erase "$(INTDIR)\statusfloater.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\viewmodes.obj"
	-@erase "$(INTDIR)\wallpaper.obj"
	-@erase "$(OUTDIR)\clist_nicer.exp"
	-@erase "$(OUTDIR)\clist_nicer.pdb"
	-@erase "..\..\bin\Debug Unicode\plugins\clist_nicer.dll"
	-@erase "..\..\bin\Debug Unicode\plugins\clist_nicer.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /i "../../include/" /d "_DEBUG"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\clist.bsc"
BSC32_SBRS= \

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib msimg32.lib advapi32.lib gdiplus.lib delayimp.lib shlwapi.lib /nologo /base:"0x6590000" /dll /incremental:yes /pdb:"$(OUTDIR)\clist_nicer.pdb" /debug /machine:I386 /out:"../../bin/Debug Unicode/plugins/clist_nicer.dll" /implib:"$(OUTDIR)\clist_nicer.lib" /pdbtype:sept
LINK32_OBJS= \
	"$(INTDIR)\cluiframes.obj" \
	"$(INTDIR)\framesmenu.obj" \
	"$(INTDIR)\groupmenu.obj" \
	"$(INTDIR)\alphablend.obj" \
	"$(INTDIR)\clc.obj" \
	"$(INTDIR)\CLCButton.obj" \
	"$(INTDIR)\clcidents.obj" \
	"$(INTDIR)\clcitems.obj" \
	"$(INTDIR)\clcmsgs.obj" \
	"$(INTDIR)\clcopts.obj" \
	"$(INTDIR)\clcpaint.obj" \
	"$(INTDIR)\clcutils.obj" \
	"$(INTDIR)\clistevents.obj" \
	"$(INTDIR)\clistmenus.obj" \
	"$(INTDIR)\clistmod.obj" \
	"$(INTDIR)\clistopts.obj" \
	"$(INTDIR)\clistsettings.obj" \
	"$(INTDIR)\clisttray.obj" \
	"$(INTDIR)\clnplus.obj" \
	"$(INTDIR)\clui.obj" \
	"$(INTDIR)\cluiopts.obj" \
	"$(INTDIR)\cluiservices.obj" \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\contact.obj" \
	"$(INTDIR)\coolsblib.obj" \
	"$(INTDIR)\coolscroll.obj" \
	"$(INTDIR)\Docking.obj" \
	"$(INTDIR)\extBackg.obj" \
	"$(INTDIR)\forkthread.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\movetogroup.obj" \
	"$(INTDIR)\rowheight_funcs.obj" \
	"$(INTDIR)\statusbar.obj" \
	"$(INTDIR)\statusfloater.obj" \
	"$(INTDIR)\viewmodes.obj" \
	"$(INTDIR)\wallpaper.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\Debug Unicode\plugins\clist_nicer.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("clist.dep")
!INCLUDE "clist.dep"
!ELSE
!MESSAGE Warning: cannot find "clist.dep"
!ENDIF
!ENDIF


!IF "$(CFG)" == "clist_nicer - Win32 Release" || "$(CFG)" == "clist_nicer - Win32 Debug" || "$(CFG)" == "clist_nicer - Win32 Release Unicode" || "$(CFG)" == "clist_nicer - Win32 Debug Unicode"
SOURCE=.\CLUIFrames\cluiframes.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\cluiframes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\cluiframes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\cluiframes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\cluiframes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\CLUIFrames\framesmenu.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\framesmenu.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\framesmenu.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\framesmenu.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\framesmenu.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\CLUIFrames\groupmenu.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\groupmenu.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\groupmenu.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\groupmenu.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\groupmenu.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\alphablend.c

"$(INTDIR)\alphablend.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"


SOURCE=.\clc.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\CLCButton.c

"$(INTDIR)\CLCButton.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"


SOURCE=.\clcidents.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcidents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcidents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcidents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcidents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clcitems.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcitems.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcitems.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcitems.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcitems.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clcmsgs.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcmsgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcmsgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcmsgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcmsgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clcopts.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clcpaint.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcpaint.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcpaint.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcpaint.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcpaint.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clcutils.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clistevents.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistevents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistevents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistevents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistevents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clistmenus.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistmenus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistmenus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistmenus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistmenus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clistmod.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistmod.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistmod.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistmod.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistmod.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clistopts.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clistsettings.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistsettings.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistsettings.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistsettings.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistsettings.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clisttray.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clisttray.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clisttray.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clisttray.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clisttray.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clnplus.cpp

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clnplus.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clnplus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GR- /GX /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clnplus.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clnplus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clui.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clui.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clui.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clui.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clui.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\cluiopts.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\cluiopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\cluiopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\cluiopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\cluiopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\cluiservices.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\cluiservices.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\cluiservices.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\cluiservices.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\cluiservices.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\commonheaders.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\clist.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yc /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\clist.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\clist.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yc /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\clist.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\contact.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\contact.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\contact.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\contact.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\contact.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\coolsb\coolsblib.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\coolsblib.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\coolsblib.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\coolsblib.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\coolsblib.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\coolsb\coolscroll.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\coolscroll.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\coolscroll.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\coolscroll.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\coolscroll.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\Docking.c

"$(INTDIR)\Docking.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"


SOURCE=.\extBackg.c

"$(INTDIR)\extBackg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"


SOURCE=.\forkthread.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\forkthread.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\forkthread.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\forkthread.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\forkthread.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\init.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\init.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\init.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\init.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\init.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\CLUIFrames\movetogroup.c

!IF  "$(CFG)" == "clist_nicer - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\movetogroup.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\movetogroup.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\movetogroup.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist_nicer - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\movetogroup.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\rowheight_funcs.c

"$(INTDIR)\rowheight_funcs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"


SOURCE=.\statusbar.c

"$(INTDIR)\statusbar.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"


SOURCE=.\statusfloater.c

"$(INTDIR)\statusfloater.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"


SOURCE=.\viewmodes.c

"$(INTDIR)\viewmodes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"


SOURCE=.\wallpaper.c

"$(INTDIR)\wallpaper.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"


SOURCE=.\resource.rc

"$(INTDIR)\resource.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF
