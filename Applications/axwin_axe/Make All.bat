@echo off
SET DJGPP=Z:\tools\djgpp\djgpp.env
path \projects\KernelBuildTools;Z:\mingw\bin;c:\windows;c:\windows\system32

mingw32-make.exe all
pause
