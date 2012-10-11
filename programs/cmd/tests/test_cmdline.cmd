@echo off
mkdir foobar
cd foobar
echo file1 > file1

rem Basic test of command line. Note a section prefix per command
rem to resync, as wine does not output anything in these cases yet.
echo --- Test 1
cmd.exe /c echo Line1
cmd.exe /c echo "Line2"
echo --- Test 2
cmd.exe /c echo Test quotes "&" work
echo --- Test 3
cmd.exe /c echo "&"
echo --- Test 4
cmd.exe /c echo "<"
echo --- Test 5
cmd.exe /c echo ">"
echo --- Test 6
cmd.exe /c echo "\"
echo --- Test 7
cmd.exe /c echo "|"
echo --- Test 8
cmd.exe /c echo "`"
echo --- Test 9
cmd.exe /c echo """
echo --- Test 10
echo on > file3
@type file3
@echo off
echo --- Test 11
cmd.exe /c echo on >file3
@type file3
@echo off
echo --- Test 12
cmd.exe /c "echo passed1"
echo --- Test 13
cmd.exe /c " echo passed2 "
echo --- Test 14
cmd.exe /c "dir /ad ..\fooba* /b"
echo --- Test 15
cmd.exe /cecho No whitespace
echo --- Test 16
cmd.exe /c
echo --- Test 17
cmd.exe /c@space@
echo --- Test 18
rem Ensure no interactive prompting when cmd.exe /c or /k
echo file2 > file2
cmd.exe /c copy file1 file2 >nul
echo No prompts or I would not get here1
rem - Try cmd.exe /k as well
cmd.exe /k "copy file1 file2 >nul && exit"
echo No prompts or I would not get here2

rem Non existing variable expansion is as per command line, i.e. left as-is
cmd.exe /c echo %%hello1%%
cmd.exe /c echo %%hello2
cmd.exe /c echo %%hello3^:h=t%%
cmd.exe /c echo %%hello4%%%%

rem Cannot issue a call from cmd.exe /c
cmd.exe /c call :hello5

rem %1-9 has no meaning
cmd.exe /c echo one = %%1

rem for loop vars need expanding
cmd.exe /c for /L %%i in (1,1,5) do @echo %%i

rem goto's are ineffective
cmd.exe /c goto :fred
cmd.exe /c goto eof

rem - %var% is expanded at read time, not execute time
set var=11
cmd.exe /c "set var=22 && setlocal && set var=33 && endlocal && echo var contents: %%var%%"

rem - endlocal ineffective on cmd.exe /c lines
cmd.exe /c "set var=22 && setlocal && set var=33 && endlocal && set var"
set var=99

rem - Environment is inherited ok
cmd.exe /c ECHO %%VAR%%

rem - Exit works
cmd.exe /c exit

cd ..
rd foobar /s /q
