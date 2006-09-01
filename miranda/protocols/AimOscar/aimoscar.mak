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

ALL : "..\..\bin\release\plugins\Aim.dll"


CLEAN :
	-@erase "$(INTDIR)\aim.obj"
	-@erase "$(INTDIR)\aim.res"
	-@erase "$(INTDIR)\client.obj"
	-@erase "$(INTDIR)\connection.obj"
	-@erase "$(INTDIR)\conv.obj"
	-@erase "$(INTDIR)\direct_connect.obj"
	-@erase "$(INTDIR)\error.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\flap.obj"
	-@erase "$(INTDIR)\links.obj"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\packets.obj"
	-@erase "$(INTDIR)\popup.obj"
	-@erase "$(INTDIR)\proxy.obj"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\snac.obj"
	-@erase "$(INTDIR)\strl.obj"
	-@erase "$(INTDIR)\theme.obj"
	-@erase "$(INTDIR)\thread.obj"
	-@erase "$(INTDIR)\tlv.obj"
	-@erase "$(INTDIR)\utility.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\windows.obj"
	-@erase "$(OUTDIR)\Aim.exp"
	-@erase "$(OUTDIR)\Aim.lib"
	-@erase "$(OUTDIR)\Aim.map"
	-@erase "$(OUTDIR)\Aim.pdb"
	-@erase "..\..\bin\release\plugins\Aim.dll"

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
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\Aim.pdb" /map:"$(INTDIR)\Aim.map" /debug /machine:I386 /out:"../../bin/release/plugins/Aim.dll" /implib:"$(OUTDIR)\Aim.lib" /ALIGN:4096 /ignore:4108 
LINK32_OBJS= \
	"$(INTDIR)\aim.obj" \
	"$(INTDIR)\client.obj" \
	"$(INTDIR)\connection.obj" \
	"$(INTDIR)\conv.obj" \
	"$(INTDIR)\direct_connect.obj" \
	"$(INTDIR)\error.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\flap.obj" \
	"$(INTDIR)\links.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\packets.obj" \
	"$(INTDIR)\popup.obj" \
	"$(INTDIR)\proxy.obj" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\snac.obj" \
	"$(INTDIR)\strl.obj" \
	"$(INTDIR)\theme.obj" \
	"$(INTDIR)\thread.obj" \
	"$(INTDIR)\tlv.obj" \
	"$(INTDIR)\utility.obj" \
	"$(INTDIR)\windows.obj" \
	"$(INTDIR)\aim.res"

"..\..\bin\release\plugins\Aim.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "aim - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "..\..\bin\debug\plugins\Aim.dll" "$(OUTDIR)\aimoscar.bsc"


