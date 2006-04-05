# Microsoft Developer Studio Generated NMAKE File, Based on aimoscar.dsp
!IF "$(CFG)" == ""
CFG=aim - Win32 Debug
!MESSAGE No configuration specified. Defaulting to aim - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "aim - Win32 Release" && "$(CFG)" != "aim - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aimoscar.mak" CFG="aim - Win32 Debug"
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

!IF  "$(CFG)" == "aim - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\release\plugins\AimOSCAR.dll"


CLEAN :
	-@erase "$(INTDIR)\aim.obj"
	-@erase "$(INTDIR)\aim.res"
	-@erase "$(INTDIR)\commands.obj"
	-@erase "$(INTDIR)\connection.obj"
	-@erase "$(INTDIR)\direct_connect.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\packets.obj"
	-@erase "$(INTDIR)\proxy.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\snac.obj"
	-@erase "$(INTDIR)\strl.obj"
	-@erase "$(INTDIR)\thread.obj"
	-@erase "$(INTDIR)\utility.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\windows.obj"
	-@erase "$(OUTDIR)\AimOSCAR.exp"
	-@erase "$(OUTDIR)\AimOSCAR.lib"
	-@erase "$(OUTDIR)\AimOSCAR.map"
	-@erase "$(OUTDIR)\AimOSCAR.pdb"
	-@erase "..\..\bin\release\plugins\AimOSCAR.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AIM_EXPORTS" /Fp"$(INTDIR)\aimoscar.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\aim.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\aimoscar.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\AimOSCAR.pdb" /map:"$(INTDIR)\AimOSCAR.map" /debug /machine:I386 /out:"../../bin/release/plugins/AimOSCAR.dll" /implib:"$(OUTDIR)\AimOSCAR.lib" /ALIGN:4096 /ignore:4108 
LINK32_OBJS= \
	"$(INTDIR)\aim.obj" \
	"$(INTDIR)\commands.obj" \
	"$(INTDIR)\connection.obj" \
	"$(INTDIR)\direct_connect.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\packets.obj" \
	"$(INTDIR)\proxy.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\snac.obj" \
	"$(INTDIR)\thread.obj" \
	"$(INTDIR)\utility.obj" \
	"$(INTDIR)\strl.obj" \
	"$(INTDIR)\windows.obj" \
	"$(INTDIR)\aim.res"

"..\..\bin\release\plugins\AimOSCAR.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "aim - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "..\..\bin\debug\plugins\AimOSCAR.dll" "$(OUTDIR)\aimoscar.bsc"


CLEAN :
	-@erase "$(INTDIR)\aim.obj"
	-@erase "$(INTDIR)\aim.res"
	-@erase "$(INTDIR)\aim.sbr"
	-@erase "$(INTDIR)\commands.obj"
	-@erase "$(INTDIR)\commands.sbr"
	-@erase "$(INTDIR)\connection.obj"
	-@erase "$(INTDIR)\connection.sbr"
	-@erase "$(INTDIR)\direct_connect.obj"
	-@erase "$(INTDIR)\direct_connect.sbr"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\file.sbr"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\md5.sbr"
	-@erase "$(INTDIR)\packets.obj"
	-@erase "$(INTDIR)\packets.sbr"
	-@erase "$(INTDIR)\proxy.obj"
	-@erase "$(INTDIR)\proxy.sbr"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\services.sbr"
	-@erase "$(INTDIR)\snac.obj"
	-@erase "$(INTDIR)\snac.sbr"
	-@erase "$(INTDIR)\strl.obj"
	-@erase "$(INTDIR)\strl.sbr"
	-@erase "$(INTDIR)\thread.obj"
	-@erase "$(INTDIR)\thread.sbr"
	-@erase "$(INTDIR)\utility.obj"
	-@erase "$(INTDIR)\utility.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\windows.obj"
	-@erase "$(INTDIR)\windows.sbr"
	-@erase "$(OUTDIR)\aimoscar.bsc"
	-@erase "$(OUTDIR)\AimOSCAR.exp"
	-@erase "$(OUTDIR)\AimOSCAR.lib"
	-@erase "$(OUTDIR)\AimOSCAR.pdb"
	-@erase "..\..\bin\debug\plugins\AimOSCAR.dll"
	-@erase "..\..\bin\debug\plugins\AimOSCAR.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W2 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AIM_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\aimoscar.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\aim.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\aimoscar.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\aim.sbr" \
	"$(INTDIR)\commands.sbr" \
	"$(INTDIR)\connection.sbr" \
	"$(INTDIR)\direct_connect.sbr" \
	"$(INTDIR)\file.sbr" \
	"$(INTDIR)\md5.sbr" \
	"$(INTDIR)\packets.sbr" \
	"$(INTDIR)\proxy.sbr" \
	"$(INTDIR)\services.sbr" \
	"$(INTDIR)\snac.sbr" \
	"$(INTDIR)\thread.sbr" \
	"$(INTDIR)\utility.sbr" \
	"$(INTDIR)\strl.sbr" \
	"$(INTDIR)\windows.sbr"

"$(OUTDIR)\aimoscar.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\AimOSCAR.pdb" /debug /machine:I386 /out:"../../bin/debug/plugins/AimOSCAR.dll" /implib:"$(OUTDIR)\AimOSCAR.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\aim.obj" \
	"$(INTDIR)\commands.obj" \
	"$(INTDIR)\connection.obj" \
	"$(INTDIR)\direct_connect.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\packets.obj" \
	"$(INTDIR)\proxy.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\snac.obj" \
	"$(INTDIR)\thread.obj" \
	"$(INTDIR)\utility.obj" \
	"$(INTDIR)\strl.obj" \
	"$(INTDIR)\windows.obj" \
	"$(INTDIR)\aim.res"

"..\..\bin\debug\plugins\AimOSCAR.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("aimoscar.dep")
!INCLUDE "aimoscar.dep"
!ELSE 
!MESSAGE Warning: cannot find "aimoscar.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "aim - Win32 Release" || "$(CFG)" == "aim - Win32 Debug"
SOURCE=.\aim.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\aim.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\aim.obj"	"$(INTDIR)\aim.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\commands.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\commands.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\commands.obj"	"$(INTDIR)\commands.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\connection.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\connection.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\connection.obj"	"$(INTDIR)\connection.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\direct_connect.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\direct_connect.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\direct_connect.obj"	"$(INTDIR)\direct_connect.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\file.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\file.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\file.obj"	"$(INTDIR)\file.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\md5.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\md5.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\md5.obj"	"$(INTDIR)\md5.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\packets.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\packets.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\packets.obj"	"$(INTDIR)\packets.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\proxy.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\proxy.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\proxy.obj"	"$(INTDIR)\proxy.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\services.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\services.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\services.obj"	"$(INTDIR)\services.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\snac.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\snac.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\snac.obj"	"$(INTDIR)\snac.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\strl.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\strl.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\strl.obj"	"$(INTDIR)\strl.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\thread.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\thread.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\thread.obj"	"$(INTDIR)\thread.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\utility.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\utility.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\utility.obj"	"$(INTDIR)\utility.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\windows.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\windows.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\windows.obj"	"$(INTDIR)\windows.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\aim.rc

"$(INTDIR)\aim.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

