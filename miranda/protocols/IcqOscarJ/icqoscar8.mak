# Microsoft Developer Studio Generated NMAKE File, Based on icqoscar8.dsp
!IF "$(CFG)" == ""
CFG=icqoscar8 - Win32 Debug
!MESSAGE No configuration specified. Defaulting to icqoscar8 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "icqoscar8 - Win32 Release" && "$(CFG)" != "icqoscar8 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "icqoscar8.mak" CFG="icqoscar8 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "icqoscar8 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "icqoscar8 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "icqoscar8 - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "..\..\bin\release\plugins\ICQ.dll" "$(OUTDIR)\icqoscar8.bsc"


CLEAN :
	-@erase "$(INTDIR)\askauthentication.obj"
	-@erase "$(INTDIR)\askauthentication.sbr"
	-@erase "$(INTDIR)\capabilities.obj"
	-@erase "$(INTDIR)\capabilities.sbr"
	-@erase "$(INTDIR)\chan_01login.obj"
	-@erase "$(INTDIR)\chan_01login.sbr"
	-@erase "$(INTDIR)\chan_02data.obj"
	-@erase "$(INTDIR)\chan_02data.sbr"
	-@erase "$(INTDIR)\chan_03error.obj"
	-@erase "$(INTDIR)\chan_03error.sbr"
	-@erase "$(INTDIR)\chan_04close.obj"
	-@erase "$(INTDIR)\chan_04close.sbr"
	-@erase "$(INTDIR)\chan_05ping.obj"
	-@erase "$(INTDIR)\chan_05ping.sbr"
	-@erase "$(INTDIR)\constants.obj"
	-@erase "$(INTDIR)\constants.sbr"
	-@erase "$(INTDIR)\db.obj"
	-@erase "$(INTDIR)\db.sbr"
	-@erase "$(INTDIR)\directpackets.obj"
	-@erase "$(INTDIR)\directpackets.sbr"
	-@erase "$(INTDIR)\dlgproc.obj"
	-@erase "$(INTDIR)\dlgproc.sbr"
	-@erase "$(INTDIR)\editlist.obj"
	-@erase "$(INTDIR)\editlist.sbr"
	-@erase "$(INTDIR)\editstring.obj"
	-@erase "$(INTDIR)\editstring.sbr"
	-@erase "$(INTDIR)\fam_01service.obj"
	-@erase "$(INTDIR)\fam_01service.sbr"
	-@erase "$(INTDIR)\fam_02location.obj"
	-@erase "$(INTDIR)\fam_02location.sbr"
	-@erase "$(INTDIR)\fam_03buddy.obj"
	-@erase "$(INTDIR)\fam_03buddy.sbr"
	-@erase "$(INTDIR)\fam_04message.obj"
	-@erase "$(INTDIR)\fam_04message.sbr"
	-@erase "$(INTDIR)\fam_09bos.obj"
	-@erase "$(INTDIR)\fam_09bos.sbr"
	-@erase "$(INTDIR)\fam_0alookup.obj"
	-@erase "$(INTDIR)\fam_0alookup.sbr"
	-@erase "$(INTDIR)\fam_0bstatus.obj"
	-@erase "$(INTDIR)\fam_0bstatus.sbr"
	-@erase "$(INTDIR)\fam_13servclist.obj"
	-@erase "$(INTDIR)\fam_13servclist.sbr"
	-@erase "$(INTDIR)\fam_15icqserver.obj"
	-@erase "$(INTDIR)\fam_15icqserver.sbr"
	-@erase "$(INTDIR)\forkthread.obj"
	-@erase "$(INTDIR)\forkthread.sbr"
	-@erase "$(INTDIR)\i18n.obj"
	-@erase "$(INTDIR)\i18n.sbr"
	-@erase "$(INTDIR)\icq_advsearch.obj"
	-@erase "$(INTDIR)\icq_advsearch.sbr"
	-@erase "$(INTDIR)\icq_avatar.obj"
	-@erase "$(INTDIR)\icq_avatar.sbr"
	-@erase "$(INTDIR)\icq_clients.obj"
	-@erase "$(INTDIR)\icq_clients.sbr"
	-@erase "$(INTDIR)\icq_db.obj"
	-@erase "$(INTDIR)\icq_db.sbr"
	-@erase "$(INTDIR)\icq_direct.obj"
	-@erase "$(INTDIR)\icq_direct.sbr"
	-@erase "$(INTDIR)\icq_directmsg.obj"
	-@erase "$(INTDIR)\icq_directmsg.sbr"
	-@erase "$(INTDIR)\icq_fieldnames.obj"
	-@erase "$(INTDIR)\icq_fieldnames.sbr"
	-@erase "$(INTDIR)\icq_filerequests.obj"
	-@erase "$(INTDIR)\icq_filerequests.sbr"
	-@erase "$(INTDIR)\icq_filetransfer.obj"
	-@erase "$(INTDIR)\icq_filetransfer.sbr"
	-@erase "$(INTDIR)\icq_firstrun.obj"
	-@erase "$(INTDIR)\icq_firstrun.sbr"
	-@erase "$(INTDIR)\icq_http.obj"
	-@erase "$(INTDIR)\icq_http.sbr"
	-@erase "$(INTDIR)\icq_infoupdate.obj"
	-@erase "$(INTDIR)\icq_infoupdate.sbr"
	-@erase "$(INTDIR)\icq_opts.obj"
	-@erase "$(INTDIR)\icq_opts.sbr"
	-@erase "$(INTDIR)\icq_packet.obj"
	-@erase "$(INTDIR)\icq_packet.sbr"
	-@erase "$(INTDIR)\icq_popups.obj"
	-@erase "$(INTDIR)\icq_popups.sbr"
	-@erase "$(INTDIR)\icq_server.obj"
	-@erase "$(INTDIR)\icq_server.sbr"
	-@erase "$(INTDIR)\icq_servlist.obj"
	-@erase "$(INTDIR)\icq_servlist.sbr"
	-@erase "$(INTDIR)\icq_uploadui.obj"
	-@erase "$(INTDIR)\icq_uploadui.sbr"
	-@erase "$(INTDIR)\icq_xstatus.obj"
	-@erase "$(INTDIR)\icq_xstatus.sbr"
	-@erase "$(INTDIR)\icq_xtraz.obj"
	-@erase "$(INTDIR)\icq_xtraz.sbr"
	-@erase "$(INTDIR)\icqosc_svcs.obj"
	-@erase "$(INTDIR)\icqosc_svcs.sbr"
	-@erase "$(INTDIR)\icqoscar.obj"
	-@erase "$(INTDIR)\icqoscar.sbr"
	-@erase "$(INTDIR)\icqoscar8.pch"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\init.sbr"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\log.sbr"
	-@erase "$(INTDIR)\loginpassword.obj"
	-@erase "$(INTDIR)\loginpassword.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\md5.sbr"
	-@erase "$(INTDIR)\resources.res"
	-@erase "$(INTDIR)\stdpackets.obj"
	-@erase "$(INTDIR)\stdpackets.sbr"
	-@erase "$(INTDIR)\tlv.obj"
	-@erase "$(INTDIR)\tlv.sbr"
	-@erase "$(INTDIR)\upload.obj"
	-@erase "$(INTDIR)\upload.sbr"
	-@erase "$(INTDIR)\userinfotab.obj"
	-@erase "$(INTDIR)\userinfotab.sbr"
	-@erase "$(INTDIR)\utilities.obj"
	-@erase "$(INTDIR)\utilities.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\ICQ.exp"
	-@erase "$(OUTDIR)\ICQ.lib"
	-@erase "$(OUTDIR)\ICQ.map"
	-@erase "$(OUTDIR)\ICQ.pdb"
	-@erase "$(OUTDIR)\icqoscar8.bsc"
	-@erase "..\..\bin\release\plugins\ICQ.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "icqoscar8_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\icqoscar8.pch" /Yu"icqoscar.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x417 /fo"$(INTDIR)\resources.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\icqoscar8.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\chan_01login.sbr" \
	"$(INTDIR)\chan_02data.sbr" \
	"$(INTDIR)\chan_03error.sbr" \
	"$(INTDIR)\chan_04close.sbr" \
	"$(INTDIR)\chan_05ping.sbr" \
	"$(INTDIR)\fam_01service.sbr" \
	"$(INTDIR)\fam_02location.sbr" \
	"$(INTDIR)\fam_03buddy.sbr" \
	"$(INTDIR)\fam_04message.sbr" \
	"$(INTDIR)\fam_09bos.sbr" \
	"$(INTDIR)\fam_0alookup.sbr" \
	"$(INTDIR)\fam_0bstatus.sbr" \
	"$(INTDIR)\fam_13servclist.sbr" \
	"$(INTDIR)\fam_15icqserver.sbr" \
	"$(INTDIR)\directpackets.sbr" \
	"$(INTDIR)\icq_direct.sbr" \
	"$(INTDIR)\icq_directmsg.sbr" \
	"$(INTDIR)\icq_filerequests.sbr" \
	"$(INTDIR)\icq_filetransfer.sbr" \
	"$(INTDIR)\icq_advsearch.sbr" \
	"$(INTDIR)\icq_db.sbr" \
	"$(INTDIR)\icqosc_svcs.sbr" \
	"$(INTDIR)\init.sbr" \
	"$(INTDIR)\log.sbr" \
	"$(INTDIR)\askauthentication.sbr" \
	"$(INTDIR)\icq_firstrun.sbr" \
	"$(INTDIR)\icq_opts.sbr" \
	"$(INTDIR)\icq_popups.sbr" \
	"$(INTDIR)\icq_uploadui.sbr" \
	"$(INTDIR)\loginpassword.sbr" \
	"$(INTDIR)\userinfotab.sbr" \
	"$(INTDIR)\constants.sbr" \
	"$(INTDIR)\db.sbr" \
	"$(INTDIR)\dlgproc.sbr" \
	"$(INTDIR)\editlist.sbr" \
	"$(INTDIR)\editstring.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\upload.sbr" \
	"$(INTDIR)\capabilities.sbr" \
	"$(INTDIR)\forkthread.sbr" \
	"$(INTDIR)\i18n.sbr" \
	"$(INTDIR)\icq_avatar.sbr" \
	"$(INTDIR)\icq_clients.sbr" \
	"$(INTDIR)\icq_fieldnames.sbr" \
	"$(INTDIR)\icq_http.sbr" \
	"$(INTDIR)\icq_infoupdate.sbr" \
	"$(INTDIR)\icq_packet.sbr" \
	"$(INTDIR)\icq_server.sbr" \
	"$(INTDIR)\icq_servlist.sbr" \
	"$(INTDIR)\icq_xstatus.sbr" \
	"$(INTDIR)\icq_xtraz.sbr" \
	"$(INTDIR)\icqoscar.sbr" \
	"$(INTDIR)\md5.sbr" \
	"$(INTDIR)\stdpackets.sbr" \
	"$(INTDIR)\tlv.sbr" \
	"$(INTDIR)\utilities.sbr"

