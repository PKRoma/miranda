# Microsoft Developer Studio Generated NMAKE File, Based on dbrw.dsp
!IF "$(CFG)" == ""
CFG=dbrw - Win32 Debug
!MESSAGE No configuration specified. Defaulting to dbrw - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "dbrw - Win32 Release" && "$(CFG)" != "dbrw - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dbrw.mak" CFG="dbrw - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dbrw - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dbrw - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "dbrw - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\release\plugins\dbx_rw.dll"


CLEAN :
	-@erase "$(INTDIR)\alter.obj"
	-@erase "$(INTDIR)\analyze.obj"
	-@erase "$(INTDIR)\attach.obj"
	-@erase "$(INTDIR)\auth.obj"
	-@erase "$(INTDIR)\btree.obj"
	-@erase "$(INTDIR)\build.obj"
	-@erase "$(INTDIR)\callback.obj"
	-@erase "$(INTDIR)\complete.obj"
	-@erase "$(INTDIR)\contacts.obj"
	-@erase "$(INTDIR)\date.obj"
	-@erase "$(INTDIR)\dbrw.obj"
	-@erase "$(INTDIR)\delete.obj"
	-@erase "$(INTDIR)\events.obj"
	-@erase "$(INTDIR)\expr.obj"
	-@erase "$(INTDIR)\func.obj"
	-@erase "$(INTDIR)\hash.obj"
	-@erase "$(INTDIR)\insert.obj"
	-@erase "$(INTDIR)\legacy.obj"
	-@erase "$(INTDIR)\loadext.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\malloc.obj"
	-@erase "$(INTDIR)\opcodes.obj"
	-@erase "$(INTDIR)\os.obj"
	-@erase "$(INTDIR)\os_os2.obj"
	-@erase "$(INTDIR)\os_unix.obj"
	-@erase "$(INTDIR)\os_win.obj"
	-@erase "$(INTDIR)\pager.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\pragma.obj"
	-@erase "$(INTDIR)\prepare.obj"
	-@erase "$(INTDIR)\printf.obj"
	-@erase "$(INTDIR)\random.obj"
	-@erase "$(INTDIR)\select.obj"
	-@erase "$(INTDIR)\settings.obj"
	-@erase "$(INTDIR)\sql.obj"
	-@erase "$(INTDIR)\table.obj"
	-@erase "$(INTDIR)\tokenize.obj"
	-@erase "$(INTDIR)\trigger.obj"
	-@erase "$(INTDIR)\update.obj"
	-@erase "$(INTDIR)\utf.obj"
	-@erase "$(INTDIR)\utf8.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\vacuum.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vdbe.obj"
	-@erase "$(INTDIR)\vdbeapi.obj"
	-@erase "$(INTDIR)\vdbeaux.obj"
	-@erase "$(INTDIR)\vdbeblob.obj"
	-@erase "$(INTDIR)\vdbefifo.obj"
	-@erase "$(INTDIR)\vdbemem.obj"
	-@erase "$(INTDIR)\vtab.obj"
	-@erase "$(INTDIR)\where.obj"
	-@erase "$(OUTDIR)\dbx_rw.exp"
	-@erase "$(OUTDIR)\dbx_rw.lib"
	-@erase "..\..\bin\release\plugins\dbx_rw.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "../../include" /D _WIN32_WINNT=0x0500 /D "DBRW_OLD_PLUGINAPI_SUPPORT" /D "NO_TCL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DBRW_EXPORTS" /Fp"$(INTDIR)\dbrw.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\dbrw.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\dbx_rw.pdb" /machine:I386 /out:"../../bin/release/plugins/dbx_rw.dll" /implib:"$(OUTDIR)\dbx_rw.lib" 
LINK32_OBJS= \
	"$(INTDIR)\contacts.obj" \
	"$(INTDIR)\dbrw.obj" \
	"$(INTDIR)\events.obj" \
	"$(INTDIR)\settings.obj" \
	"$(INTDIR)\sql.obj" \
	"$(INTDIR)\utf8.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\alter.obj" \
	"$(INTDIR)\analyze.obj" \
	"$(INTDIR)\attach.obj" \
	"$(INTDIR)\auth.obj" \
	"$(INTDIR)\btree.obj" \
	"$(INTDIR)\build.obj" \
	"$(INTDIR)\callback.obj" \
	"$(INTDIR)\complete.obj" \
	"$(INTDIR)\date.obj" \
	"$(INTDIR)\delete.obj" \
	"$(INTDIR)\expr.obj" \
	"$(INTDIR)\func.obj" \
	"$(INTDIR)\hash.obj" \
	"$(INTDIR)\insert.obj" \
	"$(INTDIR)\legacy.obj" \
	"$(INTDIR)\loadext.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\malloc.obj" \
	"$(INTDIR)\opcodes.obj" \
	"$(INTDIR)\os.obj" \
	"$(INTDIR)\os_os2.obj" \
	"$(INTDIR)\os_unix.obj" \
	"$(INTDIR)\os_win.obj" \
	"$(INTDIR)\pager.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\pragma.obj" \
	"$(INTDIR)\prepare.obj" \
	"$(INTDIR)\printf.obj" \
	"$(INTDIR)\random.obj" \
	"$(INTDIR)\select.obj" \
	"$(INTDIR)\table.obj" \
	"$(INTDIR)\tokenize.obj" \
	"$(INTDIR)\trigger.obj" \
	"$(INTDIR)\update.obj" \
	"$(INTDIR)\utf.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\vacuum.obj" \
	"$(INTDIR)\vdbe.obj" \
	"$(INTDIR)\vdbeapi.obj" \
	"$(INTDIR)\vdbeaux.obj" \
	"$(INTDIR)\vdbeblob.obj" \
	"$(INTDIR)\vdbefifo.obj" \
	"$(INTDIR)\vdbemem.obj" \
	"$(INTDIR)\vtab.obj" \
	"$(INTDIR)\where.obj"

