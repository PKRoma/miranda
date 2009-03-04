# Microsoft Developer Studio Generated NMAKE File, Based on miranda32.dsp
!IF "$(CFG)" == ""
CFG=miranda32 - Win32 Release Unicode
!MESSAGE No configuration specified. Defaulting to miranda32 - Win32 Release Unicode.
!ENDIF 

!IF "$(CFG)" != "miranda32 - Win32 Release" && "$(CFG)" != "miranda32 - Win32 Debug" && "$(CFG)" != "miranda32 - Win32 Release Unicode" && "$(CFG)" != "miranda32 - Win32 Debug Unicode"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "miranda32.mak" CFG="miranda32 - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "miranda32 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "miranda32 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "miranda32 - Win32 Release Unicode" (based on "Win32 (x86) Application")
!MESSAGE "miranda32 - Win32 Debug Unicode" (based on "Win32 (x86) Application")
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

!IF  "$(CFG)" == "miranda32 - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

!IF "$(RECURSE)" == "0" 

ALL : "..\bin\release\miranda32.exe"

!ELSE 

ALL : "zlib - Win32 Release" "..\bin\release\miranda32.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"zlib - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\about.obj"
	-@erase "$(INTDIR)\addcontact.obj"
	-@erase "$(INTDIR)\auth.obj"
	-@erase "$(INTDIR)\authdialogs.obj"
	-@erase "$(INTDIR)\autoaway.obj"
	-@erase "$(INTDIR)\awaymsg.obj"
	-@erase "$(INTDIR)\bmpfilter.obj"
	-@erase "$(INTDIR)\button.obj"
	-@erase "$(INTDIR)\clc.obj"
	-@erase "$(INTDIR)\clcfiledrop.obj"
	-@erase "$(INTDIR)\clcidents.obj"
	-@erase "$(INTDIR)\clcitems.obj"
	-@erase "$(INTDIR)\clcmsgs.obj"
	-@erase "$(INTDIR)\clcutils.obj"
	-@erase "$(INTDIR)\clistcore.obj"
	-@erase "$(INTDIR)\clistevents.obj"
	-@erase "$(INTDIR)\clistmenus.obj"
	-@erase "$(INTDIR)\clistmod.obj"
	-@erase "$(INTDIR)\clistsettings.obj"
	-@erase "$(INTDIR)\clisttray.obj"
	-@erase "$(INTDIR)\clui.obj"
	-@erase "$(INTDIR)\cluiservices.obj"
	-@erase "$(INTDIR)\colourpicker.obj"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\contact.obj"
	-@erase "$(INTDIR)\contactinfo.obj"
	-@erase "$(INTDIR)\contacts.obj"
	-@erase "$(INTDIR)\database.obj"
	-@erase "$(INTDIR)\dbini.obj"
	-@erase "$(INTDIR)\dblists.obj"
	-@erase "$(INTDIR)\dbtime.obj"
	-@erase "$(INTDIR)\dbutils.obj"
	-@erase "$(INTDIR)\Docking.obj"
	-@erase "$(INTDIR)\email.obj"
	-@erase "$(INTDIR)\extracticon.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\fileexistsdlg.obj"
	-@erase "$(INTDIR)\fileopts.obj"
	-@erase "$(INTDIR)\filerecvdlg.obj"
	-@erase "$(INTDIR)\filesenddlg.obj"
	-@erase "$(INTDIR)\filexferdlg.obj"
	-@erase "$(INTDIR)\filter.obj"
	-@erase "$(INTDIR)\findadd.obj"
	-@erase "$(INTDIR)\FontOptions.obj"
	-@erase "$(INTDIR)\FontService.obj"
	-@erase "$(INTDIR)\ftmanager.obj"
	-@erase "$(INTDIR)\genmenu.obj"
	-@erase "$(INTDIR)\genmenuopt.obj"
	-@erase "$(INTDIR)\groups.obj"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\history.obj"
	-@erase "$(INTDIR)\hotkeys.obj"
	-@erase "$(INTDIR)\hyperlink.obj"
	-@erase "$(INTDIR)\idle.obj"
	-@erase "$(INTDIR)\ignore.obj"
	-@erase "$(INTDIR)\keyboard.obj"
	-@erase "$(INTDIR)\langpack.obj"
	-@erase "$(INTDIR)\lpservices.obj"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\memory.obj"
	-@erase "$(INTDIR)\miranda.obj"
	-@erase "$(INTDIR)\miranda32.pch"
	-@erase "$(INTDIR)\modules.obj"
	-@erase "$(INTDIR)\movetogroup.obj"
	-@erase "$(INTDIR)\netlib.obj"
	-@erase "$(INTDIR)\netlibbind.obj"
	-@erase "$(INTDIR)\netlibhttp.obj"
	-@erase "$(INTDIR)\netlibhttpproxy.obj"
	-@erase "$(INTDIR)\netliblog.obj"
	-@erase "$(INTDIR)\netlibopenconn.obj"
	-@erase "$(INTDIR)\netlibopts.obj"
	-@erase "$(INTDIR)\netlibpktrecver.obj"
	-@erase "$(INTDIR)\netlibsecurity.obj"
	-@erase "$(INTDIR)\netlibsock.obj"
	-@erase "$(INTDIR)\netlibssl.obj"
	-@erase "$(INTDIR)\netlibupnp.obj"
	-@erase "$(INTDIR)\newplugins.obj"
	-@erase "$(INTDIR)\openurl.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\path.obj"
	-@erase "$(INTDIR)\profilemanager.obj"
	-@erase "$(INTDIR)\protoaccs.obj"
	-@erase "$(INTDIR)\protochains.obj"
	-@erase "$(INTDIR)\protocolorder.obj"
	-@erase "$(INTDIR)\protocols.obj"
	-@erase "$(INTDIR)\protoint.obj"
	-@erase "$(INTDIR)\protoopts.obj"
	-@erase "$(INTDIR)\resizer.obj"
	-@erase "$(INTDIR)\searchresults.obj"
	-@erase "$(INTDIR)\sendmsg.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\sha1.obj"
	-@erase "$(INTDIR)\skin2icons.obj"
	-@erase "$(INTDIR)\skinicons.obj"
	-@erase "$(INTDIR)\sounds.obj"
	-@erase "$(INTDIR)\stdinfo.obj"
	-@erase "$(INTDIR)\updatenotify.obj"
	-@erase "$(INTDIR)\url.obj"
	-@erase "$(INTDIR)\urldialogs.obj"
	-@erase "$(INTDIR)\userinfo.obj"
	-@erase "$(INTDIR)\useronline.obj"
	-@erase "$(INTDIR)\utf.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\vc6.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\visibility.obj"
	-@erase "$(INTDIR)\windowlist.obj"
	-@erase "$(INTDIR)\xmlApi.obj"
	-@erase "$(INTDIR)\xmlParser.obj"
	-@erase "$(OUTDIR)\miranda32.map"
	-@erase "$(OUTDIR)\miranda32.pdb"
	-@erase "..\bin\release\miranda32.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\vc6.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\miranda32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib shell32.lib winspool.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib version.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\miranda32.pdb" /map:"$(INTDIR)\miranda32.map" /debug /machine:I386 /out:"../bin/release/miranda32.exe" /fixed /ALIGN:4096 /ignore:4108 
LINK32_OBJS= \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\memory.obj" \
	"$(INTDIR)\miranda.obj" \
	"$(INTDIR)\modules.obj" \
	"$(INTDIR)\addcontact.obj" \
	"$(INTDIR)\autoaway.obj" \
	"$(INTDIR)\button.obj" \
	"$(INTDIR)\contacts.obj" \
	"$(INTDIR)\database.obj" \
	"$(INTDIR)\dbini.obj" \
	"$(INTDIR)\dblists.obj" \
	"$(INTDIR)\dbtime.obj" \
	"$(INTDIR)\dbutils.obj" \
	"$(INTDIR)\profilemanager.obj" \
	"$(INTDIR)\findadd.obj" \
	"$(INTDIR)\searchresults.obj" \
	"$(INTDIR)\about.obj" \
	"$(INTDIR)\help.obj" \
	"$(INTDIR)\history.obj" \
	"$(INTDIR)\idle.obj" \
	"$(INTDIR)\ignore.obj" \
	"$(INTDIR)\langpack.obj" \
	"$(INTDIR)\lpservices.obj" \
	"$(INTDIR)\netlib.obj" \
	"$(INTDIR)\netlibbind.obj" \
	"$(INTDIR)\netlibhttp.obj" \
	"$(INTDIR)\netlibhttpproxy.obj" \
	"$(INTDIR)\netliblog.obj" \
	"$(INTDIR)\netlibopenconn.obj" \
	"$(INTDIR)\netlibopts.obj" \
	"$(INTDIR)\netlibpktrecver.obj" \
	"$(INTDIR)\netlibsecurity.obj" \
	"$(INTDIR)\netlibsock.obj" \
	"$(INTDIR)\netlibssl.obj" \
	"$(INTDIR)\netlibupnp.obj" \
	"$(INTDIR)\filter.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\newplugins.obj" \
	"$(INTDIR)\protoaccs.obj" \
	"$(INTDIR)\protochains.obj" \
	"$(INTDIR)\protocols.obj" \
	"$(INTDIR)\protoint.obj" \
	"$(INTDIR)\protoopts.obj" \
	"$(INTDIR)\hotkeys.obj" \
	"$(INTDIR)\skinicons.obj" \
	"$(INTDIR)\sounds.obj" \
	"$(INTDIR)\auth.obj" \
	"$(INTDIR)\authdialogs.obj" \
	"$(INTDIR)\awaymsg.obj" \
	"$(INTDIR)\sendmsg.obj" \
	"$(INTDIR)\email.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\fileexistsdlg.obj" \
	"$(INTDIR)\fileopts.obj" \
	"$(INTDIR)\filerecvdlg.obj" \
	"$(INTDIR)\filesenddlg.obj" \
	"$(INTDIR)\filexferdlg.obj" \
	"$(INTDIR)\ftmanager.obj" \
	"$(INTDIR)\url.obj" \
	"$(INTDIR)\urldialogs.obj" \
	"$(INTDIR)\contactinfo.obj" \
	"$(INTDIR)\stdinfo.obj" \
	"$(INTDIR)\userinfo.obj" \
	"$(INTDIR)\useronline.obj" \
	"$(INTDIR)\bmpfilter.obj" \
	"$(INTDIR)\colourpicker.obj" \
	"$(INTDIR)\hyperlink.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\openurl.obj" \
	"$(INTDIR)\path.obj" \
	"$(INTDIR)\resizer.obj" \
	"$(INTDIR)\sha1.obj" \
	"$(INTDIR)\utf.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\windowlist.obj" \
	"$(INTDIR)\visibility.obj" \
	"$(INTDIR)\clc.obj" \
	"$(INTDIR)\clcfiledrop.obj" \
	"$(INTDIR)\clcidents.obj" \
	"$(INTDIR)\clcitems.obj" \
	"$(INTDIR)\clcmsgs.obj" \
	"$(INTDIR)\clcutils.obj" \
	"$(INTDIR)\clistcore.obj" \
	"$(INTDIR)\clistevents.obj" \
	"$(INTDIR)\clistmenus.obj" \
	"$(INTDIR)\clistmod.obj" \
	"$(INTDIR)\clistsettings.obj" \
	"$(INTDIR)\clisttray.obj" \
	"$(INTDIR)\clui.obj" \
	"$(INTDIR)\cluiservices.obj" \
	"$(INTDIR)\contact.obj" \
	"$(INTDIR)\Docking.obj" \
	"$(INTDIR)\genmenu.obj" \
	"$(INTDIR)\genmenuopt.obj" \
	"$(INTDIR)\groups.obj" \
	"$(INTDIR)\keyboard.obj" \
	"$(INTDIR)\movetogroup.obj" \
	"$(INTDIR)\protocolorder.obj" \
	"$(INTDIR)\FontOptions.obj" \
	"$(INTDIR)\FontService.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\extracticon.obj" \
	"$(INTDIR)\skin2icons.obj" \
	"$(INTDIR)\updatenotify.obj" \
	"$(INTDIR)\xmlApi.obj" \
	"$(INTDIR)\xmlParser.obj" \
	"$(INTDIR)\vc6.res" \
	"..\plugins\zlib\Release\zlib.lib"