"$(OUTDIR)\icqoscar8.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\ICQ.pdb" /map:"$(INTDIR)\ICQ.map" /debug /machine:I386 /out:"../../bin/release/plugins/ICQ.dll" /implib:"$(OUTDIR)\ICQ.lib" /ALIGN:4096 /ignore:4108 
LINK32_OBJS= \
	"$(INTDIR)\chan_01login.obj" \
	"$(INTDIR)\chan_02data.obj" \
	"$(INTDIR)\chan_03error.obj" \
	"$(INTDIR)\chan_04close.obj" \
	"$(INTDIR)\chan_05ping.obj" \
	"$(INTDIR)\fam_01service.obj" \
	"$(INTDIR)\fam_02location.obj" \
	"$(INTDIR)\fam_03buddy.obj" \
	"$(INTDIR)\fam_04message.obj" \
	"$(INTDIR)\fam_09bos.obj" \
	"$(INTDIR)\fam_0alookup.obj" \
	"$(INTDIR)\fam_0bstatus.obj" \
	"$(INTDIR)\fam_13servclist.obj" \
	"$(INTDIR)\fam_15icqserver.obj" \
	"$(INTDIR)\directpackets.obj" \
	"$(INTDIR)\icq_direct.obj" \
	"$(INTDIR)\icq_directmsg.obj" \
	"$(INTDIR)\icq_filerequests.obj" \
	"$(INTDIR)\icq_filetransfer.obj" \
	"$(INTDIR)\icq_advsearch.obj" \
	"$(INTDIR)\icq_db.obj" \
	"$(INTDIR)\icqosc_svcs.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\askauthentication.obj" \
	"$(INTDIR)\icq_firstrun.obj" \
	"$(INTDIR)\icq_opts.obj" \
	"$(INTDIR)\icq_popups.obj" \
	"$(INTDIR)\icq_uploadui.obj" \
	"$(INTDIR)\loginpassword.obj" \
	"$(INTDIR)\userinfotab.obj" \
	"$(INTDIR)\constants.obj" \
	"$(INTDIR)\db.obj" \
	"$(INTDIR)\dlgproc.obj" \
	"$(INTDIR)\editlist.obj" \
	"$(INTDIR)\editstring.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\upload.obj" \
	"$(INTDIR)\capabilities.obj" \
	"$(INTDIR)\forkthread.obj" \
	"$(INTDIR)\i18n.obj" \
	"$(INTDIR)\icq_avatar.obj" \
	"$(INTDIR)\icq_clients.obj" \
	"$(INTDIR)\icq_fieldnames.obj" \
	"$(INTDIR)\icq_http.obj" \
	"$(INTDIR)\icq_infoupdate.obj" \
	"$(INTDIR)\icq_packet.obj" \
	"$(INTDIR)\icq_server.obj" \
	"$(INTDIR)\icq_servlist.obj" \
	"$(INTDIR)\icq_xstatus.obj" \
	"$(INTDIR)\icq_xtraz.obj" \
	"$(INTDIR)\icqoscar.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\stdpackets.obj" \
	"$(INTDIR)\tlv.obj" \
	"$(INTDIR)\utilities.obj" \
	"$(INTDIR)\resources.res"

