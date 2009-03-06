/*
 * Copyright 2008-2009 Henri Verbeet for CodeWeavers
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

#ifndef __WINE_D3D10_PRIVATE_H
#define __WINE_D3D10_PRIVATE_H

#include "wine/debug.h"

#define COBJMACROS
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"

#include "d3d10.h"

/* TRACE helper functions */
const char *debug_d3d10_driver_type(D3D10_DRIVER_TYPE driver_type);

enum d3d10_effect_variable_type
{
    D3D10_EVT_VERTEXSHADER = 6,
    D3D10_EVT_PIXELSHADER = 7,
    D3D10_EVT_GEOMETRYSHADER = 8,
};

struct d3d10_effect_variable
{
    enum d3d10_effect_variable_type type;
    DWORD idx_offset;
    void *data;
};

struct d3d10_effect_shader_variable
{
    char *input_signature;
    UINT input_signature_size;
};

/* ID3D10EffectPass */
struct d3d10_effect_pass
{
    const struct ID3D10EffectPassVtbl *vtbl;

    char *name;
    DWORD start;
    DWORD variable_count;
    struct d3d10_effect_variable *variables;
};

/* ID3D10EffectTechnique */
struct d3d10_effect_technique
{
    const struct ID3D10EffectTechniqueVtbl *vtbl;

    char *name;
    DWORD start;
    DWORD pass_count;
    struct d3d10_effect_pass *passes;
};

/* ID3D10Effect */
extern const struct ID3D10EffectVtbl d3d10_effect_vtbl;
struct d3d10_effect
{
    const struct ID3D10EffectVtbl *vtbl;
    LONG refcount;

    ID3D10Device *device;
    DWORD technique_count;
    DWORD index_offset;
    DWORD blendstate_count;
    struct d3d10_effect_technique *techniques;
};

HRESULT d3d10_effect_parse(struct d3d10_effect *This, const void *data, SIZE_T data_size);

/* D3D10Core */
HRESULT WINAPI D3D10CoreCreateDevice(IDXGIFactory *factory, IDXGIAdapter *adapter,
        UINT flags, DWORD unknown0, ID3D10Device **device);

#endif /* __WINE_D3D10_PRIVATE_H */