"..\bin\release\miranda32.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "..\bin\debug\miranda32.exe" "$(OUTDIR)\miranda32.bsc"

!ELSE 

ALL : "zlib - Win32 Debug" "..\bin\debug\miranda32.exe" "$(OUTDIR)\miranda32.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"zlib - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\about.obj"
	-@erase "$(INTDIR)\about.sbr"
	-@erase "$(INTDIR)\addcontact.obj"
	-@erase "$(INTDIR)\addcontact.sbr"
	-@erase "$(INTDIR)\auth.obj"
	-@erase "$(INTDIR)\auth.sbr"
	-@erase "$(INTDIR)\authdialogs.obj"
	-@erase "$(INTDIR)\authdialogs.sbr"
	-@erase "$(INTDIR)\autoaway.obj"
	-@erase "$(INTDIR)\autoaway.sbr"
	-@erase "$(INTDIR)\awaymsg.obj"
	-@erase "$(INTDIR)\awaymsg.sbr"
	-@erase "$(INTDIR)\bmpfilter.obj"
	-@erase "$(INTDIR)\bmpfilter.sbr"
	-@erase "$(INTDIR)\button.obj"
	-@erase "$(INTDIR)\button.sbr"
	-@erase "$(INTDIR)\clc.obj"
	-@erase "$(INTDIR)\clc.sbr"
	-@erase "$(INTDIR)\clcfiledrop.obj"
	-@erase "$(INTDIR)\clcfiledrop.sbr"
	-@erase "$(INTDIR)\clcidents.obj"
	-@erase "$(INTDIR)\clcidents.sbr"
	-@erase "$(INTDIR)\clcitems.obj"
	-@erase "$(INTDIR)\clcitems.sbr"
	-@erase "$(INTDIR)\clcmsgs.obj"
	-@erase "$(INTDIR)\clcmsgs.sbr"
	-@erase "$(INTDIR)\clcutils.obj"
	-@erase "$(INTDIR)\clcutils.sbr"
	-@erase "$(INTDIR)\clistcore.obj"
	-@erase "$(INTDIR)\clistcore.sbr"
	-@erase "$(INTDIR)\clistevents.obj"
	-@erase "$(INTDIR)\clistevents.sbr"
	-@erase "$(INTDIR)\clistmenus.obj"
	-@erase "$(INTDIR)\clistmenus.sbr"
	-@erase "$(INTDIR)\clistmod.obj"
	-@erase "$(INTDIR)\clistmod.sbr"
	-@erase "$(INTDIR)\clistsettings.obj"
	-@erase "$(INTDIR)\clistsettings.sbr"
	-@erase "$(INTDIR)\clisttray.obj"
	-@erase "$(INTDIR)\clisttray.sbr"
	-@erase "$(INTDIR)\clui.obj"
	-@erase "$(INTDIR)\clui.sbr"
	-@erase "$(INTDIR)\cluiservices.obj"
	-@erase "$(INTDIR)\cluiservices.sbr"
	-@erase "$(INTDIR)\colourpicker.obj"
	-@erase "$(INTDIR)\colourpicker.sbr"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\commonheaders.sbr"
	-@erase "$(INTDIR)\contact.obj"
	-@erase "$(INTDIR)\contact.sbr"
	-@erase "$(INTDIR)\contactinfo.obj"
	-@erase "$(INTDIR)\contactinfo.sbr"
	-@erase "$(INTDIR)\contacts.obj"
	-@erase "$(INTDIR)\contacts.sbr"
	-@erase "$(INTDIR)\database.obj"
	-@erase "$(INTDIR)\database.sbr"
	-@erase "$(INTDIR)\dbini.obj"
	-@erase "$(INTDIR)\dbini.sbr"
	-@erase "$(INTDIR)\dblists.obj"
	-@erase "$(INTDIR)\dblists.sbr"
	-@erase "$(INTDIR)\dbtime.obj"
	-@erase "$(INTDIR)\dbtime.sbr"
	-@erase "$(INTDIR)\dbutils.obj"
	-@erase "$(INTDIR)\dbutils.sbr"
	-@erase "$(INTDIR)\Docking.obj"
	-@erase "$(INTDIR)\Docking.sbr"
	-@erase "$(INTDIR)\email.obj"
	-@erase "$(INTDIR)\email.sbr"
	-@erase "$(INTDIR)\extracticon.obj"
	-@erase "$(INTDIR)\extracticon.sbr"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\file.sbr"
	-@erase "$(INTDIR)\fileexistsdlg.obj"
	-@erase "$(INTDIR)\fileexistsdlg.sbr"
	-@erase "$(INTDIR)\fileopts.obj"
	-@erase "$(INTDIR)\fileopts.sbr"
	-@erase "$(INTDIR)\filerecvdlg.obj"
	-@erase "$(INTDIR)\filerecvdlg.sbr"
	-@erase "$(INTDIR)\filesenddlg.obj"
	-@erase "$(INTDIR)\filesenddlg.sbr"
	-@erase "$(INTDIR)\filexferdlg.obj"
	-@erase "$(INTDIR)\filexferdlg.sbr"
	-@erase "$(INTDIR)\filter.obj"
	-@erase "$(INTDIR)\filter.sbr"
	-@erase "$(INTDIR)\findadd.obj"
	-@erase "$(INTDIR)\findadd.sbr"
	-@erase "$(INTDIR)\FontOptions.obj"
	-@erase "$(INTDIR)\FontOptions.sbr"
	-@erase "$(INTDIR)\FontService.obj"
	-@erase "$(INTDIR)\FontService.sbr"
	-@erase "$(INTDIR)\ftmanager.obj"
	-@erase "$(INTDIR)\ftmanager.sbr"
	-@erase "$(INTDIR)\genmenu.obj"
	-@erase "$(INTDIR)\genmenu.sbr"
	-@erase "$(INTDIR)\genmenuopt.obj"
	-@erase "$(INTDIR)\genmenuopt.sbr"
	-@erase "$(INTDIR)\groups.obj"
	-@erase "$(INTDIR)\groups.sbr"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\help.sbr"
	-@erase "$(INTDIR)\history.obj"
	-@erase "$(INTDIR)\history.sbr"
	-@erase "$(INTDIR)\hotkeys.obj"
	-@erase "$(INTDIR)\hotkeys.sbr"
	-@erase "$(INTDIR)\hyperlink.obj"
	-@erase "$(INTDIR)\hyperlink.sbr"
	-@erase "$(INTDIR)\idle.obj"
	-@erase "$(INTDIR)\idle.sbr"
	-@erase "$(INTDIR)\ignore.obj"
	-@erase "$(INTDIR)\ignore.sbr"
	-@erase "$(INTDIR)\keyboard.obj"
	-@erase "$(INTDIR)\keyboard.sbr"
	-@erase "$(INTDIR)\langpack.obj"
	-@erase "$(INTDIR)\langpack.sbr"
	-@erase "$(INTDIR)\lpservices.obj"
	-@erase "$(INTDIR)\lpservices.sbr"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\md5.sbr"
	-@erase "$(INTDIR)\memory.obj"
	-@erase "$(INTDIR)\memory.sbr"
	-@erase "$(INTDIR)\miranda.obj"
	-@erase "$(INTDIR)\miranda.sbr"
	-@erase "$(INTDIR)\miranda32.pch"
	-@erase "$(INTDIR)\modules.obj"
	-@erase "$(INTDIR)\modules.sbr"
	-@erase "$(INTDIR)\movetogroup.obj"
	-@erase "$(INTDIR)\movetogroup.sbr"
	-@erase "$(INTDIR)\netlib.obj"
	-@erase "$(INTDIR)\netlib.sbr"
	-@erase "$(INTDIR)\netlibbind.obj"
	-@erase "$(INTDIR)\netlibbind.sbr"
	-@erase "$(INTDIR)\netlibhttp.obj"
	-@erase "$(INTDIR)\netlibhttp.sbr"
	-@erase "$(INTDIR)\netlibhttpproxy.obj"
	-@erase "$(INTDIR)\netlibhttpproxy.sbr"
	-@erase "$(INTDIR)\netliblog.obj"
	-@erase "$(INTDIR)\netliblog.sbr"
	-@erase "$(INTDIR)\netlibopenconn.obj"
	-@erase "$(INTDIR)\netlibopenconn.sbr"
	-@erase "$(INTDIR)\netlibopts.obj"
	-@erase "$(INTDIR)\netlibopts.sbr"
	-@erase "$(INTDIR)\netlibpktrecver.obj"
	-@erase "$(INTDIR)\netlibpktrecver.sbr"
	-@erase "$(INTDIR)\netlibsecurity.obj"
	-@erase "$(INTDIR)\netlibsecurity.sbr"
	-@erase "$(INTDIR)\netlibsock.obj"
	-@erase "$(INTDIR)\netlibsock.sbr"
	-@erase "$(INTDIR)\netlibssl.obj"
	-@erase "$(INTDIR)\netlibssl.sbr"
	-@erase "$(INTDIR)\netlibupnp.obj"
	-@erase "$(INTDIR)\netlibupnp.sbr"
	-@erase "$(INTDIR)\newplugins.obj"
	-@erase "$(INTDIR)\newplugins.sbr"
	-@erase "$(INTDIR)\openurl.obj"
	-@erase "$(INTDIR)\openurl.sbr"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\options.sbr"
	-@erase "$(INTDIR)\path.obj"
	-@erase "$(INTDIR)\path.sbr"
	-@erase "$(INTDIR)\profilemanager.obj"
	-@erase "$(INTDIR)\profilemanager.sbr"
	-@erase "$(INTDIR)\protoaccs.obj"
	-@erase "$(INTDIR)\protoaccs.sbr"
	-@erase "$(INTDIR)\protochains.obj"
	-@erase "$(INTDIR)\protochains.sbr"
	-@erase "$(INTDIR)\protocolorder.obj"
	-@erase "$(INTDIR)\protocolorder.sbr"
	-@erase "$(INTDIR)\protocols.obj"
	-@erase "$(INTDIR)\protocols.sbr"
	-@erase "$(INTDIR)\protoint.obj"
	-@erase "$(INTDIR)\protoint.sbr"
	-@erase "$(INTDIR)\protoopts.obj"
	-@erase "$(INTDIR)\protoopts.sbr"
	-@erase "$(INTDIR)\resizer.obj"
	-@erase "$(INTDIR)\resizer.sbr"
	-@erase "$(INTDIR)\searchresults.obj"
	-@erase "$(INTDIR)\searchresults.sbr"
	-@erase "$(INTDIR)\sendmsg.obj"
	-@erase "$(INTDIR)\sendmsg.sbr"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\services.sbr"
	-@erase "$(INTDIR)\sha1.obj"
	-@erase "$(INTDIR)\sha1.sbr"
	-@erase "$(INTDIR)\skin2icons.obj"
	-@erase "$(INTDIR)\skin2icons.sbr"
	-@erase "$(INTDIR)\skinicons.obj"
	-@erase "$(INTDIR)\skinicons.sbr"
	-@erase "$(INTDIR)\sounds.obj"
	-@erase "$(INTDIR)\sounds.sbr"
	-@erase "$(INTDIR)\stdinfo.obj"
	-@erase "$(INTDIR)\stdinfo.sbr"
	-@erase "$(INTDIR)\updatenotify.obj"
	-@erase "$(INTDIR)\updatenotify.sbr"
	-@erase "$(INTDIR)\url.obj"
	-@erase "$(INTDIR)\url.sbr"
	-@erase "$(INTDIR)\urldialogs.obj"
	-@erase "$(INTDIR)\urldialogs.sbr"
	-@erase "$(INTDIR)\userinfo.obj"
	-@erase "$(INTDIR)\userinfo.sbr"
	-@erase "$(INTDIR)\useronline.obj"
	-@erase "$(INTDIR)\useronline.sbr"
	-@erase "$(INTDIR)\utf.obj"
	-@erase "$(INTDIR)\utf.sbr"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\utils.sbr"
	-@erase "$(INTDIR)\vc6.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\visibility.obj"
	-@erase "$(INTDIR)\visibility.sbr"
	-@erase "$(INTDIR)\windowlist.obj"
	-@erase "$(INTDIR)\windowlist.sbr"
	-@erase "$(INTDIR)\xmlApi.obj"
	-@erase "$(INTDIR)\xmlApi.sbr"
	-@erase "$(INTDIR)\xmlParser.obj"
	-@erase "$(INTDIR)\xmlParser.sbr"
	-@erase "$(OUTDIR)\miranda32.bsc"
	-@erase "$(OUTDIR)\miranda32.map"
	-@erase "$(OUTDIR)\miranda32.pdb"
	-@erase "..\bin\debug\miranda32.exe"
	-@erase "..\bin\debug\miranda32.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\vc6.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\miranda32.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\commonheaders.sbr" \
	"$(INTDIR)\memory.sbr" \
	"$(INTDIR)\miranda.sbr" \
	"$(INTDIR)\modules.sbr" \
	"$(INTDIR)\addcontact.sbr" \
	"$(INTDIR)\autoaway.sbr" \
	"$(INTDIR)\button.sbr" \
	"$(INTDIR)\contacts.sbr" \
	"$(INTDIR)\database.sbr" \
	"$(INTDIR)\dbini.sbr" \
	"$(INTDIR)\dblists.sbr" \
	"$(INTDIR)\dbtime.sbr" \
	"$(INTDIR)\dbutils.sbr" \
	"$(INTDIR)\profilemanager.sbr" \
	"$(INTDIR)\findadd.sbr" \
	"$(INTDIR)\searchresults.sbr" \
	"$(INTDIR)\about.sbr" \
	"$(INTDIR)\help.sbr" \
	"$(INTDIR)\history.sbr" \
	"$(INTDIR)\idle.sbr" \
	"$(INTDIR)\ignore.sbr" \
	"$(INTDIR)\langpack.sbr" \
	"$(INTDIR)\lpservices.sbr" \
	"$(INTDIR)\netlib.sbr" \
	"$(INTDIR)\netlibbind.sbr" \
	"$(INTDIR)\netlibhttp.sbr" \
	"$(INTDIR)\netlibhttpproxy.sbr" \
	"$(INTDIR)\netliblog.sbr" \
	"$(INTDIR)\netlibopenconn.sbr" \
	"$(INTDIR)\netlibopts.sbr" \
	"$(INTDIR)\netlibpktrecver.sbr" \
	"$(INTDIR)\netlibsecurity.sbr" \
	"$(INTDIR)\netlibsock.sbr" \
	"$(INTDIR)\netlibssl.sbr" \
	"$(INTDIR)\netlibupnp.sbr" \
	"$(INTDIR)\filter.sbr" \
	"$(INTDIR)\options.sbr" \
	"$(INTDIR)\newplugins.sbr" \
	"$(INTDIR)\protoaccs.sbr" \
	"$(INTDIR)\protochains.sbr" \
	"$(INTDIR)\protocols.sbr" \
	"$(INTDIR)\protoint.sbr" \
	"$(INTDIR)\protoopts.sbr" \
	"$(INTDIR)\hotkeys.sbr" \
	"$(INTDIR)\skinicons.sbr" \
	"$(INTDIR)\sounds.sbr" \
	"$(INTDIR)\auth.sbr" \
	"$(INTDIR)\authdialogs.sbr" \
	"$(INTDIR)\awaymsg.sbr" \
	"$(INTDIR)\sendmsg.sbr" \
	"$(INTDIR)\email.sbr" \
	"$(INTDIR)\file.sbr" \
	"$(INTDIR)\fileexistsdlg.sbr" \
	"$(INTDIR)\fileopts.sbr" \
	"$(INTDIR)\filerecvdlg.sbr" \
	"$(INTDIR)\filesenddlg.sbr" \
	"$(INTDIR)\filexferdlg.sbr" \
	"$(INTDIR)\ftmanager.sbr" \
	"$(INTDIR)\url.sbr" \
	"$(INTDIR)\urldialogs.sbr" \
	"$(INTDIR)\contactinfo.sbr" \
	"$(INTDIR)\stdinfo.sbr" \
	"$(INTDIR)\userinfo.sbr" \
	"$(INTDIR)\useronline.sbr" \
	"$(INTDIR)\bmpfilter.sbr" \
	"$(INTDIR)\colourpicker.sbr" \
	"$(INTDIR)\hyperlink.sbr" \
	"$(INTDIR)\md5.sbr" \
	"$(INTDIR)\openurl.sbr" \
	"$(INTDIR)\path.sbr" \
	"$(INTDIR)\resizer.sbr" \
	"$(INTDIR)\sha1.sbr" \
	"$(INTDIR)\utf.sbr" \
	"$(INTDIR)\utils.sbr" \
	"$(INTDIR)\windowlist.sbr" \
	"$(INTDIR)\visibility.sbr" \
	"$(INTDIR)\clc.sbr" \
	"$(INTDIR)\clcfiledrop.sbr" \
	"$(INTDIR)\clcidents.sbr" \
	"$(INTDIR)\clcitems.sbr" \
	"$(INTDIR)\clcmsgs.sbr" \
	"$(INTDIR)\clcutils.sbr" \
	"$(INTDIR)\clistcore.sbr" \
	"$(INTDIR)\clistevents.sbr" \
	"$(INTDIR)\clistmenus.sbr" \
	"$(INTDIR)\clistmod.sbr" \
	"$(INTDIR)\clistsettings.sbr" \
	"$(INTDIR)\clisttray.sbr" \
	"$(INTDIR)\clui.sbr" \
	"$(INTDIR)\cluiservices.sbr" \
	"$(INTDIR)\contact.sbr" \
	"$(INTDIR)\Docking.sbr" \
	"$(INTDIR)\genmenu.sbr" \
	"$(INTDIR)\genmenuopt.sbr" \
	"$(INTDIR)\groups.sbr" \
	"$(INTDIR)\keyboard.sbr" \
	"$(INTDIR)\movetogroup.sbr" \
	"$(INTDIR)\protocolorder.sbr" \
	"$(INTDIR)\FontOptions.sbr" \
	"$(INTDIR)\FontService.sbr" \
	"$(INTDIR)\services.sbr" \
	"$(INTDIR)\extracticon.sbr" \
	"$(INTDIR)\skin2icons.sbr" \
	"$(INTDIR)\updatenotify.sbr" \
	"$(INTDIR)\xmlApi.sbr" \
	"$(INTDIR)\xmlParser.sbr"

