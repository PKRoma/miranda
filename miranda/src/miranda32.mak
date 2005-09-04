# Microsoft Developer Studio Generated NMAKE File, Based on miranda32.dsp
!IF "$(CFG)" == ""
CFG=miranda32 - Win32 Debug Unicode
!MESSAGE No configuration specified. Defaulting to miranda32 - Win32 Debug Unicode.
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
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "..\bin\release\miranda32.exe" "$(OUTDIR)\miranda32.bsc"


CLEAN :
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
	-@erase "$(INTDIR)\colourpicker.obj"
	-@erase "$(INTDIR)\colourpicker.sbr"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\commonheaders.sbr"
	-@erase "$(INTDIR)\contactinfo.obj"
	-@erase "$(INTDIR)\contactinfo.sbr"
	-@erase "$(INTDIR)\contacts.obj"
	-@erase "$(INTDIR)\contacts.sbr"
	-@erase "$(INTDIR)\database.obj"
	-@erase "$(INTDIR)\database.sbr"
	-@erase "$(INTDIR)\dblists.obj"
	-@erase "$(INTDIR)\dblists.sbr"
	-@erase "$(INTDIR)\dbtime.obj"
	-@erase "$(INTDIR)\dbtime.sbr"
	-@erase "$(INTDIR)\email.obj"
	-@erase "$(INTDIR)\email.sbr"
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
	-@erase "$(INTDIR)\findadd.obj"
	-@erase "$(INTDIR)\findadd.sbr"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\help.sbr"
	-@erase "$(INTDIR)\history.obj"
	-@erase "$(INTDIR)\history.sbr"
	-@erase "$(INTDIR)\hyperlink.obj"
	-@erase "$(INTDIR)\hyperlink.sbr"
	-@erase "$(INTDIR)\idle.obj"
	-@erase "$(INTDIR)\idle.sbr"
	-@erase "$(INTDIR)\ignore.obj"
	-@erase "$(INTDIR)\ignore.sbr"
	-@erase "$(INTDIR)\langpack.obj"
	-@erase "$(INTDIR)\langpack.sbr"
	-@erase "$(INTDIR)\lpservices.obj"
	-@erase "$(INTDIR)\lpservices.sbr"
	-@erase "$(INTDIR)\miranda.obj"
	-@erase "$(INTDIR)\miranda.sbr"
	-@erase "$(INTDIR)\miranda32.pch"
	-@erase "$(INTDIR)\modules.obj"
	-@erase "$(INTDIR)\modules.sbr"
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
	-@erase "$(INTDIR)\netlibsock.obj"
	-@erase "$(INTDIR)\netlibsock.sbr"
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
	-@erase "$(INTDIR)\protochains.obj"
	-@erase "$(INTDIR)\protochains.sbr"
	-@erase "$(INTDIR)\protocols.obj"
	-@erase "$(INTDIR)\protocols.sbr"
	-@erase "$(INTDIR)\protodir.obj"
	-@erase "$(INTDIR)\protodir.sbr"
	-@erase "$(INTDIR)\resizer.obj"
	-@erase "$(INTDIR)\resizer.sbr"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\searchresults.obj"
	-@erase "$(INTDIR)\searchresults.sbr"
	-@erase "$(INTDIR)\sendmsg.obj"
	-@erase "$(INTDIR)\sendmsg.sbr"
	-@erase "$(INTDIR)\skin.obj"
	-@erase "$(INTDIR)\skin.sbr"
	-@erase "$(INTDIR)\skinicons.obj"
	-@erase "$(INTDIR)\skinicons.sbr"
	-@erase "$(INTDIR)\sounds.obj"
	-@erase "$(INTDIR)\sounds.sbr"
	-@erase "$(INTDIR)\stdinfo.obj"
	-@erase "$(INTDIR)\stdinfo.sbr"
	-@erase "$(INTDIR)\url.obj"
	-@erase "$(INTDIR)\url.sbr"
	-@erase "$(INTDIR)\urldialogs.obj"
	-@erase "$(INTDIR)\urldialogs.sbr"
	-@erase "$(INTDIR)\userinfo.obj"
	-@erase "$(INTDIR)\userinfo.sbr"
	-@erase "$(INTDIR)\useronline.obj"
	-@erase "$(INTDIR)\useronline.sbr"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\utils.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\visibility.obj"
	-@erase "$(INTDIR)\visibility.sbr"
	-@erase "$(INTDIR)\windowlist.obj"
	-@erase "$(INTDIR)\windowlist.sbr"
	-@erase "$(OUTDIR)\miranda32.bsc"
	-@erase "$(OUTDIR)\miranda32.map"
	-@erase "..\bin\release\miranda32.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"../../core/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\resource.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\miranda32.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\commonheaders.sbr" \
	"$(INTDIR)\miranda.sbr" \
	"$(INTDIR)\modules.sbr" \
	"$(INTDIR)\addcontact.sbr" \
	"$(INTDIR)\autoaway.sbr" \
	"$(INTDIR)\button.sbr" \
	"$(INTDIR)\contacts.sbr" \
	"$(INTDIR)\database.sbr" \
	"$(INTDIR)\dblists.sbr" \
	"$(INTDIR)\dbtime.sbr" \
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
	"$(INTDIR)\netlibsock.sbr" \
	"$(INTDIR)\options.sbr" \
	"$(INTDIR)\newplugins.sbr" \
	"$(INTDIR)\protochains.sbr" \
	"$(INTDIR)\protocols.sbr" \
	"$(INTDIR)\protodir.sbr" \
	"$(INTDIR)\skin.sbr" \
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
	"$(INTDIR)\url.sbr" \
	"$(INTDIR)\urldialogs.sbr" \
	"$(INTDIR)\contactinfo.sbr" \
	"$(INTDIR)\stdinfo.sbr" \
	"$(INTDIR)\userinfo.sbr" \
	"$(INTDIR)\useronline.sbr" \
	"$(INTDIR)\bmpfilter.sbr" \
	"$(INTDIR)\colourpicker.sbr" \
	"$(INTDIR)\hyperlink.sbr" \
	"$(INTDIR)\openurl.sbr" \
	"$(INTDIR)\path.sbr" \
	"$(INTDIR)\resizer.sbr" \
	"$(INTDIR)\utils.sbr" \
	"$(INTDIR)\windowlist.sbr" \
	"$(INTDIR)\visibility.sbr"

