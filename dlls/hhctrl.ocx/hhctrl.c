/*
 * hhctrl implementation
 *
 * Copyright 2004 Krzysztof Foltman
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

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winuser.h"
#include "wine/debug.h"
#include "htmlhelp.h"

WINE_DEFAULT_DEBUG_CHANNEL(htmlhelp);

HWND WINAPI HtmlHelpW(HWND caller, LPCWSTR filename, UINT command, DWORD data)
{
    FIXME("(%p, %s, %d, %ld): stub\n", caller, debugstr_w(filename), command, data);

    /* if command is HH_DISPLAY_TOPIC just display an informative message for now */
    if (command == HH_DISPLAY_TOPIC)
        MessageBoxA( NULL, "HTML Help functionality is currently unimplemented.\n\n"
                     "Try installing Internet Explorer, or using a native hhctrl.ocx with the Mozilla ActiveX control.",
                     "Wine", MB_OK | MB_ICONEXCLAMATION );
    return 0;
}

HWND WINAPI HtmlHelpA(HWND caller, LPCSTR filename, UINT command, DWORD data)
{
    WCHAR *wfile = NULL;
    DWORD len = MultiByteToWideChar( CP_ACP, 0, filename, -1, NULL, 0 );
    HWND result;

    wfile = HeapAlloc( GetProcessHeap(), 0, len  * sizeof(WCHAR));
    MultiByteToWideChar( CP_ACP, 0, filename, -1, wfile, len );

    result = HtmlHelpW( caller, wfile, command, data );

    HeapFree( GetProcessHeap(), 0, wfile );
    return result;
}

int WINAPI doWinMain(HMODULE hMod, LPSTR cmdline)
{
    FIXME("(0x%x %s) stub!\n", (unsigned)hMod, debugstr_a(cmdline));
    return -1;
}
