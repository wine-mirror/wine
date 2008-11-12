/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
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

#ifndef __WINE_D3D10CORE_PRIVATE_H
#define __WINE_D3D10CORE_PRIVATE_H

#include "wine/debug.h"

#define COBJMACROS
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"

#include "d3d10.h"

/* TRACE helper functions */
const char *debug_d3d10_primitive_topology(D3D10_PRIMITIVE_TOPOLOGY topology);
const char *debug_dxgi_format(DXGI_FORMAT format);

/* IDirect3D10Device */
extern const struct ID3D10DeviceVtbl d3d10_device_vtbl;
extern const struct IUnknownVtbl d3d10_device_inner_unkown_vtbl;
struct d3d10_device
{
    const struct ID3D10DeviceVtbl *vtbl;
    const struct IUnknownVtbl *inner_unknown_vtbl;
    IUnknown *outer_unknown;
    LONG refcount;
};

#endif /* __WINE_D3D10CORE_PRIVATE_H */
