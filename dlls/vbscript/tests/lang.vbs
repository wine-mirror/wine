'
' Copyright 2011 Jacek Caban for CodeWeavers
'
' This library is free software; you can redistribute it and/or
' modify it under the terms of the GNU Lesser General Public
' License as published by the Free Software Foundation; either
' version 2.1 of the License, or (at your option) any later version.
'
' This library is distributed in the hope that it will be useful,
' but WITHOUT ANY WARRANTY; without even the implied warranty of
' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
' Lesser General Public License for more details.
'
' You should have received a copy of the GNU Lesser General Public
' License along with this library; if not, write to the Free Software
' Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
'

Option Explicit

dim x, y

call ok(true, "true is not true?")
ok true, "true is not true?"
call ok((true), "true is not true?")

ok not false, "not false but not true?"
ok not not true, "not not true but not true?"

Call ok(true = true, "true = true is false")
Call ok(false = false, "false = false is false")
Call ok(not (true = false), "true = false is true")
Call ok("x" = "x", """x"" = ""x"" is false")
Call ok(empty = empty, "empty = empty is false")
Call ok(empty = "", "empty = """" is false")
Call ok(0 = 0.0, "0 <> 0.0")
Call ok(16 = &h10&, "16 <> &h10&")
Call ok(010 = 10, "010 <> 10")
Call ok(10. = 10, "10. <> 10")
Call ok(&hffFFffFF& = -1, "&hffFFffFF& <> -1")
Call ok(&hffFFffFF& = -1, "&hffFFffFF& <> -1")
Call ok(--1 = 1, "--1 = " & --1)
Call ok(-empty = 0, "-empty = " & (-empty))

x = "xx"
Call ok(x = "xx", "x = " & x & " expected ""xx""")

Call ok(true <> false, "true <> false is false")
Call ok(not (true <> true), "true <> true is true")
Call ok(not ("x" <> "x"), """x"" <> ""x"" is true")
Call ok(not (empty <> empty), "empty <> empty is true")
Call ok(x <> "x", "x = ""x""")

