# Microsoft Developer Studio Generated NMAKE File, Based on tabsrmm.dsp
!IF "$(CFG)" == ""
CFG=tabSRMM - Win32 Release Unicode
!MESSAGE No configuration specified. Defaulting to tabSRMM - Win32 Release Unicode.
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

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\Bin\Debug\Plugins\tabsrmm.dll"


CLEAN :
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\colorchooser.obj"
	-@erase "$(INTDIR)\container.obj"
	-@erase "$(INTDIR)\containeroptions.obj"
	-@erase "$(INTDIR)\eventpopups.obj"
	-@erase "$(INTDIR)\formatting.obj"
	-@erase "$(INTDIR)\generic_msghandlers.obj"
	-@erase "$(INTDIR)\hotkeyhandler.obj"
	-@erase "$(INTDIR)\ImageDataObject.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\manager.obj"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\msgdialog.obj"
	-@erase "$(INTDIR)\msgdlgutils.obj"
	-@erase "$(INTDIR)\msglog.obj"
	-@erase "$(INTDIR)\msgoptions.obj"
	-@erase "$(INTDIR)\msgs.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\selectcontainer.obj"
	-@erase "$(INTDIR)\sendqueue.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\srmm.obj"
	-@erase "$(INTDIR)\srmm.pch"
	-@erase "$(INTDIR)\tabctrl.obj"
	-@erase "$(INTDIR)\tabsrmm_private.res"
	-@erase "$(INTDIR)\templates.obj"
	-@erase "$(INTDIR)\themes.obj"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\trayicon.obj"
	-@erase "$(INTDIR)\TSButton.obj"
	-@erase "$(INTDIR)\userprefs.obj"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\window.obj"
	-@erase "$(OUTDIR)\srmm.exp"
	-@erase "$(OUTDIR)\srmm.pdb"
	-@erase "..\..\Bin\Debug\Plugins\tabsrmm.dll"
	-@erase "..\..\Bin\Debug\Plugins\tabsrmm.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32
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
	"$(INTDIR)\container.obj" \
	"$(INTDIR)\containeroptions.obj" \
	"$(INTDIR)\eventpopups.obj" \
	"$(INTDIR)\formatting.obj" \
	"$(INTDIR)\generic_msghandlers.obj" \
	"$(INTDIR)\hotkeyhandler.obj" \
	"$(INTDIR)\ImageDataObject.obj" \
	"$(INTDIR)\msgdialog.obj" \
	"$(INTDIR)\msgdlgutils.obj" \
	"$(INTDIR)\msglog.obj" \
	"$(INTDIR)\msgoptions.obj" \
	"$(INTDIR)\msgs.obj" \
	"$(INTDIR)\selectcontainer.obj" \
	"$(INTDIR)\sendqueue.obj" \
	"$(INTDIR)\srmm.obj" \
	"$(INTDIR)\tabctrl.obj" \
	"$(INTDIR)\templates.obj" \
	"$(INTDIR)\themes.obj" \
	"$(INTDIR)\trayicon.obj" \
	"$(INTDIR)\TSButton.obj" \
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
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\colorchooser.obj"
	-@erase "$(INTDIR)\container.obj"
	-@erase "$(INTDIR)\containeroptions.obj"
	-@erase "$(INTDIR)\eventpopups.obj"
	-@erase "$(INTDIR)\formatting.obj"
	-@erase "$(INTDIR)\generic_msghandlers.obj"
	-@erase "$(INTDIR)\hotkeyhandler.obj"
	-@erase "$(INTDIR)\ImageDataObject.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\manager.obj"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\msgdialog.obj"
	-@erase "$(INTDIR)\msgdlgutils.obj"
	-@erase "$(INTDIR)\msglog.obj"
	-@erase "$(INTDIR)\msgoptions.obj"
	-@erase "$(INTDIR)\msgs.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\selectcontainer.obj"
	-@erase "$(INTDIR)\sendqueue.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\srmm.obj"
	-@erase "$(INTDIR)\srmm.pch"
	-@erase "$(INTDIR)\tabctrl.obj"
	-@erase "$(INTDIR)\templates.obj"
	-@erase "$(INTDIR)\themes.obj"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\trayicon.obj"
	-@erase "$(INTDIR)\TSButton.obj"
	-@erase "$(INTDIR)\userprefs.obj"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\window.obj"
	-@erase "$(OUTDIR)\srmm.exp"
	-@erase "$(OUTDIR)\tabsrmm.map"
	-@erase "$(OUTDIR)\tabsrmm.pdb"
	-@erase "..\..\Bin\Release Unicode\Plugins\tabsrmm.dll"
	-@erase ".\tabsrmm_private.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32