"$(OUTDIR)\miranda32.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib shell32.lib winspool.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib version.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\miranda32.pdb" /map:"$(INTDIR)\miranda32.map" /debug /machine:I386 /out:"../bin/debug/miranda32.exe" 
LINK32_OBJS= \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\memory.obj" \
	"$(INTDIR)\miranda.obj" \
	"$(INTDIR)\modules.obj" \
	"$(INTDIR)\addcontact.obj" \
	"$(INTDIR)\autoaway.obj" \
	"$(INTDIR)\button.obj" \
	"$(INTDIR)\contacts.obj" \
	"$(INTDIR)\database.obj" \
	"$(INTDIR)\dbini.obj" \
	"$(INTDIR)\dblists.obj" \
	"$(INTDIR)\dbtime.obj" \
	"$(INTDIR)\dbutils.obj" \
	"$(INTDIR)\profilemanager.obj" \
	"$(INTDIR)\findadd.obj" \
	"$(INTDIR)\searchresults.obj" \
	"$(INTDIR)\about.obj" \
	"$(INTDIR)\help.obj" \
	"$(INTDIR)\history.obj" \
	"$(INTDIR)\idle.obj" \
	"$(INTDIR)\ignore.obj" \
	"$(INTDIR)\langpack.obj" \
	"$(INTDIR)\lpservices.obj" \
	"$(INTDIR)\netlib.obj" \
	"$(INTDIR)\netlibbind.obj" \
	"$(INTDIR)\netlibhttp.obj" \
	"$(INTDIR)\netlibhttpproxy.obj" \
	"$(INTDIR)\netliblog.obj" \
	"$(INTDIR)\netlibopenconn.obj" \
	"$(INTDIR)\netlibopts.obj" \
	"$(INTDIR)\netlibpktrecver.obj" \
	"$(INTDIR)\netlibsecurity.obj" \
	"$(INTDIR)\netlibsock.obj" \
	"$(INTDIR)\netlibssl.obj" \
	"$(INTDIR)\netlibupnp.obj" \
	"$(INTDIR)\filter.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\newplugins.obj" \
	"$(INTDIR)\protoaccs.obj" \
	"$(INTDIR)\protochains.obj" \
	"$(INTDIR)\protocols.obj" \
	"$(INTDIR)\protoint.obj" \
	"$(INTDIR)\protoopts.obj" \
	"$(INTDIR)\hotkeys.obj" \
	"$(INTDIR)\skinicons.obj" \
	"$(INTDIR)\sounds.obj" \
	"$(INTDIR)\auth.obj" \
	"$(INTDIR)\authdialogs.obj" \
	"$(INTDIR)\awaymsg.obj" \
	"$(INTDIR)\sendmsg.obj" \
	"$(INTDIR)\email.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\fileexistsdlg.obj" \
	"$(INTDIR)\fileopts.obj" \
	"$(INTDIR)\filerecvdlg.obj" \
	"$(INTDIR)\filesenddlg.obj" \
	"$(INTDIR)\filexferdlg.obj" \
	"$(INTDIR)\ftmanager.obj" \
	"$(INTDIR)\url.obj" \
	"$(INTDIR)\urldialogs.obj" \
	"$(INTDIR)\contactinfo.obj" \
	"$(INTDIR)\stdinfo.obj" \
	"$(INTDIR)\userinfo.obj" \
	"$(INTDIR)\useronline.obj" \
	"$(INTDIR)\bmpfilter.obj" \
	"$(INTDIR)\colourpicker.obj" \
	"$(INTDIR)\hyperlink.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\openurl.obj" \
	"$(INTDIR)\path.obj" \
	"$(INTDIR)\resizer.obj" \
	"$(INTDIR)\sha1.obj" \
	"$(INTDIR)\utf.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\windowlist.obj" \
	"$(INTDIR)\visibility.obj" \
	"$(INTDIR)\clc.obj" \
	"$(INTDIR)\clcfiledrop.obj" \
	"$(INTDIR)\clcidents.obj" \
	"$(INTDIR)\clcitems.obj" \
	"$(INTDIR)\clcmsgs.obj" \
	"$(INTDIR)\clcutils.obj" \
	"$(INTDIR)\clistcore.obj" \
	"$(INTDIR)\clistevents.obj" \
	"$(INTDIR)\clistmenus.obj" \
	"$(INTDIR)\clistmod.obj" \
	"$(INTDIR)\clistsettings.obj" \
	"$(INTDIR)\clisttray.obj" \
	"$(INTDIR)\clui.obj" \
	"$(INTDIR)\cluiservices.obj" \
	"$(INTDIR)\contact.obj" \
	"$(INTDIR)\Docking.obj" \
	"$(INTDIR)\genmenu.obj" \
	"$(INTDIR)\genmenuopt.obj" \
	"$(INTDIR)\groups.obj" \
	"$(INTDIR)\keyboard.obj" \
	"$(INTDIR)\movetogroup.obj" \
	"$(INTDIR)\protocolorder.obj" \
	"$(INTDIR)\FontOptions.obj" \
	"$(INTDIR)\FontService.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\extracticon.obj" \
	"$(INTDIR)\skin2icons.obj" \
	"$(INTDIR)\updatenotify.obj" \
	"$(INTDIR)\xmlApi.obj" \
	"$(INTDIR)\xmlParser.obj" \
	"$(INTDIR)\vc6.res" \
	"..\plugins\zlib\Debug\zlib.lib"

