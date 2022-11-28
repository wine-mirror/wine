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

static HINSTANCE avicap_instance;

static const WCHAR class_name[] = L"wine_avicap_class";

static LRESULT CALLBACK avicap_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        default:
            if (msg >= WM_CAP_START && msg <= WM_CAP_END)
                FIXME("Unhandled message %#x.\n", msg);
            return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

static void register_class(void)
{
    WNDCLASSEXW class =
    {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpfnWndProc = avicap_wndproc,
        .hInstance = avicap_instance,
        .hCursor = LoadCursorW(NULL, (LPWSTR)IDC_ARROW),
        .hbrBackground = (HBRUSH)(COLOR_BTNFACE+1),
        .lpszClassName = class_name,
    };

    if (!RegisterClassExW(&class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        ERR("Failed to register class, error %lu.\n", GetLastError());
}

static void unregister_class(void)
{
    if (!UnregisterClassW(class_name, avicap_instance) && GetLastError() != ERROR_CLASS_DOES_NOT_EXIST)
        ERR("Failed to unregister class, error %lu.\n", GetLastError());
}

/***********************************************************************
 *             capCreateCaptureWindowW   (AVICAP32.@)
 */
HWND VFWAPI capCreateCaptureWindowW(const WCHAR *window_name, DWORD style,
        int x, int y, int width, int height, HWND parent, int id)
{
    TRACE("window_name %s, style %#lx, x %d, y %d, width %d, height %d, parent %p, id %#x.\n",
            debugstr_w(window_name), style, x, y, width, height, parent, id);

    return CreateWindowW(class_name, window_name, style, x, y, width, height, parent, NULL, avicap_instance, NULL);
}

/***********************************************************************
 *             capCreateCaptureWindowA   (AVICAP32.@)
 */
HWND VFWAPI capCreateCaptureWindowA(const char *window_name, DWORD style,
        int x, int y, int width, int height, HWND parent, int id)
{
    UNICODE_STRING nameW;
    HWND window;

    if (window_name)
        RtlCreateUnicodeStringFromAsciiz(&nameW, window_name);
    else
        nameW.Buffer = NULL;

    window = capCreateCaptureWindowW(nameW.Buffer, style, x, y, width, height, parent, id);
    RtlFreeUnicodeString(&nameW);

    return window;
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
    if (WINE_UNIX_CALL(unix_get_device_desc, &params)) return FALSE;

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
            DisableThreadLibraryCalls(instance);
            __wine_init_unix_call();
            register_class();
            avicap_instance = instance;
            break;

        case DLL_PROCESS_DETACH:
            if (!reserved)
                unregister_class();
            break;
    }

    return TRUE;
}
