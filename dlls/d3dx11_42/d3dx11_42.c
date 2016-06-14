/*
 * Copyright 2013 Detlef Riekenberg
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
 *
 */

#include "config.h"
#include "wine/port.h"
#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "d3dx11.h"

BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;       /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hdll);
    }

   return TRUE;
}

/***********************************************************************
 * D3DX11CheckVersion
 *
 * Checks whether we are compiling against the correct d3d and d3dx library.
 */
BOOL WINAPI D3DX11CheckVersion(UINT d3dsdkversion, UINT d3dxsdkversion)
{
    if ((d3dsdkversion == D3D11_SDK_VERSION) && (d3dxsdkversion == 42))
        return TRUE;

    return FALSE;
}
