@ECHO OFF
rem
rem   this is a quick and dirty batch file to recompile control.exe
rem   in a native windows environment using LCC. You may want to edit 
rem   the following line, which should point to your LCC base directory:
rem 

set LCCDIR=C:\LCC

rem   ---------------------------------------------------
rem    it's safe not to change anything behind this line
rem   ---------------------------------------------------

if exist control.obj del control.obj
%LCCDIR%\bin\lcc.exe -g2 -I%LCCDIR%\include\ -DWIN32 control.c
%LCCDIR%\bin\lcclnk.exe -o control2.exe control.obj %LCCDIR%\lib\shell32.lib
if exist control.obj del control.obj

:EXIT