"..\bin\debug\miranda32.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

OUTDIR=.\Release_Unicode
INTDIR=.\Release_Unicode

!IF "$(RECURSE)" == "0" 

ALL : "..\bin\Release Unicode\miranda32.exe"

!ELSE 

ALL : "zlib - Win32 Release Unicode" "..\bin\Release Unicode\miranda32.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"zlib - Win32 Release UnicodeCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\about.obj"
	-@erase "$(INTDIR)\addcontact.obj"
	-@erase "$(INTDIR)\auth.obj"
	-@erase "$(INTDIR)\authdialogs.obj"
	-@erase "$(INTDIR)\autoaway.obj"
	-@erase "$(INTDIR)\awaymsg.obj"
	-@erase "$(INTDIR)\bmpfilter.obj"
	-@erase "$(INTDIR)\button.obj"
	-@erase "$(INTDIR)\clc.obj"
	-@erase "$(INTDIR)\clcfiledrop.obj"
	-@erase "$(INTDIR)\clcidents.obj"
	-@erase "$(INTDIR)\clcitems.obj"
	-@erase "$(INTDIR)\clcmsgs.obj"
	-@erase "$(INTDIR)\clcutils.obj"
	-@erase "$(INTDIR)\clistcore.obj"
	-@erase "$(INTDIR)\clistevents.obj"
	-@erase "$(INTDIR)\clistmenus.obj"
	-@erase "$(INTDIR)\clistmod.obj"
	-@erase "$(INTDIR)\clistsettings.obj"
	-@erase "$(INTDIR)\clisttray.obj"
	-@erase "$(INTDIR)\clui.obj"
	-@erase "$(INTDIR)\cluiservices.obj"
	-@erase "$(INTDIR)\colourpicker.obj"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\contact.obj"
	-@erase "$(INTDIR)\contactinfo.obj"
	-@erase "$(INTDIR)\contacts.obj"
	-@erase "$(INTDIR)\database.obj"
	-@erase "$(INTDIR)\dbini.obj"
	-@erase "$(INTDIR)\dblists.obj"
	-@erase "$(INTDIR)\dbtime.obj"
	-@erase "$(INTDIR)\dbutils.obj"
	-@erase "$(INTDIR)\Docking.obj"
	-@erase "$(INTDIR)\email.obj"
	-@erase "$(INTDIR)\extracticon.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\fileexistsdlg.obj"
	-@erase "$(INTDIR)\fileopts.obj"
	-@erase "$(INTDIR)\filerecvdlg.obj"
	-@erase "$(INTDIR)\filesenddlg.obj"
	-@erase "$(INTDIR)\filexferdlg.obj"
	-@erase "$(INTDIR)\filter.obj"
	-@erase "$(INTDIR)\findadd.obj"
	-@erase "$(INTDIR)\FontOptions.obj"
	-@erase "$(INTDIR)\FontService.obj"
	-@erase "$(INTDIR)\ftmanager.obj"
	-@erase "$(INTDIR)\genmenu.obj"
	-@erase "$(INTDIR)\genmenuopt.obj"
	-@erase "$(INTDIR)\groups.obj"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\history.obj"
	-@erase "$(INTDIR)\hotkeys.obj"
	-@erase "$(INTDIR)\hyperlink.obj"
	-@erase "$(INTDIR)\idle.obj"
	-@erase "$(INTDIR)\ignore.obj"
	-@erase "$(INTDIR)\keyboard.obj"
	-@erase "$(INTDIR)\langpack.obj"
	-@erase "$(INTDIR)\lpservices.obj"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\memory.obj"
	-@erase "$(INTDIR)\miranda.obj"
	-@erase "$(INTDIR)\miranda32.pch"
	-@erase "$(INTDIR)\modules.obj"
	-@erase "$(INTDIR)\movetogroup.obj"
	-@erase "$(INTDIR)\netlib.obj"
	-@erase "$(INTDIR)\netlibbind.obj"
	-@erase "$(INTDIR)\netlibhttp.obj"
	-@erase "$(INTDIR)\netlibhttpproxy.obj"
	-@erase "$(INTDIR)\netliblog.obj"
	-@erase "$(INTDIR)\netlibopenconn.obj"
	-@erase "$(INTDIR)\netlibopts.obj"
	-@erase "$(INTDIR)\netlibpktrecver.obj"
	-@erase "$(INTDIR)\netlibsecurity.obj"
	-@erase "$(INTDIR)\netlibsock.obj"
	-@erase "$(INTDIR)\netlibssl.obj"
	-@erase "$(INTDIR)\netlibupnp.obj"
	-@erase "$(INTDIR)\newplugins.obj"
	-@erase "$(INTDIR)\openurl.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\path.obj"
	-@erase "$(INTDIR)\profilemanager.obj"
	-@erase "$(INTDIR)\protoaccs.obj"
	-@erase "$(INTDIR)\protochains.obj"
	-@erase "$(INTDIR)\protocolorder.obj"
	-@erase "$(INTDIR)\protocols.obj"
	-@erase "$(INTDIR)\protoint.obj"
	-@erase "$(INTDIR)\protoopts.obj"
	-@erase "$(INTDIR)\resizer.obj"
	-@erase "$(INTDIR)\searchresults.obj"
	-@erase "$(INTDIR)\sendmsg.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\sha1.obj"
	-@erase "$(INTDIR)\skin2icons.obj"
	-@erase "$(INTDIR)\skinicons.obj"
	-@erase "$(INTDIR)\sounds.obj"
	-@erase "$(INTDIR)\stdinfo.obj"
	-@erase "$(INTDIR)\updatenotify.obj"
	-@erase "$(INTDIR)\url.obj"
	-@erase "$(INTDIR)\urldialogs.obj"
	-@erase "$(INTDIR)\userinfo.obj"
	-@erase "$(INTDIR)\useronline.obj"
	-@erase "$(INTDIR)\utf.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\vc6.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\visibility.obj"
	-@erase "$(INTDIR)\windowlist.obj"
	-@erase "$(INTDIR)\xmlApi.obj"
	-@erase "$(INTDIR)\xmlParser.obj"
	-@erase "$(OUTDIR)\miranda32.map"
	-@erase "$(OUTDIR)\miranda32.pdb"
	-@erase "..\bin\Release Unicode\miranda32.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\vc6.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\miranda32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib shell32.lib winspool.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib version.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\miranda32.pdb" /map:"$(INTDIR)\miranda32.map" /debug /machine:I386 /out:"../bin/Release Unicode/miranda32.exe" /fixed /ALIGN:4096 /ignore:4108 
LINK32_OBJS= \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\memory.obj" \
	"$(INTDIR)\miranda.obj" \
	"$(INTDIR)\modules.obj" \
	"$(INTDIR)\addcontact.obj" \
	"$(INTDIR)\autoaway.obj" \
	"$(INTDIR)\button.obj" \
	"$(INTDIR)\contacts.obj" \
	"$(INTDIR)\database.obj" \
	"$(INTDIR)\dbini.obj" \
	"$(INTDIR)\dblists.obj" \
	"$(INTDIR)\dbtime.obj" \
	"$(INTDIR)\dbutils.obj" \
	"$(INTDIR)\profilemanager.obj" \
	"$(INTDIR)\findadd.obj" \
	"$(INTDIR)\searchresults.obj" \
	"$(INTDIR)\about.obj" \
	"$(INTDIR)\help.obj" \
	"$(INTDIR)\history.obj" \
	"$(INTDIR)\idle.obj" \
	"$(INTDIR)\ignore.obj" \
	"$(INTDIR)\langpack.obj" \
	"$(INTDIR)\lpservices.obj" \
	"$(INTDIR)\netlib.obj" \
	"$(INTDIR)\netlibbind.obj" \
	"$(INTDIR)\netlibhttp.obj" \
	"$(INTDIR)\netlibhttpproxy.obj" \
	"$(INTDIR)\netliblog.obj" \
	"$(INTDIR)\netlibopenconn.obj" \
	"$(INTDIR)\netlibopts.obj" \
	"$(INTDIR)\netlibpktrecver.obj" \
	"$(INTDIR)\netlibsecurity.obj" \
	"$(INTDIR)\netlibsock.obj" \
	"$(INTDIR)\netlibssl.obj" \
	"$(INTDIR)\netlibupnp.obj" \
	"$(INTDIR)\filter.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\newplugins.obj" \
	"$(INTDIR)\protoaccs.obj" \
	"$(INTDIR)\protochains.obj" \
	"$(INTDIR)\protocols.obj" \
	"$(INTDIR)\protoint.obj" \
	"$(INTDIR)\protoopts.obj" \
	"$(INTDIR)\hotkeys.obj" \
	"$(INTDIR)\skinicons.obj" \
	"$(INTDIR)\sounds.obj" \
	"$(INTDIR)\auth.obj" \
	"$(INTDIR)\authdialogs.obj" \
	"$(INTDIR)\awaymsg.obj" \
	"$(INTDIR)\sendmsg.obj" \
	"$(INTDIR)\email.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\fileexistsdlg.obj" \
	"$(INTDIR)\fileopts.obj" \
	"$(INTDIR)\filerecvdlg.obj" \
	"$(INTDIR)\filesenddlg.obj" \
	"$(INTDIR)\filexferdlg.obj" \
	"$(INTDIR)\ftmanager.obj" \
	"$(INTDIR)\url.obj" \
	"$(INTDIR)\urldialogs.obj" \
	"$(INTDIR)\contactinfo.obj" \
	"$(INTDIR)\stdinfo.obj" \
	"$(INTDIR)\userinfo.obj" \
	"$(INTDIR)\useronline.obj" \
	"$(INTDIR)\bmpfilter.obj" \
	"$(INTDIR)\colourpicker.obj" \
	"$(INTDIR)\hyperlink.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\openurl.obj" \
	"$(INTDIR)\path.obj" \
	"$(INTDIR)\resizer.obj" \
	"$(INTDIR)\sha1.obj" \
	"$(INTDIR)\utf.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\windowlist.obj" \
	"$(INTDIR)\visibility.obj" \
	"$(INTDIR)\clc.obj" \
	"$(INTDIR)\clcfiledrop.obj" \
	"$(INTDIR)\clcidents.obj" \
	"$(INTDIR)\clcitems.obj" \
	"$(INTDIR)\clcmsgs.obj" \
	"$(INTDIR)\clcutils.obj" \
	"$(INTDIR)\clistcore.obj" \
	"$(INTDIR)\clistevents.obj" \
	"$(INTDIR)\clistmenus.obj" \
	"$(INTDIR)\clistmod.obj" \
	"$(INTDIR)\clistsettings.obj" \
	"$(INTDIR)\clisttray.obj" \
	"$(INTDIR)\clui.obj" \
	"$(INTDIR)\cluiservices.obj" \
	"$(INTDIR)\contact.obj" \
	"$(INTDIR)\Docking.obj" \
	"$(INTDIR)\genmenu.obj" \
	"$(INTDIR)\genmenuopt.obj" \
	"$(INTDIR)\groups.obj" \
	"$(INTDIR)\keyboard.obj" \
	"$(INTDIR)\movetogroup.obj" \
	"$(INTDIR)\protocolorder.obj" \
	"$(INTDIR)\FontOptions.obj" \
	"$(INTDIR)\FontService.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\extracticon.obj" \
	"$(INTDIR)\skin2icons.obj" \
	"$(INTDIR)\updatenotify.obj" \
	"$(INTDIR)\xmlApi.obj" \
	"$(INTDIR)\xmlParser.obj" \
	"$(INTDIR)\vc6.res" \
	"..\plugins\zlib\Release_Unicode\zlib.lib"

