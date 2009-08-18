# Microsoft Developer Studio Generated NMAKE File, Based on tabsrmm.dsp
!IF "$(CFG)" == ""
CFG=tabSRMM - Win32 Debug
!MESSAGE No configuration specified. Defaulting to tabSRMM - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "tabSRMM - Win32 Debug" && "$(CFG)" != "tabSRMM - Win32 Release Unicode" && "$(CFG)" != "tabSRMM - Win32 Release" && "$(CFG)" != "tabSRMM - Win32 Debug Unicode"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tabsrmm.mak" CFG="tabSRMM - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tabSRMM - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabSRMM - Win32 Release Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabSRMM - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabSRMM - Win32 Debug Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\Bin\Debug\Plugins\tabsrmm.dll"


CLEAN :
	-@erase "$(INTDIR)\buttonsbar.obj"
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\colorchooser.obj"
	-@erase "$(INTDIR)\container.obj"
	-@erase "$(INTDIR)\containeroptions.obj"
	-@erase "$(INTDIR)\controls.obj"
	-@erase "$(INTDIR)\eventpopups.obj"
	-@erase "$(INTDIR)\formatting.obj"
	-@erase "$(INTDIR)\generic_msghandlers.obj"
	-@erase "$(INTDIR)\globals.obj"
	-@erase "$(INTDIR)\hotkeyhandler.obj"
	-@erase "$(INTDIR)\ImageDataObject.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\manager.obj"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\mim.obj"
	-@erase "$(INTDIR)\modplus.obj"
	-@erase "$(INTDIR)\msgdialog.obj"
	-@erase "$(INTDIR)\msgdlgutils.obj"
	-@erase "$(INTDIR)\msglog.obj"
	-@erase "$(INTDIR)\msgoptions.obj"
	-@erase "$(INTDIR)\msgoptions_plus.obj"
	-@erase "$(INTDIR)\msgs.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\selectcontainer.obj"
	-@erase "$(INTDIR)\sendqueue.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\srmm.obj"
	-@erase "$(INTDIR)\srmm.pch"
	-@erase "$(INTDIR)\tabctrl.obj"
	-@erase "$(INTDIR)\tabsrmm_private.res"
	-@erase "$(INTDIR)\taskbar.obj"
	-@erase "$(INTDIR)\templates.obj"
	-@erase "$(INTDIR)\themeio.obj"
	-@erase "$(INTDIR)\themes.obj"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\trayicon.obj"
	-@erase "$(INTDIR)\TSButton.obj"
	-@erase "$(INTDIR)\typingnotify.obj"
	-@erase "$(INTDIR)\userprefs.obj"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\window.obj"
	-@erase "$(OUTDIR)\srmm.exp"
	-@erase "$(OUTDIR)\srmm.pdb"
	-@erase "..\..\Bin\Debug\Plugins\tabsrmm.dll"
	-@erase "..\..\Bin\Debug\Plugins\tabsrmm.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

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
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\tabsrmm_private.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tabsrmm.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib shlwapi.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\srmm.pdb" /debug /machine:IX86 /out:"..\..\Bin\Debug\Plugins\tabsrmm.dll" /implib:"$(OUTDIR)\srmm.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\clist.obj" \
	"$(INTDIR)\colorchooser.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\manager.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\tools.obj" \
	"$(INTDIR)\window.obj" \
	"$(INTDIR)\buttonsbar.obj" \
	"$(INTDIR)\container.obj" \
	"$(INTDIR)\containeroptions.obj" \
	"$(INTDIR)\controls.obj" \
	"$(INTDIR)\eventpopups.obj" \
	"$(INTDIR)\formatting.obj" \
	"$(INTDIR)\generic_msghandlers.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\hotkeyhandler.obj" \
	"$(INTDIR)\ImageDataObject.obj" \
	"$(INTDIR)\mim.obj" \
	"$(INTDIR)\modplus.obj" \
	"$(INTDIR)\msgdialog.obj" \
	"$(INTDIR)\msgdlgutils.obj" \
	"$(INTDIR)\msglog.obj" \
	"$(INTDIR)\msgoptions.obj" \
	"$(INTDIR)\msgoptions_plus.obj" \
	"$(INTDIR)\msgs.obj" \
	"$(INTDIR)\selectcontainer.obj" \
	"$(INTDIR)\sendqueue.obj" \
	"$(INTDIR)\srmm.obj" \
	"$(INTDIR)\tabctrl.obj" \
	"$(INTDIR)\taskbar.obj" \
	"$(INTDIR)\templates.obj" \
	"$(INTDIR)\themeio.obj" \
	"$(INTDIR)\themes.obj" \
	"$(INTDIR)\trayicon.obj" \
	"$(INTDIR)\TSButton.obj" \
	"$(INTDIR)\typingnotify.obj" \
	"$(INTDIR)\userprefs.obj" \
	"$(INTDIR)\tabsrmm_private.res"

