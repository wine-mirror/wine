@echo off
rem #!/bin/sh
rem GCCLOAD not needed, but recommended...
rem set GCCLOAD=5
set MAKE=make
set CC=gcc
set CFLAGS=-O2 -Zmtd -D__ST_MT_ERRNO__
set YACC=bison -y
set LEX=flex -olex.yy.c
set RANLIB=ar -s
set PROGEXT=.exe
rem export CC CFLAGS YACC LEX RANLIB PROGEXT
sh configure --x-includes=%X11ROOT%/XFree86/include -x-libraries=%X11ROOT%/XFree86/lib