"..\bin\Release Unicode\miranda32.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

OUTDIR=.\Debug_Unicode
INTDIR=.\Debug_Unicode
# Begin Custom Macros
OutDir=.\Debug_Unicode
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "..\bin\Debug Unicode\miranda32.exe" "$(OUTDIR)\miranda32.bsc"

!ELSE 

ALL : "zlib - Win32 Debug Unicode" "..\bin\Debug Unicode\miranda32.exe" "$(OUTDIR)\miranda32.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"zlib - Win32 Debug UnicodeCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\about.obj"
	-@erase "$(INTDIR)\about.sbr"
	-@erase "$(INTDIR)\addcontact.obj"
	-@erase "$(INTDIR)\addcontact.sbr"
	-@erase "$(INTDIR)\auth.obj"
	-@erase "$(INTDIR)\auth.sbr"
	-@erase "$(INTDIR)\authdialogs.obj"
	-@erase "$(INTDIR)\authdialogs.sbr"
	-@erase "$(INTDIR)\autoaway.obj"
	-@erase "$(INTDIR)\autoaway.sbr"
	-@erase "$(INTDIR)\awaymsg.obj"
	-@erase "$(INTDIR)\awaymsg.sbr"
	-@erase "$(INTDIR)\bmpfilter.obj"
	-@erase "$(INTDIR)\bmpfilter.sbr"
	-@erase "$(INTDIR)\button.obj"
	-@erase "$(INTDIR)\button.sbr"
	-@erase "$(INTDIR)\clc.obj"
	-@erase "$(INTDIR)\clc.sbr"
	-@erase "$(INTDIR)\clcfiledrop.obj"
	-@erase "$(INTDIR)\clcfiledrop.sbr"
	-@erase "$(INTDIR)\clcidents.obj"
	-@erase "$(INTDIR)\clcidents.sbr"
	-@erase "$(INTDIR)\clcitems.obj"
	-@erase "$(INTDIR)\clcitems.sbr"
	-@erase "$(INTDIR)\clcmsgs.obj"
	-@erase "$(INTDIR)\clcmsgs.sbr"
	-@erase "$(INTDIR)\clcutils.obj"
	-@erase "$(INTDIR)\clcutils.sbr"
	-@erase "$(INTDIR)\clistcore.obj"
	-@erase "$(INTDIR)\clistcore.sbr"
	-@erase "$(INTDIR)\clistevents.obj"
	-@erase "$(INTDIR)\clistevents.sbr"
	-@erase "$(INTDIR)\clistmenus.obj"
	-@erase "$(INTDIR)\clistmenus.sbr"
	-@erase "$(INTDIR)\clistmod.obj"
	-@erase "$(INTDIR)\clistmod.sbr"
	-@erase "$(INTDIR)\clistsettings.obj"
	-@erase "$(INTDIR)\clistsettings.sbr"
	-@erase "$(INTDIR)\clisttray.obj"
	-@erase "$(INTDIR)\clisttray.sbr"
	-@erase "$(INTDIR)\clui.obj"
	-@erase "$(INTDIR)\clui.sbr"
	-@erase "$(INTDIR)\cluiservices.obj"
	-@erase "$(INTDIR)\cluiservices.sbr"
	-@erase "$(INTDIR)\colourpicker.obj"
	-@erase "$(INTDIR)\colourpicker.sbr"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\commonheaders.sbr"
	-@erase "$(INTDIR)\contact.obj"
	-@erase "$(INTDIR)\contact.sbr"
	-@erase "$(INTDIR)\contactinfo.obj"
	-@erase "$(INTDIR)\contactinfo.sbr"
	-@erase "$(INTDIR)\contacts.obj"
	-@erase "$(INTDIR)\contacts.sbr"
	-@erase "$(INTDIR)\database.obj"
	-@erase "$(INTDIR)\database.sbr"
	-@erase "$(INTDIR)\dbini.obj"
	-@erase "$(INTDIR)\dbini.sbr"
	-@erase "$(INTDIR)\dblists.obj"
	-@erase "$(INTDIR)\dblists.sbr"
	-@erase "$(INTDIR)\dbtime.obj"
	-@erase "$(INTDIR)\dbtime.sbr"
	-@erase "$(INTDIR)\dbutils.obj"
	-@erase "$(INTDIR)\dbutils.sbr"
	-@erase "$(INTDIR)\Docking.obj"
	-@erase "$(INTDIR)\Docking.sbr"
	-@erase "$(INTDIR)\email.obj"
	-@erase "$(INTDIR)\email.sbr"
	-@erase "$(INTDIR)\extracticon.obj"
	-@erase "$(INTDIR)\extracticon.sbr"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\file.sbr"
	-@erase "$(INTDIR)\fileexistsdlg.obj"
	-@erase "$(INTDIR)\fileexistsdlg.sbr"
	-@erase "$(INTDIR)\fileopts.obj"
	-@erase "$(INTDIR)\fileopts.sbr"
	-@erase "$(INTDIR)\filerecvdlg.obj"
	-@erase "$(INTDIR)\filerecvdlg.sbr"
	-@erase "$(INTDIR)\filesenddlg.obj"
	-@erase "$(INTDIR)\filesenddlg.sbr"
	-@erase "$(INTDIR)\filexferdlg.obj"
	-@erase "$(INTDIR)\filexferdlg.sbr"
	-@erase "$(INTDIR)\filter.obj"
	-@erase "$(INTDIR)\filter.sbr"
	-@erase "$(INTDIR)\findadd.obj"
	-@erase "$(INTDIR)\findadd.sbr"
	-@erase "$(INTDIR)\FontOptions.obj"
	-@erase "$(INTDIR)\FontOptions.sbr"
	-@erase "$(INTDIR)\FontService.obj"
	-@erase "$(INTDIR)\FontService.sbr"
	-@erase "$(INTDIR)\ftmanager.obj"
	-@erase "$(INTDIR)\ftmanager.sbr"
	-@erase "$(INTDIR)\genmenu.obj"
	-@erase "$(INTDIR)\genmenu.sbr"
	-@erase "$(INTDIR)\genmenuopt.obj"
	-@erase "$(INTDIR)\genmenuopt.sbr"
	-@erase "$(INTDIR)\groups.obj"
	-@erase "$(INTDIR)\groups.sbr"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\help.sbr"
	-@erase "$(INTDIR)\history.obj"
	-@erase "$(INTDIR)\history.sbr"
	-@erase "$(INTDIR)\hotkeys.obj"
	-@erase "$(INTDIR)\hotkeys.sbr"
	-@erase "$(INTDIR)\hyperlink.obj"
	-@erase "$(INTDIR)\hyperlink.sbr"
	-@erase "$(INTDIR)\idle.obj"
	-@erase "$(INTDIR)\idle.sbr"
	-@erase "$(INTDIR)\ignore.obj"
	-@erase "$(INTDIR)\ignore.sbr"
	-@erase "$(INTDIR)\keyboard.obj"
	-@erase "$(INTDIR)\keyboard.sbr"
	-@erase "$(INTDIR)\langpack.obj"
	-@erase "$(INTDIR)\langpack.sbr"
	-@erase "$(INTDIR)\lpservices.obj"
	-@erase "$(INTDIR)\lpservices.sbr"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\md5.sbr"
	-@erase "$(INTDIR)\memory.obj"
	-@erase "$(INTDIR)\memory.sbr"
	-@erase "$(INTDIR)\miranda.obj"
	-@erase "$(INTDIR)\miranda.sbr"
	-@erase "$(INTDIR)\miranda32.pch"
	-@erase "$(INTDIR)\modules.obj"
	-@erase "$(INTDIR)\modules.sbr"
	-@erase "$(INTDIR)\movetogroup.obj"
	-@erase "$(INTDIR)\movetogroup.sbr"
	-@erase "$(INTDIR)\netlib.obj"
	-@erase "$(INTDIR)\netlib.sbr"
	-@erase "$(INTDIR)\netlibbind.obj"
	-@erase "$(INTDIR)\netlibbind.sbr"
	-@erase "$(INTDIR)\netlibhttp.obj"
	-@erase "$(INTDIR)\netlibhttp.sbr"
	-@erase "$(INTDIR)\netlibhttpproxy.obj"
	-@erase "$(INTDIR)\netlibhttpproxy.sbr"
	-@erase "$(INTDIR)\netliblog.obj"
	-@erase "$(INTDIR)\netliblog.sbr"
	-@erase "$(INTDIR)\netlibopenconn.obj"
	-@erase "$(INTDIR)\netlibopenconn.sbr"
	-@erase "$(INTDIR)\netlibopts.obj"
	-@erase "$(INTDIR)\netlibopts.sbr"
	-@erase "$(INTDIR)\netlibpktrecver.obj"
	-@erase "$(INTDIR)\netlibpktrecver.sbr"
	-@erase "$(INTDIR)\netlibsecurity.obj"
	-@erase "$(INTDIR)\netlibsecurity.sbr"
	-@erase "$(INTDIR)\netlibsock.obj"
	-@erase "$(INTDIR)\netlibsock.sbr"
	-@erase "$(INTDIR)\netlibssl.obj"
	-@erase "$(INTDIR)\netlibssl.sbr"
	-@erase "$(INTDIR)\netlibupnp.obj"
	-@erase "$(INTDIR)\netlibupnp.sbr"
	-@erase "$(INTDIR)\newplugins.obj"
	-@erase "$(INTDIR)\newplugins.sbr"
	-@erase "$(INTDIR)\openurl.obj"
	-@erase "$(INTDIR)\openurl.sbr"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\options.sbr"
	-@erase "$(INTDIR)\path.obj"
	-@erase "$(INTDIR)\path.sbr"
	-@erase "$(INTDIR)\profilemanager.obj"
	-@erase "$(INTDIR)\profilemanager.sbr"
	-@erase "$(INTDIR)\protoaccs.obj"
	-@erase "$(INTDIR)\protoaccs.sbr"
	-@erase "$(INTDIR)\protochains.obj"
	-@erase "$(INTDIR)\protochains.sbr"
	-@erase "$(INTDIR)\protocolorder.obj"
	-@erase "$(INTDIR)\protocolorder.sbr"
	-@erase "$(INTDIR)\protocols.obj"
	-@erase "$(INTDIR)\protocols.sbr"
	-@erase "$(INTDIR)\protoint.obj"
	-@erase "$(INTDIR)\protoint.sbr"
	-@erase "$(INTDIR)\protoopts.obj"
	-@erase "$(INTDIR)\protoopts.sbr"
	-@erase "$(INTDIR)\resizer.obj"
	-@erase "$(INTDIR)\resizer.sbr"
	-@erase "$(INTDIR)\searchresults.obj"
	-@erase "$(INTDIR)\searchresults.sbr"
	-@erase "$(INTDIR)\sendmsg.obj"
	-@erase "$(INTDIR)\sendmsg.sbr"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\services.sbr"
	-@erase "$(INTDIR)\sha1.obj"
	-@erase "$(INTDIR)\sha1.sbr"
	-@erase "$(INTDIR)\skin2icons.obj"
	-@erase "$(INTDIR)\skin2icons.sbr"
	-@erase "$(INTDIR)\skinicons.obj"
	-@erase "$(INTDIR)\skinicons.sbr"
	-@erase "$(INTDIR)\sounds.obj"
	-@erase "$(INTDIR)\sounds.sbr"
	-@erase "$(INTDIR)\stdinfo.obj"
	-@erase "$(INTDIR)\stdinfo.sbr"
	-@erase "$(INTDIR)\updatenotify.obj"
	-@erase "$(INTDIR)\updatenotify.sbr"
	-@erase "$(INTDIR)\url.obj"
	-@erase "$(INTDIR)\url.sbr"
	-@erase "$(INTDIR)\urldialogs.obj"
	-@erase "$(INTDIR)\urldialogs.sbr"
	-@erase "$(INTDIR)\userinfo.obj"
	-@erase "$(INTDIR)\userinfo.sbr"
	-@erase "$(INTDIR)\useronline.obj"
	-@erase "$(INTDIR)\useronline.sbr"
	-@erase "$(INTDIR)\utf.obj"
	-@erase "$(INTDIR)\utf.sbr"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\utils.sbr"
	-@erase "$(INTDIR)\vc6.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\visibility.obj"
	-@erase "$(INTDIR)\visibility.sbr"
	-@erase "$(INTDIR)\windowlist.obj"
	-@erase "$(INTDIR)\windowlist.sbr"
	-@erase "$(INTDIR)\xmlApi.obj"
	-@erase "$(INTDIR)\xmlApi.sbr"
	-@erase "$(INTDIR)\xmlParser.obj"
	-@erase "$(INTDIR)\xmlParser.sbr"
	-@erase "$(OUTDIR)\miranda32.bsc"
	-@erase "$(OUTDIR)\miranda32.map"
	-@erase "$(OUTDIR)\miranda32.pdb"
	-@erase "..\bin\Debug Unicode\miranda32.exe"
	-@erase "..\bin\Debug Unicode\miranda32.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\vc6.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\miranda32.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\commonheaders.sbr" \
	"$(INTDIR)\memory.sbr" \
	"$(INTDIR)\miranda.sbr" \
	"$(INTDIR)\modules.sbr" \
	"$(INTDIR)\addcontact.sbr" \
	"$(INTDIR)\autoaway.sbr" \
	"$(INTDIR)\button.sbr" \
	"$(INTDIR)\contacts.sbr" \
	"$(INTDIR)\database.sbr" \
	"$(INTDIR)\dbini.sbr" \
	"$(INTDIR)\dblists.sbr" \
	"$(INTDIR)\dbtime.sbr" \
	"$(INTDIR)\dbutils.sbr" \
	"$(INTDIR)\profilemanager.sbr" \
	"$(INTDIR)\findadd.sbr" \
	"$(INTDIR)\searchresults.sbr" \
	"$(INTDIR)\about.sbr" \
	"$(INTDIR)\help.sbr" \
	"$(INTDIR)\history.sbr" \
	"$(INTDIR)\idle.sbr" \
	"$(INTDIR)\ignore.sbr" \
	"$(INTDIR)\langpack.sbr" \
	"$(INTDIR)\lpservices.sbr" \
	"$(INTDIR)\netlib.sbr" \
	"$(INTDIR)\netlibbind.sbr" \
	"$(INTDIR)\netlibhttp.sbr" \
	"$(INTDIR)\netlibhttpproxy.sbr" \
	"$(INTDIR)\netliblog.sbr" \
	"$(INTDIR)\netlibopenconn.sbr" \
	"$(INTDIR)\netlibopts.sbr" \
	"$(INTDIR)\netlibpktrecver.sbr" \
	"$(INTDIR)\netlibsecurity.sbr" \
	"$(INTDIR)\netlibsock.sbr" \
	"$(INTDIR)\netlibssl.sbr" \
	"$(INTDIR)\netlibupnp.sbr" \
	"$(INTDIR)\filter.sbr" \
	"$(INTDIR)\options.sbr" \
	"$(INTDIR)\newplugins.sbr" \
	"$(INTDIR)\protoaccs.sbr" \
	"$(INTDIR)\protochains.sbr" \
	"$(INTDIR)\protocols.sbr" \
	"$(INTDIR)\protoint.sbr" \
	"$(INTDIR)\protoopts.sbr" \
	"$(INTDIR)\hotkeys.sbr" \
	"$(INTDIR)\skinicons.sbr" \
	"$(INTDIR)\sounds.sbr" \
	"$(INTDIR)\auth.sbr" \
	"$(INTDIR)\authdialogs.sbr" \
	"$(INTDIR)\awaymsg.sbr" \
	"$(INTDIR)\sendmsg.sbr" \
	"$(INTDIR)\email.sbr" \
	"$(INTDIR)\file.sbr" \
	"$(INTDIR)\fileexistsdlg.sbr" \
	"$(INTDIR)\fileopts.sbr" \
	"$(INTDIR)\filerecvdlg.sbr" \
	"$(INTDIR)\filesenddlg.sbr" \
	"$(INTDIR)\filexferdlg.sbr" \
	"$(INTDIR)\ftmanager.sbr" \
	"$(INTDIR)\url.sbr" \
	"$(INTDIR)\urldialogs.sbr" \
	"$(INTDIR)\contactinfo.sbr" \
	"$(INTDIR)\stdinfo.sbr" \
	"$(INTDIR)\userinfo.sbr" \
	"$(INTDIR)\useronline.sbr" \
	"$(INTDIR)\bmpfilter.sbr" \
	"$(INTDIR)\colourpicker.sbr" \
	"$(INTDIR)\hyperlink.sbr" \
	"$(INTDIR)\md5.sbr" \
	"$(INTDIR)\openurl.sbr" \
	"$(INTDIR)\path.sbr" \
	"$(INTDIR)\resizer.sbr" \
	"$(INTDIR)\sha1.sbr" \
	"$(INTDIR)\utf.sbr" \
	"$(INTDIR)\utils.sbr" \
	"$(INTDIR)\windowlist.sbr" \
	"$(INTDIR)\visibility.sbr" \
	"$(INTDIR)\clc.sbr" \
	"$(INTDIR)\clcfiledrop.sbr" \
	"$(INTDIR)\clcidents.sbr" \
	"$(INTDIR)\clcitems.sbr" \
	"$(INTDIR)\clcmsgs.sbr" \
	"$(INTDIR)\clcutils.sbr" \
	"$(INTDIR)\clistcore.sbr" \
	"$(INTDIR)\clistevents.sbr" \
	"$(INTDIR)\clistmenus.sbr" \
	"$(INTDIR)\clistmod.sbr" \
	"$(INTDIR)\clistsettings.sbr" \
	"$(INTDIR)\clisttray.sbr" \
	"$(INTDIR)\clui.sbr" \
	"$(INTDIR)\cluiservices.sbr" \
	"$(INTDIR)\contact.sbr" \
	"$(INTDIR)\Docking.sbr" \
	"$(INTDIR)\genmenu.sbr" \
	"$(INTDIR)\genmenuopt.sbr" \
	"$(INTDIR)\groups.sbr" \
	"$(INTDIR)\keyboard.sbr" \
	"$(INTDIR)\movetogroup.sbr" \
	"$(INTDIR)\protocolorder.sbr" \
	"$(INTDIR)\FontOptions.sbr" \
	"$(INTDIR)\FontService.sbr" \
	"$(INTDIR)\services.sbr" \
	"$(INTDIR)\extracticon.sbr" \
	"$(INTDIR)\skin2icons.sbr" \
	"$(INTDIR)\updatenotify.sbr" \
	"$(INTDIR)\xmlApi.sbr" \
	"$(INTDIR)\xmlParser.sbr"