"$(OUTDIR)\miranda32.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib comctl32.lib winmm.lib version.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\miranda32.pdb" /map:"$(INTDIR)\miranda32.map" /machine:I386 /out:"../bin/release/miranda32.exe" /fixed /ALIGN:4096 /ignore:4108 
LINK32_OBJS= \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\miranda.obj" \
	"$(INTDIR)\modules.obj" \
	"$(INTDIR)\addcontact.obj" \
	"$(INTDIR)\autoaway.obj" \
	"$(INTDIR)\button.obj" \
	"$(INTDIR)\contacts.obj" \
	"$(INTDIR)\database.obj" \
	"$(INTDIR)\dblists.obj" \
	"$(INTDIR)\dbtime.obj" \
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
	"$(INTDIR)\netlibsock.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\newplugins.obj" \
	"$(INTDIR)\protochains.obj" \
	"$(INTDIR)\protocols.obj" \
	"$(INTDIR)\protodir.obj" \
	"$(INTDIR)\skin.obj" \
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
	"$(INTDIR)\url.obj" \
	"$(INTDIR)\urldialogs.obj" \
	"$(INTDIR)\contactinfo.obj" \
	"$(INTDIR)\stdinfo.obj" \
	"$(INTDIR)\userinfo.obj" \
	"$(INTDIR)\useronline.obj" \
	"$(INTDIR)\bmpfilter.obj" \
	"$(INTDIR)\colourpicker.obj" \
	"$(INTDIR)\hyperlink.obj" \
	"$(INTDIR)\openurl.obj" \
	"$(INTDIR)\path.obj" \
	"$(INTDIR)\resizer.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\windowlist.obj" \
	"$(INTDIR)\visibility.obj" \
	"$(INTDIR)\resource.res"

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

ALL : "..\bin\debug\miranda32.exe" "$(OUTDIR)\miranda32.bsc"


