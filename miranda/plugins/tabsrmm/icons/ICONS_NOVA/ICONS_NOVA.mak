# Microsoft Developer Studio Generated NMAKE File, Based on ICONS_NOVA.dsp
!IF "$(CFG)" == ""
CFG=ICONS_NOVA - Win32 Debug
!MESSAGE No configuration specified. Defaulting to ICONS_NOVA - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ICONS_NOVA - Win32 Release" && "$(CFG)" != "ICONS_NOVA - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ICONS_NOVA.mak" CFG="ICONS_NOVA - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ICONS_NOVA - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ICONS_NOVA - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "ICONS_NOVA - Win32 Release"

OUTDIR=.\../../../bin/Release/Icons
INTDIR=.\Release

ALL : ".\tabsrmm_icons.dll"


CLEAN :
	-@erase "$(INTDIR)\ICONS_NOVA.res"
	-@erase "$(OUTDIR)\tabsrmm_icons.exp"
	-@erase "$(OUTDIR)\tabsrmm_icons.lib"
	-@erase ".\tabsrmm_icons.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ICONS_NOVA_EXPORTS" /Fp"$(INTDIR)\ICONS_NOVA.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0xc07 /fo"$(INTDIR)\ICONS_NOVA.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ICONS_NOVA.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /dll /pdb:none /machine:I386 /out:"tabsrmm_icons.dll" /implib:"$(OUTDIR)\tabsrmm_icons.lib" /noentry 
LINK32_OBJS= \
	"$(INTDIR)\ICONS_NOVA.res"

".\tabsrmm_icons.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ICONS_NOVA - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : ".\Release\tabsrmm_icons.dll"


CLEAN :
	-@erase "$(INTDIR)\ICONS_NOVA.res"
	-@erase "$(OUTDIR)\tabsrmm_icons.exp"
	-@erase "$(OUTDIR)\tabsrmm_icons.lib"
	-@erase ".\Release\tabsrmm_icons.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ICONS_NOVA_EXPORTS" /Fp"$(INTDIR)\ICONS_NOVA.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0xc07 /fo"$(INTDIR)\ICONS_NOVA.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ICONS_NOVA.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /dll /pdb:none /machine:I386 /out:"Release/tabsrmm_icons.dll" /implib:"$(OUTDIR)\tabsrmm_icons.lib" 
LINK32_OBJS= \
	"$(INTDIR)\ICONS_NOVA.res"

".\Release\tabsrmm_icons.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("ICONS_NOVA.dep")
!INCLUDE "ICONS_NOVA.dep"
!ELSE 
!MESSAGE Warning: cannot find "ICONS_NOVA.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "ICONS_NOVA - Win32 Release" || "$(CFG)" == "ICONS_NOVA - Win32 Debug"
SOURCE=.\ICONS_NOVA.rc

"$(INTDIR)\ICONS_NOVA.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

