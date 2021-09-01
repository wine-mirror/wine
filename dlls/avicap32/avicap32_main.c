/*
 * Copyright 2002 Dmitry Timoshkov for CodeWeavers
 * Copyright 2005 Maarten Lankhorst
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
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "vfw.h"
#include "winternl.h"
#include "wine/debug.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(avicap);

static unixlib_handle_t unix_handle;

/***********************************************************************
 *             capCreateCaptureWindowW   (AVICAP32.@)
 */
HWND VFWAPI capCreateCaptureWindowW(LPCWSTR lpszWindowName, DWORD dwStyle, INT x,
                                    INT y, INT nWidth, INT nHeight, HWND hWnd,
                                    INT nID)
{
    FIXME("(%s, %08x, %08x, %08x, %08x, %08x, %p, %08x): stub\n",
           debugstr_w(lpszWindowName), dwStyle, x, y, nWidth, nHeight, hWnd, nID);
    return 0;
}

/***********************************************************************
 *             capCreateCaptureWindowA   (AVICAP32.@)
 */
HWND VFWAPI capCreateCaptureWindowA(LPCSTR lpszWindowName, DWORD dwStyle, INT x,
                                    INT y, INT nWidth, INT nHeight, HWND hWnd,
                                    INT nID)
{   UNICODE_STRING nameW;
    HWND retW;

    if (lpszWindowName) RtlCreateUnicodeStringFromAsciiz(&nameW, lpszWindowName);
    else nameW.Buffer = NULL;

    retW = capCreateCaptureWindowW(nameW.Buffer, dwStyle, x, y, nWidth, nHeight,
                                   hWnd, nID);
    RtlFreeUnicodeString(&nameW);

    return retW;
}

/***********************************************************************
 *             capGetDriverDescriptionA   (AVICAP32.@)
 */
BOOL VFWAPI capGetDriverDescriptionA(WORD wDriverIndex, LPSTR lpszName,
                                     INT cbName, LPSTR lpszVer, INT cbVer)
{
   BOOL retval;
   WCHAR devname[CAP_DESC_MAX], devver[CAP_DESC_MAX];
   TRACE("--> capGetDriverDescriptionW\n");
   retval = capGetDriverDescriptionW(wDriverIndex, devname, CAP_DESC_MAX, devver, CAP_DESC_MAX);
   if (retval) {
      WideCharToMultiByte(CP_ACP, 0, devname, -1, lpszName, cbName, NULL, NULL);
      WideCharToMultiByte(CP_ACP, 0, devver, -1, lpszVer, cbVer, NULL, NULL);
   }
   return retval;
}

/***********************************************************************
 *             capGetDriverDescriptionW   (AVICAP32.@)
 */
BOOL VFWAPI capGetDriverDescriptionW(WORD index, WCHAR *name, int name_len, WCHAR *version, int version_len)
{
    struct get_device_desc_params params;

    params.index = index;
    if (__wine_unix_call(unix_handle, unix_get_device_desc, &params))
        return FALSE;

    TRACE("Found device name %s, version %s.\n", debugstr_w(params.name), debugstr_w(params.version));
    lstrcpynW(name, params.name, name_len);
    lstrcpynW(version, params.version, version_len);
    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            NtQueryVirtualMemory(GetCurrentProcess(), instance,
                    MemoryWineUnixFuncs, &unix_handle, sizeof(unix_handle), NULL);
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}
