@echo off

for /F "tokens=1,2,3 delims= " %%i in (build.no) do call :WriteVer %%i %%j %%k

md Release
md "Release/Icons"
md "Release/Plugins"

rem ---------------------------------------------------------------------------
rem Main modules
rem ---------------------------------------------------------------------------

cd ..\src
nmake /f miranda32.mak CFG="miranda32 - Win32 Release"
if errorlevel 1 goto :Error

cd ..\..\miranda-tools\dbtool
nmake /f dbtool.mak CFG="dbtool - Win32 Release"
if errorlevel 1 goto :Error

rem ---------------------------------------------------------------------------
rem Protocols
rem ---------------------------------------------------------------------------

cd ..\..\miranda\protocols\IcqOscarJ
nmake /f IcqOscar8.mak CFG="icqoscar8 - Win32 Release"
if errorlevel 1 goto :Error

cd icons_pack
nmake /f ICONS.mak CFG="ICONS - Win32 Release"
if errorlevel 1 goto :Error

cd ..\..\MSN
nmake /f MSN.mak CFG="msn - Win32 Release"
if errorlevel 1 goto :Error

cd ..\JabberG
nmake /f jabber.mak CFG="jabberg - Win32 Release"
if errorlevel 1 goto :Error

cd ..\AimOscar
nmake /f aimoscar.mak CFG="aim - Win32 Release"
if errorlevel 1 goto :Error

cd ..\YAHOO
nmake /f Yahoo.mak CFG="Yahoo - Win32 Release"
if errorlevel 1 goto :Error

cd ..\IRC
nmake /f IRC.mak CFG="IRC - Win32 Release"
if errorlevel 1 goto :Error

rem ---------------------------------------------------------------------------
rem Plugins
rem ---------------------------------------------------------------------------

cd ..\..\plugins\chat
nmake /f chat.mak CFG="chat - Win32 Release"
if errorlevel 1 goto :Error

cd ..\clist
nmake /f clist.mak CFG="clist - Win32 Release"
if errorlevel 1 goto :Error

cd ..\clist_nicer
nmake /f clist.mak CFG="clist_nicer - Win32 Release"
if errorlevel 1 goto :Error

cd ..\db3x
nmake /f db3x.mak CFG="db3x - Win32 Release"
if errorlevel 1 goto :Error

cd ..\db3x_mmap
nmake /f db3x_mmap.mak CFG="db3x_mmap - Win32 Release"
if errorlevel 1 goto :Error

cd ..\import
nmake /f import.mak CFG="import - Win32 Release"
if errorlevel 1 goto :Error

cd ..\loadavatars
nmake /f avatars.mak CFG="loadavatars - Win32 Release"
if errorlevel 1 goto :Error

cd ..\modernb
nmake /f modernb.mak CFG="modernb - Win32 Release"
if errorlevel 1 goto :Error

cd ..\mwclist
nmake /f mwclist.mak CFG="mwclist - Win32 Release"
if errorlevel 1 goto :Error

cd ..\png2dib
nmake /f png2dib.mak CFG="png2dib - Win32 Release"
if errorlevel 1 goto :Error

cd ..\scriver
nmake /f scriver.mak CFG="scriver - Win32 Release"
if errorlevel 1 goto :Error

cd ..\srmm
nmake /f srmm.mak CFG="srmm - Win32 Release"
if errorlevel 1 goto :Error

cd ..\tabSRMM
nmake /f tabSRMM.mak CFG="tabSRMM - Win32 Release"
if errorlevel 1 goto :Error

rem ---------------------------------------------------------------------------
rem Zip it
rem ---------------------------------------------------------------------------

cd ..\..\bin\Release

dir /B /S *.dll | rebaser /NOCRC

for /F "tokens=1,2,3 delims= " %%i in (..\build.no) do call :Pack %%i %%j %%k
goto :eof

:WriteVer
set /A Version = %1
set /A SubVersion = %2
call :WriteVer2 %Version% %SubVersion% %3
goto :eof

