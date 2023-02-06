@echo off
PATH \projects\KernelBuildTools;C:\Windows
SET DJGPP=\tools\djgpp\djgpp.env
\Mingw\bin\mingw32-make.exe
filedisk /mount 0 Z:\tools\Bochs\acessbasic\acess_seperate.img.2 x:
copy ..\sh2.axe x:\sh2.axe
filedisk /umount x:
pause