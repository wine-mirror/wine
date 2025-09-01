echo Tests for cmd's builtin commands BAT
@echo off
setlocal EnableDelayedExpansion

rem All the success/failure tests are meant to be duplicated in test_builtins.cmd
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
rem generating different condition of child.bat termination (success/failure, errorlevel set or not...)
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
call :setError 666 & (erase *.idontexistatall &&echo SUCCESS !errorlevel!||echo FAILURE !errorlevel!)
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

goto :eof

rem Subroutine to set errorlevel and return
:setError
exit /B %1
rem This line runs under cmd in windows NT 4, but not in more modern versions.
