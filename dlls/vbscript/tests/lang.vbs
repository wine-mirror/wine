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

OPTION EXPLICIT  : : DIM W

dim x, y, z, e, hi
Dim obj

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
Call ok(34e5 = 3400000, "34e5 <> 3400000")
Call ok(34e+5 = 3400000, "34e+5 <> 3400000")
Call ok(56.789e5 = 5678900, "56.789e5 = 5678900")
Call ok(56.789e-2 = 0.56789, "56.789e-2 <> 0.56789")
Call ok(1e-94938484 = 0, "1e-... <> 0")
Call ok(34e0 = 34, "34e0 <> 34")
Call ok(34E1 = 340, "34E0 <> 340")
Call ok(.5 = 0.5, ".5 <> 0.5")
Call ok(.5e1 = 5, ".5e1 <> 5")
Call ok(--1 = 1, "--1 = " & --1)
Call ok(-empty = 0, "-empty = " & (-empty))
Call ok(true = -1, "! true = -1")
Call ok(false = 0, "false <> 0")
Call ok(&hff = 255, "&hff <> 255")
Call ok(&Hff = 255, "&Hff <> 255")
Call ok(&hffff = -1, "&hffff <> -1")
Call ok(&hfffe = -2, "&hfffe <> -2")
Call ok(&hffff& = 65535, "&hffff& <> -1")
Call ok(&hfffe& = 65534, "&hfffe& <> -2")
Call ok(&hffffffff& = -1, "&hffffffff& <> -1")
Call ok((&h01or&h02)=3,"&h01or&h02 <> 3")

' Hex literals with excess leading zeros
Call ok(&H000000031 = 49, "&H000000031 <> 49")
Call ok(&H0000000000000031 = 49, "&H0000000000000031 <> 49")
Call ok(&H00000000000000FF = 255, "&H00000000000000FF <> 255")
Call ok(&H0FFFFFFFF = -1, "&H0FFFFFFFF <> -1")
Call ok(&H007FFFFFFF = 2147483647, "&H007FFFFFFF <> 2147483647")
Call ok(&H000000031& = 49, "&H000000031& <> 49")
Call ok(getVT(&H00000000000000FF) = "VT_I2", "getVT(&H00000000000000FF) is not VT_I2")
Call ok(getVT(&H007FFFFFFF) = "VT_I4", "getVT(&H007FFFFFFF) is not VT_I4")
Call ok(&h0 = 0, "&h0 <> 0")
Call ok(&h0& = 0, "&h0& <> 0")
Call ok(&h00 = 0, "&h00 <> 0")
Call ok(&h000000000 = 0, "&h000000000 <> 0")

' Test concat when no space and var begins with h
hi = "y"
x = "x" &hi
Call ok(x = "xy", "x = " & x & " expected ""xy""")

W = 5
Call ok(W = 5, "W = " & W & " expected " & 5)

x = "xx"
Call ok(x = "xx", "x = " & x & " expected ""xx""")

Dim public1 : public1 = 42
Call ok(public1 = 42, "public1=" & public1 & " expected & " & 42)
Private priv1 : priv1 = 43
Call ok(priv1 = 43, "priv1=" & priv1 & " expected & " & 43)
Public pub1 : pub1 = 44
Call ok(pub1 = 44, "pub1=" & pub1 & " expected & " & 44)