CLEAN :
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
	-@erase "$(INTDIR)\colourpicker.obj"
	-@erase "$(INTDIR)\colourpicker.sbr"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\commonheaders.sbr"
	-@erase "$(INTDIR)\contactinfo.obj"
	-@erase "$(INTDIR)\contactinfo.sbr"
	-@erase "$(INTDIR)\contacts.obj"
	-@erase "$(INTDIR)\contacts.sbr"
	-@erase "$(INTDIR)\database.obj"
	-@erase "$(INTDIR)\database.sbr"
	-@erase "$(INTDIR)\dblists.obj"
	-@erase "$(INTDIR)\dblists.sbr"
	-@erase "$(INTDIR)\dbtime.obj"
	-@erase "$(INTDIR)\dbtime.sbr"
	-@erase "$(INTDIR)\email.obj"
	-@erase "$(INTDIR)\email.sbr"
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
	-@erase "$(INTDIR)\findadd.obj"
	-@erase "$(INTDIR)\findadd.sbr"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\help.sbr"
	-@erase "$(INTDIR)\history.obj"
	-@erase "$(INTDIR)\history.sbr"
	-@erase "$(INTDIR)\hyperlink.obj"
	-@erase "$(INTDIR)\hyperlink.sbr"
	-@erase "$(INTDIR)\idle.obj"
	-@erase "$(INTDIR)\idle.sbr"
	-@erase "$(INTDIR)\ignore.obj"
	-@erase "$(INTDIR)\ignore.sbr"
	-@erase "$(INTDIR)\langpack.obj"
	-@erase "$(INTDIR)\langpack.sbr"
	-@erase "$(INTDIR)\lpservices.obj"
	-@erase "$(INTDIR)\lpservices.sbr"
	-@erase "$(INTDIR)\miranda.obj"
	-@erase "$(INTDIR)\miranda.sbr"
	-@erase "$(INTDIR)\modules.obj"
	-@erase "$(INTDIR)\modules.sbr"
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
	-@erase "$(INTDIR)\netlibsock.obj"
	-@erase "$(INTDIR)\netlibsock.sbr"
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
	-@erase "$(INTDIR)\protochains.obj"
	-@erase "$(INTDIR)\protochains.sbr"
	-@erase "$(INTDIR)\protocols.obj"
	-@erase "$(INTDIR)\protocols.sbr"
	-@erase "$(INTDIR)\protodir.obj"
	-@erase "$(INTDIR)\protodir.sbr"
	-@erase "$(INTDIR)\resizer.obj"
	-@erase "$(INTDIR)\resizer.sbr"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\searchresults.obj"
	-@erase "$(INTDIR)\searchresults.sbr"
	-@erase "$(INTDIR)\sendmsg.obj"
	-@erase "$(INTDIR)\sendmsg.sbr"
	-@erase "$(INTDIR)\skin.obj"
	-@erase "$(INTDIR)\skin.sbr"
	-@erase "$(INTDIR)\skinicons.obj"
	-@erase "$(INTDIR)\skinicons.sbr"
	-@erase "$(INTDIR)\sounds.obj"
	-@erase "$(INTDIR)\sounds.sbr"
	-@erase "$(INTDIR)\stdinfo.obj"
	-@erase "$(INTDIR)\stdinfo.sbr"
	-@erase "$(INTDIR)\url.obj"
	-@erase "$(INTDIR)\url.sbr"
	-@erase "$(INTDIR)\urldialogs.obj"
	-@erase "$(INTDIR)\urldialogs.sbr"
	-@erase "$(INTDIR)\userinfo.obj"
	-@erase "$(INTDIR)\userinfo.sbr"
	-@erase "$(INTDIR)\useronline.obj"
	-@erase "$(INTDIR)\useronline.sbr"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\utils.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\visibility.obj"
	-@erase "$(INTDIR)\visibility.sbr"
	-@erase "$(INTDIR)\windowlist.obj"
	-@erase "$(INTDIR)\windowlist.sbr"
	-@erase "$(OUTDIR)\miranda32.bsc"
	-@erase "$(OUTDIR)\miranda32.map"
	-@erase "$(OUTDIR)\miranda32.pdb"
	-@erase "..\bin\debug\miranda32.exe"
	-@erase "..\bin\debug\miranda32.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /Fr"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\resource.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\miranda32.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\commonheaders.sbr" \
	"$(INTDIR)\miranda.sbr" \
	"$(INTDIR)\modules.sbr" \
	"$(INTDIR)\addcontact.sbr" \
	"$(INTDIR)\autoaway.sbr" \
	"$(INTDIR)\button.sbr" \
	"$(INTDIR)\contacts.sbr" \
	"$(INTDIR)\database.sbr" \
	"$(INTDIR)\dblists.sbr" \
	"$(INTDIR)\dbtime.sbr" \
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
	"$(INTDIR)\netlibsock.sbr" \
	"$(INTDIR)\options.sbr" \
	"$(INTDIR)\newplugins.sbr" \
	"$(INTDIR)\protochains.sbr" \
	"$(INTDIR)\protocols.sbr" \
	"$(INTDIR)\protodir.sbr" \
	"$(INTDIR)\skin.sbr" \
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
	"$(INTDIR)\url.sbr" \
	"$(INTDIR)\urldialogs.sbr" \
	"$(INTDIR)\contactinfo.sbr" \
	"$(INTDIR)\stdinfo.sbr" \
	"$(INTDIR)\userinfo.sbr" \
	"$(INTDIR)\useronline.sbr" \
	"$(INTDIR)\bmpfilter.sbr" \
	"$(INTDIR)\colourpicker.sbr" \
	"$(INTDIR)\hyperlink.sbr" \
	"$(INTDIR)\openurl.sbr" \
	"$(INTDIR)\path.sbr" \
	"$(INTDIR)\resizer.sbr" \
	"$(INTDIR)\utils.sbr" \
	"$(INTDIR)\windowlist.sbr" \
	"$(INTDIR)\visibility.sbr"