RSC_PROJ=/l 0x809 /fo".\tabsrmm_private.res" /d "NDEBUG" /d "UNICODE"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tabsrmm.bsc"
BSC32_SBRS= \

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib shlwapi.lib /nologo /base:"0x6a540000" /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\tabsrmm.pdb" /map:"$(INTDIR)\tabsrmm.map" /debug /machine:IX86 /out:"..\..\Bin\Release Unicode\Plugins\tabsrmm.dll" /implib:"$(OUTDIR)\srmm.lib" /opt:NOWIN98
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
	"$(INTDIR)\container.obj" \
	"$(INTDIR)\containeroptions.obj" \
	"$(INTDIR)\eventpopups.obj" \
	"$(INTDIR)\formatting.obj" \
	"$(INTDIR)\generic_msghandlers.obj" \
	"$(INTDIR)\hotkeyhandler.obj" \
	"$(INTDIR)\ImageDataObject.obj" \
	"$(INTDIR)\msgdialog.obj" \
	"$(INTDIR)\msgdlgutils.obj" \
	"$(INTDIR)\msglog.obj" \
	"$(INTDIR)\msgoptions.obj" \
	"$(INTDIR)\msgs.obj" \
	"$(INTDIR)\selectcontainer.obj" \
	"$(INTDIR)\sendqueue.obj" \
	"$(INTDIR)\srmm.obj" \
	"$(INTDIR)\tabctrl.obj" \
	"$(INTDIR)\templates.obj" \
	"$(INTDIR)\themes.obj" \
	"$(INTDIR)\trayicon.obj" \
	"$(INTDIR)\TSButton.obj" \
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
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\colorchooser.obj"
	-@erase "$(INTDIR)\container.obj"
	-@erase "$(INTDIR)\containeroptions.obj"
	-@erase "$(INTDIR)\eventpopups.obj"
	-@erase "$(INTDIR)\formatting.obj"
	-@erase "$(INTDIR)\generic_msghandlers.obj"
	-@erase "$(INTDIR)\hotkeyhandler.obj"
	-@erase "$(INTDIR)\ImageDataObject.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\manager.obj"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\msgdialog.obj"
	-@erase "$(INTDIR)\msgdlgutils.obj"
	-@erase "$(INTDIR)\msglog.obj"
	-@erase "$(INTDIR)\msgoptions.obj"
	-@erase "$(INTDIR)\msgs.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\selectcontainer.obj"
	-@erase "$(INTDIR)\sendqueue.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\srmm.obj"
	-@erase "$(INTDIR)\srmm.pch"
	-@erase "$(INTDIR)\tabctrl.obj"
	-@erase "$(INTDIR)\tabsrmm_private.res"
	-@erase "$(INTDIR)\templates.obj"
	-@erase "$(INTDIR)\themes.obj"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\trayicon.obj"
	-@erase "$(INTDIR)\TSButton.obj"
	-@erase "$(INTDIR)\userprefs.obj"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\window.obj"
	-@erase "$(OUTDIR)\srmm.exp"
	-@erase "$(OUTDIR)\tabsrmm.map"
	-@erase "$(OUTDIR)\tabsrmm.pdb"
	-@erase "..\..\Bin\Release\Plugins\tabsrmm.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\tabsrmm_private.res" /d "NDEBUG"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tabsrmm.bsc"
