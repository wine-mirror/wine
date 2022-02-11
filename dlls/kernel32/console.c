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

#define NONAMELESSUNION
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

/******************************************************************************
 * GetConsoleWindow [KERNEL32.@] Get hwnd of the console window.
 *
 * RETURNS
 *   Success: hwnd of the console window.
 *   Failure: NULL
 */
HWND WINAPI GetConsoleWindow(void)
{
    condrv_handle_t win;
    BOOL ret;

    ret = DeviceIoControl( RtlGetCurrentPeb()->ProcessParameters->ConsoleHandle,
                           IOCTL_CONDRV_GET_WINDOW, NULL, 0, &win, sizeof(win), NULL, NULL );
    return ret ? LongToHandle( win ) : NULL;
}


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
 *            SetConsoleTitleA   (KERNEL32.@)
 */
BOOL WINAPI SetConsoleTitleA( LPCSTR title )
{
    LPWSTR titleW;
    BOOL ret;

    DWORD len = MultiByteToWideChar( GetConsoleOutputCP(), 0, title, -1, NULL, 0 );
    if (!(titleW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR)))) return FALSE;
    MultiByteToWideChar( GetConsoleOutputCP(), 0, title, -1, titleW, len );
    ret = SetConsoleTitleW(titleW);
    HeapFree(GetProcessHeap(), 0, titleW);
    return ret;
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

/***********************************************************************
 *            GetConsoleTitleA   (KERNEL32.@)
 *
 * See GetConsoleTitleW.
 */
DWORD WINAPI GetConsoleTitleA(LPSTR title, DWORD size)
{
    WCHAR *ptr = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * size);
    DWORD ret;

    if (!ptr) return 0;
    ret = GetConsoleTitleW( ptr, size );
    if (ret)
    {
        WideCharToMultiByte( GetConsoleOutputCP(), 0, ptr, ret + 1, title, size, NULL, NULL);
        ret = strlen(title);
    }
    HeapFree(GetProcessHeap(), 0, ptr);
    return ret;
}


/***********************************************************************
 *            GetNumberOfConsoleMouseButtons   (KERNEL32.@)
 */
BOOL WINAPI GetNumberOfConsoleMouseButtons(LPDWORD nrofbuttons)
{
    FIXME("(%p): stub\n", nrofbuttons);
    *nrofbuttons = 2;
    return TRUE;
}


/******************************************************************
 *              GetConsoleDisplayMode  (KERNEL32.@)
 */
BOOL WINAPI GetConsoleDisplayMode(LPDWORD lpModeFlags)
{
    TRACE("semi-stub: %p\n", lpModeFlags);
    /* It is safe to successfully report windowed mode */
    *lpModeFlags = 0;
    return TRUE;
}

/******************************************************************
 *              SetConsoleDisplayMode  (KERNEL32.@)
 */
BOOL WINAPI SetConsoleDisplayMode(HANDLE hConsoleOutput, DWORD dwFlags,
                                  COORD *lpNewScreenBufferDimensions)
{
    TRACE("(%p, %lx, (%d, %d))\n", hConsoleOutput, dwFlags,
          lpNewScreenBufferDimensions->X, lpNewScreenBufferDimensions->Y);
    if (dwFlags == 1)
    {
        /* We cannot switch to fullscreen */
        return FALSE;
    }
    return TRUE;
}

/******************************************************************
 *              GetConsoleAliasW
 *
 *
 * RETURNS
 *    0 if an error occurred, non-zero for success
 *
 */