"$(OUTDIR)\miranda32.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib version.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\miranda32.pdb" /map:"$(INTDIR)\miranda32.map" /debug /machine:I386 /out:"../bin/debug/miranda32.exe" 
LINK32_OBJS= \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\miranda.obj" \
	"$(INTDIR)\modules.obj" \
	"$(INTDIR)\addcontact.obj" \
	"$(INTDIR)\autoaway.obj" \
	"$(INTDIR)\button.obj" \
	"$(INTDIR)\contacts.obj" \
	"$(INTDIR)\database.obj" \
	"$(INTDIR)\dblists.obj" \
	"$(INTDIR)\dbtime.obj" \
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
	"$(INTDIR)\netlibsock.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\newplugins.obj" \
	"$(INTDIR)\protochains.obj" \
	"$(INTDIR)\protocols.obj" \
	"$(INTDIR)\protodir.obj" \
	"$(INTDIR)\skin.obj" \
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
	"$(INTDIR)\url.obj" \
	"$(INTDIR)\urldialogs.obj" \
	"$(INTDIR)\contactinfo.obj" \
	"$(INTDIR)\stdinfo.obj" \
	"$(INTDIR)\userinfo.obj" \
	"$(INTDIR)\useronline.obj" \
	"$(INTDIR)\bmpfilter.obj" \
	"$(INTDIR)\colourpicker.obj" \
	"$(INTDIR)\hyperlink.obj" \
	"$(INTDIR)\openurl.obj" \
	"$(INTDIR)\path.obj" \
	"$(INTDIR)\resizer.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\windowlist.obj" \
	"$(INTDIR)\visibility.obj" \
	"$(INTDIR)\resource.res"

"..\bin\debug\miranda32.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

OUTDIR=.\Release_Unicode
INTDIR=.\Release_Unicode
# Begin Custom Macros
OutDir=.\Release_Unicode
# End Custom Macros

ALL : "..\bin\Release Unicode\miranda32.exe" "$(OUTDIR)\miranda32.bsc"


CLEAN :
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
	-@erase "$(INTDIR)\colourpicker.obj"
	-@erase "$(INTDIR)\colourpicker.sbr"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\commonheaders.sbr"
	-@erase "$(INTDIR)\contactinfo.obj"
	-@erase "$(INTDIR)\contactinfo.sbr"
	-@erase "$(INTDIR)\contacts.obj"
	-@erase "$(INTDIR)\contacts.sbr"
	-@erase "$(INTDIR)\database.obj"
	-@erase "$(INTDIR)\database.sbr"
	-@erase "$(INTDIR)\dblists.obj"
	-@erase "$(INTDIR)\dblists.sbr"
	-@erase "$(INTDIR)\dbtime.obj"
	-@erase "$(INTDIR)\dbtime.sbr"
	-@erase "$(INTDIR)\email.obj"
	-@erase "$(INTDIR)\email.sbr"
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
	-@erase "$(INTDIR)\findadd.obj"
	-@erase "$(INTDIR)\findadd.sbr"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\help.sbr"
	-@erase "$(INTDIR)\history.obj"
	-@erase "$(INTDIR)\history.sbr"
	-@erase "$(INTDIR)\hyperlink.obj"
	-@erase "$(INTDIR)\hyperlink.sbr"
	-@erase "$(INTDIR)\idle.obj"
	-@erase "$(INTDIR)\idle.sbr"
	-@erase "$(INTDIR)\ignore.obj"
	-@erase "$(INTDIR)\ignore.sbr"
	-@erase "$(INTDIR)\langpack.obj"
	-@erase "$(INTDIR)\langpack.sbr"
	-@erase "$(INTDIR)\lpservices.obj"
	-@erase "$(INTDIR)\lpservices.sbr"
	-@erase "$(INTDIR)\miranda.obj"
	-@erase "$(INTDIR)\miranda.sbr"
	-@erase "$(INTDIR)\miranda32.pch"
	-@erase "$(INTDIR)\modules.obj"
	-@erase "$(INTDIR)\modules.sbr"
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
	-@erase "$(INTDIR)\netlibsock.obj"
	-@erase "$(INTDIR)\netlibsock.sbr"
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
	-@erase "$(INTDIR)\protochains.obj"
	-@erase "$(INTDIR)\protochains.sbr"
	-@erase "$(INTDIR)\protocols.obj"
	-@erase "$(INTDIR)\protocols.sbr"
	-@erase "$(INTDIR)\protodir.obj"
	-@erase "$(INTDIR)\protodir.sbr"
	-@erase "$(INTDIR)\resizer.obj"
	-@erase "$(INTDIR)\resizer.sbr"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\searchresults.obj"
	-@erase "$(INTDIR)\searchresults.sbr"
	-@erase "$(INTDIR)\sendmsg.obj"
	-@erase "$(INTDIR)\sendmsg.sbr"
	-@erase "$(INTDIR)\skin.obj"
	-@erase "$(INTDIR)\skin.sbr"
	-@erase "$(INTDIR)\skinicons.obj"
	-@erase "$(INTDIR)\skinicons.sbr"
	-@erase "$(INTDIR)\sounds.obj"
	-@erase "$(INTDIR)\sounds.sbr"
	-@erase "$(INTDIR)\stdinfo.obj"
	-@erase "$(INTDIR)\stdinfo.sbr"
	-@erase "$(INTDIR)\url.obj"
	-@erase "$(INTDIR)\url.sbr"
	-@erase "$(INTDIR)\urldialogs.obj"
	-@erase "$(INTDIR)\urldialogs.sbr"
	-@erase "$(INTDIR)\userinfo.obj"
	-@erase "$(INTDIR)\userinfo.sbr"
	-@erase "$(INTDIR)\useronline.obj"
	-@erase "$(INTDIR)\useronline.sbr"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\utils.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\visibility.obj"
	-@erase "$(INTDIR)\visibility.sbr"
	-@erase "$(INTDIR)\windowlist.obj"
	-@erase "$(INTDIR)\windowlist.sbr"
	-@erase "$(OUTDIR)\miranda32.bsc"
	-@erase "$(OUTDIR)\miranda32.map"
	-@erase "..\bin\Release Unicode\miranda32.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"../../core/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\resource.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\miranda32.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\commonheaders.sbr" \
	"$(INTDIR)\miranda.sbr" \
	"$(INTDIR)\modules.sbr" \
	"$(INTDIR)\addcontact.sbr" \
	"$(INTDIR)\autoaway.sbr" \
	"$(INTDIR)\button.sbr" \
	"$(INTDIR)\contacts.sbr" \
	"$(INTDIR)\database.sbr" \
	"$(INTDIR)\dblists.sbr" \
	"$(INTDIR)\dbtime.sbr" \
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
	"$(INTDIR)\netlibsock.sbr" \
	"$(INTDIR)\options.sbr" \
	"$(INTDIR)\newplugins.sbr" \
	"$(INTDIR)\protochains.sbr" \
	"$(INTDIR)\protocols.sbr" \
	"$(INTDIR)\protodir.sbr" \
	"$(INTDIR)\skin.sbr" \
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
	"$(INTDIR)\url.sbr" \
	"$(INTDIR)\urldialogs.sbr" \
	"$(INTDIR)\contactinfo.sbr" \
	"$(INTDIR)\stdinfo.sbr" \
	"$(INTDIR)\userinfo.sbr" \
	"$(INTDIR)\useronline.sbr" \
	"$(INTDIR)\bmpfilter.sbr" \
	"$(INTDIR)\colourpicker.sbr" \
	"$(INTDIR)\hyperlink.sbr" \
	"$(INTDIR)\openurl.sbr" \
	"$(INTDIR)\path.sbr" \
	"$(INTDIR)\resizer.sbr" \
	"$(INTDIR)\utils.sbr" \
	"$(INTDIR)\windowlist.sbr" \
	"$(INTDIR)\visibility.sbr"

