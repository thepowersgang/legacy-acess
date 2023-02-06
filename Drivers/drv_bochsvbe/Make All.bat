@echo off
PATH \projects\KernelBuildTools;\tools\mingwCross\bin;C:\Windows
SET DJGPP=\tools\djgpp\djgpp.env

\tools\mingw32\bin\mingw32-make.exe all

filedisk /mount 0 Z:\tools\Bochs\acessbasic\acess_seperate.img.2 x:
copy kmod_bochsvbe.akm x:\kmod_bochsvbe.akm
filedisk /umount x:

pause