# Microsoft Developer Studio Generated NMAKE File, Based on aim.dsp
!IF "$(CFG)" == ""
CFG=aim - Win32 Debug
!MESSAGE No configuration specified. Defaulting to aim - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "aim - Win32 Release" && "$(CFG)" != "aim - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aim.mak" CFG="aim - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aim - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "aim - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "aim - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\release\plugins\AIM.dll"


CLEAN :
	-@erase "$(INTDIR)\aim.obj"
	-@erase "$(INTDIR)\buddies.obj"
	-@erase "$(INTDIR)\evilmode.obj"
	-@erase "$(INTDIR)\filerecv.obj"
	-@erase "$(INTDIR)\firstrun.obj"
	-@erase "$(INTDIR)\groupchat.obj"
	-@erase "$(INTDIR)\groups.obj"
	-@erase "$(INTDIR)\idle.obj"
	-@erase "$(INTDIR)\im.obj"
	-@erase "$(INTDIR)\keepalive.obj"
	-@erase "$(INTDIR)\links.obj"
	-@erase "$(INTDIR)\list.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\password.obj"
	-@erase "$(INTDIR)\pthread.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\search.obj"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\toc.obj"
	-@erase "$(INTDIR)\userinfo.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\AIM.exp"
	-@erase "$(OUTDIR)\AIM.lib"
	-@erase "$(OUTDIR)\AIM.map"
	-@erase "$(OUTDIR)\AIM.pdb"
	-@erase "..\..\bin\release\plugins\AIM.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AIM_EXPORTS" /Fp"$(INTDIR)\aim.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\resource.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\aim.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\AIM.pdb" /map:"$(INTDIR)\AIM.map" /debug /machine:I386 /out:"../../bin/release/plugins/AIM.dll" /implib:"$(OUTDIR)\AIM.lib" /ALIGN:4096 /ignore:4108 
LINK32_OBJS= \
	"$(INTDIR)\aim.obj" \
	"$(INTDIR)\buddies.obj" \
	"$(INTDIR)\evilmode.obj" \
	"$(INTDIR)\filerecv.obj" \
	"$(INTDIR)\firstrun.obj" \
	"$(INTDIR)\groupchat.obj" \
	"$(INTDIR)\groups.obj" \
	"$(INTDIR)\idle.obj" \
	"$(INTDIR)\im.obj" \
	"$(INTDIR)\keepalive.obj" \
	"$(INTDIR)\links.obj" \
	"$(INTDIR)\list.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\password.obj" \
	"$(INTDIR)\pthread.obj" \
	"$(INTDIR)\search.obj" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\toc.obj" \
	"$(INTDIR)\userinfo.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\release\plugins\AIM.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "aim - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\bin\debug\plugins\AIM.dll"


CLEAN :
	-@erase "$(INTDIR)\aim.obj"
	-@erase "$(INTDIR)\buddies.obj"
	-@erase "$(INTDIR)\evilmode.obj"
	-@erase "$(INTDIR)\filerecv.obj"
	-@erase "$(INTDIR)\firstrun.obj"
	-@erase "$(INTDIR)\groupchat.obj"
	-@erase "$(INTDIR)\groups.obj"
	-@erase "$(INTDIR)\idle.obj"
	-@erase "$(INTDIR)\im.obj"
	-@erase "$(INTDIR)\keepalive.obj"
	-@erase "$(INTDIR)\links.obj"
	-@erase "$(INTDIR)\list.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\password.obj"
	-@erase "$(INTDIR)\pthread.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\search.obj"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\toc.obj"
	-@erase "$(INTDIR)\userinfo.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\AIM.exp"
	-@erase "$(OUTDIR)\AIM.lib"
	-@erase "$(OUTDIR)\AIM.pdb"
	-@erase "..\..\bin\debug\plugins\AIM.dll"
	-@erase "..\..\bin\debug\plugins\AIM.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AIM_EXPORTS" /Fp"$(INTDIR)\aim.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\resource.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\aim.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\AIM.pdb" /debug /machine:I386 /out:"../../bin/debug/plugins/AIM.dll" /implib:"$(OUTDIR)\AIM.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\aim.obj" \
	"$(INTDIR)\buddies.obj" \
	"$(INTDIR)\evilmode.obj" \
	"$(INTDIR)\filerecv.obj" \
	"$(INTDIR)\firstrun.obj" \
	"$(INTDIR)\groupchat.obj" \
	"$(INTDIR)\groups.obj" \
	"$(INTDIR)\idle.obj" \
	"$(INTDIR)\im.obj" \
	"$(INTDIR)\keepalive.obj" \
	"$(INTDIR)\links.obj" \
	"$(INTDIR)\list.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\password.obj" \
	"$(INTDIR)\pthread.obj" \
	"$(INTDIR)\search.obj" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\toc.obj" \
	"$(INTDIR)\userinfo.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\debug\plugins\AIM.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("aim.dep")
!INCLUDE "aim.dep"
!ELSE 
!MESSAGE Warning: cannot find "aim.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "aim - Win32 Release" || "$(CFG)" == "aim - Win32 Debug"
SOURCE=.\aim.c

"$(INTDIR)\aim.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\buddies.c

"$(INTDIR)\buddies.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\evilmode.c

"$(INTDIR)\evilmode.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\filerecv.c

"$(INTDIR)\filerecv.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\firstrun.c

"$(INTDIR)\firstrun.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\groupchat.c

"$(INTDIR)\groupchat.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\groups.c

"$(INTDIR)\groups.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\idle.c

"$(INTDIR)\idle.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\im.c

"$(INTDIR)\im.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\keepalive.c

"$(INTDIR)\keepalive.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\links.c

"$(INTDIR)\links.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\list.c

"$(INTDIR)\list.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\log.c

"$(INTDIR)\log.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\options.c

"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\password.c

"$(INTDIR)\password.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\pthread.c

"$(INTDIR)\pthread.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\resource.rc

"$(INTDIR)\resource.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\search.c

"$(INTDIR)\search.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\server.c

"$(INTDIR)\server.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\services.c

"$(INTDIR)\services.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\toc.c

"$(INTDIR)\toc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\userinfo.c

"$(INTDIR)\userinfo.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\utils.c

"$(INTDIR)\utils.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\version.c

"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

