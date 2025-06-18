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
echo/
echo /
echo/word
echo /word
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
@echo --- @ with chains and brackets
(echo the @ character chains until&&@echo we leave the current depth||(
echo hidden
@echo hidden
))&&echo and can hide brackets||(@echo command hidden)||@(echo brackets hidden)
@echo ---

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
echo/
echo /
echo/word
echo /word
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

echo ------------ Testing call and echo modes ------------
rem echo on/off is propagated back to caller (except in interactive mode)
@echo off
@FOR /F "tokens=* usebackq" %%F IN (`echo`) DO SET "wine_echo_on=%%F"
goto :hopCallEchoModes

rem ensure comparison isn't locale dependant
:showEchoMode
@FOR /F "tokens=*" %%F IN (%1) DO @IF "%%F"=="%wine_echo_on%" (@echo ECHO_IS_ON) else (@echo ECHO_IS_OFF)
@del %1
@exit /b 0
:hopCallEchoModes

echo %%*> callme.cmd
rem ensure that :showEchoMode works as expected
@echo on
@echo>foo.tmp
@call :showEchoMode foo.tmp
@echo off
@echo>foo.tmp
@call :showEchoMode foo.tmp
rem test inside a batch file, that caller keeps callee echo on/off status
@echo off
@call callme.cmd @echo on
@echo>foo.tmp
@call :showEchoMode foo.tmp
@echo on
@call callme.cmd @echo off
@echo>foo.tmp
@call :showEchoMode foo.tmp
@echo off

rem test in interactive mode... echo is always preserved after a call
@echo echo on>foo.txt
@echo call callme.cmd @echo off>>foo.txt
@echo echo^>foo.tmp>>foo.txt
type foo.txt | cmd.exe > NUL
@call :showEchoMode foo.tmp

@echo echo off>foo.txt
@echo call callme.cmd @echo on>>foo.txt
@echo echo^>foo.tmp>>foo.txt
type foo.txt | cmd.exe > NUL
@call :showEchoMode foo.tmp

rem cleanup
del foo.txt
del callme.cmd
set wine_echo_on=
@echo off
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
echo --- right-to-left redirection
1>foo-out 2>foo-err 1<&2 echo foo
type foo-out 2>NUL || echo good
type foo-err 2>NUL || echo bad
erase /q foo-out foo-err
echo --- redirect at beginning of line
>foo (echo foo)
type foo
1>foo (echo foo1)
type foo
2>foo (echo foo2 >&2)
type foo
>>foo (echo fooA)
type foo
echo --- redirections within IF statements
if 1==1 echo foo1>bar
type bar & del bar
if 1==1 echo foo2>>bar
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
echo --- multiredirections
erase /q a b & (echo >a >b)
if exist a echo a shouldn't exist
if not exist b echo b should exist
erase /q a b & (echo >a >>b)
if exist a echo a shouldn't exist
if not exist b echo b should exist
erase /q a b & (echo >a | (echo b > b))
if not exist a echo a should exist
if not exist b echo b should exist
erase /q a b & (echo cc1 2>a 1>&2 2>b)
if exist a echo a shouldn't exist
if not exist b (echo b should exist) else (echo cc2 & type b)

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
set WINE_FOO=oof
echo ff^%WINE_FOO%
set WINE_FOO=bar ^| baz
set WINE_FOO
rem FIXME: echoing %WINE_FOO% gives an error (baz not recognized) but prematurely
rem exits the script on windows; redirecting stdout and/or stderr doesn't help
echo %ErrorLevel%
call :setError 0
set WINE_FOO=bar ^^^| baz
set WINE_FOO
echo %WINE_FOO%
echo %ErrorLevel%
set WINE_FOO=

echo ------------ Testing chains ------------
rem The chain operators have the following bottom-up precedence:
rem 'else' precedes nothing and matches the closest unmatched 'if' in the same bracket depth
rem '&' precedes 'else'
rem '||' precedes '&' and 'else'
rem '&&' precedes '||', '&' and 'else'
rem '|' precedes '&&', '||', '&' and 'else'
rem
rem Example: 'if 1==1 if 2==2 if 3==3 a | b && c || d & e else f else g' is interpreted as
rem          'if 1==1 (if 2==2 (if 3==3 ((((a | b) && c) || d) & e) else f) else g)'
goto :cfailend
:cfail
echo %1
call :setError 1
goto :eof
:cfailend
echo --- chain success
echo a1&echo a2
echo b1&&echo b2
echo c1||echo c2
echo ---
echo d1&echo d2&echo d3
echo e1&echo e2&&echo e3
echo f1&echo f2||echo f3
echo ---
echo g1&&echo g2&echo g3
echo h1&&echo h2&&echo h3
echo i1&&echo i2||echo i3
echo ---
echo j1||echo j2&echo j3
echo ---
echo k1||echo k2&&echo k3
echo ---
echo l1||echo l2||echo l3
echo ---
echo --- chain failure
call :cfail a1&call :cfail a2
call :cfail b1&&call :cfail b2
echo ---
call :cfail c1||call :cfail c2
call :cfail d1&call :cfail d2&call :cfail d3
call :cfail e1&call :cfail e2&&call :cfail e3
echo ---
call :cfail f1&call :cfail f2||call :cfail f3
call :cfail g1&&call :cfail g2&call :cfail g3
echo ---
call :cfail h1&&call :cfail h2&&call :cfail h3
echo ---
call :cfail i1&&call :cfail i2||call :cfail i3
echo ---
call :cfail j1||call :cfail j2&call :cfail j3
call :cfail k1||call :cfail k2&&call :cfail k3
echo ---
call :cfail l1||call :cfail l2||call :cfail l3
echo --- chain brackets
rem Brackets are like regular commands, they support redirections
rem and have the same precedence as regular commands.
echo a1&(echo a2&echo a3)
echo b1&(echo b2&&echo b3)
echo c1&(echo c2||echo c3)
echo ---
echo d1&&(echo d2&echo d3)
echo e1&&(echo e2&&echo e3)
echo f1&&(echo f2||echo f3)
echo ---
echo g1||(echo g2&echo g3)
echo ---
echo h1||(echo h2&&echo h3)
echo ---
echo i1||(echo i2||echo i3)
echo ---
call :cfail j1&(call :cfail j2&call :cfail j3)
call :cfail k1&(call :cfail k2&&call :cfail k3)
echo ---
call :cfail l1&(call :cfail l2||call :cfail l3)
call :cfail m1&&(call :cfail m2&call :cfail m3)
echo ---
call :cfail n1&&(call :cfail n2&&call :cfail n3)
echo ---
call :cfail o1&&(call :cfail o2||call :cfail o3)
echo ---
call :cfail p1||(call :cfail p2&call :cfail p3)
call :cfail q1||(call :cfail q2&&call :cfail q3)
echo ---
call :cfail r1||(call :cfail r2||call :cfail r3)
echo --- chain pipe
rem Piped commands run at the same time, so the print order varies.
rem Additionally, they don't run in the batch script context, as shown by
rem 'call :existing_label|echo read the error message'.
(echo a 1>&2|echo a 1>&2) 2>&1
echo ---
echo b1|echo b2
echo c1&&echo c2|echo c3
echo d1||echo d2|echo d3
echo ---
echo e1&echo e2|echo e3
echo f1|echo f2&&echo f3
echo g1|echo g2||echo g3
echo ---
echo h1|echo h2&echo h3
echo i1|echo i2|echo i3
echo --- chain pipe input
rem The output data of the left side of a pipe can disappear, probably
rem because it finished too fast and closed the pipe before it could be read,
rem which means we can get broken results for the tests of this section.
echo @echo off> tmp.cmd
echo set IN=X>> tmp.cmd
echo set /p IN=%%1:>> tmp.cmd
echo setlocal EnableDelayedExpansion>> tmp.cmd
echo echo [!IN!,%%1]>> tmp.cmd
echo endlocal>> tmp.cmd
echo set IN=>> tmp.cmd
echo a1|cmd /ctmp.cmd a2
echo b1|cmd /ctmp.cmd b2|cmd /ctmp.cmd b3
echo c1|cmd /ctmp.cmd c2|cmd /ctmp.cmd c3|cmd /ctmp.cmd c4
echo d1|call tmp.cmd d2
echo e1|call tmp.cmd e2|call tmp.cmd e3
echo f1|call tmp.cmd f2|call tmp.cmd f3|call tmp.cmd f4
rem FIXME these 3 tests cause "unexpected end of output"
rem test  : echo g1|tmp.cmd g2
rem result: g2:[g1,g2]
rem test  : echo h1|tmp.cmd h2|tmp.cmd h3
rem result: h3:[h2:[h1,h2],h3]@or_broken@h3:[h2:,h3]
rem test  : echo i1|tmp.cmd i2|tmp.cmd i3|tmp.cmd i4
rem result: i4:[i3:[i2:[i1,i2],i3],i4]@or_broken@i4:[i3:[i2:,i3],i4]@or_broken@i4:[i3:,i4]
del tmp.cmd
echo --- chain else
rem Command arguments are greedy and eat up the 'else' unless terminated by
rem brackets, which means the 'else' can only be recognized when the
rem 'if true' command chain ends with brackets.
if 1==1 if 2==2 if 3==3 (echo a1) else (echo a2) else echo a3
if 1==1 if 2==2 if 3==0 (echo b1) else (echo b2) else echo b3
echo ---
if 1==1 if 2==0 if 3==3 (echo c1) else (echo c2) else echo c3
echo ---
if 1==1 if 2==0 if 3==0 (echo d1) else (echo d2) else echo d3
echo ---
if 1==0 if 2==2 if 3==3 (echo e1) else (echo e2) else echo e3
echo ---
if 1==0 if 2==2 if 3==0 (echo f1) else (echo f2) else echo f3
echo ---
if 1==0 if 2==0 if 3==3 (echo g1) else (echo g2) else echo g3
echo ---
if 1==0 if 2==0 if 3==0 (echo h1) else (echo h2) else echo h3
echo ---
echo --- chain else (if true)
if 1==1 echo a1 else echo a2
if 1==1 echo b1|echo b2 else echo b3
if 1==1 echo c1&&echo c2 else echo c3
if 1==1 echo d1||echo d2 else echo d3
echo ---
if 1==1 echo e1&echo e2 else echo e3
if 1==1 echo f1 else echo f2|echo f3
if 1==1 echo g1 else echo g2&&echo g3
if 1==1 echo h1 else echo h2||echo h3
echo ---
if 1==1 echo i1 else echo i2&echo i3
if 1==1 echo j1|(echo j2) else echo j3
echo ---
if 1==1 echo k1&&(echo k2) else echo k3
if 1==1 echo l1||(echo l2) else echo l3
echo ---
if 1==1 echo m1&(echo m2) else echo m3
if 1==1 (echo n1) else echo n2|echo n3
if 1==1 (echo o1) else echo o2&&echo o3
if 1==1 (echo p1) else echo p2||echo p3
if 1==1 (echo q1) else echo q2&echo q3
echo ---
echo --- chain else (if false)
if 1==0 echo a1 else echo a2
if 1==0 echo b1|echo b2 else echo b3
if 1==0 echo c1&&echo c2 else echo c3
if 1==0 echo d1||echo d2 else echo d3
if 1==0 echo e1&echo e2 else echo e3
if 1==0 echo f1 else echo f2|echo f3
if 1==0 echo g1 else echo g2&&echo g3
if 1==0 echo h1 else echo h2||echo h3
if 1==0 echo i1 else echo i2&echo i3
if 1==0 echo j1|(echo j2) else echo j3
echo ---
if 1==0 echo k1&&(echo k2) else echo k3
if 1==0 echo l1||(echo l2) else echo l3
if 1==0 echo m1&(echo m2) else echo m3
if 1==0 (echo n1) else echo n2|echo n3
if 1==0 (echo o1) else echo o2&&echo o3
if 1==0 (echo p1) else echo p2||echo p3
echo ---
if 1==0 (echo q1) else echo q2&echo q3
echo ------------- Testing internal commands return codes
setlocal EnableDelayedExpansion