:WriteVer2
echo #include ^<windows.h^>                                                         >..\src\version.rc
echo #include ^<winres.h^>                                                         >>..\src\version.rc
echo #ifndef _MAC                                                                  >>..\src\version.rc
echo ///////////////////////////////////////////////////////////////////////////// >>..\src\version.rc
echo //                                                                            >>..\src\version.rc
echo // Version                                                                    >>..\src\version.rc
echo //                                                                            >>..\src\version.rc
echo.                                                                              >>..\src\version.rc
echo VS_VERSION_INFO VERSIONINFO                                                   >>..\src\version.rc
echo  FILEVERSION 0,%1,%2,%3                                                       >>..\src\version.rc
echo  PRODUCTVERSION 0,%1,%2,%3                                                    >>..\src\version.rc
echo  FILEFLAGSMASK 0x3fL                                                          >>..\src\version.rc
echo #ifdef _DEBUG                                                                 >>..\src\version.rc
echo  FILEFLAGS 0x1L                                                               >>..\src\version.rc
echo #else                                                                         >>..\src\version.rc
echo  FILEFLAGS 0x0L                                                               >>..\src\version.rc
echo #endif                                                                        >>..\src\version.rc
echo  FILEOS 0x40004L                                                              >>..\src\version.rc
echo  FILETYPE 0x1L                                                                >>..\src\version.rc
echo  FILESUBTYPE 0x0L                                                             >>..\src\version.rc
echo BEGIN                                                                         >>..\src\version.rc
echo     BLOCK "StringFileInfo"                                                    >>..\src\version.rc
echo     BEGIN                                                                     >>..\src\version.rc
echo         BLOCK "000004b0"                                                      >>..\src\version.rc
echo         BEGIN                                                                 >>..\src\version.rc
echo             VALUE "Comments", "Licensed under the terms of the GNU General Public License\0" >>..\src\version.rc
echo             VALUE "CompanyName", " \0"                                        >>..\src\version.rc
echo             VALUE "FileDescription", "Miranda IM\0"                           >>..\src\version.rc
echo             VALUE "FileVersion", "0.%1.%2 alpha build #%3\0"                  >>..\src\version.rc
echo             VALUE "InternalName", "miranda32\0"                               >>..\src\version.rc
echo             VALUE "LegalCopyright", "Copyright © 2000-2007 Miranda IM Project. This software is released under the terms of the GNU General Public License.\0"    >>..\src\version.rc
echo             VALUE "LegalTrademarks", "\0"                                     >>..\src\version.rc
echo             VALUE "OriginalFilename", "miranda32.exe\0"                       >>..\src\version.rc
echo             VALUE "PrivateBuild", "\0"                                        >>..\src\version.rc
echo             VALUE "ProductName", "Miranda IM\0"                               >>..\src\version.rc
echo             VALUE "ProductVersion", "0.%1.%2 alpha build #%3\0"               >>..\src\version.rc
echo             VALUE "SpecialBuild", "\0"                                        >>..\src\version.rc
echo         END                                                                   >>..\src\version.rc
echo     END                                                                       >>..\src\version.rc
echo     BLOCK "VarFileInfo"                                                       >>..\src\version.rc
echo     BEGIN                                                                     >>..\src\version.rc
echo         VALUE "Translation", 0x0, 1200                                        >>..\src\version.rc
echo     END                                                                       >>..\src\version.rc
echo END                                                                           >>..\src\version.rc
echo.                                                                              >>..\src\version.rc
echo #endif    // !_MAC                                                            >>..\src\version.rc
echo.                                                                              >>..\src\version.rc

for /F "delims=-/. tokens=1,2,3" %%i in ('date /T') do call :SetBuildDate %%i %%j %%k
for /F "delims=:/. tokens=1,2" %%i in ('time /T') do call :SetBuildTime %%i %%j