"..\..\Bin\Debug\Plugins\tabsrmm.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

OUTDIR=.\Release_Unicode
INTDIR=.\Release_Unicode

ALL : "..\..\Bin\Release Unicode\Plugins\tabsrmm.dll"


CLEAN :
	-@erase "$(INTDIR)\buttonsbar.obj"
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\colorchooser.obj"
	-@erase "$(INTDIR)\container.obj"
	-@erase "$(INTDIR)\containeroptions.obj"
	-@erase "$(INTDIR)\controls.obj"
	-@erase "$(INTDIR)\eventpopups.obj"
	-@erase "$(INTDIR)\formatting.obj"
	-@erase "$(INTDIR)\generic_msghandlers.obj"
	-@erase "$(INTDIR)\globals.obj"
	-@erase "$(INTDIR)\hotkeyhandler.obj"
	-@erase "$(INTDIR)\ImageDataObject.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\manager.obj"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\mim.obj"
	-@erase "$(INTDIR)\modplus.obj"
	-@erase "$(INTDIR)\msgdialog.obj"
	-@erase "$(INTDIR)\msgdlgutils.obj"
	-@erase "$(INTDIR)\msglog.obj"
	-@erase "$(INTDIR)\msgoptions.obj"
	-@erase "$(INTDIR)\msgoptions_plus.obj"
	-@erase "$(INTDIR)\msgs.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\selectcontainer.obj"
	-@erase "$(INTDIR)\sendqueue.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\srmm.obj"
	-@erase "$(INTDIR)\srmm.pch"
	-@erase "$(INTDIR)\tabctrl.obj"
	-@erase "$(INTDIR)\taskbar.obj"
	-@erase "$(INTDIR)\templates.obj"
	-@erase "$(INTDIR)\themeio.obj"
	-@erase "$(INTDIR)\themes.obj"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\trayicon.obj"
	-@erase "$(INTDIR)\TSButton.obj"
	-@erase "$(INTDIR)\typingnotify.obj"
	-@erase "$(INTDIR)\userprefs.obj"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\window.obj"
	-@erase "$(OUTDIR)\srmm.exp"
	-@erase "$(OUTDIR)\tabsrmm.map"
	-@erase "..\..\Bin\Release Unicode\Plugins\tabsrmm.dll"
	-@erase ".\tabsrmm_private.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

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
RSC_PROJ=/l 0x809 /fo".\tabsrmm_private.res" /d "NDEBUG" /d "UNICODE" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tabsrmm.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib shlwapi.lib /nologo /base:"0x6a540000" /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\tabsrmm.pdb" /map:"$(INTDIR)\tabsrmm.map" /machine:IX86 /out:"..\..\Bin\Release Unicode\Plugins\tabsrmm.dll" /implib:"$(OUTDIR)\srmm.lib" /opt:NOWIN98 
LINK32_OBJS= \
	"$(INTDIR)\clist.obj" \
	"$(INTDIR)\colorchooser.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\manager.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\tools.obj" \
	"$(INTDIR)\window.obj" \
	"$(INTDIR)\buttonsbar.obj" \
	"$(INTDIR)\container.obj" \
	"$(INTDIR)\containeroptions.obj" \
	"$(INTDIR)\controls.obj" \
	"$(INTDIR)\eventpopups.obj" \
	"$(INTDIR)\formatting.obj" \
	"$(INTDIR)\generic_msghandlers.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\hotkeyhandler.obj" \
	"$(INTDIR)\ImageDataObject.obj" \
	"$(INTDIR)\mim.obj" \
	"$(INTDIR)\modplus.obj" \
	"$(INTDIR)\msgdialog.obj" \
	"$(INTDIR)\msgdlgutils.obj" \
	"$(INTDIR)\msglog.obj" \
	"$(INTDIR)\msgoptions.obj" \
	"$(INTDIR)\msgoptions_plus.obj" \
	"$(INTDIR)\msgs.obj" \
	"$(INTDIR)\selectcontainer.obj" \
	"$(INTDIR)\sendqueue.obj" \
	"$(INTDIR)\srmm.obj" \
	"$(INTDIR)\tabctrl.obj" \
	"$(INTDIR)\taskbar.obj" \
	"$(INTDIR)\templates.obj" \
	"$(INTDIR)\themeio.obj" \
	"$(INTDIR)\themes.obj" \
	"$(INTDIR)\trayicon.obj" \
	"$(INTDIR)\TSButton.obj" \
	"$(INTDIR)\typingnotify.obj" \
	"$(INTDIR)\userprefs.obj" \
	".\tabsrmm_private.res"