"..\..\bin\release\plugins\ICQ.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "icqoscar8 - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "..\..\bin\debug\plugins\ICQ.dll" "$(OUTDIR)\icqoscar8.bsc"


CLEAN :
	-@erase "$(INTDIR)\askauthentication.obj"
	-@erase "$(INTDIR)\askauthentication.sbr"
	-@erase "$(INTDIR)\capabilities.obj"
	-@erase "$(INTDIR)\capabilities.sbr"
	-@erase "$(INTDIR)\chan_01login.obj"
	-@erase "$(INTDIR)\chan_01login.sbr"
	-@erase "$(INTDIR)\chan_02data.obj"
	-@erase "$(INTDIR)\chan_02data.sbr"
	-@erase "$(INTDIR)\chan_03error.obj"
	-@erase "$(INTDIR)\chan_03error.sbr"
	-@erase "$(INTDIR)\chan_04close.obj"
	-@erase "$(INTDIR)\chan_04close.sbr"
	-@erase "$(INTDIR)\chan_05ping.obj"
	-@erase "$(INTDIR)\chan_05ping.sbr"
	-@erase "$(INTDIR)\constants.obj"
	-@erase "$(INTDIR)\constants.sbr"
	-@erase "$(INTDIR)\db.obj"
	-@erase "$(INTDIR)\db.sbr"
	-@erase "$(INTDIR)\directpackets.obj"
	-@erase "$(INTDIR)\directpackets.sbr"
	-@erase "$(INTDIR)\dlgproc.obj"
	-@erase "$(INTDIR)\dlgproc.sbr"
	-@erase "$(INTDIR)\editlist.obj"
	-@erase "$(INTDIR)\editlist.sbr"
	-@erase "$(INTDIR)\editstring.obj"
	-@erase "$(INTDIR)\editstring.sbr"
	-@erase "$(INTDIR)\fam_01service.obj"
	-@erase "$(INTDIR)\fam_01service.sbr"
	-@erase "$(INTDIR)\fam_02location.obj"
	-@erase "$(INTDIR)\fam_02location.sbr"
	-@erase "$(INTDIR)\fam_03buddy.obj"
	-@erase "$(INTDIR)\fam_03buddy.sbr"
	-@erase "$(INTDIR)\fam_04message.obj"
	-@erase "$(INTDIR)\fam_04message.sbr"
	-@erase "$(INTDIR)\fam_09bos.obj"
	-@erase "$(INTDIR)\fam_09bos.sbr"
	-@erase "$(INTDIR)\fam_0alookup.obj"
	-@erase "$(INTDIR)\fam_0alookup.sbr"
	-@erase "$(INTDIR)\fam_0bstatus.obj"
	-@erase "$(INTDIR)\fam_0bstatus.sbr"
	-@erase "$(INTDIR)\fam_13servclist.obj"
	-@erase "$(INTDIR)\fam_13servclist.sbr"
	-@erase "$(INTDIR)\fam_15icqserver.obj"
	-@erase "$(INTDIR)\fam_15icqserver.sbr"
	-@erase "$(INTDIR)\forkthread.obj"
	-@erase "$(INTDIR)\forkthread.sbr"
	-@erase "$(INTDIR)\i18n.obj"
	-@erase "$(INTDIR)\i18n.sbr"
	-@erase "$(INTDIR)\icq_advsearch.obj"
	-@erase "$(INTDIR)\icq_advsearch.sbr"
	-@erase "$(INTDIR)\icq_avatar.obj"
	-@erase "$(INTDIR)\icq_avatar.sbr"
	-@erase "$(INTDIR)\icq_clients.obj"
	-@erase "$(INTDIR)\icq_clients.sbr"
	-@erase "$(INTDIR)\icq_db.obj"
	-@erase "$(INTDIR)\icq_db.sbr"
	-@erase "$(INTDIR)\icq_direct.obj"
	-@erase "$(INTDIR)\icq_direct.sbr"
	-@erase "$(INTDIR)\icq_directmsg.obj"
	-@erase "$(INTDIR)\icq_directmsg.sbr"
	-@erase "$(INTDIR)\icq_fieldnames.obj"
	-@erase "$(INTDIR)\icq_fieldnames.sbr"
	-@erase "$(INTDIR)\icq_filerequests.obj"
	-@erase "$(INTDIR)\icq_filerequests.sbr"
	-@erase "$(INTDIR)\icq_filetransfer.obj"
	-@erase "$(INTDIR)\icq_filetransfer.sbr"
	-@erase "$(INTDIR)\icq_firstrun.obj"
	-@erase "$(INTDIR)\icq_firstrun.sbr"
	-@erase "$(INTDIR)\icq_http.obj"
	-@erase "$(INTDIR)\icq_http.sbr"
	-@erase "$(INTDIR)\icq_infoupdate.obj"
	-@erase "$(INTDIR)\icq_infoupdate.sbr"
	-@erase "$(INTDIR)\icq_opts.obj"
	-@erase "$(INTDIR)\icq_opts.sbr"
	-@erase "$(INTDIR)\icq_packet.obj"
	-@erase "$(INTDIR)\icq_packet.sbr"
	-@erase "$(INTDIR)\icq_popups.obj"
	-@erase "$(INTDIR)\icq_popups.sbr"
	-@erase "$(INTDIR)\icq_server.obj"
	-@erase "$(INTDIR)\icq_server.sbr"
	-@erase "$(INTDIR)\icq_servlist.obj"
	-@erase "$(INTDIR)\icq_servlist.sbr"
	-@erase "$(INTDIR)\icq_uploadui.obj"
	-@erase "$(INTDIR)\icq_uploadui.sbr"
	-@erase "$(INTDIR)\icq_xstatus.obj"
	-@erase "$(INTDIR)\icq_xstatus.sbr"
	-@erase "$(INTDIR)\icq_xtraz.obj"
	-@erase "$(INTDIR)\icq_xtraz.sbr"
	-@erase "$(INTDIR)\icqosc_svcs.obj"
	-@erase "$(INTDIR)\icqosc_svcs.sbr"
	-@erase "$(INTDIR)\icqoscar.obj"
	-@erase "$(INTDIR)\icqoscar.sbr"
	-@erase "$(INTDIR)\icqoscar8.pch"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\init.sbr"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\log.sbr"
	-@erase "$(INTDIR)\loginpassword.obj"
	-@erase "$(INTDIR)\loginpassword.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\md5.sbr"
	-@erase "$(INTDIR)\resources.res"
	-@erase "$(INTDIR)\stdpackets.obj"
	-@erase "$(INTDIR)\stdpackets.sbr"
	-@erase "$(INTDIR)\tlv.obj"
	-@erase "$(INTDIR)\tlv.sbr"
	-@erase "$(INTDIR)\upload.obj"
	-@erase "$(INTDIR)\upload.sbr"
	-@erase "$(INTDIR)\userinfotab.obj"
	-@erase "$(INTDIR)\userinfotab.sbr"
	-@erase "$(INTDIR)\utilities.obj"
	-@erase "$(INTDIR)\utilities.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\ICQ.exp"
	-@erase "$(OUTDIR)\ICQ.lib"
	-@erase "$(OUTDIR)\ICQ.map"
	-@erase "$(OUTDIR)\ICQ.pdb"
	-@erase "$(OUTDIR)\icqoscar8.bsc"
	-@erase "..\..\bin\debug\plugins\ICQ.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "icqoscar8_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\icqoscar8.pch" /Yu"icqoscar.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resources.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\icqoscar8.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\chan_01login.sbr" \
	"$(INTDIR)\chan_02data.sbr" \
	"$(INTDIR)\chan_03error.sbr" \
	"$(INTDIR)\chan_04close.sbr" \
	"$(INTDIR)\chan_05ping.sbr" \
	"$(INTDIR)\fam_01service.sbr" \
	"$(INTDIR)\fam_02location.sbr" \
	"$(INTDIR)\fam_03buddy.sbr" \
	"$(INTDIR)\fam_04message.sbr" \
	"$(INTDIR)\fam_09bos.sbr" \
	"$(INTDIR)\fam_0alookup.sbr" \
	"$(INTDIR)\fam_0bstatus.sbr" \
	"$(INTDIR)\fam_13servclist.sbr" \
	"$(INTDIR)\fam_15icqserver.sbr" \
	"$(INTDIR)\directpackets.sbr" \
	"$(INTDIR)\icq_direct.sbr" \
	"$(INTDIR)\icq_directmsg.sbr" \
	"$(INTDIR)\icq_filerequests.sbr" \
	"$(INTDIR)\icq_filetransfer.sbr" \
	"$(INTDIR)\icq_advsearch.sbr" \
	"$(INTDIR)\icq_db.sbr" \
	"$(INTDIR)\icqosc_svcs.sbr" \
	"$(INTDIR)\init.sbr" \
	"$(INTDIR)\log.sbr" \
	"$(INTDIR)\askauthentication.sbr" \
	"$(INTDIR)\icq_firstrun.sbr" \
	"$(INTDIR)\icq_opts.sbr" \
	"$(INTDIR)\icq_popups.sbr" \
	"$(INTDIR)\icq_uploadui.sbr" \
	"$(INTDIR)\loginpassword.sbr" \
	"$(INTDIR)\userinfotab.sbr" \
	"$(INTDIR)\constants.sbr" \
	"$(INTDIR)\db.sbr" \
	"$(INTDIR)\dlgproc.sbr" \
	"$(INTDIR)\editlist.sbr" \
	"$(INTDIR)\editstring.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\upload.sbr" \
	"$(INTDIR)\capabilities.sbr" \
	"$(INTDIR)\forkthread.sbr" \
	"$(INTDIR)\i18n.sbr" \
	"$(INTDIR)\icq_avatar.sbr" \
	"$(INTDIR)\icq_clients.sbr" \
	"$(INTDIR)\icq_fieldnames.sbr" \
	"$(INTDIR)\icq_http.sbr" \
	"$(INTDIR)\icq_infoupdate.sbr" \
	"$(INTDIR)\icq_packet.sbr" \
	"$(INTDIR)\icq_server.sbr" \
	"$(INTDIR)\icq_servlist.sbr" \
	"$(INTDIR)\icq_xstatus.sbr" \
	"$(INTDIR)\icq_xtraz.sbr" \
	"$(INTDIR)\icqoscar.sbr" \
	"$(INTDIR)\md5.sbr" \
	"$(INTDIR)\stdpackets.sbr" \
	"$(INTDIR)\tlv.sbr" \
	"$(INTDIR)\utilities.sbr"

