# Microsoft Developer Studio Generated NMAKE File, Based on jabber.dsp
!IF "$( CFG)" == ""
CFG=jabberg - Win32 Release
!MESSAGE No configuration specified. Defaulting to jabberg - Win32 Release.
!ENDIF 

!IF "$( CFG)" != "jabberg - Win32 Release" && "$( CFG)" != "jabberg - Win32 Debug"
!MESSAGE Invalid configuration "$( CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "jabber.mak" CFG="jabberg - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "jabberg - Win32 Release" ( based on "Win32 ( x86) Dynamic-Link Library")
!MESSAGE "jabberg - Win32 Debug" ( based on "Win32 ( x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$( OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$( CFG)" == "jabberg - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\release\plugins\jabber.dll"


CLEAN :
	-@erase "$( INTDIR)\jabber.obj"
	-@erase "$( INTDIR)\jabber.pch"
	-@erase "$( INTDIR)\jabber_agent.obj"
	-@erase "$( INTDIR)\jabber_byte.obj"
	-@erase "$( INTDIR)\jabber_file.obj"
	-@erase "$( INTDIR)\jabber_form.obj"
	-@erase "$( INTDIR)\jabber_ft.obj"
	-@erase "$( INTDIR)\jabber_groupchat.obj"
	-@erase "$( INTDIR)\jabber_groupchatlog.obj"
	-@erase "$( INTDIR)\jabber_iq.obj"
	-@erase "$( INTDIR)\jabber_iqid.obj"
	-@erase "$( INTDIR)\jabber_iqid_muc.obj"
	-@erase "$( INTDIR)\jabber_list.obj"
	-@erase "$( INTDIR)\jabber_menu.obj"
	-@erase "$( INTDIR)\jabber_misc.obj"
	-@erase "$( INTDIR)\jabber_opt.obj"
	-@erase "$( INTDIR)\jabber_password.obj"
	-@erase "$( INTDIR)\jabber_proxy.obj"
	-@erase "$( INTDIR)\jabber_ssl.obj"
	-@erase "$( INTDIR)\jabber_svc.obj"
	-@erase "$( INTDIR)\jabber_thread.obj"
	-@erase "$( INTDIR)\jabber_userinfo.obj"
	-@erase "$( INTDIR)\jabber_util.obj"
	-@erase "$( INTDIR)\jabber_vcard.obj"
	-@erase "$( INTDIR)\jabber_ws.obj"
	-@erase "$( INTDIR)\jabber_xml.obj"
	-@erase "$( INTDIR)\jabber_xmlns.obj"
	-@erase "$( INTDIR)\msvc6.res"
	-@erase "$( INTDIR)\sha1.obj"
	-@erase "$( INTDIR)\vc60.idb"
	-@erase "$( OUTDIR)\jabber.exp"
	-@erase "$( OUTDIR)\jabber.lib"
	-@erase "$( OUTDIR)\jabber.map"
	-@erase "..\..\bin\release\plugins\jabber.dll"

"$( OUTDIR)" :
    if not exist "$( OUTDIR)/$( NULL)" mkdir "$( OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$( INTDIR)\msvc6.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$( OUTDIR)\jabber.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib version.lib /nologo /base:"0x32500000" /dll /incremental:no /pdb:"$( OUTDIR)\jabber.pdb" /map:"$( INTDIR)\jabber.map" /machine:I386 /out:"../../bin/release/plugins/jabber.dll" /implib:"$( OUTDIR)\jabber.lib" /ALIGN:4096 /ignore:4108 
LINK32_OBJS= \
	"$( INTDIR)\jabber.obj" \
	"$( INTDIR)\jabber_agent.obj" \
	"$( INTDIR)\jabber_byte.obj" \
	"$( INTDIR)\jabber_file.obj" \
	"$( INTDIR)\jabber_form.obj" \
	"$( INTDIR)\jabber_ft.obj" \
	"$( INTDIR)\jabber_groupchat.obj" \
	"$( INTDIR)\jabber_groupchatlog.obj" \
	"$( INTDIR)\jabber_iq.obj" \
	"$( INTDIR)\jabber_iqid.obj" \
	"$( INTDIR)\jabber_iqid_muc.obj" \
	"$( INTDIR)\jabber_list.obj" \
	"$( INTDIR)\jabber_menu.obj" \
	"$( INTDIR)\jabber_misc.obj" \
	"$( INTDIR)\jabber_opt.obj" \
	"$( INTDIR)\jabber_password.obj" \
	"$( INTDIR)\jabber_proxy.obj" \
	"$( INTDIR)\jabber_ssl.obj" \
	"$( INTDIR)\jabber_svc.obj" \
	"$( INTDIR)\jabber_thread.obj" \
	"$( INTDIR)\jabber_userinfo.obj" \
	"$( INTDIR)\jabber_util.obj" \
	"$( INTDIR)\jabber_vcard.obj" \
	"$( INTDIR)\jabber_ws.obj" \
	"$( INTDIR)\jabber_xml.obj" \
	"$( INTDIR)\jabber_xmlns.obj" \
	"$( INTDIR)\sha1.obj" \
	"$( INTDIR)\msvc6.res"

"..\..\bin\release\plugins\jabber.dll" : "$( OUTDIR)" $( DEF_FILE) $( LINK32_OBJS)
    $( LINK32) @<<
  $( LINK32_FLAGS) $( LINK32_OBJS)
<<

!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "..\..\bin\debug\plugins\jabber.dll" "$( OUTDIR)\jabber.bsc"


CLEAN :
	-@erase "$( INTDIR)\jabber.obj"
	-@erase "$( INTDIR)\jabber.pch"
	-@erase "$( INTDIR)\jabber.sbr"
	-@erase "$( INTDIR)\jabber_agent.obj"
	-@erase "$( INTDIR)\jabber_agent.sbr"
	-@erase "$( INTDIR)\jabber_byte.obj"
	-@erase "$( INTDIR)\jabber_byte.sbr"
	-@erase "$( INTDIR)\jabber_file.obj"
	-@erase "$( INTDIR)\jabber_file.sbr"
	-@erase "$( INTDIR)\jabber_form.obj"
	-@erase "$( INTDIR)\jabber_form.sbr"
	-@erase "$( INTDIR)\jabber_ft.obj"
	-@erase "$( INTDIR)\jabber_ft.sbr"
	-@erase "$( INTDIR)\jabber_groupchat.obj"
	-@erase "$( INTDIR)\jabber_groupchat.sbr"
	-@erase "$( INTDIR)\jabber_groupchatlog.obj"
	-@erase "$( INTDIR)\jabber_groupchatlog.sbr"
	-@erase "$( INTDIR)\jabber_iq.obj"
	-@erase "$( INTDIR)\jabber_iq.sbr"
	-@erase "$( INTDIR)\jabber_iqid.obj"
	-@erase "$( INTDIR)\jabber_iqid.sbr"
	-@erase "$( INTDIR)\jabber_iqid_muc.obj"
	-@erase "$( INTDIR)\jabber_iqid_muc.sbr"
	-@erase "$( INTDIR)\jabber_list.obj"
	-@erase "$( INTDIR)\jabber_list.sbr"
	-@erase "$( INTDIR)\jabber_menu.obj"
	-@erase "$( INTDIR)\jabber_menu.sbr"
	-@erase "$( INTDIR)\jabber_misc.obj"
	-@erase "$( INTDIR)\jabber_misc.sbr"
	-@erase "$( INTDIR)\jabber_opt.obj"
	-@erase "$( INTDIR)\jabber_opt.sbr"
	-@erase "$( INTDIR)\jabber_password.obj"
	-@erase "$( INTDIR)\jabber_password.sbr"
	-@erase "$( INTDIR)\jabber_proxy.obj"
	-@erase "$( INTDIR)\jabber_proxy.sbr"
	-@erase "$( INTDIR)\jabber_ssl.obj"
	-@erase "$( INTDIR)\jabber_ssl.sbr"
	-@erase "$( INTDIR)\jabber_svc.obj"
	-@erase "$( INTDIR)\jabber_svc.sbr"
	-@erase "$( INTDIR)\jabber_thread.obj"
	-@erase "$( INTDIR)\jabber_thread.sbr"
	-@erase "$( INTDIR)\jabber_userinfo.obj"
	-@erase "$( INTDIR)\jabber_userinfo.sbr"
	-@erase "$( INTDIR)\jabber_util.obj"
	-@erase "$( INTDIR)\jabber_util.sbr"
	-@erase "$( INTDIR)\jabber_vcard.obj"
	-@erase "$( INTDIR)\jabber_vcard.sbr"
	-@erase "$( INTDIR)\jabber_ws.obj"
	-@erase "$( INTDIR)\jabber_ws.sbr"
	-@erase "$( INTDIR)\jabber_xml.obj"
	-@erase "$( INTDIR)\jabber_xml.sbr"
	-@erase "$( INTDIR)\jabber_xmlns.obj"
	-@erase "$( INTDIR)\jabber_xmlns.sbr"
	-@erase "$( INTDIR)\msvc6.res"
	-@erase "$( INTDIR)\sha1.obj"
	-@erase "$( INTDIR)\sha1.sbr"
	-@erase "$( INTDIR)\vc60.idb"
	-@erase "$( INTDIR)\vc60.pdb"
	-@erase "$( OUTDIR)\jabber.bsc"
	-@erase "$( OUTDIR)\jabber.exp"
	-@erase "$( OUTDIR)\jabber.lib"
	-@erase "$( OUTDIR)\jabber.map"
	-@erase "$( OUTDIR)\jabber.pdb"
	-@erase "..\..\bin\debug\plugins\jabber.dll"
	-@erase "..\..\bin\debug\plugins\jabber.ilk"

"$( OUTDIR)" :
    if not exist "$( OUTDIR)/$( NULL)" mkdir "$( OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$( INTDIR)\msvc6.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$( OUTDIR)\jabber.bsc" 
BSC32_SBRS= \
	"$( INTDIR)\jabber.sbr" \
	"$( INTDIR)\jabber_agent.sbr" \
	"$( INTDIR)\jabber_byte.sbr" \
	"$( INTDIR)\jabber_file.sbr" \
	"$( INTDIR)\jabber_form.sbr" \
	"$( INTDIR)\jabber_ft.sbr" \
	"$( INTDIR)\jabber_groupchat.sbr" \
	"$( INTDIR)\jabber_groupchatlog.sbr" \
	"$( INTDIR)\jabber_iq.sbr" \
	"$( INTDIR)\jabber_iqid.sbr" \
	"$( INTDIR)\jabber_iqid_muc.sbr" \
	"$( INTDIR)\jabber_list.sbr" \
	"$( INTDIR)\jabber_menu.sbr" \
	"$( INTDIR)\jabber_misc.sbr" \
	"$( INTDIR)\jabber_opt.sbr" \
	"$( INTDIR)\jabber_password.sbr" \
	"$( INTDIR)\jabber_proxy.sbr" \
	"$( INTDIR)\jabber_ssl.sbr" \
	"$( INTDIR)\jabber_svc.sbr" \
	"$( INTDIR)\jabber_thread.sbr" \
	"$( INTDIR)\jabber_userinfo.sbr" \
	"$( INTDIR)\jabber_util.sbr" \
	"$( INTDIR)\jabber_vcard.sbr" \
	"$( INTDIR)\jabber_ws.sbr" \
	"$( INTDIR)\jabber_xml.sbr" \
	"$( INTDIR)\jabber_xmlns.sbr" \
	"$( INTDIR)\sha1.sbr"

"$( OUTDIR)\jabber.bsc" : "$( OUTDIR)" $( BSC32_SBRS)
    $( BSC32) @<<
  $( BSC32_FLAGS) $( BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib version.lib /nologo /base:"0x32500000" /dll /incremental:yes /pdb:"$( OUTDIR)\jabber.pdb" /map:"$( INTDIR)\jabber.map" /debug /machine:I386 /out:"../../bin/debug/plugins/jabber.dll" /implib:"$( OUTDIR)\jabber.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$( INTDIR)\jabber.obj" \
	"$( INTDIR)\jabber_agent.obj" \
	"$( INTDIR)\jabber_byte.obj" \
	"$( INTDIR)\jabber_file.obj" \
	"$( INTDIR)\jabber_form.obj" \
	"$( INTDIR)\jabber_ft.obj" \
	"$( INTDIR)\jabber_groupchat.obj" \
	"$( INTDIR)\jabber_groupchatlog.obj" \
	"$( INTDIR)\jabber_iq.obj" \
	"$( INTDIR)\jabber_iqid.obj" \
	"$( INTDIR)\jabber_iqid_muc.obj" \
	"$( INTDIR)\jabber_list.obj" \
	"$( INTDIR)\jabber_menu.obj" \
	"$( INTDIR)\jabber_misc.obj" \
	"$( INTDIR)\jabber_opt.obj" \
	"$( INTDIR)\jabber_password.obj" \
	"$( INTDIR)\jabber_proxy.obj" \
	"$( INTDIR)\jabber_ssl.obj" \
	"$( INTDIR)\jabber_svc.obj" \
	"$( INTDIR)\jabber_thread.obj" \
	"$( INTDIR)\jabber_userinfo.obj" \
	"$( INTDIR)\jabber_util.obj" \
	"$( INTDIR)\jabber_vcard.obj" \
	"$( INTDIR)\jabber_ws.obj" \
	"$( INTDIR)\jabber_xml.obj" \
	"$( INTDIR)\jabber_xmlns.obj" \
	"$( INTDIR)\sha1.obj" \
	"$( INTDIR)\msvc6.res"

"..\..\bin\debug\plugins\jabber.dll" : "$( OUTDIR)" $( DEF_FILE) $( LINK32_OBJS)
    $( LINK32) @<<
  $( LINK32_FLAGS) $( LINK32_OBJS)
<<

!ENDIF 

.c{$( INTDIR)}.obj::
   $( CPP) @<<
   $( CPP_PROJ) $< 
<<

.cpp{$( INTDIR)}.obj::
   $( CPP) @<<
   $( CPP_PROJ) $< 
<<

.cxx{$( INTDIR)}.obj::
   $( CPP) @<<
   $( CPP_PROJ) $< 
<<

.c{$( INTDIR)}.sbr::
   $( CPP) @<<
   $( CPP_PROJ) $< 
<<

.cpp{$( INTDIR)}.sbr::
   $( CPP) @<<
   $( CPP_PROJ) $< 
<<

.cxx{$( INTDIR)}.sbr::
   $( CPP) @<<
   $( CPP_PROJ) $< 
<<


!IF "$( NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS( "jabber.dep")
!INCLUDE "jabber.dep"
!ELSE 
!MESSAGE Warning: cannot find "jabber.dep"
!ENDIF 
!ENDIF 


!IF "$( CFG)" == "jabberg - Win32 Release" || "$( CFG)" == "jabberg - Win32 Debug"
SOURCE=.\jabber.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yc"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber.obj"	"$( INTDIR)\jabber.pch" : $( SOURCE) "$( INTDIR)"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yc"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber.obj"	"$( INTDIR)\jabber.sbr"	"$( INTDIR)\jabber.pch" : $( SOURCE) "$( INTDIR)"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_agent.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_agent.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_agent.obj"	"$( INTDIR)\jabber_agent.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_byte.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_byte.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_byte.obj"	"$( INTDIR)\jabber_byte.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_file.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_file.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_file.obj"	"$( INTDIR)\jabber_file.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_form.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_form.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_form.obj"	"$( INTDIR)\jabber_form.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_ft.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_ft.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_ft.obj"	"$( INTDIR)\jabber_ft.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_groupchat.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_groupchat.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_groupchat.obj"	"$( INTDIR)\jabber_groupchat.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_groupchatlog.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_groupchatlog.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_groupchatlog.obj"	"$( INTDIR)\jabber_groupchatlog.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_iq.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_iq.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_iq.obj"	"$( INTDIR)\jabber_iq.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_iqid.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_iqid.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_iqid.obj"	"$( INTDIR)\jabber_iqid.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_iqid_muc.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_iqid_muc.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_iqid_muc.obj"	"$( INTDIR)\jabber_iqid_muc.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_list.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_list.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_list.obj"	"$( INTDIR)\jabber_list.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_menu.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_menu.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_menu.obj"	"$( INTDIR)\jabber_menu.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_misc.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_misc.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_misc.obj"	"$( INTDIR)\jabber_misc.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_opt.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_opt.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_opt.obj"	"$( INTDIR)\jabber_opt.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_password.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_password.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_password.obj"	"$( INTDIR)\jabber_password.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_proxy.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_proxy.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_proxy.obj"	"$( INTDIR)\jabber_proxy.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_ssl.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_ssl.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_ssl.obj"	"$( INTDIR)\jabber_ssl.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_svc.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_svc.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_svc.obj"	"$( INTDIR)\jabber_svc.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_thread.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_thread.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_thread.obj"	"$( INTDIR)\jabber_thread.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_userinfo.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_userinfo.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_userinfo.obj"	"$( INTDIR)\jabber_userinfo.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_util.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_util.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_util.obj"	"$( INTDIR)\jabber_util.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_vcard.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_vcard.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_vcard.obj"	"$( INTDIR)\jabber_vcard.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_ws.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_ws.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_ws.obj"	"$( INTDIR)\jabber_ws.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_xml.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_xml.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_xml.obj"	"$( INTDIR)\jabber_xml.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\jabber_xmlns.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\jabber_xmlns.obj" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fp"$( INTDIR)\jabber.pch" /Yu"jabber.h" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\jabber_xmlns.obj"	"$( INTDIR)\jabber_xmlns.sbr" : $( SOURCE) "$( INTDIR)" "$( INTDIR)\jabber.pch"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\sha1.cpp

!IF  "$( CFG)" == "jabberg - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /c 

"$( INTDIR)\sha1.obj" : $( SOURCE) "$( INTDIR)"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ELSEIF  "$( CFG)" == "jabberg - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JABBER_EXPORTS" /FAcs /Fa"$( INTDIR)\\" /FR"$( INTDIR)\\" /Fo"$( INTDIR)\\" /Fd"$( INTDIR)\\" /FD /GZ /c 

"$( INTDIR)\sha1.obj"	"$( INTDIR)\sha1.sbr" : $( SOURCE) "$( INTDIR)"
	$( CPP) @<<
  $( CPP_SWITCHES) $( SOURCE)
<<


!ENDIF 

SOURCE=.\msvc6.rc

"$( INTDIR)\msvc6.res" : $( SOURCE) "$( INTDIR)"
	$( RSC) $( RSC_PROJ) $( SOURCE)



!ENDIF 

