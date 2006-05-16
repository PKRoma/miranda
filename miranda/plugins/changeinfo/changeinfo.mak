# Microsoft Developer Studio Generated NMAKE File, Based on changeinfo.dsp
!IF "$(CFG)" == ""
CFG=changeinfo - Win32 Debug
!MESSAGE No configuration specified. Defaulting to changeinfo - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "changeinfo - Win32 Release" && "$(CFG)" != "changeinfo - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "changeinfo.mak" CFG="changeinfo - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "changeinfo - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "changeinfo - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "changeinfo - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "..\..\bin\release\plugins\changeinfo.dll" "$(OUTDIR)\changeinfo.bsc"


CLEAN :
	-@erase "$(INTDIR)\constants.obj"
	-@erase "$(INTDIR)\constants.sbr"
	-@erase "$(INTDIR)\db.obj"
	-@erase "$(INTDIR)\db.sbr"
	-@erase "$(INTDIR)\dlgproc.obj"
	-@erase "$(INTDIR)\dlgproc.sbr"
	-@erase "$(INTDIR)\editlist.obj"
	-@erase "$(INTDIR)\editlist.sbr"
	-@erase "$(INTDIR)\editstring.obj"
	-@erase "$(INTDIR)\editstring.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\upload.obj"
	-@erase "$(INTDIR)\upload.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\changeinfo.bsc"
	-@erase "$(OUTDIR)\changeinfo.exp"
	-@erase "$(OUTDIR)\changeinfo.lib"
	-@erase "$(OUTDIR)\changeinfo.map"
	-@erase "$(OUTDIR)\changeinfo.pdb"
	-@erase "..\..\bin\release\plugins\changeinfo.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CHANGEINFO_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\changeinfo.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\changeinfo.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\constants.sbr" \
	"$(INTDIR)\db.sbr" \
	"$(INTDIR)\dlgproc.sbr" \
	"$(INTDIR)\editlist.sbr" \
	"$(INTDIR)\editstring.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\upload.sbr"

"$(OUTDIR)\changeinfo.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /base:"0x24000000" /dll /incremental:no /pdb:"$(OUTDIR)\changeinfo.pdb" /map:"$(INTDIR)\changeinfo.map" /debug /machine:I386 /out:"../../bin/release/plugins/changeinfo.dll" /implib:"$(OUTDIR)\changeinfo.lib" /ALIGN:4096 /ignore:4108 
LINK32_OBJS= \
	"$(INTDIR)\constants.obj" \
	"$(INTDIR)\db.obj" \
	"$(INTDIR)\dlgproc.obj" \
	"$(INTDIR)\editlist.obj" \
	"$(INTDIR)\editstring.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\upload.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\release\plugins\changeinfo.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "changeinfo - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\bin\debug\plugins\changeinfo.dll"


CLEAN :
	-@erase "$(INTDIR)\constants.obj"
	-@erase "$(INTDIR)\db.obj"
	-@erase "$(INTDIR)\dlgproc.obj"
	-@erase "$(INTDIR)\editlist.obj"
	-@erase "$(INTDIR)\editstring.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\upload.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\changeinfo.exp"
	-@erase "$(OUTDIR)\changeinfo.lib"
	-@erase "$(OUTDIR)\changeinfo.map"
	-@erase "$(OUTDIR)\changeinfo.pdb"
	-@erase "..\..\bin\debug\plugins\changeinfo.dll"
	-@erase "..\..\bin\debug\plugins\changeinfo.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CHANGEINFO_EXPORTS" /Fp"$(INTDIR)\changeinfo.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\changeinfo.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\changeinfo.pdb" /map:"$(INTDIR)\changeinfo.map" /debug /machine:I386 /out:"../../bin/debug/plugins/changeinfo.dll" /implib:"$(OUTDIR)\changeinfo.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\constants.obj" \
	"$(INTDIR)\db.obj" \
	"$(INTDIR)\dlgproc.obj" \
	"$(INTDIR)\editlist.obj" \
	"$(INTDIR)\editstring.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\upload.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\debug\plugins\changeinfo.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("changeinfo.dep")
!INCLUDE "changeinfo.dep"
!ELSE 
!MESSAGE Warning: cannot find "changeinfo.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "changeinfo - Win32 Release" || "$(CFG)" == "changeinfo - Win32 Debug"
SOURCE=.\constants.cpp

!IF  "$(CFG)" == "changeinfo - Win32 Release"


"$(INTDIR)\constants.obj"	"$(INTDIR)\constants.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "changeinfo - Win32 Debug"


"$(INTDIR)\constants.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\db.cpp

!IF  "$(CFG)" == "changeinfo - Win32 Release"


"$(INTDIR)\db.obj"	"$(INTDIR)\db.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "changeinfo - Win32 Debug"


"$(INTDIR)\db.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\dlgproc.cpp

!IF  "$(CFG)" == "changeinfo - Win32 Release"


"$(INTDIR)\dlgproc.obj"	"$(INTDIR)\dlgproc.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "changeinfo - Win32 Debug"


"$(INTDIR)\dlgproc.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\editlist.cpp

!IF  "$(CFG)" == "changeinfo - Win32 Release"


"$(INTDIR)\editlist.obj"	"$(INTDIR)\editlist.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "changeinfo - Win32 Debug"


"$(INTDIR)\editlist.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\editstring.cpp

!IF  "$(CFG)" == "changeinfo - Win32 Release"


"$(INTDIR)\editstring.obj"	"$(INTDIR)\editstring.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "changeinfo - Win32 Debug"


"$(INTDIR)\editstring.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\main.cpp

!IF  "$(CFG)" == "changeinfo - Win32 Release"


"$(INTDIR)\main.obj"	"$(INTDIR)\main.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "changeinfo - Win32 Debug"


"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\upload.cpp

!IF  "$(CFG)" == "changeinfo - Win32 Release"


"$(INTDIR)\upload.obj"	"$(INTDIR)\upload.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "changeinfo - Win32 Debug"


"$(INTDIR)\upload.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\resource.rc

"$(INTDIR)\resource.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