echo ^<?xml version="1.0" ?^>                                                      >%temp%\index.xml
echo ^<rss version="2.0"^>                                                         >>%temp%\index.xml
echo      ^<channel^>                                                              >>%temp%\index.xml
echo           ^<title^>Miranda IM Alpha Builds^</title^>                          >>%temp%\index.xml
echo           ^<link^>http://files.miranda-im.org/builds/^</link^>                >>%temp%\index.xml
echo           ^<language^>en-us^</language^>                                      >>%temp%\index.xml
echo           ^<lastBuildDate^>%yy%-%mm%-%dd% %hh%:%mn%^</lastBuildDate^>         >>%temp%\index.xml
echo           ^<item^>                                                            >>%temp%\index.xml
echo                ^<title^>Miranda 0.%1.%2 alpha %3^</title^>                    >>%temp%\index.xml
echo 			   ^<link^>http://files.miranda-im.org/builds/?%yy%%mm%%dd%%hh%%mn%^</link^> >>%temp%\index.xml
echo                ^<description^>                                                >>%temp%\index.xml
echo                     Miranda 0.%1.%2 alpha %3 is now available at http://files.miranda-im.org/builds/miranda-v%1a%3.zip >>%temp%\index.xml
echo                ^</description^>                                               >>%temp%\index.xml
echo                ^<pubDate^>%yy%-%mm%-%dd% %hh%:%mn%^</pubDate^>                 >>%temp%\index.xml
echo                ^<category^>Nightly Builds</category^>                         >>%temp%\index.xml
echo                ^<author^>Miranda IM Development Team^</author^>               >>%temp%\index.xml
echo           ^</item^>                                                           >>%temp%\index.xml
echo      ^</channel^>                                                             >>%temp%\index.xml
echo ^</rss^>                                                                      >>%temp%\index.xml
goto :eof

:SetBuildDate
set dd=%1
set mm=%2
set yy=%3
goto :eof

:SetBuildTime
set hh=%1
set mn=%2
goto :eof

:Pack
if %2 == 00 (
   set FileVer=v%1a%3.zip
) else (
   set FileVer=v%1%2a%3.zip
)

del "%Temp%\miranda-%FileVer%"
7z.exe a -tzip -r -mx=9 "%Temp%\miranda-%FileVer%" ./* ..\ChangeLog.txt

rd /Q /S %Temp%\pdba >nul
md %Temp%\pdba
md %Temp%\pdba\plugins

copy ..\..\src\Release\miranda32.pdb                   %Temp%\pdba
copy ..\..\..\miranda-tools\dbtool\Release\dbtool.pdb  %Temp%\pdba
rem  Protocols
copy ..\..\protocols\AimOscar\Release\AimOSCAR.pdb     %Temp%\pdba\plugins
copy ..\..\protocols\IcqOscarJ\Release\ICQ.pdb         %Temp%\pdba\plugins
copy ..\..\protocols\IRC\Release\IRC.pdb               %Temp%\pdba\plugins
copy ..\..\protocols\JabberG\Release\jabber.pdb        %Temp%\pdba\plugins
copy ..\..\protocols\MSN\Release\MSN.pdb               %Temp%\pdba\plugins
copy ..\..\protocols\Yahoo\Release\Yahoo.pdb           %Temp%\pdba\plugins
rem  Plugins
copy ..\..\plugins\chat\Release\chat.pdb               %Temp%\pdba\plugins
copy ..\..\plugins\clist\Release\clist_classic.pdb     %Temp%\pdba\plugins
copy ..\..\plugins\clist_nicer\Release\clist_nicer.pdb %Temp%\pdba\plugins
copy ..\..\plugins\db3x\Release\dbx_3x.pdb             %Temp%\pdba\plugins
copy ..\..\plugins\db3x_mmap\Release\dbx_mmap.pdb      %Temp%\pdba\plugins
copy ..\..\plugins\import\Release\import.pdb           %Temp%\pdba\plugins
copy ..\..\plugins\loadavatars\Release\loadavatars.pdb %Temp%\pdbw\plugins
copy ..\..\plugins\modernb\Release\clist_modern.pdb    %Temp%\pdba\plugins
copy ..\..\plugins\mwclist\Release\clist_mw.pdb        %Temp%\pdba\plugins
copy ..\..\plugins\png2dib\Release\png2dib.pdb         %Temp%\pdba\plugins
copy ..\..\plugins\scriver\Release\scriver.pdb         %Temp%\pdba\plugins
copy ..\..\plugins\srmm\Release\srmm.pdb               %Temp%\pdba\plugins
copy ..\..\plugins\tabSRMM\Release\tabSRMM.pdb         %Temp%\pdba\plugins

del "%Temp%\miranda-pdb-%FileVer%"
7z.exe a -tzip -r -mx=9 "%Temp%\miranda-pdb-%FileVer%" %Temp%\pdba/*
rd /Q /S %Temp%\pdba
goto :eof

:Error
echo Make failed
goto :eof
