@echo off
REM //
REM // This script redirects output to plink with the password as a command line option
REM // since it can not be done via python because the env() option can not take any flags
REM //
c:\python\python.exe tp.py %1 %2 %3 %4 %5 %6 %7 %8 %9