"$(OUTDIR)\miranda32.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib comctl32.lib winmm.lib version.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\miranda32.pdb" /map:"$(INTDIR)\miranda32.map" /machine:I386 /out:"../bin/Release Unicode/miranda32.exe" /fixed /ALIGN:4096 /ignore:4108 
LINK32_OBJS= \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\miranda.obj" \
	"$(INTDIR)\modules.obj" \
	"$(INTDIR)\addcontact.obj" \
	"$(INTDIR)\autoaway.obj" \
	"$(INTDIR)\button.obj" \
	"$(INTDIR)\contacts.obj" \
	"$(INTDIR)\database.obj" \
	"$(INTDIR)\dblists.obj" \
	"$(INTDIR)\dbtime.obj" \
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
	"$(INTDIR)\netlibsock.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\newplugins.obj" \
	"$(INTDIR)\protochains.obj" \
	"$(INTDIR)\protocols.obj" \
	"$(INTDIR)\protodir.obj" \
	"$(INTDIR)\skin.obj" \
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
	"$(INTDIR)\url.obj" \
	"$(INTDIR)\urldialogs.obj" \
	"$(INTDIR)\contactinfo.obj" \
	"$(INTDIR)\stdinfo.obj" \
	"$(INTDIR)\userinfo.obj" \
	"$(INTDIR)\useronline.obj" \
	"$(INTDIR)\bmpfilter.obj" \
	"$(INTDIR)\colourpicker.obj" \
	"$(INTDIR)\hyperlink.obj" \
	"$(INTDIR)\openurl.obj" \
	"$(INTDIR)\path.obj" \
	"$(INTDIR)\resizer.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\windowlist.obj" \
	"$(INTDIR)\visibility.obj" \
	"$(INTDIR)\resource.res"

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

ALL : "..\bin\Debug Unicode\miranda32.exe" "$(OUTDIR)\miranda32.bsc"