"$(OUTDIR)\icqoscar8.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x25000000" /dll /incremental:no /pdb:"$(OUTDIR)\ICQ.pdb" /map:"$(INTDIR)\ICQ.map" /debug /debugtype:both /machine:I386 /out:"../../bin/debug/plugins/ICQ.dll" /implib:"$(OUTDIR)\ICQ.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\chan_01login.obj" \
	"$(INTDIR)\chan_02data.obj" \
	"$(INTDIR)\chan_03error.obj" \
	"$(INTDIR)\chan_04close.obj" \
	"$(INTDIR)\chan_05ping.obj" \
	"$(INTDIR)\fam_01service.obj" \
	"$(INTDIR)\fam_02location.obj" \
	"$(INTDIR)\fam_03buddy.obj" \
	"$(INTDIR)\fam_04message.obj" \
	"$(INTDIR)\fam_09bos.obj" \
	"$(INTDIR)\fam_0alookup.obj" \
	"$(INTDIR)\fam_0bstatus.obj" \
	"$(INTDIR)\fam_13servclist.obj" \
	"$(INTDIR)\fam_15icqserver.obj" \
	"$(INTDIR)\directpackets.obj" \
	"$(INTDIR)\icq_direct.obj" \
	"$(INTDIR)\icq_directmsg.obj" \
	"$(INTDIR)\icq_filerequests.obj" \
	"$(INTDIR)\icq_filetransfer.obj" \
	"$(INTDIR)\icq_advsearch.obj" \
	"$(INTDIR)\icq_db.obj" \
	"$(INTDIR)\icqosc_svcs.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\askauthentication.obj" \
	"$(INTDIR)\icq_firstrun.obj" \
	"$(INTDIR)\icq_opts.obj" \
	"$(INTDIR)\icq_popups.obj" \
	"$(INTDIR)\icq_uploadui.obj" \
	"$(INTDIR)\loginpassword.obj" \
	"$(INTDIR)\userinfotab.obj" \
	"$(INTDIR)\constants.obj" \
	"$(INTDIR)\db.obj" \
	"$(INTDIR)\dlgproc.obj" \
	"$(INTDIR)\editlist.obj" \
	"$(INTDIR)\editstring.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\upload.obj" \
	"$(INTDIR)\capabilities.obj" \
	"$(INTDIR)\forkthread.obj" \
	"$(INTDIR)\i18n.obj" \
	"$(INTDIR)\icq_avatar.obj" \
	"$(INTDIR)\icq_clients.obj" \
	"$(INTDIR)\icq_fieldnames.obj" \
	"$(INTDIR)\icq_http.obj" \
	"$(INTDIR)\icq_infoupdate.obj" \
	"$(INTDIR)\icq_packet.obj" \
	"$(INTDIR)\icq_server.obj" \
	"$(INTDIR)\icq_servlist.obj" \
	"$(INTDIR)\icq_xstatus.obj" \
	"$(INTDIR)\icq_xtraz.obj" \
	"$(INTDIR)\icqoscar.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\stdpackets.obj" \
	"$(INTDIR)\tlv.obj" \
	"$(INTDIR)\utilities.obj" \
	"$(INTDIR)\resources.res"

