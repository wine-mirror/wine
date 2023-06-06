/*
 * Win32 console functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 * Copyright 1997 Karl Garrison
 * Copyright 1998 John Richardson
 * Copyright 1998 Marcus Meissner
 * Copyright 2001,2002,2004,2005,2010 Eric Pouech
 * Copyright 2001 Alexandre Julliard
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "winerror.h"
#include "wincon.h"
#include "wine/condrv.h"
#include "wine/debug.h"
#include "kernel_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(console);

/******************************************************************
 *		OpenConsoleW            (KERNEL32.@)
 *
 * Undocumented
 *      Open a handle to the current process console.
 *      Returns INVALID_HANDLE_VALUE on failure.
 */
HANDLE WINAPI OpenConsoleW(LPCWSTR name, DWORD access, BOOL inherit, DWORD creation)
{
    SECURITY_ATTRIBUTES sa;

    TRACE("(%s, 0x%08lx, %d, %lu)\n", debugstr_w(name), access, inherit, creation);

    if (!name || (wcsicmp( L"CONIN$", name ) && wcsicmp( L"CONOUT$", name )) || creation != OPEN_EXISTING)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = inherit;

    return CreateFileW( name, access, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, creation, 0, NULL );
}

/******************************************************************
 *		VerifyConsoleIoHandle            (KERNEL32.@)
 *
 * Undocumented
 */
BOOL WINAPI VerifyConsoleIoHandle(HANDLE handle)
{
    IO_STATUS_BLOCK io;
    DWORD mode;
    return !NtDeviceIoControlFile( handle, NULL, NULL, NULL, &io, IOCTL_CONDRV_GET_MODE,
                                   NULL, 0, &mode, sizeof(mode) );
}

/******************************************************************
 *		DuplicateConsoleHandle            (KERNEL32.@)
 *
 * Undocumented
 */
HANDLE WINAPI DuplicateConsoleHandle(HANDLE handle, DWORD access, BOOL inherit,
                                     DWORD options)
{
    HANDLE ret;
    return DuplicateHandle(GetCurrentProcess(), handle, GetCurrentProcess(), &ret,
                           access, inherit, options) ? ret : INVALID_HANDLE_VALUE;
}

/******************************************************************
 *		CloseConsoleHandle            (KERNEL32.@)
 *
 * Undocumented
 */
BOOL WINAPI CloseConsoleHandle(HANDLE handle)
{
    return CloseHandle(handle);
}

/******************************************************************
 *		GetConsoleInputWaitHandle            (KERNEL32.@)
 *
 * Undocumented
 */
HANDLE WINAPI GetConsoleInputWaitHandle(void)
{
    return GetStdHandle( STD_INPUT_HANDLE );
}


/***********************************************************************
 *            GetConsoleKeyboardLayoutNameA   (KERNEL32.@)
 */
BOOL WINAPI GetConsoleKeyboardLayoutNameA(LPSTR layoutName)
{
    FIXME( "stub %p\n", layoutName);
    return TRUE;
}

/***********************************************************************
 *            GetConsoleKeyboardLayoutNameW   (KERNEL32.@)
 */
BOOL WINAPI GetConsoleKeyboardLayoutNameW(LPWSTR layoutName)
{
    static int once;
    if (!once++)
        FIXME( "stub %p\n", layoutName);
    return TRUE;
}

BOOL WINAPI SetConsoleIcon(HICON icon)
{
    FIXME(": (%p) stub!\n", icon);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

DWORD WINAPI GetNumberOfConsoleFonts(void)
{
    return 1;
}

BOOL WINAPI SetConsoleFont(HANDLE hConsole, DWORD index)
{
    FIXME("(%p, %lu): stub!\n", hConsole, index);
    SetLastError(LOWORD(E_NOTIMPL) /* win10 1709+ */);
    return FALSE;
}

BOOL WINAPI SetConsoleKeyShortcuts(BOOL set, BYTE keys, VOID *a, DWORD b)
{
    FIXME(": (%u %u %p %lu) stub!\n", set, keys, a, b);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL WINAPI GetConsoleFontInfo(HANDLE hConsole, BOOL maximize, DWORD numfonts, CONSOLE_FONT_INFO *info)
{
    FIXME("(%p %d %lu %p): stub!\n", hConsole, maximize, numfonts, info);
    SetLastError(LOWORD(E_NOTIMPL) /* win10 1709+ */);
    return FALSE;
}
