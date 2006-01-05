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

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\Bin\Debug\Plugins\tabsrmm.dll"


CLEAN :
	-@erase "$(INTDIR)\container.obj"
	-@erase "$(INTDIR)\containeroptions.obj"
	-@erase "$(INTDIR)\eventpopups.obj"
	-@erase "$(INTDIR)\formatting.obj"
	-@erase "$(INTDIR)\hotkeyhandler.obj"
	-@erase "$(INTDIR)\ImageDataObject.obj"
	-@erase "$(INTDIR)\msgdialog.obj"
	-@erase "$(INTDIR)\msgdlgutils.obj"
	-@erase "$(INTDIR)\msglog.obj"
	-@erase "$(INTDIR)\msgoptions.obj"
	-@erase "$(INTDIR)\msgs.obj"
	-@erase "$(INTDIR)\selectcontainer.obj"
	-@erase "$(INTDIR)\sendqueue.obj"
	-@erase "$(INTDIR)\srmm.obj"
	-@erase "$(INTDIR)\srmm.pch"
	-@erase "$(INTDIR)\tabctrl.obj"
	-@erase "$(INTDIR)\tabsrmm_private.res"
	-@erase "$(INTDIR)\templates.obj"
	-@erase "$(INTDIR)\themes.obj"
	-@erase "$(INTDIR)\trayicon.obj"
	-@erase "$(INTDIR)\TSButton.obj"
	-@erase "$(INTDIR)\URLCtrl.obj"
	-@erase "$(INTDIR)\userprefs.obj"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\srmm.exp"
	-@erase "$(OUTDIR)\srmm.pdb"
	-@erase "..\..\Bin\Debug\Plugins\tabsrmm.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "_MBCS" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\tabsrmm_private.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tabsrmm.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\srmm.pdb" /debug /machine:IX86 /out:"..\..\Bin\Debug\Plugins\tabsrmm.dll" /implib:"$(OUTDIR)\srmm.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\container.obj" \
	"$(INTDIR)\containeroptions.obj" \
	"$(INTDIR)\eventpopups.obj" \
	"$(INTDIR)\formatting.obj" \
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
	"$(INTDIR)\URLCtrl.obj" \
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
	-@erase "$(INTDIR)\container.obj"
	-@erase "$(INTDIR)\containeroptions.obj"
	-@erase "$(INTDIR)\eventpopups.obj"
	-@erase "$(INTDIR)\formatting.obj"
	-@erase "$(INTDIR)\hotkeyhandler.obj"
	-@erase "$(INTDIR)\ImageDataObject.obj"
	-@erase "$(INTDIR)\msgdialog.obj"
	-@erase "$(INTDIR)\msgdlgutils.obj"
	-@erase "$(INTDIR)\msglog.obj"
	-@erase "$(INTDIR)\msgoptions.obj"
	-@erase "$(INTDIR)\msgs.obj"
	-@erase "$(INTDIR)\selectcontainer.obj"
	-@erase "$(INTDIR)\sendqueue.obj"
	-@erase "$(INTDIR)\srmm.obj"
	-@erase "$(INTDIR)\srmm.pch"
	-@erase "$(INTDIR)\tabctrl.obj"
	-@erase "$(INTDIR)\templates.obj"
	-@erase "$(INTDIR)\themes.obj"
	-@erase "$(INTDIR)\trayicon.obj"
	-@erase "$(INTDIR)\TSButton.obj"
	-@erase "$(INTDIR)\URLCtrl.obj"
	-@erase "$(INTDIR)\userprefs.obj"
	-@erase "$(INTDIR)\vc60.pdb"
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
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib /nologo /base:"0x6a540000" /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\tabsrmm.pdb" /map:"$(INTDIR)\tabsrmm.map" /debug /machine:IX86 /out:"..\..\Bin\Release Unicode\Plugins\tabsrmm.dll" /implib:"$(OUTDIR)\srmm.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\container.obj" \
	"$(INTDIR)\containeroptions.obj" \
	"$(INTDIR)\eventpopups.obj" \
	"$(INTDIR)\formatting.obj" \
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
	"$(INTDIR)\URLCtrl.obj" \
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
	-@erase "$(INTDIR)\container.obj"
	-@erase "$(INTDIR)\containeroptions.obj"
	-@erase "$(INTDIR)\eventpopups.obj"
	-@erase "$(INTDIR)\formatting.obj"
	-@erase "$(INTDIR)\hotkeyhandler.obj"
	-@erase "$(INTDIR)\ImageDataObject.obj"
	-@erase "$(INTDIR)\msgdialog.obj"
	-@erase "$(INTDIR)\msgdlgutils.obj"
	-@erase "$(INTDIR)\msglog.obj"
	-@erase "$(INTDIR)\msgoptions.obj"
	-@erase "$(INTDIR)\msgs.obj"
	-@erase "$(INTDIR)\selectcontainer.obj"
	-@erase "$(INTDIR)\sendqueue.obj"
	-@erase "$(INTDIR)\srmm.obj"
	-@erase "$(INTDIR)\srmm.pch"
	-@erase "$(INTDIR)\tabctrl.obj"
	-@erase "$(INTDIR)\tabsrmm_private.res"
	-@erase "$(INTDIR)\templates.obj"
	-@erase "$(INTDIR)\themes.obj"
	-@erase "$(INTDIR)\trayicon.obj"
	-@erase "$(INTDIR)\TSButton.obj"
	-@erase "$(INTDIR)\URLCtrl.obj"
	-@erase "$(INTDIR)\userprefs.obj"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\srmm.exp"
	-@erase "$(OUTDIR)\srmm.lib"
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
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib /nologo /base:"0x6a540000" /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\tabsrmm.pdb" /map:"$(INTDIR)\tabsrmm.map" /debug /machine:IX86 /out:"..\..\Bin\Release\Plugins\tabsrmm.dll" /implib:"$(OUTDIR)\srmm.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\container.obj" \
	"$(INTDIR)\containeroptions.obj" \
	"$(INTDIR)\eventpopups.obj" \
	"$(INTDIR)\formatting.obj" \
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
	"$(INTDIR)\URLCtrl.obj" \
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
	-@erase "$(INTDIR)\container.obj"
	-@erase "$(INTDIR)\container.sbr"
	-@erase "$(INTDIR)\containeroptions.obj"
	-@erase "$(INTDIR)\containeroptions.sbr"
	-@erase "$(INTDIR)\eventpopups.obj"
	-@erase "$(INTDIR)\eventpopups.sbr"
	-@erase "$(INTDIR)\formatting.obj"
	-@erase "$(INTDIR)\formatting.sbr"
	-@erase "$(INTDIR)\hotkeyhandler.obj"
	-@erase "$(INTDIR)\hotkeyhandler.sbr"
	-@erase "$(INTDIR)\ImageDataObject.obj"
	-@erase "$(INTDIR)\ImageDataObject.sbr"
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
	-@erase "$(INTDIR)\selectcontainer.obj"
	-@erase "$(INTDIR)\selectcontainer.sbr"
	-@erase "$(INTDIR)\sendqueue.obj"
	-@erase "$(INTDIR)\sendqueue.sbr"
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
	-@erase "$(INTDIR)\trayicon.obj"
	-@erase "$(INTDIR)\trayicon.sbr"
	-@erase "$(INTDIR)\TSButton.obj"
	-@erase "$(INTDIR)\TSButton.sbr"
	-@erase "$(INTDIR)\URLCtrl.obj"
	-@erase "$(INTDIR)\URLCtrl.sbr"
	-@erase "$(INTDIR)\userprefs.obj"
	-@erase "$(INTDIR)\userprefs.sbr"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\srmm.exp"
	-@erase "$(OUTDIR)\srmm.pdb"
	-@erase "$(OUTDIR)\tabsrmm.bsc"
	-@erase "..\..\Bin\Debug Unicode\Plugins\tabsrmm.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "SRMM_EXPORTS" /D "UNICODE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\srmm.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\tabsrmm_private.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tabsrmm.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\container.sbr" \
	"$(INTDIR)\containeroptions.sbr" \
	"$(INTDIR)\eventpopups.sbr" \
	"$(INTDIR)\formatting.sbr" \
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
	"$(INTDIR)\URLCtrl.sbr" \
	"$(INTDIR)\userprefs.sbr"