"..\..\bin\debug\plugins\ICQ.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("icqoscar8.dep")
!INCLUDE "icqoscar8.dep"
!ELSE 
!MESSAGE Warning: cannot find "icqoscar8.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "icqoscar8 - Win32 Release" || "$(CFG)" == "icqoscar8 - Win32 Debug"
SOURCE=.\resources.rc

"$(INTDIR)\resources.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\chan_01login.c

"$(INTDIR)\chan_01login.obj"	"$(INTDIR)\chan_01login.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\chan_02data.c

"$(INTDIR)\chan_02data.obj"	"$(INTDIR)\chan_02data.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\chan_03error.c

"$(INTDIR)\chan_03error.obj"	"$(INTDIR)\chan_03error.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\chan_04close.c

"$(INTDIR)\chan_04close.obj"	"$(INTDIR)\chan_04close.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\chan_05ping.c

"$(INTDIR)\chan_05ping.obj"	"$(INTDIR)\chan_05ping.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\fam_01service.c

"$(INTDIR)\fam_01service.obj"	"$(INTDIR)\fam_01service.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\fam_02location.c

"$(INTDIR)\fam_02location.obj"	"$(INTDIR)\fam_02location.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\fam_03buddy.c

"$(INTDIR)\fam_03buddy.obj"	"$(INTDIR)\fam_03buddy.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\fam_04message.c

