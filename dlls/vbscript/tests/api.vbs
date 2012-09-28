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

Dim x

Class EmptyClass
End Class

Call ok(vbSunday = 1, "vbSunday = " & vbSunday)
Call ok(getVT(vbSunday) = "VT_I2", "getVT(vbSunday) = " & getVT(vbSunday))
Call ok(vbMonday = 2, "vbMonday = " & vbMonday)
Call ok(getVT(vbMonday) = "VT_I2", "getVT(vbMonday) = " & getVT(vbMonday))
Call ok(vbTuesday = 3, "vbTuesday = " & vbTuesday)
Call ok(getVT(vbTuesday) = "VT_I2", "getVT(vbTuesday) = " & getVT(vbTuesday))
Call ok(vbWednesday = 4, "vbWednesday = " & vbWednesday)
Call ok(getVT(vbWednesday) = "VT_I2", "getVT(vbWednesday) = " & getVT(vbWednesday))
Call ok(vbThursday = 5, "vbThursday = " & vbThursday)
Call ok(getVT(vbThursday) = "VT_I2", "getVT(vbThursday) = " & getVT(vbThursday))
Call ok(vbFriday = 6, "vbFriday = " & vbFriday)
Call ok(getVT(vbFriday) = "VT_I2", "getVT(vbFriday) = " & getVT(vbFriday))
Call ok(vbSaturday = 7, "vbSaturday = " & vbSaturday)
Call ok(getVT(vbSaturday) = "VT_I2", "getVT(vbSaturday) = " & getVT(vbSaturday))

Call ok(isObject(new EmptyClass), "isObject(new EmptyClass) is not true?")
Set x = new EmptyClass
Call ok(isObject(x), "isObject(x) is not true?")
Call ok(isObject(Nothing), "isObject(Nothing) is not true?")
Call ok(not isObject(true), "isObject(true) is true?")
Call ok(not isObject(4), "isObject(4) is true?")
Call ok(not isObject("x"), "isObject(""x"") is true?")
Call ok(not isObject(Null), "isObject(Null) is true?")

Call ok(not isEmpty(new EmptyClass), "isEmpty(new EmptyClass) is true?")
Set x = new EmptyClass
Call ok(not isEmpty(x), "isEmpty(x) is true?")
x = empty
Call ok(isEmpty(x), "isEmpty(x) is not true?")
Call ok(isEmpty(empty), "isEmpty(empty) is not true?")
Call ok(not isEmpty(Nothing), "isEmpty(Nothing) is not true?")
Call ok(not isEmpty(true), "isEmpty(true) is true?")
Call ok(not isEmpty(4), "isEmpty(4) is true?")
Call ok(not isEmpty("x"), "isEmpty(""x"") is true?")
Call ok(not isEmpty(Null), "isEmpty(Null) is true?")

Call ok(not isNull(new EmptyClass), "isNull(new EmptyClass) is true?")
Set x = new EmptyClass
Call ok(not isNull(x), "isNull(x) is true?")
x = null
Call ok(isNull(x), "isNull(x) is not true?")
Call ok(not isNull(empty), "isNull(empty) is true?")
Call ok(not isNull(Nothing), "isNull(Nothing) is true?")
Call ok(not isNull(true), "isNull(true) is true?")
Call ok(not isNull(4), "isNull(4) is true?")
Call ok(not isNull("x"), "isNull(""x"") is true?")
Call ok(isNull(Null), "isNull(Null) is not true?")

Call ok(getVT(err) = "VT_DISPATCH", "getVT(err) = " & getVT(err))

Sub TestHex(x, ex)
    Call ok(hex(x) = ex, "hex(" & x & ") = " & hex(x) & " expected " & ex)
End Sub

TestHex 0, "0"
TestHex 6, "6"
TestHex 16, "10"
TestHex &hdeadbeef&, "DEADBEEF"
TestHex -1, "FFFF"
TestHex -16, "FFF0"
TestHex -934859845, "C8472BBB"
TestHex empty, "0"

Call ok(getVT(hex(null)) = "VT_NULL", "getVT(hex(null)) = " & getVT(hex(null)))
Call ok(getVT(hex(empty)) = "VT_BSTR", "getVT(hex(empty)) = " & getVT(hex(empty)))

