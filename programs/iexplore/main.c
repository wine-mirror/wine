/*
 * Internet Explorer wrapper
 *
 * Copyright 2006 Mike McCormack
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

#include <windows.h>
#include <advpub.h>
#include <ole2.h>
#include <rpcproxy.h>

#include "wine/debug.h"

extern DWORD WINAPI IEWinMain(const WCHAR*, int);

static BOOL check_native_ie(void)
{
    DWORD handle, size;
    LPWSTR file_desc;
    UINT bytes;
    void* buf;
    BOOL ret = TRUE;
    LPWORD translation;
    WCHAR file_desc_strW[48];

    size = GetFileVersionInfoSizeW(L"browseui.dll", &handle);
    if(!size)
        return TRUE;

    buf = HeapAlloc(GetProcessHeap(), 0, size);
    GetFileVersionInfoW(L"browseui.dll", 0, size,buf);
    if (VerQueryValueW(buf, L"\\VarFileInfo\\Translation", (void **)&translation, &bytes))
    {
        wsprintfW(file_desc_strW, L"\\StringFileInfo\\%04x%04x\\FileDescription", translation[0], translation[1]);
        ret = !VerQueryValueW(buf, file_desc_strW, (void**)&file_desc, &bytes) || !wcsstr(file_desc, L"Wine");
    }

    HeapFree(GetProcessHeap(), 0, buf);
    return ret;
}

static DWORD register_iexplore(BOOL doregister)
{
    HRESULT hres;

    if (check_native_ie()) {
        WINE_MESSAGE("Native IE detected, not doing registration\n");
        return 0;
    }

    hres = RegInstallA(NULL, doregister ? "RegisterIE" : "UnregisterIE", NULL);
    return FAILED(hres);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prev, WCHAR *cmdline, int show)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

    if(*cmdline == '-' || *cmdline == '/') {
        if(!wcsicmp(cmdline+1, L"regserver"))
            return register_iexplore(TRUE);
        if(!wcsicmp(cmdline+1, L"unregserver"))
            return register_iexplore(FALSE);
    }

    return IEWinMain(cmdline, show);
}
