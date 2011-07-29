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
echo:
echo :
echo:word
echo :word
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
echo:
echo :
echo:word
echo :word
echo word@space@
echo word@space@@space@

echo ------------ Testing redirection operators --------------
mkdir foobar & cd foobar
echo ...stdout redirection
echo foo>foo
type foo
echo foo 1> foo
type foo
echo foo1> foo
type foo
echo foo11> foo
type foo
echo foo12> foo
type foo
echo ...stdout appending
echo foo>foo
echo foo >>foo
type foo
del foo
echo foob >> foo
type foo
echo fooc 1>>foo
type foo
echo food1>>foo
type foo
echo food2>>foo
type foo
del foo
echo food21>>foo
type foo
cd ..
rd /s/q foobar

echo ------------ Testing ^^ escape character --------------
rem Using something like "echo foo^" asks for an additional char after a "More?" prompt on the following line; it's not possible to currently test that non-interactively
echo ^hell^o, world
echo hell^o, world
echo hell^^o, world
echo hell^^^o, world
mkdir foobar
echo baz> foobar\baz
type foobar\baz
type foobar^\baz
rd /s/q foobar
echo foo ^| echo bar
echo foo ^& echo bar
call :setError 0
echo bak ^&& echo baz 2> nul
echo %ErrorLevel%
echo foo ^> foo
echo ^<> foo
type foo
del foo
set FOO=oof
echo ff^%FOO%
set FOO=bar ^| baz
set FOO
rem FIXME: echoing %FOO% gives an error (baz not recognized) but prematurely
rem exits the script on windows; redirecting stdout and/or stderr doesn't help
echo %ErrorLevel%
call :setError 0
set FOO=bar ^^^| baz
set FOO
echo %FOO%
echo %ErrorLevel%
set FOO=

echo ------------ Testing 'set' --------------
call :setError 0
set FOOBAR 2> nul > nul
echo %ErrorLevel%
set FOOBAR =  baz
echo %ErrorLevel%
echo %FOOBAR%FOOBAR not defined
echo %FOOBAR %
set FOOBAR 2> nul
set FOOBAR =  baz2
echo %ErrorLevel%
echo %fOObAr %
set FOOBAR= bar
echo %ErrorLevel%
echo %FOOBAR%
set FOO
set FOOBAR=
set FOOB
echo %FOOBAR%FOOBAR not defined
set FOOBAR =
set FOOBA 2> nul > nul
echo %ErrorLevel%
set FOO=bar
echo %FOO%
set FOO=foo
set BAR=bar
echo %FOO%%BAR%
set BAR=
set FOO=
set FOO=%FOO%
echo %FOO%FOO not defined
set BAZ%=bazbaz
set BA
echo %BAZ%%
set BAZ%=
echo set "FOO=bar" should not include the quotes in the variable value
set "FOO=bar"
echo %FOO%
set FOO=

echo ------------ Testing variable expansion --------------
call :setError 0
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

echo ------------ Testing conditional execution --------------
echo ...unconditional ^&
call :setError 123 & echo foo1
echo bar2 & echo foo2
mkdir foobar & cd foobar
echo > foobazbar
cd .. & rd /s/q foobar
if exist foobazbar (
    echo foobar not deleted!
    cd ..
    rd /s/q foobar
) else echo foobar deleted
echo ...on success conditional ^&^&
call :setError 456 && echo foo3 > foo3
if exist foo3 (
    echo foo3 created
    del foo3
) else echo foo3 not created
echo bar4 && echo foo4
echo ...on failure conditional ^|^|
call :setError 789 || echo foo5
echo foo6 || echo bar6 > bar6
if exist bar6 (
    echo bar6 created
    del bar6
)

echo ------------ Testing type ------------
echo bar> foobaz
@echo on
type foobaz
echo ***
@echo off
type foobaz
echo ***
del foobaz

echo ------------ Testing NUL ------------
md foobar & cd foobar
rem NUL file (non) creation + case insensitivity
rem Note: "if exist" does not work with NUL, so to check for file existence we use a kludgy workaround
echo > bar
echo foo > NUL
dir /b /a-d
echo foo > nul
dir /b /a-d
echo foo > NuL
dir /b /a-d
del bar
rem NUL not special everywhere
call :setError 123
echo NUL> foo
if not exist foo (echo foo should have been created) else (
    type foo
    del foo
)
rem Empty file creation
copy nul foo > nul
if exist foo (
    echo foo created
    del foo
    type foo
) else (
    echo ***
)
cd ..
rd foobar

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
if /c==/c (
  echo if seems not to detect /c as parameter
) else (
  echo parameter detection seems to be broken
)
echo Testing case sensitivity with and without /i option
if bar==BAR echo if does not default to case sensitivity
if not bar==BAR echo if seems to default to case sensitivity
if /i foo==FOO echo if /i seems to work
if /i not foo==FOO echo if /i seems to be broken
if /I foo==FOO echo if /I seems to work
if /I not foo==FOO echo if /I seems to be broken

echo -----------Testing for -----------
echo ...plain FOR
for %%i in (A B C) do echo %%i
for %%i in (A B C) do echo %%I
for %%i in (A B C) do echo %%j
for %%i in (A B C) do call :forTestFun1 %%i
for %%i in (1,4,1) do echo %%i
for %%i in (A, B,C) do echo %%i
goto :endForTestFun1
:forTestFun1
echo %1
goto :eof
:endForTestFun1
echo ...imbricated FORs
for %%i in (X) do (
    for %%j in (Y) do (
        echo %%i %%j))
