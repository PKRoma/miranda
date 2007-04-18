# Microsoft Developer Studio Generated NMAKE File, Based on Gadu-Gadu.dsp
!IF "$(CFG)" == ""
CFG=GG - Win32 Release
!MESSAGE No configuration specified. Defaulting to GG - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "GG - Win32 Release" && "$(CFG)" != "GG - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Gadu-Gadu.mak" CFG="GG - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GG - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "GG - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "GG - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\release\plugins\GG.dll"


CLEAN :
	-@erase "$(INTDIR)\common.obj"
	-@erase "$(INTDIR)\core.obj"
	-@erase "$(INTDIR)\dcc.obj"
	-@erase "$(INTDIR)\dialogs.obj"
	-@erase "$(INTDIR)\dynstuff.obj"
	-@erase "$(INTDIR)\events.obj"
	-@erase "$(INTDIR)\filetransfer.obj"
	-@erase "$(INTDIR)\gg.obj"
	-@erase "$(INTDIR)\groupchat.obj"
	-@erase "$(INTDIR)\http.obj"
	-@erase "$(INTDIR)\image.obj"
	-@erase "$(INTDIR)\import.obj"
	-@erase "$(INTDIR)\keepalive.obj"
	-@erase "$(INTDIR)\libgadu.obj"
	-@erase "$(INTDIR)\ownerinfo.obj"
	-@erase "$(INTDIR)\pthread.obj"
	-@erase "$(INTDIR)\pubdir.obj"
	-@erase "$(INTDIR)\pubdir50.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\sha1.obj"
	-@erase "$(INTDIR)\ssl.obj"
	-@erase "$(INTDIR)\token.obj"
	-@erase "$(INTDIR)\userutils.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\GG.exp"
	-@erase "$(OUTDIR)\GG.lib"
	-@erase "$(OUTDIR)\GG.map"
	-@erase "..\..\bin\release\plugins\GG.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O1 /I "../../include" /I "libgadu" /I "libgadu/win32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GG_EXPORTS" /FAcs /Fa"$(INTDIR)\\" /Fp"$(INTDIR)\Gadu-Gadu.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\resource.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Gadu-Gadu.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib version.lib /nologo /base:"0x32500000" /dll /incremental:no /pdb:"$(OUTDIR)\GG.pdb" /map:"$(INTDIR)\GG.map" /machine:I386 /out:"../../bin/release/plugins/GG.dll" /implib:"$(OUTDIR)\GG.lib" /ALIGN:4096 /ignore:4108 
LINK32_OBJS= \
	"$(INTDIR)\common.obj" \
	"$(INTDIR)\dcc.obj" \
	"$(INTDIR)\events.obj" \
	"$(INTDIR)\http.obj" \
	"$(INTDIR)\libgadu.obj" \
	"$(INTDIR)\pubdir.obj" \
	"$(INTDIR)\pubdir50.obj" \
	"$(INTDIR)\sha1.obj" \
	"$(INTDIR)\core.obj" \
	"$(INTDIR)\dialogs.obj" \
	"$(INTDIR)\dynstuff.obj" \
	"$(INTDIR)\filetransfer.obj" \
	"$(INTDIR)\gg.obj" \
	"$(INTDIR)\groupchat.obj" \
	"$(INTDIR)\image.obj" \
	"$(INTDIR)\import.obj" \
	"$(INTDIR)\keepalive.obj" \
	"$(INTDIR)\ownerinfo.obj" \
	"$(INTDIR)\pthread.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\ssl.obj" \
	"$(INTDIR)\token.obj" \
	"$(INTDIR)\userutils.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\release\plugins\GG.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "GG - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\bin\debug\plugins\GG.dll"


