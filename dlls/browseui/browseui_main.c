/*
 * browseui - Internet Explorer / Windows Explorer standard UI
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
 * Copyright 2004 Mike McCormack (for CodeWeavers)
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"

#include "initguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

LONG BROWSEUI_refCount = 0;

HINSTANCE browseui_hinstance = 0;

/*************************************************************************
 * BROWSEUI DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hinst, fdwReason, fImpLoad);
    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;   /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinst);
            browseui_hinstance = hinst;
            break;
    }
    return TRUE;
}

/*************************************************************************
 *              DllCanUnloadNow (BROWSEUI.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return BROWSEUI_refCount ? S_FALSE : S_OK;
}

/***********************************************************************
 *              DllGetVersion (BROWSEUI.@)
 */
HRESULT WINAPI DllGetVersion(DLLVERSIONINFO *info)
{
    if (info->cbSize != sizeof(DLLVERSIONINFO)) FIXME("support DLLVERSIONINFO2\n");

    /* this is what IE6 on Windows 98 reports */
    info->dwMajorVersion = 6;
    info->dwMinorVersion = 0;
    info->dwBuildNumber = 2600;
    info->dwPlatformID = DLLVER_PLATFORM_WINDOWS;

    return NOERROR;
}

/***********************************************************************
 *              DllGetClassObject (BROWSEUI.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *ppv)
{
    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}