"..\..\Bin\Release Unicode\Plugins\tabsrmm.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\Bin\Release\Plugins\tabsrmm.dll"


CLEAN :
	-@erase "$(INTDIR)\buttonsbar.obj"
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\colorchooser.obj"
	-@erase "$(INTDIR)\container.obj"
	-@erase "$(INTDIR)\containeroptions.obj"
	-@erase "$(INTDIR)\controls.obj"
	-@erase "$(INTDIR)\eventpopups.obj"
	-@erase "$(INTDIR)\formatting.obj"
	-@erase "$(INTDIR)\generic_msghandlers.obj"
	-@erase "$(INTDIR)\globals.obj"
	-@erase "$(INTDIR)\hotkeyhandler.obj"
	-@erase "$(INTDIR)\ImageDataObject.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\manager.obj"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\mim.obj"
	-@erase "$(INTDIR)\modplus.obj"
	-@erase "$(INTDIR)\msgdialog.obj"
	-@erase "$(INTDIR)\msgdlgutils.obj"
	-@erase "$(INTDIR)\msglog.obj"
	-@erase "$(INTDIR)\msgoptions.obj"
	-@erase "$(INTDIR)\msgoptions_plus.obj"
	-@erase "$(INTDIR)\msgs.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\selectcontainer.obj"
	-@erase "$(INTDIR)\sendqueue.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\srmm.obj"
	-@erase "$(INTDIR)\srmm.pch"
	-@erase "$(INTDIR)\tabctrl.obj"
	-@erase "$(INTDIR)\tabsrmm_private.res"
	-@erase "$(INTDIR)\taskbar.obj"
	-@erase "$(INTDIR)\templates.obj"
	-@erase "$(INTDIR)\themeio.obj"
	-@erase "$(INTDIR)\themes.obj"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\trayicon.obj"
	-@erase "$(INTDIR)\TSButton.obj"
	-@erase "$(INTDIR)\typingnotify.obj"
	-@erase "$(INTDIR)\userprefs.obj"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\window.obj"
	-@erase "$(OUTDIR)\srmm.exp"
	-@erase "$(OUTDIR)\tabsrmm.map"
	-@erase "..\..\Bin\Release\Plugins\tabsrmm.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

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
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\tabsrmm_private.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tabsrmm.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib shlwapi.lib /nologo /base:"0x6a540000" /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\tabsrmm.pdb" /map:"$(INTDIR)\tabsrmm.map" /machine:IX86 /out:"..\..\Bin\Release\Plugins\tabsrmm.dll" /implib:"$(OUTDIR)\srmm.lib" /opt:NOWIN98 
LINK32_OBJS= \
	"$(INTDIR)\clist.obj" \
	"$(INTDIR)\colorchooser.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\manager.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\tools.obj" \
	"$(INTDIR)\window.obj" \
	"$(INTDIR)\buttonsbar.obj" \
	"$(INTDIR)\container.obj" \
	"$(INTDIR)\containeroptions.obj" \
	"$(INTDIR)\controls.obj" \
	"$(INTDIR)\eventpopups.obj" \
	"$(INTDIR)\formatting.obj" \
	"$(INTDIR)\generic_msghandlers.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\hotkeyhandler.obj" \
	"$(INTDIR)\ImageDataObject.obj" \
	"$(INTDIR)\mim.obj" \
	"$(INTDIR)\modplus.obj" \
	"$(INTDIR)\msgdialog.obj" \
	"$(INTDIR)\msgdlgutils.obj" \
	"$(INTDIR)\msglog.obj" \
	"$(INTDIR)\msgoptions.obj" \
	"$(INTDIR)\msgoptions_plus.obj" \
	"$(INTDIR)\msgs.obj" \
	"$(INTDIR)\selectcontainer.obj" \
	"$(INTDIR)\sendqueue.obj" \
	"$(INTDIR)\srmm.obj" \
	"$(INTDIR)\tabctrl.obj" \
	"$(INTDIR)\taskbar.obj" \
	"$(INTDIR)\templates.obj" \
	"$(INTDIR)\themeio.obj" \
	"$(INTDIR)\themes.obj" \
	"$(INTDIR)\trayicon.obj" \
	"$(INTDIR)\TSButton.obj" \
	"$(INTDIR)\typingnotify.obj" \
	"$(INTDIR)\userprefs.obj" \
	"$(INTDIR)\tabsrmm_private.res"