CLEAN :
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
	-@erase "$(INTDIR)\colourpicker.obj"
	-@erase "$(INTDIR)\colourpicker.sbr"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\commonheaders.sbr"
	-@erase "$(INTDIR)\contactinfo.obj"
	-@erase "$(INTDIR)\contactinfo.sbr"
	-@erase "$(INTDIR)\contacts.obj"
	-@erase "$(INTDIR)\contacts.sbr"
	-@erase "$(INTDIR)\database.obj"
	-@erase "$(INTDIR)\database.sbr"
	-@erase "$(INTDIR)\dblists.obj"
	-@erase "$(INTDIR)\dblists.sbr"
	-@erase "$(INTDIR)\dbtime.obj"
	-@erase "$(INTDIR)\dbtime.sbr"
	-@erase "$(INTDIR)\email.obj"
	-@erase "$(INTDIR)\email.sbr"
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
	-@erase "$(INTDIR)\findadd.obj"
	-@erase "$(INTDIR)\findadd.sbr"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\help.sbr"
	-@erase "$(INTDIR)\history.obj"
	-@erase "$(INTDIR)\history.sbr"
	-@erase "$(INTDIR)\hyperlink.obj"
	-@erase "$(INTDIR)\hyperlink.sbr"
	-@erase "$(INTDIR)\idle.obj"
	-@erase "$(INTDIR)\idle.sbr"
	-@erase "$(INTDIR)\ignore.obj"
	-@erase "$(INTDIR)\ignore.sbr"
	-@erase "$(INTDIR)\langpack.obj"
	-@erase "$(INTDIR)\langpack.sbr"
	-@erase "$(INTDIR)\lpservices.obj"
	-@erase "$(INTDIR)\lpservices.sbr"
	-@erase "$(INTDIR)\miranda.obj"
	-@erase "$(INTDIR)\miranda.sbr"
	-@erase "$(INTDIR)\modules.obj"
	-@erase "$(INTDIR)\modules.sbr"
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
	-@erase "$(INTDIR)\netlibsock.obj"
	-@erase "$(INTDIR)\netlibsock.sbr"
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
	-@erase "$(INTDIR)\protochains.obj"
	-@erase "$(INTDIR)\protochains.sbr"
	-@erase "$(INTDIR)\protocols.obj"
	-@erase "$(INTDIR)\protocols.sbr"
	-@erase "$(INTDIR)\protodir.obj"
	-@erase "$(INTDIR)\protodir.sbr"
	-@erase "$(INTDIR)\resizer.obj"
	-@erase "$(INTDIR)\resizer.sbr"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\searchresults.obj"
	-@erase "$(INTDIR)\searchresults.sbr"
	-@erase "$(INTDIR)\sendmsg.obj"
	-@erase "$(INTDIR)\sendmsg.sbr"
	-@erase "$(INTDIR)\skin.obj"
	-@erase "$(INTDIR)\skin.sbr"
	-@erase "$(INTDIR)\skinicons.obj"
	-@erase "$(INTDIR)\skinicons.sbr"
	-@erase "$(INTDIR)\sounds.obj"
	-@erase "$(INTDIR)\sounds.sbr"
	-@erase "$(INTDIR)\stdinfo.obj"
	-@erase "$(INTDIR)\stdinfo.sbr"
	-@erase "$(INTDIR)\url.obj"
	-@erase "$(INTDIR)\url.sbr"
	-@erase "$(INTDIR)\urldialogs.obj"
	-@erase "$(INTDIR)\urldialogs.sbr"
	-@erase "$(INTDIR)\userinfo.obj"
	-@erase "$(INTDIR)\userinfo.sbr"
	-@erase "$(INTDIR)\useronline.obj"
	-@erase "$(INTDIR)\useronline.sbr"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\utils.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\visibility.obj"
	-@erase "$(INTDIR)\visibility.sbr"
	-@erase "$(INTDIR)\windowlist.obj"
	-@erase "$(INTDIR)\windowlist.sbr"
	-@erase "$(OUTDIR)\miranda32.bsc"
	-@erase "$(OUTDIR)\miranda32.map"
	-@erase "$(OUTDIR)\miranda32.pdb"
	-@erase "..\bin\Debug Unicode\miranda32.exe"
	-@erase "..\bin\Debug Unicode\miranda32.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_UNICODE" /Fr"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\resource.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\miranda32.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\commonheaders.sbr" \
	"$(INTDIR)\miranda.sbr" \
	"$(INTDIR)\modules.sbr" \
	"$(INTDIR)\addcontact.sbr" \
	"$(INTDIR)\autoaway.sbr" \
	"$(INTDIR)\button.sbr" \
	"$(INTDIR)\contacts.sbr" \
	"$(INTDIR)\database.sbr" \
	"$(INTDIR)\dblists.sbr" \
	"$(INTDIR)\dbtime.sbr" \
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
	"$(INTDIR)\netlibsock.sbr" \
	"$(INTDIR)\options.sbr" \
	"$(INTDIR)\newplugins.sbr" \
	"$(INTDIR)\protochains.sbr" \
	"$(INTDIR)\protocols.sbr" \
	"$(INTDIR)\protodir.sbr" \
	"$(INTDIR)\skin.sbr" \
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
	"$(INTDIR)\url.sbr" \
	"$(INTDIR)\urldialogs.sbr" \
	"$(INTDIR)\contactinfo.sbr" \
	"$(INTDIR)\stdinfo.sbr" \
	"$(INTDIR)\userinfo.sbr" \
	"$(INTDIR)\useronline.sbr" \
	"$(INTDIR)\bmpfilter.sbr" \
	"$(INTDIR)\colourpicker.sbr" \
	"$(INTDIR)\hyperlink.sbr" \
	"$(INTDIR)\openurl.sbr" \
	"$(INTDIR)\path.sbr" \
	"$(INTDIR)\resizer.sbr" \
	"$(INTDIR)\utils.sbr" \
	"$(INTDIR)\windowlist.sbr" \
	"$(INTDIR)\visibility.sbr"