Call ok(true <> false, "true <> false is false")
Call ok(not (true <> true), "true <> true is true")
Call ok(not ("x" <> "x"), """x"" <> ""x"" is true")
Call ok(not (empty <> empty), "empty <> empty is true")
Call ok(x <> "x", "x = ""x""")
Call ok("true" <> true, """true"" = true is true")

Call ok("" = true = false, """"" = true = false is false")
Call ok(not(false = true = ""), "false = true = """" is true")
Call ok(not (false = false <> false = false), "false = false <> false = false is true")
Call ok(not ("" <> false = false), """"" <> false = false is true")

Call ok(true <> Not true, "true <> Not true should be true")
Call ok(false <> Not false, "false <> Not false should be true")
Call ok(true = Not false, "true = Not false should be true")
Call ok(Not false = true, "Not false = true should be true")
Call ok(Not true <> true, "Not true <> true should be true")
Call ok(Not true = false, "Not true = false should be true")
Call ok(Not 1 > 2, "Not 1 > 2 should be true")
Call ok(1 <> Not 0 = 0, "1 <> Not 0 = 0 should be true")
Call ok(0 = Not 1 = 1, "0 = Not 1 = 1 should be true")
Call ok(1 > Not 5 > 3, "1 > Not 5 > 3 should be true")
Call ok(Not false And false = false, "Not false And false should be false")
x = "0"
Call ok(not (x > 30), """0"" > 30 should be false")
Call ok(x < 30, """0"" < 30 should be true")
Call ok(not ("0" > 30), """0"" > 30 literal should be false")
Call ok("0" < 30, """0"" < 30 literal should be true")
Call ok("10" > 9, """10"" > 9 should be true")
Call ok(not ("10" > 100), """10"" > 100 should be false")
Call ok("10" < 100, """10"" < 100 should be true")
Call ok("42" = 42, """42"" = 42 should be true")
Call ok(not ("42" = 43), """42"" = 43 should be false")
Call ok(doubleAsString(0.5) > 0, """0.5"" > 0 should be true")
Call ok(doubleAsString(0.5) < 1, """0.5"" < 1 should be true")
Call ok(doubleAsString(3.5) = 3.5, """3.5"" = 3.5 should be true")
Call ok(not (doubleAsString(1.5) > 2), """1.5"" > 2 should be false")
Call ok(doubleAsString(2.5) > 2, """2.5"" > 2 should be true")
Call ok(30 > "0", "30 > ""0"" should be true")
Call ok(9 < "10", "9 < ""10"" should be true")
Call ok(42 = "42", "42 = ""42"" should be true")
Call ok(not ("10" > "9"), """10"" > ""9"" should be false (string comparison)")
' Locale-invariant numeric strings: parse to the same value as the literal in any locale
Call ok("+5" = 5, """+5"" = 5 should be true")
Call ok("-0" = 0, """-0"" = 0 should be true")
Call ok("05" = 5, """05"" = 5 should be true")
Call ok(" 5" = 5, """ 5"" = 5 should be true")
Call ok("5 " = 5, """5 "" = 5 should be true")
Call ok("5e0" = 5, """5e0"" = 5 should be true")
' BSTR vs Boolean: string-compare against CStr(bool) ("True"/"False"),
' case-sensitive, no whitespace trimming, no numeric coercion, no type-mismatch.
Call ok("True" = True, """True"" = True should be true")
Call ok("False" = False, """False"" = False should be true")
Call ok(not ("True" = False), """True"" = False should be false")
Call ok(not ("False" = True), """False"" = True should be false")
Call ok(not ("true" = True), """true"" = True should be false (case-sensitive)")
Call ok(not ("TRUE" = True), """TRUE"" = True should be false (case-sensitive)")
Call ok(not ("True " = True), """True "" = True should be false (no trim)")
Call ok(not (" True" = True), """ True"" = True should be false (no trim)")
Call ok(not ("1" = True), """1"" = True should be false")
Call ok(not ("-1" = True), """-1"" = True should be false")
Call ok(not ("0" = False), """0"" = False should be false")
Call ok(not ("-1" = False), """-1"" = False should be false")
Call ok(not ("True" <> True), """True"" <> True should be false")
Call ok("False" <> True, """False"" <> True should be true")
Call ok(not ("abc" = True), """abc"" = True should be false (no error)")
Call ok(not ("" = True), """"" = True should be false (no error)")
Call ok(not ("" = False), """"" = False should be false (no error)")
Call ok(True = "True", "True = ""True"" should be true")
Call ok(False = "False", "False = ""False"" should be true")
' Relational: lexicographic string comparison after coercing bool to BSTR
Call ok("True" > False, """True"" > False should be true (lex)")
Call ok(not ("True" < False), """True"" < False should be false (lex)")
Call ok(not ("False" > True), """False"" > True should be false (lex)")
Call ok("abc" > True, """abc"" > True should be true (lex)")
' Non-numeric BSTR compared to number raises type mismatch (= / <> / relational)
on error resume next
err.clear
x = ("abc" = 5)
Call ok(err.number = 13, """abc"" = 5 err.number = " & err.number)
err.clear
x = ("" = 5)
Call ok(err.number = 13, """"" = 5 err.number = " & err.number)
err.clear
x = (" " = 0)
Call ok(err.number = 13, """ "" = 0 err.number = " & err.number)
err.clear
x = ("abc" <> 5)
Call ok(err.number = 13, """abc"" <> 5 err.number = " & err.number)
err.clear
x = ("" <> 5)
Call ok(err.number = 13, """"" <> 5 err.number = " & err.number)
err.clear
x = ("abc" > 5)
Call ok(err.number = 13, """abc"" > 5 err.number = " & err.number)
err.clear
x = ("" > 0)
Call ok(err.number = 13, """"" > 0 err.number = " & err.number)
err.clear
x = (5 > "abc")
Call ok(err.number = 13, "5 > ""abc"" err.number = " & err.number)
on error goto 0

' BSTR coerces to numeric for comparison even when it carries VT_BYREF
' (e.g. ByRef parameter holding a string).
Sub TestByRefStrEq5(ByRef x)
    Call ok(x = 5,  "ByRef ""5"" = 5 should be true")
    Call ok(x < 6,  "ByRef ""5"" < 6 should be true")
End Sub
TestByRefStrEq5 "5"

' BSTR vs each numeric VT VBScript can produce.
Call ok("5" = CByte(5), """5"" = CByte(5) should be true")
Call ok("5" = CInt(5),  """5"" = CInt(5) should be true")
Call ok("5" = CLng(5),  """5"" = CLng(5) should be true")
Call ok("5" = CSng(5),  """5"" = CSng(5) should be true")
Call ok("5" = CDbl(5),  """5"" = CDbl(5) should be true")
Call ok("5" = CCur(5),  """5"" = CCur(5) should be true")
Call ok(CByte(5) = "5", "CByte(5) = ""5"" should be true")
Call ok(CCur(5)  = "5", "CCur(5) = ""5"" should be true")

' Hex / scientific BSTRs parse as numeric.
Call ok("1e2" = 100, """1e2"" = 100 should be true")
Call ok("&hff" = 255, """&hff"" = 255 should be true")
Call ok("&H1F" = 31,  """&H1F"" = 31 should be true")

' VT_UI1/VT_CY vs BSTR diverge from VT_I2: string-compare against CStr(numeric).
' Non-numeric BSTR returns False with NO error (VT_I2 raises 13 for the same).
Call ok(not ("abc" = CByte(5)), """abc"" = CByte(5) should be false (no error)")
Call ok(not ("abc" = CCur(5)),  """abc"" = CCur(5) should be false (no error)")
Call ok(not ("" = CByte(0)),    """"" = CByte(0) should be false (no error)")
Call ok(not ("" = CCur(0)),     """"" = CCur(0) should be false (no error)")
' Relational confirms lex compare: 10 > 5 numerically would be true; lex "10" < "5".
Call ok(not ("10" > CByte(5)),  """10"" > CByte(5) should be false (lex)")
Call ok(not ("5" < CCur(10)),   """5"" < CCur(10) should be false (lex)")

' VT_I8/UI8/I1/UI2/UI4/UINT cannot be produced by VBScript natively; obtain
' them via a host IDispatch (testobj). Native VBScript:
'  - VT_UI8/UI2/UI4/UINT: error 458 (VBSE_INVALID_TYPELIB_VARIABLE) on
'    both 32-bit and 64-bit when used in any expression.
'  - VT_I8: error 458 on 32-bit; on 64-bit no error but BSTR-vs-I8 compares
'    as not-equal (no CStr coercion of the numeric side).
'  - VT_I1: no error on either arch (treated as a small integer); BSTR
'    literal vs I1 compares equal via numeric coercion, matching the
'    baseline "5" = CInt(5) behavior.
Call ok("5" = testobj.i1val, """5"" = testobj.i1val should be true")

Dim x_str : x_str = "5"
Dim cmp_result
On Error Resume Next

' VT_I8 vs non-literal BSTR. 32-bit: err 458. 64-bit: cmp = False (default
' VarCmp returns BSTR > other since I8 isn't in is_numeric_vt).
Err.Clear : cmp_result = (x_str = testobj.i8val) : saved_err = Err.number : Err.Clear
Call ok(saved_err = 458 or cmp_result = False, _
    "x_str = testobj.i8val: err=" & saved_err & " result=" & cmp_result)

' VT_UI8/UI2/UI4/UINT: native errors 458 on both archs. Wine's VarCmp
' rejects VT_UI8 and VT_UINT via DISP_E_BADVARTYPE (mapped to 458)
' without a script-level gate; UI2/UI4 still go through DISP_E_TYPEMISMATCH
' (mapped to 13) and need the gate, so they remain todo_wine for now.
Err.Clear : cmp_result = ("5" = testobj.ui8val) : saved_err = Err.number : Err.Clear
Call ok(saved_err = 458, _
    "BSTR = testobj.ui8val should err 458, got " & saved_err)

Err.Clear : cmp_result = ("5" = testobj.ui2val) : saved_err = Err.number : Err.Clear
Call ok(saved_err = 458, _
    "BSTR = testobj.ui2val should err 458, got " & saved_err)

Err.Clear : cmp_result = ("5" = testobj.ui4val) : saved_err = Err.number : Err.Clear
Call ok(saved_err = 458, _
    "BSTR = testobj.ui4val should err 458, got " & saved_err)

Err.Clear : cmp_result = ("5" = testobj.uintval) : saved_err = Err.number : Err.Clear
Call ok(saved_err = 458, _
    "BSTR = testobj.uintval should err 458, got " & saved_err)

On Error Goto 0

' --- BSTR vs numeric LITERAL: BSTR coerces to a number, parse failure raises 13. ---
Dim saved_err
on error resume next
err.clear
x = ("abc" = 5.5)        : saved_err = err.number : err.clear
Call ok(saved_err = 13, "literal R8: ""abc"" = 5.5 should error 13 (got " & saved_err & "; needs literal tracking)")
x = ("abc" = 1e2)        : saved_err = err.number : err.clear
Call ok(saved_err = 13, "literal sci: ""abc"" = 1e2 should error 13 (got " & saved_err & "; needs literal tracking)")
x = ("abc" = &hff)       : saved_err = err.number : err.clear
Call ok(saved_err = 13, "literal hex: ""abc"" = &hff should error 13 (got " & saved_err & ")")
x = ("abc" = 100000)     : saved_err = err.number : err.clear
Call ok(saved_err = 13, "literal I4: ""abc"" = 100000 should error 13 (got " & saved_err & ")")
x = ("abc" = #1/15/2024#): saved_err = err.number : err.clear
Call ok(saved_err = 13, "literal date: ""abc"" = #1/15/2024# should error 13 (got " & saved_err & "; needs literal tracking)")
on error goto 0

' --- Variable holding a numeric / arithmetic / unary / function return ---
' VBScript on Windows treats these as non-literal: BSTR vs numeric uses
' string-compare, non-numeric BSTR returns False with no error.
Dim n5  : n5  = 5
Dim n10 : n10 = 10
Call ok(not ("abc" = n5),       "var: ""abc"" = (n=5) should be false")
Call ok(not ("010" = n10),      "var: ""010"" = (n=10) should be false (string)")
Call ok(not ("abc" = (5+0)),    "arith: ""abc"" = (5+0) should be false")
Call ok(not ("010" = (5+5)),    "arith: ""010"" = (5+5) should be false")
Call ok(not ("010" = (10*1)),   "arith: ""010"" = (10*1) should be false")
Call ok(not ("abc" = -5),       "neg: ""abc"" = -5 should be false")

' --- C-coercion functions return non-literal values; BSTR vs numeric
' uses string-compare regardless of the underlying VT. ---
Call ok(not ("010" = CInt(10)), "CInt: ""010"" = CInt(10) should be false")
Call ok(not ("010" = CLng(10)), "CLng: ""010"" = CLng(10) should be false")
Call ok(not ("010" = CSng(10)), "CSng: ""010"" = CSng(10) should be false")
Call ok(not ("010" = CDbl(10)), "CDbl: ""010"" = CDbl(10) should be false")
Call ok(not ("abc" = CInt(5)),  "CInt: ""abc"" = CInt(5) should be false")
Call ok(not ("abc" = CLng(5)),  "CLng: ""abc"" = CLng(5) should be false")
Call ok(not ("abc" = CSng(5)),  "CSng: ""abc"" = CSng(5) should be false")
Call ok(not ("abc" = CDbl(5)),  "CDbl: ""abc"" = CDbl(5) should be false")

' VT_R4 / VT_R8 from CSng/CDbl: relational uses lex compare.
Call ok(not ("10" > CDbl(5)), """10"" > CDbl(5) should be false (lex)")
Call ok(not ("10" > CSng(5)), """10"" > CSng(5) should be false (lex)")
Call ok(not ("9" < CDbl(10)), """9"" < CDbl(10) should be false (lex)")

Dim ws_space : ws_space = Space(3)
Dim ws_tab   : ws_tab   = Chr(9)
Dim ws_lf    : ws_lf    = Chr(10)
Dim ws_cr    : ws_cr    = Chr(13)
Dim ws_nbsp  : ws_nbsp  = Chr(160)
Call ok(not (Len("ab") > ws_space), "Len(""ab"") > Space(3) should be false")
Call ok((Len("ab") < ws_space),     "Len(""ab"") < Space(3) should be true")
Call ok(not (Len("ab") = ws_space),           "Len(""ab"") = Space(3) should be false")
Call ok(not (Len("ab") > ws_tab),   "Len(""ab"") > Chr(9) should be false")
Call ok((Len("ab") < ws_tab),       "Len(""ab"") < Chr(9) should be true")
Call ok(not (Len("ab") > ws_lf),    "Len(""ab"") > Chr(10) should be false")
Call ok((Len("ab") < ws_lf),        "Len(""ab"") < Chr(10) should be true")
Call ok(not (Len("ab") > ws_cr),    "Len(""ab"") > Chr(13) should be false")
Call ok((Len("ab") < ws_cr),        "Len(""ab"") < Chr(13) should be true")
Call ok(not (Len("ab") > ws_nbsp),  "Len(""ab"") > Chr(160) should be false")
Call ok((Len("ab") < ws_nbsp),      "Len(""ab"") < Chr(160) should be true")

Dim ctl_nul  : ctl_nul  = Chr(0)
Dim ctl_soh  : ctl_soh  = Chr(1)
Dim ctl_us   : ctl_us   = Chr(31)
Call ok((Len("ab") < ctl_nul), "Len(""ab"") < Chr(0) should be true")
Call ok((Len("ab") < ctl_soh), "Len(""ab"") < Chr(1) should be true")
Call ok((Len("ab") < ctl_us),  "Len(""ab"") < Chr(31) should be true")
Call ok((Len("ab") < (ctl_nul & "5")), "Len(""ab"") < Chr(0)&""5"" should be true")
Call ok((Len("ab") < (ctl_nul & " ")), "Len(""ab"") < Chr(0)&"" "" should be true")
Call ok((Len("ab") < (" " & ctl_nul)), "Len(""ab"") < "" ""&Chr(0) should be true")

Call ok((Len("ab") > ""),                     "Len(""ab"") > """" should be true")

Dim guard_str : guard_str = "ab"
Dim guard_err, guard_r
On Error Resume Next
Err.Clear
If Len(guard_str) > Space(3) Then
    guard_r = Left(guard_str, Space(3))
End If
guard_err = Err.number
Err.Clear
On Error Goto 0
Call ok(guard_err = 0, "Len(""ab"") > Space(3) guard should not raise (got " & guard_err & ")")

' --- VT_DATE from CDate is non-literal: string compare, no error. ---
Dim cdt : cdt = CDate("2024-01-15")
Call ok(not ("abc" = cdt), "CDate: ""abc"" = CDate(...) should be false")

' --- Function return / ByVal / ByRef strip "literal" status. ---
Function GetFiveLit()
    GetFiveLit = 5
End Function
Call ok(not ("abc" = GetFiveLit()), "fn return: ""abc"" = GetFiveLit() should be false")

Sub TestByValStripsLit(ByVal v)
    Call ok(not ("abc" = v), "ByVal: ""abc"" = v should be false")
End Sub
TestByValStripsLit 5

Sub TestByRefStripsLit(ByRef v)
    Call ok(not ("abc" = v), "ByRef: ""abc"" = v should be false")
End Sub
Dim litvar : litvar = 5
TestByRefStripsLit litvar

' Const references compile to an EXPR_MEMBER node so the comparison is
' tagged non-literal even though the value is inlined; "abc" = FIVE goes
' through the BSTR-vs-numeric string-compare path and returns False with
' no error.
Const FIVE_C = 5
Call ok(not ("abc" = FIVE_C), "Const: ""abc"" = FIVE_C should be false")

Call ok(getVT(false) = "VT_BOOL", "getVT(false) is not VT_BOOL")
Call ok(getVT(true) = "VT_BOOL", "getVT(true) is not VT_BOOL")
Call ok(getVT("") = "VT_BSTR", "getVT("""") is not VT_BSTR")
Call ok(getVT("test") = "VT_BSTR", "getVT(""test"") is not VT_BSTR")
Call ok(getVT(Empty) = "VT_EMPTY", "getVT(Empty) is not VT_EMPTY")
Call ok(getVT(null) = "VT_NULL", "getVT(null) is not VT_NULL")
Call ok(getVT(0) = "VT_I2", "getVT(0) is not VT_I2")
Call ok(getVT(1) = "VT_I2", "getVT(1) is not VT_I2")
Call ok(getVT(0.5) = "VT_R8", "getVT(0.5) is not VT_R8")
Call ok(getVT(.5) = "VT_R8", "getVT(.5) is not VT_R8")
Call ok(getVT(0.0) = "VT_R8", "getVT(0.0) is not VT_R8")
Call ok(getVT(2147483647) = "VT_I4", "getVT(2147483647) is not VT_I4")
Call ok(getVT(2147483648) = "VT_R8", "getVT(2147483648) is not VT_R8")
Call ok(getVT(&h10&) = "VT_I2", "getVT(&h10&) is not VT_I2")
Call ok(getVT(&h10000&) = "VT_I4", "getVT(&h10000&) is not VT_I4")
Call ok(getVT(&H10000&) = "VT_I4", "getVT(&H10000&) is not VT_I4")
Call ok(getVT(&hffFFffFF&) = "VT_I2", "getVT(&hffFFffFF&) is not VT_I2")
Call ok(getVT(&hffFFffFE&) = "VT_I2", "getVT(&hffFFffFE &) is not VT_I2")
Call ok(getVT(&hffF&) = "VT_I2", "getVT(&hffFF&) is not VT_I2")
Call ok(getVT(&hffFF&) = "VT_I4", "getVT(&hffFF&) is not VT_I4")
Call ok(getVT(# 1/1/2011 #) = "VT_DATE", "getVT(# 1/1/2011 #) is not VT_DATE")
Call ok(getVT(1e2) = "VT_R8", "getVT(1e2) is not VT_R8")
Call ok(getVT(1e0) = "VT_R8", "getVT(1e0) is not VT_R8")
Call ok(getVT(0.1e2) = "VT_R8", "getVT(0.1e2) is not VT_R8")
Call ok(getVT(1 & 100000) = "VT_BSTR", "getVT(1 & 100000) is not VT_BSTR")
Call ok(getVT(-empty) = "VT_I2", "getVT(-empty) = " & getVT(-empty))
Call ok(getVT(-null) = "VT_NULL", "getVT(-null) = " & getVT(-null))
Call ok(getVT(y) = "VT_EMPTY*", "getVT(y) = " & getVT(y))
Call ok(getVT(nothing) = "VT_DISPATCH", "getVT(nothing) = " & getVT(nothing))
set x = nothing
Call ok(getVT(x) = "VT_DISPATCH*", "getVT(x=nothing) = " & getVT(x))
x = true
Call ok(getVT(x) = "VT_BOOL*", "getVT(x) = " & getVT(x))
Call ok(getVT(false or true) = "VT_BOOL", "getVT(false) is not VT_BOOL")
x = "x"
Call ok(getVT(x) = "VT_BSTR*", "getVT(x) is not VT_BSTR*")
x = 0.0
Call ok(getVT(x) = "VT_R8*", "getVT(x) = " & getVT(x))

Call ok(isNullDisp(nothing), "nothing is not nulldisp?")

x = "xx"
Call ok("ab" & "cd" = "abcd", """ab"" & ""cd"" <> ""abcd""")
Call ok("ab " & null = "ab ", """ab"" & null = " & ("ab " & null))
Call ok("ab " & empty = "ab ", """ab"" & empty = " & ("ab " & empty))
Call ok(1 & 100000 = "1100000", "1 & 100000 = " & (1 & 100000))
Call ok("ab" & x = "abxx", """ab"" & x = " & ("ab"&x))

if(isEnglishLang) then
    Call ok("" & true = "True", """"" & true = " & true)
    Call ok(true & false = "TrueFalse", "true & false = " & (true & false))
end if

call ok(true and true, "true and true is not true")
call ok(true and not false, "true and not false is not true")
call ok(not (false and true), "not (false and true) is not true")
call ok(getVT(null and true) = "VT_NULL", "getVT(null and true) = " & getVT(null and true))

call ok(false or true, "false or uie is false?")
call ok(not (false or false), "false or false is not false?")
call ok(false and false or true, "false and false or true is false?")
call ok(true or false and false, "true or false and false is false?")
call ok(null or true, "null or true is false")

call ok(true xor false, "true xor false is false?")
call ok(not (false xor false), "false xor false is true?")
call ok(not (true or false xor true), "true or false xor true is true?")
call ok(not (true xor false or true), "true xor false or true is true?")

call ok(false eqv false, "false does not equal false?")
call ok(not (false eqv true), "false equals true?")
call ok(getVT(false eqv null) = "VT_NULL", "getVT(false eqv null) = " & getVT(false eqv null))

call ok(true imp true, "true does not imp true?")
call ok(false imp false, "false does not imp false?")
call ok(not (true imp false), "true imp false?")
call ok(false imp null, "false imp null is false?")

' Smoke check that VBScript's `And` operator reaches VarAnd correctly and
' propagates the result payload. The full VarAnd+Null conformance table
' lives in dlls/oleaut32/tests/vartest.c.
Call ok((False And Null) = False,            "False And Null is not False")
Call ok(isNull(True And Null),               "True And Null is not Null")
Call ok(isNull(Null And Null),               "Null And Null is not Null")
Call ok((CInt(0) And Null) = 0,              "CInt(0) And Null is not 0")
Call ok(getVT(CInt(0) And Null) = "VT_I2",   "getVT(CInt(0) And Null) = " & getVT(CInt(0) And Null))
Call ok(isNull(CInt(5) And Null),            "CInt(5) And Null is not Null")

' Smoke checks that VBScript's sibling logical operators reach Var*
' correctly and propagate the result payload. The full conformance tables
' live in dlls/oleaut32/tests/vartest.c.
Call ok((True Or Null) = True,               "True Or Null is not True")
Call ok(isNull(False Or Null),               "False Or Null is not Null")
Call ok(isNull(CInt(5) Xor Null),            "CInt(5) Xor Null is not Null")
Call ok(isNull(CInt(5) Eqv Null),            "CInt(5) Eqv Null is not Null")
Call ok(isNull(CDate(-1) Imp Null),          "CDate(-1) Imp Null is not Null")
Call ok((Not CLng(0)) = -1,                  "Not CLng(0) is not -1")

' VBScript-specific: for VT_UI1 Imp VT_NULL, native VBScript keeps UI1
' width and returns the bitwise complement of the left operand, rather
' than applying VarImp's three-valued "all-ones Imp unknown = unknown"
' rule (which returns VT_NULL at the C level for UI1 0xFF). interp_imp
' has a narrow special case to match this native behavior.
Call ok((CByte(0) Imp Null) = 255,           "CByte(0) Imp Null is not 255")
Call ok(getVT(CByte(0) Imp Null) = "VT_UI1", "getVT(CByte(0) Imp Null) = " & getVT(CByte(0) Imp Null))
Call ok((CByte(170) Imp Null) = 85,          "CByte(170) Imp Null is not 85")
Call ok((CByte(255) Imp Null) = 0,           "CByte(255) Imp Null is not 0")
Call ok(getVT(CByte(255) Imp Null) = "VT_UI1",    "getVT(CByte(255) Imp Null) = " & getVT(CByte(255) Imp Null))

' Empty at rest keeps its type tag. Variable references come through with
' VT_BYREF|VT_VARIANT, which getVT renders as "VT_EMPTY*".
Call ok(getVT(Empty) = "VT_EMPTY",           "getVT(Empty) = " & getVT(Empty))
Dim emp : emp = Empty
Call ok(getVT(emp) = "VT_EMPTY*",            "getVT(emp) = " & getVT(emp))

' Binary logical / bitwise ops coerce Empty to VT_I4 0 before evaluation, so
' the operator's normal widening rules see a VT_I4 left/right operand.
Call ok(getVT(Empty And Empty) = "VT_I4",    "getVT(Empty And Empty) = " & getVT(Empty And Empty))
Call ok((Empty And Empty) = 0,               "Empty And Empty is not 0")
Call ok(getVT(Empty And CInt(0)) = "VT_I4",  "getVT(Empty And CInt(0)) = " & getVT(Empty And CInt(0)))
Call ok(getVT(Empty And CInt(5)) = "VT_I4",  "getVT(Empty And CInt(5)) = " & getVT(Empty And CInt(5)))
Call ok(getVT(Empty And False) = "VT_I4",    "getVT(Empty And False) = " & getVT(Empty And False))
Call ok(getVT(Empty And Null) = "VT_I4",     "getVT(Empty And Null) = " & getVT(Empty And Null))

Call ok(getVT(Empty Or Empty) = "VT_I4",     "getVT(Empty Or Empty) = " & getVT(Empty Or Empty))
Call ok((Empty Or CInt(5)) = 5,              "Empty Or CInt(5) is not 5")
Call ok(getVT(Empty Or CInt(5)) = "VT_I4",   "getVT(Empty Or CInt(5)) = " & getVT(Empty Or CInt(5)))
Call ok(getVT(Empty Or False) = "VT_I4",     "getVT(Empty Or False) = " & getVT(Empty Or False))
Call ok(isNull(Empty Or Null),               "Empty Or Null is not Null")

Call ok(getVT(Empty Xor Empty) = "VT_I4",    "getVT(Empty Xor Empty) = " & getVT(Empty Xor Empty))
Call ok((Empty Xor CInt(5)) = 5,             "Empty Xor CInt(5) is not 5")
Call ok(getVT(Empty Xor CInt(5)) = "VT_I4",  "getVT(Empty Xor CInt(5)) = " & getVT(Empty Xor CInt(5)))
Call ok(isNull(Empty Xor Null),              "Empty Xor Null is not Null")

Call ok((Empty Eqv Empty) = -1,              "Empty Eqv Empty is not -1")
Call ok(getVT(Empty Eqv Empty) = "VT_I4",    "getVT(Empty Eqv Empty) = " & getVT(Empty Eqv Empty))
Call ok(isNull(Empty Eqv Null),              "Empty Eqv Null is not Null")

Call ok((Empty Imp Empty) = -1,              "Empty Imp Empty is not -1")
Call ok(getVT(Empty Imp Empty) = "VT_I4",    "getVT(Empty Imp Empty) = " & getVT(Empty Imp Empty))
Call ok((Empty Imp False) = -1,              "Empty Imp False is not -1")
Call ok((Empty Imp Null) = -1,               "Empty Imp Null is not -1")
Call ok(getVT(Empty Imp Null) = "VT_I4",     "getVT(Empty Imp Null) = " & getVT(Empty Imp Null))

Call ok((Not Empty) = -1,                    "Not Empty is not -1")
Call ok(getVT(Not Empty) = "VT_I4",          "getVT(Not Empty) = " & getVT(Not Empty))

' Arithmetic binary ops coerce Empty to VT_I2 0 — narrower than the logical
' family — so the widening picks up whichever side has the larger numeric
' type and the result reflects that.
Call ok((Empty + Empty) = 0,                 "Empty + Empty is not 0")
Call ok(getVT(Empty + Empty) = "VT_I2",      "getVT(Empty + Empty) = " & getVT(Empty + Empty))
Call ok((Empty + CInt(5)) = 5,               "Empty + CInt(5) is not 5")
Call ok(getVT(Empty + CInt(5)) = "VT_I2",    "getVT(Empty + CInt(5)) = " & getVT(Empty + CInt(5)))
Call ok((Empty + CLng(123456)) = 123456,     "Empty + CLng(123456) is not 123456")
Call ok(getVT(Empty + CLng(123456)) = "VT_I4",    "getVT(Empty + CLng(123456)) = " & getVT(Empty + CLng(123456)))
Call ok(getVT(Empty + CDbl(1.5)) = "VT_R8",  "getVT(Empty + CDbl(1.5)) = " & getVT(Empty + CDbl(1.5)))
Call ok(getVT(Empty + CCur(12.34)) = "VT_CY",     "getVT(Empty + CCur(12.34)) = " & getVT(Empty + CCur(12.34)))
Call ok((Empty + False) = 0,                 "Empty + False is not 0")
Call ok(getVT(Empty + False) = "VT_I2",      "getVT(Empty + False) = " & getVT(Empty + False))

Call ok((Empty - CInt(5)) = -5,              "Empty - CInt(5) is not -5")
Call ok(getVT(Empty - CInt(5)) = "VT_I2",    "getVT(Empty - CInt(5)) = " & getVT(Empty - CInt(5)))

Call ok((Empty * CInt(5)) = 0,               "Empty * CInt(5) is not 0")
Call ok(getVT(Empty * CInt(5)) = "VT_I2",    "getVT(Empty * CInt(5)) = " & getVT(Empty * CInt(5)))

Call ok((Empty \ CInt(1)) = 0,               "Empty \\ CInt(1) is not 0")
Call ok(getVT(Empty \ CInt(1)) = "VT_I2",    "getVT(Empty \\ CInt(1)) = " & getVT(Empty \ CInt(1)))

Call ok((Empty Mod CInt(3)) = 0,             "Empty Mod CInt(3) is not 0")
Call ok(getVT(Empty Mod CInt(3)) = "VT_I2",  "getVT(Empty Mod CInt(3)) = " & getVT(Empty Mod CInt(3)))

Call ok((-Empty) = 0,                        "-Empty is not 0")
Call ok(getVT(-Empty) = "VT_I2",             "getVT(-Empty) = " & getVT(-Empty))

' Unary + and string concat do NOT coerce Empty. Unary + is essentially a
' no-op, and & makes "" out of Empty inside the concat, not a number.
Call ok(getVT(+Empty) = "VT_EMPTY",          "getVT(+Empty) = " & getVT(+Empty))
Call ok((Empty & "foo") = "foo",             "Empty & 'foo' is not 'foo'")
Call ok(getVT(Empty & "foo") = "VT_BSTR",    "getVT(Empty & 'foo') = " & getVT(Empty & "foo"))
Call ok(("foo" & Empty) = "foo",             "'foo' & Empty is not 'foo'")

' Reverse-order smoke tests: Empty on the right of commutative ops must
' coerce too, otherwise a missing-r-coerce bug would only show up for
' particular operand shapes. Values match the Empty-on-left rows above.
Call ok((CInt(5) And Empty) = 0,             "CInt(5) And Empty is not 0")
Call ok(getVT(CInt(5) And Empty) = "VT_I4",  "getVT(CInt(5) And Empty) = " & getVT(CInt(5) And Empty))
Call ok((CInt(5) Or  Empty) = 5,             "CInt(5) Or Empty is not 5")
Call ok(getVT(CInt(5) Or  Empty) = "VT_I4",  "getVT(CInt(5) Or Empty) = " & getVT(CInt(5) Or Empty))
Call ok((CInt(5) Xor Empty) = 5,             "CInt(5) Xor Empty is not 5")
Call ok(getVT(CInt(5) Xor Empty) = "VT_I4",  "getVT(CInt(5) Xor Empty) = " & getVT(CInt(5) Xor Empty))
Call ok((CInt(5) +   Empty) = 5,             "CInt(5) + Empty is not 5")
Call ok(getVT(CInt(5) +   Empty) = "VT_I2",  "getVT(CInt(5) + Empty) = " & getVT(CInt(5) + Empty))
Call ok((CInt(5) *   Empty) = 0,             "CInt(5) * Empty is not 0")
Call ok(getVT(CInt(5) *   Empty) = "VT_I2",  "getVT(CInt(5) * Empty) = " & getVT(CInt(5) * Empty))

' Asymmetric-operator reverse-order tests. Imp is A Imp B = Not A Or B so
' the result depends on the left operand's bit pattern. Subtraction and
' exponent are also non-commutative and each direction is a separate case.
Call ok((CInt(0) Imp Empty) = -1,            "CInt(0) Imp Empty is not -1")
Call ok(getVT(CInt(0) Imp Empty) = "VT_I4",  "getVT(CInt(0) Imp Empty) = " & getVT(CInt(0) Imp Empty))
Call ok((CInt(5) Imp Empty) = -6,            "CInt(5) Imp Empty is not -6")
Call ok(getVT(CInt(5) Imp Empty) = "VT_I4",  "getVT(CInt(5) Imp Empty) = " & getVT(CInt(5) Imp Empty))
Call ok((False Imp Empty) = -1,              "False Imp Empty is not -1")
Call ok(getVT(False Imp Empty) = "VT_I4",    "getVT(False Imp Empty) = " & getVT(False Imp Empty))
Call ok((True Imp Empty) = 0,                "True Imp Empty is not 0")
Call ok(getVT(True Imp Empty) = "VT_I4",     "getVT(True Imp Empty) = " & getVT(True Imp Empty))
Call ok(isNull(Null Imp Empty),              "Null Imp Empty is not Null")

Call ok((CInt(5) - Empty) = 5,               "CInt(5) - Empty is not 5")
Call ok(getVT(CInt(5) - Empty) = "VT_I2",    "getVT(CInt(5) - Empty) = " & getVT(CInt(5) - Empty))
Call ok((CLng(7) - Empty) = 7,               "CLng(7) - Empty is not 7")
Call ok(getVT(CLng(7) - Empty) = "VT_I4",    "getVT(CLng(7) - Empty) = " & getVT(CLng(7) - Empty))

Call ok((Empty ^ CInt(2)) = 0,               "Empty ^ CInt(2) is not 0")
Call ok(getVT(Empty ^ CInt(2)) = "VT_R8",    "getVT(Empty ^ CInt(2)) = " & getVT(Empty ^ CInt(2)))
Call ok((CInt(2) ^ Empty) = 1,               "CInt(2) ^ Empty is not 1")
Call ok(getVT(CInt(2) ^ Empty) = "VT_R8",    "getVT(CInt(2) ^ Empty) = " & getVT(CInt(2) ^ Empty))

' Dividing BY Empty is dividing by zero once the arithmetic coercion runs,
' so /, \ and Mod must raise runtime error 11 (division by zero).
Dim div_r
On Error Resume Next
Err.Clear
div_r = CInt(1) / Empty
Call ok(Err.Number = 11, "CInt(1) / Empty should raise 11: " & Err.Number)
Err.Clear
div_r = CInt(1) \ Empty
Call ok(Err.Number = 11, "CInt(1) \\ Empty should raise 11: " & Err.Number)
Err.Clear
div_r = CInt(3) Mod Empty
Call ok(Err.Number = 11, "CInt(3) Mod Empty should raise 11: " & Err.Number)
Err.Clear
On Error Goto 0

Call ok(2 >= 1, "! 2 >= 1")
Call ok(2 >= 2, "! 2 >= 2")
Call ok(2 => 1, "! 2 => 1")
Call ok(2 => 2, "! 2 => 2")
Call ok(not(true >= 2), "true >= 2 ?")
Call ok(2 > 1, "! 2 > 1")
Call ok(false > true, "! false < true")
Call ok(0 > true, "! 0 > true")
Call ok(not (true > 0), "true > 0")
Call ok(not (0 > 1 = 1), "0 > 1 = 1")
Call ok(1 < 2, "! 1 < 2")
Call ok(1 = 1 < 0, "! 1 = 1 < 0")
Call ok(1 <= 2, "! 1 <= 2")
Call ok(2 <= 2, "! 2 <= 2")
Call ok(1 =< 2, "! 1 =< 2")
Call ok(2 =< 2, "! 2 =< 2")
Call ok(not (2 >< 2), "2 >< 2")
Call ok(2 >< 1, "! 2 >< 1")
Call ok(not (2 <> 2), "2 <> 2")
Call ok(2 <> 1, "! 2 <> 1")

Call ok(isNull(0 = null), "'(0 = null)' is not null")
Call ok(isNull(null = 1), "'(null = 1)' is not null")
Call ok(isNull(0 > null), "'(0 > null)' is not null")
Call ok(isNull(null > 1), "'(null > 1)' is not null")
Call ok(isNull(0 < null), "'(0 < null)' is not null")
Call ok(isNull(null < 1), "'(null < 1)' is not null")
Call ok(isNull(0 <> null), "'(0 <> null)' is not null")
Call ok(isNull(null <> 1), "'(null <> 1)' is not null")
Call ok(isNull(0 >= null), "'(0 >= null)' is not null")
Call ok(isNull(null >= 1), "'(null >= 1)' is not null")
Call ok(isNull(0 <= null), "'(0 <= null)' is not null")
Call ok(isNull(null <= 1), "'(null <= 1)' is not null")

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
Call ok(empty mod 2 = 0, "empty mod 2 = " & (empty mod 2))

Call ok(5 \ 2 = 2, "5 \ 2 = " & (5\2))
Call ok(4.6 \ 1.5 = 2, "4.6 \ 1.5 = " & (4.6\1.5))
Call ok(4.6 \ 1.49 = 5, "4.6 \ 1.49 = " & (4.6\1.49))
Call ok(2+3\4 = 2, "2+3\4 = " & (2+3\4))

Call ok(2*3 = 6, "2*3 = " & (2*3))
Call ok(3/2 = 1.5, "3/2 = " & (3/2))
Call ok(5\4/2 = 2, "5\4/2 = " & (5\2/1))
Call ok(12/3\2 = 2, "12/3\2 = " & (12/3\2))
Call ok(5/1000000 = 0.000005, "5/1000000 = " & (5/1000000))

Call ok(2^3 = 8, "2^3 = " & (2^3))
Call ok(2^3^2 = 64, "2^3^2 = " & (2^3^2))
Call ok(-3^2 = 9, "-3^2 = " & (-3^2))
Call ok(2*3^2 = 18, "2*3^2 = " & (2*3^2))

x =_
    3
x _
    = 3

x = 3

' Maximum identifier length (255 chars)
Dim aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa = 42
Call ok(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa = 42, "255-char identifier should work")

' Bracketed identifiers
Dim [my var]
[my var] = 42
Call ok([my var] = 42, "[my var] = " & [my var])

Dim [hello world!]
[hello world!] = "works"
Call ok([hello world!] = "works", "[hello world!] = " & [hello world!])

Dim []
[] = "empty"
Call ok([] = "empty", "[] = " & [])

Dim [   ]
[   ] = "spaces"
Call ok([   ] = "spaces", "[   ] = " & [   ])

Dim [dim]
[dim] = 99
Call ok([dim] = 99, "[dim] = " & [dim])

Class BracketTestObj
    Public [my property]
    Public Sub Class_Initialize
        [my property] = "init"
    End Sub
End Class

Dim bracketObj
Set bracketObj = New BracketTestObj
bracketObj.[my property] = "updated"
Call ok(bracketObj.[my property] = "updated", "bracketObj.[my property] = " & bracketObj.[my property])

Dim [loop var]
Dim bracketTotal : bracketTotal = 0
For [loop var] = 1 To 3
    bracketTotal = bracketTotal + [loop var]
Next
Call ok(bracketTotal = 6, "For [loop var] total = " & bracketTotal)

' Chained call syntax
Class ChainedCallTarget
    Public Function Ret()
        Set Ret = Me
    End Function
End Class

Dim chainObj
Set chainObj = New ChainedCallTarget
chainObj.Ret().Ret()
chainObj.Ret() _
.Ret()
chainObj _
.Ret() _
.Ret()

if true then y = true : x = y
ok x, "x is false"

x = true : if false then x = false
ok x, "x is false, if false called?"

if not false then x = true
ok x, "x is false, if not false not called?"

if not false then x = "test" : x = true
ok x, "x is false, if not false not called?"

if false then x = y : call ok(false, "if false .. : called")

if false then x = y : call ok(false, "if false .. : called") else x = "else"
Call ok(x = "else", "else not called?")

if true then x = y else y = x : Call ok(false, "in else?")

if false then :

if false then x = y : if true then call ok(false, "embedded if called")

if false then x=1 else x=2 end if
if true then x=1 end if

x = false
if false then x = true : x = true
Call ok(x = false, "x <> false")

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

' Else without following newline
x = false
If false Then
   Call ok(false, "inside if false")
Else x = true
End If
Call ok(x, "else not called?")

' Else with colon before statement following newline
x = false
If false Then
   Call ok(false, "inside if false")
Else
: x = true
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

' ElseIf with statement on same line
x = false
If false Then
   Call ok(false, "inside if false")
ElseIf not False Then x = true
End If
Call ok(x, "elseif not called?")

' ElseIf with statement following statement separator
x = false
If false Then
   Call ok(false, "inside if false")
ElseIf not False Then
: x = true
End If
Call ok(x, "elseif not called?")

If false Then x = 1 Else
If false Then x = 1 Else:

' Else with trailing End If on same line
x = 1
If false Then
   x = 2
Else x = 3 End If
Call ok(x = 3, "Else with trailing End If failed")

' Else with colon separator before End If
x = 1
If false Then
   x = 2
Else x = 3:End If
Call ok(x = 3, "Else with colon before End If failed")

' Else with colon then statement with trailing End If
x = 1
If false Then
   x = 2
Else:x = 3 End If
Call ok(x = 3, "Else colon then statement with trailing End If failed")

' Else with multiple statements and trailing End If
x = 1
y = 1
If false Then
   x = 2
Else x = 3:y = 4 End If
Call ok(x = 3, "Else multi-statement trailing End If failed for x")
Call ok(y = 4, "Else multi-statement trailing End If failed for y")

' Else with body statement on next line with trailing End If
x = 1
If false Then
   x = 2
Else
   x = 3 End If
Call ok(x = 3, "Else body statement with trailing End If failed")

' ElseIf single-line with trailing End If
x = 1
If false Then
   x = 2
ElseIf true Then x = 3 End If
Call ok(x = 3, "ElseIf single-line with trailing End If failed")

' Empty Else block with newline
x = 1
If false Then
   x = 2
Else
End If
Call ok(x = 1, "empty Else block should be accepted")

x = false
If true Then
  :x = true
End If
Call ok(x, "colon-prefixed statement in If block not executed")

x = false
if 1 then x = true
Call ok(x, "if 1 not run?")

x = false
if &h10000& then x = true
Call ok(x, "if &h10000& not run?")

x = false
y = false
while not (x and y)
    if x then
        y = true
    end if
    x = true
wend
call ok((x and y), "x or y is false after while")

if false then
' empty body
end if

if false then
    x = false
elseif true then
' empty body
end if

if false then
    x = false
else
' empty body
end if

while false
wend

if empty then
   ok false, "if empty executed"
end if

while empty
   ok false, "while empty executed"
wend

x = 0
if "0" then
   ok false, "if ""0"" executed"
else
   x = 1
end if
Call ok(x = 1, "if ""0"" else not executed")

x = 0
if "-1" then
   x = 1
else
   ok false, "if ""-1"" else executed"
end if
Call ok(x = 1, "if ""-1"" not executed")

x = 0
if 0.1 then
   x = 1
else
   ok false, "if ""0.1"" else executed"
end if
Call ok(x = 1, "if ""0.1"" not executed")

x = 0
if "TRUE" then
   x = 1
else
   ok false, "if ""TRUE"" else executed"
end if
Call ok(x = 1, "if ""TRUE"" not executed")

x = 0
if "#TRUE#" then
   x = 1
else
   ok false, "if ""#TRUE#"" else executed"
end if
Call ok(x = 1, "if ""#TRUE#"" not executed")

x = 0
if (not "#FALSE#") then
   x = 1
else
   ok false, "if ""not #FALSE#"" else executed"
end if
Call ok(x = 1, "if ""not #FALSE#"" not executed")

Class ValClass
    Public myval

    Public default Property Get defprop
        defprop = myval
    End Property
End Class

Dim MyObject
Set MyObject = New ValClass

MyObject.myval = 1
Call ok(CBool(MyObject) = True, "CBool(MyObject) = " & CBool(MyObject))
x = 0
if MyObject then
   x = 1
else
   ok false, "if ""MyObject(1)"" else executed"
end if
Call ok(x = 1, "if ""MyObject(1)"" not executed")

MyObject.myval = 0
Call ok(CBool(MyObject) = False, "CBool(MyObject) = " & CBool(MyObject))
x = 0
if not MyObject then
   x = 1
else
   ok false, "if ""MyObject(0)"" else executed"
end if
Call ok(x = 1, "if ""MyObject(0)"" not executed")

x = 0
WHILE x < 3 : x = x + 1 : Wend
Call ok(x = 3, "x not equal to 3")

x = 0
WHILE x < 3 : x = x + 1
Wend
Call ok(x = 3, "x not equal to 3")

z = 2
while z > -4 :


z = z -2
wend

x = false
y = false
do while not (x and y)
    if x then
        y = true
    end if
    x = true
loop
call ok((x and y), "x or y is false after while")

do while false
loop

do while false : loop

do while true
    exit do
    ok false, "exit do didn't work"
loop

x = 0
Do While x < 2 : x = x + 1
Loop
Call ok(x = 2, "x not equal to 2")

x = 0
Do While x < 2 : x = x + 1: Loop
Call ok(x = 2, "x not equal to 2")

x = 0
Do While x >= -2 :
x = x - 1
Loop
Call ok(x = -3, "x not equal to -3")

x = false
y = false
do until x and y
    if x then
        y = true
    end if
    x = true
loop
call ok((x and y), "x or y is false after do until")

do until true
loop

do until false
    exit do
    ok false, "exit do didn't work"
loop

x = 0
Do: :: x = x + 2
Loop Until x = 4
Call ok(x = 4, "x not equal to 4")

x = 0
Do: :: x = x + 2 :::  : Loop Until x = 4
Call ok(x = 4, "x not equal to 4")

x = 5
Do: :

: x = x * 2
Loop Until x = 40
Call ok(x = 40, "x not equal to 40")


x = false
do
    if x then exit do
    x = true
loop
call ok(x, "x is false after do..loop?")

x = 0
Do :If x = 6 Then
        Exit Do
    End If
    x = x + 3
Loop
Call ok(x = 6, "x not equal to 6")

x = false
y = false
do
    if x then
        y = true
    end if
    x = true
loop until x and y
call ok((x and y), "x or y is false after while")

do
loop until true

do
    exit do
    ok false, "exit do didn't work"
loop until false

x = false
y = false
do
    if x then
        y = true
    end if
    x = true
loop while not (x and y)
call ok((x and y), "x or y is false after while")

do
loop while false

do
    exit do
    ok false, "exit do didn't work"
loop while true

y = "for1:"
for x = 5 to 8
    y = y & " " & x
next
Call ok(y = "for1: 5 6 7 8", "y = " & y)

y = "for2:"
for x = 5 to 8 step 2
    y = y & " " & x
next
Call ok(y = "for2: 5 7", "y = " & y)

y = "for3:"
x = 2
for x = x+3 to 8
    y = y & " " & x
next
Call ok(y = "for3: 5 6 7 8", "y = " & y)

y = "for4:"
for x = 5 to 4
    y = y & " " & x
next
Call ok(y = "for4:", "y = " & y)

y = "for5:"
for x = 5 to 3 step true
    y = y & " " & x
next
Call ok(y = "for5: 5 4 3", "y = " & y)

y = "for6:"
z = 4
for x = 5 to z step 3-4
    y = y & " " & x
    z = 0
next
Call ok(y = "for6: 5 4", "y = " & y)

y = "for7:"
z = 1
for x = 5 to 8 step z
    y = y & " " & x
    z = 2
next
Call ok(y = "for7: 5 6 7 8", "y = " & y)

z = 0
For x = 10 To 18 Step 2 : : z = z + 1
Next
Call ok(z = 5, "z not equal to 5")

y = "for8:"
for x = 5 to 8
    y = y & " " & x
    x = x+1
next
Call ok(y = "for8: 5 7", "y = " & y)

function testfor( startvalue, endvalue, stepvalue, steps)
    Dim s
    s=0
    for x=startvalue to endvalue step stepvalue
        s = s + 1
    Next
    Call ok( s = steps, "counted " & s & " steps in for loop, expected " & steps)
end function

Call testfor (1, 2, 1, 2)
Call testfor ("1", 2, 1, 2)
Call testfor (1, "2", 1, 2)
Call testfor (1, 2, "1", 2)
Call testfor ("1", "2", "1", 2)
if (isEnglishLang) then
    Call testfor (1, 2, 0.5, 3)
    Call testfor (1, 2.5, 0.5, 4)
    Call testfor ("1", 2,  0.5, 3)
    Call testfor ("1", 2.5, 0.5, 4)
    Call testfor (1, "2",  0.5, 3)
    Call testfor (1, "2.5", 0.5, 4)
    Call testfor (1, 2, "0.5", 3)
    Call testfor (1, 2.5, "0.5", 4)
    Call testfor ("1", "2", "0.5", 3)
    Call testfor ("1", "2.5", "0.5", 4)
end if

' Empty is treated as 0 in for-to loop expressions
y = 0
for x = empty to 3
    y = y + 1
next
call ok(y = 4, "for empty to 3: y = " & y)
call ok(x = 4, "for empty to 3: x = " & x)

y = 0
for x = 0 to empty
    y = y + 1
next
call ok(y = 1, "for 0 to empty: y = " & y)
call ok(x = 1, "for 0 to empty: x = " & x)

for x = 1.5 to 1
    Call ok(false, "for..to called when unexpected")
next

for x = 1 to 100
    exit for
    Call ok(false, "exit for not escaped the loop?")
next

for x = 1 to 5 :
:
:   :exit for
    Call ok(false, "exit for not escaped the loop?")
next

dim a1(8)
a1(6)=8
for x=1 to 8:a1(x)=x-1:next
Call ok(a1(6) = 5, "colon used in for loop")

a1(6)=8
for x=1 to 8:y=1
a1(x)=x-2:next
Call ok(a1(6) = 4, "colon used in for loop")

do while true
    for x = 1 to 100
        exit do
    next
loop

' For loop control variable should not be modified when expression evaluation fails
on error resume next

x = 6
y = 0
err.clear
for x = 1/0 to 100
    y = y + 1
next
call ok(err.number = 92, "for (from error): err.number = " & err.number)
call ok(x = 6, "for (from error): x = " & x)

x = 6
y = 0
err.clear
for x = 100 to 1/0
    y = y + 1
next
call ok(err.number = 92, "for (to error): err.number = " & err.number)
call ok(x = 6, "for (to error): x = " & x)

x = 6
y = 0
err.clear
for x = 100 to 200 step 1/0
    y = y + 1
next
call ok(err.number = 92, "for (step error): err.number = " & err.number)
call ok(x = 6, "for (step error): x = " & x)

z = 99
y = 0
err.clear
for z = 1 to UBound(empty)
    y = y + 1
next
call ok(err.number = 92, "for (UBound(empty)): err.number = " & err.number)
call ok(z = 99, "for (UBound(empty)): z = " & z)

' For loop variable set to incompatible type during iteration
x = 0
y = 0
err.clear
for x = 1 to 5
    y = y + 1
    if x = 3 then x = "not a number"
next
call ok(y = 3, "for (type change): y = " & y)
call ok(x = "not a number", "for (type change): x = " & x)
call ok(err.number = 13, "for (type change): err.number = " & err.number)

on error goto 0

' For loop expression evaluation order: from, to, step
dim eval_order
function trackEval(val, label)
    eval_order = eval_order & label
    trackEval = val
end function

eval_order = ""
for x = trackEval(1, "F") to trackEval(5, "T") step trackEval(1, "S")
next
call ok(eval_order = "FTS", "for eval order = " & eval_order)

eval_order = ""
for x = trackEval(1, "F") to trackEval(5, "T")
next
call ok(eval_order = "FT", "for eval order (no step) = " & eval_order)

if null then call ok(false, "if null evaluated")

while null
    call ok(false, "while null evaluated")
wend

Call collectionObj.reset()
y = 0
for each x in collectionObj :

   :y = y + 3
next
Call ok(y = 9, "y = " & y)

Call collectionObj.reset()
y = 0
x = 10
z = 0
for each x in collectionObj : z = z + 2
    y = y+1
    Call ok(x = y, "x <> y")
next
Call ok(y = 3, "y = " & y)
Call ok(z = 6, "z = " & z)
Call ok(getVT(x) = "VT_EMPTY*", "getVT(x) = " & getVT(x))

Call collectionObj.reset()
y = 0
x = 10
z = 0
for each x in collectionObj : z = z + 2 : y = y+1 ::
Call ok(x = y, "x <> y") : next
Call ok(y = 3, "y = " & y)
Call ok(z = 6, "z = " & z)

Call collectionObj.reset()
y = false
for each x in collectionObj
    if x = 2 then exit for
    y = 1
next
Call ok(y = 1, "y = " & y)
Call ok(x = 2, "x = " & x)

Set obj = collectionObj
Call obj.reset()
y = 0
x = 10
for each x in obj
    y = y+1
    Call ok(x = y, "x <> y")
next
Call ok(y = 3, "y = " & y)
Call ok(getVT(x) = "VT_EMPTY*", "getVT(x) = " & getVT(x))

x = false
select case 3
    case 2
        Call ok(false, "unexpected case")
    case 2
        Call ok(false, "unexpected case")
    case 4
        Call ok(false, "unexpected case")
    case "test"
    case "another case"
        Call ok(false, "unexpected case")
    case 0, false, 2+1, 10
        x = true
    case ok(false, "unexpected case")
        Call ok(false, "unexpected case")
    case else
        Call ok(false, "unexpected case")
end select
Call ok(x, "wrong case")

x = false
select case 3
    case 3
        x = true
end select
Call ok(x, "wrong case")

x = false
select case 2+2
    case 3
        Call ok(false, "unexpected case")
    case else
        x = true
end select
Call ok(x, "wrong case")

y = "3"
x = false
select case y
    case "3"
        x = true
    case 3
        Call ok(false, "unexpected case")
end select
Call ok(x, "wrong case")

select case 0
    case 1
        Call ok(false, "unexpected case")
    case "2"
        Call ok(false, "unexpected case")
end select

select case 0
end select

x = false
select case 2
    case 3,1,2,4: x = true
    case 5,6,7
        Call ok(false, "unexpected case")
end select
Call ok(x, "wrong case")

x = false
select case 2: case 5,6,7: Call ok(false, "unexpected case")
    case 2,1,2,4
        x = true
    case else: Call ok(false, "unexpected case else")
end select
Call ok(x, "wrong case")

x = False
select case 1  :

    :case 3, 4 :


    case 5
:
        Call ok(false, "unexpected case") :
    Case Else:

        x = True
end select
Call ok(x, "wrong case")

select case 0
    case 1
    case else
       'empty else with comment test
end select

select case 0 : case 1 : case else : end select

' Case without separator
function SelectCaseTest(x)
    select case x
        case 0: SelectCaseTest = 100
        case 1  SelectCaseTest = 200
        case 2
                SelectCaseTest = 300
        case 3
        case 4  SelectCaseTest = 400
        case else SelectCaseTest = 500
    end select
end function

call ok(SelectCaseTest(0) = 100, "Unexpected case " & SelectCaseTest(0))
call ok(SelectCaseTest(1) = 200, "Unexpected case " & SelectCaseTest(1))
call ok(SelectCaseTest(2) = 300, "Unexpected case " & SelectCaseTest(2))
call ok(SelectCaseTest(3) = vbEmpty, "Unexpected case " & SelectCaseTest(3))
call ok(SelectCaseTest(4) = 400, "Unexpected case " & SelectCaseTest(4))
call ok(SelectCaseTest(5) = 500, "Unexpected case " & SelectCaseTest(5))

if false then
Sub testsub
    x = true
End Sub
end if

x = false
Call testsub
Call ok(x, "x is false, testsub not called?")

if false then
Sub testsub_one_line() x = true End Sub
end if

x = false
Call testsub_one_line
Call ok(x, "x is false, testsub_one_line not called?")

Sub SubSetTrue(v)
    Call ok(not v, "v is not true")
    v = true
End Sub

x = false
SubSetTrue x
Call ok(x, "x was not set by SubSetTrue")

SubSetTrue false
Call ok(not false, "false is no longer false?")

Sub SubSetTrue2(ByRef v)
    Call ok(not v, "v is not true")
    v = true
End Sub

x = false
SubSetTrue2 x
Call ok(x, "x was not set by SubSetTrue")

Sub TestSubArgVal(ByVal v)
    Call ok(not v, "v is not false")
    v = true
    Call ok(v, "v is not true?")
End Sub

x = false
Call TestSubArgVal(x)
Call ok(not x, "x is true after TestSubArgVal call?")

Sub TestSubMultiArgs(a,b,c,d,e)
    Call ok(a=1, "a = " & a)
    Call ok(b=2, "b = " & b)
    Call ok(c=3, "c = " & c)
    Call ok(d=4, "d = " & d)
    Call ok(e=5, "e = " & e)
End Sub

Sub TestSubExit(ByRef a)
    If a Then
        Exit Sub
    End If
    Call ok(false, "Exit Sub not called?")
End Sub

Call TestSubExit(true)

Sub TestSubExit2
    for x = 1 to 100
        Exit Sub
    next
End Sub
Call TestSubExit2

TestSubMultiArgs 1, 2, 3, 4, 5
Call TestSubMultiArgs(1, 2, 3, 4, 5)

Sub TestSubParenExpr(a, b)
    Call ok(a=16, "a = " & a)
    Call ok(b=7, "b = " & b)
End Sub

TestSubParenExpr (2) * 8, 7
TestSubParenExpr 8 * (2), 7

Sub TestSubParenExprAdd(a, b)
    Call ok(a=6, "a = " & a)
    Call ok(b=7, "b = " & b)
End Sub

TestSubParenExprAdd (2) + 4, 7
TestSubParenExprAdd 4 + (2), 7

Sub TestSubParenExprNoSpace(a)
    Call ok(a=6, "a = " & a)
End Sub

TestSubParenExprNoSpace(10 \ 2) + 1

' Regression test: function call with space before ( in expression context
' e.g. x = (CInt (2) + 1) * 3 must parse and evaluate correctly
x = CInt (2) + 1
Call ok(x = 3, "CInt (2) + 1 = " & x)
x = (CInt (2) + 1) * 3
Call ok(x = 9, "(CInt (2) + 1) * 3 = " & x)

' Regression test: function call with space before ( and * in expression context
' e.g. x = CInt (2) * 3 must treat CInt (2) as a function call, not expression grouping
x = CInt (2) * 3
Call ok(x = 6, "CInt (2) * 3 = " & x)

' Test member expression in statement context: obj.Method (x) * y, z
Class TestObjParenExpr
    Sub Check(a, b)
        Call ok(a=16, "obj a = " & a)
        Call ok(b=7, "obj b = " & b)
    End Sub
End Class

Dim objParenExpr
Set objParenExpr = New TestObjParenExpr
objParenExpr.Check (2) * 8, 7

Sub TestSubParenExprConcat(a, b)
    Call ok(a="helloworld", "a = " & a)
    Call ok(b=7, "b = " & b)
End Sub

TestSubParenExprConcat ("hello") & "world", 7

' Test: function call as argument with & after paren must be a call, not grouping
' e.g. TestSub Mid ("hello", 2) & "x" should call TestSub with "ellox"
' Mid("hello", 2) returns "ello", & "x" concatenates to "ellox"
Sub TestSubArgCallConcat(a)
    Call ok(a="ellox", "a = " & a)
End Sub

TestSubArgCallConcat Mid ("hello", 2) & "x"

' Test: obj(idx).method (expr) * val, y in statement context
' The (expr) after .method must be expression grouping, not call paren
Class TestIndexedObjParenExpr
    Public arr_(1)
    Public Sub Init()
        Set arr_(0) = New TestObjParenExpr
    End Sub
    Public Default Property Get Item(idx)
        Set Item = arr_(idx)
    End Property
End Class

Dim idxObj
Set idxObj = New TestIndexedObjParenExpr
Call idxObj.Init()
idxObj(0).Check (2) * 8, 7

' No-space variants of Sub-first-arg paren pattern.
' On native VBScript, `S(x) OP y` in statement context treats the whole
' `S(x) OP y` as a call to S with argument `(x) OP y` — for every binary
' operator except `=` (parsed as assignment).
' Each case is wrapped in Execute so parse failures of one don't abort the rest.
Dim npArg, npArgA, npArgB
Sub NpS(a)
    npArg = a
End Sub
Sub NpT(a, b)
    npArgA = a
    npArgB = b
End Sub

Sub CheckNpS(src, expected)
    npArg = Empty
    On Error Resume Next
    Err.Clear
    Execute src
    Dim e : e = Err.Number
    On Error GoTo 0
    Call ok(e = 0, "parse error for " & src & ": err=" & e)
    If e = 0 Then Call ok(npArg = expected, src & ": npArg = " & npArg & " expected " & expected)
End Sub

CheckNpS "NpS(10)+5",                15
CheckNpS "NpS(10)-3",                7
CheckNpS "NpS(10)*3",                30
CheckNpS "NpS(10)/2",                5
CheckNpS "NpS(10)\3",                3
CheckNpS "NpS(10)^2",                100
CheckNpS "NpS(""hi"")&""!""",        "hi!"
CheckNpS "NpS(10) Mod 3",            1
CheckNpS "NpS(10)<>10",              False
CheckNpS "NpS(10)<5",                False
CheckNpS "NpS(10)>5",                True
CheckNpS "NpS(10)<=10",              True
CheckNpS "NpS(10)>=5",               True
CheckNpS "NpS(1) And 1",             1
CheckNpS "NpS(0) Or 1",              1
CheckNpS "NpS(1) Xor 1",             0
CheckNpS "NpS(1) Eqv 1",             -1
CheckNpS "NpS(1) Imp 1",             -1
CheckNpS "NpS(Nothing) Is Nothing",  True

' Two-arg form: S(x) OP y, z — result of `(x) OP y` is first arg, z is second.
Sub CheckNpT(src, expectedA, expectedB)
    npArgA = Empty
    npArgB = Empty
    On Error Resume Next
    Err.Clear
    Execute src
    Dim e : e = Err.Number
    On Error GoTo 0
    Call ok(e = 0, "parse error for " & src & ": err=" & e)
    If e = 0 Then Call ok(npArgA = expectedA and npArgB = expectedB, _
        src & ": a=" & npArgA & " b=" & npArgB)
End Sub

CheckNpT "NpT(10)+5, 7",             15,      7
CheckNpT "NpT(10)*3, 7",             30,      7
CheckNpT "NpT(""hi"")&""!"", 7",     "hi!",   7

' Member expression: obj.Method(x) OP y — no space, same pattern.
Class NpCls
    Sub Check(a)
        npArg = a
    End Sub
End Class
Dim npObj
Set npObj = New NpCls
CheckNpS "npObj.Check(10)+5",        15
CheckNpS "npObj.Check(10)*3",        30

Function ParenId(a)
    ParenId = a
End Function

Dim parenRes
parenRes = 0
If False Then
ElseIf ParenId(3) <= ParenId(4) + 0.1 Then
    parenRes = 1
End If
Call ok(parenRes = 1, "ElseIf f(x) <= f(y) + z: parenRes = " & parenRes)

parenRes = 0
If False Then
ElseIf ParenId(3) * 2 > 0 Then
    parenRes = 1
End If
Call ok(parenRes = 1, "ElseIf f(x) * y > z: parenRes = " & parenRes)

Dim parenOuter, parenInner
ReDim parenOuter(3)
parenInner = Array(1, 3, 5, 7)
parenOuter(parenInner(1) And 1) = 99
Call ok(parenOuter(1) = 99, "outer(inner(i) And k) = v: parenOuter(1) = " & parenOuter(1))

Sub TestSubLocalVal
    x = false
    Call ok(not x, "local x is not false?")
    Dim x
    Dim a,b, c
End Sub

x = true
y = true
Call TestSubLocalVal
Call ok(x, "global x is not true?")

Public Sub TestPublicSub
End Sub
Call TestPublicSub

Private Sub TestPrivateSub
End Sub
Call TestPrivateSub

Public Sub TestSeparatorSub : :
:
End Sub
Call TestSeparatorSub

if false then
Function testfunc
    x = true
End Function
end if

x = false
Call TestFunc
Call ok(x, "x is false, testfunc not called?")

if false then
Function testfunc_one_line() x = true End Function
end if

x = false
Call testfunc_one_line
Call ok(x, "x is false, testfunc_one_line not called?")

Function FuncSetTrue(v)
    Call ok(not v, "v is not true")
    v = true
End Function

x = false
FuncSetTrue x
Call ok(x, "x was not set by FuncSetTrue")

FuncSetTrue false
Call ok(not false, "false is no longer false?")

Function FuncSetTrue2(ByRef v)
    Call ok(not v, "v is not true")
    v = true
End Function

x = false
FuncSetTrue2 x
Call ok(x, "x was not set by FuncSetTrue")

Function TestFuncArgVal(ByVal v)
    Call ok(not v, "v is not false")
    v = true
    Call ok(v, "v is not true?")
End Function

x = false
Call TestFuncArgVal(x)
Call ok(not x, "x is true after TestFuncArgVal call?")

const c10 = 10
Sub TestParamvsConst(c10)
    Call ok( c10 = 42, "precedence between const and parameter wrong!")
End Sub
Call TestParamvsConst(42)

Sub TestDimVsConst
    dim c10
    c10 = 42
    Call ok( c10 = 42, "precedence between const and dim is wrong")
End Sub
Call TestDimVsConst

Sub TestIllegalAssignment
    on error resume next

    ' Assign to Const should give error 501
    err.clear
    c10 = 99
    Call ok(err.number = 501, "assign to const: err.number = " & err.number)
    Call ok(c10 = 10, "c10 = " & c10)

    ' Set on Const should give error 501
    err.clear
    set c10 = Nothing
    Call ok(err.number = 501, "set const: err.number = " & err.number)

    ' Assign to Sub name should give error 501
    err.clear
    TestIllegalAssignment = 10
    Call ok(err.number = 501, "assign to sub name: err.number = " & err.number)
End Sub
Call TestIllegalAssignment

' Assign to function name from outside should give error 501
Function IllegalAssignTarget
    IllegalAssignTarget = 0
End Function
on error resume next
err.clear
IllegalAssignTarget = 10
Call ok(err.number = 501, "assign to func name: err.number = " & err.number)
on error goto 0

Function TestFuncMultiArgs(a,b,c,d,e)
    Call ok(a=1, "a = " & a)
    Call ok(b=2, "b = " & b)
    Call ok(c=3, "c = " & c)
    Call ok(d=4, "d = " & d)
    Call ok(e=5, "e = " & e)
End Function

TestFuncMultiArgs 1, 2, 3, 4, 5
Call TestFuncMultiArgs(1, 2, 3, 4, 5)

Function TestFuncLocalVal
    x = false
    Call ok(not x, "local x is not false?")
    Dim x
End Function

x = true
y = true
Call TestFuncLocalVal
Call ok(x, "global x is not true?")

Function TestFuncExit(ByRef a)
    If a Then
        Exit Function
    End If
    Call ok(false, "Exit Function not called?")
End Function

Call TestFuncExit(true)

Function TestFuncExit2(ByRef a)
    For x = 1 to 100
        For y = 1 to 100
            Exit Function
        Next
    Next
    Call ok(false, "Exit Function not called?")
End Function

Call TestFuncExit2(true)

Sub SubParseTest
End Sub : x = false
Call SubParseTest

Function FuncParseTest
End Function : x = false

Function ReturnTrue
     ReturnTrue = false
     ReturnTrue = true
End Function

Call ok(ReturnTrue(), "ReturnTrue returned false?")

Function SetVal(ByRef x, ByVal v)
    x = v
    SetVal = x
    Exit Function
End Function

x = false
ok SetVal(x, true), "SetVal returned false?"
Call ok(x, "x is not set to true by SetVal?")

Public Function TestPublicFunc
End Function
Call TestPublicFunc

Private Function TestPrivateFunc
End Function
Call TestPrivateFunc

Public Function TestSepFunc(ByVal a) : :
: TestSepFunc = a
End Function
Call ok(TestSepFunc(1) = 1, "Function did not return 1")

ok duplicatedfunc() = 2, "duplicatedfunc = " & duplicatedfunc()

function duplicatedfunc
    ok false, "duplicatedfunc called"
end function

sub duplicatedfunc
    ok false, "duplicatedfunc called"
end sub

function duplicatedfunc
    duplicatedfunc = 2
end function

ok duplicatedfunc() = 2, "duplicatedfunc = " & duplicatedfunc()

' Stop has an effect only in debugging mode
Stop

set x = testObj
Call ok(getVT(x) = "VT_DISPATCH*", "getVT(x=testObj) = " & getVT(x))

Set obj = New EmptyClass
Call ok(getVT(obj) = "VT_DISPATCH*", "getVT(obj) = " & getVT(obj))

Class EmptyClass
End Class

Set x = obj
Call ok(getVT(x) = "VT_DISPATCH*", "getVT(x) = " & getVT(x))

Class TestClass
    Public publicProp
    Public publicArrayProp

    Private privateProp

    Public Function publicFunction()
        privateSub()
        publicFunction = 4
    End Function

    Public Property Get gsProp()
        gsProp = privateProp
        funcCalled = "gsProp get"
        exit property
        Call ok(false, "exit property not returned?")
    End Property

    Public Default Property Get DefValGet
        DefValGet = privateProp
        funcCalled = "GetDefVal"
    End Property

    Public Property Let DefValGet(x)
    End Property

    Public publicProp2

    Public Sub publicSub
    End Sub

    Public Property Let gsProp(val)
        privateProp = val
        funcCalled = "gsProp let"
        exit property
        Call ok(false, "exit property not returned?")
    End Property

    Public Property Set gsProp(val)
        funcCalled = "gsProp set"
        exit property
        Call ok(false, "exit property not returned?")
    End Property

    Public Sub setPrivateProp(x)
        privateProp = x
    End Sub

    Function getPrivateProp
        getPrivateProp = privateProp
    End Function

    Private Sub privateSub
    End Sub

    Public Sub Class_Initialize
        publicProp2 = 2
        ReDim publicArrayProp(2)

        publicArrayProp(0) = 1
        publicArrayProp(1) = 2

        privateProp = true
        Call ok(getVT(privateProp) = "VT_BOOL*", "getVT(privateProp) = " & getVT(privateProp))
        Call ok(getVT(publicProp2) = "VT_I2*", "getVT(publicProp2) = " & getVT(publicProp2))
        Call ok(getVT(Me.publicProp2) = "VT_I2", "getVT(Me.publicProp2) = " & getVT(Me.publicProp2))
    End Sub

    Property Get gsGetProp(x)
        gsGetProp = x
    End Property
End Class

Call testDisp(new testClass)

Set obj = New TestClass

Call ok(obj.publicFunction = 4, "obj.publicFunction = " & obj.publicFunction)
Call ok(obj.publicFunction() = 4, "obj.publicFunction() = " & obj.publicFunction())

obj.publicSub()
Call obj.publicSub
Call obj.publicFunction()

Call ok(getVT(obj.publicProp) = "VT_EMPTY", "getVT(obj.publicProp) = " & getVT(obj.publicProp))
obj.publicProp = 3
Call ok(getVT(obj.publicProp) = "VT_I2", "getVT(obj.publicProp) = " & getVT(obj.publicProp))
Call ok(obj.publicProp = 3, "obj.publicProp = " & obj.publicProp)
obj.publicProp() = 3

Call ok(obj.getPrivateProp() = true, "obj.getPrivateProp() = " & obj.getPrivateProp())
Call obj.setPrivateProp(6)
Call ok(obj.getPrivateProp = 6, "obj.getPrivateProp = " & obj.getPrivateProp)

Dim funcCalled
funcCalled = ""
Call ok(obj.gsProp = 6, "obj.gsProp = " & obj.gsProp)
Call ok(funcCalled = "gsProp get", "funcCalled = " & funcCalled)
obj.gsProp = 3
Call ok(funcCalled = "gsProp let", "funcCalled = " & funcCalled)
Call ok(obj.getPrivateProp = 3, "obj.getPrivateProp = " & obj.getPrivateProp)
Set obj.gsProp = New testclass
Call ok(funcCalled = "gsProp set", "funcCalled = " & funcCalled)

x = obj
Call ok(x = 3, "(x = obj) = " & x)
Call ok(funcCalled = "GetDefVal", "funcCalled = " & funcCalled)
funcCalled = ""
Call ok(obj = 3, "(x = obj) = " & obj)
Call ok(funcCalled = "GetDefVal", "funcCalled = " & funcCalled)

Call obj.Class_Initialize
Call ok(obj.getPrivateProp() = true, "obj.getPrivateProp() = " & obj.getPrivateProp())

'Accessing array property by index
Call ok(obj.publicArrayProp(0) = 1, "obj.publicArrayProp(0) = " & obj.publicArrayProp(0))
Call ok(obj.publicArrayProp(1) = 2, "obj.publicArrayProp(1) = " & obj.publicArrayProp(1))
x = obj.publicArrayProp(2)
Call ok(getVT(x) = "VT_EMPTY*", "getVT(x) = " & getVT(x))
Call ok(obj.publicArrayProp("0") = 1, "obj.publicArrayProp(0) = " & obj.publicArrayProp("0"))

' Indexing a scalar (non-array) public property: native raises 438 for both
' get and set (regardless of arg count or arg type).
On Error Resume Next
Err.Clear
x = obj.publicProp(0)
Call ok(Err.number = 438, "obj.publicProp(0) Err.number = " & Err.number)
Err.Clear
obj.publicProp(0) = 5
Call ok(Err.number = 438, "obj.publicProp(0) = 5 Err.number = " & Err.number)
Err.Clear
x = obj.publicProp(0, 1)
Call ok(Err.number = 438, "obj.publicProp(0, 1) Err.number = " & Err.number)
Err.Clear
obj.publicProp("k") = 5
Call ok(Err.number = 438, "obj.publicProp(""k"") = 5 Err.number = " & Err.number)
Err.Clear
On Error Goto 0

x = (New testclass).publicProp

Class TermTest
    Public Sub Class_Terminate()
        funcCalled = "terminate"
    End Sub
End Class

Set obj = New TermTest
funcCalled = ""
Set obj = Nothing
Call ok(funcCalled = "terminate", "funcCalled = " & funcCalled)

Set obj = New TermTest
funcCalled = ""
Call obj.Class_Terminate
Call ok(funcCalled = "terminate", "funcCalled = " & funcCalled)
funcCalled = ""
Set obj = Nothing
Call ok(funcCalled = "terminate", "funcCalled = " & funcCalled)

Call (New testclass).publicSub()
Call (New testclass).publicSub

class PropTest
    property get prop0()
        prop0 = 1
    end property

    property get prop1(x)
        prop1 = x+1
    end property

    property get prop2(x, y)
        prop2 = x+y
    end property
end class

set obj = new PropTest

call ok(obj.prop0 = 1, "obj.prop0 = " & obj.prop0)
call ok(obj.prop1(3) = 4, "obj.prop1(3) = " & obj.prop1(3))
call ok(obj.prop2(3,4) = 7, "obj.prop2(3,4) = " & obj.prop2(3,4))
call obj.prop0()
call obj.prop1(2)
call obj.prop2(3,4)

x = "following ':' is correct syntax" :
x = "following ':' is correct syntax" :: :
:: x = "also correct syntax"
rem another ugly way for comments
x = "rem as simplestatement" : rem rem comment
:

Set obj = new EmptyClass
Set x = obj
Set y = new EmptyClass

Call ok(obj is x, "obj is not x")
Call ok(x is obj, "x is not obj")
Call ok(not (obj is y), "obj is not y")
Call ok(not obj is y, "obj is not y")
Call ok(not (x is Nothing), "x is 1")
Call ok(Nothing is Nothing, "Nothing is not Nothing")
Call ok(x is obj and true, "x is obj and true is false")

Class TestMe
    Public Sub Test(MyMe)
        Call ok(Me is MyMe, "Me is not MyMe")
    End Sub
End Class

Set obj = New TestMe
Call obj.test(obj)

Class TestMeIndex
    Private arr_(1)
    Public Default Property Get Item(idx)
        Item = arr_(idx)
    End Property
    Public Sub SetVal(idx, val)
        arr_(idx) = val
    End Sub
    Public Sub TestAccess()
        SetVal 0, "hello"
        SetVal 1, "world"
        Call ok(Me(0) = "hello", "Me(0) = " & Me(0))
        Call ok(Me(1) = "world", "Me(1) = " & Me(1))
    End Sub
End Class

Set obj = New TestMeIndex
Call obj.TestAccess()

Call ok(getVT(test) = "VT_DISPATCH", "getVT(test) = " & getVT(test))
Call ok(Me is Test, "Me is not Test")

Const c1 = 1, c2 = 2, c3 = -3
Private Const c4 = 4
Public Const c5 = 5
Call ok(c1 = 1, "c1 = " & c1)
Call ok(getVT(c1) = "VT_I2", "getVT(c1) = " & getVT(c1))
Call ok(c3 = -3, "c3 = " & c3)
Call ok(getVT(c3) = "VT_I2", "getVT(c3) = " & getVT(c3))
Call ok(c4 = 4, "c4 = " & c4)
Call ok(getVT(c4) = "VT_I2", "getVT(c4) = " & getVT(c4))
Call ok(c5 = 5, "c5 = " & c5)
Call ok(getVT(c5) = "VT_I2", "getVT(c5) = " & getVT(c5))

Const cb = True, cs = "test", cnull = null
Call ok(cb, "cb = " & cb)
Call ok(getVT(cb) = "VT_BOOL", "getVT(cb) = " & getVT(cb))
Call ok(cs = "test", "cs = " & cs)
Call ok(getVT(cs) = "VT_BSTR", "getVT(cs) = " & getVT(cs))
Call ok(isNull(cnull), "cnull = " & cnull)
Call ok(getVT(cnull) = "VT_NULL", "getVT(cnull) = " & getVT(cnull))

Call ok(+1 = 1, "+1 != 1")
Call ok(+true = true, "+1 != 1")
Call ok(getVT(+true) = "VT_BOOL", "getVT(+true) = " & getVT(+true))
Call ok(+"true" = "true", """+true"" != true")
Call ok(getVT(+"true") = "VT_BSTR", "getVT(+""true"") = " & getVT(+"true"))
Call ok(+obj is obj, "+obj != obj")
Call ok(+--+-+1 = -1, "+--+-+1 != -1")

if false then Const conststr = "str"
Call ok(conststr = "str", "conststr = " & conststr)
Call ok(getVT(conststr) = "VT_BSTR", "getVT(conststr) = " & getVT(conststr))
Call ok(conststr = "str", "conststr = " & conststr)

Sub ConstTestSub
    Const funcconst = 1
    Call ok(c1 = 1, "c1 = " & c1)
    Call ok(funcconst = 1, "funcconst = " & funcconst)
End Sub

Call ConstTestSub
Dim funcconst

if forward_const = 99 then
    Call ok(true, "forward_const = 99")
else
    Call ok(false, "forward_const <> 99")
end if
Const forward_const = 99

' Property may be used as an identifier (although it's a keyword)
Sub TestProperty
    Dim Property
    PROPERTY = true
    Call ok(property, "property = " & property)

    for property = 1 to 2
    next
End Sub

Call TestProperty

Class Property
    Public Sub Property()
    End Sub

    Sub Test(byref property)
    End Sub
End Class

Class Property2
    Function Property()
    End Function

    Sub Test(property)
    End Sub

    Sub Test2(byval property)
    End Sub
End Class

Class SeparatorTest : : Dim varTest1
:
    Private Sub Class_Initialize : varTest1 = 1
    End Sub ::

    Property Get Test1() :
        Test1 = varTest1
    End Property ::
: :
    Property Let Test1(a) :
        varTest1 = a
    End Property :

    Public Function AddToTest1(ByVal a)  :: :
        varTest1 = varTest1 + a
        AddToTest1 = varTest1
    End Function :    End Class : ::   Set obj = New SeparatorTest

Call ok(obj.Test1 = 1, "obj.Test1 is not 1")
obj.Test1 = 6
Call ok(obj.Test1 = 6, "obj.Test1 is not 6")
obj.AddToTest1(5)
Call ok(obj.Test1 = 11, "obj.Test1 is not 11")

set obj = unkObj
set x = obj
call ok(getVT(obj) = "VT_UNKNOWN*", "getVT(obj) = " & getVT(obj))
call ok(getVT(x) = "VT_UNKNOWN*", "getVT(x) = " & getVT(x))
call ok(getVT(unkObj) = "VT_UNKNOWN", "getVT(unkObj) = " & getVT(unkObj))
call ok(obj is unkObj, "obj is not unkObj")

' Array tests

Call ok(getVT(arr) = "VT_EMPTY*", "getVT(arr) = " & getVT(arr))

Dim arr(3)
Dim arr2(4,3), arr3(5,4,3), arr0(0), noarr()

Call ok(getVT(arr) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(arr) = " & getVT(arr))
Call ok(getVT(arr2) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(arr2) = " & getVT(arr2))
Call ok(getVT(arr0) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(arr0) = " & getVT(arr0))
Call ok(getVT(noarr) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(noarr) = " & getVT(noarr))

Call testArray(1, arr)
Call testArray(2, arr2)
Call testArray(3, arr3)
Call testArray(0, arr0)
Call testArray(-1, noarr)

Call ok(getVT(arr(1)) = "VT_EMPTY*", "getVT(arr(1)) = " & getVT(arr(1)))
Call ok(getVT(arr2(1,2)) = "VT_EMPTY*", "getVT(arr2(1,2)) = " & getVT(arr2(1,2)))
Call ok(getVT(arr3(1,2,2)) = "VT_EMPTY*", "getVT(arr3(1,2,3)) = " & getVT(arr3(1,2,2)))
Call ok(getVT(arr(0)) = "VT_EMPTY*", "getVT(arr(0)) = " & getVT(arr(0)))
Call ok(getVT(arr(3)) = "VT_EMPTY*", "getVT(arr(3)) = " & getVT(arr(3)))
Call ok(getVT(arr0(0)) = "VT_EMPTY*", "getVT(arr0(0)) = " & getVT(arr0(0)))

arr(2) = 3
Call ok(arr(2) = 3, "arr(2) = " & arr(2))
Call ok(getVT(arr(2)) = "VT_I2*", "getVT(arr(2)) = " & getVT(arr(2)))

arr3(3,2,1) = 1
arr3(1,2,3) = 2
Call ok(arr3(3,2,1) = 1, "arr3(3,2,1) = " & arr3(3,2,1))
Call ok(arr3(1,2,3) = 2, "arr3(1,2,3) = " & arr3(1,2,3))
arr2(4,3) = 1
Call ok(arr2(4,3) = 1, "arr2(4,3) = " & arr2(4,3))

x = arr3
Call ok(x(3,2,1) = 1, "x(3,2,1) = " & x(3,2,1))

Function getarr()
    Dim arr(3)
    arr(2) = 2
    getarr = arr
    arr(3) = 3
End Function

x = getarr()
Call ok(getVT(x) = "VT_ARRAY|VT_VARIANT*", "getVT(x) = " & getVT(x))
Call ok(x(2) = 2, "x(2) = " & x(2))
Call ok(getVT(x(3)) = "VT_EMPTY*", "getVT(x(3)) = " & getVT(x(3)))

x(1) = 1
Call ok(x(1) = 1, "x(1) = " & x(1))
x = getarr()
Call ok(getVT(x(1)) = "VT_EMPTY*", "getVT(x(1)) = " & getVT(x(1)))
Call ok(x(2) = 2, "x(2) = " & x(2))

x(1) = 1
y = x
x(1) = 2
Call ok(y(1) = 1, "y(1) = " & y(1))

for x=1 to 1
    Dim forarr(3)
    if x=1 then
        Call ok(getVT(forarr(1)) = "VT_EMPTY*", "getVT(forarr(1)) = " & getVT(forarr(1)))
    else
        Call ok(forarr(1) = x, "forarr(1) = " & forarr(1))
    end if
    forarr(1) = x+1
next

x=1
Call ok(forarr(x) = 2, "forarr(x) = " & forarr(x))

sub accessArr()
    ok arr(1) = 1, "arr(1) = " & arr(1)
    arr(1) = 2
end sub
arr(1) = 1
call accessArr
ok arr(1) = 2, "arr(1) = " & arr(1)

sub accessArr2(x,y)
    ok arr2(x,y) = 1, "arr2(x,y) = " & arr2(x,y)
    x = arr2(x,y)
    arr2(x,y) = 2
end sub
arr2(1,2) = 1
call accessArr2(1, 2)
ok arr2(1,2) = 2, "arr2(1,2) = " & arr2(1,2)

x = Array(Array(3))
call ok(x(0)(0) = 3, "x(0)(0) = " & x(0)(0))

Class ArrayReturnContainer
    Public Default Property Get Item(key)
        If key = "Key" Then
            Item = Array("Value1", Array("SubValue1", "SubValue2"))
        End If
    End Property
End Class

Dim containerObj
Set containerObj = New ArrayReturnContainer
call ok(containerObj.Item("Key")(0) = "Value1", "containerObj.Item(Key)(0) = " & containerObj.Item("Key")(0))
call ok(containerObj.Item("Key")(1)(0) = "SubValue1", "containerObj.Item(Key)(1)(0) = " & containerObj.Item("Key")(1)(0))
call ok(containerObj.Item("Key")(1)(1) = "SubValue2", "containerObj.Item(Key)(1)(1) = " & containerObj.Item("Key")(1)(1))
call ok(containerObj("Key")(0) = "Value1", "containerObj(Key)(0) = " & containerObj("Key")(0))
call ok(containerObj("Key")(1)(0) = "SubValue1", "containerObj(Key)(1)(0) = " & containerObj("Key")(1)(0))

call ok(Split("1;2", ";")(0) = "1", "Split(""1;2"", "";"")(0) = " & Split("1;2", ";")(0))
call ok(Split("1;2", ";")(1) = "2", "Split(""1;2"", "";"")(1) = " & Split("1;2", ";")(1))
Function GetABC()
    GetABC = Array("a", "b", "c")
End Function
call ok(GetABC()(0) = "a", "GetABC()(0) = " & GetABC()(0))
call ok(GetABC()(1) = "b", "GetABC()(1) = " & GetABC()(1))
call ok(GetABC()(2) = "c", "GetABC()(2) = " & GetABC()(2))

Function GetNested()
    GetNested = Array(Array(1, 2), Array(3, 4))
End Function
call ok(GetNested()(0)(0) = 1, "GetNested()(0)(0) = " & GetNested()(0)(0))
call ok(GetNested()(0)(1) = 2, "GetNested()(0)(1) = " & GetNested()(0)(1))
call ok(GetNested()(1)(0) = 3, "GetNested()(1)(0) = " & GetNested()(1)(0))
call ok(GetNested()(1)(1) = 4, "GetNested()(1)(1) = " & GetNested()(1)(1))

x = GetABC()(0) & GetABC()(2)
call ok(x = "ac", "GetABC()(0) & GetABC()(2) = " & x)

call ok(Array(10,20,30)(1) = 20, "Array(10,20,30)(1) = " & Array(10,20,30)(1))

' Chained call with dot accessor: dict.Keys()(0)
Dim chainDict
Set chainDict = CreateObject("Scripting.Dictionary")
chainDict.Add "first", 1
chainDict.Add "second", 2
call ok(chainDict.Keys()(0) = "first", "dict.Keys()(0) = " & chainDict.Keys()(0))
call ok(chainDict.Keys()(1) = "second", "dict.Keys()(1) = " & chainDict.Keys()(1))
call ok(chainDict.Items()(0) = 1, "dict.Items()(0) = " & chainDict.Items()(0))

' Dot accessor after chained index: dict.Items()(0).Item("key")
Dim chainInner
Set chainInner = CreateObject("Scripting.Dictionary")
chainInner.Add "x", 42
Dim chainOuter
Set chainOuter = CreateObject("Scripting.Dictionary")
chainOuter.Add "inner", chainInner
call ok(chainOuter.Items()(0).Item("x") = 42, "dict.Items()(0).Item(""x"") = " & chainOuter.Items()(0).Item("x"))

' Chained call as argument to function: Len(GetABC()(0))
call ok(Len(GetABC()(0)) = 1, "Len(GetABC()(0)) = " & Len(GetABC()(0)))
call ok(UBound(GetABC()) = 2, "UBound(GetABC()) = " & UBound(GetABC()))

' Function call as argument to built-in
Function GetStr()
    GetStr = "hello"
End Function
call ok(Len(GetStr()) = 5, "Len(GetStr()) = " & Len(GetStr()))

function seta0(arr)
    arr(0) = 2
    seta0 = 1
end function

x = Array(1)
seta0 x
ok x(0) = 2, "x(0) = " & x(0)

x = Array(1)
seta0 (x)
ok x(0) = 1, "x(0) = " & x(0)

x = Array(1)
call (((seta0))) ((x))
ok x(0) = 1, "x(0) = " & x(0)

x = Array(1)
call (((seta0))) (x)
ok x(0) = 2, "x(0) = " & x(0)

x = Array(Array(3))
seta0 x(0)
call ok(x(0)(0) = 2, "x(0)(0) = " & x(0)(0))

x = Array(Array(3))
seta0 (x(0))
call ok(x(0)(0) = 3, "x(0)(0) = " & x(0)(0))

x = Array(Array("a", 0))
x(0)(1) = 5
call ok(x(0)(1) = 5, "x(0)(1) = " & x(0)(1))

x = Array(Array(Empty, Empty))
Set x(0)(1) = New EmptyClass
call ok(getVT(x(0)(1)) = "VT_DISPATCH*", "getVT(x(0)(1)) = " & getVT(x(0)(1)))
Set x(0)(0) = Nothing
call ok(x(0)(0) Is Nothing, "x(0)(0) is not Nothing")

On Error Resume Next
x = Array(Nothing)
x(0)(0) = 5
call ok(Err.Number = 13, "assign to Nothing(0): Err.Number = " & Err.Number)
Err.Clear
x = Array(42)
x(0)(0) = 5
call ok(Err.Number = 13, "assign to Integer(0): Err.Number = " & Err.Number)
Err.Clear
x = Array(Empty)
x(0)(0) = 5
call ok(Err.Number = 13, "assign to Empty(0): Err.Number = " & Err.Number)
Err.Clear
x = Array(Null)
x(0)(0) = 5
call ok(Err.Number = 13, "assign to Null(0): Err.Number = " & Err.Number)
On Error GoTo 0

y = (seta0)(x)
ok y = 1, "y = " & y

y = ((x))(0)
ok y = 2, "y = " & y

sub changearg(x)
    x = 2
end sub

x = Array(1)
changearg x(0)
ok x(0) = 2, "x(0) = " & x(0)
ok getVT(x) = "VT_ARRAY|VT_VARIANT*", "getVT(x) after redim = " & getVT(x)

x = Array(1)
changearg (x(0))
ok x(0) = 1, "x(0) = " & x(0)

x = Array(1)
redim x(4)
ok ubound(x) = 4, "ubound(x) = " & ubound(x)
ok x(0) = empty, "x(0) = " & x(0)

x = 1
redim x(3)
ok ubound(x) = 3, "ubound(x) = " & ubound(x)

x(0) = 1
x(1) = 2
x(2) = 3
x(2) = 4

redim preserve x(1)
ok ubound(x) = 1, "ubound(x) = " & ubound(x)
ok x(0) = 1, "x(0) = " & x(1)
ok x(1) = 2, "x(1) = " & x(1)

redim preserve x(2)
ok ubound(x) = 2, "ubound(x) = " & ubound(x)
ok x(0) = 1, "x(0) = " & x(0)
ok x(1) = 2, "x(1) = " & x(1)
ok x(2) = vbEmpty, "x(2) = " & x(2)

on error resume next
redim preserve x(2,2)
e = err.number
on error goto 0
ok e = 9, "e = " & e ' VBSE_OUT_OF_BOUNDS, cannot change cDims

x = Array(1, 2)
redim x(-1)
ok lbound(x) = 0, "lbound(x) = " & lbound(x)
ok ubound(x) = -1, "ubound(x) = " & ubound(x)

redim x(3, 2)
ok ubound(x) = 3, "ubound(x) = " & ubound(x)
ok ubound(x, 1) = 3, "ubound(x, 1) = " & ubound(x, 1)
ok ubound(x, 2) = 2, "ubound(x, 2) = " & ubound(x, 2) & " expected 2"

redim x(1, 3)
x(0,0) = 1.1
x(0,1) = 1.2
x(0,2) = 1.3
x(0,3) = 1.4
x(1,0) = 2.1
x(1,1) = 2.2
x(1,2) = 2.3
x(1,3) = 2.4

redim preserve x(1,1)
ok ubound(x, 1) = 1, "ubound(x, 1) = " & ubound(x, 1)
ok ubound(x, 2) = 1, "ubound(x, 2) = " & ubound(x, 2)
ok x(0,0) = 1.1, "x(0,0) = " & x(0,0)
ok x(0,1) = 1.2, "x(0,1) = " & x(0,1)
ok x(1,0) = 2.1, "x(1,0) = " & x(1,0)
ok x(1,1) = 2.2, "x(1,1) = " & x(1,1)

redim preserve x(1,2)
ok ubound(x, 1) = 1, "ubound(x, 1) = " & ubound(x, 1)
ok ubound(x, 2) = 2, "ubound(x, 2) = " & ubound(x, 2)
ok x(0,0) = 1.1, "x(0,0) = " & x(0,0)
ok x(0,1) = 1.2, "x(0,1) = " & x(0,1)
ok x(1,0) = 2.1, "x(1,0) = " & x(1,0)
ok x(1,1) = 2.2, "x(1,1) = " & x(1,1)
ok x(0,2) = vbEmpty, "x(0,2) = " & x(0,2)
ok x(1,2) = vbEmpty, "x(1,2) = " & x(1,1)

on error resume next
redim preserve x(2,2)
e = err.number
on error goto 0
ok e = 9, "e = " & e ' VBSE_OUT_OF_BOUNDS, can only change rightmost dimension

redim undeclared_array(3)
ok ubound(undeclared_array) = 3, "ubound(undeclared_array) = " & ubound(undeclared_array)
undeclared_array(3) = 10
ok undeclared_array(3) = 10, "undeclared_array(3) = " & undeclared_array(3)

sub TestReDimFixed
    on error resume next

    dim staticarray(4)
    err.clear
    redim staticarray(3)
    call ok(err.number = 10, "err.number = " & err.number)
    call ok(isArrayFixed(staticarray), "Expected fixed size array")

    err.clear
    redim staticarray("abc")
    call ok(err.number = 10, "err.number = " & err.number)

    dim staticarray2(4)
    err.clear
    redim preserve staticarray2(5)
    call ok(err.number = 10, "err.number = " & err.number)
    call ok(isArrayFixed(staticarray2), "Expected fixed size array")

    err.clear
    redim preserve staticarray2("abc")
    ' Win10+ builds return INVALID_CALL (5)
    call ok(err.number = 5 or err.number = 13, "err.number = " & err.number)
end sub
Call TestRedimFixed

sub TestRedimInputArg
    on error resume next

    dim x

    x = Array(1)
    err.clear
    redim x("abc")
    call ok(err.number = 13, "err.number = " & err.number)

    err.clear
    redim preserve x("abc")
    ' Win10+ builds return INVALID_CALL (5)
    call ok(err.number = 5 or err.number = 13, "err.number = " & err.number)
end sub
Call TestRedimInputArg

sub TestReDimList
    dim x, y

    x = Array(1)
    y = Array(1)
    redim x(1, 3), y(2)
    ok ubound(x, 1) = 1, "ubound(x, 1) = " & ubound(x, 1)
    ok ubound(x, 2) = 3, "ubound(x, 2) = " & ubound(x, 2)
    ok ubound(y, 1) = 2, "ubound(y, 1) = " & ubound(y, 1)

    x(0,0) = 1.1
    x(0,1) = 1.2
    x(0,2) = 1.3
    x(0,3) = 1.4
    x(1,0) = 2.1
    x(1,1) = 2.2
    x(1,2) = 2.3
    x(1,3) = 2.4

    y(0) = 2.1
    y(1) = 2.2
    y(2) = 2.3

    redim preserve x(1,1), y(3)
    ok ubound(x, 1) = 1, "ubound(x, 1) = " & ubound(x, 1)
    ok ubound(x, 2) = 1, "ubound(x, 2) = " & ubound(x, 2)
    ok x(0,0) = 1.1, "x(0,0) = " & x(0,0)
    ok x(0,1) = 1.2, "x(0,1) = " & x(0,1)
    ok x(1,0) = 2.1, "x(1,0) = " & x(1,0)
    ok x(1,1) = 2.2, "x(1,1) = " & x(1,1)
    ok ubound(y, 1) = 3, "ubound(y, 1) = " & ubound(y, 1)
    ok y(0) = 2.1, "y(0) = " & y(0)
    ok y(1) = 2.2, "y(1) = " & y(1)
    ok y(2) = 2.3, "y(2) = " & y(2)
    ok y(3) = vbEmpty, "y(3) = " & y(3)
end sub
call TestReDimList

dim rx
redim rx(4)
sub TestReDimByRef(byref x)
   ok ubound(x) = 4, "ubound(x) = " & ubound(x)
   redim x(6)
   ok ubound(x) = 6, "ubound(x) = " & ubound(x)
end sub
call TestReDimByRef(rx)
ok ubound(rx) = 6, "ubound(rx) = " & ubound(rx)

redim rx(5)
rx(3)=2
sub TestReDimPreserveByRef(byref x)
   ok ubound(x) = 5, "ubound(x) = " & ubound(x)
   ok x(3) = 2, "x(3) = " & x(3)
   redim preserve x(7)
   ok ubound(x) = 7, "ubound(x) = " & ubound(x)
   ok x(3) = 2, "x(3) = " & x(3)
end sub
call TestReDimPreserveByRef(rx)
ok ubound(rx) = 7, "ubound(rx) = " & ubound(rx)
ok rx(3) = 2, "rx(3) = " & rx(3)

' ReDim on an uninitialized dynamic array (Dim arr() has a NULL SAFEARRAY pointer)
dim dynarr()
redim dynarr(3)
ok ubound(dynarr) = 3, "ubound(dynarr) = " & ubound(dynarr)
dynarr(0) = "a"
dynarr(3) = "b"
ok dynarr(0) = "a", "dynarr(0) = " & dynarr(0)
ok dynarr(3) = "b", "dynarr(3) = " & dynarr(3)
redim dynarr(5)
ok ubound(dynarr) = 5, "ubound(dynarr) = " & ubound(dynarr)
ok dynarr(0) = empty, "dynarr(0) after redim = " & dynarr(0)

' ReDim Preserve on an uninitialized dynamic array should also work and retain data
dim dynarr2()
redim preserve dynarr2(3)
ok ubound(dynarr2) = 3, "ubound(dynarr2) = " & ubound(dynarr2)
dynarr2(0) = "x"
redim preserve dynarr2(5)
ok ubound(dynarr2) = 5, "ubound(dynarr2) = " & ubound(dynarr2)
ok dynarr2(0) = "x", "dynarr2(0) after redim preserve = " & dynarr2(0)

' Array dimension mismatch: should give error 9 (Subscript out of range)
dim dimArr2d(3, 3)
dimArr2d(0, 0) = "hello"
on error resume next

' 2D array accessed with 1 index
err.clear
y = dimArr2d(0)
ok err.number = 9, "2D array 1 index: err.number = " & err.number

' 1D array accessed with 2 indices
dim dimArr1d(3)
err.clear
y = dimArr1d(0, 0)
ok err.number = 9, "1D array 2 indices: err.number = " & err.number

' 2D array accessed with 3 indices
err.clear
y = dimArr2d(0, 0, 0)
ok err.number = 9, "2D array 3 indices: err.number = " & err.number

' Assign to 2D array with 1 index
err.clear
dimArr2d(0) = "test"
ok err.number = 9, "assign 2D array 1 index: err.number = " & err.number

' Assign to 1D array with 2 indices
err.clear
dimArr1d(0, 0) = "test"
ok err.number = 9, "assign 1D array 2 indices: err.number = " & err.number

' Uninitialized dynamic array access
dim dimDynArr()
err.clear
y = dimDynArr(0)
ok err.number = 9, "uninitialized dynamic array access: err.number = " & err.number
on error goto 0

' Erase on fixed-size array: clears elements to default values
dim eraseFixed(3)
eraseFixed(0) = "a"
eraseFixed(1) = 2
eraseFixed(2) = True
Erase eraseFixed
ok eraseFixed(0) = empty, "eraseFixed(0) after Erase = " & eraseFixed(0)
ok eraseFixed(1) = empty, "eraseFixed(1) after Erase = " & eraseFixed(1)
ok eraseFixed(2) = empty, "eraseFixed(2) after Erase = " & eraseFixed(2)
ok ubound(eraseFixed) = 3, "ubound(eraseFixed) after Erase = " & ubound(eraseFixed)

' Erase on dynamic array: deallocates the array
dim eraseDyn()
redim eraseDyn(3)
eraseDyn(0) = "x"
Erase eraseDyn
on error resume next
y = eraseDyn(0)
e = err.number
on error goto 0
ok e = 9, "access after Erase dynamic: err.number = " & e

' Erase with multiple arrays (separate statements)
dim eraseMulti1(2), eraseMulti2(2)
eraseMulti1(0) = "a"
eraseMulti2(0) = "b"
Erase eraseMulti1
Erase eraseMulti2
ok eraseMulti1(0) = empty, "eraseMulti1(0) after Erase = " & eraseMulti1(0)
ok eraseMulti2(0) = empty, "eraseMulti2(0) after Erase = " & eraseMulti2(0)

' Erase on non-array variable: should give type mismatch error
dim eraseNotArray
eraseNotArray = 42
on error resume next
Erase eraseNotArray
ok err.number = 13, "Erase non-array: err.number = " & err.number
err.clear

' Erase on Empty variable: should give type mismatch error
dim eraseEmpty
Erase eraseEmpty
ok err.number = 13, "Erase empty var: err.number = " & err.number
err.clear

' Erase on Null variable: should give type mismatch error
dim eraseNull
eraseNull = Null
Erase eraseNull
ok err.number = 13, "Erase null var: err.number = " & err.number
err.clear

' Erase on Object variable: should give type mismatch error
dim eraseObj
set eraseObj = new regexp
Erase eraseObj
ok err.number = 13, "Erase object var: err.number = " & err.number
err.clear

' Erase on Nothing variable: should give type mismatch error
dim eraseNothing
set eraseNothing = Nothing
Erase eraseNothing
ok err.number = 13, "Erase nothing var: err.number = " & err.number
err.clear

' Erase on undeclared identifier: error 500 (Variable is undefined) with Option Explicit
Erase eraseUndeclared
ok err.number = 500, "Erase undeclared: err.number = " & err.number
err.clear
on error goto 0

' Erase on uninitialized dynamic array: no error
dim eraseUninit()
Erase eraseUninit

' Erase twice on fixed-size array: no error
dim eraseTwice(2)
eraseTwice(0) = "x"
Erase eraseTwice
Erase eraseTwice
ok eraseTwice(0) = empty, "eraseTwice(0) after double Erase = " & eraseTwice(0)

' Erase twice on dynamic array: no error
dim eraseTwiceDyn()
redim eraseTwiceDyn(2)
eraseTwiceDyn(0) = "y"
Erase eraseTwiceDyn
Erase eraseTwiceDyn

' ReDim after Erase on dynamic array
dim eraseReDim()
redim eraseReDim(3)
eraseReDim(0) = "before"
Erase eraseReDim
redim eraseReDim(5)
ok ubound(eraseReDim) = 5, "ubound(eraseReDim) after Erase+ReDim = " & ubound(eraseReDim)
ok eraseReDim(0) = empty, "eraseReDim(0) after Erase+ReDim = " & eraseReDim(0)

' Erase on ByRef array parameter: clears the original
sub TestEraseByRef(ByRef arr)
    Erase arr
end sub
dim eraseByRefArr(2)
eraseByRefArr(0) = "hello"
call TestEraseByRef(eraseByRefArr)
ok eraseByRefArr(0) = empty, "eraseByRefArr(0) after ByRef Erase = " & eraseByRefArr(0)
ok ubound(eraseByRefArr) = 2, "ubound(eraseByRefArr) after ByRef Erase = " & ubound(eraseByRefArr)

' Erase on ByVal array parameter: does not affect the original
sub TestEraseByVal(ByVal arr)
    Erase arr
end sub
dim eraseByValArr(2)
eraseByValArr(0) = "world"
call TestEraseByVal(eraseByValArr)
ok eraseByValArr(0) = "world", "eraseByValArr(0) after ByVal Erase = " & eraseByValArr(0)

Class ArrClass
    Dim classarr(3)
    Dim classnoarr()
    Dim var

    Private Sub Class_Initialize
        Call ok(getVT(classarr) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(classarr) = " & getVT(classarr))
        Call testArray(-1, classnoarr)
        classarr(0) = 1
        classarr(1) = 2
        classarr(2) = 3
        classarr(3) = 4
    End Sub

    Public Sub testVarVT
        Call ok(getVT(var) = "VT_ARRAY|VT_VARIANT*", "getVT(var) = " & getVT(var))
    End Sub
End Class

Set obj = new ArrClass
Call ok(getVT(obj.classarr) = "VT_ARRAY|VT_VARIANT", "getVT(obj.classarr) = " & getVT(obj.classarr))
Call ok(obj.classarr(1) = 2, "obj.classarr(1) = " & obj.classarr(1))

obj.var = arr
Call ok(getVT(obj.var) = "VT_ARRAY|VT_VARIANT", "getVT(obj.var) = " & getVT(obj.var))
Call obj.testVarVT

Sub arrarg(byref refarr, byval valarr, byref refarr2, byval valarr2)
    Call ok(getVT(refarr) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(refarr) = " & getVT(refarr))
    Call ok(getVT(valarr) = "VT_ARRAY|VT_VARIANT*", "getVT(valarr) = " & getVT(valarr))
    Call ok(getVT(refarr2) = "VT_ARRAY|VT_VARIANT*", "getVT(refarr2) = " & getVT(refarr2))
    Call ok(getVT(valarr2) = "VT_ARRAY|VT_VARIANT*", "getVT(valarr2) = " & getVT(valarr2))
End Sub

Call arrarg(arr, arr, obj.classarr, obj.classarr)

Sub arrarg2(byref refarr(), byval valarr(), byref refarr2(), byval valarr2())
    Call ok(getVT(refarr) = "VT_ARRAY|VT_BYREF|VT_VARIANT*", "getVT(refarr) = " & getVT(refarr))
    Call ok(getVT(valarr) = "VT_ARRAY|VT_VARIANT*", "getVT(valarr) = " & getVT(valarr))
    Call ok(getVT(refarr2) = "VT_ARRAY|VT_VARIANT*", "getVT(refarr2) = " & getVT(refarr2))
    Call ok(getVT(valarr2) = "VT_ARRAY|VT_VARIANT*", "getVT(valarr2) = " & getVT(valarr2))
End Sub

Call arrarg2(arr, arr, obj.classarr, obj.classarr)

Sub testarrarg(arg(), vt)
    Call ok(getVT(arg) = vt, "getVT() = " & getVT(arg) & " expected " & vt)
End Sub

Call testarrarg(1, "VT_I2*")
Call testarrarg(false, "VT_BOOL*")
Call testarrarg(Empty, "VT_EMPTY*")

Sub modifyarr(arr)
    Call ok(arr(0) = "not modified", "arr(0) = " & arr(0))
    arr(0) = "modified"
End Sub

arr(0) = "not modified"
Call modifyarr(arr)
Call ok(arr(0) = "modified", "arr(0) = " & arr(0))

arr(0) = "not modified"
modifyarr(arr)
Call ok(arr(0) = "not modified", "arr(0) = " & arr(0))

for x = 0 to UBound(arr)
    arr(x) = x
next
y = 0
for each x in arr
    Call ok(x = y, "x = " & x & ", expected " & y)
    Call ok(arr(y) = y, "arr(" & y & ") = " & arr(y))
    arr(y) = 1
    x = 1
    y = y+1
next
Call ok(y = 4, "y = " & y & " after array enumeration")

for x=0 to UBound(arr2, 1)
    for y=0 to UBound(arr2, 2)
        arr2(x, y) = x + y*(UBound(arr2, 1)+1)
    next
next
y = 0
for each x in arr2
    Call ok(x = y, "x = " & x & ", expected " & y)
    y = y+1
next
Call ok(y = 20, "y = " & y & " after array enumeration")

for each x in noarr
    Call ok(false, "Empty array contains: " & x)
next

' Indexing non-array variables should give type mismatch (error 13)
sub test_index_non_array
    dim tmp
    on error resume next

    ' indexed assign
    x = 42
    err.clear
    x(0) = 1
    call ok(err.number = 13, "assign int(0): err.number = " & err.number)

    x = "hello"
    err.clear
    x(0) = 1
    call ok(err.number = 13, "assign str(0): err.number = " & err.number)

    x = True
    err.clear
    x(0) = 1
    call ok(err.number = 13, "assign bool(0): err.number = " & err.number)

    x = Empty
    err.clear
    x(0) = 1
    call ok(err.number = 13, "assign empty(0): err.number = " & err.number)

    x = Null
    err.clear
    x(0) = 1
    call ok(err.number = 13, "assign null(0): err.number = " & err.number)

    x = 3.14
    err.clear
    x(0) = 1
    call ok(err.number = 13, "assign double(0): err.number = " & err.number)

    x = Now
    err.clear
    x(0) = 1
    call ok(err.number = 13, "assign date(0): err.number = " & err.number)

    ' indexed read
    x = 42
    err.clear
    tmp = x(0)
    call ok(err.number = 13, "read int(0): err.number = " & err.number)

    x = "hello"
    err.clear
    tmp = x(0)
    call ok(err.number = 13, "read str(0): err.number = " & err.number)

    x = True
    err.clear
    tmp = x(0)
    call ok(err.number = 13, "read bool(0): err.number = " & err.number)

    x = Empty
    err.clear
    tmp = x(0)
    call ok(err.number = 13, "read empty(0): err.number = " & err.number)

    x = Null
    err.clear
    tmp = x(0)
    call ok(err.number = 13, "read null(0): err.number = " & err.number)

    x = 3.14
    err.clear
    tmp = x(0)
    call ok(err.number = 13, "read double(0): err.number = " & err.number)

    x = Now
    err.clear
    tmp = x(0)
    call ok(err.number = 13, "read date(0): err.number = " & err.number)

    ' multi-index on non-array
    x = 42
    err.clear
    tmp = x(0, 1)
    call ok(err.number = 13, "read int(0,1): err.number = " & err.number)

    err.clear
    x(0, 1) = 1
    call ok(err.number = 13, "assign int(0,1): err.number = " & err.number)

    on error goto 0
end sub
call test_index_non_array

' It's allowed to declare non-builtin RegExp class...
class RegExp
     public property get Global()
         Call ok(false, "Global called")
         Global = "fail"
     end property
end class

' ...but there is no way to use it because builtin instance is always created
set x = new RegExp
Call ok(x.Global = false, "x.Global = " & x.Global)

sub test_nothing_errors
    dim x
    on error resume next

    x = 1
    err.clear
    x = nothing
    call ok(err.number = 91, "err.number = " & err.number)
    call ok(x = 1, "x = " & x)

    err.clear
    x = not nothing
    call ok(err.number = 91, "err.number = " & err.number)
    call ok(x = 1, "x = " & x)

    err.clear
    x = "" & nothing
    call ok(err.number = 91, "err.number = " & err.number)
    call ok(x = 1, "x = " & x)
end sub
call test_nothing_errors()

sub test_identifiers
    ' test keywords that can also be a declared identifier
    Dim default
    default = "xx"
    Call ok(default = "xx", "default = " & default & " expected ""xx""")

    Dim error
    error = "xx"
    Call ok(error = "xx", "error = " & error & " expected ""xx""")

    Dim explicit
    explicit = "xx"
    Call ok(explicit = "xx", "explicit = " & explicit & " expected ""xx""")

    Dim step
    step = "xx"
    Call ok(step = "xx", "step = " & step & " expected ""xx""")

    Dim property
    property = "xx"
    Call ok(property = "xx", "property = " & property & " expected ""xx""")
end sub
call test_identifiers()

Class class_test_identifiers_as_function_name
    Sub Property ( par )
    End Sub

    Function Error( par )
    End Function

    Sub Default ()
    End Sub

    Function Explicit (par)
        Explicit = par
    End Function

    Sub Step ( default )
    End Sub
End Class

Class class_test_identifiers_as_property_name
    Public Property Get Property()
    End Property

    Public Property Let Error(par)
        Error = par
    End Property

    Public Property Set Default(par)
        Set Default = par
    End Property
End Class

' Local Dim inside a class method should not conflict with a global Function of the same name
Function GlobalShadowFunc()
    GlobalShadowFunc = 42
End Function

Class TestLocalDimShadowsGlobalFunc
    Public Sub TestShadow()
        Dim GlobalShadowFunc
        GlobalShadowFunc = 10
        Call ok(GlobalShadowFunc = 10, "local Dim should shadow global Function: " & GlobalShadowFunc)
    End Sub

    Private Function ClassPrivateFunc()
        ClassPrivateFunc = 99
    End Function

    Public Sub TestPrivate()
        Call ok(ClassPrivateFunc() = 99, "private class function should be callable: " & ClassPrivateFunc())
    End Sub
End Class

' Global function with same name as private class function
Function ClassPrivateFunc()
    ClassPrivateFunc = 1
End Function

Dim objShadow : Set objShadow = New TestLocalDimShadowsGlobalFunc
objShadow.TestShadow
objShadow.TestPrivate

sub test_dotIdentifiers
    ' test keywords that can also be an identifier after a dot
    Call ok(testObj.rem = 10, "testObj.rem = " & testObj.rem & " expected 10")
    Call ok(testObj.true = 10, "testObj.true = " & testObj.true & " expected 10")
    Call ok(testObj.false = 10, "testObj.false = " & testObj.false & " expected 10")
    Call ok(testObj.not = 10, "testObj.not = " & testObj.not & " expected 10")
    Call ok(testObj.and = 10, "testObj.and = " & testObj.and & " expected 10")
    Call ok(testObj.or = 10, "testObj.or = " & testObj.or & " expected 10")
    Call ok(testObj.xor = 10, "testObj.xor = " & testObj.xor & " expected 10")
    Call ok(testObj.eqv = 10, "testObj.eqv = " & testObj.eqv & " expected 10")
    Call ok(testObj.imp = 10, "testObj.imp = " & testObj.imp & " expected 10")
    Call ok(testObj.is = 10, "testObj.is = " & testObj.is & " expected 10")
    Call ok(testObj.mod = 10, "testObj.mod = " & testObj.mod & " expected 10")
    Call ok(testObj.call = 10, "testObj.call = " & testObj.call & " expected 10")
    Call ok(testObj.dim = 10, "testObj.dim = " & testObj.dim & " expected 10")
    Call ok(testObj.sub = 10, "testObj.sub = " & testObj.sub & " expected 10")
    Call ok(testObj.function = 10, "testObj.function = " & testObj.function & " expected 10")
    Call ok(testObj.get = 10, "testObj.get = " & testObj.get & " expected 10")
    Call ok(testObj.let = 10, "testObj.let = " & testObj.let & " expected 10")
    Call ok(testObj.const = 10, "testObj.const = " & testObj.const & " expected 10")
    Call ok(testObj.if = 10, "testObj.if = " & testObj.if & " expected 10")
    Call ok(testObj.else = 10, "testObj.else = " & testObj.else & " expected 10")
    Call ok(testObj.elseif = 10, "testObj.elseif = " & testObj.elseif & " expected 10")
    Call ok(testObj.end = 10, "testObj.end = " & testObj.end & " expected 10")
    Call ok(testObj.then = 10, "testObj.then = " & testObj.then & " expected 10")
    Call ok(testObj.exit = 10, "testObj.exit = " & testObj.exit & " expected 10")
    Call ok(testObj.while = 10, "testObj.while = " & testObj.while & " expected 10")
    Call ok(testObj.wend = 10, "testObj.wend = " & testObj.wend & " expected 10")
    Call ok(testObj.do = 10, "testObj.do = " & testObj.do & " expected 10")
    Call ok(testObj.loop = 10, "testObj.loop = " & testObj.loop & " expected 10")
    Call ok(testObj.until = 10, "testObj.until = " & testObj.until & " expected 10")
    Call ok(testObj.for = 10, "testObj.for = " & testObj.for & " expected 10")
    Call ok(testObj.to = 10, "testObj.to = " & testObj.to & " expected 10")
    Call ok(testObj.each = 10, "testObj.each = " & testObj.each & " expected 10")
    Call ok(testObj.in = 10, "testObj.in = " & testObj.in & " expected 10")
    Call ok(testObj.select = 10, "testObj.select = " & testObj.select & " expected 10")
    Call ok(testObj.case = 10, "testObj.case = " & testObj.case & " expected 10")
    Call ok(testObj.byref = 10, "testObj.byref = " & testObj.byref & " expected 10")
    Call ok(testObj.byval = 10, "testObj.byval = " & testObj.byval & " expected 10")
    Call ok(testObj.option = 10, "testObj.option = " & testObj.option & " expected 10")
    Call ok(testObj.nothing = 10, "testObj.nothing = " & testObj.nothing & " expected 10")
    Call ok(testObj.empty = 10, "testObj.empty = " & testObj.empty & " expected 10")
    Call ok(testObj.null = 10, "testObj.null = " & testObj.null & " expected 10")
    Call ok(testObj.class = 10, "testObj.class = " & testObj.class & " expected 10")
    Call ok(testObj.set = 10, "testObj.set = " & testObj.set & " expected 10")
    Call ok(testObj.new = 10, "testObj.new = " & testObj.new & " expected 10")
    Call ok(testObj.public = 10, "testObj.public = " & testObj.public & " expected 10")
    Call ok(testObj.private = 10, "testObj.private = " & testObj.private & " expected 10")
    Call ok(testObj.next = 10, "testObj.next = " & testObj.next & " expected 10")
    Call ok(testObj.on = 10, "testObj.on = " & testObj.on & " expected 10")
    Call ok(testObj.resume = 10, "testObj.resume = " & testObj.resume & " expected 10")
    Call ok(testObj.goto = 10, "testObj.goto = " & testObj.goto & " expected 10")
    Call ok(testObj.with = 10, "testObj.with = " & testObj.with & " expected 10")
    Call ok(testObj.redim = 10, "testObj.redim = " & testObj.redim & " expected 10")
    Call ok(testObj.preserve = 10, "testObj.preserve = " & testObj.preserve & " expected 10")
    Call ok(testObj.property = 10, "testObj.property = " & testObj.property & " expected 10")
    Call ok(testObj.me = 10, "testObj.me = " & testObj.me & " expected 10")
    Call ok(testObj.stop = 10, "testObj.stop = " & testObj.stop & " expected 10")
end sub
call test_dotIdentifiers

' Test End statements not required to be preceded by a newline or separator
Sub EndTestSub
    x = 1 End Sub

Sub EndTestSubWithCall
    x = 1
    Call ok(x = 1, "x = " & x)End Sub
Call EndTestSubWithCall()

Function EndTestFunc(x)
    Call ok(x > 0, "x = " & x)End Function
EndTestFunc(1)

Class EndTestClassWithStorageId
    Public x End Class

Class EndTestClassWithDim
    Dim x End Class

Class EndTestClassWithFunc
    Function test(ByVal x)
        x = 0 End Function End Class

Class EndTestClassWithProperty
    Public x
    Public default Property Get defprop
        defprop = x End Property End Class

class TestPropSyntax
    public prop

    function getProp()
        set getProp = prop
    end function

    public default property get def()
        def = ""
    end property
end class

Class TestPropParam
    Public oDict
    Public gotNothing
    Public m_obj
    Public m_objType

    Public Property Set bar(obj)
        Set m_obj = obj
    End Property
    Public Property Set foo(par,obj)
        Set m_obj = obj
        if obj is Nothing Then gotNothing = True
        oDict = par
    End Property
    Public Property Let Key(oldKey,newKey)
        oDict = oldKey & newKey
    End Property
    Public Property Let three(uno,due,tre)
        oDict = uno & due & tre
    End Property
    Public Property Let ten(a,b,c,d,e,f,g,h,i,j)
        oDict = a & b & c & d & e & f & g & h & i & j
    End Property
    Public Property Let objProp(aInput)
        m_objType = getVT(aInput)
        If IsObject(aInput) Then
            Set m_obj = aInput
        Else
            m_obj = aInput
        End If
    End Property
End Class

Set x = new TestPropParam
x.key("old") = "new"
call ok(x.oDict = "oldnew","x.oDict = " & x.oDict & " expected oldnew")
x.three(1,2) = 3
call ok(x.oDict = "123","x.oDict = " & x.oDict & " expected 123")
x.ten(1,2,3,4,5,6,7,8,9) = 0
call ok(x.oDict = "1234567890","x.oDict = " & x.oDict & " expected 1234567890")
Set x.bar = Nothing
call ok(x.gotNothing=Empty,"x.gotNothing = " & x.gotNothing  & " expected Empty")
Set x.foo("123") = Nothing
call ok(x.oDict = "123","x.oDict = " & x.oDict & " expected 123")
call ok(x.gotNothing=True,"x.gotNothing = " & x.gotNothing  & " expected true")

' Property Let receives VT_DISPATCH argument as-is (does not extract default value)
Set y = New EndTestClassWithProperty
y.x = 42
x.objProp = y
call ok(x.m_objType = "VT_DISPATCH*", "Property Let aInput type: " & x.m_objType & " expected VT_DISPATCH*")
call ok(x.m_obj = 42, "Property Let with object argument failed, m_obj = " & x.m_obj)

' Property Let receives object without default property as VT_DISPATCH
Set z = New EmptyClass
x.objProp = z
call ok(x.m_objType = "VT_DISPATCH*", "Property Let no-default aInput type: " & x.m_objType & " expected VT_DISPATCH*")

' Set with only Property Let defined should fail (no fallback to Let)
On Error Resume Next
Set x.objProp = y
call ok(Err.Number = 438, "Set Property Let only: Err.Number = " & Err.Number & " expected 438")
On Error GoTo 0

' Wrong number of arguments error (450)
Sub ArityTestSub(a, b)
End Sub

Function ArityTestFunc(a)
    ArityTestFunc = a
End Function

On Error Resume Next

Err.Clear
ArityTestSub 1
Call ok(Err.Number = 450, "too few args sub: err = " & Err.Number)

Err.Clear
ArityTestSub 1, 2, 3
Call ok(Err.Number = 450, "too many args sub: err = " & Err.Number)

Err.Clear
Dim arityResult
arityResult = ArityTestFunc()
Call ok(Err.Number = 450, "too few args func: err = " & Err.Number)

Err.Clear
arityResult = ArityTestFunc(1, 2)
Call ok(Err.Number = 450, "too many args func: err = " & Err.Number)

On Error GoTo 0

set x = new TestPropSyntax
set x.prop = new TestPropSyntax
set x.prop.prop = new TestPropSyntax
x.prop.prop.prop = 2
call ok(x.getProp().getProp.prop = 2, "x.getProp().getProp.prop = " & x.getProp().getProp.prop)
x.getprop.getprop().prop = 3
call ok(x.getProp.prop.prop = 3, "x.getProp.prop.prop = " & x.getProp.prop.prop)
set x.getprop.getprop().prop = new emptyclass
set obj = new emptyclass
set x.getprop.getprop().prop = obj
call ok(x.getprop.getprop().prop is obj, "x.getprop.getprop().prop is not obj (emptyclass)")

ok getVT(x) = "VT_DISPATCH*", "getVT(x) = " & getVT(x)
ok getVT(x()) = "VT_BSTR", "getVT(x()) = " & getVT(x())

Class TestClassVariablesMulti
    Public pub1, pub2
    Public pubArray(3), pubArray2(5, 10)
    Private priv1, priv2
    Private error, explicit, step
    Dim dim1, dim2

    Private Sub Class_Initialize()
	pub1 = 1
        pub2 = 2
        pubArray(0) = 3
        pubArray2(0, 0) = 4
        priv1 = 5
        priv2 = 6
        dim1 = 7
        dim2 = 8
        error = 9
        explicit = 10
        step = 11
    End Sub
End Class

Set x = new TestClassVariablesMulti
call ok(x.pub1 = 1, "x.pub1 = " & x.pub1)
call ok(x.pub2 = 2, "x.pub2 = " & x.pub2)
call ok(ubound(x.pubArray) = 3, "ubound(x.pubArray) = " & ubound(x.pubArray))
call ok(ubound(x.pubArray2, 1) = 5, "ubound(x.pubArray2, 1) = " & ubound(x.pubArray2, 1))
call ok(ubound(x.pubArray2, 2) = 10, "ubound(x.pubArray2, 2) = " & ubound(x.pubArray2, 2))
call ok(x.pubArray(0) = 3, "x.pubArray(0) = " & x.pubArray(0))
call ok(x.dim1 = 7, "x.dim1 = " & x.dim1)
call ok(x.dim2 = 8, "x.dim2 = " & x.dim2)

on error resume next
x.priv1 = 1
call ok(err.number = 438, "err.number = " & err.number)
err.clear
x.priv2 = 2
call ok(err.number = 438, "err.number = " & err.number)
err.clear
x.pubArray(0) = 1
call ok(err.number = 0, "set x.pubArray(0) err.number = " & err.number)
call ok(x.pubArray(0) = 1, "x.pubArray(0) = " & x.pubArray(0))
on error goto 0

funcCalled = ""
class DefaultSubTest1
 Public  default Sub init(a)
    funcCalled = "init" & a
 end sub
end class

Set obj = New DefaultSubTest1
obj.init(1)
call ok(funcCalled = "init1","funcCalled=" & funcCalled)
funcCalled = ""
obj(2)
call ok(funcCalled = "init2","funcCalled=" & funcCalled)

class DefaultSubTest2
 Public Default Function init
    funcCalled = "init"
 end function
end class

Set obj = New DefaultSubTest2
funcCalled = ""
obj.init()
call ok(funcCalled = "init","funcCalled=" & funcCalled)
funcCalled = ""
' todo this is not yet supported
'funcCalled = ""
'obj()
'call ok(funcCalled = "init","funcCalled=" & funcCalled)

with nothing
end with

set x = new TestPropSyntax
with x
     .prop = 1
     ok .prop = 1, ".prop = "&.prop
end with
ok x.prop = 1, "x.prop = " & x.prop

with new TestPropSyntax
     .prop = 1
     ok .prop = 1, ".prop = "&.prop
end with

function testsetresult(x, y)
    set testsetresult = new TestPropSyntax
    testsetresult.prop = x
    y = testsetresult.prop + 1
end function

set x = testsetresult(1, 2)
ok x.prop = 1, "x.prop = " & x.prop

set arr(0) = new TestPropSyntax
arr(0).prop = 1
ok arr(0).prop = 1, "arr(0) = " & arr(0).prop

function recursingfunction(x)
    if (x) then exit function
    recursingfunction = 2
    dim y
    y = recursingfunction
    call ok(y = 2, "y = " & y)
    recursingfunction = 1
    call recursingfunction(True)
end function
call ok(recursingfunction(False) = 1, "unexpected return value " & recursingfunction(False))

x = false
function recursingfunction2
    if (x) then exit function
    recursingfunction2 = 2
    dim y
    y = recursingfunction2
    call ok(y = 2, "y = " & y)
    recursingfunction2 = 1
    x = true
    recursingfunction2()
end function
call ok(recursingfunction2() = 1, "unexpected return value " & recursingfunction2())

Dim recursionDepth
recursionDepth = 0
Sub RecurseN(n)
    recursionDepth = recursionDepth + 1
    If n > 1 Then RecurseN n - 1
End Sub
RecurseN 100
Call ok(recursionDepth = 100, "recursion depth: " & recursionDepth & " expected 100")

Sub RecurseForever()
    RecurseForever
End Sub

Sub MutualA()
    MutualB
End Sub
Sub MutualB()
    MutualA
End Sub

On Error Resume Next
Err.Clear
MutualA
Call ok(Err.Number = 28, "mutual recursion: err.number = " & Err.Number)

Err.Clear
RecurseForever
Call ok(Err.Number = 28, "infinite recursion: err.number = " & Err.Number)
On Error GoTo 0

' Regression: early-return paths in exec_script must not leak
' ctx->call_depth, otherwise a hot loop of arity-mismatch calls under
' OERN saturates the limit and breaks every later call.
Sub callDepthLeakProbe(a, b, c)
End Sub
Sub callDepthLeakSink()
End Sub
Dim callDepthLeakI
On Error Resume Next
For callDepthLeakI = 1 To 1100
    Err.Clear
    callDepthLeakProbe
Next
Err.Clear
callDepthLeakSink
Call ok(Err.Number = 0, "call_depth leak after arity-mismatch flood: err.number = " & Err.Number)
On Error GoTo 0

function f2(x,y)
end function

f2 1 = 1, 2

function f1(x)
    ok x = true, "x = " & x
end function

f1 1 = 1
f1 1 = (1)
f1 not 1 = 0

arr (0) = 2 xor -2

' Test calling a named item object with arguments (DISPID_VALUE)
Call ok(indexedObj(3) = 6, "indexedObj(3) = " & indexedObj(3))
Call ok(indexedObj(0) = 0, "indexedObj(0) = " & indexedObj(0))

' GetRef tests
Function GetRefTestFunc()
    GetRefTestFunc = 42
End Function

Dim getRefRef
Set getRefRef = GetRef("GetRefTestFunc")
Call ok(IsObject(getRefRef), "IsObject(GetRef result) should be true")
Call ok(getRefRef() = 42, "GetRef result call returned " & getRefRef())

' GetRef with parameters
Function GetRefAddFunc(a, b)
    GetRefAddFunc = a + b
End Function

Set getRefRef = GetRef("GetRefAddFunc")
Call ok(getRefRef(3, 4) = 7, "GetRef add call returned " & getRefRef(3, 4))

' GetRef with a Sub
Dim getRefSubCalled
getRefSubCalled = False
Sub GetRefTestSub()
    getRefSubCalled = True
End Sub

Set getRefRef = GetRef("GetRefTestSub")
Call getRefRef()
Call ok(getRefSubCalled, "GetRef sub was not called")

' GetRef with Sub that has parameters
Dim getRefSubResult
Sub GetRefTestSubArgs(a, b)
    getRefSubResult = a + b
End Sub

Set getRefRef = GetRef("GetRefTestSubArgs")
Call getRefRef(10, 20)
Call ok(getRefSubResult = 30, "GetRef sub with args returned " & getRefSubResult)

' GetRef case insensitivity
Function getRefCaseFunc()
    getRefCaseFunc = "hello"
End Function

Set getRefRef = GetRef("GETREFCASEFUNC")
Call ok(getRefRef() = "hello", "GetRef case insensitive returned " & getRefRef())

' GetRef default value (calling without parens triggers default property)
Set getRefRef = GetRef("GetRefTestFunc")
Dim getRefResult
getRefResult = getRefRef
Call ok(getRefResult = 42, "GetRef default value returned " & getRefResult)
Call ok(getVT(getRefResult) = "VT_I2*", "GetRef default value type is " & getVT(getRefResult))

' GetRef can be passed to another function
Function GetRefCallIt(fn)
    GetRefCallIt = fn()
End Function

Set getRefRef = GetRef("GetRefTestFunc")
Call ok(GetRefCallIt(getRefRef) = 42, "GetRef passed to function returned " & GetRefCallIt(getRefRef))

' GetRef error cases
On Error Resume Next

Err.Clear
Set getRefRef = GetRef("NonExistentFunc")
Call ok(Err.Number = 5, "GetRef non-existent function error is " & Err.Number)

Err.Clear
Set getRefRef = GetRef("")
Call ok(Err.Number = 5, "GetRef empty string error is " & Err.Number)

Err.Clear
Set getRefRef = GetRef(123)
Call ok(Err.Number = 13, "GetRef numeric arg error is " & Err.Number)

Err.Clear
Set getRefRef = GetRef(Null)
Call ok(Err.Number = 13, "GetRef Null arg error is " & Err.Number)

Err.Clear
Set getRefRef = GetRef(Empty)
Call ok(Err.Number = 13, "GetRef Empty arg error is " & Err.Number)

Err.Clear
Set getRefRef = GetRef(vbNullString)
Call ok(Err.Number = 5, "GetRef vbNullString error is " & Err.Number)

' GetRef called as a statement (result discarded). Must not crash even
' though no res pointer is passed to the builtin.
Err.Clear
Call GetRef("GetRefTestFunc")
Call ok(Err.Number = 0, "Call GetRef statement err = " & Err.Number)

Err.Clear
GetRef "GetRefTestFunc"
Call ok(Err.Number = 0, "Bare GetRef statement err = " & Err.Number)

' Eval tests
Call ok(Eval("1 + 2") = 3, "Eval(""1 + 2"") = " & Eval("1 + 2"))
Call ok(Eval("""test""") = "test", "Eval(""""""test"""""") = " & Eval("""test"""))
Call ok(Eval("true") = true, "Eval(""true"") = " & Eval("true"))

x = 5
Call ok(Eval("x + 1") = 6, "Eval(""x + 1"") = " & Eval("x + 1"))
Call ok(Eval("x * x") = 25, "Eval(""x * x"") = " & Eval("x * x"))

Sub TestEvalLocalScope
    Dim a
    a = 10
    Call ok(Eval("a") = 10, "Eval(""a"") in local scope = " & Eval("a"))
    Call ok(Eval("a + 5") = 15, "Eval(""a + 5"") in local scope = " & Eval("a + 5"))
End Sub
Call TestEvalLocalScope

Function TestEvalLocalArgs(x)
    TestEvalLocalArgs = Eval("x * 2")
End Function
Call ok(TestEvalLocalArgs(7) = 14, "TestEvalLocalArgs(7) = " & TestEvalLocalArgs(7))

Dim evalLocal
Sub TestEvalNoLeak
    Dim evalLocal
    evalLocal = 77
    Call ok(Eval("evalLocal") = 77, "Eval evalLocal = " & Eval("evalLocal"))
End Sub
Call TestEvalNoLeak
Call ok(getVT(evalLocal) = "VT_EMPTY*", "evalLocal leaked from Eval scope: " & getVT(evalLocal))

' Eval with non-BSTR arguments: non-string values are returned as-is
Call ok(Eval(42) = 42, "Eval(42) = " & Eval(42))
Call ok(getVT(Eval(42)) = "VT_I2", "Eval(42) type = " & getVT(Eval(42)))
Call ok(Eval(3.14) = 3.14, "Eval(3.14) = " & Eval(3.14))
Call ok(Eval(True) = True, "Eval(True) = " & Eval(True))
Call ok(IsNull(Eval(Null)), "Eval(Null) should be Null")
Call ok(IsEmpty(Eval(Empty)), "Eval(Empty) should be Empty")

' Eval with object that has a default property: converts to string via default, then evaluates
Class EvalHasDefault
    Public Default Function GetValue()
        GetValue = "True"
    End Function
End Class
Dim evalHasDefaultObj
Set evalHasDefaultObj = New EvalHasDefault
Call ok(Eval(evalHasDefaultObj) = True, "Eval(HasDefault) = " & Eval(evalHasDefaultObj))

' Eval with object without default property: should fail with error 438
Class EvalNoDefault
    Public Name
End Class
Dim evalNoDefaultObj
Set evalNoDefaultObj = New EvalNoDefault

On Error Resume Next
Err.Clear
Dim evalObjResult
evalObjResult = Eval(evalNoDefaultObj)
Call ok(Err.Number = 438, "Eval(NoDefault) err.number = " & Err.Number & " expected 438")

' Eval(Nothing) should fail with error 91
Err.Clear
Dim evalNothingResult
evalNothingResult = Eval(Nothing)
Call ok(Err.Number = 91, "Eval(Nothing) err.number = " & Err.Number & " expected 91")

' Eval(dispatch) in If..Is should trigger error, not silently return the object
Err.Clear
Dim evalCheckResult
evalCheckResult = True
If Not Eval(evalNoDefaultObj) Is evalNoDefaultObj Or Err Then evalCheckResult = False
Call ok(evalCheckResult = False, "Eval(obj) Is obj: checkResult = " & evalCheckResult & " expected False")

' Execute/ExecuteGlobal with non-BSTR should fail with type mismatch
Err.Clear
Execute 42
Call ok(Err.Number = 13, "Execute(42) err.number = " & Err.Number & " expected 13")

Err.Clear
ExecuteGlobal 42
Call ok(Err.Number = 13, "ExecuteGlobal(42) err.number = " & Err.Number & " expected 13")

' Execute/ExecuteGlobal with dispatch: converts via default property
Class ExecHasDefault
    Public Default Function GetValue()
        GetValue = "execHdResult = 99"
    End Function
End Class
Dim execHdObj : Set execHdObj = New ExecHasDefault
Dim execHdResult : execHdResult = 0

Err.Clear
Execute execHdObj
Call ok(Err.Number = 0, "Execute(HasDefault) err.number = " & Err.Number)
Call ok(execHdResult = 99, "Execute(HasDefault) execHdResult = " & execHdResult)

Err.Clear
execHdResult = 0
ExecuteGlobal execHdObj
Call ok(Err.Number = 0, "ExecuteGlobal(HasDefault) err.number = " & Err.Number)
Call ok(execHdResult = 99, "ExecuteGlobal(HasDefault) execHdResult = " & execHdResult)

' Execute/ExecuteGlobal with dispatch without default property: error 438
Err.Clear
Execute evalNoDefaultObj
Call ok(Err.Number = 438, "Execute(NoDefault) err.number = " & Err.Number & " expected 438")

Err.Clear
ExecuteGlobal evalNoDefaultObj
Call ok(Err.Number = 438, "ExecuteGlobal(NoDefault) err.number = " & Err.Number & " expected 438")

On Error GoTo 0

' ExecuteGlobal tests
x = 0
ExecuteGlobal "x = 42"
Call ok(x = 42, "ExecuteGlobal x = " & x)

ExecuteGlobal "Function evalTestFunc : evalTestFunc = 7 : End Function"
Call ok(evalTestFunc() = 7, "evalTestFunc() = " & evalTestFunc())

' ExecuteGlobal: Dim creates a new global variable
ExecuteGlobal "Dim egVar1 : egVar1 = 42"
Call ok(egVar1 = 42, "ExecuteGlobal Dim egVar1 = " & egVar1)

' ExecuteGlobal: Dim variable persists across calls
ExecuteGlobal "Dim egVar2"
ExecuteGlobal "egVar2 = 99"
Call ok(egVar2 = 99, "ExecuteGlobal Dim egVar2 (set later) = " & egVar2)

' ExecuteGlobal inside Sub: Dim creates a GLOBAL variable
Sub TestExecGlobalDimInSub
    ExecuteGlobal "Dim egFromSub : egFromSub = 123"
End Sub
Call TestExecGlobalDimInSub
Call ok(egFromSub = 123, "ExecuteGlobal Dim in Sub - egFromSub = " & egFromSub)

' ExecuteGlobal: Dim + Function in same call
ExecuteGlobal "Dim egFuncVar : Function egGetVar() : egGetVar = egFuncVar : End Function"
egFuncVar = 777
Call ok(egGetVar() = 777, "ExecuteGlobal Dim+Function: egGetVar() = " & egGetVar())

' ExecuteGlobal: ReDim creates global dynamic array
ExecuteGlobal "ReDim egDynArr(1) : egDynArr(0) = ""a"" : egDynArr(1) = ""b"""
Call ok(egDynArr(0) = "a", "ExecuteGlobal ReDim egDynArr(0) = " & egDynArr(0))
Call ok(egDynArr(1) = "b", "ExecuteGlobal ReDim egDynArr(1) = " & egDynArr(1))

' Execute tests
x = 0
Execute "x = 99"
Call ok(x = 99, "Execute x = " & x)

Sub TestExecuteLocalScope
    Dim a
    a = 10
    Execute "a = 20"
    Call ok(a = 20, "Execute local assign: a = " & a)
End Sub
Call TestExecuteLocalScope

Dim executeLocal
Sub TestExecuteNoLeak
    Dim executeLocal
    executeLocal = 10
    Execute "executeLocal = 55"
    Call ok(executeLocal = 55, "executeLocal = " & executeLocal)
End Sub
Call TestExecuteNoLeak
Call ok(getVT(executeLocal) = "VT_EMPTY*", "executeLocal leaked from Execute scope: " & getVT(executeLocal))

' Execute at global scope: Dim creates a new global variable
Execute "Dim exVar1 : exVar1 = 77"
Call ok(exVar1 = 77, "Execute (global) Dim exVar1 = " & exVar1)

' Execute inside Sub: Dim creates variable visible in caller's scope
Sub TestExecuteDimInSub
    Dim localA
    localA = 10
    Execute "Dim localB : localB = 20"
    Call ok(localB = 20, "Execute Dim in Sub - localB = " & localB)
    Call ok(localA = 10, "Execute Dim in Sub - localA still = " & localA)
End Sub
Call TestExecuteDimInSub

' Execute inside Function: Dim creates variable visible in function scope
Function TestExecuteDimInFunc
    Execute "Dim funcVar : funcVar = 55"
    TestExecuteDimInFunc = funcVar
End Function
Call ok(TestExecuteDimInFunc() = 55, "Execute Dim in Function = " & TestExecuteDimInFunc())

' Execute: Dim same name as existing local has no effect, value preserved
Sub TestExecuteDimShadow
    Dim x
    x = 100
    Execute "Dim x"
    Call ok(x = 100, "x after Execute Dim x = " & x)
    Execute "x = 200"
    Call ok(x = 200, "x after Execute x=200 = " & x)
End Sub
Call TestExecuteDimShadow

' Execute: Dim then use in same string, visible in caller
Sub TestExecuteDimAndUse
    Execute "Dim euVar : euVar = 999"
    Call ok(euVar = 999, "Execute Dim+use euVar = " & euVar)
End Sub
Call TestExecuteDimAndUse

' Execute: ReDim creates dynamic array visible in caller
Sub TestExecuteReDim
    Execute "ReDim dynArr(2) : dynArr(0) = 10 : dynArr(1) = 20 : dynArr(2) = 30"
    Call ok(dynArr(1) = 20, "Execute ReDim dynArr(1) = " & dynArr(1))
End Sub
Call TestExecuteReDim

' Option Explicit is per-compilation-unit in Execute/ExecuteGlobal
On Error Resume Next

' Option Explicit inside same Execute string enforces Dim requirement
Err.Clear
Execute "Option Explicit : noDimExec = 101"
Call ok(Err.Number <> 0, "Execute Option Explicit undeclared should error: err=" & Err.Number)
ok Err.Number = 500, "Execute Option Explicit undeclared: err=" & Err.Number & " expected 500"

' Option Explicit inside same ExecuteGlobal string enforces Dim requirement
Err.Clear
ExecuteGlobal "Option Explicit : noDimExecGlobal = 102"
Call ok(Err.Number <> 0, "ExecuteGlobal Option Explicit undeclared should error: err=" & Err.Number)
ok Err.Number = 500, "ExecuteGlobal Option Explicit undeclared: err=" & Err.Number & " expected 500"

' Option Explicit with Dim in same string works
Err.Clear
ExecuteGlobal "Option Explicit : Dim oeDimVar : oeDimVar = 555"
Call ok(Err.Number = 0, "ExecuteGlobal Option Explicit + Dim: err=" & Err.Number)
Call ok(oeDimVar = 555, "ExecuteGlobal Option Explicit + Dim: oeDimVar = " & oeDimVar)

' Option Explicit does not propagate to subsequent calls
Err.Clear
ExecuteGlobal "Option Explicit"
Call ok(Err.Number = 0, "ExecuteGlobal Option Explicit alone: err=" & Err.Number)
Err.Clear
ExecuteGlobal "noDimAfterOE = 123"
Call ok(Err.Number = 0, "ExecuteGlobal after separate OE: err=" & Err.Number)

On Error Goto 0


' Eval/Execute/ExecuteGlobal error handling tests
On Error Resume Next

' Test Eval with syntax error
Err.Clear
Call Eval("1 + ")
Call ok(Err.Number = 1002, "Eval syntax error: Err.Number = " & Err.Number & " expected 1002")
Call ok(Err.Description = "Syntax error", "Eval syntax error: Err.Description = """ & Err.Description & """ expected ""Syntax error""")
Call ok(Err.Source = "Microsoft VBScript compilation error", "Eval syntax error: Err.Source = """ & Err.Source & """")

' Test Execute with syntax error
Err.Clear
Execute "x = "
Call ok(Err.Number = 1002, "Execute syntax error: Err.Number = " & Err.Number & " expected 1002")
Call ok(Err.Description = "Syntax error", "Execute syntax error: Err.Description = """ & Err.Description & """ expected ""Syntax error""")
Call ok(Err.Source = "Microsoft VBScript compilation error", "Execute syntax error: Err.Source = """ & Err.Source & """")

' Test ExecuteGlobal with syntax error
Err.Clear
ExecuteGlobal "y = "
Call ok(Err.Number = 1002, "ExecuteGlobal syntax error: Err.Number = " & Err.Number & " expected 1002")
Call ok(Err.Description = "Syntax error", "ExecuteGlobal syntax error: Err.Description = """ & Err.Description & """ expected ""Syntax error""")
Call ok(Err.Source = "Microsoft VBScript compilation error", "ExecuteGlobal syntax error: Err.Source = """ & Err.Source & """")

' Eval with non-string arguments returns the value directly
Err.Clear
Call ok(Eval(123) = 123, "Eval(123) = " & Eval(123))
Call ok(Err.Number = 0, "Eval(123) Err.Number = " & Err.Number)

Err.Clear
Call ok(Eval(True) = True, "Eval(True) = " & Eval(True))
Call ok(Err.Number = 0, "Eval(True) Err.Number = " & Err.Number)

Err.Clear
Call ok(IsNull(Eval(Null)), "Eval(Null) should be Null")
Call ok(Err.Number = 0, "Eval(Null) Err.Number = " & Err.Number)

Err.Clear
Call ok(IsEmpty(Eval(Empty)), "Eval(Empty) should be Empty")
Call ok(Err.Number = 0, "Eval(Empty) Err.Number = " & Err.Number)

' Execute with non-string arguments returns type mismatch
Err.Clear
Execute 123
Call ok(Err.Number = 13, "Execute 123 Err.Number = " & Err.Number)

Err.Clear
Execute Null
Call ok(Err.Number = 13, "Execute Null Err.Number = " & Err.Number)

Err.Clear
Execute Empty
Call ok(Err.Number = 13, "Execute Empty Err.Number = " & Err.Number)

' ExecuteGlobal with non-string arguments returns type mismatch
Err.Clear
ExecuteGlobal 123
Call ok(Err.Number = 13, "ExecuteGlobal 123 Err.Number = " & Err.Number)

Err.Clear
ExecuteGlobal Null
Call ok(Err.Number = 13, "ExecuteGlobal Null Err.Number = " & Err.Number)

Err.Clear
ExecuteGlobal Empty
Call ok(Err.Number = 13, "ExecuteGlobal Empty Err.Number = " & Err.Number)

' Eval with Dim is a syntax error (Eval expects an expression, not a statement)
Err.Clear
Eval "Dim evalDimVar"
Call ok(Err.Number = 1002, "Eval Dim: Err.Number = " & Err.Number & " expected 1002")

' Test runtime error in Eval is caught by On Error Resume Next
Err.Clear
Call Eval("CBool(""notabool"")")
Call ok(Err.Number = 13, "Eval type mismatch: Err.Number = " & Err.Number & " expected 13")
Call ok(Err.Source = "Microsoft VBScript runtime error", "Eval type mismatch: Err.Source = """ & Err.Source & """")

On Error Goto 0
' Duplicate Sub declarations: last one wins
dim dupSubResult
dupSubResult = 0
Sub DupSub()
    dupSubResult = 1
End Sub
Sub DupSub()
    dupSubResult = 2
End Sub
DupSub
call ok(dupSubResult = 2, "dupSubResult = " & dupSubResult)

' Sub replaced by Function with same name: Function wins
dim dupMixedResult
dupMixedResult = 0
Sub DupMixed()
    dupMixedResult = 1
End Sub
Function DupMixed()
    dupMixedResult = 2
    DupMixed = 42
End Function
dim dupMixedRet
dupMixedRet = DupMixed()
call ok(dupMixedResult = 2, "dupMixedResult = " & dupMixedResult)
call ok(dupMixedRet = 42, "dupMixedRet = " & dupMixedRet)

' Test calling a dispatch variable as statement (invokes default property)
funcCalled = ""
Set obj = New DefaultSubTest1
obj 3
Call ok(funcCalled = "init3", "dispatch var as statement: funcCalled = " & funcCalled)

' Test calling a dispatch variable (default Function, no args) as statement
funcCalled = ""
Set obj = New DefaultSubTest2
obj
Call ok(funcCalled = "init", "dispatch var (default func) as statement: funcCalled = " & funcCalled)

' Test calling non-dispatch variables as statement gives type mismatch (error 13)
On Error Resume Next

dim intCallVar
intCallVar = 42
Err.Clear
intCallVar
Call ok(Err.Number = 13, "int var as statement: err = " & Err.Number)

dim strCallVar
strCallVar = "hello"
Err.Clear
strCallVar
Call ok(Err.Number = 13, "string var as statement: err = " & Err.Number)

dim emptyCallVar
Err.Clear
emptyCallVar
Call ok(Err.Number = 13, "empty var as statement: err = " & Err.Number)

dim boolCallVar
boolCallVar = True
Err.Clear
boolCallVar
Call ok(Err.Number = 13, "bool var as statement: err = " & Err.Number)

On Error GoTo 0

' Tests for missing runtime error codes
On Error Resume Next

' Error 11: Division by zero
Err.Clear
Dim divZeroResult
divZeroResult = 1 \ 0
Call ok(Err.Number = 11, "division by zero: err.number = " & Err.Number)

' Error 94: Invalid use of Null
Err.Clear
Dim nullResult
nullResult = CLng(Null)
todo_wine_ok Err.Number = 94, "CLng(Null): err.number = " & Err.Number

' Error 429: ActiveX component can't create object
Err.Clear
Dim badObj
Set badObj = CreateObject("No.Such.Object.XXXXXX")
Call ok(Err.Number = 429, "CreateObject bad ProgID: err.number = " & Err.Number)

' Error 451: Object not a collection
Err.Clear
Dim forEachVar
For Each forEachVar In 42
Next
Call ok(Err.Number = 451, "For Each over integer: err.number = " & Err.Number)

' Error 457: Duplicate key in Dictionary
Err.Clear
Dim dictObj
Set dictObj = CreateObject("Scripting.Dictionary")
dictObj.Add "key1", 1
dictObj.Add "key1", 2
Call ok(Err.Number = 457, "duplicate Dictionary key: err.number = " & Err.Number)

' Error 500: `New` with an undeclared identifier resolves the name as a
' variable first and fails with "Variable is undefined" before reaching
' class lookup.
Err.Clear
Dim undefinedNewTarget
Set undefinedNewTarget = New NoSuchClass
Call ok(Err.Number = 500, "New undeclared identifier: err.number = " & Err.Number)

' Error 506: Class not defined. To reach the class lookup path, the
' identifier must be a declared variable that isn't a class.
Err.Clear
Dim notAClass
notAClass = 42
Dim undefinedClassObj
Set undefinedClassObj = New notAClass
Call ok(Err.Number = 506, "New non-class variable: err.number = " & Err.Number)

On Error GoTo 0

' === GetLocale / SetLocale / locale-sensitive Format* ===
Dim origLcid
origLcid = GetLocale()
Call ok(origLcid > 0, "GetLocale initial: " & origLcid)

' SetLocale returns the previous LCID.
Call ok(SetLocale(1033) = origLcid, "SetLocale(1033) return value")
Call ok(GetLocale() = 1033, "GetLocale after SetLocale(1033): " & GetLocale())

' en-US formatting
Call ok(FormatNumber(1234567.89) = "1,234,567.89", "FormatNumber en-US: " & FormatNumber(1234567.89))
Call ok(FormatCurrency(1234567.89) = "$1,234,567.89", "FormatCurrency en-US: " & FormatCurrency(1234567.89))
Call ok(FormatPercent(0.1234) = "12.34%", "FormatPercent en-US: " & FormatPercent(0.1234))
Call ok(FormatDateTime(DateSerial(2026,3,15), 1) = "Sunday, March 15, 2026", _
    "FormatDateTime en-US: " & FormatDateTime(DateSerial(2026,3,15), 1))

' de-DE: '.' thousands, ',' decimal
Call SetLocale(1031)
Call ok(GetLocale() = 1031, "GetLocale after SetLocale(1031)")
Call ok(FormatNumber(1234567.89) = "1.234.567,89", "FormatNumber de-DE: " & FormatNumber(1234567.89))
Call ok(FormatPercent(0.1234) = "12,34%", "FormatPercent de-DE: " & FormatPercent(0.1234))

' ja-JP: Anglo number formatting
Call SetLocale(1041)
Call ok(GetLocale() = 1041, "GetLocale after SetLocale(1041)")
Call ok(FormatNumber(1234567.89) = "1,234,567.89", "FormatNumber ja-JP: " & FormatNumber(1234567.89))
Call ok(FormatPercent(0.1234) = "12.34%", "FormatPercent ja-JP: " & FormatPercent(0.1234))

' Variant coercion to string (CStr etc.) uses the script LCID.
Call SetLocale(1031)
Call ok(CStr(1.5) = "1,5", "CStr(1.5) de-DE: " & CStr(1.5))
Call SetLocale(1033)
Call ok(CStr(1.5) = "1.5", "CStr(1.5) en-US: " & CStr(1.5))

' Day/Month/Year parse date strings using the script LCID.
Call SetLocale(1031)
Call ok(Day("15.03.2026") = 15, "Day(""15.03.2026"") de-DE: " & Day("15.03.2026"))
Call ok(Month("15.03.2026") = 3, "Month(""15.03.2026"") de-DE: " & Month("15.03.2026"))
Call ok(Year("15.03.2026") = 2026, "Year(""15.03.2026"") de-DE: " & Year("15.03.2026"))

' The German-format string is rejected under en-US.
Call SetLocale(1033)
Dim dateErr
On Error Resume Next
Err.Clear
Call Day("15.03.2026")
dateErr = Err.Number
Err.Clear
On Error Goto 0
Call ok(dateErr <> 0, "Day(""15.03.2026"") en-US should error: err=" & dateErr)

' Restore original locale.
Call SetLocale(origLcid)
Call ok(GetLocale() = origLcid, "restore: GetLocale = " & GetLocale())

reportSuccess()