DWORD WINAPI GetConsoleAliasW(LPWSTR lpSource, LPWSTR lpTargetBuffer,
                              DWORD TargetBufferLength, LPWSTR lpExename)
{
    FIXME("(%s,%p,%ld,%s): stub\n", debugstr_w(lpSource), lpTargetBuffer, TargetBufferLength, debugstr_w(lpExename));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/******************************************************************
 *              GetConsoleProcessList  (KERNEL32.@)
 */
DWORD WINAPI GetConsoleProcessList(LPDWORD processlist, DWORD processcount)
{
    FIXME("(%p,%ld): stub\n", processlist, processcount);

    if (!processlist || processcount < 1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    return 0;
}

/* Undocumented, called by native doskey.exe */
DWORD WINAPI GetConsoleCommandHistoryA(DWORD unknown1, DWORD unknown2, DWORD unknown3)
{
    FIXME(": (0x%lx, 0x%lx, 0x%lx) stub!\n", unknown1, unknown2, unknown3);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/* Undocumented, called by native doskey.exe */
DWORD WINAPI GetConsoleCommandHistoryW(DWORD unknown1, DWORD unknown2, DWORD unknown3)
{
    FIXME(": (0x%lx, 0x%lx, 0x%lx) stub!\n", unknown1, unknown2, unknown3);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/* Undocumented, called by native doskey.exe */
DWORD WINAPI GetConsoleCommandHistoryLengthA(LPCSTR unknown)
{
    FIXME(": (%s) stub!\n", debugstr_a(unknown));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/* Undocumented, called by native doskey.exe */
DWORD WINAPI GetConsoleCommandHistoryLengthW(LPCWSTR unknown)
{
    FIXME(": (%s) stub!\n", debugstr_w(unknown));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

DWORD WINAPI GetConsoleAliasesLengthA(LPSTR unknown)
{
    FIXME(": (%s) stub!\n", debugstr_a(unknown));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

DWORD WINAPI GetConsoleAliasesLengthW(LPWSTR unknown)
{
    FIXME(": (%s) stub!\n", debugstr_w(unknown));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

DWORD WINAPI GetConsoleAliasExesLengthA(void)
{
    FIXME(": stub!\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

DWORD WINAPI GetConsoleAliasExesLengthW(void)
{
    FIXME(": stub!\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

VOID WINAPI ExpungeConsoleCommandHistoryA(LPCSTR unknown)
{
    FIXME(": (%s) stub!\n", debugstr_a(unknown));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

VOID WINAPI ExpungeConsoleCommandHistoryW(LPCWSTR unknown)
{
    FIXME(": (%s) stub!\n", debugstr_w(unknown));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

BOOL WINAPI AddConsoleAliasA(LPSTR source, LPSTR target, LPSTR exename)
{
    FIXME(": (%s, %s, %s) stub!\n", debugstr_a(source), debugstr_a(target), debugstr_a(exename));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI AddConsoleAliasW(LPWSTR source, LPWSTR target, LPWSTR exename)
{
    FIXME(": (%s, %s, %s) stub!\n", debugstr_w(source), debugstr_w(target), debugstr_w(exename));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
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


BOOL WINAPI GetCurrentConsoleFontEx(HANDLE hConsole, BOOL maxwindow, CONSOLE_FONT_INFOEX *fontinfo)
{
    DWORD size;
    struct
    {
        struct condrv_output_info info;
        WCHAR face_name[LF_FACESIZE - 1];
    } data;

    if (fontinfo->cbSize != sizeof(CONSOLE_FONT_INFOEX))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!DeviceIoControl( hConsole, IOCTL_CONDRV_GET_OUTPUT_INFO, NULL, 0,
                          &data, sizeof(data), &size, NULL ))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    fontinfo->nFont = 0;
    if (maxwindow)
    {
        fontinfo->dwFontSize.X = min( data.info.width, data.info.max_width );
        fontinfo->dwFontSize.Y = min( data.info.height, data.info.max_height );
    }
    else
    {
        fontinfo->dwFontSize.X = data.info.font_width;
        fontinfo->dwFontSize.Y = data.info.font_height;
    }
    size -= sizeof(data.info);
    if (size) memcpy( fontinfo->FaceName, data.face_name, size );
    fontinfo->FaceName[size / sizeof(WCHAR)] = 0;
    fontinfo->FontFamily = data.info.font_pitch_family;
    fontinfo->FontWeight = data.info.font_weight;
    return TRUE;
}

BOOL WINAPI GetCurrentConsoleFont(HANDLE hConsole, BOOL maxwindow, CONSOLE_FONT_INFO *fontinfo)
{
    BOOL ret;
    CONSOLE_FONT_INFOEX res;

    res.cbSize = sizeof(CONSOLE_FONT_INFOEX);

    ret = GetCurrentConsoleFontEx(hConsole, maxwindow, &res);
    if(ret)
    {
        fontinfo->nFont = res.nFont;
        fontinfo->dwFontSize.X = res.dwFontSize.X;
        fontinfo->dwFontSize.Y = res.dwFontSize.Y;
    }
    return ret;
}

static COORD get_console_font_size(HANDLE hConsole, DWORD index)
{
    struct condrv_output_info info;
    COORD c = {0,0};

    if (index >= GetNumberOfConsoleFonts())
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return c;
    }

    if (DeviceIoControl( hConsole, IOCTL_CONDRV_GET_OUTPUT_INFO, NULL, 0, &info, sizeof(info), NULL, NULL ))
    {
        c.X = info.font_width;
        c.Y = info.font_height;
    }
    else SetLastError( ERROR_INVALID_HANDLE );
    return c;
}

#if defined(__i386__) && !defined(__MINGW32__) && !defined(_MSC_VER)
#undef GetConsoleFontSize
DWORD WINAPI GetConsoleFontSize(HANDLE hConsole, DWORD index)
{
    union {
        COORD c;
        DWORD w;
    } x;

    x.c = get_console_font_size(hConsole, index);
    return x.w;
}
#else
COORD WINAPI GetConsoleFontSize(HANDLE hConsole, DWORD index)
{
    return get_console_font_size(hConsole, index);
}
#endif /* !defined(__i386__) */

BOOL WINAPI GetConsoleFontInfo(HANDLE hConsole, BOOL maximize, DWORD numfonts, CONSOLE_FONT_INFO *info)
{
    FIXME("(%p %d %lu %p): stub!\n", hConsole, maximize, numfonts, info);
    SetLastError(LOWORD(E_NOTIMPL) /* win10 1709+ */);
    return FALSE;
}