CLEAN :
	-@erase "$(INTDIR)\common.obj"
	-@erase "$(INTDIR)\core.obj"
	-@erase "$(INTDIR)\dcc.obj"
	-@erase "$(INTDIR)\dialogs.obj"
	-@erase "$(INTDIR)\dynstuff.obj"
	-@erase "$(INTDIR)\events.obj"
	-@erase "$(INTDIR)\filetransfer.obj"
	-@erase "$(INTDIR)\gg.obj"
	-@erase "$(INTDIR)\groupchat.obj"
	-@erase "$(INTDIR)\http.obj"
	-@erase "$(INTDIR)\image.obj"
	-@erase "$(INTDIR)\import.obj"
	-@erase "$(INTDIR)\keepalive.obj"
	-@erase "$(INTDIR)\libgadu.obj"
	-@erase "$(INTDIR)\ownerinfo.obj"
	-@erase "$(INTDIR)\pthread.obj"
	-@erase "$(INTDIR)\pubdir.obj"
	-@erase "$(INTDIR)\pubdir50.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\sha1.obj"
	-@erase "$(INTDIR)\ssl.obj"
	-@erase "$(INTDIR)\token.obj"
	-@erase "$(INTDIR)\userutils.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\GG.exp"
	-@erase "$(OUTDIR)\GG.lib"
	-@erase "$(OUTDIR)\GG.map"
	-@erase "..\..\bin\debug\plugins\GG.dll"
	-@erase "..\..\bin\debug\plugins\GG.ilk"
	-@erase "..\..\bin\debug\plugins\GG.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /I "libgadu" /I "libgadu/win32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GG_EXPORTS" /FAcs /Fa"$(INTDIR)\\" /Fp"$(INTDIR)\Gadu-Gadu.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\resource.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Gadu-Gadu.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib version.lib /nologo /base:"0x32500000" /dll /incremental:yes /pdb:"../../bin/debug/plugins/GG.pdb" /map:"$(INTDIR)\GG.map" /debug /machine:I386 /out:"../../bin/debug/plugins/GG.dll" /implib:"$(OUTDIR)\GG.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\common.obj" \
	"$(INTDIR)\dcc.obj" \
	"$(INTDIR)\events.obj" \
	"$(INTDIR)\http.obj" \
	"$(INTDIR)\libgadu.obj" \
	"$(INTDIR)\pubdir.obj" \
	"$(INTDIR)\pubdir50.obj" \
	"$(INTDIR)\sha1.obj" \
	"$(INTDIR)\core.obj" \
	"$(INTDIR)\dialogs.obj" \
	"$(INTDIR)\dynstuff.obj" \
	"$(INTDIR)\filetransfer.obj" \
	"$(INTDIR)\gg.obj" \
	"$(INTDIR)\groupchat.obj" \
	"$(INTDIR)\image.obj" \
	"$(INTDIR)\import.obj" \
	"$(INTDIR)\keepalive.obj" \
	"$(INTDIR)\ownerinfo.obj" \
	"$(INTDIR)\pthread.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\ssl.obj" \
	"$(INTDIR)\token.obj" \
	"$(INTDIR)\userutils.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\debug\plugins\GG.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("Gadu-Gadu.dep")
!INCLUDE "Gadu-Gadu.dep"
!ELSE 
!MESSAGE Warning: cannot find "Gadu-Gadu.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "GG - Win32 Release" || "$(CFG)" == "GG - Win32 Debug"
SOURCE=.\libgadu\common.c

"$(INTDIR)\common.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libgadu\dcc.c

"$(INTDIR)\dcc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libgadu\events.c

"$(INTDIR)\events.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libgadu\http.c

"$(INTDIR)\http.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libgadu\libgadu.c

"$(INTDIR)\libgadu.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libgadu\pubdir.c

"$(INTDIR)\pubdir.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libgadu\pubdir50.c

"$(INTDIR)\pubdir50.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libgadu\sha1.c

"$(INTDIR)\sha1.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\core.c

"$(INTDIR)\core.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dialogs.c

"$(INTDIR)\dialogs.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dynstuff.c

"$(INTDIR)\dynstuff.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\filetransfer.c

"$(INTDIR)\filetransfer.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\gg.c

"$(INTDIR)\gg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\groupchat.c

"$(INTDIR)\groupchat.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\image.cpp

"$(INTDIR)\image.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\import.c

"$(INTDIR)\import.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\keepalive.c

"$(INTDIR)\keepalive.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ownerinfo.c

"$(INTDIR)\ownerinfo.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\pthread.c

"$(INTDIR)\pthread.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\services.c

"$(INTDIR)\services.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ssl.c

"$(INTDIR)\ssl.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\token.cpp

"$(INTDIR)\token.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\userutils.c

"$(INTDIR)\userutils.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\resource.rc

"$(INTDIR)\resource.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

