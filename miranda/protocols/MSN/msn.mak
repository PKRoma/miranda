# Microsoft Developer Studio Generated NMAKE File, Based on msn.dsp
!IF "$(CFG)" == ""
CFG=msn - Win32 Release
!MESSAGE No configuration specified. Defaulting to msn - Win32 Debug.
!ENDIF

!IF "$(CFG)" != "msn - Win32 Release" && "$(CFG)" != "msn - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "msn.mak" CFG="msn - Win32 Debug"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "msn - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "msn - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE
!ERROR An invalid configuration is specified.
!ENDIF

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE
NULL=nul
!ENDIF

!IF  "$(CFG)" == "msn - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\release\plugins\msn.dll"


CLEAN :
	-@erase "$(INTDIR)\mmdecsjis.obj"
	-@erase "$(INTDIR)\msn.obj"
	-@erase "$(INTDIR)\msn.pch"
	-@erase "$(INTDIR)\msn_bitmap.obj"
	-@erase "$(INTDIR)\msn_chat.obj"
	-@erase "$(INTDIR)\msn_commands.obj"
	-@erase "$(INTDIR)\msn_contact.obj"
	-@erase "$(INTDIR)\msn_errors.obj"
	-@erase "$(INTDIR)\msn_http.obj"
	-@erase "$(INTDIR)\msn_libstr.obj"
	-@erase "$(INTDIR)\msn_lists.obj"
	-@erase "$(INTDIR)\msn_md5c.obj"
	-@erase "$(INTDIR)\msn_mime.obj"
	-@erase "$(INTDIR)\msn_misc.obj"
	-@erase "$(INTDIR)\msn_msgqueue.obj"
	-@erase "$(INTDIR)\msn_opts.obj"
	-@erase "$(INTDIR)\msn_p2p.obj"
	-@erase "$(INTDIR)\msn_p2ps.obj"
	-@erase "$(INTDIR)\msn_srv.obj"
	-@erase "$(INTDIR)\msn_ssl.obj"
	-@erase "$(INTDIR)\msn_std.obj"
	-@erase "$(INTDIR)\msn_svcs.obj"
	-@erase "$(INTDIR)\msn_switchboard.obj"
	-@erase "$(INTDIR)\msn_threads.obj"
	-@erase "$(INTDIR)\msn_useropts.obj"
	-@erase "$(INTDIR)\msn_ws.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\sha1.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\msn.exp"
	-@erase "$(OUTDIR)\msn.lib"
	-@erase "$(OUTDIR)\msn.map"
	-@erase "..\..\bin\release\plugins\msn.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSN_EXPORTS" /FAcs /Fa"$(INTDIR)\\" /Fp"$(INTDIR)\msn.pch" /Yu"msn_global.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

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
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /d "NDEBUG"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\msn.bsc"
BSC32_SBRS= \

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib comctl32.lib Rpcrt4.lib /nologo /base:"0x19000000" /dll /incremental:no /pdb:"$(OUTDIR)\msn.pdb" /map:"$(INTDIR)\msn.map" /machine:I386 /def:".\msn.def" /out:"../../bin/release/plugins/msn.dll" /implib:"$(OUTDIR)\msn.lib" /filealign:512
DEF_FILE= \
	".\msn.def"
