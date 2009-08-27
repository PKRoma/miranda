@echo off

rem build tabSRMM with msbuild.exe from Windows SDK
rem requires a properly installed Windows SDK (Version 6 or later)

msbuild.exe tabsrmm_9.sln /t:rebuild /p:Configuration="Release Unicode" /p:Platform="Win32"
msbuild.exe tabsrmm_9.sln /t:rebuild /p:Configuration="Release Unicode" /p:Platform="x64"
msbuild.exe tabsrmm_9.sln /t:rebuild /p:Configuration="Release" /p:Platform="Win32"