rem All the success/failure tests are meant to be duplicated in test_builtins.bat
rem So be sure to update both files at once
echo --- success/failure for basics
call :setError 0 &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!
call :setError 33 &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!
call :setError 666 & (echo foo &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (echo foo >> h:\i\dont\exist\at\all.txt &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & echo foo >> h:\i\dont\exist\at\all.txt & echo ERRORLEVEL !errorlevel!
echo --- success/failure for IF/FOR blocks
call :setError 666 & ((if 1==1 echo "">NUL) &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & ((if 1==0 echo "">NUL) &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & ((if 1==1 (call :setError 33)) &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & ((if 1==0 (call :setError 33) else call :setError 34) &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & ((for %%i in () do echo "") &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & ((for %%i in () do call :setError 33) &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & ((for %%i in (a) do call :setError 0) &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & ((for %%i in (a) do call :setError 33) &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
echo --- success/failure for external command
mkdir foo & cd foo
call :setError 666 & (I\dont\exist.exe &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & I\dont\exist.exe & echo ERRORLEVEL !errorlevel!
call :setError 666 & (Idontexist.exe &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & Idontexist.exe & echo ERRORLEVEL !errorlevel!
call :setError 666 & (cmd.exe /c "echo foo & exit /b 0" &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (cmd.exe /c "echo foo & exit /b 1024" &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (I\dont\exist.html &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem can't run this test, generates a nice popup under windows
rem echo:>foobar.IDontExist
rem call :setError 666 & (foobar.IDontExist &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
cd .. && rd /q /s foo
echo --- success/failure for CALL command
mkdir foo & cd foo
echo exit /b %%1 > foobarEB.bat
echo type NUL > foobarS0.bat
echo rmdir foobar.dir > foobarSEL.bat
echo title foo >> foobarSEL.bat
echo rmdir foobar.dir > foobarF2.bat
echo type NUL > foobarS0WS.bat
echo.>> foobarS0WS.bat
echo goto :EOF > foobarGE.bat
echo goto :end > foobarGL.bat
echo :end >> foobarGL.bat
echo goto :end > foobarGX.bat
echo rmdir foobar.dir > foobarFGE.bat
echo goto :EOF >> foobarFGE.bat
echo rmdir foobar.dir > foobarFGL.bat
echo goto :end >> foobarFGL.bat
echo :end >> foobarFGL.bat
echo rmdir foobar.dir > foobarFGX.bat
echo goto :end >> foobarFGX.bat
rem call :setError 666 & (call I\dont\exist.exe &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem terminates batch exec on native...
call :setError 666 & (call Idontexist.exe &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call .\foobarEB.bat 0 &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call .\foobarEB.bat 1024 &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call .\foobarS0.bat &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call .\foobarS0WS.bat &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call .\foobarSEL.bat &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call .\foobarF2.bat &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call .\foobarGE.bat &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call .\foobarGL.bat &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call .\foobarGX.bat &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call .\foobarFGE.bat &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call .\foobarFGL.bat &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call .\foobarFGX.bat &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call cmd.exe /c "echo foo & exit /b 0" &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call cmd.exe /c "echo foo & exit /b 1025" &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call rmdir foobar.dir &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
cd .. && rd /q /s foo
echo --- success/failure for pipes
call :setError 666 & ((echo a | echo b) &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & ((echo a | call :setError 34) &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & ((call :setError 33 | echo a) &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & ((echo a | rmdir I\dont\exist\at\all) &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & ((rmdir I\dont\exist\at\all | echo a) &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem in a pipe, if LHS or RHS can't be started, the whole cmd is abandonned (not just the pipe!!)
echo ^( %%1 ^| %%2 ^) > foo.bat
echo echo AFTER %%ERRORLEVEL%% >> foo.bat
call :setError 666 & (cmd.exe /q /c "call foo.bat echo I\dont\exist.exe" &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (cmd.exe /q /c "call foo.bat I\dont\exist.exe echo" &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
erase /q foo.bat
echo --- success/failure for START command
call :setError 666 & (start "" /foobar >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem call :setError 666 & (start /B I\dont\exist.exe &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem can't run this test, generates a nice popup under windows
call :setError 666 & (start "" /B /WAIT cmd.exe /c "echo foo & exit /b 1024" &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (start "" /B cmd.exe /c "(choice /C:YN /T:3 /D:Y > NUL) & exit /b 1024" &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
echo --- success/failure for TYPE command
mkdir foo & cd foo
echo a > fileA
echo b > fileB
call :setError 666 & (type &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (type NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (type i\dont\exist\at\all.txt &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (type file* i\dont\exist\at\all.txt &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
echo ---
call :setError 666 & (type i\dont\exist\at\all.txt file* &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
cd .. && rd /q /s foo

echo --- success/failure for COPY command
mkdir foo & cd foo
echo a > fileA
echo b > fileB
call :setError 666 & (copy fileA >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (copy fileA fileZ >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (copy fileA fileZ /-Y >NUL <NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (copy fileA+fileD fileZ /-Y >NUL <NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (copy fileD+fileA fileZ /-Y >NUL <NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
if exist fileD echo Unexpected fileD
cd .. && rd /q /s foo

echo --- success/failure for MOVE command
mkdir foo & cd foo
echo a > fileA
echo b > fileB
call :setError 666 & (move >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (move fileA fileC >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (move fileC nowhere\fileC >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (move fileD fileE >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (move fileC fileB /-Y >NUL <NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
cd .. && rd /q /s foo

echo --- success/failure for RENAME command
mkdir foo & cd foo
echo a > fileA
echo b > fileB
call :setError 666 & (rename fileB >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (rename fileB fileA >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (rename fileB nowhere\fileB >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (rename fileD fileC >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (rename fileB fileC >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
cd .. && rd /q /s foo

echo --- success/failure for ERASE command
mkdir foo & cd foo
echo a > fileA
echo b > fileB
echo e > fileE
call :setError 666 & (erase &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (erase fileE &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (erase i\dont\exist\at\all.txt &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (erase file* i\dont\exist\at\all.txt &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
cd .. && rd /q /s foo

echo --- success/failure for change drive command
pushd C:\
call :setError 666 & (c: &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (1: &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call c: &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (call 1: &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
popd

echo --- success/failure for MKDIR,MD command
mkdir foo & cd foo
call :setError 666 & (mkdir &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (mkdir abc &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (mkdir abc &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (mkdir @:\cba\abc &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (mkdir NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
cd .. && rd /q /s foo

echo --- success/failure for CD command
mkdir foo & cd foo
mkdir abc
call :setError 666 & (cd abc >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (cd abc >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (cd ..  >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (cd     >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
cd .. && rd /q /s foo

echo --- success/failure for PUSHD/POPD commands
mkdir foo & cd foo
mkdir abc
call :setError 666 & (pushd &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (pushd abc &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (pushd abc &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (popd abc &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (popd &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & popd & echo ERRORLEVEL !errorlevel!
cd .. && rd /q /s foo

echo --- success/failure for DIR command
mkdir foo & cd foo
echo a > fileA
echo b > fileB
mkdir dir
echo b > dir\fileB
call :setError 666 & (dir /e >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (dir zzz >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (dir fileA zzz >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (dir zzz fileA >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (dir dir\zzz >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (dir file* >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
cd .. && rd /q /s foo
echo --- success/failure for RMDIR/RD command
mkdir foo & cd foo
mkdir abc
call :setError 666 & (rmdir &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
echo "">abc\abc
call :setError 666 & (rmdir abc &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (rmdir abc\abc &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
erase abc\abc
call :setError 666 & (rmdir abc &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (rmdir abc &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (rmdir @:\cba\abc &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
cd .. && rd /q /s foo
mkdir foo & cd foo
mkdir abc
call :setError 666 & rmdir & echo ERRORLEVEL !errorlevel!
echo "">abc\abc
call :setError 666 & rmdir abc & echo ERRORLEVEL !errorlevel!
call :setError 666 & rmdir abc\abc & echo ERRORLEVEL !errorlevel!
erase abc\abc
call :setError 666 & rmdir abc & echo ERRORLEVEL !errorlevel!
call :setError 666 & rmdir abc & echo ERRORLEVEL !errorlevel!
call :setError 666 & rmdir @:\cba\abc & echo ERRORLEVEL !errorlevel!
cd .. && rd /q /s foo

echo --- success/failure for MKLINK command
mkdir foo & cd foo
call :setError 666 & (mklink &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (mklink /h foo &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (mklink /h foo foo &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (mklink /z foo foo &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
echo bar > foo
call :setError 666 & (mklink /h foo foo &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (mklink /h bar foo >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (mklink /h bar foo &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
cd .. && rd /q /s foo

echo --- success/failure for SETLOCAL/ENDLOCAL commands
call :setError 666 & (setlocal foobar &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (setlocal &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (endlocal foobar &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (setlocal DisableExtensions &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (setlocal EnableExtensions &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
echo --- success/failure for DATE command
call :setError 666 & (date /t >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (date AAAA >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem need evelated priviledges to set the date
echo --- success/failure for TIME command
call :setError 666 & (time /t >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (time AAAA >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem need evelated priviledges to set the time
echo --- success/failure for BREAK command
call :setError 666 & (break &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (break 345 &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
echo --- success/failure for VER command
call :setError 666 & (ver >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (ver foo >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (ver /f >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
echo --- success/failure for VERIFY command
call :setError 666 & (verify >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (verify on >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (verify foobar >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
echo --- success/failure for VOL command
call :setError 666 & (vol >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (vol c: >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (vol foobar >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (vol /Z >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
echo --- success/failure for LABEL command
call :setError 666 & (<NUL label >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem need evelated priviledges to test

echo --- success/failure for PATH command
call :setError 666 & (path >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
set SAVED_PATH=%PATH% > NUL
call :setError 666 & (path @:\I\dont\Exist &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
path
call :setError 666 & (path ; &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
path
call :setError 666 & (path !SAVED_PATH! &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
set "SAVED_PATH="
echo --- success/failure for SET command
call :setError 666 & (set >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (set "SAVED_PATH=%PATH%" >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (set S >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (set "SAVED_PATH=" >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (set "SAVED_PATH=" >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (set /Q >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (set ThisVariableLikelyDoesntExist >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem missing /A and /p tests
echo --- success/failure for ASSOC command
call :setError 666 & (assoc >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (assoc cmd >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (assoc .idontexist >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem testing changing the assoc requires elevated privilege
echo --- success/failure for FTYPE command
call :setError 666 & (ftype >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (ftype cmdfile >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (ftype fileidontexist >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem testing changing the ftype requires elevated privilege
echo --- success/failure for SHIFT command
call :setError 666 & shift /abc &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!
call :testSuccessFailureShift 1
goto :afterSuccessFailureShift
:testSuccessFailureShift
call :setError 666 & shift &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!
call :setError 666 & shift &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!
goto :eof
:afterSuccessFailureShift
echo --- success/failure for HELP command
call :setError 666 & help >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!
call :setError 666 & help dir >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!
call :setError 666 & help ACommandThatLikelyDoesntExist >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!
echo --- success/failure for PROMPT command
call :setError 666 & prompt >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!
rem doesn't seem to set errors either on invalid $ escapes, nor qualifiers

echo --- success/failure for CLS command
call :setError 666 & (cls &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (cls foobar &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (cls /X &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
echo --- success/failure for COLOR command
call :setError 666 & (color fc < NUL > NUL 2>&1 &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem TODO: color is also hard to test: it requires fd 1 to be bound to a console, so we can't redirect its output
echo --- success/failure for TITLE command
call :setError 666 & (title a new title &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (title &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
echo --- success/failure for CHOICE command
call :setError 666 & (choice <NUL >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (choice /c <NUL >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & ((echo A | choice /C:BA) >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (choice /C:BA <NUL >NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem syntax errors in command return INVALID_FUNCTION, need to find a test for returning 255
echo --- success/failure for MORE command
call :setError 666 & (more NUL &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (more I\dont\exist.txt > NUL 2>&1 &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
call :setError 666 & (echo foo | more &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
echo --- success/failure for PAUSE command
call :setError 666 & (pause < NUL > NUL 2>&1 &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
rem TODO: pause is harder to test when fd 1 is a console handle as we don't control output
echo ---
setlocal DisableDelayedExpansion
echo ------------ Testing 'set' ------------
call :setError 0
rem Remove any WINE_FOO* WINE_BA* environment variables from shell before proceeding
for /f "delims==" %%i in ('set WINE_ba') do set %%i=
for /f "delims==" %%i in ('set WINE_foo') do set %%i=
set WINE_FOOBAR 2> nul > nul
echo %ErrorLevel%
set WINE_FOOBAR =  baz
echo %ErrorLevel%
echo %WINE_FOOBAR%WINE_FOOBAR not defined
echo %WINE_FOOBAR %
set WINE_FOOBAR 2> nul
set WINE_FOOBAR =  baz2
echo %ErrorLevel%
echo %WINE_fOObAr %
set WINE_FOOBAR= bar
echo %ErrorLevel%
echo %WINE_FOOBAR%
set WINE_FOO
set WINE_FOOBAR=
set WINE_FOOB
echo %WINE_FOOBAR%WINE_FOOBAR not defined
set WINE_FOOBAR =
set WINE_FOOBA 2> nul > nul
echo %ErrorLevel%
set WINE_FOO=bar
echo %WINE_FOO%
set WINE_FOO=foo
set WINE_BAR=bar
echo %WINE_FOO%%WINE_BAR%
set WINE_BAR=
set WINE_FOO=
set WINE_FOO=%WINE_FOO%
echo %WINE_FOO%WINE_FOO not defined
set WINE_BAZ%=bazbaz
set WINE_BA
echo %WINE_BAZ%%
set WINE_BAZ%=
echo set "WINE_FOO=bar" should not include the quotes in the variable value
set "WINE_FOO=bar"
echo %WINE_FOO%
set@tab@WINE_FOO=foo
echo %WINE_FOO%
set@tab@WINE_FOO=
echo '%WINE_FOO%'
set WINE_FOO=foo@space@
echo '%WINE_FOO%'
set WINE_FOO=foo@tab@
echo '%WINE_FOO%'
rem Space symbol must appear in `var`
set WINE_FOO=value@space@
echo '%WINE_FOO%'
rem Space symbol must NOT appear in `var`
set "WINE_FOO=value"@space@
echo '%WINE_FOO%'
rem Mixed examples:
set WINE_FOO=jim fred
echo '%WINE_FOO%'
set WINE_FOO="jim" fred
echo '%WINE_FOO%'
set "WINE_FOO=jim fred"
echo '%WINE_FOO%'
set "WINE_FOO=jim" fred
echo '%WINE_FOO%'
rem Only the final quote ends the string
set "WINE_FOO=apple"banana"grape"orange
echo '%WINE_FOO%'
set WINE_FOO=
rem set PATH must work with quotes
set PATH_BACKUP=%PATH%
mkdir folder
mkdir "fol;der"
echo echo I'm here! > "fol;der\sub1.bat"
echo echo I'm here! > folder\sub1.bat
set PATH=nothing;"fol;der"
call sub1
set PATH="folder
call sub1
set PATH=folder"
call sub1
del "fol;der\sub1.bat"
del folder\sub1.bat
rmdir "fol;der"
rmdir folder
PATH=%PATH_BACKUP%
set WINE_FOO=foo
echo bar| cmd /v:on /c "set /p WINE_FOO=prompt & echo X!WINE_FOO!X"
echo:| cmd /v:on /c "set /p WINE_FOO=prompt & echo Y!WINE_FOO!Y"
echo:| cmd /v:on /c "set /p WINE_FOO='prompt' & echo Y!WINE_FOO!Y"
echo:| cmd /v:on /c "set /p WINE_FOO="prompt" & echo Y!WINE_FOO!Y"
set =

echo ------------ Testing 'choice' ------------

rem Windows XP and Windows 2000 do not come with choice
rem echo is used for @or_broken@ formatting
choice /C:ABC /M "Example message" /D A /T:0
if %ERRORLEVEL% EQU 9009 (
  echo choice unavailable
)
echo %ERRORLEVEL%
choice /C ABC "/M:Example message" /D:B /T 0
if %ERRORLEVEL% EQU 9009 (
  echo choice unavailable
)
echo %ERRORLEVEL%
choice /C def /D:f /T:0
if %ERRORLEVEL% EQU 9009 (
  echo choice unavailable
)
echo %ERRORLEVEL%
REM If a pipe fails due to a nonexistent command
REM it will stop the whole program's execution
if %ERRORLEVEL% NEQ 9009 (
  echo Y | choice /C ABCXYZ /D A /T 2
)
if %ERRORLEVEL% EQU 9009 (
  echo choice unavailable
)
echo %ERRORLEVEL%
choice /C ABC /N /D A /T 0
if %ERRORLEVEL% EQU 9009 (
  echo choice unavailable
)
echo %ERRORLEVEL%
choice /C abcABC /CS /D:A /T:0
if %ERRORLEVEL% EQU 9009 (
  echo choice unavailable
)
echo %ERRORLEVEL%
rem intentional error
choice /C abcABC /D:A /T:0 >NUL 2>NUL
echo %ERRORLEVEL%

echo ------------ Testing variable expansion ------------
call :setError 0
echo ~p0 should be path containing batch file
echo %~p0
mkdir dummydir
cd dummydir
echo %~p0
cd ..
rmdir dummydir
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
echo %WINE_UNKNOWN%S
echo P%WINE_UNKNOWN%
echo P%WINE_UNKNOWN%S
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
set WINE_VAR=qwerty
echo %WINE_VAR:~0,1%
echo %WINE_VAR:~0,3%
echo %WINE_VAR:~2,2%
echo '%WINE_VAR:~-2,3%'
echo '%WINE_VAR:~-2,1%'
echo %WINE_VAR:~2,-1%
echo %WINE_VAR:~2,-3%
echo '%WINE_VAR:~-2,-4%'
echo %WINE_VAR:~-3,-2%
echo %WINE_VAR:~4,4%
echo '%WINE_VAR:~5%'
echo '%WINE_VAR:~6%'
echo '%WINE_VAR:~7%'
echo '%WINE_VAR:~-0%'
echo '%WINE_VAR:~,%'
echo '%WINE_VAR:~,2%'
echo '%WINE_VAR:~2a%'
echo '%WINE_VAR:~2a,2%'
echo '%WINE_VAR:~a,2%'
echo '%WINE_VAR:~2,2a%'
echo '%WINE_VAR:~-%'

echo ------------ Testing variable partial replacement ------------
set WINE_VAR=qwertyQWERTY
echo %WINE_VAR:qw=az%
echo %WINE_VAR:qw=%
echo %WINE_VAR:*TY==_%
echo %WINE_VAR:*TY=%
set WINE_VAR=
mkdir dummydir
set WINE_VAR=\foo;\bar;%CD%
cd dummydir
for %%i in (dummydir) do echo %%~$WINE_VAR:i
echo %CD:~-6,6%
cd ..
rmdir dummydir

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
for %%i in (foo) do echo '%%~zi'
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
for %%i in ("") do echo '%%~i,%%~fi,%%~di,%%~pi,%%~ni,%%~xi,%%~si,%%~ai,%%~ti,%%~zi'

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
set WINE_VAR='%~s1'
echo %WINE_VAR%
set WINE_VAR=
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

setlocal EnableDelayedExpansion
set WINE_FOO=foo bar
for %%i in ("!WINE_FOO!") do echo %%i
for %%i in (!WINE_FOO!) do echo %%i

setlocal DisableDelayedExpansion

set "BEFORE_DELAY=before!"
setlocal EnableDelayedExpansion
set "AFTER_DELAY=after^!"
echo !BEFORE_DELAY!
echo !AFTER_DELAY!
setlocal DisableDelayedExpansion

echo --- in digit variables
for %%0 in (a b) do echo %%0 %%1 %%2
echo ------------ Testing parameter zero ------------
call :func parm1 parm2
goto :endParm0
:func
echo %~0 %~1
echo [%0] [%~d0] [%~p0] [%~n0] [%~x0] [%~s0]
goto :EOF
:endParm0

echo ------------ Testing variable delayed expansion ------------
rem NT4 doesn't support this
echo --- default mode (load-time expansion)
set WINE_FOO=foo
echo %WINE_FOO%
echo !WINE_FOO!
if %WINE_FOO% == foo (
    set WINE_FOO=bar
    if %WINE_FOO% == bar (echo bar) else echo foo
)

set WINE_FOO=foo
if %WINE_FOO% == foo (
    set WINE_FOO=bar
    if !WINE_FOO! == bar (echo bar) else echo foo
)

echo --- runtime (delayed) expansion mode
setlocal EnableDelayedExpansion
set WINE_FOO=foo
echo %WINE_FOO%
echo !WINE_FOO!
if %WINE_FOO% == foo (
    set WINE_FOO=bar
    if %WINE_FOO% == bar (echo bar) else echo foo
)

set WINE_FOO=foo
if %WINE_FOO% == foo (
    set WINE_FOO=bar
    if !WINE_FOO! == bar (echo bar) else echo foo
)
set WINE_FOO=32
if %WINE_FOO% == 32 (
  set WINE_FOO=33
  call :setError 33
  if errorlevel !WINE_FOO! (echo gotitright) else echo gotitwrong
)
echo %ErrorLevel%
setlocal DisableDelayedExpansion
echo %ErrorLevel%
set WINE_FOO=foo
echo %WINE_FOO%
echo !WINE_FOO!
set WINE_FOO=

setlocal EnableDelayedExpansion
set WINE_FOO=foo bar
if !WINE_FOO!=="" (echo empty) else echo not empty
setlocal DisableDelayedExpansion

echo --- using /V cmd flag
echo @echo off> tmp.cmd
echo set WINE_FOO=foo>> tmp.cmd
echo echo %%WINE_FOO%%>> tmp.cmd
echo echo !WINE_FOO!>> tmp.cmd
echo set WINE_FOO=>> tmp.cmd
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
echo foo> foobay
echo ---4
type foobaz foobay > foobax 2> foobaw
echo ---5
type foobax
echo ---6
type foobaw
echo ---7
del foobaz foobay foobax foobaw

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
echo 1234 >a.a
copy a.a+NUL b.b >nul
call :CheckFileSize a.a 7 b.b 8
copy NUL+a.a b.b >nul
call :CheckFileSize a.a 7 b.b 8
mkdir subdir
copy a.a+NUL subdir\ >nul
call :CheckFileSize a.a 7 subdir\a.a 8
del subdir\a.a
cd subdir
copy ..\a.a NUL >nul
if exist a.a echo Failed
cd ..
rd subdir /s /q
del a.a b.b
cd .. & rd foobar /s /q

echo ------------ Testing if/else ------------
echo --- if/else should work with blocks
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
SET elseIF=0
if 1 == 1 (
  SET /a elseIF=%elseIF%+1
) else if 1 == 1 (
  SET /a elseIF=%elseIF%+2
) else (
  SET /a elseIF=%elseIF%+2
)
if %elseIF% == 1 (
  echo else if seems to work
) else (
  echo else if seems to be broken
)
SET elseIF=0
if 1 == 2 (
  SET /a elseIF=%elseIF%+2
) else if 1 == 1 (
  SET /a elseIF=%elseIF%+1
) else (
  SET /a elseIF=%elseIF%+2
)
if %elseIF% == 1 (
  echo else if seems to work
) else (
  echo else if seems to be broken
)
SET elseIF=0
if 1 == 2 (
  SET /a elseIF=%elseIF%+2
) else if 1 == 2 (
  SET /a elseIF=%elseIF%+2
) else (
  SET /a elseIF=%elseIF%+1
)
if %elseIF% == 1 (
  echo else if seems to work
) else (
  echo else if seems to be broken
)
if "x" == "a" (
  echo broken1
) else (
  echo expected1
  if "y" == "b" echo broken2
  echo expected post-embedded if
)
if ()==() (
  echo comparison operators surrounded by brackets seem to work
) else (
  echo comparison operators surrounded by brackets seem to be broken
)
if 1(==1( (
  echo comparison operators surrounded by brackets seem to work
) else (
  echo comparison operators surrounded by brackets seem to be broken
)
if )==) (
  echo comparison operators surrounded by brackets seem to work
) else (
  echo comparison operators surrounded by brackets seem to be broken
)
if /i not (a)==(b) (
  echo comparison operators surrounded by brackets seem to work
) else (
  echo comparison operators surrounded by brackets seem to be broken
)
if defined windir echo windir is defined
if not defined windir echo windir is defined
if not exist %windir% (
  echo windir does not exist
) else (
  echo windir does exist
)
if 1 == 0 (
   echo 1 == 0 should not be true
@space@
) else echo block containing a line with just space seems to work
if 1 == 0 (
   echo 1 == 0 should not be true
@tab@
) else echo block containing a line with just tab seems to work
if 1 == 0 (
   echo 1 == 0 should not be true
@space@@tab@
) else echo block containing a line with just space and tab seems to work
if 1 == 0 (
   echo 1 == 0 should not be true
@tab@@space@
) else echo block containing a line with just tab and space seems to work
if 1 == 0 (
   echo 1 == 0 should not be true
@space@
@space@
) else echo block containing two lines with just space seems to work
if 1 == 0 (
   echo 1 == 0 should not be true
@tab@
@tab@
) else echo block containing two lines with just tab seems to work
::
set WINE_IDONTEXIST=
echo @if [%%WINE_IDONTEXIST%%] == [] (@tab@> blockclosing.cmd
echo   echo with closing bracket>> blockclosing.cmd
echo )>> blockclosing.cmd
cmd.exe /Q /C blockclosing.cmd
echo %ERRORLEVEL% ok
::
echo @if 1 == 1 (> blockclosing.cmd
echo   echo without closing bracket first>> blockclosing.cmd
echo   echo without closing bracket second>> blockclosing.cmd
cmd.exe /Q /C blockclosing.cmd
echo %ERRORLEVEL% two lines
::
echo echo before both blocks> blockclosing.cmd
echo @if 1 == 1 (>> blockclosing.cmd
echo   echo before nested block without closing bracket>> blockclosing.cmd
echo   @if 2 == 2 (>> blockclosing.cmd
echo     echo without closing bracket>> blockclosing.cmd
echo )>> blockclosing.cmd
echo echo outside of block without closing bracket>> blockclosing.cmd
cmd.exe /Q /C blockclosing.cmd
echo %ERRORLEVEL% nested
::
del blockclosing.cmd
echo --- case sensitivity with and without /i option
if bar==BAR echo if does not default to case sensitivity
if not bar==BAR echo if seems to default to case sensitivity
if /i foo==FOO echo if /i seems to work
if /i not foo==FOO echo if /i seems to be broken
if /I foo==FOO echo if /I seems to work
if /I not foo==FOO echo if /I seems to be broken

echo --- string comparisons
if abc == abc  (echo equal) else echo non equal
if abc =="abc" (echo equal) else echo non equal
if "abc"== abc (echo equal) else echo non equal
if "abc"== "abc" (echo equal) else echo non equal

echo --- tabs handling
if@tab@1==1 echo doom
if @tab@1==1 echo doom
if 1==1 (echo doom) else@tab@echo quake
if@tab@not @tab@1==@tab@0 @tab@echo lol
if 1==0@tab@(echo doom) else echo quake
if 1==0 (echo doom)@tab@else echo quake
if 1==0 (echo doom) else@tab@echo quake

echo --- comparison operators
rem NT4 misevaluates conditionals in for loops so we have to use subroutines as workarounds
echo ------ for strings
rem NT4 stops processing of the whole batch file as soon as it finds a
rem comparison operator non fully uppercased, such as lss instead of LSS, so we
rem can't test those here.
if LSS LSS LSSfoo (echo LSS string can be used as operand for LSS comparison)
if LSS LSS LSS (echo bar)
if 1.1 LSS 1.10 (echo floats are handled as strings)
if "9" LSS "10" (echo numbers in quotes recognized!) else echo numbers in quotes are handled as strings
if not "-1" LSS "1" (echo negative numbers as well) else echo NT4
if /i foo LSS FoOc echo if /i seems to work for LSS
if /I not foo LSS FOOb echo if /I seems to be broken for LSS
set WINE_STR_PARMS=A B AB BA AA
for %%i in (%WINE_STR_PARMS%) do (
    for %%j in (%WINE_STR_PARMS%) do (
        call :LSStest %%i %%j))
if b LSS B (echo b LSS B) else echo NT4
if /I b LSS B echo b LSS B insensitive
if b LSS A echo b LSS A
if /I b LSS A echo b LSS A insensitive
if a LSS B (echo a LSS B) else echo NT4
if /I a LSS B echo a LSS B insensitive
if A LSS b echo A LSS b
if /I A LSS b echo A LSS b insensitive
for %%i in (%WINE_STR_PARMS%) do (
    for %%j in (%WINE_STR_PARMS%) do (
        call :LEQtest %%i %%j))
if b LEQ B (echo b LEQ B) else echo NT4
if /I b LEQ B echo b LEQ B insensitive
if b LEQ A echo b LEQ A
if /I b LEQ A echo b LEQ A insensitive
if a LEQ B (echo a LEQ B) else echo NT4
if /I a LEQ B echo a LEQ B insensitive
if A LEQ b echo A LEQ b
if /I A LEQ b echo A LEQ b insensitive
for %%i in (%WINE_STR_PARMS%) do (
    for %%j in (%WINE_STR_PARMS%) do (
        call :EQUtest %%i %%j))
if /I A EQU a echo A EQU a insensitive
for %%i in (%WINE_STR_PARMS%) do (
    for %%j in (%WINE_STR_PARMS%) do (
        call :NEQtest %%i %%j))
for %%i in (%WINE_STR_PARMS%) do (
    for %%j in (%WINE_STR_PARMS%) do (
        call :GEQtest %%i %%j))
for %%i in (%WINE_STR_PARMS%) do (
    for %%j in (%WINE_STR_PARMS%) do (
        call :GTRtest %%i %%j))

echo ------------ Testing if/exist ------------
mkdir subdir
echo something>subdir\bar
echo something else>foo
if exist foo (
   echo exist explicit works
) else (
   echo ERROR exist explicit broken
)
if exist bar (
   echo ERROR exist explicit unknown file broken
) else (
   echo exist explicit unknown file works
)
if exist subdir\bar (
   echo exist explicit in subdir works
) else (
   echo ERROR exist explicit in subdir broken
)
if exist fo* (
   echo exist simple wildcard works
) else (
   echo ERROR exist simple wildcard broken
)
if exist subdir\ba* (
   echo exist wildcard works
) else (
   echo ERROR exist wildcard broken
)
if not exist subdir\ba* (
   echo ERROR negate exist wildcard broken
) else (
   echo negate exist wildcard works
)
if exist idontexist\ba* (
   echo ERROR exist wildcard bad subdir broken
) else (
   echo exist wildcard bad subdir broken works
)
if exist subdir (
   echo exist subdir ok
) else (
   echo ERROR exist subdir not working
)
if exist subdir\. (
   echo exist subdir with \. ok
) else (
   echo ERROR exist subdir with \. not working
)
if exist subdir\ (
   echo exist subdir with \ ok
) else (
   echo ERROR exist subdir with \ not working
)
if exist "subdir\" (
   echo exist subdir with \ and quotes ok
) else (
   echo ERROR exist subdir with \ and quotes not working
)
if exist subdir/. (
   echo exist subdir with /. ok
) else (
   echo ERROR exist subdir with /. not working
)
if exist subdir/ (
   echo exist subdir with / ok
) else (
   echo ERROR exist subdir with / not working
)
if exist "subdir/" (
   echo exist subdir with / and quotes ok
) else (
   echo ERROR exist subdir with / and quotes not working
)
if not exist "" (
    echo exist empty string works
) else (
    echo exist empty string broken
)
del foo subdir\bar
rd subdir

if exist %~D0 (echo ok) else echo failure
if exist %~D0\ (echo ok) else echo failure
if exist %~D0\. (echo ok) else echo failure

echo ------ for numbers
if -1 LSS 1 (echo negative numbers handled)
if not -1 LSS -10 (echo negative numbers handled)
if not 9 LSS 010 (echo octal handled)
if not -010 LSS -8 (echo also in negative form)
if 4 LSS 0x5 (echo hexa handled)
if not -1 LSS -0x1A (echo also in negative form)
if 11 LSS 101 (echo 11 LSS 101)
set WINE_INT_PARMS=0 1 10 9
for %%i in (%WINE_INT_PARMS%) do (
    for %%j in (%WINE_INT_PARMS%) do (
        call :LSStest %%i %%j))
for %%i in (%WINE_INT_PARMS%) do (
    for %%j in (%WINE_INT_PARMS%) do (
        call :LEQtest %%i %%j))
for %%i in (%WINE_INT_PARMS%) do (
    for %%j in (%WINE_INT_PARMS%) do (
        call :EQUtest %%i %%j))
if 011 EQU 9 (echo octal ok)
if 0xA1 EQU 161 (echo hexa ok)
if 0xA1 EQU "161" (echo hexa should be recognized) else (echo string/hexa compare ok)
if "0xA1" EQU 161 (echo hexa should be recognized) else (echo string/hexa compare ok)
for %%i in (%WINE_INT_PARMS%) do (
    for %%j in (%WINE_INT_PARMS%) do (
        call :NEQtest %%i %%j))
for %%i in (%WINE_INT_PARMS%) do (
    for %%j in (%WINE_INT_PARMS%) do (
        call :GEQtest %%i %%j))
for %%i in (%WINE_INT_PARMS%) do (
    for %%j in (%WINE_INT_PARMS%) do (
        call :GTRtest %%i %%j))
echo ------ for numbers and stringified numbers
if not "1" EQU 1 (echo strings and integers not equal) else echo foo
if not 1 EQU "1" (echo strings and integers not equal) else echo foo
if '1' EQU 1 echo '1' EQU 1
if 1 EQU '1' echo 1 EQU '1'
if not "1" GEQ 1 (echo foo) else echo bar
if "10" GEQ "1" echo "10" GEQ "1"
if '1' GEQ 1 (echo '1' GEQ 1) else echo NT4
if 1 GEQ "1" echo 1 GEQ "1"
if "1" GEQ "1" echo "1" GEQ "1"
if '1' GEQ "1" echo '1' GEQ "1"
if "10" GEQ "1" echo "10" GEQ "1"
if not 1 GEQ '1' (echo non NT4) else echo 1 GEQ '1'
for %%i in ("1" '1') do call :GEQtest %%i '1'
if "10" GEQ '1' (echo "10" GEQ '1') else echo foo
if 1 GEQ "10" (echo 1 GEQ "10") else echo foo
if "1" GEQ "10" (echo 1 GEQ "10") else echo foo
if '1' GEQ "10" (echo '1' GEQ "10") else echo foo
if "10" GEQ "10" (echo "10" GEQ "10")
echo --- unconditional ampersand after if one line
if "0"=="0" echo 1 & echo 2 & echo 3 else echo 4
echo ---
echo x & if "0"=="1" echo 1 & echo 2
echo ---
echo x & if "0"=="1" echo 1 & echo 2 & echo 3
echo ---
echo x & if "0"=="1" (echo 1 & echo 2 & echo 3)
echo ---
echo x & if "0"=="1" echo 1 & echo 2 & echo 3 else echo 4
echo ---
goto :endIfCompOpsSubroutines

rem IF subroutines helpers
:LSStest
if %1 LSS %2 echo %1 LSS %2
goto :eof
:LEQtest
if %1 LEQ %2 echo %1 LEQ %2
goto :eof
:EQUtest
if %1 EQU %2 echo %1 EQU %2
goto :eof
:NEQtest
if %1 NEQ %2 echo %1 NEQ %2
goto :eof
:GEQtest
if %1 GEQ %2 echo %1 GEQ %2
goto :eof
:GTRtest
if %1 GTR %2 echo %1 GTR %2
goto :eof

:endIfCompOpsSubroutines
set WINE_STR_PARMS=
set WINE_INT_PARMS=

echo ------------ Testing if/for ------------
if ""=="" for %%i in (A) DO (echo %%i)
if not ""=="" for %%i in (B) DO (echo %%i)

echo ------------ Testing if/set ------------
rem a left parenthesis is part of the value, not the start of an 'if' block
set x=C:\Program Files (x86)
if ""=="" set y=%x%\dummy
echo %y%
if 1 == 1 set z= (
echo '%z%'
rem 'set' in one-line 'if' statement does not interfere with other 'if' blocks
setlocal enableDelayedExpansion
if 1 == 1 (
    if 1 == 1 set z=zeta
    if 1 == 1 (
         echo !z!
    )
)
endlocal

echo --- Testing if + var subst in delayed expansion mode
setlocal enableDelayedExpansion
for %%i in (abc 10.0 11.0) do (
    set result=%%i
    echo [DEBUG] checking {!result!}
    if "!result:~0,3!"=="10." (
        echo SDKVER=!result!
    )
)
endlocal

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
echo --- nested FORs and args tempering
set "WINE_ARGS= -foo=bar -x=y"
:test_for_loop_params_parse
for /F "tokens=1,* delims= " %%a in ("%WINE_ARGS%") do (
    for /F "tokens=1,2 delims==" %%1 in ("%%a") do (
        echo inner argument {%%1, %%2}
    )
    set "WINE_ARGS=%%b"
    goto :test_for_loop_params_parse
)
set "WINE_ARGS="
echo --- nesting and delayed expansion
setlocal enabledelayedexpansion
set WINE_ARGS=1
for %%a in (a b) do (
  set /a WINE_ARGS+=1
  echo %%a %WINE_ARGS% !WINE_ARGS!
  for /l %%b in (1 1 !WINE_ARGS!) do (
    if !WINE_ARGS!==%%b (echo %%b-A1) else echo %%b-A2
    if %WINE_ARGS%==%%b (echo %%b-B1) else echo %%b-B2
  )
)
setlocal disabledelayedexpansion
echo --- nesting if/for
for %%a in ("f"
"g"
"h"
) do if #==# (echo %%a)

mkdir foobar & cd foobar
mkdir foo
mkdir bar
mkdir baz
mkdir pop
echo > bazbaz
echo --- basic wildcards
for %%i in (ba*) do echo %%i
echo --- wildcards in subdirs
echo something>pop\bar1
echo something>pop\bar2.txt
echo something>pop\bar3
for %%f in (pop\ba*) do ( call echo %%f )
rmdir /s/q pop
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
echo > baz\bazbaz
goto :TestForR

:SetExpected
del temp.bat 2>nul
call :WriteLine set WINE_found=N
for /l %%i in (1,1,%WINE_expectedresults%) do (
  call :WriteLine if "%%%%WINE_expectedresults.%%i%%%%"=="%%%%~1" set WINE_found=Y
  call :WriteLine if "%%%%WINE_found%%%%"=="Y" set WINE_expectedresults.%%i=
  call :WriteLine if "%%%%WINE_found%%%%"=="Y" goto :eof
)
call :WriteLine echo Got unexpected result: "%%%%~1"
goto :eof

:WriteLine
echo %*>> temp.bat
goto :EOF

:ValidateExpected
del temp.bat 2>nul
for /l %%i in (1,1,%WINE_expectedresults%) do (
  call :WriteLine if not "%%%%WINE_expectedresults.%%i%%%%"=="" echo Found missing result: "%%%%WINE_expectedresults.%%i%%%%"
)
call temp.bat
del temp.bat 2>nul
goto :eof

:TestForR
rem %CD% does not work on NT4 so use the following workaround
for /d %%i in (.) do set WINE_CURDIR=%%~dpnxi

echo --- for /R
echo Plain directory enumeration
set WINE_expectedresults=4
set WINE_expectedresults.1=%WINE_CURDIR%\.
set WINE_expectedresults.2=%WINE_CURDIR%\bar\.
set WINE_expectedresults.3=%WINE_CURDIR%\baz\.
set WINE_expectedresults.4=%WINE_CURDIR%\foo\.
call :SetExpected
for /R %%i in (.) do call temp.bat "%%i"
call :ValidateExpected

echo Plain directory enumeration from provided root
set WINE_expectedresults=4
set WINE_expectedresults.1=%WINE_CURDIR%\.
set WINE_expectedresults.2=%WINE_CURDIR%\bar\.
set WINE_expectedresults.3=%WINE_CURDIR%\baz\.
set WINE_expectedresults.4=%WINE_CURDIR%\foo\.
if "%CD%"=="" goto :SkipBrokenNT4
call :SetExpected
for /R "%WINE_CURDIR%" %%i in (.) do call temp.bat "%%i"
call :ValidateExpected
:SkipBrokenNT4

echo File enumeration
set WINE_expectedresults=2
set WINE_expectedresults.1=%WINE_CURDIR%\baz\bazbaz
set WINE_expectedresults.2=%WINE_CURDIR%\bazbaz
call :SetExpected
for /R %%i in (baz*) do call temp.bat "%%i"
call :ValidateExpected

echo File enumeration from provided root
set WINE_expectedresults=2
set WINE_expectedresults.1=%WINE_CURDIR%\baz\bazbaz
set WINE_expectedresults.2=%WINE_CURDIR%\bazbaz
call :SetExpected
for /R %%i in (baz*) do call temp.bat "%%i"
call :ValidateExpected

echo Mixed enumeration
set WINE_expectedresults=6
set WINE_expectedresults.1=%WINE_CURDIR%\.
set WINE_expectedresults.2=%WINE_CURDIR%\bar\.
set WINE_expectedresults.3=%WINE_CURDIR%\baz\.
set WINE_expectedresults.4=%WINE_CURDIR%\baz\bazbaz
set WINE_expectedresults.5=%WINE_CURDIR%\bazbaz
set WINE_expectedresults.6=%WINE_CURDIR%\foo\.
call :SetExpected
for /R %%i in (. baz*) do call temp.bat "%%i"
call :ValidateExpected

echo Mixed enumeration from provided root
set WINE_expectedresults=6
set WINE_expectedresults.1=%WINE_CURDIR%\.
set WINE_expectedresults.2=%WINE_CURDIR%\bar\.
set WINE_expectedresults.3=%WINE_CURDIR%\baz\.
set WINE_expectedresults.4=%WINE_CURDIR%\baz\bazbaz
set WINE_expectedresults.5=%WINE_CURDIR%\bazbaz
set WINE_expectedresults.6=%WINE_CURDIR%\foo\.
call :SetExpected
for /R %%i in (. baz*) do call temp.bat "%%i"
call :ValidateExpected

echo With duplicates enumeration
set WINE_expectedresults=12
set WINE_expectedresults.1=%WINE_CURDIR%\bar\bazbaz
set WINE_expectedresults.2=%WINE_CURDIR%\bar\fred
set WINE_expectedresults.3=%WINE_CURDIR%\baz\bazbaz
set WINE_expectedresults.4=%WINE_CURDIR%\baz\bazbaz
set WINE_expectedresults.5=%WINE_CURDIR%\baz\bazbaz
set WINE_expectedresults.6=%WINE_CURDIR%\baz\fred
set WINE_expectedresults.7=%WINE_CURDIR%\bazbaz
set WINE_expectedresults.8=%WINE_CURDIR%\bazbaz
set WINE_expectedresults.9=%WINE_CURDIR%\bazbaz
set WINE_expectedresults.10=%WINE_CURDIR%\foo\bazbaz
set WINE_expectedresults.11=%WINE_CURDIR%\foo\fred
set WINE_expectedresults.12=%WINE_CURDIR%\fred
call :SetExpected
for /R %%i in (baz* bazbaz fred ba*) do call temp.bat "%%i"
call :ValidateExpected

echo Strip missing wildcards, keep unwildcarded names
set WINE_expectedresults=6
set WINE_expectedresults.1=%WINE_CURDIR%\bar\jim
set WINE_expectedresults.2=%WINE_CURDIR%\baz\bazbaz
set WINE_expectedresults.3=%WINE_CURDIR%\baz\jim
set WINE_expectedresults.4=%WINE_CURDIR%\bazbaz
set WINE_expectedresults.5=%WINE_CURDIR%\foo\jim
set WINE_expectedresults.6=%WINE_CURDIR%\jim
call :SetExpected
for /R %%i in (baz* fred* jim) do call temp.bat "%%i"
call :ValidateExpected

echo for /R passed
echo --- Complex wildcards unix and windows slash
cd ..
echo Windows slashes, valid path
for %%f in (foobar\baz\bazbaz) do echo ASIS: %%f
for %%f in (foobar\baz\*) do echo WC  : %%f
echo Windows slashes, invalid path
for %%f in (foobar\jim\bazbaz) do echo ASIS: %%f
for %%f in (foobar\jim\*) do echo WC  : %%f
echo Unix slashes, valid path
for %%f in (foobar/baz/bazbaz) do echo ASIS: %%f
for %%f in (foobar/baz/*) do echo WC  : %%f
echo Unix slashes, invalid path
for %%f in (foobar/jim/bazbaz) do echo ASIS: %%f
for %%f in (foobar/jim/*) do echo WC  : %%f
echo Done
rd /s/Q foobar
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
echo --- rems inside for loops
for /f %%i IN ("hello") DO (
   REM foo|echo ERROR unexpected execution 1
   @REM foo|echo ERROR unexpected execution 2
   @     REM foo|echo ERROR unexpected execution 3
)
echo --- ifs inside for loops
for %%i in (test) do (
    echo a1
    if 1==1 (
        echo b1
    ) else (
        echo c1
    )
    echo d1
)
for %%i in (test) do (
    echo a2
    if 1==1 (
        echo b2
    ) else echo c2
    echo d2
)
for %%i in (test) do (
    echo a3
    if 1==0 (
        echo b3
    ) else echo c3
    echo d3
)
for %%i in (test) do (
    echo a4
    if 1==0 (
        echo b4
    ) else (
        echo c4
    )
    echo d4
)
echo --- EXIT /B inside FOR loops
goto :after_exitBinsideForLoop
:exitBinsideForLoop
for /l %%i in (1,1,3) do (
  echo %%i
  if %%i==2 exit /b 0
)
:after_exitBinsideForLoop
call :exitBinsideForLoop
echo --- set /a
goto :testseta

Rem Ideally for /f can be used rather than building a command to execute
rem but that does not work on NT4
:checkenvvars
if "%1"=="" goto :eof
call :executecmd set wine_result=%%%1%%
if "%wine_result%"=="%2" (
  echo %1 correctly %2
) else echo ERROR: %1 incorrectly %wine_result% [%2]
set %1=
shift
shift
rem shift
goto :checkenvvars
:executecmd
%*
goto :eof

:testseta
rem No output when using "set expr" syntax, unless in interactive mode
rem Need to use "set envvar=expr" to use in a batch script
echo ------ individual operations
set WINE_foo=0
set /a WINE_foo=1 +2 & call :checkenvvars WINE_foo 3
set /a WINE_foo=1 +-2 & call :checkenvvars WINE_foo -1
set /a WINE_foo=1 --2 & call :checkenvvars WINE_foo 3
set /a WINE_foo=2* 3 & call :checkenvvars WINE_foo 6
set /a WINE_foo=-2* -5 & call :checkenvvars WINE_foo 10
set /a WINE_foo=12/3 & call :checkenvvars WINE_foo 4
set /a WINE_foo=13/3 & call :checkenvvars WINE_foo 4
set /a WINE_foo=-13/3 & call :checkenvvars WINE_foo -4
rem FIXME Divide by zero should return an error, but error messages cannot be tested with current infrastructure
set /a WINE_foo=5 %% 5 & call :checkenvvars WINE_foo 0
set /a WINE_foo=5 %% 3 & call :checkenvvars WINE_foo 2
set /a WINE_foo=5 %% -3 & call :checkenvvars WINE_foo 2
set /a WINE_foo=-5 %% -3 & call :checkenvvars WINE_foo -2
set /a WINE_foo=1 ^<^< 0 & call :checkenvvars WINE_foo 1
set /a WINE_foo=1 ^<^< 2 & call :checkenvvars WINE_foo 4
set /a WINE_foo=1 ^<^< -2 & call :checkenvvars WINE_foo 0
set /a WINE_foo=-1 ^<^< -2 & call :checkenvvars WINE_foo 0
set /a WINE_foo=-1 ^<^< 2 & call :checkenvvars WINE_foo -4
set /a WINE_foo=9 ^>^> 0 & call :checkenvvars WINE_foo 9
set /a WINE_foo=9 ^>^> 2 & call :checkenvvars WINE_foo 2
set /a WINE_foo=9 ^>^> -2 & call :checkenvvars WINE_foo 0
set /a WINE_foo=-9 ^>^> -2 & call :checkenvvars WINE_foo -1
set /a WINE_foo=-9 ^>^> 2 & call :checkenvvars WINE_foo -3
set /a WINE_foo=5 ^& 0 & call :checkenvvars WINE_foo 0
set /a WINE_foo=5 ^& 1 & call :checkenvvars WINE_foo 1
set /a WINE_foo=5 ^& 3 & call :checkenvvars WINE_foo 1
set /a WINE_foo=5 ^& 4 & call :checkenvvars WINE_foo 4
set /a WINE_foo=5 ^& 1 & call :checkenvvars WINE_foo 1
set /a WINE_foo=5 ^| 0 & call :checkenvvars WINE_foo 5
set /a WINE_foo=5 ^| 1 & call :checkenvvars WINE_foo 5
set /a WINE_foo=5 ^| 3 & call :checkenvvars WINE_foo 7
set /a WINE_foo=5 ^| 4 & call :checkenvvars WINE_foo 5
set /a WINE_foo=5 ^| 1 & call :checkenvvars WINE_foo 5
set /a WINE_foo=5 ^^ 0 & call :checkenvvars WINE_foo 5
set /a WINE_foo=5 ^^ 1 & call :checkenvvars WINE_foo 4
set /a WINE_foo=5 ^^ 3 & call :checkenvvars WINE_foo 6
set /a WINE_foo=5 ^^ 4 & call :checkenvvars WINE_foo 1
set /a WINE_foo=5 ^^ 1 & call :checkenvvars WINE_foo 4
echo ------ precedence and grouping
set /a WINE_foo=4 + 2*3 & call :checkenvvars WINE_foo 10
set /a WINE_foo=(4+2)*3 & call :checkenvvars WINE_foo 18
set /a WINE_foo=4 * 3/5 & call :checkenvvars WINE_foo 2
set /a WINE_foo=(4 * 3)/5 & call :checkenvvars WINE_foo 2
set /a WINE_foo=4 * 5 %% 4 & call :checkenvvars WINE_foo 0
set /a WINE_foo=4 * (5 %% 4) & call :checkenvvars WINE_foo 4
set /a WINE_foo=3 %% (5 + 8 %% 3 ^^ 2) & call :checkenvvars WINE_foo 3
set /a WINE_foo=3 %% (5 + 8 %% 3 ^^ -2) & call :checkenvvars WINE_foo 3
echo ------ octal and hexadecimal
set /a WINE_foo=0xf + 3 & call :checkenvvars WINE_foo 18
set /a WINE_foo=0xF + 3 & call :checkenvvars WINE_foo 18
set /a WINE_foo=015 + 2 & call :checkenvvars WINE_foo 15
set /a WINE_foo=3, 8+3,0 & call :checkenvvars WINE_foo 3
echo ------ variables
set /a WINE_foo=WINE_bar=3, WINE_bar+1 & call :checkenvvars WINE_foo 3 WINE_bar 3
set /a WINE_foo=WINE_bar=3, WINE_bar+=1 & call :checkenvvars WINE_foo 3 WINE_bar 4
set /a WINE_foo=WINE_bar=3, WINE_baz=1, WINE_baz+=WINE_bar, WINE_baz & call :checkenvvars WINE_foo 3 WINE_bar 3 WINE_baz 4
set WINE_bar=3
set /a WINE_foo=WINE_bar*= WINE_bar & call :checkenvvars WINE_foo 9 WINE_bar 9
set /a WINE_foo=WINE_whateverNonExistingVar & call :checkenvvars WINE_foo 0
set WINE_bar=4
set /a WINE_foo=WINE_whateverNonExistingVar + WINE_bar & call :checkenvvars WINE_foo 4 WINE_bar 4
set WINE_bar=4
set /a WINE_foo=WINE_bar -= WINE_bar + 7 & call :checkenvvars WINE_foo -7 WINE_bar -7
set WINE_bar=-7
set /a WINE_foo=WINE_bar /= 3 + 2 & call :checkenvvars WINE_foo -1 WINE_bar -1
set /a WINE_foo=WINE_bar=5, WINE_bar %%=2 & call :checkenvvars WINE_foo 5 WINE_bar 1
set WINE_bar=1
set /a WINE_foo=WINE_bar ^<^<= 2 & call :checkenvvars WINE_foo 4 WINE_bar 4
set WINE_bar=4
set /a WINE_foo=WINE_bar ^>^>= 2 & call :checkenvvars WINE_foo 1 WINE_bar 1
set WINE_bar=1
set /a WINE_foo=WINE_bar ^&= 2 & call :checkenvvars WINE_foo 0 WINE_bar 0
set /a WINE_foo=WINE_bar=5, WINE_bar ^|= 2 & call :checkenvvars WINE_foo 5 WINE_bar 7
set /a WINE_foo=WINE_bar=5, WINE_bar ^^= 2 & call :checkenvvars WINE_foo 5 WINE_bar 7
set WINE_baz=4
set /a WINE_foo=WINE_bar=19, WINE_bar %%= 4 + (WINE_baz %%= 7) & call :checkenvvars WINE_foo 19 WINE_bar 3 WINE_baz 4
echo --- quotes
set /a WINE_foo=1
call :checkenvvars WINE_foo 1
set /a "WINE_foo=1"
call :checkenvvars WINE_foo 1
set /a WINE_foo=1,WINE_bar=2
call :checkenvvars WINE_foo 1 WINE_bar 2
set /a "WINE_foo=1,WINE_bar=2"
call :checkenvvars WINE_foo 1 WINE_bar 2
set /a "WINE_foo=1","WINE_bar=2"
call :checkenvvars WINE_foo 1 WINE_bar 2
set /a ""WINE_foo=1","WINE_bar=2""
call :checkenvvars WINE_foo 1 WINE_bar 2
set /a WINE_foo=1,WINE_bar=2,WINE_baz=3
call :checkenvvars WINE_foo 1 WINE_bar 2 WINE_baz 3
set /a "WINE_foo=1,WINE_bar=2,WINE_baz=3"
call :checkenvvars WINE_foo 1 WINE_bar 2 WINE_baz 3
set /a "WINE_foo=1","WINE_bar=2","WINE_baz=3"
call :checkenvvars WINE_foo 1 WINE_bar 2 WINE_baz 3
set /a ""WINE_foo=1","WINE_bar=2","WINE_baz=3""
call :checkenvvars WINE_foo 1 WINE_bar 2 WINE_baz 3
set /a ""WINE_foo=1","WINE_bar=2"","WINE_baz=3"
call :checkenvvars WINE_foo 1 WINE_bar 2 WINE_baz 3
set /a """"""WINE_foo=1""""""
call :checkenvvars WINE_foo 1
set /a """"""WINE_foo=1","WINE_bar=5""","WINE_baz=2""
call :checkenvvars WINE_foo 1 WINE_bar 5 WINE_baz 2
set /a WINE_foo="3"+"4"+"5+6"
call :checkenvvars WINE_foo 18
set WINE_foo=3
set /a WINE_bar="WINE_""foo"+4
call :checkenvvars WINE_foo 3 WINE_bar 7
echo --- whitespace are ignored between double char operators
set WINE_foo=4
set WINE_bar=5
set /a     WINE_foo   +    = 6
set /a     WINE_bar     *    = WINE_foo
call :checkenvvars WINE_foo 10 WINE_bar 50
set WINE_foo=4
set WINE_bar=5
set /a     WINE_foo   +    = "6  < < 7"
set /a     WINE_bar     *    = WINE_foo  +  WINE_foo
call :checkenvvars WINE_foo 772 WINE_bar 7720
set /a     WINE_foo=6 7
set /a     WINE_ var1=8
set WINE_foo=
echo --- invalid operator sequence
set WINE_foo=4
set /a =4
set /a *=4
set /a ^>=4"
set /a ^<=4"
set /a WINE_foo^>^<=4
echo %WINE_foo%
set /a WINE_foo^>^>^>=4
echo %WINE_foo%
echo ----- negative prefix
set /a WINE_foo=-1
call :checkenvvars WINE_foo -1
set /a WINE_foo=--1
call :checkenvvars WINE_foo 1
set /a WINE_foo=3--3
call :checkenvvars WINE_foo 6
set /a WINE_foo=3---3
call :checkenvvars WINE_foo 0
set /a WINE_foo=3----3
call :checkenvvars WINE_foo 6
set /a WINE_foo=-~1
call :checkenvvars WINE_foo 2
set /a WINE_foo=~-1
call :checkenvvars WINE_foo 0
set /a WINE_foo=3+-~1
call :checkenvvars WINE_foo 5
set /a WINE_foo=3+~-1
call :checkenvvars WINE_foo 3
echo ----- assignment tests involving the end destination
set WINE_foo=3
set /a WINE_foo+=3+(WINE_foo=4)
call :checkenvvars WINE_foo 11
set WINE_foo=2
set /a WINE_bar=3+(WINE_foo=6)
call :checkenvvars WINE_foo 6 WINE_bar 9
set WINE_foo=2
set /a WINE_bar=3+(WINE_foo=6,WINE_baz=7)
call :checkenvvars WINE_foo 6 WINE_bar 10 WINE_baz 7
set WINE_foo=2
set /a WINE_bar=WINE_foo=7
call :checkenvvars WINE_foo 7 WINE_bar 7
echo ----- equal precedence on stack
rem Unary - don't reduce if precedence is equal
set /a WINE_foo=!!1
call :checkenvvars WINE_foo 1
set /a WINE_foo=!!0
call :checkenvvars WINE_foo 0
set /a WINE_foo=~~1
call :checkenvvars WINE_foo 1
set /a WINE_foo=~~0
call :checkenvvars WINE_foo 0
set /a WINE_foo=--1
call :checkenvvars WINE_foo 1
set /a WINE_foo=+-1
call :checkenvvars WINE_foo -1
set /a WINE_foo=-+1
call :checkenvvars WINE_foo -1
set /a WINE_foo=++1
call :checkenvvars WINE_foo 1
set /a WINE_foo=!~1
call :checkenvvars WINE_foo 0
set /a WINE_foo=~!1
call :checkenvvars WINE_foo -1
set /a WINE_foo=!-1
call :checkenvvars WINE_foo 0
set /a WINE_foo=-!1
call :checkenvvars WINE_foo 0
set /a WINE_foo=!-0
call :checkenvvars WINE_foo 1
set /a WINE_foo=-!0
call :checkenvvars WINE_foo -1
rem Aritmatic - Reduce if precedence is equal
set /a WINE_foo=10*5/2
call :checkenvvars WINE_foo 25
set /a WINE_foo=5/2*10
call :checkenvvars WINE_foo 20
set /a WINE_foo=10/5/2
call :checkenvvars WINE_foo 1
set /a WINE_foo=5%%2*4
call :checkenvvars WINE_foo 4
set /a WINE_foo=10-5+2
call :checkenvvars WINE_foo 7
set /a WINE_foo=1^<^<4^>^>1
call :checkenvvars WINE_foo 8
rem Assignment - don't reduce if precedence is equal
set /a WINE_foo=5
set /a WINE_bar=WINE_foo=6
call :checkenvvars WINE_foo 6 WINE_bar 6

echo --- for /F
mkdir foobar & cd foobar
echo ------ string argument
rem NT4 does not support usebackq
for /F %%i in ("a b c") do echo %%i
for /F %%i in (  "a b c"    ) do echo X%%iX
for /f usebackq %%i in ('a b c') do echo %%i>output_file
if not exist output_file (echo no output) else (type output_file & del output_file)
for /f usebackq %%i in (   'a b c'   ) do echo X%%iX>output_file
if not exist output_file (echo no output) else (type output_file & del output_file)
for /f %%i in ("a ") do echo %%i
for /f usebackq %%i in ('a ') do echo %%i>output_file
if not exist output_file (echo no output) else (type output_file & del output_file)
for /f %%i in ("a") do echo %%i
for /f usebackq %%i in ('a') do echo %%i>output_file
if not exist output_file (echo no output) else (type output_file & del output_file)
fOr /f %%i in (" a") do echo %%i
for /f usebackq %%i in (' a') do echo %%i>output_file
if not exist output_file (echo no output) else (type output_file & del output_file)
for /f %%i in (" a ") do echo %%i
for /f usebackq %%i in (' a ') do echo %%i>output_file
if not exist output_file (echo no output) else (type output_file & del output_file)
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
echo ------ quoting and file access
echo a >  f.zzz
echo b >> f.zzz
erase f2.zzz
for /f %%a in (f.zzz) do echo A%%a
for /f %%a in ("f.zzz") do echo B%%a
for /f %%a in (f2.zzz) do echo C%%a
for /f %%a in ("f2.zzz") do echo D%%a
for /f "usebackq" %%a in (f.zzz) do echo E%%a
for /f "usebackq" %%a in ("f.zzz") do echo F%%a
for /f "usebackq" %%a in (f2.zzz) do echo G%%a
for /f "usebackq" %%a in ("f2.zzz") do echo H%%a
for /f %%a in (f*.zzz) do echo I%%a
for /f %%a in ("f*.zzz") do echo J%%a
erase f.zzz
echo ------ command argument
rem Not implemented on NT4, need to skip it as no way to get output otherwise
if "%CD%"=="" goto :SkipFORFcmdNT4
for /f %%i in ('echo.Passed1') do echo %%i
for /f "usebackq" %%i in (`echo.Passed2`) do echo %%i
for /f usebackq %%i in (`echo.Passed3`) do echo %%i
for /f "usebackq" %%i in (`"c:\windows\system32\cmd.exe" /C echo Passed4`) do echo %%i
for /f "usebackq" %%i in (`""c:\windows\system32\cmd.exe" /C echo Passed5"`) do echo %%i
for /f %%i in (  'echo.Passed6'  ) do echo %%i
for /f "usebackq" %%i in (   `echo.Passed7` ) do echo %%i
goto :ContinueFORF
:SkipFORFcmdNT4
for /l %%i in (1,1,7) do echo Missing functionality - Broken%%i
:ContinueFORF
rem FIXME: Rest not testable right now in wine: not implemented and would need
rem preliminary grep-like program implementation (e.g. like findstr or fc) even
rem for a simple todo_wine test
rem (for /f "usebackq" %%i in (`echo z a b`) do echo %%i) || echo not supported
rem (for /f usebackq %%i in (`echo z a b`) do echo %%i) || echo not supported
echo ------ eol option
if "%CD%"=="" goto :SkipFORFeolNT4
echo Line one>foo
echo and Line two>>foo
echo Line three>>foo
for /f "eol=L" %%i in (foo) do echo %%i
for /f "eol=a" %%i in (foo) do echo %%i
del foo
goto :ContinueFORFeol
:SkipFORFeolNT4
for /l %%i in (1,1,3) do echo Broken NT4 functionality%%i
:ContinueFORFeol
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
for /f "skip=2@tab@" %%i in (foo) do echo %%i
for /f "skip=3" %%i in (foo) do echo %%i > output_file
if not exist output_file (echo no output) else (del output_file)
for /f "skip=4" %%i in (foo) do echo %%i > output_file
if not exist output_file (echo no output) else (del output_file)
for /f "skip=02" %%i in (foo) do echo %%i
for /f "skip=0x2" %%i in (foo) do echo %%i
for /f "skip=1" %%i in ("skipme") do echo %%i > output_file
if not exist output_file (echo no output) else (del output_file)
echo ------ tokens= option
rem Basic
for /f %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m o=%%o
for /f "tokens=2" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m o=%%o
for /f "tokens=1,3,5-7" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m o=%%o
rem Show * means the rest
for /f "tokens=1,5*" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m o=%%o
for /f "tokens=6,9*" %%i in ("a b c d e f g h i j k l m n o p q r s t u v w x y z") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m o=%%o
rem Show * means the rest (not tokenized and rebuilt)
for /f "tokens=6,9*" %%i in ("a b c d e f g h i j k l m  n;;==  o p q r s t u v w x y z") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m o=%%o
rem Order is irrelevant
for /f "tokens=1,2,3*" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m n=%%n o=%%o
for /f "tokens=3,2,1*" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m n=%%n o=%%o
rem Duplicates are ignored
for /f "tokens=1,2,1*" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m n=%%n o=%%o
rem errors can exist
(for /f "tokens=1,2*,4" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m n=%%n o=%%o) || echo failure %%i
rem Large tokens are allowed
for /f "tokens=25,1,5*" %%i in ("a b c d e f g h i j k l m n o p q r s t u v w x y z A B C D E F G H I J K L M N O P Q R S T U V W X Y Z") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m n=%%n o=%%o
rem Show tokens blanked in advance regardless of uniqueness of requested tokens
for /f "tokens=1,1,1,2*" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m n=%%n o=%%o
for /f "tokens=1-2,1-2,1-2" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m n=%%n o=%%o
rem Show mapping of most of the ASCII characters (on top of letters & digits)
for /f "tokens=1-20" %%u in ("a b c d e f g h i j k l m n o p q r s t u v w x y z A B C D E F G H I J K L M N O P Q R S T U V W X Y Z") do echo u=%%u v=%%v w=%%w x=%%x y=%%y z=%%z A=%%A a=%%a
for /f "tokens=1-20" %%U in ("a b c d e f g h i j k l m n o p q r s t u v w x y z A B C D E F G H I J K L M N O P Q R S T U V W X Y Z") do echo U=%%U V=%%V W=%%W X=%%X Y=%%Y Z=%%Z ^[=%%^[ ^\=%%^\ ^]=%%^] ^^=%%^^ _=%%_ `=%%` A=%%A a=%%a
rem Testing limits (max number of contiguous variables, limit at 127)
(for /f "tokens=1-31" %%A in ("a b c d e f g h i j k l m n o p q r s t u v w x y z A B C D E F G H I J K L M N O P Q R S T U V W X Y Z") do echo U=%%U V=%%V W=%%W X=%%X Y=%%Y Z=%%Z ^[=%%^[ ^\=%%^\ ^]=%%^] ^^=%%^^ _=%%_ `=%%` A=%%A a=%%a) || echo failure
(for /f "tokens=1-32" %%A in ("a b c d e f g h i j k l m n o p q r s t u v w x y z A B C D E F G H I J K L M N O P Q R S T U V W X Y Z") do echo U=%%U V=%%V W=%%W X=%%X Y=%%Y Z=%%Z ^[=%%^[ ^\=%%^\ ^]=%%^] ^^=%%^^ _=%%_ `=%%` A=%%A a=%%a) || echo failure %%A
for /f "tokens=1-20" %%} in ("a b c d e f g h i j k l m n o p q r s t u v w x y z A B C D E F G H I J K L M N O P Q R S T U V W X Y Z") do echo ^}=%%^} ^~=%%^~
rem Show negative ranges have no effect
for /f "tokens=1-3,5" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m o=%%o
for /f "tokens=3-1,5" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m o=%%o
rem Show duplicates stop * from working
for /f "tokens=1,2,3*" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m n=%%n o=%%o
for /f "tokens=1,1,3*" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m n=%%n o=%%o
for /f "tokens=2,2,3*" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m n=%%n o=%%o
for /f "tokens=3,2,3*" %%i in ("a b c d e f g") do echo h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m n=%%n o=%%o
rem Special case tokens=* or tokens=n,*
echo 3.14>testfile
FOR /F "tokens=*"  %%A IN (testfile) DO @echo 1:%%A,%%B
FOR /F "tokens=1*" %%A IN (testfile) DO @echo 2:%%A,%%B
FOR /F "tokens=2*" %%A IN (testfile) DO @echo 3:%%A,%%B
FOR /F "tokens=1,*@tab@delims=." %%A IN (testfile) DO @echo 4:%%A,%%B
del testfile
cd ..
rd /s/q foobar
echo ------ parameter splitting
echo forFParameterSplittingFunc "myparam1=myvalue1 myparam2=myparam2" mytest> foo
for /f "tokens=1 delims=;" %%i in (foo) do (call :%%i)
del foo
for /f "tokens=1 delims=;" %%i in ("forFParameterSplittingFunc "myparam1^=myvalue1 myparam2^=myparam2" mytest") do (call :%%i)
goto :forFParameterSplittingEnd
:forFParameterSplittingFunc
echo %~0 %~1 %~2 %~3 %~4 %~5
goto :eof
:forFParameterSplittingEnd
echo 3.14>testfile
FOR /F "delims=. tokens=*"  %%A IN (testfile) DO @echo 4:%%A,%%B
FOR /F "delims=. tokens=1*" %%A IN (testfile) DO @echo 5:%%A,%%B
FOR /F "delims=. tokens=2*" %%A IN (testfile) DO @echo 6:%%A,%%B
FOR /F "delims=. tokens=3*" %%A IN (testfile) DO @echo 7:%%A,%%B
del testfile
rem file contains NUL, created by the .exe
for /f %%A in (nul_test_file) DO echo %%A
for /f "tokens=*" %%A in (nul_test_file) DO echo %%A

echo ------------ Testing del ------------
echo abc > file
echo deleting 'file'
del file
if errorlevel 0 (
    echo errorlevel is 0, good
) else (
    echo unexpected errorlevel, got %errorlevel%
)
if not exist file (
    echo successfully deleted 'file'
) else (
    echo error deleting 'file'
)
echo attempting to delete 'file', even though it is not present
del file
if errorlevel 0 (
    echo errorlevel is 0, good
) else (
    echo unexpected errorlevel, got %errorlevel%
)

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

echo ------------ Testing dir /o ------------
mkdir foobar & cd foobar
echo AAA>a1.aa
mkdir a1.ab
echo A>a1.ac
echo AA>a2.aa
mkdir a2.ac
echo ---
dir /B /O:
echo ---
dir /B /O:GN
echo ---
dir /B /O:G-N
echo ---
dir /B /O:GNE
echo ---
dir /B /O:G-NE
echo ---
dir /B /O:G-E-N
cd .. & rd /s/q foobar
echo ------------ Testing attrib ------------
rem FIXME Add tests for archive, hidden and system attributes + mixed attributes modifications
mkdir foobar & cd foobar
echo foo original contents> foo
attrib
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
rem Modifying associations requires some privileges...
net session >nul 2>&1
if errorlevel 1 goto :SkipAssoc

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
goto ContinueFType
:SkipAssoc
echo --- setting association
echo ---
echo .foo=bar
echo .foo=bar
echo +++
echo .foo=bar
echo --- resetting association
echo ---
echo +++
echo ---


:ContinueFType
echo ------------ Testing ftype ------------
rem Modifying associations requires some privileges...
net session >nul 2>&1
if errorlevel 1 goto :SkipFType
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

echo --- testing association
echo dummy>test.foo
ftype footype=cmd.exe /c "echo '%%1'"
test.foo
ftype footype=cmd.exe /c "echo '%%*'"
test.foo one two three
del test.foo
copy C:\windows\system32\cmd.exe test.foo >nul 2>&1
test.foo /c "echo foobar"
del test.foo

echo --- resetting association
assoc .foo=

rem Removing a file type association doesn't work on XP due to a bug, so a workaround is needed
setlocal EnableDelayedExpansion
set WINE_FOO=original value
ftype footype=
ftype footype > baz
for /F %%i in ('type baz') do (set WINE_FOO=buggyXP)
rem Resetting actually works on wine/NT4, but is reported as failing due to the peculiar test (and non-support for EnabledDelayedExpansion)
rem FIXME Revisit once a grep-like program like ftype is implemented
rem (e.g. to check baz's size using dir /b instead)
echo !WINE_FOO!

rem cleanup registry
echo REGEDIT4> regCleanup.reg
echo.>> regCleanup.reg
echo [-HKEY_CLASSES_ROOT\footype]>> regCleanup.reg
regedit /s regCleanup.reg
set WINE_FOO=
endlocal
cd .. & rd /s/q foobar
goto ContinueCall
:SkipFType
echo --- setting association
echo ---
echo footype=foo_opencmd
echo .foo=footype
echo footype=foo_opencmd
echo +++
echo footype=foo_opencmd
echo --- testing association
echo footype=cmd.exe /c "echo '%%1'"
echo Skipped as not enough permissions
echo footype=cmd.exe /c "echo '%%*'"
echo Skipped as not enough permissions
echo Skipped as not enough permissions
echo --- resetting association
echo original value

:ContinueCall
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
set WINE_FOOBAZ_VAR=foobaz
call echo Should expand %WINE_FOOBAZ_VAR%
set WINE_FOOBAZ_VAR=
echo>batfile
call dir /b
echo>robinfile
if 1==1 call del batfile
dir /b
if exist batfile echo batfile shouldn't exist
rem arcane command, first resets errorlevel, second sets it to one
(call )
echo %ErrorLevel%
(call)
echo %ErrorLevel%
rem ... but not for 'if' or 'for'
call :setError 0
call if 1==1 echo bar 2> nul
echo %ErrorLevel%
call :setError 0
call for %%i in (foo bar baz) do echo %%i 2> nul
echo %ErrorLevel%
rem First look for programs in the path before trying a builtin
echo echo non-builtin dir> dir.cmd
call dir /b
del dir.cmd
rem The below line equates to call (, which does nothing, then the
rem subsequent lines are executed.
call (
  echo Line one
  echo Line two
)
rem The below line equates to call if, which always fails, then the
rem subsequent lines are executed. Note cmd.exe swallows all lines
rem starting with )
call if 1==1 (
  echo Get if
) else (
  echo ... and else!
)
call call call echo passed
set WINE_FOO=WINE_BAR
set WINE_BAR=abc
call echo %%%WINE_FOO%%%
call cmd.exe /c echo %%%WINE_FOO%%%
call echo %%%%%WINE_FOO%%%%%
call cmd.exe /c echo %%%%%WINE_FOO%%%%%

set WINE_BAR=abc
set WINE_FOO=%%WINE_BAR%%

call :call_expand %WINE_FOO% %%WINE_FOO%% %%%WINE_FOO%%%
goto :call_expand_done

:call_expand
set WINE_BAR=def
echo %1 %2 %3
call echo %1 %2 %3
exit /b 0

:call_expand_done

echo --- search with dots
echo @echo a> .bat
call .bat
echo @echo b> f00.bat.bat
call f00.bat || echo fail1
call f00 2> nul || echo fail2

cd .. & rd /s/q foobar

echo --- builtin in expansions

mkdir foobar & cd foobar

set foobar=echo

echo echo %%*>bar.bat
%foobar% bar p1 %foobar%
%%foobar%% bar p2 %%foobar%%
%%%foobar%%% bar p3 %%%foobar%%%
call %foobar% bar cp1 %foobar%
call %%foobar%% bar cp2 %%foobar%%
call %%%foobar%%% bar cp3 %%%foobar%%%
setlocal EnableDelayedExpansion
!foobar! bar b1 !foobar!
!!foobar!! bar b2 !!foobar!!
!!!foobar!!! bar b3 !!!foobar!!!
call !foobar! bar cb1 !foobar!
call !!foobar!! bar cb2 !!foobar!!
call !!!foobar!!! bar cb3 !!!foobar!!!
call !!!!foobar!!!! bar cb4 !!!!foobar!!!!
setlocal DisableDelayedExpansion
set foobar=

cd .. & rd /s/q foobar

echo --- mixing batch and builtins
erase /q echo.bat test.bat 2> NUL
echo @echo foo> echo.bat
echo @echo bar> test.bat & call test.bat
echo @echo.bat bar> test.bat & call test.bat
echo @call echo bar> test.bat & call test.bat
echo @call echo.bat bar> test.bat & call test.bat
erase /q echo.bat 2> NUL
echo @echo bar> test.bat & call test.bat
echo @echo.bat bar> test.bat & call test.bat
echo @call echo bar> test.bat & call test.bat
echo @call echo.bat bar> test.bat & call test.bat
erase /q test.bat 2> NUL

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

echo ---------- Testing copy
md foobar2
cd foobar2
rem Note echo adds 0x0d 0x0a on the end of the line in the file
echo AAA> file1
echo BBBBBB> file2
echo CCCCCCCCC> file3
md dir1
goto :testcopy

:CheckExist
if exist "%1" (
  echo Passed: Found expected %1
) else (
  echo Failed: Did not find expected %1
)
del /q "%1" >nul 2>&1
shift
if not "%1"=="" goto :CheckExist
goto :eof

:CheckNotExist
if not exist "%1" (
  echo Passed: Did not find %1
) else (
  echo Failed: Unexpectedly found %1
  del /q "%1" >nul 2>&1
)
shift
if not "%1"=="" goto :CheckNotExist
goto :eof

rem Note: No way to check file size on NT4 so skip the test
:CheckFileSize
if not exist "%1" (
  echo Failed: File missing when requested filesize check [%2]
  goto :ContinueFileSizeChecks
)
for %%i in (%1) do set WINE_filesize=%%~zi
if "%WINE_filesize%"=="%2" (
    echo Passed: file size check on %1 [%WINE_filesize%]
) else (
  if "%WINE_filesize%"=="%%~zi" (
    echo Skipping file size check on NT4
  ) else (
    echo Failed: file size check on %1 [%WINE_filesize% != %2]
  )
)
:ContinueFileSizeChecks
shift
shift
if not "%1"=="" goto :CheckFileSize
goto :eof

:testcopy

rem -----------------------
rem Simple single file copy
rem -----------------------
rem Simple single file copy, normally used syntax
copy file1 dummy.file >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist dummy.file

rem Simple single file copy, destination supplied as two forms of directory
copy file1 dir1 >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist dir1\file1

copy file1 dir1\ >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist dir1\file1

rem Simple single file copy, destination supplied as fully qualified destination
copy file1 dir1\file99 >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist dir1\file99

rem Simple single file copy, destination not supplied
cd dir1
copy ..\file1 >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist file1
cd ..

rem Simple single file copy, destination supplied as nonexistent directory
copy file1 dir2\ >nul 2>&1
if not errorlevel 1 echo Incorrect errorlevel
call :CheckNotExist dir2 dir2\file1

rem -----------------------
rem Wildcarded copy
rem -----------------------
rem Simple single file copy, destination supplied as two forms of directory
copy file? dir1 >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist dir1\file1 dir1\file2 dir1\file3

copy file* dir1\ >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist dir1\file1 dir1\file2 dir1\file3

rem Simple single file copy, destination not supplied
cd dir1
copy ..\file*.* >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist file1 file2 file3
cd ..

rem Simple wildcarded file copy, destination supplied as nonexistent directory
copy file? dir2\ >nul 2>&1
if not errorlevel 1 echo Incorrect errorlevel
call :CheckNotExist dir2 dir2\file1 dir2\file2 dir2\file3

rem ------------------------------------------------
rem Confirm overwrite works (cannot test prompting!)
rem ------------------------------------------------
copy file1 testfile >nul 2>&1
copy /y file2 testfile >nul 2>&1
call :CheckExist testfile

rem ------------------------------------------------
rem Test concatenation
rem ------------------------------------------------
rem simple case, no wildcards
copy file1+file2 testfile >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist testfile

rem simple case, wildcards, no concatenation
copy file* testfile >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist testfile

rem simple case, wildcards, and concatenation
echo ddddd > fred
copy file*+fred testfile >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist testfile

rem simple case, wildcards, and concatenation
copy fred+file* testfile >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist testfile

rem Calculate destination name
copy fred+file* dir1 >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist dir1\fred

rem Calculate destination name
copy fred+file* dir1\ >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist dir1\fred

rem Calculate destination name (none supplied)
cd dir1
copy ..\fred+..\file* >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist fred

copy ..\fr*+..\file1  >nul 2>&1
if errorlevel 1 echo Incorrect errorlevel
call :CheckExist fred
cd ..

rem ******************************************************************
rem ASCII and BINARY tests
rem Note: hard coded numbers deliberate because need to ensure whether
rem an additional EOF has been added or not. There is no way to handle
rem EOFs in batch, so assume if a single byte appears, it's an EOF!
rem ******************************************************************

rem Confirm original sizes of file1,2,3
call :CheckFileSize file1 5 file2 8 file3 11

cd dir1

rem ----------------------------------------------
rem Show concatenation defaults copy to ascii mode
rem ----------------------------------------------
rem Simple default copy source to destination (should not append EOF 5)
copy ..\file1 file1_default >nul 2>&1
call :CheckFileSize file1_default 5

rem Simple binary copy source to destination (should not append EOF 5)
copy /b ..\file1 file1_default2 >nul 2>&1
call :CheckFileSize file1_default2 5

rem Simple ascii copy source to destination (should append EOF 5+1, 8+1, 11+1)
copy /a ..\file1 file1_plus_eof >nul 2>&1
call :CheckFileSize file1_plus_eof 6
copy /a ..\file2 file2_plus_eof >nul 2>&1
call :CheckFileSize file2_plus_eof 9
copy /a ..\file3 file3_plus_eof >nul 2>&1
call :CheckFileSize file3_plus_eof 12

rem Concat 2 files, ascii mode - (only one EOF on the end 5+8+1)
copy /a ..\file1+..\file2 file12_plus_eof >nul 2>&1
call :CheckFileSize file12_plus_eof 14

rem Concat 2 files, binary mode - (no EOF on the end 5+8)
copy /b ..\file1+..\file2 file12_no_eof >nul 2>&1
call :CheckFileSize file12_no_eof 13

rem Concat 2 files, default mode - (one EOF on the end 5+8+1)
copy ..\file1+..\file2 file12_eof2 >nul 2>&1
call :CheckFileSize file12_eof2 14

rem Test copying when destination is one of the sources.
rem Concat file1+file2+file3 into file1, should produce file1+file2+file3 = 24
copy /y ..\file? .\ >nul 2>&1
copy /y /b file1+file2+file3 file1 >nul 2>&1
call :CheckFileSize file1 24

rem Concat file1+file2+file3 into file2, should produce file1+file3 = 16
copy /y ..\file? .\ >nul 2>&1
copy /y /b file1+file2+file3 file2 >nul 2>&1
call :CheckFileSize file2 16

rem Concat file1+file2+file3 into file3, should produce file1+file2 = 13
copy /y ..\file? .\ >nul 2>&1
copy /y /b file1+file2+file3 file3 >nul 2>&1
call :CheckFileSize file3 13

rem --------------------------------------------------------------
rem Show ascii source copy stops at first EOF, binary does the lot
rem --------------------------------------------------------------
copy file1_plus_eof /b file1_binary_srccopy /b >nul 2>&1
call :CheckFileSize file1_binary_srccopy 6

copy file1_plus_eof /a file1_ascii_srccopy /b >nul 2>&1
call :CheckFileSize file1_ascii_srccopy 5

rem --------------------------------------------------------------
rem Show results of concatenating files (ending in EOFs) and /a /b
rem --------------------------------------------------------------

rem Default and ascii copy reads as ascii, stripping EOFs, so 6-1 + 9-1 + 12-1 + 1
copy file1_plus_eof+file2_plus_eof+file3_plus_eof file123_default_copy >nul 2>&1
call :CheckFileSize file123_default_copy 25
copy /a file1_plus_eof+file2_plus_eof+file3_plus_eof file123_ascii_copy >nul 2>&1
call :CheckFileSize file123_ascii_copy 25

rem In binary mode, we get 3 eofs, so 6 + 9 + 12 = 27
copy /b file1_plus_eof + file2_plus_eof + file3_plus_eof file123_binary_copy >nul 2>&1
call :CheckFileSize file123_binary_copy 27

rem We can select which we want the eofs from by postfixing it with /a or /b
rem so here have first and third with eof, second as ascii 6 + 9-1 + 12
copy file1_plus_eof /b + file2_plus_eof /a + file3_plus_eof /b file123_mixed_copy1 >nul 2>&1
call :CheckFileSize file123_mixed_copy1 26

rem By postfixing the destination with /a, we ask for an ascii destination which appends EOF
rem so here have first and third with eof, second as ascii 6 + 9-1 + 12 + extra EOF
rem Note the delta between this and the previous one also shows that the destination
rem ascii/binary is inherited from the last /a or /b on the line
copy file1_plus_eof /b + file2_plus_eof /a + file3_plus_eof /b file123_mixed_copy2 /a >nul 2>&1
call :CheckFileSize file123_mixed_copy2 27

rem so here have second with eof, first and third as ascii 6-1 + 9 + 12-1
rem Note the delta between the next two also shows that the destination ascii/binary is
rem inherited from the last /a or /b on the line, so the first has an extra EOF
copy file1_plus_eof /a + file2_plus_eof /b + file3_plus_eof /a file123_mixed_copy3 >nul 2>&1
call :CheckFileSize file123_mixed_copy3 26
copy file1_plus_eof /a + file2_plus_eof /b + file3_plus_eof /a file123_mixed_copy4 /b >nul 2>&1
call :CheckFileSize file123_mixed_copy4 25

rem -------------------------------------------------------------------------------------------
rem This shows when concatenating, an ascii destination always adds on an EOF but when we
rem are not concatenating, it's a direct copy regardless of destination if being read as binary
rem -------------------------------------------------------------------------------------------

rem All 3 have eof's, plus an extra = 6 + 9 + 12 + eof
copy /b file1_plus_eof + file2_plus_eof + file3_plus_eof file123_mixed_copy5 /a >nul 2>&1
call :CheckFileSize file123_mixed_copy5 28

rem All 2 have eof's, plus an extra = 6 + 12 + eof
copy /b file1_plus_eof + file3_plus_eof file123_mixed_copy6 /a >nul 2>&1
call :CheckFileSize file123_mixed_copy6 19

rem One file has EOF, but doesn't get an extra one, i.e. 6
copy /b file1_plus_eof file123_mixed_copy7 /a >nul 2>&1
call :CheckFileSize file123_mixed_copy7 6

rem Syntax means concatenate so ascii destination kicks in
copy /b file1_plus_eof* file123_mixed_copy8 /a >nul 2>&1
call :CheckFileSize file123_mixed_copy8 7

del *.* /q
cd ..

rem ---------------------------------------
rem Error combinations
rem ---------------------------------------
rem Specify source directory but name is a file
call :setError 0
copy file1\ dir1\ >NUL 2>&1
if errorlevel 1 echo Passed: errorlevel invalid check 1
if not errorlevel 1 echo Failed: errorlevel invalid check 1
call :CheckNotExist dir1\file1

rem Overwrite same file
call :setError 0
copy file1 file1 >NUL 2>&1
if errorlevel 1 echo Passed: errorlevel invalid check 2
if not errorlevel 1 echo Failed: errorlevel invalid check 2

rem Supply same file identified as a directory
call :setError 0
copy file1 file1\ >NUL 2>&1
if errorlevel 1 echo Passed: errorlevel invalid check 3
if not errorlevel 1 echo Failed: errorlevel invalid check 3

cd ..
rd foobar2 /s /q

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
rem %CD% does not work on NT4 so use the following workaround
for /d %%i in (.) do set WINE_CURDIR=%%~dpnxi
echo @echo off> test.cmd
echo echo %%WINE_VAR%%>> test.cmd
echo setlocal>> test.cmd
echo set WINE_VAR=localval>> test.cmd
echo md foobar2>> test.cmd
echo cd foobar2>> test.cmd
echo echo %%WINE_VAR%%>> test.cmd
echo for /d %%%%i in (.) do echo %%%%~dpnxi>> test.cmd
echo endlocal>> test.cmd
echo echo %%WINE_VAR%%>> test.cmd
echo for /d %%%%i in (.) do echo %%%%~dpnxi>> test.cmd
set WINE_VAR=globalval
call test.cmd
echo %WINE_VAR%
for /d %%i in (.) do echo %%~dpnxi
cd /d %WINE_CURDIR%
rd foobar2
set WINE_VAR=
echo --- setlocal with no corresponding endlocal
echo @echo off> test.cmd
echo echo %%WINE_VAR%%>> test.cmd
echo setlocal>> test.cmd
echo set WINE_VAR=localval>> test.cmd
echo md foobar2>> test.cmd
echo cd foobar2>> test.cmd
echo echo %%WINE_VAR%%>> test.cmd
echo for /d %%%%i in (.) do echo %%%%~dpnxi>> test.cmd
set WINE_VAR=globalval
rem %CD% does not work on NT4 so use the following workaround
for /d %%i in (.) do set WINE_CURDIR=%%~dpnxi
call test.cmd
echo %WINE_VAR%
for /d %%i in (.) do echo %%~dpnxi
cd /d %WINE_CURDIR%
rd foobar2
set WINE_VAR=
echo --- setlocal within same batch program
set WINE_var1=one
set WINE_var2=
set WINE_var3=
rem %CD% does not work on NT4 so use the following workaround
for /d %%i in (.) do set WINE_CURDIR=%%~dpnxi
setlocal
set WINE_var2=two
mkdir foobar2
cd foobar2
setlocal
set WINE_var3=three
if "%WINE_var1%"=="one" echo Var1 ok 1
if "%WINE_var2%"=="two" echo Var2 ok 2
if "%WINE_var3%"=="three" echo Var3 ok 3
for /d %%i in (.) do set WINE_curdir2=%%~dpnxi
if "%WINE_curdir2%"=="%WINE_CURDIR%\foobar2" echo Directory is ok 1
endlocal
if "%WINE_var1%"=="one" echo Var1 ok 1
if "%WINE_var2%"=="two" echo Var2 ok 2
if "%WINE_var3%"=="" echo Var3 ok 3
for /d %%i in (.) do set WINE_curdir2=%%~dpnxi
if "%WINE_curdir2%"=="%WINE_CURDIR%\foobar2" echo Directory is ok 2
endlocal
if "%WINE_var1%"=="one" echo Var1 ok 1
if "%WINE_var2%"=="" echo Var2 ok 2
if "%WINE_var3%"=="" echo Var3 ok 3
for /d %%i in (.) do set WINE_curdir2=%%~dpnxi
if "%WINE_curdir2%"=="%WINE_CURDIR%" echo Directory is ok 3
rd foobar2 /s /q
set WINE_var1=

echo --- Mismatched set and end locals
mkdir foodir2 2>nul
mkdir foodir3 2>nul
mkdir foodir4 2>nul
rem %CD% does not work on NT4 so use the following workaround
for /d %%i in (.) do set WINE_curdir=%%~dpnxi

echo @echo off> 2set1end.cmd
echo echo %%WINE_var%%>> 2set1end.cmd
echo setlocal>> 2set1end.cmd
echo set WINE_VAR=2set1endvalue1>> 2set1end.cmd
echo cd ..\foodir3>> 2set1end.cmd
echo setlocal>> 2set1end.cmd
echo set WINE_VAR=2set1endvalue2>> 2set1end.cmd
echo cd ..\foodir4>> 2set1end.cmd
echo endlocal>> 2set1end.cmd
echo echo %%WINE_var%%>> 2set1end.cmd
echo for /d %%%%i in (.) do echo %%%%~dpnxi>> 2set1end.cmd

echo @echo off> 1set2end.cmd
echo echo %%WINE_var%%>> 1set2end.cmd
echo setlocal>> 1set2end.cmd
echo set WINE_VAR=1set2endvalue1>> 1set2end.cmd
echo cd ..\foodir3>> 1set2end.cmd
echo endlocal>> 1set2end.cmd
echo echo %%WINE_var%%>> 1set2end.cmd
echo for /d %%%%i in (.) do echo %%%%~dpnxi>> 1set2end.cmd
echo endlocal>> 1set2end.cmd
echo echo %%WINE_var%%>> 1set2end.cmd
echo for /d %%%%i in (.) do echo %%%%~dpnxi>> 1set2end.cmd

echo --- Extra setlocal in called batch
set WINE_VAR=value1
rem -- setlocal1 == this batch, should never be used inside a called routine
setlocal
set WINE_var=value2
cd foodir2
call "%WINE_CURDIR%\2set1end.cmd"
echo Finished:
echo %WINE_VAR%
for /d %%i in (.) do echo %%~dpnxi
endlocal
echo %WINE_VAR%
for /d %%i in (.) do echo %%~dpnxi
cd /d %WINE_CURDIR%

echo --- Extra endlocal in called batch
set WINE_VAR=value1
rem -- setlocal1 == this batch, should never be used inside a called routine
setlocal
set WINE_var=value2
cd foodir2
call "%WINE_CURDIR%\1set2end.cmd"
echo Finished:
echo %WINE_VAR%
for /d %%i in (.) do echo %%~dpnxi
endlocal
echo %WINE_VAR%
for /d %%i in (.) do echo %%~dpnxi
cd /d %WINE_CURDIR%

echo --- endlocal in called function rather than batch pgm is ineffective
@echo off
set WINE_var=1
set WINE_var2=1
setlocal
set WINE_var=2
call :endlocalroutine
echo %WINE_var%
endlocal
echo %WINE_var%
goto :endlocalfinished
:endlocalroutine
echo %WINE_var%
endlocal
echo %WINE_var%
setlocal
set WINE_var2=2
endlocal
echo %WINE_var2%
endlocal
echo %WINE_var%
echo %WINE_var2%
goto :eof
:endlocalfinished
echo %WINE_var%

set WINE_var=
set WINE_var2=
cd .. & rd /q/s foobar

echo ------------ Testing Errorlevel ------------
rem WARNING: Do *not* add tests using ErrorLevel after this section
should_not_exist 2> nul > nul
echo %ErrorLevel%
rem nt 4.0 doesn't really support a way of setting errorlevel, so this is weak
rem See http://www.robvanderwoude.com/exit.php
call :setError -9999
echo %ErrorLevel%
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
rem Verify that (call ) sets errorlevel to 0
(call )
if errorlevel 1 echo errorlevel should have been 0

echo ------------ Testing GOTO ------------
if a==a goto dest1
echo FAILURE at dest 1
:dest1
echo goto with no leading space worked
if a==a goto :dest1b
echo FAILURE at dest 1b
:dest1b
echo goto with colon and no leading space worked
if b==b goto dest2
echo FAILURE at dest 2
 :dest2
echo goto with a leading space worked
if c==c goto dest3
echo FAILURE at dest 3
	:dest3
echo goto with a leading tab worked
if d==d goto dest4
echo FAILURE at dest 4
:dest4@space@
echo goto with a following space worked
if e==e goto dest5
echo FAILURE at dest 5
:dest5&& echo FAILURE
echo goto with following amphersands worked

del failure.txt >nul 2>&1
if f==f goto dest6
echo FAILURE at dest 6
:dest6>FAILURE.TXT
if exist FAILURE.TXT echo FAILURE at dest 6 as file exists
echo goto with redirections worked
del FAILURE.TXT >nul 2>&1

:: some text that is ignored | dir >cmd_output | another test
if exist cmd_output echo FAILURE at dest 6 as file exists
echo Ignoring double colons worked
del cmd_output >nul 2>&1

rem goto a label which does not exist issues an error message and
rem acts the same as goto :EOF, and ensure ::label is never matched
del testgoto.bat >nul 2>&1
echo goto :dest7 ^>nul 2^>^&1 >> testgoto.bat
echo echo FAILURE at dest 7 - Should have not found label and issued an error plus ended the batch>> testgoto.bat
echo ::dest7>> testgoto.bat
echo echo FAILURE at dest 7 - Incorrectly went to label >> testgoto.bat
call testgoto.bat
del testgoto.bat >nul 2>&1

del testgoto.bat >nul 2>&1
echo goto ::dest8 ^>nul 2^>^&1 >> testgoto.bat
echo echo FAILURE at dest 8 - Should have not found label and issued an error plus ended the batch>> testgoto.bat
echo ::dest8>> testgoto.bat
echo echo FAILURE at dest 8 - Incorrectly went to label >> testgoto.bat
call testgoto.bat
del testgoto.bat >nul 2>&1

if g==g goto dest9
echo FAILURE at dest 9
:dest91
echo FAILURE at dest 91
@   :     dest9>rubbish
echo label with mixed whitespace and no echo worked

if h==h goto :dest10:this is ignored
echo FAILURE at dest 10
:dest10:this is also ignored
echo Correctly ignored trailing information

rem Testing which label is reached when there are many options
echo Begin:
set nextlabel=
call :sub
set nextlabel=middle
goto :sub

:sub
echo ..First sub
if not "%nextlabel%"=="" goto :%nextlabel%
goto :EOF

:sub
echo ..Second sub
if not "%nextlabel%"=="" goto :%nextlabel%
goto :EOF

:middle
echo Middle:
set nextlabel=
call :sub
set nextlabel=nearend
goto :sub

:sub
echo ..Third sub
if not "%nextlabel%"=="" goto :%nextlabel%
goto :EOF

:nearend
echo Near end:
set nextlabel=
call :sub
set nextlabel=end
goto :sub

:sub
echo ..Fourth sub
if not "%nextlabel%"=="" goto :%nextlabel%
goto :EOF

:end
echo At end:
set nextlabel=
call :sub
set nextlabel=done
goto :sub

:done
echo Finished

echo ------------ Testing PATH ------------
set WINE_backup_path=%path%
set path=original
path
path try2
path
path=try3
path

echo ------------ Testing PATH Evaluate ------------
mkdir folder
echo echo I'm here! > folder\sub1.bat

echo Test normal PATH usage
set path=%cd%\folder
call sub1.bat

echo Test PATH usage with leading semicolon
set path=;%cd%\folder
call sub1.bat

echo Test PATH usage with fallback path
set path=%cd%;%cd%\folder
call sub1.bat

echo Test PATH usage with double semicolon
set path=%cd%;;%cd%\folder
call sub1.bat

del folder\sub1.bat
rmdir folder
set path=%WINE_backup_path%
set WINE_backup_path=

echo ------------ Testing start /W ------------
echo start /W failed to wait>foobar.txt
start /W "" cmd /C "ping -n1 & echo start /W seems to really wait>foobar.txt"& type foobar.txt& del foobar.txt

echo ------------ Testing changing the drive letter ----------
pushd C:\

echo Normal:
call :setError 0
C:
if errorlevel 1 echo Normal drive change failed

echo Normal+space
call :setError 0
C:@space@
if errorlevel 1 echo Normal+space drive change failed

echo Normal+space+garbage
call :setError 0
C: garbage
if errorlevel 1 echo Normal+space+garbage drive change failed

call :setError 0
echo Quoted should fail
"C:"
if not errorlevel 1 echo quoted drive change unexpectedly worked

echo Normal+tab
call :setError 0
C:@tab@
if errorlevel 1 echo Normal+tab drive change failed

echo Normal+tab+garbage
call :setError 0
C:@tab@garbagetab
if errorlevel 1 echo Normal+tab+garbage drive change failed

popd

echo ------------ Testing length wrt. MAX_PATH ------------
rem native cmd limits all path lengths to MAX_PATH=260
pushd c:\
set depth=25
for /L %%d in (0,1,25) do (
  mkdir abcdefghij > NUL 2>&1
  if exist abcdefghij (
     cd abcdefghij
     set depth=%%d
  )
)
echo %depth%
rem even relative paths are transformed to absolute, and tested against MAX_PATH
echo abc > 01234567890123
if exist 01234567890123 (echo Success) else echo Failure
echo abc > 012345678901234
if exist 012345678901234 (echo Failure) else echo Success
popd
rmdir /s /q c:\abcdefghij
echo ---- Testing nasty bits ----
echo erase /q foobar.bat > foobar.bat
echo echo shouldnot >> foobar.bat
call foobar.bat
if exist foobar.bat (echo stillthere & erase /q foobar.bat >NUL)

echo ------------ Testing updated code page execution ------------
echo @echo off>utf8.cmd
echo chcp 65001>>utf8.cmd
echo set utf8=@\xE3@@\xA1@@\xA1@@\xE3@@\xA1@@\xA1@>>utf8.cmd
echo if not %%utf8:~0,2%%==%%utf8%% exit 1 >>utf8.cmd
start /wait cmd /cutf8.cmd
if errorlevel 1 (echo Failure) else echo Success
del utf8.cmd

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