LINK32_OBJS= \
	"$(INTDIR)\mmdecsjis.obj" \
	"$(INTDIR)\msn.obj" \
	"$(INTDIR)\msn_bitmap.obj" \
	"$(INTDIR)\msn_chat.obj" \
	"$(INTDIR)\msn_commands.obj" \
	"$(INTDIR)\msn_contact.obj" \
	"$(INTDIR)\msn_errors.obj" \
	"$(INTDIR)\msn_http.obj" \
	"$(INTDIR)\msn_libstr.obj" \
	"$(INTDIR)\msn_lists.obj" \
	"$(INTDIR)\msn_md5c.obj" \
	"$(INTDIR)\msn_mime.obj" \
	"$(INTDIR)\msn_misc.obj" \
	"$(INTDIR)\msn_msgqueue.obj" \
	"$(INTDIR)\msn_opts.obj" \
	"$(INTDIR)\msn_p2p.obj" \
	"$(INTDIR)\msn_p2ps.obj" \
	"$(INTDIR)\msn_ssl.obj" \
	"$(INTDIR)\msn_std.obj" \
	"$(INTDIR)\msn_svcs.obj" \
	"$(INTDIR)\msn_switchboard.obj" \
	"$(INTDIR)\msn_threads.obj" \
	"$(INTDIR)\msn_useropts.obj" \
	"$(INTDIR)\msn_ws.obj" \
	"$(INTDIR)\sha1.obj" \
	"$(INTDIR)\resource.res" \
	"$(INTDIR)\msn_srv.obj"

"..\..\bin\release\plugins\msn.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "msn - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "..\..\bin\debug\plugins\msn.dll" "$(OUTDIR)\msn.bsc"


CLEAN :
	-@erase "$(INTDIR)\mmdecsjis.obj"
	-@erase "$(INTDIR)\mmdecsjis.sbr"
	-@erase "$(INTDIR)\msn.obj"
	-@erase "$(INTDIR)\msn.pch"
	-@erase "$(INTDIR)\msn.sbr"
	-@erase "$(INTDIR)\msn_bitmap.obj"
	-@erase "$(INTDIR)\msn_bitmap.sbr"
	-@erase "$(INTDIR)\msn_chat.obj"
	-@erase "$(INTDIR)\msn_chat.sbr"
	-@erase "$(INTDIR)\msn_commands.obj"
	-@erase "$(INTDIR)\msn_commands.sbr"
	-@erase "$(INTDIR)\msn_contact.obj"
	-@erase "$(INTDIR)\msn_contact.sbr"
	-@erase "$(INTDIR)\msn_errors.obj"
	-@erase "$(INTDIR)\msn_errors.sbr"
	-@erase "$(INTDIR)\msn_http.obj"
	-@erase "$(INTDIR)\msn_http.sbr"
	-@erase "$(INTDIR)\msn_libstr.obj"
	-@erase "$(INTDIR)\msn_libstr.sbr"
	-@erase "$(INTDIR)\msn_lists.obj"
	-@erase "$(INTDIR)\msn_lists.sbr"
	-@erase "$(INTDIR)\msn_md5c.obj"
	-@erase "$(INTDIR)\msn_md5c.sbr"
	-@erase "$(INTDIR)\msn_mime.obj"
	-@erase "$(INTDIR)\msn_mime.sbr"
	-@erase "$(INTDIR)\msn_misc.obj"
	-@erase "$(INTDIR)\msn_misc.sbr"
	-@erase "$(INTDIR)\msn_msgqueue.obj"
	-@erase "$(INTDIR)\msn_msgqueue.sbr"
	-@erase "$(INTDIR)\msn_opts.obj"
	-@erase "$(INTDIR)\msn_opts.sbr"
	-@erase "$(INTDIR)\msn_p2p.obj"
	-@erase "$(INTDIR)\msn_p2p.sbr"
	-@erase "$(INTDIR)\msn_p2ps.obj"
	-@erase "$(INTDIR)\msn_p2ps.sbr"
	-@erase "$(INTDIR)\msn_srv.obj"
	-@erase "$(INTDIR)\msn_srv.sbr"
	-@erase "$(INTDIR)\msn_ssl.obj"
	-@erase "$(INTDIR)\msn_ssl.sbr"
	-@erase "$(INTDIR)\msn_std.obj"
	-@erase "$(INTDIR)\msn_std.sbr"
	-@erase "$(INTDIR)\msn_svcs.obj"
	-@erase "$(INTDIR)\msn_svcs.sbr"
	-@erase "$(INTDIR)\msn_switchboard.obj"
	-@erase "$(INTDIR)\msn_switchboard.sbr"
	-@erase "$(INTDIR)\msn_threads.obj"
	-@erase "$(INTDIR)\msn_threads.sbr"
	-@erase "$(INTDIR)\msn_useropts.obj"
	-@erase "$(INTDIR)\msn_useropts.sbr"
	-@erase "$(INTDIR)\msn_ws.obj"
	-@erase "$(INTDIR)\msn_ws.sbr"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\sha1.obj"
	-@erase "$(INTDIR)\sha1.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\msn.bsc"
	-@erase "$(OUTDIR)\msn.exp"
	-@erase "$(OUTDIR)\msn.lib"
	-@erase "$(OUTDIR)\msn.pdb"
	-@erase "..\..\bin\debug\plugins\msn.dll"
	-@erase "..\..\bin\debug\plugins\msn.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /Gi /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSN_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\msn.pch" /Yu"msn_global.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

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
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /d "_DEBUG"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\msn.bsc"
BSC32_SBRS= \
	"$(INTDIR)\mmdecsjis.sbr" \
	"$(INTDIR)\msn.sbr" \
	"$(INTDIR)\msn_bitmap.sbr" \
	"$(INTDIR)\msn_chat.sbr" \
	"$(INTDIR)\msn_commands.sbr" \
	"$(INTDIR)\msn_contact.sbr" \
	"$(INTDIR)\msn_errors.sbr" \
	"$(INTDIR)\msn_http.sbr" \
	"$(INTDIR)\msn_libstr.sbr" \
	"$(INTDIR)\msn_lists.sbr" \
	"$(INTDIR)\msn_md5c.sbr" \
	"$(INTDIR)\msn_mime.sbr" \
	"$(INTDIR)\msn_misc.sbr" \
	"$(INTDIR)\msn_msgqueue.sbr" \
	"$(INTDIR)\msn_opts.sbr" \
	"$(INTDIR)\msn_p2p.sbr" \
	"$(INTDIR)\msn_p2ps.sbr" \
	"$(INTDIR)\msn_ssl.sbr" \
	"$(INTDIR)\msn_std.sbr" \
	"$(INTDIR)\msn_svcs.sbr" \
	"$(INTDIR)\msn_switchboard.sbr" \
	"$(INTDIR)\msn_threads.sbr" \
	"$(INTDIR)\msn_useropts.sbr" \
	"$(INTDIR)\msn_ws.sbr" \
	"$(INTDIR)\sha1.sbr" \
	"$(INTDIR)\msn_srv.sbr"

