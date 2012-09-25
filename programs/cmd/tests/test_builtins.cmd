echo Tests for cmd's builtin commands

@echo on
echo ------------ Testing 'echo' [ON] ------------
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
echo off now
echo word@space@
echo word@space@@space@
 echo word
echo@tab@word
echo@tab@word @tab@
echo@tab@word@tab@@space@
@tab@echo word
echo @tab@word
echo  @tab@word
echo@tab@@tab@word
echo @tab@ on @space@

@echo off
echo off@tab@@space@
@echo noecho1
 @echo noecho2
@@@@@echo echo3
echo ------------ Testing 'echo' [OFF] ------------
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
echo on again
echo word@space@
echo word@space@@space@
 echo word
echo@tab@word
echo@tab@word @tab@
echo@tab@word@tab@@space@
@tab@echo word
echo @tab@word
echo  @tab@word
echo@tab@@tab@word

echo ------------ Testing mixed echo modes ------------
echo @echo on> mixedEchoModes.cmd
echo if 1==1 echo foo>> mixedEchoModes.cmd
echo if 1==1 @echo bar>> mixedEchoModes.cmd
echo @echo off>> mixedEchoModes.cmd
echo if 1==1 echo foo2>> mixedEchoModes.cmd
echo if 1==1 @echo bar2>> mixedEchoModes.cmd
type mixedEchoModes.cmd
cmd /c mixedEchoModes.cmd
del mixedEchoModes.cmd

