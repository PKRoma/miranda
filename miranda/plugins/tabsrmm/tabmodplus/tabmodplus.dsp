# Microsoft Developer Studio Project File - Name="tabmodplus" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=tabmodplus - Win32 Release
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "tabmodplus.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "tabmodplus.mak" CFG="tabmodplus - Win32 Release"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "tabmodplus - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabmodplus - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabmodplus - Win32 Unicode Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tabmodplus - Win32 Unicode Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tabmodplus - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /Ob2 /GF PRECOMP_VC7_TOBEREMOVED /c
# ADD CPP /nologo /MD /W3 /GX /O1 /Ob2 /I "../../../include" /I "../api" /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib msimg32.lib /nologo /base:"0x67100000" /subsystem:windows /dll /map /machine:IX86 /out:".\$(ConfigurationName)\tabmodplus.dll" /pdbtype:sept /filealign:0x200
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib msimg32.lib /nologo /base:"0x67100000" /subsystem:windows /dll /map /machine:IX86 /out:"../../../bin/Release/plugins/tabmodplus.dll" /pdbtype:sept /filealign:0x200
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "tabmodplus - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\debug"
# PROP BASE Intermediate_Dir ".\debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /ZI /Od /FR /GZ PRECOMP_VC7_TOBEREMOVED /c
# ADD CPP /nologo /MD /W3 /GX /ZI /Od /I "../../../include" /I "../api" /FR /GZ PRECOMP_VC7_TOBEREMOVED /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x417 /d "NDEBUG"
# ADD RSC /l 0x417 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib msimg32.lib /nologo /base:"0x67100000" /subsystem:windows /dll /incremental:no /pdb:".\release\tabmodplus.pdb" /map /debug /machine:IX86 /out:"$(ConfigurationName)\tabmodplus.dll" /implib:".\release/tabmodplus.lib" /pdbtype:sept /filealign:0x200
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib msimg32.lib /nologo /base:"0x67100000" /subsystem:windows /dll /incremental:no /pdb:".\release\tabmodplus.pdb" /map /debug /machine:IX86 /out:"../../../bin/Debug/plugins/tabmodplus.dll" /implib:".\release/tabmodplus.lib" /pdbtype:sept /filealign:0x200
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "tabmodplus - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "$(ConfigurationName)"
# PROP BASE Intermediate_Dir "$(ConfigurationName)"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\UNICODE_RELEASE"
# PROP Intermediate_Dir ".\UNICODE_RELEASE"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /Ob2 /D "_UNICODE" /Fo".\Release/" /GF PRECOMP_VC7_TOBEREMOVED /c
# ADD CPP /nologo /MD /W3 /GX /O1 /Ob2 /I "../../../include" /I "../api" /D "_UNICODE" /D "UNICODE" /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib msimg32.lib /nologo /base:"0x67100000" /subsystem:windows /dll /pdb:".\Release\tabmodplus.pdb" /map:".\Release\tabmodplus.map" /machine:IX86 /out:".\$(ConfigurationName)\tabmodplus.dll" /implib:".\Release/tabmodplus.lib" /pdbtype:sept /filealign:0x200
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib msimg32.lib /nologo /base:"0x67100000" /subsystem:windows /dll /map /machine:IX86 /out:"../../../bin/Release Unicode/plugins/tabmodplus.dll" /pdbtype:sept /filealign:0x200
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "tabmodplus - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "$(ConfigurationName)"
# PROP BASE Intermediate_Dir "$(ConfigurationName)"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\UNICODE_DEBUG"
# PROP Intermediate_Dir ".\UNICODE_DEBUG"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /ZI /Od /D "_UNICODE" /D "PRECOMP_VC7_TOBEREMOVED" /FR /Fo".\debug/" /Fd".\debug/" /GZ "DEBUG" /c
# ADD CPP /nologo /MD /W3 /GX /ZI /Od /I "../../../include" /I "../api" /D "_UNICODE" /D "UNICODE" /FR /GZ "UNICODE_DEBUG" /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x417 /d "NDEBUG"
# ADD RSC /l 0x417 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x67100000" /subsystem:windows /dll /incremental:no /pdb:".\release\tabmodplus.pdb" /map:".\debug\tabmodplus.map" /debug /machine:IX86 /implib:".\release/tabmodplus.lib" /pdbtype:sept /filealign:0x200
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x67100000" /subsystem:windows /dll /incremental:no /map /debug /machine:IX86 /out:"../../../bin/Debug Unicode/plugins/tabmodplus.dll" /pdbtype:sept /filealign:0x200
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "tabmodplus - Win32 Release"
# Name "tabmodplus - Win32 Debug"
# Name "tabmodplus - Win32 Unicode Release"
# Name "tabmodplus - Win32 Unicode Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=main.c
DEP_CPP_MAIN_=\
	"..\..\..\include\m_database.h"\
	"..\..\..\include\m_icolib.h"\
	"..\..\..\include\m_langpack.h"\
	"..\..\..\include\m_message.h"\
	"..\..\..\include\m_options.h"\
	"..\..\..\include\m_plugins.h"\
	"..\..\..\include\m_protocols.h"\
	"..\..\..\include\m_system.h"\
	"..\..\..\include\m_utils.h"\
	"..\..\..\include\newpluginapi.h"\
	"..\..\..\include\statusmodes.h"\
	"..\api\m_fingerprint.h"\
	".\m_msg_buttonsbar.h"\
	".\tabmodplus.h"\
	