"$(INTDIR)\fam_04message.obj"	"$(INTDIR)\fam_04message.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\fam_09bos.c

"$(INTDIR)\fam_09bos.obj"	"$(INTDIR)\fam_09bos.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\fam_0alookup.c

"$(INTDIR)\fam_0alookup.obj"	"$(INTDIR)\fam_0alookup.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\fam_0bstatus.c

"$(INTDIR)\fam_0bstatus.obj"	"$(INTDIR)\fam_0bstatus.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\fam_13servclist.c

"$(INTDIR)\fam_13servclist.obj"	"$(INTDIR)\fam_13servclist.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\fam_15icqserver.c

"$(INTDIR)\fam_15icqserver.obj"	"$(INTDIR)\fam_15icqserver.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\directpackets.c

"$(INTDIR)\directpackets.obj"	"$(INTDIR)\directpackets.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_direct.c

"$(INTDIR)\icq_direct.obj"	"$(INTDIR)\icq_direct.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_directmsg.c

"$(INTDIR)\icq_directmsg.obj"	"$(INTDIR)\icq_directmsg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_filerequests.c

"$(INTDIR)\icq_filerequests.obj"	"$(INTDIR)\icq_filerequests.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_filetransfer.c

"$(INTDIR)\icq_filetransfer.obj"	"$(INTDIR)\icq_filetransfer.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_advsearch.c