x = InStr(1, "abcd", "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr("abcd", "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr("abc", "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr("abcbc", "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr("bcabc", "bc")
Call ok(x = 1, "InStr returned " & x)

x = InStr(3, "abcd", "bc")
Call ok(x = 0, "InStr returned " & x)

x = InStr("abcd", "bcx")
Call ok(x = 0, "InStr returned " & x)

x = InStr(5, "abcd", "bc")
Call ok(x = 0, "InStr returned " & x)

x = "abcd"
x = InStr(x, "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr("abcd", null)
Call ok(isNull(x), "InStr returned " & x)
x = InStr(null, "abcd")
Call ok(isNull(x), "InStr returned " & x)
x = InStr(2, null, "abcd")
Call ok(isNull(x), "InStr returned " & x)

x = InStr(1.3, "abcd", "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr(2.3, "abcd", "bc")
Call ok(x = 2, "InStr returned " & x)

x = InStr(2.6, "abcd", "bc")
Call ok(x = 0, "InStr returned " & x)

Sub TestMid(str, start, len, ex)
    x = Mid(str, start, len)
    Call ok(x = ex, "Mid(" & str & ", " & start & ", " & len & ") = " & x & " expected " & ex)
End Sub

Sub TestMid2(str, start, ex)
    x = Mid(str, start)
    Call ok(x = ex, "Mid(" & str & ", " & start & ") = " & x & " expected " & ex)
End Sub

TestMid "test", 2, 2, "es"
TestMid "test", 2, 4, "est"
TestMid "test", 1, 2, "te"
TestMid "test", 1, 0, ""
TestMid "test", 1, 0, ""
TestMid "test", 5, 2, ""
TestMid2 "test", 1, "test"
TestMid2 "test", 2, "est"
TestMid2 "test", 4, "t"
TestMid2 "test", 5, ""

Sub TestUCase(str, ex)
    x = UCase(str)
    Call ok(x = ex, "UCase(" & str & ") = " & x & " expected " & ex)
End Sub

TestUCase "test", "TEST"
TestUCase "123aBC?", "123ABC?"
TestUCase "", ""
TestUCase 1, "1"
if isEnglishLang then TestUCase true, "TRUE"
TestUCase 0.123, doubleAsString(0.123)
TestUCase Empty, ""
Call ok(getVT(UCase(Null)) = "VT_NULL", "getVT(UCase(Null)) = " & getVT(UCase(Null)))

Sub TestLCase(str, ex)
    x = LCase(str)
    Call ok(x = ex, "LCase(" & str & ") = " & x & " expected " & ex)
End Sub

TestLCase "test", "test"
TestLCase "123aBC?", "123abc?"
TestLCase "", ""
TestLCase 1, "1"
if isEnglishLang then TestLCase true, "true"
TestLCase 0.123, doubleAsString(0.123)
TestLCase Empty, ""
Call ok(getVT(LCase(Null)) = "VT_NULL", "getVT(LCase(Null)) = " & getVT(LCase(Null)))

Call ok(Len("abc") = 3, "Len(abc) = " & Len("abc"))
Call ok(Len("") = 0, "Len() = " & Len(""))
Call ok(Len(1) = 1, "Len(1) = " & Len(1))
Call ok(isNull(Len(null)), "Len(null) = " & Len(null))
Call ok(Len(empty) = 0, "Len(empty) = " & Len(empty))

Call ok(Space(1) = " ", "Space(1) = " & Space(1) & """")
Call ok(Space(0) = "", "Space(0) = " & Space(0) & """")
Call ok(Space(false) = "", "Space(false) = " & Space(false) & """")
Call ok(Space(5) = "     ", "Space(5) = " & Space(5) & """")
Call ok(Space(5.2) = "     ", "Space(5.2) = " & Space(5.2) & """")
Call ok(Space(5.8) = "      ", "Space(5.8) = " & Space(5.8) & """")
Call ok(Space(5.5) = "      ", "Space(5.5) = " & Space(5.5) & """")

Sub TestStrReverse(str, ex)
    Call ok(StrReverse(str) = ex, "StrReverse(" & str & ") = " & StrReverse(str))
End Sub

TestStrReverse "test", "tset"
TestStrReverse "", ""
TestStrReverse 123, "321"
if isEnglishLang then TestStrReverse true, "eurT"

Sub TestRound(val, exval, vt)
    Call ok(Round(val) = exval, "Round(" & val & ") = " & Round(val))
    Call ok(getVT(Round(val)) = vt, "getVT(Round(" & val & ")) = " & getVT(Round(val)))
End Sub

TestRound 3, 3, "VT_I2"
TestRound 3.3, 3, "VT_R8"
TestRound 3.8, 4, "VT_R8"
TestRound 3.5, 4, "VT_R8"
TestRound -3.3, -3, "VT_R8"
TestRound -3.5, -4, "VT_R8"
TestRound "2", 2, "VT_R8"
TestRound true, true, "VT_BOOL"
TestRound false, false, "VT_BOOL"

if isEnglishLang then
    Call ok(WeekDayName(1) = "Sunday", "WeekDayName(1) = " & WeekDayName(1))
    Call ok(WeekDayName(3) = "Tuesday", "WeekDayName(3) = " & WeekDayName(3))
    Call ok(WeekDayName(7) = "Saturday", "WeekDayName(7) = " & WeekDayName(7))
    Call ok(WeekDayName(1.1) = "Sunday", "WeekDayName(1.1) = " & WeekDayName(1.1))
    Call ok(WeekDayName(1, false) = "Sunday", "WeekDayName(1, false) = " & WeekDayName(1, false))
    Call ok(WeekDayName(1, true) = "Sun", "WeekDayName(1, true) = " & WeekDayName(1, true))
    Call ok(WeekDayName(1, 10) = "Sun", "WeekDayName(1, 10) = " & WeekDayName(1, 10))
    Call ok(WeekDayName(1, true, 0) = "Sun", "WeekDayName(1, true, 0) = " & WeekDayName(1, true, 0))
    Call ok(WeekDayName(1, true, 2) = "Mon", "WeekDayName(1, true, 2) = " & WeekDayName(1, true, 2))
    Call ok(WeekDayName(1, true, 7) = "Sat", "WeekDayName(1, true, 7) = " & WeekDayName(1, true, 7))
    Call ok(WeekDayName(1, true, 7.1) = "Sat", "WeekDayName(1, true, 7.1) = " & WeekDayName(1, true, 7.1))

    Call ok(MonthName(1) = "January", "MonthName(1) = " & MonthName(1))
    Call ok(MonthName(12) = "December", "MonthName(12) = " & MonthName(12))
    Call ok(MonthName(1, 0) = "January", "MonthName(1, 0) = " & MonthName(1, 0))
    Call ok(MonthName(12, false) = "December", "MonthName(12, false) = " & MonthName(12, false))
    Call ok(MonthName(1, 10) = "Jan", "MonthName(1, 10) = " & MonthName(1, 10))
    Call ok(MonthName(12, true) = "Dec", "MonthName(12, true) = " & MonthName(12, true))
end if

Call ok(getVT(Now()) = "VT_DATE", "getVT(Now()) = " & getVT(Now()))

Call reportSuccess()
