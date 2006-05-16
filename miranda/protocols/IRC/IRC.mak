# Microsoft Developer Studio Generated NMAKE File, Based on IRC.dsp
!IF "$(CFG)" == ""
CFG=IRC - Win32 Release
!MESSAGE No configuration specified. Defaulting to IRC - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "IRC - Win32 Release" && "$(CFG)" != "IRC - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "IRC.mak" CFG="IRC - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "IRC - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IRC - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "IRC - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\release\plugins\IRC.dll"


CLEAN :
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\commandmonitor.obj"
	-@erase "$(INTDIR)\input.obj"
	-@erase "$(INTDIR)\IRC.res"
	-@erase "$(INTDIR)\irclib.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\output.obj"
	-@erase "$(INTDIR)\scripting.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\windows.obj"
	-@erase "$(OUTDIR)\IRC.exp"
	-@erase "$(OUTDIR)\IRC.lib"
	-@erase "$(OUTDIR)\IRC.map"
	-@erase "$(OUTDIR)\IRC.pdb"
	-@erase "..\..\bin\release\plugins\IRC.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IRC_EXPORTS" /Fp"$(INTDIR)\IRC.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x41d /fo"$(INTDIR)\IRC.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\IRC.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib wsock32.lib /nologo /base:"0x54010000" /dll /incremental:no /pdb:"$(OUTDIR)\IRC.pdb" /map:"$(INTDIR)\IRC.map" /debug /machine:I386 /out:"../../bin/release/plugins/IRC.dll" /implib:"$(OUTDIR)\IRC.lib" 
LINK32_OBJS= \
	"$(INTDIR)\clist.obj" \
	"$(INTDIR)\commandmonitor.obj" \
	"$(INTDIR)\input.obj" \
	"$(INTDIR)\irclib.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\output.obj" \
	"$(INTDIR)\scripting.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\tools.obj" \
	"$(INTDIR)\windows.obj" \
	"$(INTDIR)\IRC.res"

"..\..\bin\release\plugins\IRC.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "IRC - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\bin\debug\plugins\IRC.dll"


CLEAN :
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\commandmonitor.obj"
	-@erase "$(INTDIR)\input.obj"
	-@erase "$(INTDIR)\IRC.res"
	-@erase "$(INTDIR)\irclib.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\output.obj"
	-@erase "$(INTDIR)\scripting.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\windows.obj"
	-@erase "$(OUTDIR)\IRC.exp"
	-@erase "$(OUTDIR)\IRC.lib"
	-@erase "$(OUTDIR)\IRC.map"
	-@erase "$(OUTDIR)\IRC.pdb"
	-@erase "..\..\bin\debug\plugins\IRC.dll"
	-@erase "..\..\bin\debug\plugins\IRC.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IRC_EXPORTS" /Fp"$(INTDIR)\IRC.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x41d /fo"$(INTDIR)\IRC.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\IRC.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib wsock32.lib /nologo /base:"0x54010000" /dll /incremental:yes /pdb:"$(OUTDIR)\IRC.pdb" /map:"$(INTDIR)\IRC.map" /debug /machine:I386 /out:"../../bin/debug/plugins/IRC.dll" /implib:"$(OUTDIR)\IRC.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\clist.obj" \
	"$(INTDIR)\commandmonitor.obj" \
	"$(INTDIR)\input.obj" \
	"$(INTDIR)\irclib.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\output.obj" \
	"$(INTDIR)\scripting.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\tools.obj" \
	"$(INTDIR)\windows.obj" \
	"$(INTDIR)\IRC.res"

"..\..\bin\debug\plugins\IRC.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("IRC.dep")
!INCLUDE "IRC.dep"
!ELSE 
!MESSAGE Warning: cannot find "IRC.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "IRC - Win32 Release" || "$(CFG)" == "IRC - Win32 Debug"
SOURCE=.\clist.cpp

"$(INTDIR)\clist.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\commandmonitor.cpp

"$(INTDIR)\commandmonitor.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\input.cpp

"$(INTDIR)\input.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\irclib.cpp

"$(INTDIR)\irclib.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\main.cpp

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\options.cpp

"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\output.cpp

"$(INTDIR)\output.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\scripting.cpp

"$(INTDIR)\scripting.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\services.cpp

"$(INTDIR)\services.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tools.cpp

"$(INTDIR)\tools.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\windows.cpp

"$(INTDIR)\windows.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IRC.rc

"$(INTDIR)\IRC.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

