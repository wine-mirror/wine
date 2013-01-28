/*
 * CSCRIPT Implementation
 *
 * Copyright 2011 André Hentschel
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

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>
#include <winbase.h>

#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(cscript);

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR cmdline, int cmdshow)
{
    static const WCHAR wscriptW[] = {'\\','w','s','c','r','i','p','t','.','e','x','e',0};
    static const WCHAR parbW[] = {' ','/','B',' ',0};
    WCHAR app[MAX_PATH];
    WCHAR cmd[MAX_PATH];
    PROCESS_INFORMATION pi;
    BOOL ret;
    DWORD exitcode;
    STARTUPINFOW si = { sizeof(si) };

    WINE_FIXME("(%p %p %s %x) forwarding to wscript\n", hInst, hPrevInst, wine_dbgstr_w(cmdline), cmdshow);

    GetSystemDirectoryW(app, MAX_PATH);
    strcatW(app, wscriptW);
    strcpyW(cmd, app);
    strcatW(cmd, parbW);
    strcatW(cmd, cmdline);

    if (!CreateProcessW(app, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) return 1;
    WaitForSingleObject( pi.hProcess, INFINITE );

    ret = GetExitCodeProcess(pi.hProcess, &exitcode);
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    if (ret)
        return exitcode;
    else
        return 1;
}
