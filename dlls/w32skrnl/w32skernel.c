/*
 * W32SKRNL
 * DLL for Win32s
 *
 * Copyright (c) 1997 Andreas Mohr
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
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"

/***********************************************************************
 *		_kGetWin32sDirectory@0 (W32SKRNL.14)
 */
LPSTR WINAPI GetWin32sDirectory(void)
{
    static const char win32s[] = "\\win32s";

    int len = GetSystemDirectoryA( NULL, 0 );
    LPSTR text = HeapAlloc( GetProcessHeap(), 0, len + sizeof(win32s) - 1 );
    GetSystemDirectoryA( text, len );
    strcat( text, win32s );
    return text;
}

/***********************************************************************
 *           GetCurrentTask32   (W32SKRNL.3)
 */
HTASK16 WINAPI GetCurrentTask32(void)
{
    return GetCurrentTask();
}
