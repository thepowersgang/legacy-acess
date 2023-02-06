@echo off
set DJGPP=\tools\djgpp\djgpp.env
path Z:\tools\mingwCross\bin;\projects\KernelBuildTools
mingw32-make.exe all
pause