"$(OUTDIR)\msn.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib comctl32.lib Rpcrt4.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\msn.pdb" /debug /machine:I386 /def:".\msn.def" /out:"../../bin/debug/plugins/msn.dll" /implib:"$(OUTDIR)\msn.lib" /pdbtype:sept
DEF_FILE= \
	".\msn.def"
LINK32_OBJS= \
	"$(INTDIR)\mmdecsjis.obj" \
	"$(INTDIR)\msn.obj" \
	"$(INTDIR)\msn_bitmap.obj" \
	"$(INTDIR)\msn_chat.obj" \
	"$(INTDIR)\msn_commands.obj" \
	"$(INTDIR)\msn_contact.obj" \
	"$(INTDIR)\msn_errors.obj" \
	"$(INTDIR)\msn_http.obj" \
	"$(INTDIR)\msn_libstr.obj" \
	"$(INTDIR)\msn_lists.obj" \
	"$(INTDIR)\msn_md5c.obj" \
	"$(INTDIR)\msn_mime.obj" \
	"$(INTDIR)\msn_misc.obj" \
	"$(INTDIR)\msn_msgqueue.obj" \
	"$(INTDIR)\msn_opts.obj" \
	"$(INTDIR)\msn_p2p.obj" \
	"$(INTDIR)\msn_p2ps.obj" \
	"$(INTDIR)\msn_ssl.obj" \
	"$(INTDIR)\msn_std.obj" \
	"$(INTDIR)\msn_svcs.obj" \
	"$(INTDIR)\msn_switchboard.obj" \
	"$(INTDIR)\msn_threads.obj" \
	"$(INTDIR)\msn_useropts.obj" \
	"$(INTDIR)\msn_ws.obj" \
	"$(INTDIR)\sha1.obj" \
	"$(INTDIR)\resource.res" \
	"$(INTDIR)\msn_srv.obj"