"$(OUTDIR)\miranda32.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib version.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\miranda32.pdb" /map:"$(INTDIR)\miranda32.map" /debug /machine:I386 /out:"../bin/Debug Unicode/miranda32.exe" 
LINK32_OBJS= \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\miranda.obj" \
	"$(INTDIR)\modules.obj" \
	"$(INTDIR)\addcontact.obj" \
	"$(INTDIR)\autoaway.obj" \
	"$(INTDIR)\button.obj" \
	"$(INTDIR)\contacts.obj" \
	"$(INTDIR)\database.obj" \
	"$(INTDIR)\dblists.obj" \
	"$(INTDIR)\dbtime.obj" \
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
	"$(INTDIR)\netlibsock.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\newplugins.obj" \
	"$(INTDIR)\protochains.obj" \
	"$(INTDIR)\protocols.obj" \
	"$(INTDIR)\protodir.obj" \
	"$(INTDIR)\skin.obj" \
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
	"$(INTDIR)\url.obj" \
	"$(INTDIR)\urldialogs.obj" \
	"$(INTDIR)\contactinfo.obj" \
	"$(INTDIR)\stdinfo.obj" \
	"$(INTDIR)\userinfo.obj" \
	"$(INTDIR)\useronline.obj" \
	"$(INTDIR)\bmpfilter.obj" \
	"$(INTDIR)\colourpicker.obj" \
	"$(INTDIR)\hyperlink.obj" \
	"$(INTDIR)\openurl.obj" \
	"$(INTDIR)\path.obj" \
	"$(INTDIR)\resizer.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\windowlist.obj" \
	"$(INTDIR)\visibility.obj" \
	"$(INTDIR)\resource.res"

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
SOURCE=.\core\commonheaders.c

!IF  "$(CFG)" == "miranda32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\commonheaders.sbr"	"$(INTDIR)\miranda32.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /Fr"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\commonheaders.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\commonheaders.sbr"	"$(INTDIR)\miranda32.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_UNICODE" /Fr"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\commonheaders.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\core\miranda.c

!IF  "$(CFG)" == "miranda32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\miranda.obj"	"$(INTDIR)\miranda.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /Fr"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\miranda.obj"	"$(INTDIR)\miranda.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\miranda.obj"	"$(INTDIR)\miranda.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_UNICODE" /Fr"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\miranda.obj"	"$(INTDIR)\miranda.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\core\modules.c

!IF  "$(CFG)" == "miranda32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\modules.obj"	"$(INTDIR)\modules.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /Fr"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\modules.obj"	"$(INTDIR)\modules.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\modules.obj"	"$(INTDIR)\modules.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_UNICODE" /Fr"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\modules.obj"	"$(INTDIR)\modules.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\modules\addcontact\addcontact.c

!IF  "$(CFG)" == "miranda32 - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"../../core/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\addcontact.obj"	"$(INTDIR)\addcontact.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /Fr"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\addcontact.obj"	"$(INTDIR)\addcontact.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O1 /I "../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "UNICODE" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\miranda32.pch" /Yu"../../core/commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\addcontact.obj"	"$(INTDIR)\addcontact.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "miranda32 - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_NOSDK" /D "_UNICODE" /Fr"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\addcontact.obj"	"$(INTDIR)\addcontact.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\modules\autoaway\autoaway.c

"$(INTDIR)\autoaway.obj"	"$(INTDIR)\autoaway.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\button\button.c

"$(INTDIR)\button.obj"	"$(INTDIR)\button.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\contacts\contacts.c

"$(INTDIR)\contacts.obj"	"$(INTDIR)\contacts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\database\database.c

"$(INTDIR)\database.obj"	"$(INTDIR)\database.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\database\dblists.c

"$(INTDIR)\dblists.obj"	"$(INTDIR)\dblists.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\database\dbtime.c

"$(INTDIR)\dbtime.obj"	"$(INTDIR)\dbtime.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\database\profilemanager.c

"$(INTDIR)\profilemanager.obj"	"$(INTDIR)\profilemanager.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\findadd\findadd.c

"$(INTDIR)\findadd.obj"	"$(INTDIR)\findadd.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\findadd\searchresults.c

"$(INTDIR)\searchresults.obj"	"$(INTDIR)\searchresults.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\help\about.c

"$(INTDIR)\about.obj"	"$(INTDIR)\about.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\help\help.c

"$(INTDIR)\help.obj"	"$(INTDIR)\help.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\history\history.c

"$(INTDIR)\history.obj"	"$(INTDIR)\history.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\idle\idle.c

"$(INTDIR)\idle.obj"	"$(INTDIR)\idle.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\ignore\ignore.c

"$(INTDIR)\ignore.obj"	"$(INTDIR)\ignore.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\langpack\langpack.c

"$(INTDIR)\langpack.obj"	"$(INTDIR)\langpack.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\langpack\lpservices.c

"$(INTDIR)\lpservices.obj"	"$(INTDIR)\lpservices.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\netlib\netlib.c

"$(INTDIR)\netlib.obj"	"$(INTDIR)\netlib.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\netlib\netlibbind.c