echo ------------ Testing parameterization ------------
call :TestParm a b c
call :TestParm "a b c"
call :TestParm "a b"\c
call :TestParm a=~`+,.{}!+b
call :TestParm a;b
call :TestParm "a;b"
call :TestParm a^;b
call :TestParm a[b]{c}(d)e
call :TestParm a&echo second line
call :TestParm a   b,,,c
call :TestParm a==b;;c
call :TestParm       a,,,  b
goto :TestRem

:TestParm
echo '%1', '%2', '%3'
goto :eof

:TestRem
echo ------------ Testing rem ------------
rem Hello
rem  Hello
rem   Hello || foo
rem echo lol
rem echo foo & echo bar
rem @tab@  Hello
rem@tab@  Hello
rem@tab@echo foo & echo bar
@echo on
rem Hello
rem  Hello
rem   Hello || foo
rem echo lol
rem echo foo & echo bar
rem @tab@  Hello
rem@tab@  Hello
rem@tab@echo foo & echo bar
@echo off

echo ------------ Testing redirection operators ------------
mkdir foobar & cd foobar
echo --- stdout redirection
echo foo>foo
type foo
echo foo 1> foo
type foo
echo foo@tab@1> foo
type foo
echo foo 1>@tab@foo
type foo
echo foo@tab@1>@tab@foo
type foo
echo foo7 7> foo
type foo
echo foo9 9> foo
type foo
echo foo1> foo
type foo
echo foo11> foo
type foo
echo foo12> foo
type foo
echo foo13>"foo"
type foo
echo foo14>."\foo"
type foo
echo foo15>."\f"oo
type foo
del foo
echo1>foo
type foo
echo --- stdout appending
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
echo food2>>"foo"
type foo
del foo
echo food21>>foo
type foo
del foo
echo foo> foo
echo foo7 7>> foo || (echo not supported & del foo)
if exist foo (type foo) else echo not supported
echo --- redirections within IF statements
if 1==1 echo foo1>bar
type bar & del bar
echo -----
if 1==1 (echo foo2>bar) else echo baz2>bar
type bar & del bar
if 1==1 (echo foo3) else echo baz3>bar
type bar || echo file does not exist, ok
if 1==1 (echo foo4>bar) else echo baz4>bar
type bar & del bar
if 1==0 (echo foo5>bar) else echo baz5>bar
type bar & del bar
if 1==0 (echo foo6) else echo baz6 1>bar
type bar & del bar
if 1==0 (echo foo7 1>bar) else echo baz7>bar
type bar & del bar
if 1==0 (echo foo8 1>bar) else echo baz8>bak
type bak
if 1==1 (echo foo>bar & echo baz)
type bar
if 1==1 (
   echo foo>bar
   echo baz
)
type bar
(if 1==1 (echo A) else echo B) > C
type C
(if 1==0 (echo A) else echo B) > C
type C
(if 1==0 (echo A > B) else echo C)
cd .. & rd /s/q foobar

echo ------------ Testing circumflex escape character ------------
rem Using something like "echo foo^" asks for an additional char after a "More?" prompt on the following line; it's not possible to currently test that non-interactively
echo ^hell^o, world
echo hell^o, world
echo hell^^o, world
echo hell^^^o, world
echo hello^
world
echo hello^

world
echo hello^


echo finished
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

echo ------------ Testing 'set' ------------
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
set@tab@FOO=foo
echo %FOO%
set@tab@FOO=
echo '%FOO%'
set FOO=foo@space@
echo '%FOO%'
set FOO=foo@tab@
echo '%FOO%'
set FOO=

echo ------------ Testing variable expansion ------------
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

echo ------------ Testing variable substrings ------------
set VAR=qwerty
echo %VAR:~0,1%
echo %VAR:~0,3%
echo %VAR:~2,2%
echo '%VAR:~-2,3%'
echo '%VAR:~-2,1%'
echo %VAR:~2,-1%
echo %VAR:~2,-3%
echo '%VAR:~-2,-4%'
echo %VAR:~-3,-2%
set VAR=

echo ------------ Testing variable substitution ------------
echo --- in FOR variables
for %%i in ("A B" C) do echo %%i
rem check works when prefix with @
@for %%i in ("A B" C) do echo %%i
rem quotes removal
for %%i in ("A B" C) do echo '%%~i'
rem fully qualified path
for %%f in ("C D" E) do echo %%~ff
rem drive letter
for %%i in ("F G" H) do echo %%~di
rem path
for %%d in ("I J" K) do echo %%~pd
rem filename
for %%i in ("L M" N) do echo %%~ni
rem file extension
for %%i in ("O. P.OOL" Q.TABC hello) do echo '%%~xi'
rem path with short path names
for %%I in ("R S" T ABCDEFGHIJK.LMNOP) do echo '%%~sI'
rem file attribute
for %%i in ("U V" W) do echo '%%~ai'
echo foo> foo
for %%i in (foo) do echo '%%~ai'
del foo
rem file date/time
rem Not fully testable, until we can grep dir's output to get foo's creation time in an envvar...
for %%i in ("a b" c) do echo '%%~ti'
rem file size
rem Similar issues as above
for %%i in ("a b" c) do echo '%%~zi'
rem combined options
for %%i in ("d e" f) do echo %%~dpi
for %%i in ("g h" i) do echo %%~sdi
for %%i in ("g h" i) do echo %%~dsi
for %%i in ("j k" l.eh) do echo '%%~xsi'

echo --- in parameters
for %%i in ("A B" C) do call :echoFun %%i
rem quotes removal
for %%i in ("A B" C) do call :echoFunQ %%i
rem fully qualified path
for %%f in ("C D" E) do call :echoFunF %%f
rem drive letter
for %%i in ("F G" H) do call :echoFunD %%i
rem path
for %%d in ("I J" K) do call :echoFunP %%d
rem filename
for %%i in ("L M" N) do call :echoFunN %%i
rem file extension
for %%i in ("O. P.OOL" Q.TABC hello) do call :echoFunX %%i
rem path with short path names
for %%I in ("R S" T ABCDEFGHIJK.LMNOP) do call :echoFunS %%I
rem NT4 aborts whole script execution when encountering ~a, ~t and ~z substitutions, preventing full testing
rem combined options
for %%i in ("d e" f) do call :echoFunDP %%i
for %%i in ("g h" i) do call :echoFunSD %%i
for %%i in ("g h" i) do call :echoFunDS %%i
for %%i in ("j k" l.eh) do call :echoFunXS %%i

goto :endEchoFuns
:echoFun
echo %1
goto :eof

:echoFunQ
echo '%~1'
goto :eof

:echoFunF
echo %~f1
goto :eof

:echoFunD
echo %~d1
goto :eof

:echoFunP
echo %~p1
goto :eof

:echoFunN
echo %~n1
goto :eof

:echoFunX
echo '%~x1'
goto :eof

:echoFunS
rem some NT4 workaround
set VAR='%~s1'
echo %VAR%
set VAR=
goto :eof

:echoFunDP
echo %~dp1
goto :eof

:echoFunSD
echo %~sd1
goto :eof

:echoFunDS
echo %~ds1
goto :eof

:echoFunXS
echo '%~xs1'
goto :eof
:endEchoFuns

echo ------------ Testing variable delayed expansion ------------
rem NT4 doesn't support this
echo --- default mode (load-time expansion)
set FOO=foo
echo %FOO%
echo !FOO!
if %FOO% == foo (
    set FOO=bar
    if %FOO% == bar (echo bar) else echo foo
)

set FOO=foo
if %FOO% == foo (
    set FOO=bar
    if !FOO! == bar (echo bar) else echo foo
)

echo --- runtime (delayed) expansion mode
setlocal EnableDelayedExpansion
set FOO=foo
echo %FOO%
echo !FOO!
if %FOO% == foo (
    set FOO=bar
    if %FOO% == bar (echo bar) else echo foo
)

set FOO=foo
if %FOO% == foo (
    set FOO=bar
    if !FOO! == bar (echo bar) else echo foo
)
echo %ErrorLevel%
setlocal DisableDelayedExpansion
echo %ErrorLevel%
set FOO=foo
echo %FOO%
echo !FOO!
set FOO=
echo --- using /V cmd flag
echo @echo off> tmp.cmd
echo set FOO=foo>> tmp.cmd
echo echo %%FOO%%>> tmp.cmd
echo echo !FOO!>> tmp.cmd
echo set FOO=>> tmp.cmd
cmd /V:ON /C tmp.cmd
cmd /V:OfF /C tmp.cmd
del tmp.cmd

echo ------------ Testing conditional execution ------------
echo --- unconditional ampersand
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
echo --- on success conditional and
call :setError 456 && echo foo3 > foo3
if exist foo3 (
    echo foo3 created
    del foo3
) else echo foo3 not created
echo bar4 && echo foo4
echo --- on failure conditional or
call :setError 789 || echo foo5
echo foo6 || echo bar6 > bar6
if exist bar6 (
    echo bar6 created
    del bar6
)

echo ------------ Testing cd ------------
mkdir foobar
cd foobar
echo blabla > singleFile
dir /b
echo Current dir: %CD%
cd
cd ..
cd
cd foobar@space@
cd
cd ..
cd
cd @space@foobar
cd
cd..
cd
cd foobar
cd..@space@
cd
if not exist foobar (cd ..)
cd foobar
cd@tab@..@tab@@space@@tab@
cd
if not exist foobar (cd ..)
cd foobar
mkdir "bar bak"
cd "bar bak"
cd
cd ..
cd ".\bar bak"
cd
cd ..
cd .\"bar bak"
cd
cd ..
cd bar bak
cd
cd "bar bak@space@"@tab@@space@
cd
cd ..\..
cd
rd /Q/s foobar
mkdir foobar
cd /d@tab@foobar
cd
cd ..
rd /q/s foobar

echo ------------ Testing type ------------
echo bar> foobaz
@echo on
type foobaz
echo ---
@echo off
type foobaz@tab@
echo ---1
type ."\foobaz"
echo ---2
type ".\foobaz"
echo ---3
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
@tab@dir /b@tab@/a-d
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
cd .. & rd foobar

echo ------------ Testing if/else ------------
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
echo Testing string comparisons
if abc == abc  (echo equal) else echo non equal
if abc =="abc" (echo equal) else echo non equal
if "abc"== abc (echo equal) else echo non equal
if "abc"== "abc" (echo equal) else echo non equal
echo Testing tabs handling
if@tab@1==1 echo doom
if @tab@1==1 echo doom
if 1==1 (echo doom) else@tab@echo quake
if@tab@not @tab@1==@tab@0 @tab@echo lol
if 1==0@tab@(echo doom) else echo quake
if 1==0 (echo doom)@tab@else echo quake
if 1==0 (echo doom) else@tab@echo quake

echo ------------ Testing for ------------
echo --- plain FOR
for %%i in (A B C) do echo %%i
for %%i in (A B C) do echo %%I
for %%i in (A B C) do echo %%j
for %%i in (A B C) do call :forTestFun1 %%i
for %%i in (1,4,1) do echo %%i
for %%i in (A, B,C) do echo %%i
for %%i in  (X) do echo %%i
for@tab@%%i in  (X2) do echo %%i
for %%i in@tab@(X3) do echo %%i
for %%i in (@tab@ foo@tab@) do echo %%i
for@tab@ %%i in@tab@(@tab@M) do echo %%i
for %%i@tab@in (X)@tab@do@tab@echo %%i
for@tab@ %%j in@tab@(@tab@M, N, O@tab@) do echo %%j
for %%i in (`echo A B`) do echo %%i
for %%i in ('echo A B') do echo %%i
for %%i in ("echo A B") do echo %%i
for %%i in ("A B" C) do echo %%i
goto :endForTestFun1
:forTestFun1
echo %1
goto :eof
:endForTestFun1
echo --- imbricated FORs
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
echo --- basic wildcards
for %%i in (ba*) do echo %%i
echo --- for /d
for /d %%i in (baz foo bar) do echo %%i 2>&1
rem Confirm we don't match files:
for /d %%i in (bazb*) do echo %%i 2>&1
for /d %%i in (bazb2*) do echo %%i 2>&1
rem Show we pass through non wildcards
for /d %%i in (PASSED) do echo %%i
for /d %%i in (xxx) do (
  echo %%i - Should be xxx
  echo Expected second line
)
rem Show we issue no messages on failures
for /d %%i in (FAILED?) do echo %%i 2>&1
for /d %%i in (FAILED?) do (
  echo %%i - Unexpected!
  echo FAILED Unexpected second line
)
for /d %%i in (FAILED*) do echo %%i 2>&1
for /d %%i in (FAILED*) do (
  echo %%i - Unexpected!
  echo FAILED Unexpected second line
)
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
cd .. & rd /s/Q foobar
echo --- for /L
rem Some cases loop forever writing 0s, like e.g. (1,0,1), (1,a,3) or (a,b,c); those can't be tested here
for /L %%i in (1,2,0) do echo %%i
for@tab@/L %%i in (1,2,0) do echo %%i
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
rem Test boundaries
for /l %%i in (1,1,4) do echo %%i
for /l %%i in (1,2,4) do echo %%i
for /l %%i in (4,-1,1) do echo %%i
for /l %%i in (4,-2,1) do echo %%i
for /l %%i in (1,-1,4) do echo %%i
for /l %%i in (4,1,1) do echo %%i
for /L %%i in (a,2,b) do echo %%i
for /L %%i in (1,1,1) do echo %%i
for /L %%i in (1,-2,-1) do echo %%i
for /L %%i in (-1,-1,-1) do echo %%i
for /L %%i in (1,2, 3) do echo %%i
rem Test zero iteration skips the body of the for
for /L %%i in (2,2,1) do (
  echo %%i
  echo FAILED
)
echo --- for /a
rem No output when using "set expr" syntax, unless in interactive mode
rem Need to use "set envvar=expr" to use in a batch script
echo ------ individual operations
set var=0
set /a var=1 +2 & echo %var%
set /a var=1 +-2 & echo %var%
set /a var=1 --2 & echo %var%
set /a var=2* 3 & echo %var%
set /a var=-2* -5 & echo %var%
set /a var=12/3 & echo %var%
set /a var=13/3 & echo %var%
set /a var=-13/3 & echo %var%
rem FIXME Divide by zero should return an error, but error messages cannot be tested with current infrastructure
set /a var=5 %% 5 & echo %var%
set /a var=5 %% 3 & echo %var%
set /a var=5 %% -3 & echo %var%
set /a var=-5 %% -3 & echo %var%
set /a var=1 ^<^< 0 & echo %var%
set /a var=1 ^<^< 2 & echo %var%
set /a var=1 ^<^< -2 & echo %var%
set /a var=-1 ^<^< -2 & echo %var%
set /a var=-1 ^<^< 2 & echo %var%
set /a var=9 ^>^> 0 & echo %var%
set /a var=9 ^>^> 2 & echo %var%
set /a var=9 ^>^> -2 & echo %var%
set /a var=-9 ^>^> -2 & echo %var%
set /a var=-9 ^>^> 2 & echo %var%
set /a var=5 ^& 0 & echo %var%
set /a var=5 ^& 1 & echo %var%
set /a var=5 ^& 3 & echo %var%
set /a var=5 ^& 4 & echo %var%
set /a var=5 ^& 1 & echo %var%
set /a var=5 ^| 0 & echo %var%
set /a var=5 ^| 1 & echo %var%
set /a var=5 ^| 3 & echo %var%
set /a var=5 ^| 4 & echo %var%
set /a var=5 ^| 1 & echo %var%
set /a var=5 ^^ 0 & echo %var%
set /a var=5 ^^ 1 & echo %var%
set /a var=5 ^^ 3 & echo %var%
set /a var=5 ^^ 4 & echo %var%
set /a var=5 ^^ 1 & echo %var%
echo ------ precedence and grouping
set /a var=4 + 2*3 & echo %var%
set /a var=(4+2)*3 & echo %var%
set /a var=4 * 3/5 & echo %var%
set /a var=(4 * 3)/5 & echo %var%
set /a var=4 * 5 %% 4 & echo %var%
set /a var=4 * (5 %% 4) & echo %var%
set /a var=3 %% (5 + 8 %% 3 ^^ 2) & echo %var%
set /a var=3 %% (5 + 8 %% 3 ^^ -2) & echo %var%
echo ------ octal and hexadecimal
set /a var=0xf + 3 & echo %var%
set /a var=0xF + 3 & echo %var%
set /a var=015 + 2 & echo %var%
set /a var=3, 8+3,0 & echo %var%
echo ------ variables
set /a var=foo=3, foo+1 & echo %var%
if defined foo (echo %foo%) else (
    echo foo not defined
)
set /a var=foo=3, foo+=1 & echo %var%
set /a var=foo=3, bar=1, bar+=foo, bar & echo %var%
set /a var=foo*= foo & echo %var%
set /a var=whateverNonExistingVar & echo %var%
set /a var=whateverNonExistingVar + bar & echo %var%
set /a var=foo -= foo + 7 & echo %var%
set /a var=foo /= 3 + 2 & echo %var%
set /a var=foo=5, foo %%=2 & echo %var%
set /a var=foo ^<^<= 2 & echo %var%
set /a var=foo ^>^>= 2 & echo %var%
set /a var=foo ^&= 2 & echo %var%
set /a var=foo=5, foo ^|= 2 & echo %var%
set /a var=foo=5, foo ^^= 2 & echo %var%
set /a var=foo=19, foo %%= 4 + (bar %%= 7) & echo.
set foo=
set bar=
set var=
echo --- for /F
mkdir foobar & cd foobar
echo ------ string argument
for /F %%i in ("a b c") do echo %%i
for /f %%i in ("a ") do echo %%i
for /f %%i in ("a") do echo %%i
fOr /f %%i in (" a") do echo %%i
for /f %%i in (" a ") do echo %%i
echo ------ fileset argument
echo --------- basic blank handling
echo a b c>foo
for /f %%i in (foo) do echo %%i
echo a >foo
for /f %%i in (foo) do echo %%i
echo a>foo
for /f %%i in (foo) do echo %%i
echo  a>foo
for /f %%i in (foo) do echo %%i
echo  a >foo
for /f %%i in (foo) do echo %%i
echo. > foo
for /f %%i in (foo) do echo %%i
echo. >> foo
echo b > foo
for /f %%i in (foo) do echo %%i
echo --------- multi-line with empty lines
echo a Z f> foo
echo. >> foo
echo.>> foo
echo b bC>> foo
echo c>> foo
echo. >> foo
for /f %%b in (foo) do echo %%b
echo --------- multiple files
echo q w > bar
echo.>> bar
echo kkk>>bar
for /f %%k in (foo bar) do echo %%k
for /f %%k in (bar foo) do echo %%k
rem echo ------ command argument
rem Not implemented on NT4
rem FIXME: Not testable right now in wine: not implemented and would need
rem preliminary grep-like program implementation (e.g. like findstr or fc) even
rem for a simple todo_wine test
rem (for /f "usebackq" %%i in (`echo z a b`) do echo %%i) || echo not supported
rem (for /f usebackq %%i in (`echo z a b`) do echo %%i) || echo not supported
echo ------ eol option
for /f "eol=@" %%i in ("    ad") do echo %%i
for /f "eol=@" %%i in (" z@y") do echo %%i
for /f "eol=|" %%i in ("a|d") do echo %%i
for /f "eol=@" %%i in ("@y") do echo %%i > output_file
if not exist output_file (echo no output) else (del output_file)
for /f "eol==" %%i in ("=y") do echo %%i > output_file
if not exist output_file (echo no output) else (del output_file)
echo ------ delims option
for /f "delims=|" %%i in ("a|d") do echo %%i
for /f "delims=|" %%i in ("a |d") do echo %%i
for /f "delims=|" %%i in ("a d|") do echo %%i
for /f "delims=| " %%i in ("a d|") do echo %%i
for /f "delims==" %%i in ("C r=d|") do echo %%i
for /f "delims=" %%i in ("foo bar baz") do echo %%i
for /f "delims=" %%i in ("c:\foo bar baz\..") do echo %%~fi
echo ------ skip option
echo a > foo
echo b >> foo
echo c >> foo
for /f "skip=2" %%i in (foo) do echo %%i
for /f "skip=3" %%i in (foo) do echo %%i > output_file
if not exist output_file (echo no output) else (del output_file)
for /f "skip=4" %%i in (foo) do echo %%i > output_file
if not exist output_file (echo no output) else (del output_file)
cd ..
rd /s/q foobar

echo ------------ Testing del /a ------------
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

echo ------------ Testing del /q ------------
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

echo ------------ Testing del /s ------------
mkdir "foo bar"
cd "foo bar"
mkdir "foo:"
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
rmdir "foo:"
cd ..
rmdir "foo bar"

echo ------------ Testing rename ------------
mkdir foobar & cd foobar
echo --- ren and rename are synonymous
echo > foo
rename foo bar
if exist foo echo foo should be renamed!
if exist bar echo foo renamed to bar
ren bar foo
if exist bar echo bar should be renamed!
if exist foo echo bar renamed to foo
echo --- name collision
echo foo>foo
echo bar>bar
ren foo bar 2> nul
type foo
type bar
rem no-op
ren foo foo
mkdir baz
ren foo baz\abc
echo --- rename read-only files
echo > file1
attrib +r file1
ren file1 file2
if not exist file1 (
    if exist file2 (
        echo read-only file renamed
    )
) else (
    echo read-only file not renamed!
)
echo --- rename directories
mkdir rep1
ren rep1 rep2
if not exist rep1 (
    if exist rep2 (
        echo dir renamed
    )
)
attrib +r rep2
ren rep2 rep1
if not exist rep2 (
    if exist rep1 (
        echo read-only dir renamed
    )
)
echo --- rename in other directory
if not exist baz\abc (
    echo rename impossible in other directory
    if exist foo echo original file still present
) else (
    echo shouldn't rename in other directory!
    if not exist foo echo original file not present anymore
)
cd .. & rd /s/q foobar

echo ------------ Testing move ------------
mkdir foobar & cd foobar
echo --- file move
echo >foo
move foo bar > nul 2>&1
if not exist foo (
    if exist bar (
        echo file move succeeded
    )
)
echo bar>bar
echo baz> baz
move /Y bar baz > nul 2>&1
if not exist bar (
    if exist baz (
        echo file move with overwrite succeeded
    )
) else (
    echo file overwrite impossible!
    del bar
)
type baz

attrib +r baz
move baz bazro > nul 2>&1
if not exist baz (
    if exist bazro (
        echo read-only files are moveable
        move bazro baz > nul 2>&1
    )
) else (
    echo read-only file not moved!
)
attrib -r baz
mkdir rep
move baz rep > nul 2>&1
if not exist baz (
    if exist rep\baz (
        echo file moved in subdirectory
    )
)
call :setError 0
move rep\baz . > nul 2>&1
move /Y baz baz > nul 2>&1
if errorlevel 1 (
    echo moving a file to itself should be a no-op!
) else (
    echo moving a file to itself is a no-op
)
echo ErrorLevel: %ErrorLevel%
call :setError 0
del baz
echo --- directory move
mkdir foo\bar
mkdir baz
echo baz2>baz\baz2
move baz foo\bar > nul 2>&1
if not exist baz (
    if exist foo\bar\baz\baz2 (
        echo simple directory move succeeded
    )
)
call :setError 0
mkdir baz
move baz baz > nul 2>&1
echo moving a directory to itself gives error; errlevel %ErrorLevel%
echo ------ dir in dir move
rd /s/q foo
mkdir foo bar
echo foo2>foo\foo2
echo bar2>bar\bar2
move foo bar > nul 2>&1
if not exist foo (
    if exist bar (
        dir /b /ad bar
        dir /b /a-d bar
        dir /b bar\foo
    )
)
cd .. & rd /s/q foobar

echo ------------ Testing mkdir ------------
call :setError 0
echo --- md and mkdir are synonymous
mkdir foobar
echo %ErrorLevel%
rmdir foobar
md foobar
echo %ErrorLevel%
rmdir foobar
echo --- creating an already existing directory/file must fail
mkdir foobar
md foobar
echo %ErrorLevel%
rmdir foobar
echo > foobar
mkdir foobar
echo %ErrorLevel%
del foobar
echo --- multilevel path creation
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
echo --- trailing backslashes
mkdir foo\\\\
echo %ErrorLevel%
if exist foo (rmdir foo & echo dir created
) else ( echo dir not created )
echo %ErrorLevel%
echo --- invalid chars
mkdir ?
echo mkdir ? gives errorlevel %ErrorLevel%
call :setError 0
mkdir ?\foo
echo mkdir ?\foo gives errorlevel %ErrorLevel%
call :setError 0
mkdir foo\?
echo mkdir foo\? gives errorlevel %ErrorLevel%
if exist foo (rmdir foo & echo ok, foo created
) else ( echo foo not created )
call :setError 0
mkdir foo\bar\?
echo mkdir foo\bar\? gives errorlevel %ErrorLevel%
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
echo --- multiple directories at once
mkdir foobaz & cd foobaz
mkdir foo bar\baz foobar "bazbaz" .\"zabzab"
if exist foo (echo foo created) else echo foo not created!
if exist bar (echo bar created) else echo bar not created!
if exist foobar (echo foobar created) else echo foobar not created!
if exist bar\baz (echo bar\baz created) else echo bar\baz not created!
if exist bazbaz (echo bazbaz created) else echo bazbaz not created!
if exist zabzab (echo zabzab created) else echo zabzab not created!
cd .. & rd /s/q foobaz
call :setError 0
mkdir foo\*
echo mkdir foo\* errorlevel %ErrorLevel%
if exist foo (rmdir foo & echo ok, foo created
) else ( echo bad, foo not created )

echo ------------ Testing rmdir ------------
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
rem Removing nonexistent directory
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
rmdir /s /Q foo 2>&1
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
cd .. & rd /s/q foobaz

echo ------------ Testing pushd/popd ------------
cd
echo --- popd is no-op when dir stack is empty
popd
cd
echo --- pushing non-existing dir
pushd foobar
cd
echo --- basic behaviour
mkdir foobar\baz
pushd foobar
cd
popd
cd
pushd foobar
pushd baz
cd
popd
cd
pushd baz
popd
cd
popd
cd
pushd .
cd foobar\baz
pushd ..
cd
popd
popd
cd
rd /s/q foobar

echo ------------ Testing attrib ------------
rem FIXME Add tests for archive, hidden and system attributes + mixed attributes modifications
mkdir foobar & cd foobar
echo foo original contents> foo
attrib foo
echo > bar
echo --- read-only attribute
rem Read-only files cannot be altered or deleted, unless forced
attrib +R foo
attrib foo
dir /Ar /B
echo bar>> foo
type foo
del foo > NUL 2>&1
if exist foo (
    echo Read-only file not deleted
) else (
    echo Should not delete read-only file!
)
del /F foo
if not exist foo (
    echo Read-only file forcibly deleted
) else (
    echo Should delete read-only file with del /F!
    attrib -r foo
    del foo
)
cd .. & rd /s/q foobar
echo --- recursive behaviour
mkdir foobar\baz & cd foobar
echo > level1
echo > whatever
echo > baz\level2
attrib baz\level2
cd ..
attrib +R l*vel? /S > nul 2>&1
cd foobar
attrib level1
attrib baz\level2
echo > bar
attrib bar
cd .. & rd /s/q foobar
echo --- folders processing
mkdir foobar
attrib foobar
cd foobar
mkdir baz
echo toto> baz\toto
attrib +r baz /s /d > nul 2>&1
attrib baz
attrib baz\toto
echo lulu>>baz\toto
type baz\toto
echo > baz\lala
rem Oddly windows allows file creation in a read-only directory...
if exist baz\lala (echo file created in read-only dir) else echo file not created
cd .. & rd /s/q foobar

echo ------------ Testing assoc ------------
rem FIXME Can't test error messages in the current test system, so we have to use some kludges
rem FIXME Revise once || conditional execution is fixed
mkdir foobar & cd foobar
echo --- setting association
assoc .foo > baz
type baz
echo ---

assoc .foo=bar
assoc .foo

rem association set system-wide
echo @echo off> tmp.cmd
echo echo +++>> tmp.cmd
echo assoc .foo>> tmp.cmd
cmd /c tmp.cmd

echo --- resetting association
assoc .foo=
assoc .foo > baz
type baz
echo ---

rem association removal set system-wide
cmd /c tmp.cmd > baz
type baz
echo ---
cd .. & rd /s/q foobar

echo ------------ Testing ftype ------------
rem FIXME Can't test error messages in the current test system, so we have to use some kludges
rem FIXME Revise once || conditional execution is fixed
mkdir foobar & cd foobar
echo --- setting association
ftype footype> baz
type baz
echo ---

ftype footype=foo_opencmd
assoc .foo=footype
ftype footype

rem association set system-wide
echo @echo off> tmp.cmd
echo echo +++>> tmp.cmd
echo ftype footype>> tmp.cmd
cmd /c tmp.cmd

echo --- resetting association
assoc .foo=

rem Removing a file type association doesn't work on XP due to a bug, so a workaround is needed
setlocal EnableDelayedExpansion
set FOO=original value
ftype footype=
ftype footype > baz
for /F %%i in ('type baz') do (set FOO=buggyXP)
rem Resetting actually works on wine/NT4, but is reported as failing due to the peculiar test (and non-support for EnabledDelayedExpansion)
rem FIXME Revisit once a grep-like program like ftype is implemented
rem (e.g. to check baz's size using dir /b instead)
echo !FOO!

rem cleanup registry
echo REGEDIT4> regCleanup.reg
echo.>> regCleanup.reg
echo [-HKEY_CLASSES_ROOT\footype]>> regCleanup.reg
regedit /s regCleanup.reg
set FOO=
endlocal
cd .. & rd /s/q foobar

echo ------------ Testing CALL ------------
mkdir foobar & cd foobar
echo --- external script
echo echo foo %%1> foo.cmd
call foo
call foo.cmd 8
echo echo %%1 %%2 > foo.cmd
call foo.cmd foo
call foo.cmd foo bar
call foo.cmd foo ""
call foo.cmd "" bar
call foo.cmd foo ''
call foo.cmd '' bar
del foo.cmd

echo --- internal routines
call :testRoutine :testRoutine
goto :endTestRoutine
:testRoutine
echo bar %1
goto :eof
:endTestRoutine

call :testRoutineArgs foo
call :testRoutineArgs foo bar
call :testRoutineArgs foo ""
call :testRoutineArgs ""  bar
call :testRoutineArgs foo ''
call :testRoutineArgs ''  bar
goto :endTestRoutineArgs
:testRoutineArgs
echo %1 %2
goto :eof
:endTestRoutineArgs

echo --- with builtins
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
cd .. & rd /s/q foobar

echo ------------ Testing SHIFT ------------

call :shiftFun p1 p2 p3 p4 p5
goto :endShiftFun

:shiftFun
echo '%1' '%2' '%3' '%4' '%5'
shift
echo '%1' '%2' '%3' '%4' '%5'
shift@tab@ /1
echo '%1' '%2' '%3' '%4' '%5'
shift /2
echo '%1' '%2' '%3' '%4' '%5'
shift /-1
echo '%1' '%2' '%3' '%4' '%5'
shift /0
echo '%1' '%2' '%3' '%4' '%5'
goto :eof
:endShiftFun

echo ------------ Testing cmd invocation ------------
rem FIXME: only a stub ATM
echo --- a batch file can delete itself
echo del foo.cmd>foo.cmd
cmd /q /c foo.cmd
if not exist foo.cmd (
    echo file correctly deleted
) else (
    echo file should be deleted!
    del foo.cmd
)
echo --- a batch file can alter itself
echo echo bar^>foo.cmd>foo.cmd
cmd /q /c foo.cmd > NUL 2>&1
if exist foo.cmd (
    type foo.cmd
    del foo.cmd
) else (
    echo file not created!
)

echo ------------ Testing setlocal/endlocal ------------
call :setError 0
rem Note: setlocal EnableDelayedExpansion already tested in the variable delayed expansion test section
mkdir foobar & cd foobar
echo --- enable/disable extensions
setlocal DisableEXTensions
echo ErrLev: %ErrorLevel%
endlocal
echo ErrLev: %ErrorLevel%
echo @echo off> tmp.cmd
echo echo ErrLev: %%ErrorLevel%%>> tmp.cmd
rem Enabled by default
cmd /C tmp.cmd
cmd /E:OfF /C tmp.cmd
cmd /e:oN /C tmp.cmd

rem FIXME: creating file before setting envvar value to prevent parsing-time evaluation (due to EnableDelayedExpansion not being implemented/available yet)
echo --- setlocal with corresponding endlocal
echo @echo off> test.cmd
echo echo %%VAR%%>> test.cmd
echo setlocal>> test.cmd
echo set VAR=localval>> test.cmd
echo echo %%VAR%%>> test.cmd
echo endlocal>> test.cmd
echo echo %%VAR%%>> test.cmd
set VAR=globalval
call test.cmd
echo %VAR%
set VAR=
echo --- setlocal with no corresponding endlocal
echo @echo off> test.cmd
echo echo %%VAR%%>> test.cmd
echo setlocal>> test.cmd
echo set VAR=localval>> test.cmd
echo echo %%VAR%%>> test.cmd
set VAR=globalval
call test.cmd
echo %VAR%
set VAR=
cd .. & rd /q/s foobar

echo ------------ Testing Errorlevel ------------
rem WARNING: Do *not* add tests using ErrorLevel after this section
should_not_exist 2> nul > nul
echo %ErrorLevel%
rem nt 4.0 doesn't really support a way of setting errorlevel, so this is weak
rem See http://www.robvanderwoude.com/exit.php
call :setError 1
echo %ErrorLevel%
if errorlevel 2 echo errorlevel too high, bad
if errorlevel 1 echo errorlevel just right, good
if errorlevel 01 echo errorlevel with leading zero just right, good
if errorlevel -1 echo errorlevel with negative number OK
if errorlevel 0x1 echo hexa should not be recognized!
if errorlevel 1a echo invalid error level recognized!
call :setError 0
echo abc%ErrorLevel%def
if errorlevel 1 echo errorlevel nonzero, bad
if not errorlevel 1 echo errorlevel zero, good
if not errorlevel 0x1 echo hexa should not be recognized!
if not errorlevel 1a echo invalid error level recognized!
rem Now verify that setting a real variable hides its magic variable
set errorlevel=7
echo %ErrorLevel% should be 7
if errorlevel 7 echo setting var worked too well, bad
call :setError 3
echo %ErrorLevel% should still be 7

echo ------------ Testing GOTO ------------
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

echo ------------ Testing PATH ------------
set backup_path=%path%
set path=original
path
path try2
path
path=try3
path
set path=%backup_path%
set backup_path=

echo ------------ Testing combined CALLs/GOTOs ------------
echo @echo off>foo.cmd
echo goto :eof>>foot.cmd
echo :eof>>foot.cmd
echo echo world>>foo.cmd

echo @echo off>foot.cmd
echo echo cheball>>foot.cmd
echo.>>foot.cmd
echo call :bar>>foot.cmd
echo if "%%1"=="deleteMe" (del foot.cmd)>>foot.cmd
echo goto :eof>>foot.cmd
echo.>>foot.cmd
echo :bar>>foot.cmd
echo echo barbare>>foot.cmd
echo goto :eof>>foot.cmd

call foo.cmd
call foot
call :bar
del foo.cmd
rem Script execution stops after the following line
foot deleteMe
call :foo
call :foot
goto :endFuns

:foot
echo foot

:foo
echo foo
goto :eof

:endFuns

:bar
echo bar
call :foo

:baz
echo baz
goto :eof

echo Final message is not output since earlier 'foot' processing stops script execution
echo Do NOT add any tests below this line

echo ------------ Done, jumping to EOF -----------
goto :eof
rem Subroutine to set errorlevel and return
rem in windows nt 4.0, this always sets errorlevel 1, since /b isn't supported
:setError
exit /B %1
rem This line runs under cmd in windows NT 4, but not in more modern versions.
