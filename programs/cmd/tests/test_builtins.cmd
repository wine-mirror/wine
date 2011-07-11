echo Tests for cmd's builtin commands

@echo on
echo ------------ Testing 'echo' [ON] --------------
echo word
echo 'singlequotedword'
echo "doublequotedword"
@echo at-echoed-word
echo "/?"
echo.
echo .
echo.word
echo .word
echo word@space@
echo word@space@@space@

@echo off
echo ------------ Testing 'echo' [OFF] --------------
echo word
echo 'singlequotedword'
echo "doublequotedword"
@echo at-echoed-word
echo "/?"
echo.
echo .
echo.word
echo .word
echo word@space@
echo word@space@@space@

echo ------------ Testing 'set' --------------
echo set "FOO=bar" should not include the quotes in the variable value
set "FOO=bar"
echo %FOO%

echo ------------ Testing variable expansion --------------
echo ~dp0 should be directory containing batch file
echo %~dp0
mkdir dummydir
cd dummydir
echo %~dp0
cd ..
rmdir dummydir
echo CD value %CD%
echo %%
echo P%
echo %P
echo %UNKNOWN%S
echo P%UNKNOWN%
echo P%UNKNOWN%S
echo %ERRORLEVEL
echo %ERRORLEVEL%
echo %ERRORLEVEL%%ERRORLEVEL%
echo %ERRORLEVEL%ERRORLEVEL%
echo %ERRORLEVEL%%
echo %ERRORLEVEL%%%
echo P%ERRORLEVEL%
echo %ERRORLEVEL%S
echo P%ERRORLEVEL%S

echo ------------ Testing if/else --------------
echo if/else should work with blocks
if 0 == 0 (
  echo if seems to work
) else (
  echo if seems to be broken
)
if 1 == 0 (
  echo else seems to be broken
) else (
  echo else seems to work
)
echo Testing case sensitivity with and without /i option
if bar==BAR echo if does not default to case sensitivity
if not bar==BAR echo if seems to default to case sensitivity
if /i foo==FOO echo if /i seems to work
if /i not foo==FOO echo if /i seems to be broken
if /I foo==FOO echo if /I seems to work
if /I not foo==FOO echo if /I seems to be broken

echo -----------Testing for -----------
for %%i in (A B C) do echo %%i
for %%i in (A B C) do call :forTestFun1 %%i
goto :endForTestFun1
:forTestFun1
echo %1
goto :eof
:endForTestFun1
for %%i in (X) do (
    for %%j in (Y) do (
        echo %%i %%j ))
for %%i in (A B) do (
    for %%j in (C D) do (
        echo %%i %%j ))
for %%i in (A B) do (
    for %%j in (C D) do (
        call :forTestFun2 %%i %%j ))
goto :endForTestFun2
:forTestFun2
echo %1 %2
goto :eof
:endForTestFun2

echo -----------Testing del /a-----------
del /f/q *.test > nul
echo r > r.test
attrib +r r.test
echo not-r > not-r.test

if not exist not-r.test echo not-r.test not found before delete, bad
del /a:-r *.test
if not exist not-r.test echo not-r.test not found after delete, good

if not exist r.test echo r.test not found before delete, bad
if exist r.test echo r.test found before delete, good
del /a:r *.test
if not exist r.test echo r.test not found after delete, good
if exist r.test echo r.test found after delete, bad

echo ------------ Testing del /q --------------
mkdir del_q_dir
cd del_q_dir
echo abc > file1
echo abc > file2.dat
rem If /q doesn't work, cmd will prompt and the test case should hang
del /q * > nul
for %%a in (1 2.dat) do if exist file%%a echo del /q * failed on file%%a
for %%a in (1 2.dat) do if not exist file%%a echo del /q * succeeded on file%%a
cd ..
rmdir del_q_dir

echo ------------ Testing del /s --------------
mkdir "foo bar"
cd "foo bar"
echo hi > file1.dat
echo there > file2.dat
echo bub > file3.dat
echo bye > "file with spaces.dat"
cd ..
del /s file1.dat > nul
del file2.dat /s > nul
del "file3.dat" /s > nul
del "file with spaces.dat" /s > nul
cd "foo bar"
for %%f in (1 2 3) do if exist file%%f.dat echo Del /s failed on file%%f
for %%f in (1 2 3) do if exist file%%f.dat del file%%f.dat
if exist "file with spaces.dat" echo Del /s failed on "file with spaces.dat"
if exist "file with spaces.dat" del "file with spaces.dat"
cd ..
rmdir "foo bar"