BSC32_SBRS= \

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib shlwapi.lib /nologo /base:"0x6a540000" /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\tabsrmm.pdb" /map:"$(INTDIR)\tabsrmm.map" /debug /machine:IX86 /out:"..\..\Bin\Release\Plugins\tabsrmm.dll" /implib:"$(OUTDIR)\srmm.lib" /pdbtype:sept /opt:NOWIN98
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
	"$(INTDIR)\container.obj" \
	"$(INTDIR)\containeroptions.obj" \
	"$(INTDIR)\eventpopups.obj" \
	"$(INTDIR)\formatting.obj" \
	"$(INTDIR)\generic_msghandlers.obj" \
	"$(INTDIR)\hotkeyhandler.obj" \
	"$(INTDIR)\ImageDataObject.obj" \
	"$(INTDIR)\msgdialog.obj" \
	"$(INTDIR)\msgdlgutils.obj" \
	"$(INTDIR)\msglog.obj" \
	"$(INTDIR)\msgoptions.obj" \
	"$(INTDIR)\msgs.obj" \
	"$(INTDIR)\selectcontainer.obj" \
	"$(INTDIR)\sendqueue.obj" \
	"$(INTDIR)\srmm.obj" \
	"$(INTDIR)\tabctrl.obj" \
	"$(INTDIR)\templates.obj" \
	"$(INTDIR)\themes.obj" \
	"$(INTDIR)\trayicon.obj" \
	"$(INTDIR)\TSButton.obj" \
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
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\clist.sbr"
	-@erase "$(INTDIR)\colorchooser.obj"
	-@erase "$(INTDIR)\colorchooser.sbr"
	-@erase "$(INTDIR)\container.obj"
	-@erase "$(INTDIR)\container.sbr"
	-@erase "$(INTDIR)\containeroptions.obj"
	-@erase "$(INTDIR)\containeroptions.sbr"
	-@erase "$(INTDIR)\eventpopups.obj"
	-@erase "$(INTDIR)\eventpopups.sbr"
	-@erase "$(INTDIR)\formatting.obj"
	-@erase "$(INTDIR)\formatting.sbr"
	-@erase "$(INTDIR)\generic_msghandlers.obj"
	-@erase "$(INTDIR)\generic_msghandlers.sbr"
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
	-@erase "$(INTDIR)\msgdialog.obj"
	-@erase "$(INTDIR)\msgdialog.sbr"
	-@erase "$(INTDIR)\msgdlgutils.obj"
	-@erase "$(INTDIR)\msgdlgutils.sbr"
	-@erase "$(INTDIR)\msglog.obj"
	-@erase "$(INTDIR)\msglog.sbr"
	-@erase "$(INTDIR)\msgoptions.obj"
	-@erase "$(INTDIR)\msgoptions.sbr"
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
	-@erase "$(INTDIR)\templates.obj"
	-@erase "$(INTDIR)\templates.sbr"
	-@erase "$(INTDIR)\themes.obj"
	-@erase "$(INTDIR)\themes.sbr"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\tools.sbr"
	-@erase "$(INTDIR)\trayicon.obj"
	-@erase "$(INTDIR)\trayicon.sbr"
	-@erase "$(INTDIR)\TSButton.obj"
	-@erase "$(INTDIR)\TSButton.sbr"
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

CPP_PROJ=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32
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
	"$(INTDIR)\container.sbr" \
	"$(INTDIR)\containeroptions.sbr" \
	"$(INTDIR)\eventpopups.sbr" \
	"$(INTDIR)\formatting.sbr" \
	"$(INTDIR)\generic_msghandlers.sbr" \
	"$(INTDIR)\hotkeyhandler.sbr" \
	"$(INTDIR)\ImageDataObject.sbr" \
	"$(INTDIR)\msgdialog.sbr" \
	"$(INTDIR)\msgdlgutils.sbr" \
	"$(INTDIR)\msglog.sbr" \
	"$(INTDIR)\msgoptions.sbr" \
	"$(INTDIR)\msgs.sbr" \
	"$(INTDIR)\selectcontainer.sbr" \
	"$(INTDIR)\sendqueue.sbr" \
	"$(INTDIR)\srmm.sbr" \
	"$(INTDIR)\tabctrl.sbr" \
	"$(INTDIR)\templates.sbr" \
	"$(INTDIR)\themes.sbr" \
	"$(INTDIR)\trayicon.sbr" \
	"$(INTDIR)\TSButton.sbr" \
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
	"$(INTDIR)\container.obj" \
	"$(INTDIR)\containeroptions.obj" \
	"$(INTDIR)\eventpopups.obj" \
	"$(INTDIR)\formatting.obj" \
	"$(INTDIR)\generic_msghandlers.obj" \
	"$(INTDIR)\hotkeyhandler.obj" \
	"$(INTDIR)\ImageDataObject.obj" \
	"$(INTDIR)\msgdialog.obj" \
	"$(INTDIR)\msgdlgutils.obj" \
	"$(INTDIR)\msglog.obj" \
	"$(INTDIR)\msgoptions.obj" \
	"$(INTDIR)\msgs.obj" \
	"$(INTDIR)\selectcontainer.obj" \
	"$(INTDIR)\sendqueue.obj" \
	"$(INTDIR)\srmm.obj" \
	"$(INTDIR)\tabctrl.obj" \
	"$(INTDIR)\templates.obj" \
	"$(INTDIR)\themes.obj" \
	"$(INTDIR)\trayicon.obj" \
	"$(INTDIR)\TSButton.obj" \
	"$(INTDIR)\userprefs.obj" \
	"$(INTDIR)\tabsrmm_private.res"

"..\..\Bin\Debug Unicode\Plugins\tabsrmm.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF

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


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("tabsrmm.dep")
!INCLUDE "tabsrmm.dep"
!ELSE
!MESSAGE Warning: cannot find "tabsrmm.dep"
!ENDIF
!ENDIF


