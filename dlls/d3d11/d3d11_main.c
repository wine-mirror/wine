/*
 * Direct3D 11
 *
 * Copyright 2013 Austin English
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "d3d11.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

static const char *debug_d3d_driver_type(D3D_DRIVER_TYPE driver_type)
{
    switch(driver_type)
    {
#define D3D11_TO_STR(x) case x: return #x
        D3D11_TO_STR(D3D_DRIVER_TYPE_UNKNOWN);
        D3D11_TO_STR(D3D_DRIVER_TYPE_HARDWARE);
        D3D11_TO_STR(D3D_DRIVER_TYPE_REFERENCE);
        D3D11_TO_STR(D3D_DRIVER_TYPE_NULL);
        D3D11_TO_STR(D3D_DRIVER_TYPE_SOFTWARE);
        D3D11_TO_STR(D3D_DRIVER_TYPE_WARP);
#undef D3D11_TO_STR
        default:
            return wine_dbg_sprintf("Unrecognized D3D_DRIVER_TYPE %#x\n", driver_type);
    }
}

HRESULT WINAPI D3D11CreateDevice(IDXGIAdapter *adapter, D3D_DRIVER_TYPE driver_type, HMODULE swrast, UINT flags,
                                 const D3D_FEATURE_LEVEL *feature_levels, UINT levels, UINT sdk_version,
                                 ID3D11Device **device, D3D_FEATURE_LEVEL *feature_level, ID3D11DeviceContext **context)
{
    FIXME("stub: adapter %p, driver_type %s, swrast %p, flags %#x, feature_levels %p, levels %#x, sdk_version %d, "
          "device %p, feature_level %p, context %p\n", adapter, debug_d3d_driver_type(driver_type), swrast,
          flags, feature_levels, levels, sdk_version, device, feature_level, context);
    return E_OUTOFMEMORY;
}