echo ----------- Testing mkdir -----------
call :setError 0
rem md and mkdir are synonymous
mkdir foobar
echo %ErrorLevel%
rmdir foobar
md foobar
echo %ErrorLevel%
rmdir foobar
rem Creating an already existing directory/file must fail
mkdir foobar
md foobar
echo %ErrorLevel%
rmdir foobar
echo > foobar
mkdir foobar
echo %ErrorLevel%
del foobar
rem Multi-level path creation
mkdir foo
echo %ErrorLevel%
mkdir foo\bar\baz
echo %ErrorLevel%
cd foo
echo %ErrorLevel%
cd bar
echo %ErrorLevel%
cd baz
echo %ErrorLevel%
echo > ..\..\bar2
mkdir ..\..\..\foo\bar2
echo %ErrorLevel%
del ..\..\bar2
mkdir ..\..\..\foo\bar2
echo %ErrorLevel%
rmdir ..\..\..\foo\bar2
cd ..
rmdir baz
cd ..
rmdir bar
cd ..
rmdir foo
echo %ErrorLevel%
rem Trailing backslashes
mkdir foo\\\\
echo %ErrorLevel%
if exist foo (rmdir foo && echo dir created
) else ( echo dir not created )
echo %ErrorLevel%
rem Invalid chars
mkdir ?
echo %ErrorLevel%
call :setError 0
mkdir ?\foo
echo %ErrorLevel%
call :setError 0
mkdir foo\?
echo %ErrorLevel%
if exist foo (rmdir foo && echo ok, foo created
) else ( echo foo not created )
call :setError 0
mkdir foo\bar\?
echo %ErrorLevel%
call :setError 0
if not exist foo (
    echo bad, foo not created
) else (
    cd foo
    if exist bar (
        echo ok, foo\bar created
        rmdir bar
    )
    cd ..
    rmdir foo
)

echo ----------- Testing rmdir -----------
call :setError 0
rem rd and rmdir are synonymous
mkdir foobar
rmdir foobar
echo %ErrorLevel%
if not exist foobar echo dir removed
mkdir foobar
rd foobar
echo %ErrorLevel%
if not exist foobar echo dir removed
rem Removing non-existent directory
rmdir foobar
echo %ErrorLevel%
rem Removing single-level directories
echo > foo
rmdir foo
echo %ErrorLevel%
if exist foo echo file not removed
del foo
mkdir foo
echo > foo\bar
rmdir foo
echo %ErrorLevel%
if exist foo echo non-empty dir not removed
del foo\bar
mkdir foo\bar
rmdir foo
echo %ErrorLevel%
if exist foo echo non-empty dir not removed
rmdir foo\bar
rmdir foo

echo -----------Testing Errorlevel-----------
rem nt 4.0 doesn't really support a way of setting errorlevel, so this is weak
rem See http://www.robvanderwoude.com/exit.php
call :setError 1
echo %ErrorLevel%
if errorlevel 2 echo errorlevel too high, bad
if errorlevel 1 echo errorlevel just right, good
call :setError 0
echo abc%ErrorLevel%def
if errorlevel 1 echo errorlevel nonzero, bad
if not errorlevel 1 echo errorlevel zero, good
rem Now verify that setting a real variable hides its magic variable
set errorlevel=7
echo %ErrorLevel% should be 7
if errorlevel 7 echo setting var worked too well, bad
call :setError 3
echo %ErrorLevel% should still be 7

echo -----------Testing GOTO-----------
if a==a goto dest1
:dest1
echo goto with no leading space worked
if b==b goto dest2
 :dest2
echo goto with a leading space worked
if c==c goto dest3
	:dest3
echo goto with a leading tab worked
if d==d goto dest4
:dest4@space@
echo goto with a following space worked

echo -----------Done, jumping to EOF-----------
goto :eof
rem Subroutine to set errorlevel and return
rem in windows nt 4.0, this always sets errorlevel 1, since /b isn't supported
:setError
exit /B %1
rem This line runs under cmd in windows NT 4, but not in more modern versions.
