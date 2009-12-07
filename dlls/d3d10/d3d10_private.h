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
#include "wine/rbtree.h"

#define COBJMACROS
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"

#include "d3d10.h"

/* TRACE helper functions */
const char *debug_d3d10_driver_type(D3D10_DRIVER_TYPE driver_type) DECLSPEC_HIDDEN;
const char *debug_d3d10_shader_variable_class(D3D10_SHADER_VARIABLE_CLASS c) DECLSPEC_HIDDEN;
const char *debug_d3d10_shader_variable_type(D3D10_SHADER_VARIABLE_TYPE t) DECLSPEC_HIDDEN;

void *d3d10_rb_alloc(size_t size) DECLSPEC_HIDDEN;
void *d3d10_rb_realloc(void *ptr, size_t size) DECLSPEC_HIDDEN;
void d3d10_rb_free(void *ptr) DECLSPEC_HIDDEN;

enum d3d10_effect_object_type
{
    D3D10_EOT_VERTEXSHADER = 6,
    D3D10_EOT_PIXELSHADER = 7,
    D3D10_EOT_GEOMETRYSHADER = 8,
};

struct d3d10_effect_object
{
    struct d3d10_effect_pass *pass;
    enum d3d10_effect_object_type type;
    DWORD idx_offset;
    void *data;
};

struct d3d10_effect_shader_variable
{
    char *input_signature;
    UINT input_signature_size;
    union
    {
        ID3D10VertexShader *vs;
        ID3D10PixelShader *ps;
        ID3D10GeometryShader *gs;
    } shader;
};

/* ID3D10EffectType */
struct d3d10_effect_type
{
    const struct ID3D10EffectTypeVtbl *vtbl;

    DWORD id;
    struct wine_rb_entry entry;
    struct d3d10_effect *effect;

    char *name;
    DWORD element_count;
    DWORD size_unpacked;
    DWORD stride;
    DWORD size_packed;
    DWORD member_count;
    DWORD column_count;
    DWORD row_count;
    D3D10_SHADER_VARIABLE_TYPE basetype;
    D3D10_SHADER_VARIABLE_CLASS type_class;
    struct d3d10_effect_type *elementtype;
    struct d3d10_effect_type_member *members;
};

struct d3d10_effect_type_member
{
    char *name;
    char *semantic;
    DWORD buffer_offset;
    struct d3d10_effect_type *type;
};

/* ID3D10EffectVariable */
struct d3d10_effect_variable
{
    const struct ID3D10EffectVariableVtbl *vtbl;

    struct d3d10_effect_variable *buffer;
    struct d3d10_effect *effect;

    char *name;
    char *semantic;
    DWORD buffer_offset;
    DWORD annotation_count;
    DWORD flag;
    DWORD data_size;
    struct d3d10_effect_type *type;
    struct d3d10_effect_variable *elements;
    struct d3d10_effect_variable *members;
    struct d3d10_effect_variable *annotations;
};

/* ID3D10EffectPass */
struct d3d10_effect_pass
{
    const struct ID3D10EffectPassVtbl *vtbl;

    struct d3d10_effect_technique *technique;
    char *name;
    DWORD start;
    DWORD object_count;
    DWORD annotation_count;
    struct d3d10_effect_object *objects;
    struct d3d10_effect_variable *annotations;
};

/* ID3D10EffectTechnique */
struct d3d10_effect_technique
{
    const struct ID3D10EffectTechniqueVtbl *vtbl;

    struct d3d10_effect *effect;
    char *name;
    DWORD pass_count;
    DWORD annotation_count;
    struct d3d10_effect_pass *passes;
    struct d3d10_effect_variable *annotations;
};

/* ID3D10Effect */
extern const struct ID3D10EffectVtbl d3d10_effect_vtbl DECLSPEC_HIDDEN;
struct d3d10_effect
{
    const struct ID3D10EffectVtbl *vtbl;
    LONG refcount;

    ID3D10Device *device;
    DWORD version;
    DWORD local_buffer_count;
    DWORD variable_count;
    DWORD local_variable_count;
    DWORD sharedbuffers_count;
    DWORD sharedobjects_count;
    DWORD technique_count;
    DWORD index_offset;
    DWORD texture_count;
    DWORD dephstencilstate_count;
    DWORD blendstate_count;
    DWORD rasterizerstate_count;
    DWORD samplerstate_count;
    DWORD rendertargetview_count;
    DWORD depthstencilview_count;
    DWORD shader_call_count;
    DWORD shader_compile_count;

    struct wine_rb_tree types;
    struct d3d10_effect_variable *local_buffers;
    struct d3d10_effect_variable *local_variables;
    struct d3d10_effect_technique *techniques;
};

HRESULT d3d10_effect_parse(struct d3d10_effect *This, const void *data, SIZE_T data_size) DECLSPEC_HIDDEN;

/* D3D10Core */
HRESULT WINAPI D3D10CoreCreateDevice(IDXGIFactory *factory, IDXGIAdapter *adapter,
        UINT flags, void *unknown0, ID3D10Device **device);

#endif /* __WINE_D3D10_PRIVATE_H */
