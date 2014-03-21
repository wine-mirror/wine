'
' Copyright 2014 Jacek Caban for CodeWeavers
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

const E_TESTERROR = &h80080008&

const VB_E_FORLOOPNOTINITIALIZED = 92
const VB_E_OBJNOTCOLLECTION = 451

call ok(Err.Number = 0, "Err.Number = " & Err.Number)

dim calledFunc

sub returnTrue
    calledFunc = true
    returnTrue = true
end sub

sub testThrow
    on error resume next

    dim x, y

    call throwInt(1000)
    call ok(Err.Number = 0, "Err.Number = " & Err.Number)

    call throwInt(E_TESTERROR)
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    call throwInt(1000)
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    call Err.clear()
    call ok(Err.Number = 0, "Err.Number = " & Err.Number)

    x = 6
    calledFunc = false
    x = throwInt(E_TESTERROR) and returnTrue()
    call ok(x = 6, "x = " & x)
    call ok(not calledFunc, "calledFunc = " & calledFunc)
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = false
    call Err.clear()
    if false and throwInt(E_TESTERROR) then
        x = true
    else
        call ok(false, "unexpected if else branch on throw")
    end if
    call ok(x, "if branch not taken")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = false
    call Err.clear()
    if throwInt(E_TESTERROR) then x = true
    call ok(x, "if branch not taken")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = false
    call Err.clear()
    if false then
        call ok(false, "unexpected if else branch on throw")
    elseif throwInt(E_TESTERROR) then
        x = true
    else
        call ok(false, "unexpected if else branch on throw")
    end if
    call ok(x, "elseif branch not taken")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    call Err.clear()
    if true then
        call throwInt(E_TESTERROR)
    else
        call ok(false, "unexpected if else branch on throw")
    end if
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = false
    call Err.clear()
    do while throwInt(E_TESTERROR)
        call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)
        x = true
        exit do
    loop
    call ok(x, "if branch not taken")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = 0
    call Err.clear()
    do
        x = x+1
        call ok(Err.Number = 0, "Err.Number = " & Err.Number)
    loop while throwInt(E_TESTERROR)
    call ok(x = 1, "if branch not taken")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = 0
    call Err.clear()
    do
        x = x+1
        call ok(Err.Number = 0, "Err.Number = " & Err.Number)
    loop until throwInt(E_TESTERROR)
    call ok(x = 1, "if branch not taken")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    call Err.clear()
    x = 0
    while x < 2
        x = x+1
        call throwInt(E_TESTERROR)
    wend
    call ok(x = 2, "x = " & x)
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    call Err.clear()
    x = 2
    y = 0
    for each x in throwInt(E_TESTERROR)
        call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)
        y = y+1
    next
    call ok(x = 2, "x = " & x)
    call ok(y = 1, "y = " & y)
    'todo_wine call ok(Err.Number = VB_E_OBJNOTCOLLECTION, "Err.Number = " & Err.Number)

    Err.clear()
    y = 0
    x = 6
    for x = throwInt(E_TESTERROR) to 100
        call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)
        call ok(x = 6, "x = " & x)
        y = y+1
    next
    call ok(y = 1, "y = " & y)
    call ok(x = 6, "x = " & x)
    'todo_wine call ok(Err.Number = VB_E_FORLOOPNOTINITIALIZED, "Err.Number = " & Err.Number)

    Err.clear()
    y = 0
    x = 6
    for x = 100 to throwInt(E_TESTERROR)
        call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)
        'todo_wine call ok(x = 6, "x = " & x)
        y = y+1
    next
    call ok(y = 1, "y = " & y)
    'todo_wine call ok(x = 6, "x = " & x)
    'todo_wine call ok(Err.Number = VB_E_FORLOOPNOTINITIALIZED, "Err.Number = " & Err.Number)

    select case throwInt(E_TESTERROR)
    case true
         call ok(false, "unexpected case true")
    case false
         call ok(false, "unexpected case false")
    case empty
         call ok(false, "unexpected case empty")
    case else
         call ok(false, "unexpected case else")
    end select
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    x = false
    select case false
    case true
         call ok(false, "unexpected case true")
    case throwInt(E_TESTERROR)
         x = true
    case else
         call ok(false, "unexpected case else")
    end select
    call ok(x, "case not executed")
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)

    'Exception in non-trivial stack context
    for x = 1 to 1
        for each y in collectionObj
            select case 3
            case 1
                call ok(false, "unexpected case")
            case throwInt(E_TESTERROR)
                exit for
            case 2
                call ok(false, "unexpected case")
            end select
        next
    next
end sub

call testThrow

dim x

sub testOnError(resumeNext)
    if resumeNext then
        on error resume next
    else
        on error goto 0
    end if
    x = 1
    throwInt(E_TESTERROR)
    x = 2
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)
end sub

sub callTestOnError(resumeNext)
    on error resume next
    call testOnError(resumeNext)
    call ok(Err.Number = E_TESTERROR, "Err.Number = " & Err.Number)
end sub

x = 0
call callTestOnError(true)
call ok(x = 2, "x = " & x)

x = 0
call callTestOnError(false)
call ok(x = 1, "x = " & x)

sub testForEachError()
    on error resume next

    dim x, y
    y = false
    for each x in empty
        y = true
    next
    call ok(y, "for each not executed")
    'todo_wine call ok(Err.Number = VB_E_OBJNOTCOLLECTION, "Err.Number = " & Err.Number)
end sub

call testForEachError()

call reportSuccess()