"$(OUTDIR)\miranda32.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib shell32.lib winspool.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib version.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\miranda32.pdb" /map:"$(INTDIR)\miranda32.map" /debug /machine:I386 /out:"../bin/Debug Unicode/miranda32.exe" 
LINK32_OBJS= \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\memory.obj" \
	"$(INTDIR)\miranda.obj" \
	"$(INTDIR)\modules.obj" \
	"$(INTDIR)\addcontact.obj" \
	"$(INTDIR)\autoaway.obj" \
	"$(INTDIR)\button.obj" \
	"$(INTDIR)\contacts.obj" \
	"$(INTDIR)\database.obj" \
	"$(INTDIR)\dbini.obj" \
	"$(INTDIR)\dblists.obj" \
	"$(INTDIR)\dbtime.obj" \
	"$(INTDIR)\dbutils.obj" \
	"$(INTDIR)\profilemanager.obj" \
	"$(INTDIR)\findadd.obj" \
	"$(INTDIR)\searchresults.obj" \
	"$(INTDIR)\about.obj" \
	"$(INTDIR)\help.obj" \
	"$(INTDIR)\history.obj" \
	"$(INTDIR)\idle.obj" \
	"$(INTDIR)\ignore.obj" \
	"$(INTDIR)\langpack.obj" \
	"$(INTDIR)\lpservices.obj" \
	"$(INTDIR)\netlib.obj" \
	"$(INTDIR)\netlibbind.obj" \
	"$(INTDIR)\netlibhttp.obj" \
	"$(INTDIR)\netlibhttpproxy.obj" \
	"$(INTDIR)\netliblog.obj" \
	"$(INTDIR)\netlibopenconn.obj" \
	"$(INTDIR)\netlibopts.obj" \
	"$(INTDIR)\netlibpktrecver.obj" \
	"$(INTDIR)\netlibsecurity.obj" \
	"$(INTDIR)\netlibsock.obj" \
	"$(INTDIR)\netlibssl.obj" \
	"$(INTDIR)\netlibupnp.obj" \
	"$(INTDIR)\filter.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\newplugins.obj" \
	"$(INTDIR)\protoaccs.obj" \
	"$(INTDIR)\protochains.obj" \
	"$(INTDIR)\protocols.obj" \
	"$(INTDIR)\protoint.obj" \
	"$(INTDIR)\protoopts.obj" \
	"$(INTDIR)\hotkeys.obj" \
	"$(INTDIR)\skinicons.obj" \
	"$(INTDIR)\sounds.obj" \
	"$(INTDIR)\auth.obj" \
	"$(INTDIR)\authdialogs.obj" \
	"$(INTDIR)\awaymsg.obj" \
	"$(INTDIR)\sendmsg.obj" \
	"$(INTDIR)\email.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\fileexistsdlg.obj" \
	"$(INTDIR)\fileopts.obj" \
	"$(INTDIR)\filerecvdlg.obj" \
	"$(INTDIR)\filesenddlg.obj" \
	"$(INTDIR)\filexferdlg.obj" \
	"$(INTDIR)\ftmanager.obj" \
	"$(INTDIR)\url.obj" \
	"$(INTDIR)\urldialogs.obj" \
	"$(INTDIR)\contactinfo.obj" \
	"$(INTDIR)\stdinfo.obj" \
	"$(INTDIR)\userinfo.obj" \
	"$(INTDIR)\useronline.obj" \
	"$(INTDIR)\bmpfilter.obj" \
	"$(INTDIR)\colourpicker.obj" \
	"$(INTDIR)\hyperlink.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\openurl.obj" \
	"$(INTDIR)\path.obj" \
	"$(INTDIR)\resizer.obj" \
	"$(INTDIR)\sha1.obj" \
	"$(INTDIR)\utf.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\windowlist.obj" \
	"$(INTDIR)\visibility.obj" \
	"$(INTDIR)\clc.obj" \
	"$(INTDIR)\clcfiledrop.obj" \
	"$(INTDIR)\clcidents.obj" \
	"$(INTDIR)\clcitems.obj" \
	"$(INTDIR)\clcmsgs.obj" \
	"$(INTDIR)\clcutils.obj" \
	"$(INTDIR)\clistcore.obj" \
	"$(INTDIR)\clistevents.obj" \
	"$(INTDIR)\clistmenus.obj" \
	"$(INTDIR)\clistmod.obj" \
	"$(INTDIR)\clistsettings.obj" \
	"$(INTDIR)\clisttray.obj" \
	"$(INTDIR)\clui.obj" \
	"$(INTDIR)\cluiservices.obj" \
	"$(INTDIR)\contact.obj" \
	"$(INTDIR)\Docking.obj" \
	"$(INTDIR)\genmenu.obj" \
	"$(INTDIR)\genmenuopt.obj" \
	"$(INTDIR)\groups.obj" \
	"$(INTDIR)\keyboard.obj" \
	"$(INTDIR)\movetogroup.obj" \
	"$(INTDIR)\protocolorder.obj" \
	"$(INTDIR)\FontOptions.obj" \
	"$(INTDIR)\FontService.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\extracticon.obj" \
	"$(INTDIR)\skin2icons.obj" \
	"$(INTDIR)\updatenotify.obj" \
	"$(INTDIR)\xmlApi.obj" \
	"$(INTDIR)\xmlParser.obj" \
	"$(INTDIR)\vc6.res" \
	"..\plugins\zlib\Debug_Unicode\zlib.lib"