"..\..\bin\release\plugins\dbx_rw.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "dbrw - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\bin\debug\plugins\dbx_rw.dll"


CLEAN :
	-@erase "$(INTDIR)\alter.obj"
	-@erase "$(INTDIR)\analyze.obj"
	-@erase "$(INTDIR)\attach.obj"
	-@erase "$(INTDIR)\auth.obj"
	-@erase "$(INTDIR)\btree.obj"
	-@erase "$(INTDIR)\build.obj"
	-@erase "$(INTDIR)\callback.obj"
	-@erase "$(INTDIR)\complete.obj"
	-@erase "$(INTDIR)\contacts.obj"
	-@erase "$(INTDIR)\date.obj"
	-@erase "$(INTDIR)\dbrw.obj"
	-@erase "$(INTDIR)\delete.obj"
	-@erase "$(INTDIR)\events.obj"
	-@erase "$(INTDIR)\expr.obj"
	-@erase "$(INTDIR)\func.obj"
	-@erase "$(INTDIR)\hash.obj"
	-@erase "$(INTDIR)\insert.obj"
	-@erase "$(INTDIR)\legacy.obj"
	-@erase "$(INTDIR)\loadext.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\malloc.obj"
	-@erase "$(INTDIR)\opcodes.obj"
	-@erase "$(INTDIR)\os.obj"
	-@erase "$(INTDIR)\os_os2.obj"
	-@erase "$(INTDIR)\os_unix.obj"
	-@erase "$(INTDIR)\os_win.obj"
	-@erase "$(INTDIR)\pager.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\pragma.obj"
	-@erase "$(INTDIR)\prepare.obj"
	-@erase "$(INTDIR)\printf.obj"
	-@erase "$(INTDIR)\random.obj"
	-@erase "$(INTDIR)\select.obj"
	-@erase "$(INTDIR)\settings.obj"
	-@erase "$(INTDIR)\sql.obj"
	-@erase "$(INTDIR)\table.obj"
	-@erase "$(INTDIR)\tokenize.obj"
	-@erase "$(INTDIR)\trigger.obj"
	-@erase "$(INTDIR)\update.obj"
	-@erase "$(INTDIR)\utf.obj"
	-@erase "$(INTDIR)\utf8.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\vacuum.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\vdbe.obj"
	-@erase "$(INTDIR)\vdbeapi.obj"
	-@erase "$(INTDIR)\vdbeaux.obj"
	-@erase "$(INTDIR)\vdbeblob.obj"
	-@erase "$(INTDIR)\vdbefifo.obj"
	-@erase "$(INTDIR)\vdbemem.obj"
	-@erase "$(INTDIR)\vtab.obj"
	-@erase "$(INTDIR)\where.obj"
	-@erase "$(OUTDIR)\dbx_rw.exp"
	-@erase "$(OUTDIR)\dbx_rw.lib"
	-@erase "$(OUTDIR)\dbx_rw.pdb"
	-@erase "..\..\bin\debug\plugins\dbx_rw.dll"
	-@erase "..\..\bin\debug\plugins\dbx_rw.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D _WIN32_WINNT=0x0500 /D "DBRW_OLD_PLUGINAPI_SUPPORT" /D "DBRW_DEBUG" /D "DBRW_LOGGING" /D "NO_TCL" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DBRW_EXPORTS" /Fp"$(INTDIR)\dbrw.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\dbrw.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\dbx_rw.pdb" /debug /machine:I386 /out:"../../bin/debug/plugins/dbx_rw.dll" /implib:"$(OUTDIR)\dbx_rw.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\contacts.obj" \
	"$(INTDIR)\dbrw.obj" \
	"$(INTDIR)\events.obj" \
	"$(INTDIR)\settings.obj" \
	"$(INTDIR)\sql.obj" \
	"$(INTDIR)\utf8.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\alter.obj" \
	"$(INTDIR)\analyze.obj" \
	"$(INTDIR)\attach.obj" \
	"$(INTDIR)\auth.obj" \
	"$(INTDIR)\btree.obj" \
	"$(INTDIR)\build.obj" \
	"$(INTDIR)\callback.obj" \
	"$(INTDIR)\complete.obj" \
	"$(INTDIR)\date.obj" \
	"$(INTDIR)\delete.obj" \
	"$(INTDIR)\expr.obj" \
	"$(INTDIR)\func.obj" \
	"$(INTDIR)\hash.obj" \
	"$(INTDIR)\insert.obj" \
	"$(INTDIR)\legacy.obj" \
	"$(INTDIR)\loadext.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\malloc.obj" \
	"$(INTDIR)\opcodes.obj" \
	"$(INTDIR)\os.obj" \
	"$(INTDIR)\os_os2.obj" \
	"$(INTDIR)\os_unix.obj" \
	"$(INTDIR)\os_win.obj" \
	"$(INTDIR)\pager.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\pragma.obj" \
	"$(INTDIR)\prepare.obj" \
	"$(INTDIR)\printf.obj" \
	"$(INTDIR)\random.obj" \
	"$(INTDIR)\select.obj" \
	"$(INTDIR)\table.obj" \
	"$(INTDIR)\tokenize.obj" \
	"$(INTDIR)\trigger.obj" \
	"$(INTDIR)\update.obj" \
	"$(INTDIR)\utf.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\vacuum.obj" \
	"$(INTDIR)\vdbe.obj" \
	"$(INTDIR)\vdbeapi.obj" \
	"$(INTDIR)\vdbeaux.obj" \
	"$(INTDIR)\vdbeblob.obj" \
	"$(INTDIR)\vdbefifo.obj" \
	"$(INTDIR)\vdbemem.obj" \
	"$(INTDIR)\vtab.obj" \
	"$(INTDIR)\where.obj"

