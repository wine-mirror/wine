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
