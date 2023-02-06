@echo off
path Z:\tools\mingwCross\bin;\projects\KernelBuildTools;c:\windows;c:\windows\system32

mingw32-make.exe all

filedisk /mount 0 Z:\tools\Bochs\acessbasic\acess_seperate.img.2 x:
copy ..\ld-acess.so x:\Acess\Libs\ld-acess.so
filedisk /umount x:
filedisk /umount x:

pause