!IF "$(CFG)" == "tabSRMM - Win32 Debug" || "$(CFG)" == "tabSRMM - Win32 Release Unicode" || "$(CFG)" == "tabSRMM - Win32 Release" || "$(CFG)" == "tabSRMM - Win32 Debug Unicode"
SOURCE=.\chat\clist.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\clist.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\clist.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\clist.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\clist.obj"	"$(INTDIR)\clist.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\chat\colorchooser.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\colorchooser.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\colorchooser.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\colorchooser.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\colorchooser.obj"	"$(INTDIR)\colorchooser.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\chat\log.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\log.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\log.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\log.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\log.obj"	"$(INTDIR)\log.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\chat\main.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\main.obj"	"$(INTDIR)\main.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\chat\manager.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\manager.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\manager.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\manager.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\manager.obj"	"$(INTDIR)\manager.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\chat\message.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\message.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\message.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\message.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\message.obj"	"$(INTDIR)\message.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\chat\options.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\options.obj"	"$(INTDIR)\options.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\chat\services.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\services.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\services.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\services.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\services.obj"	"$(INTDIR)\services.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\chat\tools.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\tools.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\tools.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\tools.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\tools.obj"	"$(INTDIR)\tools.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\chat\window.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\window.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\window.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\window.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"../commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\window.obj"	"$(INTDIR)\window.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\container.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\container.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\container.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\container.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\container.obj"	"$(INTDIR)\container.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\containeroptions.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\containeroptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\containeroptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\containeroptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\containeroptions.obj"	"$(INTDIR)\containeroptions.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=eventpopups.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\eventpopups.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\eventpopups.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\eventpopups.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\eventpopups.obj"	"$(INTDIR)\eventpopups.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\formatting.cpp

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\formatting.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF  /EHsc /c

"$(INTDIR)\formatting.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF  /EHsc /c

"$(INTDIR)\formatting.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\formatting.obj"	"$(INTDIR)\formatting.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\generic_msghandlers.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\generic_msghandlers.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\generic_msghandlers.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\generic_msghandlers.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\generic_msghandlers.obj"	"$(INTDIR)\generic_msghandlers.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\hotkeyhandler.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\hotkeyhandler.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\hotkeyhandler.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\hotkeyhandler.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\hotkeyhandler.obj"	"$(INTDIR)\hotkeyhandler.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=ImageDataObject.cpp

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

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\ImageDataObject.obj"	"$(INTDIR)\ImageDataObject.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=msgdialog.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ  /GZ /c

"$(INTDIR)\msgdialog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\msgdialog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\msgdialog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ  /GZ /c

"$(INTDIR)\msgdialog.obj"	"$(INTDIR)\msgdialog.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=msgdlgutils.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\msgdlgutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\msgdlgutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\msgdlgutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\msgdlgutils.obj"	"$(INTDIR)\msgdlgutils.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=msglog.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ  /GZ /c

"$(INTDIR)\msglog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\msglog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\msglog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ  /GZ /c

"$(INTDIR)\msglog.obj"	"$(INTDIR)\msglog.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=msgoptions.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ  /GZ /c

"$(INTDIR)\msgoptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\msgoptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\msgoptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ  /GZ /c

"$(INTDIR)\msgoptions.obj"	"$(INTDIR)\msgoptions.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=msgs.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ  /GZ /c

"$(INTDIR)\msgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\msgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\msgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ  /GZ /c

"$(INTDIR)\msgs.obj"	"$(INTDIR)\msgs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\selectcontainer.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\selectcontainer.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\selectcontainer.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\selectcontainer.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\selectcontainer.obj"	"$(INTDIR)\selectcontainer.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=sendqueue.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\sendqueue.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\sendqueue.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\sendqueue.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\sendqueue.obj"	"$(INTDIR)\sendqueue.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=srmm.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ  /GZ /c

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

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ  /GZ /c

"$(INTDIR)\srmm.obj"	"$(INTDIR)\srmm.sbr"	"$(INTDIR)\srmm.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\tabctrl.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\tabctrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\tabctrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\tabctrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\tabctrl.obj"	"$(INTDIR)\tabctrl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=templates.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\templates.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\templates.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\templates.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\templates.obj"	"$(INTDIR)\templates.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\themes.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\themes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\themes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\themes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\themes.obj"	"$(INTDIR)\themes.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=trayicon.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\trayicon.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\trayicon.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\trayicon.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\trayicon.obj"	"$(INTDIR)\trayicon.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=TSButton.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\TSButton.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\TSButton.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\TSButton.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\TSButton.obj"	"$(INTDIR)\TSButton.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=userprefs.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\userprefs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\userprefs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GF /c

"$(INTDIR)\userprefs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c

"$(INTDIR)\userprefs.obj"	"$(INTDIR)\userprefs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


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
