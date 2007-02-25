# Microsoft Developer Studio Generated NMAKE File, Based on png2dib.dsp
!IF "$(CFG)" == ""
CFG=png2dib - Win32 Debug
!MESSAGE No configuration specified. Defaulting to png2dib - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "png2dib - Win32 Release" && "$(CFG)" != "png2dib - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "png2dib.mak" CFG="png2dib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "png2dib - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "png2dib - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "png2dib - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

!IF "$(RECURSE)" == "0" 

ALL : "..\..\bin\Release\Plugins\png2dib.dll"

!ELSE 

ALL : "zlib - Win32 Release" "..\..\bin\Release\Plugins\png2dib.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"zlib - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\png.obj"
	-@erase "$(INTDIR)\png2dib.obj"
	-@erase "$(INTDIR)\pnggccrd.obj"
	-@erase "$(INTDIR)\pngget.obj"
	-@erase "$(INTDIR)\pngmem.obj"
	-@erase "$(INTDIR)\pngpread.obj"
	-@erase "$(INTDIR)\pngread.obj"
	-@erase "$(INTDIR)\pngrio.obj"
	-@erase "$(INTDIR)\pngrtran.obj"
	-@erase "$(INTDIR)\pngrutil.obj"
	-@erase "$(INTDIR)\pngset.obj"
	-@erase "$(INTDIR)\pngtrans.obj"
	-@erase "$(INTDIR)\pngvcrd.obj"
	-@erase "$(INTDIR)\pngwio.obj"
	-@erase "$(INTDIR)\pngwrite.obj"
	-@erase "$(INTDIR)\pngwtran.obj"
	-@erase "$(INTDIR)\pngwutil.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.res"
	-@erase "$(OUTDIR)\png2dib.exp"
	-@erase "$(OUTDIR)\png2dib.lib"
	-@erase "$(OUTDIR)\png2dib.map"
	-@erase "$(OUTDIR)\png2dib.pdb"
	-@erase "..\..\bin\Release\Plugins\png2dib.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NO_GZIP" /D "PNG_NO_STDIO" /D "PNG_NO_CONSOLE_IO" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x419 /fo"$(INTDIR)\version.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\png2dib.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\png2dib.pdb" /map:"$(INTDIR)\png2dib.map" /debug /machine:I386 /def:".\png2dib.def" /out:"../../bin/Release/Plugins/png2dib.dll" /implib:"$(OUTDIR)\png2dib.lib" /filealign:512 
DEF_FILE= \
	".\png2dib.def"
LINK32_OBJS= \
	"$(INTDIR)\png.obj" \
	"$(INTDIR)\pnggccrd.obj" \
	"$(INTDIR)\pngget.obj" \
	"$(INTDIR)\pngmem.obj" \
	"$(INTDIR)\pngpread.obj" \
	"$(INTDIR)\pngread.obj" \
	"$(INTDIR)\pngrio.obj" \
	"$(INTDIR)\pngrtran.obj" \
	"$(INTDIR)\pngrutil.obj" \
	"$(INTDIR)\pngset.obj" \
	"$(INTDIR)\pngtrans.obj" \
	"$(INTDIR)\pngvcrd.obj" \
	"$(INTDIR)\pngwio.obj" \
	"$(INTDIR)\pngwrite.obj" \
	"$(INTDIR)\pngwtran.obj" \
	"$(INTDIR)\pngwutil.obj" \
	"$(INTDIR)\png2dib.obj" \
	"$(INTDIR)\version.res" \
	"..\zlib\Release\zlib.lib"

"..\..\bin\Release\Plugins\png2dib.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "..\..\bin\Debug\plugins\png2dib.dll" "$(OUTDIR)\png2dib.bsc"

!ELSE 