"..\bin\Debug Unicode\miranda32.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("miranda32.dep")
!INCLUDE "miranda32.dep"
!ELSE 
!MESSAGE Warning: cannot find "miranda32.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "miranda32 - Win32 Release" || "$(CFG)" == "miranda32 - Win32 Debug" || "$(CFG)" == "miranda32 - Win32 Release Unicode" || "$(CFG)" == "miranda32 - Win32 Debug Unicode"
SOURCE=.\core\commonheaders.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\miranda32.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\commonheaders.sbr"	"$(INTDIR)\miranda32.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\miranda32.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\commonheaders.sbr"	"$(INTDIR)\miranda32.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\core\memory.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\memory.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\memory.obj"	"$(INTDIR)\memory.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\memory.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\memory.obj"	"$(INTDIR)\memory.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\core\miranda.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\miranda.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\miranda.obj"	"$(INTDIR)\miranda.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\miranda.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\miranda.obj"	"$(INTDIR)\miranda.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\core\modules.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\modules.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\modules.obj"	"$(INTDIR)\modules.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\modules.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\modules.obj"	"$(INTDIR)\modules.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\modules\addcontact\addcontact.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\addcontact.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\addcontact.obj"	"$(INTDIR)\addcontact.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\addcontact.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\addcontact.obj"	"$(INTDIR)\addcontact.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\modules\autoaway\autoaway.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\autoaway.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\autoaway.obj"	"$(INTDIR)\autoaway.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\autoaway.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\autoaway.obj"	"$(INTDIR)\autoaway.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\button\button.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\button.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\button.obj"	"$(INTDIR)\button.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\button.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\button.obj"	"$(INTDIR)\button.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\contacts\contacts.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\contacts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\contacts.obj"	"$(INTDIR)\contacts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\contacts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\contacts.obj"	"$(INTDIR)\contacts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\database\database.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\database.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\database.obj"	"$(INTDIR)\database.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\database.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\database.obj"	"$(INTDIR)\database.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\database\dbini.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yu"../../core/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dbini.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"../../core/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dbini.obj"	"$(INTDIR)\dbini.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yu"../../core/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dbini.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"../../core/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dbini.obj"	"$(INTDIR)\dbini.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\modules\database\dblists.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\dblists.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\dblists.obj"	"$(INTDIR)\dblists.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\dblists.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\dblists.obj"	"$(INTDIR)\dblists.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\database\dbtime.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\dbtime.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\dbtime.obj"	"$(INTDIR)\dbtime.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\dbtime.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\dbtime.obj"	"$(INTDIR)\dbtime.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\database\dbutils.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\dbutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\dbutils.obj"	"$(INTDIR)\dbutils.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\dbutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\dbutils.obj"	"$(INTDIR)\dbutils.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\database\profilemanager.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\profilemanager.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\profilemanager.obj"	"$(INTDIR)\profilemanager.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\profilemanager.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\profilemanager.obj"	"$(INTDIR)\profilemanager.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\findadd\findadd.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\findadd.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\findadd.obj"	"$(INTDIR)\findadd.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\findadd.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\findadd.obj"	"$(INTDIR)\findadd.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\findadd\searchresults.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\searchresults.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\searchresults.obj"	"$(INTDIR)\searchresults.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\searchresults.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\searchresults.obj"	"$(INTDIR)\searchresults.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\help\about.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\about.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\about.obj"	"$(INTDIR)\about.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\about.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\about.obj"	"$(INTDIR)\about.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\help\help.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\help.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\help.obj"	"$(INTDIR)\help.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\help.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\help.obj"	"$(INTDIR)\help.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\history\history.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\history.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\history.obj"	"$(INTDIR)\history.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\history.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\history.obj"	"$(INTDIR)\history.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\idle\idle.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\idle.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\idle.obj"	"$(INTDIR)\idle.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\idle.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\idle.obj"	"$(INTDIR)\idle.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\ignore\ignore.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\ignore.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\ignore.obj"	"$(INTDIR)\ignore.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\ignore.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\ignore.obj"	"$(INTDIR)\ignore.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\langpack\langpack.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\langpack.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\langpack.obj"	"$(INTDIR)\langpack.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\langpack.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\langpack.obj"	"$(INTDIR)\langpack.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\langpack\lpservices.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\lpservices.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\lpservices.obj"	"$(INTDIR)\lpservices.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\lpservices.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\lpservices.obj"	"$(INTDIR)\lpservices.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\netlib\netlib.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\netlib.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\netlib.obj"	"$(INTDIR)\netlib.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\netlib.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\netlib.obj"	"$(INTDIR)\netlib.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\netlib\netlibbind.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\netlibbind.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\netlibbind.obj"	"$(INTDIR)\netlibbind.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\netlibbind.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\netlibbind.obj"	"$(INTDIR)\netlibbind.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\netlib\netlibhttp.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\netlibhttp.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\netlibhttp.obj"	"$(INTDIR)\netlibhttp.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\netlibhttp.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\netlibhttp.obj"	"$(INTDIR)\netlibhttp.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\netlib\netlibhttpproxy.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\netlibhttpproxy.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\netlibhttpproxy.obj"	"$(INTDIR)\netlibhttpproxy.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\netlibhttpproxy.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\netlibhttpproxy.obj"	"$(INTDIR)\netlibhttpproxy.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\netlib\netliblog.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\netliblog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\netliblog.obj"	"$(INTDIR)\netliblog.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\netliblog.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\netliblog.obj"	"$(INTDIR)\netliblog.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\netlib\netlibopenconn.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\netlibopenconn.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\netlibopenconn.obj"	"$(INTDIR)\netlibopenconn.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\netlibopenconn.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\netlibopenconn.obj"	"$(INTDIR)\netlibopenconn.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\netlib\netlibopts.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\netlibopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\netlibopts.obj"	"$(INTDIR)\netlibopts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\netlibopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\netlibopts.obj"	"$(INTDIR)\netlibopts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\netlib\netlibpktrecver.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\netlibpktrecver.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\netlibpktrecver.obj"	"$(INTDIR)\netlibpktrecver.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\netlibpktrecver.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\netlibpktrecver.obj"	"$(INTDIR)\netlibpktrecver.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\netlib\netlibsecurity.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\netlibsecurity.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\netlibsecurity.obj"	"$(INTDIR)\netlibsecurity.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\netlibsecurity.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\netlibsecurity.obj"	"$(INTDIR)\netlibsecurity.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\netlib\netlibsock.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\netlibsock.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\netlibsock.obj"	"$(INTDIR)\netlibsock.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\netlibsock.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\netlibsock.obj"	"$(INTDIR)\netlibsock.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\netlib\netlibssl.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\netlibssl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\netlibssl.obj"	"$(INTDIR)\netlibssl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\netlibssl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\netlibssl.obj"	"$(INTDIR)\netlibssl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\netlib\netlibupnp.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\netlibupnp.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\netlibupnp.obj"	"$(INTDIR)\netlibupnp.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\netlibupnp.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\netlibupnp.obj"	"$(INTDIR)\netlibupnp.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\options\filter.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\filter.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\filter.obj"	"$(INTDIR)\filter.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\filter.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\filter.obj"	"$(INTDIR)\filter.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\options\options.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\options.obj"	"$(INTDIR)\options.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\options.obj"	"$(INTDIR)\options.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\plugins\newplugins.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\newplugins.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\newplugins.obj"	"$(INTDIR)\newplugins.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\newplugins.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\newplugins.obj"	"$(INTDIR)\newplugins.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\protocols\protoaccs.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\protoaccs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\protoaccs.obj"	"$(INTDIR)\protoaccs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\protoaccs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\protoaccs.obj"	"$(INTDIR)\protoaccs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\protocols\protochains.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\protochains.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\protochains.obj"	"$(INTDIR)\protochains.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\protochains.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\protochains.obj"	"$(INTDIR)\protochains.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\protocols\protocols.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\protocols.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\protocols.obj"	"$(INTDIR)\protocols.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\protocols.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\protocols.obj"	"$(INTDIR)\protocols.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\protocols\protoint.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\protoint.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\protoint.obj"	"$(INTDIR)\protoint.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\protoint.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\protoint.obj"	"$(INTDIR)\protoint.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\protocols\protoopts.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\protoopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\protoopts.obj"	"$(INTDIR)\protoopts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\protoopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\protoopts.obj"	"$(INTDIR)\protoopts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\skin\hotkeys.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\hotkeys.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\hotkeys.obj"	"$(INTDIR)\hotkeys.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\hotkeys.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\hotkeys.obj"	"$(INTDIR)\hotkeys.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\skin\skinicons.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\skinicons.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\skinicons.obj"	"$(INTDIR)\skinicons.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\skinicons.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\skinicons.obj"	"$(INTDIR)\skinicons.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\skin\sounds.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\sounds.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\sounds.obj"	"$(INTDIR)\sounds.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\sounds.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\sounds.obj"	"$(INTDIR)\sounds.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\srauth\auth.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\auth.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\auth.obj"	"$(INTDIR)\auth.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\auth.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\auth.obj"	"$(INTDIR)\auth.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\srauth\authdialogs.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\authdialogs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\authdialogs.obj"	"$(INTDIR)\authdialogs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\authdialogs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\authdialogs.obj"	"$(INTDIR)\authdialogs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\srawaymsg\awaymsg.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\awaymsg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\awaymsg.obj"	"$(INTDIR)\awaymsg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\awaymsg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\awaymsg.obj"	"$(INTDIR)\awaymsg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\srawaymsg\sendmsg.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\sendmsg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\sendmsg.obj"	"$(INTDIR)\sendmsg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\sendmsg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\sendmsg.obj"	"$(INTDIR)\sendmsg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\sremail\email.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\email.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\email.obj"	"$(INTDIR)\email.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\email.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\email.obj"	"$(INTDIR)\email.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\srfile\file.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\file.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\file.obj"	"$(INTDIR)\file.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\file.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\file.obj"	"$(INTDIR)\file.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\srfile\fileexistsdlg.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\fileexistsdlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\fileexistsdlg.obj"	"$(INTDIR)\fileexistsdlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\fileexistsdlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\fileexistsdlg.obj"	"$(INTDIR)\fileexistsdlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\srfile\fileopts.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\fileopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\fileopts.obj"	"$(INTDIR)\fileopts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\fileopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\fileopts.obj"	"$(INTDIR)\fileopts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\srfile\filerecvdlg.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\filerecvdlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\filerecvdlg.obj"	"$(INTDIR)\filerecvdlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\filerecvdlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\filerecvdlg.obj"	"$(INTDIR)\filerecvdlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\srfile\filesenddlg.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\filesenddlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\filesenddlg.obj"	"$(INTDIR)\filesenddlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\filesenddlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\filesenddlg.obj"	"$(INTDIR)\filesenddlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\srfile\filexferdlg.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\filexferdlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\filexferdlg.obj"	"$(INTDIR)\filexferdlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\filexferdlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\filexferdlg.obj"	"$(INTDIR)\filexferdlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\srfile\ftmanager.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\ftmanager.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\ftmanager.obj"	"$(INTDIR)\ftmanager.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\ftmanager.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\ftmanager.obj"	"$(INTDIR)\ftmanager.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\srurl\url.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\url.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\url.obj"	"$(INTDIR)\url.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\url.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\url.obj"	"$(INTDIR)\url.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\srurl\urldialogs.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\urldialogs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\urldialogs.obj"	"$(INTDIR)\urldialogs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\urldialogs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\urldialogs.obj"	"$(INTDIR)\urldialogs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\userinfo\contactinfo.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\contactinfo.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\contactinfo.obj"	"$(INTDIR)\contactinfo.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\contactinfo.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\contactinfo.obj"	"$(INTDIR)\contactinfo.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\userinfo\stdinfo.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\stdinfo.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\stdinfo.obj"	"$(INTDIR)\stdinfo.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\stdinfo.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\stdinfo.obj"	"$(INTDIR)\stdinfo.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\userinfo\userinfo.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\userinfo.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\userinfo.obj"	"$(INTDIR)\userinfo.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\userinfo.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\userinfo.obj"	"$(INTDIR)\userinfo.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\useronline\useronline.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\useronline.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\useronline.obj"	"$(INTDIR)\useronline.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\useronline.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\useronline.obj"	"$(INTDIR)\useronline.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\utils\bmpfilter.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\bmpfilter.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\bmpfilter.obj"	"$(INTDIR)\bmpfilter.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\bmpfilter.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\bmpfilter.obj"	"$(INTDIR)\bmpfilter.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\utils\colourpicker.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\colourpicker.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\colourpicker.obj"	"$(INTDIR)\colourpicker.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\colourpicker.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\colourpicker.obj"	"$(INTDIR)\colourpicker.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\utils\hyperlink.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\hyperlink.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\hyperlink.obj"	"$(INTDIR)\hyperlink.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\hyperlink.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\hyperlink.obj"	"$(INTDIR)\hyperlink.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\utils\md5.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\md5.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\md5.obj"	"$(INTDIR)\md5.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\md5.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\md5.obj"	"$(INTDIR)\md5.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\utils\openurl.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\openurl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\openurl.obj"	"$(INTDIR)\openurl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\openurl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\openurl.obj"	"$(INTDIR)\openurl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\utils\path.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\path.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\path.obj"	"$(INTDIR)\path.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\path.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\path.obj"	"$(INTDIR)\path.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\utils\resizer.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\resizer.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\resizer.obj"	"$(INTDIR)\resizer.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\resizer.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\resizer.obj"	"$(INTDIR)\resizer.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\utils\sha1.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\sha1.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\sha1.obj"	"$(INTDIR)\sha1.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\sha1.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\sha1.obj"	"$(INTDIR)\sha1.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\utils\utf.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\utf.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\utf.obj"	"$(INTDIR)\utf.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\utf.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\utf.obj"	"$(INTDIR)\utf.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\utils\utils.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\utils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\utils.obj"	"$(INTDIR)\utils.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\utils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\utils.obj"	"$(INTDIR)\utils.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\utils\windowlist.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\windowlist.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\windowlist.obj"	"$(INTDIR)\windowlist.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\windowlist.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\windowlist.obj"	"$(INTDIR)\windowlist.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\visibility\visibility.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\visibility.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\visibility.obj"	"$(INTDIR)\visibility.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\visibility.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\visibility.obj"	"$(INTDIR)\visibility.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\clc.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\clc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\clc.obj"	"$(INTDIR)\clc.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\clc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\clc.obj"	"$(INTDIR)\clc.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\clcfiledrop.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\clcfiledrop.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\clcfiledrop.obj"	"$(INTDIR)\clcfiledrop.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\clcfiledrop.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\clcfiledrop.obj"	"$(INTDIR)\clcfiledrop.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\clcidents.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\clcidents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\clcidents.obj"	"$(INTDIR)\clcidents.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\clcidents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\clcidents.obj"	"$(INTDIR)\clcidents.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\clcitems.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\clcitems.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\clcitems.obj"	"$(INTDIR)\clcitems.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\clcitems.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\clcitems.obj"	"$(INTDIR)\clcitems.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\clcmsgs.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\clcmsgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\clcmsgs.obj"	"$(INTDIR)\clcmsgs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\clcmsgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\clcmsgs.obj"	"$(INTDIR)\clcmsgs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\clcutils.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\clcutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\clcutils.obj"	"$(INTDIR)\clcutils.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\clcutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\clcutils.obj"	"$(INTDIR)\clcutils.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\clistcore.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\clistcore.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\clistcore.obj"	"$(INTDIR)\clistcore.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\clistcore.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\clistcore.obj"	"$(INTDIR)\clistcore.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\clistevents.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\clistevents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\clistevents.obj"	"$(INTDIR)\clistevents.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\clistevents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\clistevents.obj"	"$(INTDIR)\clistevents.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\clistmenus.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\clistmenus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\clistmenus.obj"	"$(INTDIR)\clistmenus.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\clistmenus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\clistmenus.obj"	"$(INTDIR)\clistmenus.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\clistmod.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\clistmod.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\clistmod.obj"	"$(INTDIR)\clistmod.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\clistmod.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\clistmod.obj"	"$(INTDIR)\clistmod.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\clistsettings.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\clistsettings.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\clistsettings.obj"	"$(INTDIR)\clistsettings.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\clistsettings.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\clistsettings.obj"	"$(INTDIR)\clistsettings.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\clisttray.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\clisttray.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\clisttray.obj"	"$(INTDIR)\clisttray.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\clisttray.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\clisttray.obj"	"$(INTDIR)\clisttray.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\clui.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\clui.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\clui.obj"	"$(INTDIR)\clui.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\clui.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\clui.obj"	"$(INTDIR)\clui.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\cluiservices.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\cluiservices.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\cluiservices.obj"	"$(INTDIR)\cluiservices.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\cluiservices.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\cluiservices.obj"	"$(INTDIR)\cluiservices.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\contact.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\contact.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\contact.obj"	"$(INTDIR)\contact.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\contact.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\contact.obj"	"$(INTDIR)\contact.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\Docking.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\Docking.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\Docking.obj"	"$(INTDIR)\Docking.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\Docking.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\Docking.obj"	"$(INTDIR)\Docking.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\genmenu.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\genmenu.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\genmenu.obj"	"$(INTDIR)\genmenu.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\genmenu.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\genmenu.obj"	"$(INTDIR)\genmenu.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\genmenuopt.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\genmenuopt.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\genmenuopt.obj"	"$(INTDIR)\genmenuopt.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\genmenuopt.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\genmenuopt.obj"	"$(INTDIR)\genmenuopt.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\groups.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\groups.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\groups.obj"	"$(INTDIR)\groups.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\groups.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\groups.obj"	"$(INTDIR)\groups.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\keyboard.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\keyboard.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\keyboard.obj"	"$(INTDIR)\keyboard.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\keyboard.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\keyboard.obj"	"$(INTDIR)\keyboard.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\movetogroup.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\movetogroup.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\movetogroup.obj"	"$(INTDIR)\movetogroup.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\movetogroup.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\movetogroup.obj"	"$(INTDIR)\movetogroup.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\clist\protocolorder.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\protocolorder.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\protocolorder.obj"	"$(INTDIR)\protocolorder.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\protocolorder.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\protocolorder.obj"	"$(INTDIR)\protocolorder.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\fonts\FontOptions.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\FontOptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\FontOptions.obj"	"$(INTDIR)\FontOptions.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\FontOptions.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\FontOptions.obj"	"$(INTDIR)\FontOptions.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\fonts\FontService.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\FontService.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\FontService.obj"	"$(INTDIR)\FontService.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\FontService.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\FontService.obj"	"$(INTDIR)\FontService.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\fonts\services.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\services.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\services.obj"	"$(INTDIR)\services.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\services.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\services.obj"	"$(INTDIR)\services.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\icolib\extracticon.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\extracticon.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\extracticon.obj"	"$(INTDIR)\extracticon.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\extracticon.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /D "_STATIC" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\extracticon.obj"	"$(INTDIR)\extracticon.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\modules\icolib\skin2icons.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\skin2icons.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\skin2icons.obj"	"$(INTDIR)\skin2icons.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\skin2icons.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\skin2icons.obj"	"$(INTDIR)\skin2icons.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\updatenotify\updatenotify.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\updatenotify.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\updatenotify.obj"	"$(INTDIR)\updatenotify.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\updatenotify.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\updatenotify.obj"	"$(INTDIR)\updatenotify.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\xml\xmlApi.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\xmlApi.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\xmlApi.obj"	"$(INTDIR)\xmlApi.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\xmlApi.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\xmlApi.obj"	"$(INTDIR)\xmlApi.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\modules\xml\xmlParser.cpp

