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

Dim winetest
Set winetest = CreateObject("Wine.Test")

Sub ok(expr, msg)
    Call winetest.ok(expr, msg)
End Sub

Call ok(WScript is WSH, "WScript is not WSH")

Call ok(WScript.Timeout = 0, "default Timeout = " & WScript.Timeout)
Call ok(TypeName(WScript.Timeout) = "Long", "Timeout TypeName = " & TypeName(WScript.Timeout))

WScript.Timeout = 30
Call ok(WScript.Timeout = 30, "WScript.Timeout = " & WScript.Timeout)

WScript.Timeout = 0
Call ok(WScript.Timeout = 0, "WScript.Timeout = " & WScript.Timeout)

Dim prev_timeout
prev_timeout = WScript.Timeout
On Error Resume Next
Err.Clear
WScript.Timeout = -1
Call ok(Err.Number = 5, "negative Timeout err.Number = " & Err.Number)
Call ok(WScript.Timeout = prev_timeout, "negative Timeout changed value to " & WScript.Timeout)
On Error Goto 0

Call winetest.reportSuccess()
