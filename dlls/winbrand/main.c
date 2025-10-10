/*
 * Copyright 2025 Esme Povirk for CodeWeavers
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
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winbrand);

LPWSTR WINAPI BrandingFormatString(LPWSTR format)
{
    const WCHAR* env = L"WINDOWS_SHORT=Windows 10\0WINDOWS_LONG=Windows 10 Pro\0";
    NTSTATUS stat;
    SIZE_T len;
    LPWSTR result;

    FIXME("%s: semi-stub\n", debugstr_w(format));

    stat = RtlExpandEnvironmentStrings(env, format, wcslen(format), NULL, 0, &len);
    if (stat != STATUS_SUCCESS && stat != STATUS_BUFFER_TOO_SMALL)
        return NULL;

    result = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!result)
        return NULL;

    stat = RtlExpandEnvironmentStrings(env, format, wcslen(format), result, len, NULL);
    if (stat != STATUS_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, result);
        return NULL;
    }

    return result;
}
