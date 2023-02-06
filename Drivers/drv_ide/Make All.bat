@echo off
PATH \projects\Acess\bin;\tools\unix\usr\local\wbin;C:\Windows
SET DJGPP=\tools\djgpp\djgpp.env
\tools\mingw32\bin\mingw32-make.exe all
filedisk /mount 0 Z:\tools\Bochs\acessbasic\acess_seperate.img.2 x:
copy kmod_fdd.akm x:\kmod_fdd.akm
filedisk /umount x:
pause