"..\..\Bin\Release\Plugins\tabsrmm.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

OUTDIR=.\Debug_Unicode
INTDIR=.\Debug_Unicode
# Begin Custom Macros
OutDir=.\Debug_Unicode
# End Custom Macros

ALL : "..\..\Bin\Debug Unicode\Plugins\tabsrmm.dll" "$(OUTDIR)\tabsrmm.bsc"


CLEAN :
	-@erase "$(INTDIR)\buttonsbar.obj"
	-@erase "$(INTDIR)\buttonsbar.sbr"
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\clist.sbr"
	-@erase "$(INTDIR)\colorchooser.obj"
	-@erase "$(INTDIR)\colorchooser.sbr"
	-@erase "$(INTDIR)\container.obj"
	-@erase "$(INTDIR)\container.sbr"
	-@erase "$(INTDIR)\containeroptions.obj"
	-@erase "$(INTDIR)\containeroptions.sbr"
	-@erase "$(INTDIR)\controls.obj"
	-@erase "$(INTDIR)\controls.sbr"
	-@erase "$(INTDIR)\eventpopups.obj"
	-@erase "$(INTDIR)\eventpopups.sbr"
	-@erase "$(INTDIR)\formatting.obj"
	-@erase "$(INTDIR)\formatting.sbr"
	-@erase "$(INTDIR)\generic_msghandlers.obj"
	-@erase "$(INTDIR)\generic_msghandlers.sbr"
	-@erase "$(INTDIR)\globals.obj"
	-@erase "$(INTDIR)\globals.sbr"
	-@erase "$(INTDIR)\hotkeyhandler.obj"
	-@erase "$(INTDIR)\hotkeyhandler.sbr"
	-@erase "$(INTDIR)\ImageDataObject.obj"
	-@erase "$(INTDIR)\ImageDataObject.sbr"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\log.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\manager.obj"
	-@erase "$(INTDIR)\manager.sbr"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\message.sbr"
	-@erase "$(INTDIR)\mim.obj"
	-@erase "$(INTDIR)\mim.sbr"
	-@erase "$(INTDIR)\modplus.obj"
	-@erase "$(INTDIR)\modplus.sbr"
	-@erase "$(INTDIR)\msgdialog.obj"
	-@erase "$(INTDIR)\msgdialog.sbr"
	-@erase "$(INTDIR)\msgdlgutils.obj"
	-@erase "$(INTDIR)\msgdlgutils.sbr"
	-@erase "$(INTDIR)\msglog.obj"
	-@erase "$(INTDIR)\msglog.sbr"
	-@erase "$(INTDIR)\msgoptions.obj"
	-@erase "$(INTDIR)\msgoptions.sbr"
	-@erase "$(INTDIR)\msgoptions_plus.obj"
	-@erase "$(INTDIR)\msgoptions_plus.sbr"
	-@erase "$(INTDIR)\msgs.obj"
	-@erase "$(INTDIR)\msgs.sbr"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\options.sbr"
	-@erase "$(INTDIR)\selectcontainer.obj"
	-@erase "$(INTDIR)\selectcontainer.sbr"
	-@erase "$(INTDIR)\sendqueue.obj"
	-@erase "$(INTDIR)\sendqueue.sbr"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\services.sbr"
	-@erase "$(INTDIR)\srmm.obj"
	-@erase "$(INTDIR)\srmm.pch"
	-@erase "$(INTDIR)\srmm.sbr"
	-@erase "$(INTDIR)\tabctrl.obj"
	-@erase "$(INTDIR)\tabctrl.sbr"
	-@erase "$(INTDIR)\tabsrmm_private.res"
	-@erase "$(INTDIR)\taskbar.obj"
	-@erase "$(INTDIR)\taskbar.sbr"
	-@erase "$(INTDIR)\templates.obj"
	-@erase "$(INTDIR)\templates.sbr"
	-@erase "$(INTDIR)\themeio.obj"
	-@erase "$(INTDIR)\themeio.sbr"
	-@erase "$(INTDIR)\themes.obj"
	-@erase "$(INTDIR)\themes.sbr"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\tools.sbr"
	-@erase "$(INTDIR)\trayicon.obj"
	-@erase "$(INTDIR)\trayicon.sbr"
	-@erase "$(INTDIR)\TSButton.obj"
	-@erase "$(INTDIR)\TSButton.sbr"
	-@erase "$(INTDIR)\typingnotify.obj"
	-@erase "$(INTDIR)\typingnotify.sbr"
	-@erase "$(INTDIR)\userprefs.obj"
	-@erase "$(INTDIR)\userprefs.sbr"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\window.obj"
	-@erase "$(INTDIR)\window.sbr"
	-@erase "$(OUTDIR)\srmm.exp"
	-@erase "$(OUTDIR)\srmm.pdb"
	-@erase "$(OUTDIR)\tabsrmm.bsc"
	-@erase "..\..\Bin\Debug Unicode\Plugins\tabsrmm.dll"
	-@erase "..\..\Bin\Debug Unicode\Plugins\tabsrmm.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

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
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\tabsrmm_private.res" /d "_DEBUG" /d "UNICODE" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tabsrmm.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\clist.sbr" \
	"$(INTDIR)\colorchooser.sbr" \
	"$(INTDIR)\log.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\manager.sbr" \
	"$(INTDIR)\message.sbr" \
	"$(INTDIR)\options.sbr" \
	"$(INTDIR)\services.sbr" \
	"$(INTDIR)\tools.sbr" \
	"$(INTDIR)\window.sbr" \
	"$(INTDIR)\buttonsbar.sbr" \
	"$(INTDIR)\container.sbr" \
	"$(INTDIR)\containeroptions.sbr" \
	"$(INTDIR)\controls.sbr" \
	"$(INTDIR)\eventpopups.sbr" \
	"$(INTDIR)\formatting.sbr" \
	"$(INTDIR)\generic_msghandlers.sbr" \
	"$(INTDIR)\globals.sbr" \
	"$(INTDIR)\hotkeyhandler.sbr" \
	"$(INTDIR)\ImageDataObject.sbr" \
	"$(INTDIR)\mim.sbr" \
	"$(INTDIR)\modplus.sbr" \
	"$(INTDIR)\msgdialog.sbr" \
	"$(INTDIR)\msgdlgutils.sbr" \
	"$(INTDIR)\msglog.sbr" \
	"$(INTDIR)\msgoptions.sbr" \
	"$(INTDIR)\msgoptions_plus.sbr" \
	"$(INTDIR)\msgs.sbr" \
	"$(INTDIR)\selectcontainer.sbr" \
	"$(INTDIR)\sendqueue.sbr" \
	"$(INTDIR)\srmm.sbr" \
	"$(INTDIR)\tabctrl.sbr" \
	"$(INTDIR)\taskbar.sbr" \
	"$(INTDIR)\templates.sbr" \
	"$(INTDIR)\themeio.sbr" \
	"$(INTDIR)\themes.sbr" \
	"$(INTDIR)\trayicon.sbr" \
	"$(INTDIR)\TSButton.sbr" \
	"$(INTDIR)\typingnotify.sbr" \
	"$(INTDIR)\userprefs.sbr"