ALL : "zlib - Win32 Debug" "..\..\bin\Debug\plugins\png2dib.dll" "$(OUTDIR)\png2dib.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"zlib - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\png.obj"
	-@erase "$(INTDIR)\png.sbr"
	-@erase "$(INTDIR)\png2dib.obj"
	-@erase "$(INTDIR)\png2dib.sbr"
	-@erase "$(INTDIR)\pnggccrd.obj"
	-@erase "$(INTDIR)\pnggccrd.sbr"
	-@erase "$(INTDIR)\pngget.obj"
	-@erase "$(INTDIR)\pngget.sbr"
	-@erase "$(INTDIR)\pngmem.obj"
	-@erase "$(INTDIR)\pngmem.sbr"
	-@erase "$(INTDIR)\pngpread.obj"
	-@erase "$(INTDIR)\pngpread.sbr"
	-@erase "$(INTDIR)\pngread.obj"
	-@erase "$(INTDIR)\pngread.sbr"
	-@erase "$(INTDIR)\pngrio.obj"
	-@erase "$(INTDIR)\pngrio.sbr"
	-@erase "$(INTDIR)\pngrtran.obj"
	-@erase "$(INTDIR)\pngrtran.sbr"
	-@erase "$(INTDIR)\pngrutil.obj"
	-@erase "$(INTDIR)\pngrutil.sbr"
	-@erase "$(INTDIR)\pngset.obj"
	-@erase "$(INTDIR)\pngset.sbr"
	-@erase "$(INTDIR)\pngtrans.obj"
	-@erase "$(INTDIR)\pngtrans.sbr"
	-@erase "$(INTDIR)\pngvcrd.obj"
	-@erase "$(INTDIR)\pngvcrd.sbr"
	-@erase "$(INTDIR)\pngwio.obj"
	-@erase "$(INTDIR)\pngwio.sbr"
	-@erase "$(INTDIR)\pngwrite.obj"
	-@erase "$(INTDIR)\pngwrite.sbr"
	-@erase "$(INTDIR)\pngwtran.obj"
	-@erase "$(INTDIR)\pngwtran.sbr"
	-@erase "$(INTDIR)\pngwutil.obj"
	-@erase "$(INTDIR)\pngwutil.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.res"
	-@erase "$(OUTDIR)\png2dib.bsc"
	-@erase "$(OUTDIR)\png2dib.exp"
	-@erase "$(OUTDIR)\png2dib.lib"
	-@erase "$(OUTDIR)\png2dib.pdb"
	-@erase "..\..\bin\Debug\plugins\png2dib.dll"
	-@erase "..\..\bin\Debug\plugins\png2dib.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NO_GZIP" /D "PNG_NO_STDIO" /D "PNG_NO_CONSOLE_IO" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\png2dib.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
RSC_PROJ=/l 0x419 /fo"$(INTDIR)\version.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\png2dib.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\png.sbr" \
	"$(INTDIR)\pnggccrd.sbr" \
	"$(INTDIR)\pngget.sbr" \
	"$(INTDIR)\pngmem.sbr" \
	"$(INTDIR)\pngpread.sbr" \
	"$(INTDIR)\pngread.sbr" \
	"$(INTDIR)\pngrio.sbr" \
	"$(INTDIR)\pngrtran.sbr" \
	"$(INTDIR)\pngrutil.sbr" \
	"$(INTDIR)\pngset.sbr" \
	"$(INTDIR)\pngtrans.sbr" \
	"$(INTDIR)\pngvcrd.sbr" \
	"$(INTDIR)\pngwio.sbr" \
	"$(INTDIR)\pngwrite.sbr" \
	"$(INTDIR)\pngwtran.sbr" \
	"$(INTDIR)\pngwutil.sbr" \
	"$(INTDIR)\png2dib.sbr"

"$(OUTDIR)\png2dib.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\png2dib.pdb" /debug /machine:I386 /def:".\png2dib.def" /out:"../../bin/Debug/plugins/png2dib.dll" /implib:"$(OUTDIR)\png2dib.lib" /pdbtype:sept 
DEF_FILE= \
	".\png2dib.def"
