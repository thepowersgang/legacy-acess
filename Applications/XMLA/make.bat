@echo off
gcc -c main.c -o main.o

gcc -o xmla.exe main.o -L\projects\Libraries -lxmlparse
pause