for %%i in (X) do (
    for %%I in (Y) do (
        echo %%i %%I))
for %%i in (A B) do (
    for %%j in (C D) do (
        echo %%i %%j))
for %%i in (A B) do (
    for %%j in (C D) do (
        call :forTestFun2 %%i %%j ))
goto :endForTestFun2
:forTestFun2
echo %1 %2
goto :eof
:endForTestFun2
mkdir foobar & cd foobar
mkdir foo
mkdir bar
mkdir baz
echo > bazbaz
echo ...basic wildcards
for %%i in (ba*) do echo %%i
echo ...for /d
for /d %%i in (baz foo bar) do echo %%i
rem FIXME for /d incorrectly parses when wildcards are used
rem for /d %%i in (bazb*) do echo %%i
rem FIXME can't test wildcard expansion here since it's listed in directory
rem order, and not in alphabetic order.
rem Proper testing would need a currently missing "sort" program implementation.
rem for /d %%i in (ba*) do echo %%i>> tmp
rem sort < tmp
rem del tmp
rem for /d %%i in (?a*) do echo %%i>> tmp
rem sort < tmp
rem del tmp
rem for /d %%i in (*) do echo %%i>> tmp
rem sort < tmp
rem del tmp
cd ..
rd /s/Q foobar
echo ...for /L
rem Some cases loop forever writing 0s, like e.g. (1,0,1), (1,a,3) or (a,b,c); those can't be tested here
for /L %%i in (1,2,0) do echo %%i
for /L %%i in (1,2,6) do echo %%i
for /l %%i in (1 ,2,6) do echo %%i
for /L %%i in (a,2,3) do echo %%i
for /L %%i in (1,2,-1) do echo %%i
for /L %%i in (-4,-1,-1) do echo %%i
for /L %%i in (1,-2,-2) do echo %%i
for /L %%i in (1,2,a) do echo %%i
echo ErrorLevel %ErrorLevel%
for /L %%i in (1,a,b) do echo %%i
echo ErrorLevel %ErrorLevel%
rem FIXME: following test cases cannot be currently tested due to an inconsistent/buggy 'for /L' parsing.
rem for /L %%i in (a,2,b) do echo %%i
rem for /L %%i in (1,1,1) do echo %%i
rem for /L %%i in (1,-2,-1) do echo %%i
rem for /L %%i in (-1,-1,-1) do echo %%i
rem for /L %%i in (1,2, 3) do echo %%i

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
if exist foo (rmdir foo & echo dir created
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
if exist foo (rmdir foo & echo ok, foo created
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
rem multiples directories at once
mkdir foobaz & cd foobaz
mkdir foo bar\baz foobar
if exist foo (echo foo created) else echo foo not created!
if exist bar (echo bar created) else echo bar not created!
if exist foobar (echo foobar created) else echo foobar not created!
if exist bar\baz (echo bar\baz created) else echo bar\baz not created!
cd ..
rd /s/q foobaz

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
rem Recursive rmdir
mkdir foo\bar\baz
rmdir /s /Q foo
if not exist foo (
    echo recursive rmdir succeeded
) else (
    rd foo\bar\baz
    rd foo\bar
    rd foo
)
mkdir foo\bar\baz
echo foo > foo\bar\brol
rmdir /s /Q foo
if not exist foo (
    echo recursive rmdir succeeded
) else (
    rd foo\bar\baz
    del foo\bar\brol
    rd foo\bar
    rd foo
)
rem multiples directories at once
mkdir foobaz & cd foobaz
mkdir foo
mkdir bar\baz
mkdir foobar
rd /s/q foo bar foobar
if not exist foo (echo foo removed) else echo foo not removed!
if not exist bar (echo bar removed) else echo bar not removed!
if not exist foobar (echo foobar removed) else echo foobar not removed!
if not exist bar\baz (echo bar\baz removed) else echo bar\baz not removed!
cd ..
rd /s/q foobaz

echo ------------ Testing CALL --------------
mkdir foobar & cd foobar
rem External script
echo echo foo %%1> foo.cmd
call foo
call foo.cmd 8
del foo.cmd
rem Internal routines
call :testRoutine :testRoutine
goto :endTestRoutine
:testRoutine
echo bar %1
goto :eof
:endTestRoutine
rem Should work for builtins...
call mkdir foo
echo %ErrorLevel%
if exist foo (echo foo created) else echo foo should exist!
rmdir foo
set FOOBAZ_VAR=foobaz
call echo Should expand %FOOBAZ_VAR%
set FOOBAZ_VAR=
echo>batfile
call dir /b
echo>robinfile
if 1==1 call del batfile
dir /b
if exist batfile echo batfile shouldn't exist
rem ... but not for 'if' or 'for'
call if 1==1 echo bar 2> nul
echo %ErrorLevel%
call :setError 0
call for %%i in (foo bar baz) do echo %%i 2> nul
echo %ErrorLevel%
rem First look for programs in the path before trying a builtin
echo echo non-builtin dir> dir.cmd
call dir /b
cd ..
rd /s/q foobar

echo -----------Testing Errorlevel-----------
rem WARNING: Do *not* add tests using ErrorLevel after this section
should_not_exist 2> nul > nul
echo %ErrorLevel%
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