LINK32_OBJS= \
	"$(INTDIR)\png.obj" \
	"$(INTDIR)\pnggccrd.obj" \
	"$(INTDIR)\pngget.obj" \
	"$(INTDIR)\pngmem.obj" \
	"$(INTDIR)\pngpread.obj" \
	"$(INTDIR)\pngread.obj" \
	"$(INTDIR)\pngrio.obj" \
	"$(INTDIR)\pngrtran.obj" \
	"$(INTDIR)\pngrutil.obj" \
	"$(INTDIR)\pngset.obj" \
	"$(INTDIR)\pngtrans.obj" \
	"$(INTDIR)\pngvcrd.obj" \
	"$(INTDIR)\pngwio.obj" \
	"$(INTDIR)\pngwrite.obj" \
	"$(INTDIR)\pngwtran.obj" \
	"$(INTDIR)\pngwutil.obj" \
	"$(INTDIR)\png2dib.obj" \
	"$(INTDIR)\version.res" \
	"..\zlib\Debug\zlib.lib"

"..\..\bin\Debug\plugins\png2dib.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("png2dib.dep")
!INCLUDE "png2dib.dep"
!ELSE 
!MESSAGE Warning: cannot find "png2dib.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "png2dib - Win32 Release" || "$(CFG)" == "png2dib - Win32 Debug"
SOURCE=.\libpng\png.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\png.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\png.obj"	"$(INTDIR)\png.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pnggccrd.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pnggccrd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pnggccrd.obj"	"$(INTDIR)\pnggccrd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngget.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngget.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngget.obj"	"$(INTDIR)\pngget.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngmem.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngmem.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngmem.obj"	"$(INTDIR)\pngmem.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngpread.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngpread.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngpread.obj"	"$(INTDIR)\pngpread.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngread.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngread.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngread.obj"	"$(INTDIR)\pngread.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngrio.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngrio.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngrio.obj"	"$(INTDIR)\pngrio.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngrtran.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngrtran.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngrtran.obj"	"$(INTDIR)\pngrtran.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngrutil.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngrutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngrutil.obj"	"$(INTDIR)\pngrutil.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngset.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngset.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngset.obj"	"$(INTDIR)\pngset.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngtrans.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngtrans.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngtrans.obj"	"$(INTDIR)\pngtrans.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngvcrd.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngvcrd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngvcrd.obj"	"$(INTDIR)\pngvcrd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngwio.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngwio.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngwio.obj"	"$(INTDIR)\pngwio.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngwrite.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngwrite.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngwrite.obj"	"$(INTDIR)\pngwrite.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngwtran.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngwtran.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngwtran.obj"	"$(INTDIR)\pngwtran.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\libpng\pngwutil.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\pngwutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\pngwutil.obj"	"$(INTDIR)\pngwutil.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\png2dib.c

!IF  "$(CFG)" == "png2dib - Win32 Release"


"$(INTDIR)\png2dib.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"


"$(INTDIR)\png2dib.obj"	"$(INTDIR)\png2dib.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\version.rc

"$(INTDIR)\version.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!IF  "$(CFG)" == "png2dib - Win32 Release"

"zlib - Win32 Release" : 
   cd "\Miranda\miranda\plugins\zlib"
   $(MAKE) /$(MAKEFLAGS) /F .\zlib.mak CFG="zlib - Win32 Release" 
   cd "..\png2dib"

"zlib - Win32 ReleaseCLEAN" : 
   cd "\Miranda\miranda\plugins\zlib"
   $(MAKE) /$(MAKEFLAGS) /F .\zlib.mak CFG="zlib - Win32 Release" RECURSE=1 CLEAN 
   cd "..\png2dib"

!ELSEIF  "$(CFG)" == "png2dib - Win32 Debug"

"zlib - Win32 Debug" : 
   cd "\Miranda\miranda\plugins\zlib"
   $(MAKE) /$(MAKEFLAGS) /F .\zlib.mak CFG="zlib - Win32 Debug" 
   cd "..\png2dib"

"zlib - Win32 DebugCLEAN" : 
   cd "\Miranda\miranda\plugins\zlib"
   $(MAKE) /$(MAKEFLAGS) /F .\zlib.mak CFG="zlib - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\png2dib"

!ENDIF 


!ENDIF 