"..\..\bin\debug\plugins\dbx_rw.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("dbrw.dep")
!INCLUDE "dbrw.dep"
!ELSE 
!MESSAGE Warning: cannot find "dbrw.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "dbrw - Win32 Release" || "$(CFG)" == "dbrw - Win32 Debug"
SOURCE=.\contacts.c

"$(INTDIR)\contacts.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dbrw.c

"$(INTDIR)\dbrw.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\events.c

"$(INTDIR)\events.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\settings.c

"$(INTDIR)\settings.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\sql.c

"$(INTDIR)\sql.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\utf8.c

"$(INTDIR)\utf8.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\utils.c

"$(INTDIR)\utils.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\sqlite3\alter.c

"$(INTDIR)\alter.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\analyze.c

"$(INTDIR)\analyze.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\attach.c

"$(INTDIR)\attach.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\auth.c

"$(INTDIR)\auth.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\btree.c

"$(INTDIR)\btree.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\build.c

"$(INTDIR)\build.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\callback.c

"$(INTDIR)\callback.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\complete.c

"$(INTDIR)\complete.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\date.c

"$(INTDIR)\date.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\delete.c

"$(INTDIR)\delete.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\expr.c

"$(INTDIR)\expr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\func.c

"$(INTDIR)\func.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\hash.c

"$(INTDIR)\hash.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\insert.c

"$(INTDIR)\insert.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\legacy.c

"$(INTDIR)\legacy.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\loadext.c

"$(INTDIR)\loadext.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\main.c

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\malloc.c

"$(INTDIR)\malloc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\opcodes.c

"$(INTDIR)\opcodes.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\os.c

"$(INTDIR)\os.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\os_os2.c

"$(INTDIR)\os_os2.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\os_unix.c

"$(INTDIR)\os_unix.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\os_win.c

"$(INTDIR)\os_win.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\pager.c

"$(INTDIR)\pager.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\parse.c

"$(INTDIR)\parse.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\pragma.c

"$(INTDIR)\pragma.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\prepare.c

"$(INTDIR)\prepare.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\printf.c

"$(INTDIR)\printf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\random.c

"$(INTDIR)\random.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\select.c

"$(INTDIR)\select.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\table.c

"$(INTDIR)\table.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\tokenize.c

"$(INTDIR)\tokenize.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\trigger.c

"$(INTDIR)\trigger.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\update.c

"$(INTDIR)\update.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\utf.c

"$(INTDIR)\utf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\util.c

"$(INTDIR)\util.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\vacuum.c

"$(INTDIR)\vacuum.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\vdbe.c

"$(INTDIR)\vdbe.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\vdbeapi.c

"$(INTDIR)\vdbeapi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\vdbeaux.c

"$(INTDIR)\vdbeaux.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\vdbeblob.c

"$(INTDIR)\vdbeblob.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\vdbefifo.c

"$(INTDIR)\vdbefifo.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\vdbemem.c

"$(INTDIR)\vdbemem.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\vtab.c

"$(INTDIR)\vtab.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\sqlite3\where.c

"$(INTDIR)\where.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

