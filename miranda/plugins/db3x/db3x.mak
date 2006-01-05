# Microsoft Developer Studio Generated NMAKE File, Based on db3x.dsp
!IF "$(CFG)" == ""
CFG=db3x - Win32 Debug
!MESSAGE No configuration specified. Defaulting to db3x - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "db3x - Win32 Release" && "$(CFG)" != "db3x - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "db3x.mak" CFG="db3x - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "db3x - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "db3x - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "db3x - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\release\plugins\dbx_3x.dll"


CLEAN :
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\database.obj"
	-@erase "$(INTDIR)\db3x.pch"
	-@erase "$(INTDIR)\dbcache.obj"
	-@erase "$(INTDIR)\dbcontacts.obj"
	-@erase "$(INTDIR)\dbevents.obj"
	-@erase "$(INTDIR)\dbheaders.obj"
	-@erase "$(INTDIR)\dbini.obj"
	-@erase "$(INTDIR)\dbmodulechain.obj"
	-@erase "$(INTDIR)\dbsettings.obj"
	-@erase "$(INTDIR)\encrypt.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\utf.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\dbx_3x.exp"
	-@erase "$(OUTDIR)\dbx_3x.lib"
	-@erase "$(OUTDIR)\dbx_3x.map"
	-@erase "$(OUTDIR)\dbx_3x.pdb"
	-@erase "..\..\bin\release\plugins\dbx_3x.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\db3x.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x5130000" /dll /incremental:no /pdb:"$(OUTDIR)\dbx_3x.pdb" /map:"$(INTDIR)\dbx_3x.map" /debug /machine:I386 /out:"../../bin/release/plugins/dbx_3x.dll" /implib:"$(OUTDIR)\dbx_3x.lib" /IGNORE:4089 
LINK32_OBJS= \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\database.obj" \
	"$(INTDIR)\dbcache.obj" \
	"$(INTDIR)\dbcontacts.obj" \
	"$(INTDIR)\dbevents.obj" \
	"$(INTDIR)\dbheaders.obj" \
	"$(INTDIR)\dbini.obj" \
	"$(INTDIR)\dbmodulechain.obj" \
	"$(INTDIR)\dbsettings.obj" \
	"$(INTDIR)\encrypt.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\utf.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\release\plugins\dbx_3x.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "db3x - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "..\..\bin\debug\plugins\dbx_3x.dll" "$(OUTDIR)\db3x.bsc"


CLEAN :
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\commonheaders.sbr"
	-@erase "$(INTDIR)\database.obj"
	-@erase "$(INTDIR)\database.sbr"
	-@erase "$(INTDIR)\db3x.pch"
	-@erase "$(INTDIR)\dbcache.obj"
	-@erase "$(INTDIR)\dbcache.sbr"
	-@erase "$(INTDIR)\dbcontacts.obj"
	-@erase "$(INTDIR)\dbcontacts.sbr"
	-@erase "$(INTDIR)\dbevents.obj"
	-@erase "$(INTDIR)\dbevents.sbr"
	-@erase "$(INTDIR)\dbheaders.obj"
	-@erase "$(INTDIR)\dbheaders.sbr"
	-@erase "$(INTDIR)\dbini.obj"
	-@erase "$(INTDIR)\dbini.sbr"
	-@erase "$(INTDIR)\dbmodulechain.obj"
	-@erase "$(INTDIR)\dbmodulechain.sbr"
	-@erase "$(INTDIR)\dbsettings.obj"
	-@erase "$(INTDIR)\dbsettings.sbr"
	-@erase "$(INTDIR)\encrypt.obj"
	-@erase "$(INTDIR)\encrypt.sbr"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\init.sbr"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\utf.obj"
	-@erase "$(INTDIR)\utf.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\db3x.bsc"
	-@erase "$(OUTDIR)\dbx_3x.exp"
	-@erase "$(OUTDIR)\dbx_3x.lib"
	-@erase "$(OUTDIR)\dbx_3x.map"
	-@erase "$(OUTDIR)\dbx_3x.pdb"
	-@erase "..\..\bin\debug\plugins\dbx_3x.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\db3x.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\commonheaders.sbr" \
	"$(INTDIR)\database.sbr" \
	"$(INTDIR)\dbcache.sbr" \
	"$(INTDIR)\dbcontacts.sbr" \
	"$(INTDIR)\dbevents.sbr" \
	"$(INTDIR)\dbheaders.sbr" \
	"$(INTDIR)\dbini.sbr" \
	"$(INTDIR)\dbmodulechain.sbr" \
	"$(INTDIR)\dbsettings.sbr" \
	"$(INTDIR)\encrypt.sbr" \
	"$(INTDIR)\init.sbr" \
	"$(INTDIR)\utf.sbr"

