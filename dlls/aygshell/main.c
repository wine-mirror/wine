/*
 * CE aygshell.dll
 *
 * Copyright 2010 André Hentschel
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
#include "windows.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(aygshell);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

BOOL SHCreateMenuBar(void /*SHMENUBARINFO*/ *pmb)
{
    FIXME("stub!\n");
    return TRUE;
}

BOOL SHInitExtraControls(void)
{
    FIXME("stub!\n");
    return TRUE;
}

BOOL SHInitDialog(void*/*PSHINITDLGINFO*/pshidi)
{
    FIXME("stub!\n");
    return TRUE;
}

BOOL SHFullScreen(HWND hwndRequester, DWORD dwState)
{
    FIXME("stub!\n");
    return TRUE;
}

DWORD SHRecognizeGesture(void/*SHRGINFO*/ *shrg)
{
    FIXME("stub!\n");
    return 0;
}

BOOL SHHandleWMSettingChange(HWND hwnd, WPARAM wParam, LPARAM lParam, void/*SHACTIVATEINFO*/ *psai)
{
    FIXME("stub!\n");
    return TRUE;
}
