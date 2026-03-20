'
' Copyright 2026 Francis De Brabandere
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

' Tests that require implicit variable creation (no Option Explicit)

' assigning to undeclared variable should implicitly create it
x = 1
ok x = 1, "x = " & x

' reading undeclared variable should return Empty
ok getVT(y) = "VT_EMPTY*", "getVT(y) = " & getVT(y)
ok y = "", "y = " & y

' indexed assign to undeclared variable should give type mismatch
on error resume next
z(0) = 42
ok err.number = 13, "err.number = " & err.number
on error goto 0

call reportSuccess()