"$(OUTDIR)\tabsrmm.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib msimg32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\srmm.pdb" /debug /machine:IX86 /out:"..\..\Bin\Debug Unicode\Plugins\tabsrmm.dll" /implib:"$(OUTDIR)\srmm.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\container.obj" \
	"$(INTDIR)\containeroptions.obj" \
	"$(INTDIR)\eventpopups.obj" \
	"$(INTDIR)\formatting.obj" \
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
	"$(INTDIR)\URLCtrl.obj" \
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
SOURCE=.\container.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\container.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\container.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\container.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\container.obj"	"$(INTDIR)\container.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ENDIF 

SOURCE=.\containeroptions.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\containeroptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\containeroptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\containeroptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\containeroptions.obj"	"$(INTDIR)\containeroptions.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ENDIF 

SOURCE=eventpopups.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\eventpopups.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\eventpopups.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\eventpopups.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\eventpopups.obj"	"$(INTDIR)\eventpopups.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


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

SOURCE=.\hotkeyhandler.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\hotkeyhandler.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\hotkeyhandler.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\hotkeyhandler.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\hotkeyhandler.obj"	"$(INTDIR)\hotkeyhandler.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


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


"$(INTDIR)\msgdlgutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\msgdlgutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\msgdlgutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\msgdlgutils.obj"	"$(INTDIR)\msgdlgutils.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


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


"$(INTDIR)\selectcontainer.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\selectcontainer.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\selectcontainer.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\selectcontainer.obj"	"$(INTDIR)\selectcontainer.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ENDIF 

SOURCE=sendqueue.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\sendqueue.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\sendqueue.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\sendqueue.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\sendqueue.obj"	"$(INTDIR)\sendqueue.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


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


"$(INTDIR)\tabctrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\tabctrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\tabctrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\tabctrl.obj"	"$(INTDIR)\tabctrl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ENDIF 

SOURCE=templates.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\templates.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\templates.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\templates.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\templates.obj"	"$(INTDIR)\templates.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ENDIF 

SOURCE=.\themes.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\themes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\themes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\themes.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\themes.obj"	"$(INTDIR)\themes.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ENDIF 

SOURCE=trayicon.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\trayicon.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\trayicon.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\trayicon.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\trayicon.obj"	"$(INTDIR)\trayicon.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ENDIF 

SOURCE=TSButton.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\TSButton.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\TSButton.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\TSButton.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\TSButton.obj"	"$(INTDIR)\TSButton.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ENDIF 

SOURCE=.\URLCtrl.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\URLCtrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\URLCtrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\URLCtrl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\URLCtrl.obj"	"$(INTDIR)\URLCtrl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ENDIF 

SOURCE=userprefs.c

!IF  "$(CFG)" == "tabSRMM - Win32 Debug"


"$(INTDIR)\userprefs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release Unicode"


"$(INTDIR)\userprefs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Release"


"$(INTDIR)\userprefs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


!ELSEIF  "$(CFG)" == "tabSRMM - Win32 Debug Unicode"


"$(INTDIR)\userprefs.obj"	"$(INTDIR)\userprefs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\srmm.pch"


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