"$(OUTDIR)\tabsrmm.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib shlwapi.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\srmm.pdb" /debug /machine:IX86 /out:"..\..\Bin\Debug Unicode\Plugins\tabsrmm.dll" /implib:"$(OUTDIR)\srmm.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\clist.obj" \
	"$(INTDIR)\colorchooser.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\manager.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\tools.obj" \
	"$(INTDIR)\window.obj" \
	"$(INTDIR)\buttonsbar.obj" \
	"$(INTDIR)\container.obj" \
	"$(INTDIR)\containeroptions.obj" \
	"$(INTDIR)\controls.obj" \
	"$(INTDIR)\eventpopups.obj" \
	"$(INTDIR)\formatting.obj" \
	"$(INTDIR)\generic_msghandlers.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\hotkeyhandler.obj" \
	"$(INTDIR)\ImageDataObject.obj" \
	"$(INTDIR)\mim.obj" \
	"$(INTDIR)\modplus.obj" \
	"$(INTDIR)\msgdialog.obj" \
	"$(INTDIR)\msgdlgutils.obj" \
	"$(INTDIR)\msglog.obj" \
	"$(INTDIR)\msgoptions.obj" \
	"$(INTDIR)\msgoptions_plus.obj" \
	"$(INTDIR)\msgs.obj" \
	"$(INTDIR)\selectcontainer.obj" \
	"$(INTDIR)\sendqueue.obj" \
	"$(INTDIR)\srmm.obj" \
	"$(INTDIR)\tabctrl.obj" \
	"$(INTDIR)\taskbar.obj" \
	"$(INTDIR)\templates.obj" \
	"$(INTDIR)\themeio.obj" \
	"$(INTDIR)\themes.obj" \
	"$(INTDIR)\trayicon.obj" \
	"$(INTDIR)\TSButton.obj" \
	"$(INTDIR)\typingnotify.obj" \
	"$(INTDIR)\userprefs.obj" \
	"$(INTDIR)\tabsrmm_private.res"