"$(INTDIR)\netlibbind.obj"	"$(INTDIR)\netlibbind.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\netlib\netlibhttp.c

"$(INTDIR)\netlibhttp.obj"	"$(INTDIR)\netlibhttp.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\netlib\netlibhttpproxy.c

"$(INTDIR)\netlibhttpproxy.obj"	"$(INTDIR)\netlibhttpproxy.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\netlib\netliblog.c

"$(INTDIR)\netliblog.obj"	"$(INTDIR)\netliblog.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\netlib\netlibopenconn.c

"$(INTDIR)\netlibopenconn.obj"	"$(INTDIR)\netlibopenconn.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\netlib\netlibopts.c

"$(INTDIR)\netlibopts.obj"	"$(INTDIR)\netlibopts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\netlib\netlibpktrecver.c

"$(INTDIR)\netlibpktrecver.obj"	"$(INTDIR)\netlibpktrecver.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\netlib\netlibsock.c

"$(INTDIR)\netlibsock.obj"	"$(INTDIR)\netlibsock.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\options\options.c

"$(INTDIR)\options.obj"	"$(INTDIR)\options.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\plugins\newplugins.c

"$(INTDIR)\newplugins.obj"	"$(INTDIR)\newplugins.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\protocols\protochains.c

"$(INTDIR)\protochains.obj"	"$(INTDIR)\protochains.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\protocols\protocols.c

"$(INTDIR)\protocols.obj"	"$(INTDIR)\protocols.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\protocols\protodir.c

"$(INTDIR)\protodir.obj"	"$(INTDIR)\protodir.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\skin\skin.c

"$(INTDIR)\skin.obj"	"$(INTDIR)\skin.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\skin\skinicons.c

"$(INTDIR)\skinicons.obj"	"$(INTDIR)\skinicons.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\skin\sounds.c

"$(INTDIR)\sounds.obj"	"$(INTDIR)\sounds.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\srauth\auth.c

"$(INTDIR)\auth.obj"	"$(INTDIR)\auth.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\srauth\authdialogs.c

"$(INTDIR)\authdialogs.obj"	"$(INTDIR)\authdialogs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\srawaymsg\awaymsg.c

"$(INTDIR)\awaymsg.obj"	"$(INTDIR)\awaymsg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\srawaymsg\sendmsg.c

"$(INTDIR)\sendmsg.obj"	"$(INTDIR)\sendmsg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\sremail\email.c

"$(INTDIR)\email.obj"	"$(INTDIR)\email.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\srfile\file.c

"$(INTDIR)\file.obj"	"$(INTDIR)\file.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\srfile\fileexistsdlg.c

"$(INTDIR)\fileexistsdlg.obj"	"$(INTDIR)\fileexistsdlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\srfile\fileopts.c

"$(INTDIR)\fileopts.obj"	"$(INTDIR)\fileopts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\srfile\filerecvdlg.c

"$(INTDIR)\filerecvdlg.obj"	"$(INTDIR)\filerecvdlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\srfile\filesenddlg.c

"$(INTDIR)\filesenddlg.obj"	"$(INTDIR)\filesenddlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\srfile\filexferdlg.c

"$(INTDIR)\filexferdlg.obj"	"$(INTDIR)\filexferdlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\srurl\url.c

"$(INTDIR)\url.obj"	"$(INTDIR)\url.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\srurl\urldialogs.c

"$(INTDIR)\urldialogs.obj"	"$(INTDIR)\urldialogs.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\userinfo\contactinfo.c

"$(INTDIR)\contactinfo.obj"	"$(INTDIR)\contactinfo.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\userinfo\stdinfo.c

"$(INTDIR)\stdinfo.obj"	"$(INTDIR)\stdinfo.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\userinfo\userinfo.c

"$(INTDIR)\userinfo.obj"	"$(INTDIR)\userinfo.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\useronline\useronline.c

"$(INTDIR)\useronline.obj"	"$(INTDIR)\useronline.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\utils\bmpfilter.c

"$(INTDIR)\bmpfilter.obj"	"$(INTDIR)\bmpfilter.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\utils\colourpicker.c

"$(INTDIR)\colourpicker.obj"	"$(INTDIR)\colourpicker.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\utils\hyperlink.c

"$(INTDIR)\hyperlink.obj"	"$(INTDIR)\hyperlink.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\utils\openurl.c

"$(INTDIR)\openurl.obj"	"$(INTDIR)\openurl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\utils\path.c

"$(INTDIR)\path.obj"	"$(INTDIR)\path.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\utils\resizer.c

"$(INTDIR)\resizer.obj"	"$(INTDIR)\resizer.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\utils\utils.c

"$(INTDIR)\utils.obj"	"$(INTDIR)\utils.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\utils\windowlist.c

"$(INTDIR)\windowlist.obj"	"$(INTDIR)\windowlist.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\modules\visibility\visibility.c

"$(INTDIR)\visibility.obj"	"$(INTDIR)\visibility.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\miranda32.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\resource.rc

"$(INTDIR)\resource.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