"..\..\bin\debug\plugins\msn.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("msn.dep")
!INCLUDE "msn.dep"
!ELSE
!MESSAGE Warning: cannot find "msn.dep"
!ENDIF
!ENDIF


!IF "$(CFG)" == "msn - Win32 Release" || "$(CFG)" == "msn - Win32 Debug"
SOURCE=.\mmdecsjis.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\mmdecsjis.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\mmdecsjis.obj"	"$(INTDIR)\mmdecsjis.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn.cpp

!IF  "$(CFG)" == "msn - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSN_EXPORTS" /FAcs /Fa"$(INTDIR)\\" /Fp"$(INTDIR)\msn.pch" /Yc"msn_global.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\msn.obj"	"$(INTDIR)\msn.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /Gi /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSN_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\msn.pch" /Yc"msn_global.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\msn.obj"	"$(INTDIR)\msn.sbr"	"$(INTDIR)\msn.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\msn_bitmap.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_bitmap.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_bitmap.obj"	"$(INTDIR)\msn_bitmap.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_chat.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_chat.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_chat.obj"	"$(INTDIR)\msn_chat.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_commands.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_commands.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_commands.obj"	"$(INTDIR)\msn_commands.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_contact.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_contact.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_contact.obj"	"$(INTDIR)\msn_contact.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_errors.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_errors.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_errors.obj"	"$(INTDIR)\msn_errors.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_http.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_http.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_http.obj"	"$(INTDIR)\msn_http.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_libstr.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_libstr.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_libstr.obj"	"$(INTDIR)\msn_libstr.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_lists.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_lists.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_lists.obj"	"$(INTDIR)\msn_lists.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_md5c.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_md5c.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_md5c.obj"	"$(INTDIR)\msn_md5c.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_mime.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_mime.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_mime.obj"	"$(INTDIR)\msn_mime.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_misc.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_misc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_misc.obj"	"$(INTDIR)\msn_misc.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_msgqueue.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_msgqueue.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_msgqueue.obj"	"$(INTDIR)\msn_msgqueue.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_opts.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_opts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_opts.obj"	"$(INTDIR)\msn_opts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_p2p.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_p2p.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_p2p.obj"	"$(INTDIR)\msn_p2p.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_p2ps.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_p2ps.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_p2ps.obj"	"$(INTDIR)\msn_p2ps.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_srv.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_srv.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_srv.obj"	"$(INTDIR)\msn_srv.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_ssl.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_ssl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_ssl.obj"	"$(INTDIR)\msn_ssl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_std.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_std.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_std.obj"	"$(INTDIR)\msn_std.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_svcs.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_svcs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_svcs.obj"	"$(INTDIR)\msn_svcs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_switchboard.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_switchboard.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_switchboard.obj"	"$(INTDIR)\msn_switchboard.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_threads.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_threads.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_threads.obj"	"$(INTDIR)\msn_threads.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_useropts.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_useropts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_useropts.obj"	"$(INTDIR)\msn_useropts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\msn_ws.cpp

!IF  "$(CFG)" == "msn - Win32 Release"


"$(INTDIR)\msn_ws.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"


"$(INTDIR)\msn_ws.obj"	"$(INTDIR)\msn_ws.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\msn.pch"


!ENDIF

SOURCE=.\sha1.c

!IF  "$(CFG)" == "msn - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /Zi /O1 /Oy /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSN_EXPORTS" /FAcs /Fa"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\sha1.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "msn - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /Gi /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSN_EXPORTS" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\sha1.obj"	"$(INTDIR)\sha1.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\resource.rc

"$(INTDIR)\resource.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF
