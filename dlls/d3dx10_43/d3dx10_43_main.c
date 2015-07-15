/*
 * D3DX10 main file
 *
 * Copyright (c) 2010 Owen Rudge for CodeWeavers
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
#include "wine/debug.h"
#include "wine/unicode.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"

#include "d3d10.h"
#include "d3dx10core.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

   return TRUE;
}

/***********************************************************************
 * D3DX10CheckVersion
 *
 * Checks whether we are compiling against the correct d3d and d3dx library.
 */
BOOL WINAPI D3DX10CheckVersion(UINT d3dsdkvers, UINT d3dxsdkvers)
{
    if ((d3dsdkvers == D3D10_SDK_VERSION) && (d3dxsdkvers == 43))
        return TRUE;

    return FALSE;
}

HRESULT WINAPI D3DX10CreateEffectFromFileA(const char *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT hlslflags, UINT fxflags, ID3D10Device *device,
        ID3D10EffectPool *effectpool, ID3DX10ThreadPump *pump, ID3D10Effect **effect, ID3D10Blob **errors,
        HRESULT *hresult)
{
    FIXME("filename %s, defines %p, include %p, profile %s, hlslflags %#x, fxflags %#x, "
            "device %p, effectpool %p, pump %p, effect %p, errors %p, hresult %p\n",
            debugstr_a(filename), defines, include, debugstr_a(profile), hlslflags, fxflags,
            device, effectpool, pump, effect, errors, hresult);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10CreateEffectFromFileW(const WCHAR *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT hlslflags, UINT fxflags, ID3D10Device *device,
        ID3D10EffectPool *effectpool, ID3DX10ThreadPump *pump, ID3D10Effect **effect, ID3D10Blob **errors,
        HRESULT *hresult)
{
    FIXME("filename %s, defines %p, include %p, profile %s, hlslflags %#x, fxflags %#x, "
            "device %p, effectpool %p, pump %p, effect %p, errors %p, hresult %p\n",
            debugstr_w(filename), defines, include, debugstr_a(profile), hlslflags, fxflags, device,
            effectpool, pump, effect, errors, hresult);

    return E_NOTIMPL;
}