"$(INTDIR)\icq_advsearch.obj"	"$(INTDIR)\icq_advsearch.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_db.c

"$(INTDIR)\icq_db.obj"	"$(INTDIR)\icq_db.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icqosc_svcs.c

"$(INTDIR)\icqosc_svcs.obj"	"$(INTDIR)\icqosc_svcs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\init.c

"$(INTDIR)\init.obj"	"$(INTDIR)\init.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\log.c

"$(INTDIR)\log.obj"	"$(INTDIR)\log.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\UI\askauthentication.c

"$(INTDIR)\askauthentication.obj"	"$(INTDIR)\askauthentication.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\icq_firstrun.c

"$(INTDIR)\icq_firstrun.obj"	"$(INTDIR)\icq_firstrun.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_opts.c

"$(INTDIR)\icq_opts.obj"	"$(INTDIR)\icq_opts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_popups.c

"$(INTDIR)\icq_popups.obj"	"$(INTDIR)\icq_popups.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_uploadui.c

"$(INTDIR)\icq_uploadui.obj"	"$(INTDIR)\icq_uploadui.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\UI\loginpassword.c

"$(INTDIR)\loginpassword.obj"	"$(INTDIR)\loginpassword.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\UI\userinfotab.c

"$(INTDIR)\userinfotab.obj"	"$(INTDIR)\userinfotab.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\changeinfo\constants.c