Call ok(getVT(false) = "VT_BOOL", "getVT(false) is not VT_BOOL")
Call ok(getVT(true) = "VT_BOOL", "getVT(true) is not VT_BOOL")
Call ok(getVT("") = "VT_BSTR", "getVT("""") is not VT_BSTR")
Call ok(getVT("test") = "VT_BSTR", "getVT(""test"") is not VT_BSTR")
Call ok(getVT(Empty) = "VT_EMPTY", "getVT(Empty) is not VT_EMPTY")
Call ok(getVT(null) = "VT_NULL", "getVT(null) is not VT_NULL")
Call ok(getVT(0) = "VT_I2", "getVT(0) is not VT_I2")
Call ok(getVT(1) = "VT_I2", "getVT(1) is not VT_I2")
Call ok(getVT(0.5) = "VT_R8", "getVT(0.5) is not VT_R8")
Call ok(getVT(0.0) = "VT_R8", "getVT(0.0) is not VT_R8")
Call ok(getVT(2147483647) = "VT_I4", "getVT(2147483647) is not VT_I4")
Call ok(getVT(2147483648) = "VT_R8", "getVT(2147483648) is not VT_R8")
Call ok(getVT(&h10&) = "VT_I2", "getVT(&h10&) is not VT_I2")
Call ok(getVT(&h10000&) = "VT_I4", "getVT(&h10000&) is not VT_I4")
Call ok(getVT(&H10000&) = "VT_I4", "getVT(&H10000&) is not VT_I4")
Call ok(getVT(&hffFFffFF&) = "VT_I2", "getVT(&hffFFffFF&) is not VT_I2")
Call ok(getVT(1 & 100000) = "VT_BSTR", "getVT(1 & 100000) is not VT_BSTR")
Call ok(getVT(-empty) = "VT_I2", "getVT(-empty) = " & getVT(-empty))
Call ok(getVT(-null) = "VT_NULL", "getVT(-null) = " & getVT(-null))
Call ok(getVT(y) = "VT_EMPTY*", "getVT(y) = " & getVT(y))
x = true
Call ok(getVT(x) = "VT_BOOL*", "getVT(x) = " & getVT(x))

x = "xx"
Call ok("ab" & "cd" = "abcd", """ab"" & ""cd"" <> ""abcd""")
Call ok("ab " & null = "ab ", """ab"" & null = " & ("ab " & null))
Call ok("ab " & empty = "ab ", """ab"" & empty = " & ("ab " & empty))
Call ok(1 & 100000 = "1100000", "1 & 100000 = " & (1 & 100000))
Call ok("ab" & x = "abxx", """ab"" & x = " & ("ab"&x))

if(isEnglishLocale) then
    Call ok("" & true = "True", """"" & true = " & true)
    Call ok(true & false = "TrueFalse", "true & false = " & (true & false))
end if

x = 3
Call ok(2+2 = 4, "2+2 = " & (2+2))
Call ok(false + 6 + true = 5, "false + 6 + true <> 5")
Call ok(getVT(2+null) = "VT_NULL", "getVT(2+null) = " & getVT(2+null))
Call ok(2+empty = 2, "2+empty = " & (2+empty))
Call ok(x+x = 6, "x+x = " & (x+x))

Call ok(5-1 = 4, "5-1 = " & (5-1))
Call ok(3+5-true = 9, "3+5-true <> 9")
Call ok(getVT(2-null) = "VT_NULL", "getVT(2-null) = " & getVT(2-null))
Call ok(2-empty = 2, "2-empty = " & (2-empty))
Call ok(2-x = -1, "2-x = " & (2-x))

Call ok(9 Mod 6 = 3, "9 Mod 6 = " & (9 Mod 6))
Call ok(11.6 Mod 5.5 = False, "11.6 Mod 5.5 = " & (11.6 Mod 5.5 = 0.6))
Call ok(7 Mod 4+2 = 5, "7 Mod 4+2 <> 5")
Call ok(getVT(2 mod null) = "VT_NULL", "getVT(2 mod null) = " & getVT(2 mod null))
Call ok(getVT(null mod 2) = "VT_NULL", "getVT(null mod 2) = " & getVT(null mod 2))
'FIXME: Call ok(empty mod 2 = 0, "empty mod 2 = " & (empty mod 2))

Call ok(5 \ 2 = 2, "5 \ 2 = " & (5\2))
Call ok(4.6 \ 1.5 = 2, "4.6 \ 1.5 = " & (4.6\1.5))
Call ok(4.6 \ 1.49 = 5, "4.6 \ 1.49 = " & (4.6\1.49))
Call ok(2+3\4 = 2, "2+3\4 = " & (2+3\4))

Call ok(2*3 = 6, "2*3 = " & (2*3))
Call ok(3/2 = 1.5, "3/2 = " & (3/2))
Call ok(5\4/2 = 2, "5\4/2 = " & (5\2/1))
Call ok(12/3\2 = 2, "12/3\2 = " & (12/3\2))

Call ok(2^3 = 8, "2^3 = " & (2^3))
Call ok(2^3^2 = 64, "2^3^2 = " & (2^3^2))
Call ok(-3^2 = 9, "-3^2 = " & (-3^2))
Call ok(2*3^2 = 18, "2*3^2 = " & (2*3^2))

if false then
    ok false, "if false called"
end if

x = true
if x then
    x = false
end if
Call ok(not x, "x is false, if not evaluated?")

x = false
If false Then
   Call ok(false, "inside if false")
Else
   x = true
End If
Call ok(x, "else not called?")

x = false
If false Then
   Call ok(false, "inside if false")
ElseIf not True Then
   Call ok(false, "inside elseif not true")
Else
   x = true
End If
Call ok(x, "else not called?")

x = false
If false Then
   Call ok(false, "inside if false")
   x = 1
   y = 10+x
ElseIf not False Then
   x = true
Else
   Call ok(false, "inside else not true")
End If
Call ok(x, "elseif not called?")

x = false
If false Then
   Call ok(false, "inside if false")
ElseIf not False Then
   x = true
End If
Call ok(x, "elseif not called?")

if false then
Sub testsub
    x = true
End Sub
end if

reportSuccess()
