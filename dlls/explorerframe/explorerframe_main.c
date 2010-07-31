/*
 * ExplorerFrame main functions
 *
 * Copyright 2010 David Hedberg
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"

#include "explorerframe_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(explorerframe);

HINSTANCE explorerframe_hinstance;
LONG EFRAME_refCount = 0;

/*************************************************************************
 *              DllMain (ExplorerFrame.@)
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p, 0x%x, %p\n", hinst, fdwReason, fImpLoad);
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinst);
        explorerframe_hinstance = hinst;
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/*************************************************************************
 *              DllCanUnloadNow (ExplorerFrame.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("refCount is %d\n", EFRAME_refCount);
    return EFRAME_refCount ? S_FALSE : S_OK;
}
