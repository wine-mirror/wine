/*
 * Copyright (C) the Wine project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _APISETCONSOLE_
#define _APISETCONSOLE_

#include <wincontypes.h>

/* Console Mode flags */
#define ENABLE_PROCESSED_INPUT         0x0001
#define ENABLE_LINE_INPUT              0x0002
#define ENABLE_ECHO_INPUT              0x0004
#define ENABLE_WINDOW_INPUT            0x0008
#define ENABLE_MOUSE_INPUT             0x0010
#define ENABLE_INSERT_MODE             0x0020
#define ENABLE_QUICK_EDIT_MODE         0x0040
#define ENABLE_EXTENDED_FLAGS          0x0080
#define ENABLE_AUTO_POSITION           0x0100
#define ENABLE_VIRTUAL_TERMINAL_INPUT  0x0200

#define ENABLE_PROCESSED_OUTPUT             0x0001
#define ENABLE_WRAP_AT_EOL_OUTPUT           0x0002
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING  0x0004
#define DISABLE_NEWLINE_AUTO_RETURN         0x0008
#define ENABLE_LVB_GRID_WORLDWIDE           0x0010

#define PSEUDOCONSOLE_INHERIT_CURSOR 0x01

/* handler routine control signal type */
#define CTRL_C_EVENT        0
#define CTRL_BREAK_EVENT    1
#define CTRL_CLOSE_EVENT    2
#define CTRL_LOGOFF_EVENT   5
#define CTRL_SHUTDOWN_EVENT 6

typedef BOOL (WINAPI *PHANDLER_ROUTINE)(DWORD dwCtrlType);

typedef struct _CONSOLE_READCONSOLE_CONTROL
{
    ULONG       nLength;
    ULONG       nInitialChars;
    ULONG       dwCtrlWakeupMask;
    ULONG       dwControlKeyState;
} CONSOLE_READCONSOLE_CONTROL,*LPCONSOLE_READCONSOLE_CONTROL;

#ifdef __cplusplus
extern "C" {
#endif

WINBASEAPI BOOL    WINAPI AllocConsole(void);
WINBASEAPI BOOL    WINAPI AttachConsole(DWORD);
WINBASEAPI void    WINAPI ClosePseudoConsole(HPCON);
WINBASEAPI HRESULT WINAPI CreatePseudoConsole(COORD,HANDLE,HANDLE,DWORD,HPCON *);
WINBASEAPI BOOL    WINAPI FreeConsole(void);
WINBASEAPI UINT    WINAPI GetConsoleCP(void);
WINBASEAPI BOOL    WINAPI GetConsoleMode( HANDLE,DWORD *);
WINBASEAPI UINT    WINAPI GetConsoleOutputCP(void);
WINBASEAPI BOOL    WINAPI GetNumberOfConsoleInputEvents( HANDLE,DWORD *);
WINBASEAPI BOOL    WINAPI PeekConsoleInputA(HANDLE,PINPUT_RECORD,DWORD,DWORD *);
WINBASEAPI BOOL    WINAPI PeekConsoleInputW(HANDLE,PINPUT_RECORD,DWORD,DWORD *);
#define                   PeekConsoleInput WINELIB_NAME_AW(PeekConsoleInput)
WINBASEAPI BOOL    WINAPI ReadConsoleA(HANDLE,void *,DWORD,DWORD *,void *);
WINBASEAPI BOOL    WINAPI ReadConsoleW(HANDLE,void *,DWORD,DWORD *,void *);
#define                   ReadConsole WINELIB_NAME_AW(ReadConsole)
WINBASEAPI BOOL    WINAPI ReadConsoleInputA(HANDLE,PINPUT_RECORD,DWORD,DWORD *);
WINBASEAPI BOOL    WINAPI ReadConsoleInputW(HANDLE,PINPUT_RECORD,DWORD,DWORD *);
#define                   ReadConsoleInput WINELIB_NAME_AW(ReadConsoleInput)
WINBASEAPI HRESULT WINAPI ResizePseudoConsole(HPCON,COORD);
WINBASEAPI BOOL    WINAPI SetConsoleCtrlHandler( PHANDLER_ROUTINE,BOOL);
WINBASEAPI BOOL    WINAPI SetConsoleMode( HANDLE,DWORD);
WINBASEAPI BOOL    WINAPI WriteConsoleA(HANDLE,const void *,DWORD,DWORD *,void *);
WINBASEAPI BOOL    WINAPI WriteConsoleW(HANDLE,const void *,DWORD,DWORD *,void *);
#define                   WriteConsole WINELIB_NAME_AW(WriteConsole)

#ifdef __cplusplus
}
#endif

#endif /* _APISETCONSOLE_ */
