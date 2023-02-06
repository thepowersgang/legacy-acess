@echo off
PATH \projects\KernelBuildTools
SET DJGPP=Z:\tools\djgpp\djgpp.env
\Mingw\bin\mingw32-make.exe acessos
copy AcessOS.bin B:\AcessOS.bin
pause