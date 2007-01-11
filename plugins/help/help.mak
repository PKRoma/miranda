# Microsoft Developer Studio Generated NMAKE File, Based on help.dsp
!IF "$(CFG)" == ""
CFG=help - Win32 Debug Unicode
!MESSAGE No configuration specified. Defaulting to help - Win32 Debug Unicode.
!ENDIF 

!IF "$(CFG)" != "help - Win32 Release" && "$(CFG)" != "help - Win32 Debug" && "$(CFG)" != "help - Win32 Release Unicode" && "$(CFG)" != "help - Win32 Debug Unicode"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "help.mak" CFG="help - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "help - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "help - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "help - Win32 Release Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "help - Win32 Debug Unicode" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "help - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\release\plugins\help.dll"


CLEAN :
	-@erase "$(INTDIR)\datastore.obj"
	-@erase "$(INTDIR)\dlgboxsubclass.obj"
	-@erase "$(INTDIR)\help.pch"
	-@erase "$(INTDIR)\helpdlg.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\streaminout.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\help.exp"
	-@erase "$(OUTDIR)\help.lib"
	-@erase "$(OUTDIR)\help.map"
	-@erase "$(OUTDIR)\help.pdb"
	-@erase "..\..\bin\release\plugins\help.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HELP_EXPORTS" /Fp"$(INTDIR)\help.pch" /Yu"help.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\help.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /base:"0x20100000" /dll /incremental:no /pdb:"$(OUTDIR)\help.pdb" /map:"$(INTDIR)\help.map" /debug /machine:I386 /out:"../../bin/release/plugins/help.dll" /implib:"$(OUTDIR)\help.lib" /ALIGN:4096 /ignore:4108 
LINK32_OBJS= \
	"$(INTDIR)\datastore.obj" \
	"$(INTDIR)\dlgboxsubclass.obj" \
	"$(INTDIR)\helpdlg.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\streaminout.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\release\plugins\help.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "help - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\bin\debug\plugins\help.dll"


CLEAN :
	-@erase "$(INTDIR)\datastore.obj"
	-@erase "$(INTDIR)\dlgboxsubclass.obj"
	-@erase "$(INTDIR)\help.pch"
	-@erase "$(INTDIR)\helpdlg.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\streaminout.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\help.exp"
	-@erase "$(OUTDIR)\help.pdb"
	-@erase "..\..\bin\debug\plugins\help.dll"
	-@erase "..\..\bin\debug\plugins\help.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HELP_EXPORTS" /Fp"$(INTDIR)\help.pch" /Yu"help.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\help.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\help.pdb" /debug /machine:I386 /out:"../../bin/debug/plugins/help.dll" /implib:"$(OUTDIR)\help.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\datastore.obj" \
	"$(INTDIR)\dlgboxsubclass.obj" \
	"$(INTDIR)\helpdlg.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\streaminout.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\debug\plugins\help.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "help - Win32 Release Unicode"

OUTDIR=.\Release_Unicode
INTDIR=.\Release_Unicode

ALL : "..\..\bin\Release Unicode\plugins\help.dll"


CLEAN :
	-@erase "$(INTDIR)\datastore.obj"
	-@erase "$(INTDIR)\dlgboxsubclass.obj"
	-@erase "$(INTDIR)\help.pch"
	-@erase "$(INTDIR)\helpdlg.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\streaminout.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\help.exp"
	-@erase "$(OUTDIR)\help.map"
	-@erase "$(OUTDIR)\help.pdb"
	-@erase "..\..\bin\Release Unicode\plugins\help.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "HELP_EXPORTS" /Fp"$(INTDIR)\help.pch" /Yu"help.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\help.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /base:"0x20100000" /dll /incremental:no /pdb:"$(OUTDIR)\help.pdb" /map:"$(INTDIR)\help.map" /debug /machine:I386 /out:"../../bin/Release Unicode/plugins/help.dll" /implib:"$(OUTDIR)\help.lib" /ALIGN:4096 /ignore:4108 
LINK32_OBJS= \
	"$(INTDIR)\datastore.obj" \
	"$(INTDIR)\dlgboxsubclass.obj" \
	"$(INTDIR)\helpdlg.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\streaminout.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\Release Unicode\plugins\help.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "help - Win32 Debug Unicode"

OUTDIR=.\help___Win32_Debug_Unicode
INTDIR=.\help___Win32_Debug_Unicode

ALL : "..\..\bin\Debug Unicode\plugins\help.dll"


CLEAN :
	-@erase "$(INTDIR)\datastore.obj"
	-@erase "$(INTDIR)\dlgboxsubclass.obj"
	-@erase "$(INTDIR)\help.pch"
	-@erase "$(INTDIR)\helpdlg.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\streaminout.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\help.exp"
	-@erase "$(OUTDIR)\help.pdb"
	-@erase "..\..\bin\Debug Unicode\plugins\help.dll"
	-@erase "..\..\bin\Debug Unicode\plugins\help.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "HELP_EXPORTS" /Fp"$(INTDIR)\help.pch" /Yu"help.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\help.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\help.pdb" /debug /machine:I386 /out:"../../bin/Debug Unicode/plugins/help.dll" /implib:"$(OUTDIR)\help.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\datastore.obj" \
	"$(INTDIR)\dlgboxsubclass.obj" \
	"$(INTDIR)\helpdlg.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\streaminout.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\Debug Unicode\plugins\help.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("help.dep")
!INCLUDE "help.dep"
!ELSE 
!MESSAGE Warning: cannot find "help.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "help - Win32 Release" || "$(CFG)" == "help - Win32 Debug" || "$(CFG)" == "help - Win32 Release Unicode" || "$(CFG)" == "help - Win32 Debug Unicode"
SOURCE=.\datastore.cpp

"$(INTDIR)\datastore.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\help.pch"


SOURCE=.\dlgboxsubclass.cpp

"$(INTDIR)\dlgboxsubclass.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\help.pch"


SOURCE=.\helpdlg.cpp

"$(INTDIR)\helpdlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\help.pch"


SOURCE=.\main.cpp

!IF  "$(CFG)" == "help - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HELP_EXPORTS" /Fp"$(INTDIR)\help.pch" /Yc"help.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\main.obj"	"$(INTDIR)\help.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "help - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HELP_EXPORTS" /Fp"$(INTDIR)\help.pch" /Yc"help.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\main.obj"	"$(INTDIR)\help.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "help - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "HELP_EXPORTS" /Fp"$(INTDIR)\help.pch" /Yc"help.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\main.obj"	"$(INTDIR)\help.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "help - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_USRDLL" /D "HELP_EXPORTS" /Fp"$(INTDIR)\help.pch" /Yc"help.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\main.obj"	"$(INTDIR)\help.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\options.cpp

"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\help.pch"


SOURCE=.\streaminout.cpp

"$(INTDIR)\streaminout.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\help.pch"


SOURCE=.\utils.cpp

"$(INTDIR)\utils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\help.pch"


SOURCE=.\resource.rc

"$(INTDIR)\resource.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

