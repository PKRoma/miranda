# Microsoft Developer Studio Generated NMAKE File, Based on chat.dsp
!IF "$(CFG)" == ""
CFG=chat - Win32 Debug Unicode
!MESSAGE No configuration specified. Defaulting to chat - Win32 Debug Unicode.
!ENDIF 

!IF "$(CFG)" != "chat - Win32 Release" && "$(CFG)" != "chat - Win32 Debug" && "$(CFG)" != "chat - Win32 Debug Unicode" && "$(CFG)" != "chat - Win32 Release Unicode"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "chat.mak" CFG="chat - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "chat - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "chat - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "chat - Win32 Debug Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "chat - Win32 Release Unicode" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "chat - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\Release\Plugins\chat.dll"


CLEAN :
	-@erase "$(INTDIR)\Chat.res"
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\colorchooser.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\manager.obj"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\richutil.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\window.obj"
	-@erase "$(OUTDIR)\chat.exp"
	-@erase "$(OUTDIR)\chat.lib"
	-@erase "$(OUTDIR)\chat.map"
	-@erase "..\..\bin\Release\Plugins\chat.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CHAT_EXPORTS" /Fp"$(INTDIR)\chat.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x41d /fo"$(INTDIR)\Chat.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\chat.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib Version.lib /nologo /base:"0x54110000" /dll /incremental:no /pdb:"$(OUTDIR)\chat.pdb" /map:"$(INTDIR)\chat.map" /machine:I386 /out:"../../bin/Release/Plugins/chat.dll" /implib:"$(OUTDIR)\chat.lib" 
LINK32_OBJS= \
	"$(INTDIR)\clist.obj" \
	"$(INTDIR)\colorchooser.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\manager.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\richutil.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\tools.obj" \
	"$(INTDIR)\window.obj" \
	"$(INTDIR)\Chat.res"

"..\..\bin\Release\Plugins\chat.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "chat - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\bin\Debug\Plugins\chat.dll"


CLEAN :
	-@erase "$(INTDIR)\Chat.res"
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\colorchooser.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\manager.obj"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\richutil.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\window.obj"
	-@erase "$(OUTDIR)\chat.exp"
	-@erase "$(OUTDIR)\chat.lib"
	-@erase "$(OUTDIR)\chat.pdb"
	-@erase "..\..\bin\Debug\Plugins\chat.dll"
	-@erase "..\..\bin\Debug\Plugins\chat.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CHAT_EXPORTS" /Fp"$(INTDIR)\chat.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x41d /fo"$(INTDIR)\Chat.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\chat.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib Version.lib /nologo /base:"0x54110000" /dll /incremental:yes /pdb:"$(OUTDIR)\chat.pdb" /debug /machine:I386 /out:"../../bin/Debug/Plugins/chat.dll" /implib:"$(OUTDIR)\chat.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\clist.obj" \
	"$(INTDIR)\colorchooser.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\manager.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\richutil.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\tools.obj" \
	"$(INTDIR)\window.obj" \
	"$(INTDIR)\Chat.res"

"..\..\bin\Debug\Plugins\chat.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "chat - Win32 Debug Unicode"

OUTDIR=.\chat___Win32_Debug_Unicode
INTDIR=.\chat___Win32_Debug_Unicode

ALL : "..\..\bin\Debug\Plugins\chat.dll"


CLEAN :
	-@erase "$(INTDIR)\Chat.res"
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\colorchooser.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\manager.obj"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\richutil.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\window.obj"
	-@erase "$(OUTDIR)\chat.exp"
	-@erase "$(OUTDIR)\chat.lib"
	-@erase "$(OUTDIR)\chat.pdb"
	-@erase "..\..\bin\Debug\Plugins\chat.dll"
	-@erase "..\..\bin\Debug\Plugins\chat.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CHAT_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\chat.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x41d /fo"$(INTDIR)\Chat.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\chat.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Version.lib shlwapi.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\chat.pdb" /debug /machine:I386 /out:"../../bin/Debug/Plugins/chat.dll" /implib:"$(OUTDIR)\chat.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\clist.obj" \
	"$(INTDIR)\colorchooser.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\manager.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\richutil.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\tools.obj" \
	"$(INTDIR)\window.obj" \
	"$(INTDIR)\Chat.res"

"..\..\bin\Debug\Plugins\chat.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "chat - Win32 Release Unicode"

OUTDIR=.\Release_Unicode
INTDIR=.\Release_Unicode

ALL : "..\..\bin\Release Unicode\Plugins\chat.dll"


CLEAN :
	-@erase "$(INTDIR)\Chat.res"
	-@erase "$(INTDIR)\clist.obj"
	-@erase "$(INTDIR)\colorchooser.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\manager.obj"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\richutil.obj"
	-@erase "$(INTDIR)\services.obj"
	-@erase "$(INTDIR)\tools.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\window.obj"
	-@erase "$(OUTDIR)\chat.exp"
	-@erase "$(OUTDIR)\chat.lib"
	-@erase "..\..\bin\Release Unicode\Plugins\chat.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CHAT_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\chat.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x41d /fo"$(INTDIR)\Chat.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\chat.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Version.lib shlwapi.lib /nologo /dll /pdb:none /machine:I386 /out:"../../bin/Release Unicode/Plugins/chat.dll" /implib:"$(OUTDIR)\chat.lib" 
LINK32_OBJS= \
	"$(INTDIR)\clist.obj" \
	"$(INTDIR)\colorchooser.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\manager.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\richutil.obj" \
	"$(INTDIR)\services.obj" \
	"$(INTDIR)\tools.obj" \
	"$(INTDIR)\window.obj" \
	"$(INTDIR)\Chat.res"

"..\..\bin\Release Unicode\Plugins\chat.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("chat.dep")
!INCLUDE "chat.dep"
!ELSE 
!MESSAGE Warning: cannot find "chat.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "chat - Win32 Release" || "$(CFG)" == "chat - Win32 Debug" || "$(CFG)" == "chat - Win32 Debug Unicode" || "$(CFG)" == "chat - Win32 Release Unicode"
SOURCE=.\clist.c

"$(INTDIR)\clist.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\colorchooser.c

"$(INTDIR)\colorchooser.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\log.c

"$(INTDIR)\log.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\main.c

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\manager.c

"$(INTDIR)\manager.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\message.c

"$(INTDIR)\message.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\options.c

"$(INTDIR)\options.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\richutil.c

"$(INTDIR)\richutil.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\services.c

"$(INTDIR)\services.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tools.c

"$(INTDIR)\tools.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\window.c

"$(INTDIR)\window.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Chat.rc

"$(INTDIR)\Chat.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