CLEAN :
	-@erase "$(INTDIR)\aim.obj"
	-@erase "$(INTDIR)\aim.res"
	-@erase "$(INTDIR)\aim.sbr"
	-@erase "$(INTDIR)\client.obj"
	-@erase "$(INTDIR)\client.sbr"
	-@erase "$(INTDIR)\connection.obj"
	-@erase "$(INTDIR)\connection.sbr"
	-@erase "$(INTDIR)\conv.obj"
	-@erase "$(INTDIR)\conv.sbr"
	-@erase "$(INTDIR)\direct_connect.obj"
	-@erase "$(INTDIR)\direct_connect.sbr"
	-@erase "$(INTDIR)\error.obj"
	-@erase "$(INTDIR)\error.sbr"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\file.sbr"
	-@erase "$(INTDIR)\flap.obj"
	-@erase "$(INTDIR)\flap.sbr"
	-@erase "$(INTDIR)\links.obj"
	-@erase "$(INTDIR)\links.sbr"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\md5.sbr"
	-@erase "$(INTDIR)\packets.obj"
	-@erase "$(INTDIR)\packets.sbr"
	-@erase "$(INTDIR)\popup.obj"
	-@erase "$(INTDIR)\popup.sbr"
	-@erase "$(INTDIR)\proxy.obj"
	-@erase "$(INTDIR)\proxy.sbr"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\server.sbr"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\services.sbr"
	-@erase "$(INTDIR)\snac.obj"
	-@erase "$(INTDIR)\snac.sbr"
	-@erase "$(INTDIR)\strl.obj"
	-@erase "$(INTDIR)\strl.sbr"
	-@erase "$(INTDIR)\theme.obj"
	-@erase "$(INTDIR)\theme.sbr"
	-@erase "$(INTDIR)\thread.obj"
	-@erase "$(INTDIR)\thread.sbr"
	-@erase "$(INTDIR)\tlv.obj"
	-@erase "$(INTDIR)\tlv.sbr"
	-@erase "$(INTDIR)\utility.obj"
	-@erase "$(INTDIR)\utility.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\windows.obj"
	-@erase "$(INTDIR)\windows.sbr"
	-@erase "$(OUTDIR)\Aim.exp"
	-@erase "$(OUTDIR)\Aim.lib"
	-@erase "$(OUTDIR)\Aim.pdb"
	-@erase "$(OUTDIR)\aimoscar.bsc"
	-@erase "..\..\bin\debug\plugins\Aim.dll"
	-@erase "..\..\bin\debug\plugins\Aim.ilk"

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
	"$(INTDIR)\client.sbr" \
	"$(INTDIR)\connection.sbr" \
	"$(INTDIR)\conv.sbr" \
	"$(INTDIR)\direct_connect.sbr" \
	"$(INTDIR)\error.sbr" \
	"$(INTDIR)\file.sbr" \
	"$(INTDIR)\flap.sbr" \
	"$(INTDIR)\links.sbr" \
	"$(INTDIR)\md5.sbr" \
	"$(INTDIR)\packets.sbr" \
	"$(INTDIR)\popup.sbr" \
	"$(INTDIR)\proxy.sbr" \
	"$(INTDIR)\server.sbr" \
	"$(INTDIR)\services.sbr" \
	"$(INTDIR)\snac.sbr" \
	"$(INTDIR)\strl.sbr" \
	"$(INTDIR)\theme.sbr" \
	"$(INTDIR)\thread.sbr" \
	"$(INTDIR)\tlv.sbr" \
	"$(INTDIR)\utility.sbr" \
	"$(INTDIR)\windows.sbr"

"$(OUTDIR)\aimoscar.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\Aim.pdb" /debug /machine:I386 /out:"../../bin/debug/plugins/Aim.dll" /implib:"$(OUTDIR)\Aim.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\aim.obj" \
	"$(INTDIR)\client.obj" \
	"$(INTDIR)\connection.obj" \
	"$(INTDIR)\conv.obj" \
	"$(INTDIR)\direct_connect.obj" \
	"$(INTDIR)\error.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\flap.obj" \
	"$(INTDIR)\links.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\packets.obj" \
	"$(INTDIR)\popup.obj" \
	"$(INTDIR)\proxy.obj" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\snac.obj" \
	"$(INTDIR)\strl.obj" \
	"$(INTDIR)\theme.obj" \
	"$(INTDIR)\thread.obj" \
	"$(INTDIR)\tlv.obj" \
	"$(INTDIR)\utility.obj" \
	"$(INTDIR)\windows.obj" \
	"$(INTDIR)\aim.res"

"..\..\bin\debug\plugins\Aim.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

SOURCE=.\client.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\client.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\client.obj"	"$(INTDIR)\client.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\connection.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\connection.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\connection.obj"	"$(INTDIR)\connection.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\conv.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\conv.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\conv.obj"	"$(INTDIR)\conv.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\direct_connect.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\direct_connect.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\direct_connect.obj"	"$(INTDIR)\direct_connect.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\error.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\error.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\error.obj"	"$(INTDIR)\error.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\file.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\file.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\file.obj"	"$(INTDIR)\file.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\flap.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\flap.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\flap.obj"	"$(INTDIR)\flap.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\links.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\links.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\links.obj"	"$(INTDIR)\links.sbr" : $(SOURCE) "$(INTDIR)"


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

SOURCE=.\popup.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\popup.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\popup.obj"	"$(INTDIR)\popup.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\proxy.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\proxy.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\proxy.obj"	"$(INTDIR)\proxy.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\server.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\server.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\server.obj"	"$(INTDIR)\server.sbr" : $(SOURCE) "$(INTDIR)"


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

SOURCE=.\theme.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\theme.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\theme.obj"	"$(INTDIR)\theme.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\thread.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\thread.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\thread.obj"	"$(INTDIR)\thread.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\tlv.cpp

!IF  "$(CFG)" == "aim - Win32 Release"


"$(INTDIR)\tlv.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "aim - Win32 Debug"


"$(INTDIR)\tlv.obj"	"$(INTDIR)\tlv.sbr" : $(SOURCE) "$(INTDIR)"


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

