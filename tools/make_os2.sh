#!/bin/sh
# GCCLOAD not needed, but recommended...
#GCCLOAD=5
CC=gcc
YACC="bison -y"
LEX="flex -olex.yy.c"
RANLIB="ar -s"
PROGEXT=".exe"
export CC YACC LEX RANLIB PROGEXT
./configure --x-includes=$X11ROOT/XFree86/include -x-libraries=$X11ROOT/XFree86/lib
