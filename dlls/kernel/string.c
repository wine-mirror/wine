/*
 * Kernel string functions
 *
 * Copyright 2001 Dmitry Timoshkov for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"


static INT (WINAPI *pLoadStringA)(HINSTANCE, UINT, LPSTR, INT);
static INT (WINAPI *pwvsprintfA)(LPSTR, LPCSTR, va_list);

/***********************************************************************
 * Helper for k32 family functions
 */
static void *user32_proc_address(const char *proc_name)
{
    static HMODULE hUser32;

    if(!hUser32) hUser32 = LoadLibraryA("user32.dll");
    return GetProcAddress(hUser32, proc_name);
}


/***********************************************************************
 *		k32CharToOemBuffA   (KERNEL32.11)
 */
BOOL WINAPI k32CharToOemBuffA(LPCSTR s, LPSTR d, DWORD len)
{
    WCHAR *bufW;

    if ((bufW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, s, len, bufW, len );
        WideCharToMultiByte( CP_OEMCP, 0, bufW, len, d, len, NULL, NULL );
        HeapFree( GetProcessHeap(), 0, bufW );
    }
    return TRUE;
}


/***********************************************************************
 *		k32CharToOemA   (KERNEL32.10)
 */
BOOL WINAPI k32CharToOemA(LPCSTR s, LPSTR d)
{
    if (!s || !d) return TRUE;
    return k32CharToOemBuffA( s, d, strlen(s) + 1 );
}


/***********************************************************************
 *		k32OemToCharBuffA   (KERNEL32.13)
 */
BOOL WINAPI k32OemToCharBuffA(LPCSTR s, LPSTR d, DWORD len)
{
    WCHAR *bufW;

    if ((bufW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_OEMCP, 0, s, len, bufW, len );
        WideCharToMultiByte( CP_ACP, 0, bufW, len, d, len, NULL, NULL );
        HeapFree( GetProcessHeap(), 0, bufW );
    }
    return TRUE;
}


/***********************************************************************
 *		k32OemToCharA   (KERNEL32.12)
 */
BOOL WINAPI k32OemToCharA(LPCSTR s, LPSTR d)
{
    return k32OemToCharBuffA( s, d, strlen(s) + 1 );
}


/**********************************************************************
 *		k32LoadStringA   (KERNEL32.14)
 */
INT WINAPI k32LoadStringA(HINSTANCE instance, UINT resource_id,
                          LPSTR buffer, INT buflen)
{
    if(!pLoadStringA) pLoadStringA = user32_proc_address("LoadStringA");
    return pLoadStringA(instance, resource_id, buffer, buflen);
}


/***********************************************************************
 *		k32wvsprintfA   (KERNEL32.16)
 */
INT WINAPI k32wvsprintfA(LPSTR buffer, LPCSTR spec, va_list args)
{
    if(!pwvsprintfA) pwvsprintfA = user32_proc_address("wvsprintfA");
    return (*pwvsprintfA)(buffer, spec, args);
}


/***********************************************************************
 *		k32wsprintfA   (KERNEL32.15)
 */
INT WINAPIV k32wsprintfA(LPSTR buffer, LPCSTR spec, ...)
{
    va_list args;
    INT res;

    va_start(args, spec);
    res = k32wvsprintfA(buffer, spec, args);
    va_end(args);
    return res;
}


/***************************************************************************
 *
 * Win 2.x string functions now moved to USER
 *
 * We rather want to implement them here instead of doing Callouts
 */

/***********************************************************************
 *		Reserved1 (KERNEL.77)
 */
SEGPTR WINAPI KERNEL_AnsiNext16(SEGPTR current)
{
    return (*(char *)MapSL(current)) ? current + 1 : current;
}

/***********************************************************************
 *		Reserved2(KERNEL.78)
 */
SEGPTR WINAPI KERNEL_AnsiPrev16( SEGPTR start, SEGPTR current )
{
    return (current==start)?start:current-1;
}

/***********************************************************************
 *		Reserved3 (KERNEL.79)
 */
SEGPTR WINAPI KERNEL_AnsiUpper16( SEGPTR strOrChar )
{
    /* uppercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        char *s = MapSL(strOrChar);
        while (*s)
        {
            *s = toupper(*s);
            s++;
        }
        return strOrChar;
    }
    else return toupper((char)strOrChar);
}

/***********************************************************************
 *		Reserved4 (KERNEL.80)
 */
SEGPTR WINAPI KERNEL_AnsiLower16( SEGPTR strOrChar )
{
    /* lowercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        char *s = MapSL(strOrChar);
        while (*s)
        {
            *s = tolower(*s);
            s++;
        }
        return strOrChar;
    }
    else return tolower((char)strOrChar);
}


/***********************************************************************
 *		Reserved5 (KERNEL.87)
 */
INT16 WINAPI KERNEL_lstrcmp16( LPCSTR str1, LPCSTR str2 )
{
    return (INT16)strcmp( str1, str2 );
}
