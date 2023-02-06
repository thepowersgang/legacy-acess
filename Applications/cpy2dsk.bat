@echo off
C:\windows\filedisk /mount 0 Z:\tools\Bochs\acessbasic\acess_seperate.img.2 x:
copy %1 x:\%2
C:\windows\filedisk /umount x: