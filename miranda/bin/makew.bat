@echo off

rem ---------------------------------------------------------------------------
rem Main modules
rem ---------------------------------------------------------------------------

cd ..\src
nmake /f miranda32.mak CFG="miranda32 - Win32 Release Unicode"
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

cd ..\MSN
nmake /f MSN.mak CFG="msn - Win32 Release"
if errorlevel 1 goto :Error

cd ..\JabberG
nmake /f jabber.mak CFG="jabberg - Win32 Release"
if errorlevel 1 goto :Error

cd ..\AimTOC2
nmake /f aim.mak CFG="aim - Win32 Release"
if errorlevel 1 goto :Error

cd ..\YAHOO
nmake /f Yahoo.mak CFG="Yahoo - Win32 Release"
if errorlevel 1 goto :Error

cd ..\IRC
nmake /f IRC.mak CFG="IRC - Win32 Release SSL"
if errorlevel 1 goto :Error

rem ---------------------------------------------------------------------------
rem Plugins
rem ---------------------------------------------------------------------------

cd ..\..\plugins\chat
nmake /f chat.mak CFG="chat - Win32 Release Unicode"
if errorlevel 1 goto :Error

cd ..\clist
nmake /f clist.mak CFG="clist - Win32 Release Unicode"
if errorlevel 1 goto :Error

cd ..\db3x
nmake /f db3x.mak CFG="db3x - Win32 Release"
if errorlevel 1 goto :Error

cd ..\historyw
nmake /f historyw.mak CFG="historyw - Win32 Release"
if errorlevel 1 goto :Error

cd ..\import
nmake /f import.mak CFG="import - Win32 Release"
if errorlevel 1 goto :Error

cd ..\mwclist
nmake /f mwclist.mak CFG="mwclist - Win32 Release"
if errorlevel 1 goto :Error

cd ..\srmm
nmake /f srmm.mak CFG="srmm - Win32 Release Unicode"
if errorlevel 1 goto :Error

rem ---------------------------------------------------------------------------
rem Zip it
rem ---------------------------------------------------------------------------

cd "..\..\bin\Release Unicode"
copy ..\release\dbtool.exe
copy ..\release\Plugins\AIM.dll      Plugins
copy ..\release\Plugins\clist_mw.dll Plugins
copy ..\release\Plugins\dbx_3x.dll   Plugins
copy ..\release\Plugins\ICQ.dll      Plugins
copy ..\release\Plugins\import.dll   Plugins
copy ..\release\Plugins\IRC.dll      Plugins
copy ..\release\Plugins\jabber.dll   Plugins
copy ..\release\Plugins\msn.dll      Plugins
copy ..\release\Plugins\Yahoo.dll    Plugins

for /F "tokens=1,2 delims= " %%i in (..\build.no) do call :Pack %%i %%j
exit

:Pack
del %Temp%\miranda-v%1a%2w.zip
7za.exe a -tzip -r -mx=9 %Temp%\miranda-v%1a%2w.zip ./*  ..\ChangeLog
goto :eof

:Error
echo Make failed
exit
