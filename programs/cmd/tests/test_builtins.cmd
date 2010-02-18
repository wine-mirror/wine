echo Tests for cmd's builtin commands
@echo off

echo ------------ Testing 'echo' --------------
echo word
echo 'singlequotedword'
echo "doublequotedword"
@echo at-echoed-word
echo "/?"
echo.
echo .

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
