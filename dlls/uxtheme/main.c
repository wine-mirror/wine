/*
 * Win32 5.1 Themes
 *
 * Copyright (C) 2003 Kevin Koltzau
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "uxtheme.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

/* For the moment, do nothing here. */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
	    break;
	case DLL_PROCESS_DETACH:
	    break;
    }
    return TRUE;
}

BOOL WINAPI IsAppThemed(void)
{
    FIXME("\n");
    return FALSE;
}

BOOL WINAPI IsThemeActive(void)
{
    FIXME("\n");
    return FALSE;
}

HRESULT WINAPI EnableThemeDialogTexture(HWND hwnd, DWORD dwFlags) {
    FIXME("%p 0x%08lx\n", hwnd, dwFlags);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

HRESULT WINAPI EnableTheming(BOOL fEnable) {
    FIXME("%d\n", fEnable);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

HTHEME WINAPI OpenThemeData(HWND hwnd, LPCWSTR pszClassList) {
    FIXME("%p %s\n", hwnd, debugstr_w(pszClassList));
    return NULL;
}

HRESULT WINAPI SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName,
                              LPCWSTR pszSubIdList) {
    FIXME("%p %s %s\n", hwnd, debugstr_w(pszSubAppName),
          debugstr_w(pszSubIdList));
    return ERROR_CALL_NOT_IMPLEMENTED;
}