"$(INTDIR)\constants.obj"	"$(INTDIR)\constants.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\changeinfo\db.c

"$(INTDIR)\db.obj"	"$(INTDIR)\db.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\changeinfo\dlgproc.c

"$(INTDIR)\dlgproc.obj"	"$(INTDIR)\dlgproc.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\changeinfo\editlist.c

"$(INTDIR)\editlist.obj"	"$(INTDIR)\editlist.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\changeinfo\editstring.c

"$(INTDIR)\editstring.obj"	"$(INTDIR)\editstring.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\changeinfo\main.c

"$(INTDIR)\main.obj"	"$(INTDIR)\main.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\changeinfo\upload.c

"$(INTDIR)\upload.obj"	"$(INTDIR)\upload.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\capabilities.c

"$(INTDIR)\capabilities.obj"	"$(INTDIR)\capabilities.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\forkthread.c

"$(INTDIR)\forkthread.obj"	"$(INTDIR)\forkthread.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\i18n.c

"$(INTDIR)\i18n.obj"	"$(INTDIR)\i18n.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_avatar.c

"$(INTDIR)\icq_avatar.obj"	"$(INTDIR)\icq_avatar.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_clients.c

"$(INTDIR)\icq_clients.obj"	"$(INTDIR)\icq_clients.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_fieldnames.c

"$(INTDIR)\icq_fieldnames.obj"	"$(INTDIR)\icq_fieldnames.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_http.c

"$(INTDIR)\icq_http.obj"	"$(INTDIR)\icq_http.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_infoupdate.c

"$(INTDIR)\icq_infoupdate.obj"	"$(INTDIR)\icq_infoupdate.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_packet.c

"$(INTDIR)\icq_packet.obj"	"$(INTDIR)\icq_packet.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_server.c

"$(INTDIR)\icq_server.obj"	"$(INTDIR)\icq_server.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_servlist.c

"$(INTDIR)\icq_servlist.obj"	"$(INTDIR)\icq_servlist.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_xstatus.c

"$(INTDIR)\icq_xstatus.obj"	"$(INTDIR)\icq_xstatus.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icq_xtraz.c

"$(INTDIR)\icq_xtraz.obj"	"$(INTDIR)\icq_xtraz.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\icqoscar.c

!IF  "$(CFG)" == "icqoscar8 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O1 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "icqoscar8_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\icqoscar8.pch" /Yc"icqoscar.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\icqoscar.obj"	"$(INTDIR)\icqoscar.sbr"	"$(INTDIR)\icqoscar8.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "icqoscar8 - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "icqoscar8_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\icqoscar8.pch" /Yc"icqoscar.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\icqoscar.obj"	"$(INTDIR)\icqoscar.sbr"	"$(INTDIR)\icqoscar8.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\md5.c

"$(INTDIR)\md5.obj"	"$(INTDIR)\md5.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\stdpackets.c

"$(INTDIR)\stdpackets.obj"	"$(INTDIR)\stdpackets.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\tlv.c

"$(INTDIR)\tlv.obj"	"$(INTDIR)\tlv.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"


SOURCE=.\utilities.c

"$(INTDIR)\utilities.obj"	"$(INTDIR)\utilities.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\icqoscar8.pch"



!ENDIF 