!IF  "$(CFG)" == "miranda32 - Win32 Release"


"$(INTDIR)\xmlParser.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"


"$(INTDIR)\xmlParser.obj"	"$(INTDIR)\xmlParser.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"


"$(INTDIR)\xmlParser.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"


"$(INTDIR)\xmlParser.obj"	"$(INTDIR)\xmlParser.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\vc6.rc

"$(INTDIR)\vc6.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!IF  "$(CFG)" == "miranda32 - Win32 Release"

"zlib - Win32 Release" : 
   cd "\Miranda\miranda\plugins\zlib"
   $(MAKE) /$(MAKEFLAGS) /F .\zlib.mak CFG="zlib - Win32 Release" 
   cd "..\..\src"

"zlib - Win32 ReleaseCLEAN" : 
   cd "\Miranda\miranda\plugins\zlib"
   $(MAKE) /$(MAKEFLAGS) /F .\zlib.mak CFG="zlib - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\src"

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

"zlib - Win32 Debug" : 
   cd "\Miranda\miranda\plugins\zlib"
   $(MAKE) /$(MAKEFLAGS) /F .\zlib.mak CFG="zlib - Win32 Debug" 
   cd "..\..\src"

"zlib - Win32 DebugCLEAN" : 
   cd "\Miranda\miranda\plugins\zlib"
   $(MAKE) /$(MAKEFLAGS) /F .\zlib.mak CFG="zlib - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\src"

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

"zlib - Win32 Release Unicode" : 
   cd "\Miranda\miranda\plugins\zlib"
   $(MAKE) /$(MAKEFLAGS) /F .\zlib.mak CFG="zlib - Win32 Release Unicode" 
   cd "..\..\src"

"zlib - Win32 Release UnicodeCLEAN" : 
   cd "\Miranda\miranda\plugins\zlib"
   $(MAKE) /$(MAKEFLAGS) /F .\zlib.mak CFG="zlib - Win32 Release Unicode" RECURSE=1 CLEAN 
   cd "..\..\src"

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

"zlib - Win32 Debug Unicode" : 
   cd "\Miranda\miranda\plugins\zlib"
   $(MAKE) /$(MAKEFLAGS) /F .\zlib.mak CFG="zlib - Win32 Debug Unicode" 
   cd "..\..\src"

"zlib - Win32 Debug UnicodeCLEAN" : 
   cd "\Miranda\miranda\plugins\zlib"
   $(MAKE) /$(MAKEFLAGS) /F .\zlib.mak CFG="zlib - Win32 Debug Unicode" RECURSE=1 CLEAN 
   cd "..\..\src"

!ENDIF 


!ENDIF 