!IF  "$(CFG)" == "tabmodplus - Win32 Release"

# ADD CPP /nologo /GX /O1

!ELSEIF  "$(CFG)" == "tabmodplus - Win32 Debug"

# ADD CPP /nologo /GX /Od /FR /GZ

!ELSEIF  "$(CFG)" == "tabmodplus - Win32 Unicode Release"

# ADD CPP /nologo /GX /O1

!ELSEIF  "$(CFG)" == "tabmodplus - Win32 Unicode Debug"

# ADD CPP /nologo /GX /Od /FR /GZ

!ENDIF 

# End Source File
# Begin Source File

SOURCE=options.c
DEP_CPP_OPTIO=\
	"..\..\..\include\m_database.h"\
	"..\..\..\include\m_icolib.h"\
	"..\..\..\include\m_langpack.h"\
	"..\..\..\include\m_message.h"\
	"..\..\..\include\m_options.h"\
	"..\..\..\include\m_plugins.h"\
	"..\..\..\include\m_protocols.h"\
	"..\..\..\include\m_system.h"\
	"..\..\..\include\m_utils.h"\
	"..\..\..\include\newpluginapi.h"\
	"..\..\..\include\statusmodes.h"\
	"..\api\m_fingerprint.h"\
	".\m_msg_buttonsbar.h"\
	".\tabmodplus.h"\
	

!IF  "$(CFG)" == "tabmodplus - Win32 Release"

# ADD CPP /nologo /GX /O1

!ELSEIF  "$(CFG)" == "tabmodplus - Win32 Debug"

# ADD CPP /nologo /GX /Od /FR /GZ

!ELSEIF  "$(CFG)" == "tabmodplus - Win32 Unicode Release"

# ADD CPP /nologo /GX /O1

!ELSEIF  "$(CFG)" == "tabmodplus - Win32 Unicode Debug"

# ADD CPP /nologo /GX /Od /FR /GZ

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=resource.h
# End Source File
# Begin Source File

SOURCE=.\tabmodplus.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\icons\imgclose.ico
# End Source File
# Begin Source File

SOURCE=.\icons\imgopen.ico
# End Source File
# Begin Source File

SOURCE=.\tabmodplus.rc
# End Source File
# End Group
# End Target
# End Project