"$(OUTDIR)\db3x.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\dbx_3x.pdb" /map:"$(INTDIR)\dbx_3x.map" /debug /machine:I386 /out:"../../bin/debug/plugins/dbx_3x.dll" /implib:"$(OUTDIR)\dbx_3x.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\database.obj" \
	"$(INTDIR)\dbcache.obj" \
	"$(INTDIR)\dbcontacts.obj" \
	"$(INTDIR)\dbevents.obj" \
	"$(INTDIR)\dbheaders.obj" \
	"$(INTDIR)\dbini.obj" \
	"$(INTDIR)\dbmodulechain.obj" \
	"$(INTDIR)\dbsettings.obj" \
	"$(INTDIR)\encrypt.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\utf.obj" \
	"$(INTDIR)\resource.res"

"..\..\bin\debug\plugins\dbx_3x.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("db3x.dep")
!INCLUDE "db3x.dep"
!ELSE 
!MESSAGE Warning: cannot find "db3x.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "db3x - Win32 Release" || "$(CFG)" == "db3x - Win32 Debug"
SOURCE=.\commonheaders.c

!IF  "$(CFG)" == "db3x - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fp"$(INTDIR)\db3x.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\db3x.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "db3x - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\db3x.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\commonheaders.sbr"	"$(INTDIR)\db3x.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\database.c

!IF  "$(CFG)" == "db3x - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\database.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "db3x - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\database.obj"	"$(INTDIR)\database.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\dbcache.c

!IF  "$(CFG)" == "db3x - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dbcache.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "db3x - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\dbcache.obj"	"$(INTDIR)\dbcache.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\dbcontacts.c

!IF  "$(CFG)" == "db3x - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dbcontacts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "db3x - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\dbcontacts.obj"	"$(INTDIR)\dbcontacts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\dbevents.c

!IF  "$(CFG)" == "db3x - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dbevents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "db3x - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\dbevents.obj"	"$(INTDIR)\dbevents.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\dbheaders.c

!IF  "$(CFG)" == "db3x - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dbheaders.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "db3x - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\dbheaders.obj"	"$(INTDIR)\dbheaders.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\dbini.c

!IF  "$(CFG)" == "db3x - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dbini.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "db3x - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\dbini.obj"	"$(INTDIR)\dbini.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\dbmodulechain.c

!IF  "$(CFG)" == "db3x - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dbmodulechain.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "db3x - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\dbmodulechain.obj"	"$(INTDIR)\dbmodulechain.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\dbsettings.c

!IF  "$(CFG)" == "db3x - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dbsettings.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "db3x - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\dbsettings.obj"	"$(INTDIR)\dbsettings.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\encrypt.c

!IF  "$(CFG)" == "db3x - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\encrypt.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "db3x - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\encrypt.obj"	"$(INTDIR)\encrypt.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\init.c

!IF  "$(CFG)" == "db3x - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\init.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "db3x - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DB3X_EXPORTS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\db3x.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\init.obj"	"$(INTDIR)\init.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\utf.c

!IF  "$(CFG)" == "db3x - Win32 Release"


"$(INTDIR)\utf.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"


!ELSEIF  "$(CFG)" == "db3x - Win32 Debug"


"$(INTDIR)\utf.obj"	"$(INTDIR)\utf.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\db3x.pch"


!ENDIF 

SOURCE=.\resource.rc

"$(INTDIR)\resource.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