"..\..\Bin\Debug Unicode\Plugins\tabsrmm.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("tabsrmm.dep")
!INCLUDE "tabsrmm.dep"
!ELSE 
!MESSAGE Warning: cannot find "tabsrmm.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "tabSRMM - Win32 Debug" || "$(CFG)" == "tabSRMM - Win32 Release Unicode" || "$(CFG)" == "tabSRMM - Win32 Release" || "$(CFG)" == "tabSRMM - Win32 Debug Unicode"
SOURCE=.\chat\clist.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\clist.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\clist.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\clist.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\clist.obj"	"$(INTDIR)\clist.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\chat\colorchooser.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\colorchooser.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\colorchooser.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\colorchooser.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\colorchooser.obj"	"$(INTDIR)\colorchooser.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\chat\log.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\log.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\log.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\log.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\log.obj"	"$(INTDIR)\log.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\chat\main.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\main.obj"	"$(INTDIR)\main.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\chat\manager.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\manager.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\manager.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\manager.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\manager.obj"	"$(INTDIR)\manager.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\chat\message.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\message.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\message.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\message.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\message.obj"	"$(INTDIR)\message.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\chat\options.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\options.obj"	"$(INTDIR)\options.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\chat\services.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\services.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\services.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\services.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\services.obj"	"$(INTDIR)\services.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\chat\tools.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\tools.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\tools.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\tools.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\tools.obj"	"$(INTDIR)\tools.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\chat\window.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\window.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\window.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\window.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\window.obj"	"$(INTDIR)\window.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\src\buttonsbar.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\buttonsbar.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\buttonsbar.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\buttonsbar.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\buttonsbar.obj"	"$(INTDIR)\buttonsbar.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\container.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\container.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\container.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\container.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\container.obj"	"$(INTDIR)\container.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\containeroptions.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\containeroptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\containeroptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\containeroptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\containeroptions.obj"	"$(INTDIR)\containeroptions.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\controls.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\controls.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\controls.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\controls.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\controls.obj"	"$(INTDIR)\controls.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\eventpopups.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\eventpopups.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\eventpopups.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\eventpopups.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\eventpopups.obj"	"$(INTDIR)\eventpopups.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\formatting.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\formatting.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\formatting.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\formatting.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\formatting.obj"	"$(INTDIR)\formatting.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\src\generic_msghandlers.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\generic_msghandlers.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\generic_msghandlers.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\generic_msghandlers.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\generic_msghandlers.obj"	"$(INTDIR)\generic_msghandlers.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\globals.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\globals.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\globals.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\globals.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\globals.obj"	"$(INTDIR)\globals.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\hotkeyhandler.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\hotkeyhandler.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\hotkeyhandler.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\hotkeyhandler.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\hotkeyhandler.obj"	"$(INTDIR)\hotkeyhandler.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\ImageDataObject.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\ImageDataObject.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\ImageDataObject.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\ImageDataObject.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\ImageDataObject.obj"	"$(INTDIR)\ImageDataObject.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\src\mim.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\mim.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\mim.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\mim.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\mim.obj"	"$(INTDIR)\mim.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\tabmodplus\modplus.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\modplus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\modplus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\modplus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\modplus.obj"	"$(INTDIR)\modplus.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\src\msgdialog.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\msgdialog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\msgdialog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\msgdialog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\msgdialog.obj"	"$(INTDIR)\msgdialog.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\msgdlgutils.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\msgdlgutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\msgdlgutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\msgdlgutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\msgdlgutils.obj"	"$(INTDIR)\msgdlgutils.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\msglog.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\msglog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\msglog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\msglog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\msglog.obj"	"$(INTDIR)\msglog.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\msgoptions.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\msgoptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\msgoptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\msgoptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\msgoptions.obj"	"$(INTDIR)\msgoptions.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\tabmodplus\msgoptions_plus.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\msgoptions_plus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\msgoptions_plus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\msgoptions_plus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../src/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\msgoptions_plus.obj"	"$(INTDIR)\msgoptions_plus.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\src\msgs.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\msgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\msgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\msgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\msgs.obj"	"$(INTDIR)\msgs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\selectcontainer.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\selectcontainer.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\selectcontainer.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\selectcontainer.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\selectcontainer.obj"	"$(INTDIR)\selectcontainer.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sendqueue.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\sendqueue.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\sendqueue.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\sendqueue.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\sendqueue.obj"	"$(INTDIR)\sendqueue.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\srmm.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\srmm.obj"	"$(INTDIR)\srmm.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\srmm.obj"	"$(INTDIR)\srmm.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c 

