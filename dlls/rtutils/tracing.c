/*
 * Tracing API functions
 *
 * Copyright 2009 Alexander Scott-Johns
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

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winuser.h"
#include "winnls.h"

#include "wine/debug.h"

#include "rtutils.h"

WINE_DEFAULT_DEBUG_CHANNEL(rtutils);

/******************************************************************************
 * TraceRegisterExW (RTUTILS.@)
 */
DWORD WINAPI TraceRegisterExW(LPCWSTR name, DWORD flags)
{
    FIXME("(%s, %lx): stub\n", debugstr_w(name), flags);
    return INVALID_TRACEID;
}

/******************************************************************************
 * TraceRegisterExA (RTUTILS.@)
 *
 * See TraceRegisterExW.
 */
DWORD WINAPI TraceRegisterExA(LPCSTR name, DWORD flags)
{
    DWORD id;
    int lenW = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0);
    WCHAR* nameW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
    if (!nameW)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return INVALID_TRACEID;
    }
    MultiByteToWideChar(CP_ACP, 0, name, -1, nameW, lenW);
    id = TraceRegisterExW(nameW, flags);
    HeapFree(GetProcessHeap(), 0, nameW);
    return id;
}