"$(INTDIR)\srmm.obj"	"$(INTDIR)\srmm.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 

"$(INTDIR)\srmm.obj"	"$(INTDIR)\srmm.sbr"	"$(INTDIR)\srmm.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\src\tabctrl.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\tabctrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\tabctrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\tabctrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\tabctrl.obj"	"$(INTDIR)\tabctrl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\taskbar.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\taskbar.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\taskbar.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\taskbar.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\taskbar.obj"	"$(INTDIR)\taskbar.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\templates.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\templates.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\templates.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\templates.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\templates.obj"	"$(INTDIR)\templates.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\themeio.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\themeio.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\themeio.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\themeio.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\themeio.obj"	"$(INTDIR)\themeio.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\themes.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\themes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\themes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\themes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\themes.obj"	"$(INTDIR)\themes.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\trayicon.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\trayicon.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\trayicon.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\trayicon.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\trayicon.obj"	"$(INTDIR)\trayicon.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\TSButton.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\TSButton.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\TSButton.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\TSButton.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\TSButton.obj"	"$(INTDIR)\TSButton.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\typingnotify.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\typingnotify.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\typingnotify.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\typingnotify.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\typingnotify.obj"	"$(INTDIR)\typingnotify.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\userprefs.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\userprefs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\userprefs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\userprefs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\userprefs.obj"	"$(INTDIR)\userprefs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\tabsrmm_private.rc

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\tabsrmm_private.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


".\tabsrmm_private.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\tabsrmm_private.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\tabsrmm_private.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

