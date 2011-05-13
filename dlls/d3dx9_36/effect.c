/*
 * Copyright 2010 Christian Costa
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
#include "wine/port.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "wingdi.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

static const struct ID3DXEffectVtbl ID3DXEffect_Vtbl;
static const struct ID3DXBaseEffectVtbl ID3DXBaseEffect_Vtbl;
static const struct ID3DXEffectCompilerVtbl ID3DXEffectCompiler_Vtbl;

struct d3dx_parameter
{
    struct ID3DXBaseEffectImpl *base;

    char *name;
    char *semantic;
    void *data;
    D3DXPARAMETER_CLASS class;
    D3DXPARAMETER_TYPE  type;
    UINT rows;
    UINT columns;
    UINT element_count;
    UINT annotation_count;
    UINT member_count;
    DWORD flags;
    UINT bytes;

    D3DXHANDLE *annotation_handles;
    D3DXHANDLE *member_handles;
};

struct d3dx_pass
{
    struct ID3DXBaseEffectImpl *base;

    char *name;
    UINT state_count;
    UINT annotation_count;

    D3DXHANDLE *annotation_handles;
};

struct d3dx_technique
{
    struct ID3DXBaseEffectImpl *base;

    char *name;
    UINT pass_count;
    UINT annotation_count;

    D3DXHANDLE *annotation_handles;
    D3DXHANDLE *pass_handles;
};

struct ID3DXBaseEffectImpl
{
    ID3DXBaseEffect ID3DXBaseEffect_iface;
    LONG ref;

    struct ID3DXEffectImpl *effect;

    UINT parameter_count;
    UINT technique_count;
    UINT object_count;

    D3DXHANDLE *parameter_handles;
    D3DXHANDLE *technique_handles;
    D3DXHANDLE *objects;
};

struct ID3DXEffectImpl
{
    ID3DXEffect ID3DXEffect_iface;
    LONG ref;

    LPD3DXEFFECTSTATEMANAGER manager;
    LPDIRECT3DDEVICE9 device;
    LPD3DXEFFECTPOOL pool;

    ID3DXBaseEffect *base_effect;
};

struct ID3DXEffectCompilerImpl
{
    ID3DXEffectCompiler ID3DXEffectCompiler_iface;
    LONG ref;

    ID3DXBaseEffect *base_effect;
};

static struct d3dx_parameter *get_parameter_by_name(struct ID3DXBaseEffectImpl *base,
        struct d3dx_parameter *parameter, LPCSTR name);
static struct d3dx_parameter *get_parameter_annotation_by_name(struct d3dx_parameter *parameter, LPCSTR name);

static inline void read_dword(const char **ptr, DWORD *d)
{
    memcpy(d, *ptr, sizeof(*d));
    *ptr += sizeof(*d);
}

static void skip_dword_unknown(const char **ptr, unsigned int count)
{
    unsigned int i;
    DWORD d;

    FIXME("Skipping %u unknown DWORDs:\n", count);
    for (i = 0; i < count; ++i)
    {
        read_dword(ptr, &d);
        FIXME("\t0x%08x\n", d);
    }
}

static inline struct d3dx_parameter *get_parameter_struct(D3DXHANDLE handle)
{
    return (struct d3dx_parameter *) handle;
}

static inline struct d3dx_pass *get_pass_struct(D3DXHANDLE handle)
{
    return (struct d3dx_pass *) handle;
}

static inline struct d3dx_technique *get_technique_struct(D3DXHANDLE handle)
{
    return (struct d3dx_technique *) handle;
}

static inline D3DXHANDLE get_parameter_handle(struct d3dx_parameter *parameter)
{
    return (D3DXHANDLE) parameter;
}

static inline D3DXHANDLE get_technique_handle(struct d3dx_technique *technique)
{
    return (D3DXHANDLE) technique;
}

static inline D3DXHANDLE get_pass_handle(struct d3dx_pass *pass)
{
    return (D3DXHANDLE) pass;
}

static struct d3dx_technique *is_valid_technique(struct ID3DXBaseEffectImpl *base, D3DXHANDLE technique)
{
    unsigned int i;

    for (i = 0; i < base->technique_count; ++i)
    {
        if (base->technique_handles[i] == technique)
        {
            return get_technique_struct(technique);
        }
    }

    return NULL;
}

static struct d3dx_pass *is_valid_pass(struct ID3DXBaseEffectImpl *base, D3DXHANDLE pass)
{
    unsigned int i, k;

    for (i = 0; i < base->technique_count; ++i)
    {
        struct d3dx_technique *technique = get_technique_struct(base->technique_handles[i]);

        for (k = 0; k < technique->pass_count; ++k)
        {
            if (technique->pass_handles[k] == pass)
            {
                return get_pass_struct(pass);
            }
        }
    }

    return NULL;
}

static struct d3dx_parameter *is_valid_sub_parameter(struct d3dx_parameter *param, D3DXHANDLE parameter)
{
    unsigned int i, count;
    struct d3dx_parameter *p;

    for (i = 0; i < param->annotation_count; ++i)
    {
        if (param->annotation_handles[i] == parameter)
        {
            return get_parameter_struct(parameter);
        }

        p = is_valid_sub_parameter(get_parameter_struct(param->annotation_handles[i]), parameter);
        if (p) return p;
    }

    if (param->element_count) count = param->element_count;
    else count = param->member_count;

    for (i = 0; i < count; ++i)
    {
        if (param->member_handles[i] == parameter)
        {
            return get_parameter_struct(parameter);
        }

        p = is_valid_sub_parameter(get_parameter_struct(param->member_handles[i]), parameter);
        if (p) return p;
    }

    return NULL;
}

static struct d3dx_parameter *is_valid_parameter(struct ID3DXBaseEffectImpl *base, D3DXHANDLE parameter)
{
    unsigned int i, k, m;
    struct d3dx_parameter *p;

    for (i = 0; i < base->parameter_count; ++i)
    {
        if (base->parameter_handles[i] == parameter)
        {
            return get_parameter_struct(parameter);
        }

        p = is_valid_sub_parameter(get_parameter_struct(base->parameter_handles[i]), parameter);
        if (p) return p;
    }

    for (i = 0; i < base->technique_count; ++i)
    {
        struct d3dx_technique *technique = get_technique_struct(base->technique_handles[i]);

        for (k = 0; k < technique->pass_count; ++k)
        {
            struct d3dx_pass *pass = get_pass_struct(technique->pass_handles[k]);

            for (m = 0; m < pass->annotation_count; ++m)
            {
                if (pass->annotation_handles[i] == parameter)
                {
                    return get_parameter_struct(parameter);
                }

                p = is_valid_sub_parameter(get_parameter_struct(pass->annotation_handles[m]), parameter);
                if (p) return p;
            }
        }

        for (k = 0; k < technique->annotation_count; ++k)
        {
            if (technique->annotation_handles[k] == parameter)
            {
                return get_parameter_struct(parameter);
            }

            p = is_valid_sub_parameter(get_parameter_struct(technique->annotation_handles[k]), parameter);
            if (p) return p;
        }
    }

    return NULL;
}

static void free_parameter(D3DXHANDLE handle, BOOL element, BOOL child)
{
    unsigned int i;
    struct d3dx_parameter *param = get_parameter_struct(handle);

    TRACE("Free parameter %p, child %s\n", param, child ? "yes" : "no");

    if (!param)
    {
        return;
    }

    if (param->annotation_handles)
    {
        for (i = 0; i < param->annotation_count; ++i)
        {
            free_parameter(param->annotation_handles[i], FALSE, FALSE);
        }
        HeapFree(GetProcessHeap(), 0, param->annotation_handles);
    }

    if (param->member_handles)
    {
        unsigned int count;

        if (param->element_count) count = param->element_count;
        else count = param->member_count;

        for (i = 0; i < count; ++i)
        {
            free_parameter(param->member_handles[i], param->element_count != 0, TRUE);
        }
        HeapFree(GetProcessHeap(), 0, param->member_handles);
    }

    if (param->class == D3DXPC_OBJECT && !param->element_count)
    {
        switch (param->type)
        {
            case D3DXPT_STRING:
                HeapFree(GetProcessHeap(), 0, *(LPSTR *)param->data);
                break;

            case D3DXPT_TEXTURE:
            case D3DXPT_TEXTURE1D:
            case D3DXPT_TEXTURE2D:
            case D3DXPT_TEXTURE3D:
            case D3DXPT_TEXTURECUBE:
            case D3DXPT_PIXELSHADER:
            case D3DXPT_VERTEXSHADER:
                if (*(IUnknown **)param->data) IUnknown_Release(*(IUnknown **)param->data);
                break;

            default:
                FIXME("Unhandled type %s\n", debug_d3dxparameter_type(param->type));
                break;
        }
    }

    if (!child)
    {
        HeapFree(GetProcessHeap(), 0, param->data);
    }

    /* only the parent has to release name and semantic */
    if (!element)
    {
        HeapFree(GetProcessHeap(), 0, param->name);
        HeapFree(GetProcessHeap(), 0, param->semantic);
    }

    HeapFree(GetProcessHeap(), 0, param);
}

static void free_pass(D3DXHANDLE handle)
{
    unsigned int i;
    struct d3dx_pass *pass = get_pass_struct(handle);

    TRACE("Free pass %p\n", pass);

    if (!pass)
    {
        return;
    }

    if (pass->annotation_handles)
    {
        for (i = 0; i < pass->annotation_count; ++i)
        {
            free_parameter(pass->annotation_handles[i], FALSE, FALSE);
        }
        HeapFree(GetProcessHeap(), 0, pass->annotation_handles);
    }

    HeapFree(GetProcessHeap(), 0, pass->name);
    HeapFree(GetProcessHeap(), 0, pass);
}

static void free_technique(D3DXHANDLE handle)
{
    unsigned int i;
    struct d3dx_technique *technique = get_technique_struct(handle);

    TRACE("Free technique %p\n", technique);

    if (!technique)
    {
        return;
    }

    if (technique->annotation_handles)
    {
        for (i = 0; i < technique->annotation_count; ++i)
        {
            free_parameter(technique->annotation_handles[i], FALSE, FALSE);
        }
        HeapFree(GetProcessHeap(), 0, technique->annotation_handles);
    }

    if (technique->pass_handles)
    {
        for (i = 0; i < technique->pass_count; ++i)
        {
            free_pass(technique->pass_handles[i]);
        }
        HeapFree(GetProcessHeap(), 0, technique->pass_handles);
    }

    HeapFree(GetProcessHeap(), 0, technique->name);
    HeapFree(GetProcessHeap(), 0, technique);
}

static void free_base_effect(struct ID3DXBaseEffectImpl *base)
{
    unsigned int i;

    TRACE("Free base effect %p\n", base);

    if (base->parameter_handles)
    {
        for (i = 0; i < base->parameter_count; ++i)
        {
            free_parameter(base->parameter_handles[i], FALSE, FALSE);
        }
        HeapFree(GetProcessHeap(), 0, base->parameter_handles);
    }

    if (base->technique_handles)
    {
        for (i = 0; i < base->technique_count; ++i)
        {
            free_technique(base->technique_handles[i]);
        }
        HeapFree(GetProcessHeap(), 0, base->technique_handles);
    }
}

static void free_effect(struct ID3DXEffectImpl *effect)
{
    TRACE("Free effect %p\n", effect);

    if (effect->base_effect)
    {
        effect->base_effect->lpVtbl->Release(effect->base_effect);
    }

    if (effect->pool)
    {
        effect->pool->lpVtbl->Release(effect->pool);
    }

    if (effect->manager)
    {
        IUnknown_Release(effect->manager);
    }

    IDirect3DDevice9_Release(effect->device);
}

static void free_effect_compiler(struct ID3DXEffectCompilerImpl *compiler)
{
    TRACE("Free effect compiler %p\n", compiler);

    if (compiler->base_effect)
    {
        compiler->base_effect->lpVtbl->Release(compiler->base_effect);
    }
}

static INT get_int(D3DXPARAMETER_TYPE type, void *data)
{
    INT i;

    switch (type)
    {
        case D3DXPT_FLOAT:
            i = *(FLOAT *)data;
            break;

        case D3DXPT_INT:
            i = *(INT *)data;
            break;

        case D3DXPT_BOOL:
            i = *(BOOL *)data;
            break;

        default:
            i = 0;
            FIXME("Unhandled type %s. This should not happen!\n", debug_d3dxparameter_type(type));
            break;
    }

    return i;
}

inline static FLOAT get_float(D3DXPARAMETER_TYPE type, void *data)
{
    FLOAT f;

    switch (type)
    {
        case D3DXPT_FLOAT:
            f = *(FLOAT *)data;
            break;

        case D3DXPT_INT:
            f = *(INT *)data;
            break;

        case D3DXPT_BOOL:
            f = *(BOOL *)data;
            break;

        default:
            f = 0.0f;
            FIXME("Unhandled type %s. This should not happen!\n", debug_d3dxparameter_type(type));
            break;
    }

    return f;
}

static inline BOOL get_bool(void *data)
{
    return (*(DWORD *)data) ? TRUE : FALSE;
}

static struct d3dx_parameter *get_parameter_element_by_name(struct d3dx_parameter *parameter, LPCSTR name)
{
    UINT element;
    struct d3dx_parameter *temp_parameter;
    LPCSTR part;

    TRACE("parameter %p, name %s\n", parameter, debugstr_a(name));

    if (!name || !*name) return parameter;

    element = atoi(name);
    part = strchr(name, ']') + 1;

    if (parameter->element_count > element)
    {
        temp_parameter = get_parameter_struct(parameter->member_handles[element]);

        switch (*part++)
        {
            case '.':
                return get_parameter_by_name(NULL, temp_parameter, part);

            case '@':
                return get_parameter_annotation_by_name(temp_parameter, part);

            case '\0':
                TRACE("Returning parameter %p\n", temp_parameter);
                return temp_parameter;

            default:
                FIXME("Unhandled case \"%c\"\n", *--part);
                break;
        }
    }

    TRACE("Parameter not found\n");
    return NULL;
}

static struct d3dx_parameter *get_parameter_annotation_by_name(struct d3dx_parameter *parameter, LPCSTR name)
{
    UINT i, length;
    struct d3dx_parameter *temp_parameter;
    LPCSTR part;

    TRACE("parameter %p, name %s\n", parameter, debugstr_a(name));

    if (!name || !*name) return parameter;

    length = strcspn( name, "[.@" );
    part = name + length;

    for (i = 0; i < parameter->annotation_count; ++i)
    {
        temp_parameter = get_parameter_struct(parameter->annotation_handles[i]);

        if (!strcmp(temp_parameter->name, name))
        {
            TRACE("Returning parameter %p\n", temp_parameter);
            return temp_parameter;
        }
        else if (strlen(temp_parameter->name) == length && !strncmp(temp_parameter->name, name, length))
        {
            switch (*part++)
            {
                case '.':
                    return get_parameter_by_name(NULL, temp_parameter, part);

                case '[':
                    return get_parameter_element_by_name(temp_parameter, part);

                default:
                    FIXME("Unhandled case \"%c\"\n", *--part);
                    break;
            }
        }
    }

    TRACE("Parameter not found\n");
    return NULL;
}

static struct d3dx_parameter *get_parameter_by_name(struct ID3DXBaseEffectImpl *base,
        struct d3dx_parameter *parameter, LPCSTR name)
{
    UINT i, count, length;
    struct d3dx_parameter *temp_parameter;
    D3DXHANDLE *handles;
    LPCSTR part;

    TRACE("base %p, parameter %p, name %s\n", base, parameter, debugstr_a(name));

    if (!name || !*name) return parameter;

    if (!parameter)
    {
        count = base->parameter_count;
        handles = base->parameter_handles;
    }
    else
    {
        count = parameter->member_count;
        handles = parameter->member_handles;
    }

    length = strcspn( name, "[.@" );
    part = name + length;

    for (i = 0; i < count; i++)
    {
        temp_parameter = get_parameter_struct(handles[i]);

        if (!strcmp(temp_parameter->name, name))
        {
            TRACE("Returning parameter %p\n", temp_parameter);
            return temp_parameter;
        }
        else if (strlen(temp_parameter->name) == length && !strncmp(temp_parameter->name, name, length))
        {
            switch (*part++)
            {
                case '.':
                    return get_parameter_by_name(NULL, temp_parameter, part);

                case '@':
                    return get_parameter_annotation_by_name(temp_parameter, part);

                case '[':
                    return get_parameter_element_by_name(temp_parameter, part);

                default:
                    FIXME("Unhandled case \"%c\"\n", *--part);
                    break;
            }
        }
    }

    TRACE("Parameter not found\n");
    return NULL;
}

static inline DWORD d3dx9_effect_version(DWORD major, DWORD minor)
{
    return (0xfeff0000 | ((major) << 8) | (minor));
}

static inline struct ID3DXBaseEffectImpl *impl_from_ID3DXBaseEffect(ID3DXBaseEffect *iface)
{
    return CONTAINING_RECORD(iface, struct ID3DXBaseEffectImpl, ID3DXBaseEffect_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXBaseEffectImpl_QueryInterface(ID3DXBaseEffect *iface, REFIID riid, void **object)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    TRACE("iface %p, riid %s, object %p\n", This, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXBaseEffect))
    {
        This->ID3DXBaseEffect_iface.lpVtbl->AddRef(iface);
        *object = This;
        return S_OK;
    }

    ERR("Interface %s not found\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXBaseEffectImpl_AddRef(ID3DXBaseEffect *iface)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    TRACE("iface %p: AddRef from %u\n", iface, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXBaseEffectImpl_Release(ID3DXBaseEffect *iface)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("iface %p: Release from %u\n", iface, ref + 1);

    if (!ref)
    {
        free_base_effect(This);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ID3DXBaseEffect methods ***/
static HRESULT WINAPI ID3DXBaseEffectImpl_GetDesc(ID3DXBaseEffect *iface, D3DXEFFECT_DESC *desc)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, desc %p partial stub\n", This, desc);

    if (!desc)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }

    /* Todo: add creator and function count */
    desc->Creator = NULL;
    desc->Functions = 0;
    desc->Parameters = This->parameter_count;
    desc->Techniques = This->technique_count;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetParameterDesc(ID3DXBaseEffect *iface, D3DXHANDLE parameter, D3DXPARAMETER_DESC *desc)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, parameter);

    TRACE("iface %p, parameter %p, desc %p\n", This, parameter, desc);

    if (!param) param = get_parameter_struct(iface->lpVtbl->GetParameterByName(iface, NULL, parameter));

    if (!desc || !param)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }

    desc->Name = param->name;
    desc->Semantic = param->semantic;
    desc->Class = param->class;
    desc->Type = param->type;
    desc->Rows = param->rows;
    desc->Columns = param->columns;
    desc->Elements = param->element_count;
    desc->Annotations = param->annotation_count;
    desc->StructMembers = param->member_count;
    desc->Flags = param->flags;
    desc->Bytes = param->bytes;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetTechniqueDesc(ID3DXBaseEffect *iface, D3DXHANDLE technique, D3DXTECHNIQUE_DESC *desc)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_technique *tech = technique ? is_valid_technique(This, technique) : get_technique_struct(This->technique_handles[0]);

    TRACE("iface %p, technique %p, desc %p\n", This, technique, desc);

    if (!desc || !tech)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }

    desc->Name = tech->name;
    desc->Passes = tech->pass_count;
    desc->Annotations = tech->annotation_count;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetPassDesc(ID3DXBaseEffect *iface, D3DXHANDLE pass, D3DXPASS_DESC *desc)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_pass *p = is_valid_pass(This, pass);

    TRACE("iface %p, pass %p, desc %p\n", This, pass, desc);

    if (!desc || !p)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }

    desc->Name = p->name;
    desc->Annotations = p->annotation_count;

    FIXME("Pixel shader and vertex shader are not supported, yet.\n");
    desc->pVertexShaderFunction = NULL;
    desc->pVertexShaderFunction = NULL;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetFunctionDesc(ID3DXBaseEffect *iface, D3DXHANDLE shader, D3DXFUNCTION_DESC *desc)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, shader %p, desc %p stub\n", This, shader, desc);

    return E_NOTIMPL;
}

static D3DXHANDLE WINAPI ID3DXBaseEffectImpl_GetParameter(ID3DXBaseEffect *iface, D3DXHANDLE parameter, UINT index)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, parameter);

    TRACE("iface %p, parameter %p, index %u\n", This, parameter, index);

    if (!param) param = get_parameter_by_name(This, NULL, parameter);

    if (!parameter)
    {
        if (index < This->parameter_count)
        {
            TRACE("Returning parameter %p\n", This->parameter_handles[index]);
            return This->parameter_handles[index];
        }
    }
    else
    {
        if (param && !param->element_count && index < param->member_count)
        {
            TRACE("Returning parameter %p\n", param->member_handles[index]);
            return param->member_handles[index];
        }
    }

    WARN("Invalid argument specified.\n");

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXBaseEffectImpl_GetParameterByName(ID3DXBaseEffect *iface, D3DXHANDLE parameter, LPCSTR name)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, parameter);
    D3DXHANDLE handle;

    TRACE("iface %p, parameter %p, name %s\n", This, parameter, debugstr_a(name));

    if (!param) param = get_parameter_by_name(This, NULL, parameter);

    if (!name)
    {
        handle = get_parameter_handle(param);
        TRACE("Returning parameter %p\n", handle);
        return handle;
    }

    handle = get_parameter_handle(get_parameter_by_name(This, param, name));
    TRACE("Returning parameter %p\n", handle);

    return handle;
}

static D3DXHANDLE WINAPI ID3DXBaseEffectImpl_GetParameterBySemantic(ID3DXBaseEffect *iface, D3DXHANDLE parameter, LPCSTR semantic)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, parameter);
    struct d3dx_parameter *temp_param;
    UINT i;

    TRACE("iface %p, parameter %p, semantic %s\n", This, parameter, debugstr_a(semantic));

    if (!param) param = get_parameter_by_name(This, NULL, parameter);

    if (!parameter)
    {
        for (i = 0; i < This->parameter_count; ++i)
        {
            temp_param = get_parameter_struct(This->parameter_handles[i]);

            if (!temp_param->semantic)
            {
                if (!semantic)
                {
                    TRACE("Returning parameter %p\n", This->parameter_handles[i]);
                    return This->parameter_handles[i];
                }
                continue;
            }

            if (!strcasecmp(temp_param->semantic, semantic))
            {
                TRACE("Returning parameter %p\n", This->parameter_handles[i]);
                return This->parameter_handles[i];
            }
        }
    }
    else if (param)
    {
        for (i = 0; i < param->member_count; ++i)
        {
            temp_param = get_parameter_struct(param->member_handles[i]);

            if (!temp_param->semantic)
            {
                if (!semantic)
                {
                    TRACE("Returning parameter %p\n", param->member_handles[i]);
                    return param->member_handles[i];
                }
                continue;
            }

            if (!strcasecmp(temp_param->semantic, semantic))
            {
                TRACE("Returning parameter %p\n", param->member_handles[i]);
                return param->member_handles[i];
            }
        }
    }

    WARN("Invalid argument specified\n");

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXBaseEffectImpl_GetParameterElement(ID3DXBaseEffect *iface, D3DXHANDLE parameter, UINT index)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, parameter);

    TRACE("iface %p, parameter %p, index %u\n", This, parameter, index);

    if (!param) param = get_parameter_by_name(This, NULL, parameter);

    if (!param)
    {
        if (index < This->parameter_count)
        {
            TRACE("Returning parameter %p\n", This->parameter_handles[index]);
            return This->parameter_handles[index];
        }
    }
    else
    {
        if (index < param->element_count)
        {
            TRACE("Returning parameter %p\n", param->member_handles[index]);
            return param->member_handles[index];
        }
    }

    WARN("Invalid argument specified\n");

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXBaseEffectImpl_GetTechnique(ID3DXBaseEffect *iface, UINT index)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    TRACE("iface %p, index %u\n", This, index);

    if (index >= This->technique_count)
    {
        WARN("Invalid argument specified.\n");
        return NULL;
    }

    TRACE("Returning technique %p\n", This->technique_handles[index]);

    return This->technique_handles[index];
}

static D3DXHANDLE WINAPI ID3DXBaseEffectImpl_GetTechniqueByName(ID3DXBaseEffect *iface, LPCSTR name)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    unsigned int i;

    TRACE("iface %p, name %s stub\n", This, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid argument specified.\n");
        return NULL;
    }

    for (i = 0; i < This->technique_count; ++i)
    {
        struct d3dx_technique *tech = get_technique_struct(This->technique_handles[i]);

        if (!strcmp(tech->name, name))
        {
            TRACE("Returning technique %p\n", This->technique_handles[i]);
            return This->technique_handles[i];
        }
    }

    WARN("Invalid argument specified.\n");

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXBaseEffectImpl_GetPass(ID3DXBaseEffect *iface, D3DXHANDLE technique, UINT index)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_technique *tech = is_valid_technique(This, technique);

    TRACE("iface %p, technique %p, index %u\n", This, technique, index);

    if (!tech) tech = get_technique_struct(iface->lpVtbl->GetTechniqueByName(iface, technique));

    if (tech && index < tech->pass_count)
    {
        TRACE("Returning pass %p\n", tech->pass_handles[index]);
        return tech->pass_handles[index];
    }

    WARN("Invalid argument specified.\n");

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXBaseEffectImpl_GetPassByName(ID3DXBaseEffect *iface, D3DXHANDLE technique, LPCSTR name)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_technique *tech = is_valid_technique(This, technique);

    TRACE("iface %p, technique %p, name %s\n", This, technique, debugstr_a(name));

    if (!tech) tech = get_technique_struct(iface->lpVtbl->GetTechniqueByName(iface, technique));

    if (tech && name)
    {
        unsigned int i;

        for (i = 0; i < tech->pass_count; ++i)
        {
            struct d3dx_pass *pass = get_pass_struct(tech->pass_handles[i]);

            if (!strcmp(pass->name, name))
            {
                TRACE("Returning pass %p\n", tech->pass_handles[i]);
                return tech->pass_handles[i];
            }
        }
    }

    WARN("Invalid argument specified.\n");

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXBaseEffectImpl_GetFunction(ID3DXBaseEffect *iface, UINT index)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, index %u stub\n", This, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXBaseEffectImpl_GetFunctionByName(ID3DXBaseEffect *iface, LPCSTR name)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, name %s stub\n", This, debugstr_a(name));

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXBaseEffectImpl_GetAnnotation(ID3DXBaseEffect *iface, D3DXHANDLE object, UINT index)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, object);
    struct d3dx_pass *pass = is_valid_pass(This, object);
    struct d3dx_technique *technique = is_valid_technique(This, object);
    UINT annotation_count = 0;
    D3DXHANDLE *annotation_handles = NULL;

    FIXME("iface %p, object %p, index %u partial stub\n", This, object, index);

    if (pass)
    {
        annotation_count = pass->annotation_count;
        annotation_handles = pass->annotation_handles;
    }
    else if (technique)
    {
        annotation_count = technique->annotation_count;
        annotation_handles = technique->annotation_handles;
    }
    else
    {
        if (!param) param = get_parameter_by_name(This, NULL, object);

        if (param)
        {
            annotation_count = param->annotation_count;
            annotation_handles = param->annotation_handles;
        }
    }
    /* Todo: add funcs */

    if (index < annotation_count)
    {
        TRACE("Returning parameter %p\n", annotation_handles[index]);
        return annotation_handles[index];
    }

    WARN("Invalid argument specified\n");

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXBaseEffectImpl_GetAnnotationByName(ID3DXBaseEffect *iface, D3DXHANDLE object, LPCSTR name)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, object);
    struct d3dx_pass *pass = is_valid_pass(This, object);
    struct d3dx_technique *technique = is_valid_technique(This, object);
    UINT annotation_count = 0, i;
    D3DXHANDLE *annotation_handles = NULL;

    FIXME("iface %p, object %p, name %s partial stub\n", This, object, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid argument specified\n");
        return NULL;
    }

    if (pass)
    {
        annotation_count = pass->annotation_count;
        annotation_handles = pass->annotation_handles;
    }
    else if (technique)
    {
        annotation_count = technique->annotation_count;
        annotation_handles = technique->annotation_handles;
    }
    else
    {
        if (!param) param = get_parameter_by_name(This, NULL, object);

        if (param)
        {
            annotation_count = param->annotation_count;
            annotation_handles = param->annotation_handles;
        }
    }
    /* Todo: add funcs */

    for (i = 0; i < annotation_count; i++)
    {
        struct d3dx_parameter *anno = get_parameter_struct(annotation_handles[i]);

        if (!strcmp(anno->name, name))
        {
            TRACE("Returning parameter %p\n", anno);
            return get_parameter_handle(anno);
        }
    }

    WARN("Invalid argument specified\n");

    return NULL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetValue(ID3DXBaseEffect *iface, D3DXHANDLE parameter, LPCVOID data, UINT bytes)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, data %p, bytes %u stub\n", This, parameter, data, bytes);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetValue(ID3DXBaseEffect *iface, D3DXHANDLE parameter, LPVOID data, UINT bytes)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, parameter);

    TRACE("iface %p, parameter %p, data %p, bytes %u\n", This, parameter, data, bytes);

    if (!param) param = get_parameter_by_name(This, NULL, parameter);

    if (data && param && param->data && param->bytes <= bytes)
    {
        if (param->type == D3DXPT_VERTEXSHADER || param->type == D3DXPT_PIXELSHADER
                || param->type == D3DXPT_TEXTURE || param->type == D3DXPT_TEXTURE1D
                || param->type == D3DXPT_TEXTURE2D || param->type ==  D3DXPT_TEXTURE3D
                || param->type == D3DXPT_TEXTURECUBE)
        {
            UINT i;

            for (i = 0; i < (param->element_count ? param->element_count : 1); ++i)
            {
                IUnknown *unk = ((IUnknown **)param->data)[i];
                if (unk) IUnknown_AddRef(unk);
                ((IUnknown **)data)[i] = unk;
            }
        }
        else
        {
            TRACE("Copy %u bytes\n", param->bytes);
            memcpy(data, param->data, param->bytes);
        }
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetBool(ID3DXBaseEffect *iface, D3DXHANDLE parameter, BOOL b)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, b %u stub\n", This, parameter, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetBool(ID3DXBaseEffect *iface, D3DXHANDLE parameter, BOOL *b)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, parameter);

    TRACE("iface %p, parameter %p, b %p\n", This, parameter, b);

    if (!param) param = get_parameter_by_name(This, NULL, parameter);

    if (b && param && !param->element_count && param->class == D3DXPC_SCALAR)
    {
        *b = get_bool(param->data);
        TRACE("Returning %s\n", *b ? "TRUE" : "FALSE");
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetBoolArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, CONST BOOL *b, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, b %p, count %u stub\n", This, parameter, b, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetBoolArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, BOOL *b, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, b %p, count %u stub\n", This, parameter, b, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetInt(ID3DXBaseEffect *iface, D3DXHANDLE parameter, INT n)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, n %u stub\n", This, parameter, n);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetInt(ID3DXBaseEffect *iface, D3DXHANDLE parameter, INT *n)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, parameter);

    TRACE("iface %p, parameter %p, n %p\n", This, parameter, n);

    if (!param) param = get_parameter_by_name(This, NULL, parameter);

    if (n && param && !param->element_count && param->class == D3DXPC_SCALAR)
    {
        *n = get_int(param->type, param->data);
        TRACE("Returning %i\n", *n);
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetIntArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, CONST INT *n, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, n %p, count %u stub\n", This, parameter, n, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetIntArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, INT *n, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, n %p, count %u stub\n", This, parameter, n, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetFloat(ID3DXBaseEffect *iface, D3DXHANDLE parameter, FLOAT f)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, f %f stub\n", This, parameter, f);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetFloat(ID3DXBaseEffect *iface, D3DXHANDLE parameter, FLOAT *f)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, parameter);

    TRACE("iface %p, parameter %p, f %p\n", This, parameter, f);

    if (!param) param = get_parameter_by_name(This, NULL, parameter);

    if (f && param && !param->element_count && param->class == D3DXPC_SCALAR)
    {
        f = param->data;
        TRACE("Returning %f\n", *f);
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetFloatArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, CONST FLOAT *f, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, f %p, count %u stub\n", This, parameter, f, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetFloatArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, FLOAT *f, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, f %p, count %u stub\n", This, parameter, f, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetVector(ID3DXBaseEffect* iface, D3DXHANDLE parameter, CONST D3DXVECTOR4* vector)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, vector %p stub\n", This, parameter, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetVector(ID3DXBaseEffect *iface, D3DXHANDLE parameter, D3DXVECTOR4 *vector)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, vector %p stub\n", This, parameter, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetVectorArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, CONST D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, vector %p, count %u stub\n", This, parameter, vector, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetVectorArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, vector %p, count %u stub\n", This, parameter, vector, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetMatrix(ID3DXBaseEffect *iface, D3DXHANDLE parameter, CONST D3DXMATRIX *matrix)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, matrix %p stub\n", This, parameter, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetMatrix(ID3DXBaseEffect *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, matrix %p stub\n", This, parameter, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetMatrixArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, CONST D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, matrix %p, count %u stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetMatrixArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, matrix %p, count %u stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetMatrixPointerArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, CONST D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, matrix %p, count %u stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetMatrixPointerArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, matrix %p, count %u stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetMatrixTranspose(ID3DXBaseEffect *iface, D3DXHANDLE parameter, CONST D3DXMATRIX *matrix)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, matrix %p stub\n", This, parameter, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetMatrixTranspose(ID3DXBaseEffect *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, matrix %p stub\n", This, parameter, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetMatrixTransposeArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, CONST D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, matrix %p, count %u stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetMatrixTransposeArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, matrix %p, count %u stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetMatrixTransposePointerArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, CONST D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, matrix %p, count %u stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetMatrixTransposePointerArray(ID3DXBaseEffect *iface, D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, matrix %p, count %u stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetString(ID3DXBaseEffect *iface, D3DXHANDLE parameter, LPCSTR string)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, string %p stub\n", This, parameter, string);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetString(ID3DXBaseEffect *iface, D3DXHANDLE parameter, LPCSTR *string)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, parameter);

    TRACE("iface %p, parameter %p, string %p\n", This, parameter, string);

    if (!param) param = get_parameter_by_name(This, NULL, parameter);

    if (string && param && !param->element_count && param->type == D3DXPT_STRING)
    {
        *string = *(LPCSTR *)param->data;
        TRACE("Returning %s\n", debugstr_a(*string));
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetTexture(ID3DXBaseEffect *iface, D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9 texture)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, texture %p stub\n", This, parameter, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetTexture(ID3DXBaseEffect *iface, D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9 *texture)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, parameter);

    TRACE("iface %p, parameter %p, texture %p\n", This, parameter, texture);

    if (texture && param && !param->element_count &&
            (param->type == D3DXPT_TEXTURE || param->type == D3DXPT_TEXTURE1D
            || param->type == D3DXPT_TEXTURE2D || param->type ==  D3DXPT_TEXTURE3D
            || param->type == D3DXPT_TEXTURECUBE))
    {
        *texture = *(LPDIRECT3DBASETEXTURE9 *)param->data;
        if (*texture) IDirect3DBaseTexture9_AddRef(*texture);
        TRACE("Returning %p\n", *texture);
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetPixelShader(ID3DXBaseEffect *iface, D3DXHANDLE parameter, LPDIRECT3DPIXELSHADER9 *pshader)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, parameter);

    TRACE("iface %p, parameter %p, pshader %p\n", This, parameter, pshader);

    if (!param) param = get_parameter_by_name(This, NULL, parameter);

    if (pshader && param && !param->element_count && param->type == D3DXPT_PIXELSHADER)
    {
        *pshader = *(LPDIRECT3DPIXELSHADER9 *)param->data;
        if (*pshader) IDirect3DPixelShader9_AddRef(*pshader);
        TRACE("Returning %p\n", *pshader);
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_GetVertexShader(ID3DXBaseEffect *iface, D3DXHANDLE parameter, LPDIRECT3DVERTEXSHADER9 *vshader)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);
    struct d3dx_parameter *param = is_valid_parameter(This, parameter);

    TRACE("iface %p, parameter %p, vshader %p\n", This, parameter, vshader);

    if (!param) param = get_parameter_by_name(This, NULL, parameter);

    if (vshader && param && !param->element_count && param->type == D3DXPT_VERTEXSHADER)
    {
        *vshader = *(LPDIRECT3DVERTEXSHADER9 *)param->data;
        if (*vshader) IDirect3DVertexShader9_AddRef(*vshader);
        TRACE("Returning %p\n", *vshader);
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI ID3DXBaseEffectImpl_SetArrayRange(ID3DXBaseEffect *iface, D3DXHANDLE parameter, UINT start, UINT end)
{
    struct ID3DXBaseEffectImpl *This = impl_from_ID3DXBaseEffect(iface);

    FIXME("iface %p, parameter %p, start %u, end %u stub\n", This, parameter, start, end);

    return E_NOTIMPL;
}

static const struct ID3DXBaseEffectVtbl ID3DXBaseEffect_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXBaseEffectImpl_QueryInterface,
    ID3DXBaseEffectImpl_AddRef,
    ID3DXBaseEffectImpl_Release,
    /*** ID3DXBaseEffect methods ***/
    ID3DXBaseEffectImpl_GetDesc,
    ID3DXBaseEffectImpl_GetParameterDesc,
    ID3DXBaseEffectImpl_GetTechniqueDesc,
    ID3DXBaseEffectImpl_GetPassDesc,
    ID3DXBaseEffectImpl_GetFunctionDesc,
    ID3DXBaseEffectImpl_GetParameter,
    ID3DXBaseEffectImpl_GetParameterByName,
    ID3DXBaseEffectImpl_GetParameterBySemantic,
    ID3DXBaseEffectImpl_GetParameterElement,
    ID3DXBaseEffectImpl_GetTechnique,
    ID3DXBaseEffectImpl_GetTechniqueByName,
    ID3DXBaseEffectImpl_GetPass,
    ID3DXBaseEffectImpl_GetPassByName,
    ID3DXBaseEffectImpl_GetFunction,
    ID3DXBaseEffectImpl_GetFunctionByName,
    ID3DXBaseEffectImpl_GetAnnotation,
    ID3DXBaseEffectImpl_GetAnnotationByName,
    ID3DXBaseEffectImpl_SetValue,
    ID3DXBaseEffectImpl_GetValue,
    ID3DXBaseEffectImpl_SetBool,
    ID3DXBaseEffectImpl_GetBool,
    ID3DXBaseEffectImpl_SetBoolArray,
    ID3DXBaseEffectImpl_GetBoolArray,
    ID3DXBaseEffectImpl_SetInt,
    ID3DXBaseEffectImpl_GetInt,
    ID3DXBaseEffectImpl_SetIntArray,
    ID3DXBaseEffectImpl_GetIntArray,
    ID3DXBaseEffectImpl_SetFloat,
    ID3DXBaseEffectImpl_GetFloat,
    ID3DXBaseEffectImpl_SetFloatArray,
    ID3DXBaseEffectImpl_GetFloatArray,
    ID3DXBaseEffectImpl_SetVector,
    ID3DXBaseEffectImpl_GetVector,
    ID3DXBaseEffectImpl_SetVectorArray,
    ID3DXBaseEffectImpl_GetVectorArray,
    ID3DXBaseEffectImpl_SetMatrix,
    ID3DXBaseEffectImpl_GetMatrix,
    ID3DXBaseEffectImpl_SetMatrixArray,
    ID3DXBaseEffectImpl_GetMatrixArray,
    ID3DXBaseEffectImpl_SetMatrixPointerArray,
    ID3DXBaseEffectImpl_GetMatrixPointerArray,
    ID3DXBaseEffectImpl_SetMatrixTranspose,
    ID3DXBaseEffectImpl_GetMatrixTranspose,
    ID3DXBaseEffectImpl_SetMatrixTransposeArray,
    ID3DXBaseEffectImpl_GetMatrixTransposeArray,
    ID3DXBaseEffectImpl_SetMatrixTransposePointerArray,
    ID3DXBaseEffectImpl_GetMatrixTransposePointerArray,
    ID3DXBaseEffectImpl_SetString,
    ID3DXBaseEffectImpl_GetString,
    ID3DXBaseEffectImpl_SetTexture,
    ID3DXBaseEffectImpl_GetTexture,
    ID3DXBaseEffectImpl_GetPixelShader,
    ID3DXBaseEffectImpl_GetVertexShader,
    ID3DXBaseEffectImpl_SetArrayRange,
};

static inline struct ID3DXEffectImpl *impl_from_ID3DXEffect(ID3DXEffect *iface)
{
    return CONTAINING_RECORD(iface, struct ID3DXEffectImpl, ID3DXEffect_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXEffectImpl_QueryInterface(ID3DXEffect *iface, REFIID riid, void **object)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXEffect))
    {
        This->ID3DXEffect_iface.lpVtbl->AddRef(iface);
        *object = This;
        return S_OK;
    }

    ERR("Interface %s not found\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXEffectImpl_AddRef(ID3DXEffect *iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    TRACE("(%p)->(): AddRef from %u\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXEffectImpl_Release(ID3DXEffect *iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Release from %u\n", This, ref + 1);

    if (!ref)
    {
        free_effect(This);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ID3DXBaseEffect methods ***/
static HRESULT WINAPI ID3DXEffectImpl_GetDesc(ID3DXEffect *iface, D3DXEFFECT_DESC *desc)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetDesc(base, desc);
}

static HRESULT WINAPI ID3DXEffectImpl_GetParameterDesc(ID3DXEffect *iface, D3DXHANDLE parameter, D3DXPARAMETER_DESC *desc)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetParameterDesc(base, parameter, desc);
}

static HRESULT WINAPI ID3DXEffectImpl_GetTechniqueDesc(ID3DXEffect *iface, D3DXHANDLE technique, D3DXTECHNIQUE_DESC *desc)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetTechniqueDesc(base, technique, desc);
}

static HRESULT WINAPI ID3DXEffectImpl_GetPassDesc(ID3DXEffect *iface, D3DXHANDLE pass, D3DXPASS_DESC *desc)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetPassDesc(base, pass, desc);
}

static HRESULT WINAPI ID3DXEffectImpl_GetFunctionDesc(ID3DXEffect *iface, D3DXHANDLE shader, D3DXFUNCTION_DESC *desc)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetFunctionDesc(base, shader, desc);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetParameter(ID3DXEffect *iface, D3DXHANDLE parameter, UINT index)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetParameter(base, parameter, index);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetParameterByName(ID3DXEffect *iface, D3DXHANDLE parameter, LPCSTR name)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetParameterByName(base, parameter, name);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetParameterBySemantic(ID3DXEffect *iface, D3DXHANDLE parameter, LPCSTR semantic)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetParameterBySemantic(base, parameter, semantic);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetParameterElement(ID3DXEffect *iface, D3DXHANDLE parameter, UINT index)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetParameterElement(base, parameter, index);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetTechnique(ID3DXEffect *iface, UINT index)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetTechnique(base, index);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetTechniqueByName(ID3DXEffect *iface, LPCSTR name)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetTechniqueByName(base, name);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetPass(ID3DXEffect *iface, D3DXHANDLE technique, UINT index)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetPass(base, technique, index);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetPassByName(ID3DXEffect *iface, D3DXHANDLE technique, LPCSTR name)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetPassByName(base, technique, name);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetFunction(ID3DXEffect *iface, UINT index)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetFunction(base, index);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetFunctionByName(ID3DXEffect *iface, LPCSTR name)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetFunctionByName(base, name);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetAnnotation(ID3DXEffect *iface, D3DXHANDLE object, UINT index)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetAnnotation(base, object, index);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetAnnotationByName(ID3DXEffect *iface, D3DXHANDLE object, LPCSTR name)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetAnnotationByName(base, object, name);
}

static HRESULT WINAPI ID3DXEffectImpl_SetValue(ID3DXEffect *iface, D3DXHANDLE parameter, LPCVOID data, UINT bytes)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetValue(base, parameter, data, bytes);
}

static HRESULT WINAPI ID3DXEffectImpl_GetValue(ID3DXEffect *iface, D3DXHANDLE parameter, LPVOID data, UINT bytes)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetValue(base, parameter, data, bytes);
}

static HRESULT WINAPI ID3DXEffectImpl_SetBool(ID3DXEffect *iface, D3DXHANDLE parameter, BOOL b)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetBool(base, parameter, b);
}

static HRESULT WINAPI ID3DXEffectImpl_GetBool(ID3DXEffect *iface, D3DXHANDLE parameter, BOOL *b)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetBool(base, parameter, b);
}

static HRESULT WINAPI ID3DXEffectImpl_SetBoolArray(ID3DXEffect *iface, D3DXHANDLE parameter, CONST BOOL *b, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetBoolArray(base, parameter, b, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetBoolArray(ID3DXEffect *iface, D3DXHANDLE parameter, BOOL *b, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetBoolArray(base, parameter, b, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetInt(ID3DXEffect *iface, D3DXHANDLE parameter, INT n)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetInt(base, parameter, n);
}

static HRESULT WINAPI ID3DXEffectImpl_GetInt(ID3DXEffect *iface, D3DXHANDLE parameter, INT *n)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetInt(base, parameter, n);
}

static HRESULT WINAPI ID3DXEffectImpl_SetIntArray(ID3DXEffect *iface, D3DXHANDLE parameter, CONST INT *n, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetIntArray(base, parameter, n, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetIntArray(ID3DXEffect *iface, D3DXHANDLE parameter, INT *n, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetIntArray(base, parameter, n, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetFloat(ID3DXEffect *iface, D3DXHANDLE parameter, FLOAT f)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetFloat(base, parameter, f);
}

static HRESULT WINAPI ID3DXEffectImpl_GetFloat(ID3DXEffect *iface, D3DXHANDLE parameter, FLOAT *f)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetFloat(base, parameter, f);
}

static HRESULT WINAPI ID3DXEffectImpl_SetFloatArray(ID3DXEffect *iface, D3DXHANDLE parameter, CONST FLOAT *f, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetFloatArray(base, parameter, f, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetFloatArray(ID3DXEffect *iface, D3DXHANDLE parameter, FLOAT *f, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetFloatArray(base, parameter, f, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetVector(ID3DXEffect *iface, D3DXHANDLE parameter, CONST D3DXVECTOR4 *vector)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetVector(base, parameter, vector);
}

static HRESULT WINAPI ID3DXEffectImpl_GetVector(ID3DXEffect *iface, D3DXHANDLE parameter, D3DXVECTOR4 *vector)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetVector(base, parameter, vector);
}

static HRESULT WINAPI ID3DXEffectImpl_SetVectorArray(ID3DXEffect *iface, D3DXHANDLE parameter, CONST D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetVectorArray(base, parameter, vector, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetVectorArray(ID3DXEffect *iface, D3DXHANDLE parameter, D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetVectorArray(base, parameter, vector, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrix(ID3DXEffect *iface, D3DXHANDLE parameter, CONST D3DXMATRIX *matrix)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetMatrix(base, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrix(ID3DXEffect *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetMatrix(base, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixArray(ID3DXEffect *iface, D3DXHANDLE parameter, CONST D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetMatrixArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixArray(ID3DXEffect *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetMatrixArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixPointerArray(ID3DXEffect *iface, D3DXHANDLE parameter, CONST D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetMatrixPointerArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixPointerArray(ID3DXEffect *iface, D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetMatrixPointerArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixTranspose(ID3DXEffect *iface, D3DXHANDLE parameter, CONST D3DXMATRIX *matrix)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetMatrixTranspose(base, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixTranspose(ID3DXEffect *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetMatrixTranspose(base, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixTransposeArray(ID3DXEffect *iface, D3DXHANDLE parameter, CONST D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetMatrixTransposeArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixTransposeArray(ID3DXEffect *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetMatrixTransposeArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixTransposePointerArray(ID3DXEffect *iface, D3DXHANDLE parameter, CONST D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetMatrixTransposePointerArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixTransposePointerArray(ID3DXEffect *iface, D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetMatrixTransposePointerArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetString(ID3DXEffect *iface, D3DXHANDLE parameter, LPCSTR string)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetString(base, parameter, string);
}

static HRESULT WINAPI ID3DXEffectImpl_GetString(ID3DXEffect *iface, D3DXHANDLE parameter, LPCSTR *string)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetString(base, parameter, string);
}

static HRESULT WINAPI ID3DXEffectImpl_SetTexture(ID3DXEffect *iface, D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9 texture)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetTexture(base, parameter, texture);
}

static HRESULT WINAPI ID3DXEffectImpl_GetTexture(ID3DXEffect *iface, D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9 *texture)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetTexture(base, parameter, texture);
}

static HRESULT WINAPI ID3DXEffectImpl_GetPixelShader(ID3DXEffect *iface, D3DXHANDLE parameter, LPDIRECT3DPIXELSHADER9 *pshader)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetPixelShader(base, parameter, pshader);
}

static HRESULT WINAPI ID3DXEffectImpl_GetVertexShader(ID3DXEffect *iface, D3DXHANDLE parameter, LPDIRECT3DVERTEXSHADER9 *vshader)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetVertexShader(base, parameter, vshader);
}

static HRESULT WINAPI ID3DXEffectImpl_SetArrayRange(ID3DXEffect *iface, D3DXHANDLE parameter, UINT start, UINT end)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetArrayRange(base, parameter, start, end);
}

/*** ID3DXEffect methods ***/
static HRESULT WINAPI ID3DXEffectImpl_GetPool(ID3DXEffect *iface, LPD3DXEFFECTPOOL *pool)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, pool %p\n", This, pool);

    if (!pool)
    {
        WARN("Invalid argument supplied.\n");
        return D3DERR_INVALIDCALL;
    }

    if (This->pool)
    {
        This->pool->lpVtbl->AddRef(This->pool);
    }

    *pool = This->pool;

    TRACE("Returning pool %p\n", *pool);

    return S_OK;
}

static HRESULT WINAPI ID3DXEffectImpl_SetTechnique(ID3DXEffect* iface, D3DXHANDLE technique)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p): stub\n", This, technique);

    return E_NOTIMPL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetCurrentTechnique(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(): stub\n", This);

    return NULL;
}

static HRESULT WINAPI ID3DXEffectImpl_ValidateTechnique(ID3DXEffect* iface, D3DXHANDLE technique)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p): stub\n", This, technique);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXEffectImpl_FindNextValidTechnique(ID3DXEffect* iface, D3DXHANDLE technique, D3DXHANDLE* next_technique)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, technique, next_technique);

    return E_NOTIMPL;
}

static BOOL WINAPI ID3DXEffectImpl_IsParameterUsed(ID3DXEffect* iface, D3DXHANDLE parameter, D3DXHANDLE technique)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, technique);

    return FALSE;
}

static HRESULT WINAPI ID3DXEffectImpl_Begin(ID3DXEffect* iface, UINT *passes, DWORD flags)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p, %#x): stub\n", This, passes, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_BeginPass(ID3DXEffect* iface, UINT pass)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%u): stub\n", This, pass);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_CommitChanges(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_EndPass(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_End(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetDevice(ID3DXEffect *iface, LPDIRECT3DDEVICE9 *device)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, device %p\n", This, device);

    if (!device)
    {
        WARN("Invalid argument supplied.\n");
        return D3DERR_INVALIDCALL;
    }

    IDirect3DDevice9_AddRef(This->device);

    *device = This->device;

    TRACE("Returning device %p\n", *device);

    return S_OK;
}

static HRESULT WINAPI ID3DXEffectImpl_OnLostDevice(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_OnResetDevice(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetStateManager(ID3DXEffect *iface, LPD3DXEFFECTSTATEMANAGER manager)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, manager %p\n", This, manager);

    if (This->manager) IUnknown_Release(This->manager);
    if (manager) IUnknown_AddRef(manager);

    This->manager = manager;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXEffectImpl_GetStateManager(ID3DXEffect *iface, LPD3DXEFFECTSTATEMANAGER *manager)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, manager %p\n", This, manager);

    if (!manager)
    {
        WARN("Invalid argument supplied.\n");
        return D3DERR_INVALIDCALL;
    }

    if (This->manager) IUnknown_AddRef(This->manager);
    *manager = This->manager;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXEffectImpl_BeginParameterBlock(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_EndParameterBlock(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(): stub\n", This);

    return NULL;
}

static HRESULT WINAPI ID3DXEffectImpl_ApplyParameterBlock(ID3DXEffect* iface, D3DXHANDLE parameter_block)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p): stub\n", This, parameter_block);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_DeleteParameterBlock(ID3DXEffect* iface, D3DXHANDLE parameter_block)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p): stub\n", This, parameter_block);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_CloneEffect(ID3DXEffect* iface, LPDIRECT3DDEVICE9 device, LPD3DXEFFECT* effect)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, device, effect);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetRawValue(ID3DXEffect* iface, D3DXHANDLE parameter, LPCVOID data, UINT byte_offset, UINT bytes)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p, %p, %u, %u): stub\n", This, parameter, data, byte_offset, bytes);

    return E_NOTIMPL;
}

static const struct ID3DXEffectVtbl ID3DXEffect_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXEffectImpl_QueryInterface,
    ID3DXEffectImpl_AddRef,
    ID3DXEffectImpl_Release,
    /*** ID3DXBaseEffect methods ***/
    ID3DXEffectImpl_GetDesc,
    ID3DXEffectImpl_GetParameterDesc,
    ID3DXEffectImpl_GetTechniqueDesc,
    ID3DXEffectImpl_GetPassDesc,
    ID3DXEffectImpl_GetFunctionDesc,
    ID3DXEffectImpl_GetParameter,
    ID3DXEffectImpl_GetParameterByName,
    ID3DXEffectImpl_GetParameterBySemantic,
    ID3DXEffectImpl_GetParameterElement,
    ID3DXEffectImpl_GetTechnique,
    ID3DXEffectImpl_GetTechniqueByName,
    ID3DXEffectImpl_GetPass,
    ID3DXEffectImpl_GetPassByName,
    ID3DXEffectImpl_GetFunction,
    ID3DXEffectImpl_GetFunctionByName,
    ID3DXEffectImpl_GetAnnotation,
    ID3DXEffectImpl_GetAnnotationByName,
    ID3DXEffectImpl_SetValue,
    ID3DXEffectImpl_GetValue,
    ID3DXEffectImpl_SetBool,
    ID3DXEffectImpl_GetBool,
    ID3DXEffectImpl_SetBoolArray,
    ID3DXEffectImpl_GetBoolArray,
    ID3DXEffectImpl_SetInt,
    ID3DXEffectImpl_GetInt,
    ID3DXEffectImpl_SetIntArray,
    ID3DXEffectImpl_GetIntArray,
    ID3DXEffectImpl_SetFloat,
    ID3DXEffectImpl_GetFloat,
    ID3DXEffectImpl_SetFloatArray,
    ID3DXEffectImpl_GetFloatArray,
    ID3DXEffectImpl_SetVector,
    ID3DXEffectImpl_GetVector,
    ID3DXEffectImpl_SetVectorArray,
    ID3DXEffectImpl_GetVectorArray,
    ID3DXEffectImpl_SetMatrix,
    ID3DXEffectImpl_GetMatrix,
    ID3DXEffectImpl_SetMatrixArray,
    ID3DXEffectImpl_GetMatrixArray,
    ID3DXEffectImpl_SetMatrixPointerArray,
    ID3DXEffectImpl_GetMatrixPointerArray,
    ID3DXEffectImpl_SetMatrixTranspose,
    ID3DXEffectImpl_GetMatrixTranspose,
    ID3DXEffectImpl_SetMatrixTransposeArray,
    ID3DXEffectImpl_GetMatrixTransposeArray,
    ID3DXEffectImpl_SetMatrixTransposePointerArray,
    ID3DXEffectImpl_GetMatrixTransposePointerArray,
    ID3DXEffectImpl_SetString,
    ID3DXEffectImpl_GetString,
    ID3DXEffectImpl_SetTexture,
    ID3DXEffectImpl_GetTexture,
    ID3DXEffectImpl_GetPixelShader,
    ID3DXEffectImpl_GetVertexShader,
    ID3DXEffectImpl_SetArrayRange,
    /*** ID3DXEffect methods ***/
    ID3DXEffectImpl_GetPool,
    ID3DXEffectImpl_SetTechnique,
    ID3DXEffectImpl_GetCurrentTechnique,
    ID3DXEffectImpl_ValidateTechnique,
    ID3DXEffectImpl_FindNextValidTechnique,
    ID3DXEffectImpl_IsParameterUsed,
    ID3DXEffectImpl_Begin,
    ID3DXEffectImpl_BeginPass,
    ID3DXEffectImpl_CommitChanges,
    ID3DXEffectImpl_EndPass,
    ID3DXEffectImpl_End,
    ID3DXEffectImpl_GetDevice,
    ID3DXEffectImpl_OnLostDevice,
    ID3DXEffectImpl_OnResetDevice,
    ID3DXEffectImpl_SetStateManager,
    ID3DXEffectImpl_GetStateManager,
    ID3DXEffectImpl_BeginParameterBlock,
    ID3DXEffectImpl_EndParameterBlock,
    ID3DXEffectImpl_ApplyParameterBlock,
    ID3DXEffectImpl_DeleteParameterBlock,
    ID3DXEffectImpl_CloneEffect,
    ID3DXEffectImpl_SetRawValue
};

static inline struct ID3DXEffectCompilerImpl *impl_from_ID3DXEffectCompiler(ID3DXEffectCompiler *iface)
{
    return CONTAINING_RECORD(iface, struct ID3DXEffectCompilerImpl, ID3DXEffectCompiler_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXEffectCompilerImpl_QueryInterface(ID3DXEffectCompiler *iface, REFIID riid, void **object)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, riid %s, object %p\n", This, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXEffectCompiler))
    {
        This->ID3DXEffectCompiler_iface.lpVtbl->AddRef(iface);
        *object = This;
        return S_OK;
    }

    ERR("Interface %s not found\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXEffectCompilerImpl_AddRef(ID3DXEffectCompiler *iface)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p: AddRef from %u\n", iface, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXEffectCompilerImpl_Release(ID3DXEffectCompiler *iface)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("iface %p: Release from %u\n", iface, ref + 1);

    if (!ref)
    {
        free_effect_compiler(This);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ID3DXBaseEffect methods ***/
static HRESULT WINAPI ID3DXEffectCompilerImpl_GetDesc(ID3DXEffectCompiler *iface, D3DXEFFECT_DESC *desc)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetDesc(base, desc);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetParameterDesc(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, D3DXPARAMETER_DESC *desc)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetParameterDesc(base, parameter, desc);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetTechniqueDesc(ID3DXEffectCompiler *iface, D3DXHANDLE technique, D3DXTECHNIQUE_DESC *desc)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetTechniqueDesc(base, technique, desc);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetPassDesc(ID3DXEffectCompiler *iface, D3DXHANDLE pass, D3DXPASS_DESC *desc)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetPassDesc(base, pass, desc);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetFunctionDesc(ID3DXEffectCompiler *iface, D3DXHANDLE shader, D3DXFUNCTION_DESC *desc)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetFunctionDesc(base, shader, desc);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetParameter(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, UINT index)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetParameter(base, parameter, index);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetParameterByName(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, LPCSTR name)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetParameterByName(base, parameter, name);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetParameterBySemantic(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, LPCSTR semantic)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetParameterBySemantic(base, parameter, semantic);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetParameterElement(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, UINT index)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetParameterElement(base, parameter, index);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetTechnique(ID3DXEffectCompiler *iface, UINT index)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetTechnique(base, index);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetTechniqueByName(ID3DXEffectCompiler *iface, LPCSTR name)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetTechniqueByName(base, name);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetPass(ID3DXEffectCompiler *iface, D3DXHANDLE technique, UINT index)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetPass(base, technique, index);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetPassByName(ID3DXEffectCompiler *iface, D3DXHANDLE technique, LPCSTR name)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetPassByName(base, technique, name);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetFunction(ID3DXEffectCompiler *iface, UINT index)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetFunction(base, index);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetFunctionByName(ID3DXEffectCompiler *iface, LPCSTR name)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetFunctionByName(base, name);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetAnnotation(ID3DXEffectCompiler *iface, D3DXHANDLE object, UINT index)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetAnnotation(base, object, index);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetAnnotationByName(ID3DXEffectCompiler *iface, D3DXHANDLE object, LPCSTR name)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetAnnotationByName(base, object, name);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetValue(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, LPCVOID data, UINT bytes)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetValue(base, parameter, data, bytes);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetValue(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, LPVOID data, UINT bytes)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetValue(base, parameter, data, bytes);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetBool(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, BOOL b)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetBool(base, parameter, b);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetBool(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, BOOL *b)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetBool(base, parameter, b);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetBoolArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, CONST BOOL *b, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetBoolArray(base, parameter, b, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetBoolArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, BOOL *b, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetBoolArray(base, parameter, b, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetInt(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, INT n)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetInt(base, parameter, n);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetInt(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, INT *n)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetInt(base, parameter, n);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetIntArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, CONST INT *n, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetIntArray(base, parameter, n, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetIntArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, INT *n, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetIntArray(base, parameter, n, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetFloat(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, FLOAT f)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetFloat(base, parameter, f);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetFloat(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, FLOAT *f)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetFloat(base, parameter, f);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetFloatArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, CONST FLOAT *f, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetFloatArray(base, parameter, f, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetFloatArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, FLOAT *f, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetFloatArray(base, parameter, f, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetVector(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, CONST D3DXVECTOR4 *vector)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetVector(base, parameter, vector);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetVector(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, D3DXVECTOR4 *vector)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetVector(base, parameter, vector);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetVectorArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, CONST D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetVectorArray(base, parameter, vector, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetVectorArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetVectorArray(base, parameter, vector, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrix(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, CONST D3DXMATRIX *matrix)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetMatrix(base, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrix(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetMatrix(base, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, CONST D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetMatrixArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetMatrixArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixPointerArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, CONST D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetMatrixPointerArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixPointerArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetMatrixPointerArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixTranspose(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, CONST D3DXMATRIX *matrix)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetMatrixTranspose(base, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixTranspose(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetMatrixTranspose(base, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixTransposeArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, CONST D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetMatrixTransposeArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixTransposeArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetMatrixTransposeArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixTransposePointerArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, CONST D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetMatrixTransposePointerArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixTransposePointerArray(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetMatrixTransposePointerArray(base, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetString(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, LPCSTR string)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetString(base, parameter, string);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetString(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, LPCSTR *string)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetString(base, parameter, string);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetTexture(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9 texture)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetTexture(base, parameter, texture);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetTexture(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9 *texture)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetTexture(base, parameter, texture);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetPixelShader(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, LPDIRECT3DPIXELSHADER9 *pshader)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetPixelShader(base, parameter, pshader);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetVertexShader(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, LPDIRECT3DVERTEXSHADER9 *vshader)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_GetVertexShader(base, parameter, vshader);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetArrayRange(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, UINT start, UINT end)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ID3DXBaseEffect *base = This->base_effect;

    TRACE("Forward iface %p, base %p\n", This, base);

    return ID3DXBaseEffectImpl_SetArrayRange(base, parameter, start, end);
}

/*** ID3DXEffectCompiler methods ***/
static HRESULT WINAPI ID3DXEffectCompilerImpl_SetLiteral(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, BOOL literal)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);

    FIXME("iface %p, parameter %p, literal %u\n", This, parameter, literal);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetLiteral(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, BOOL *literal)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);

    FIXME("iface %p, parameter %p, literal %p\n", This, parameter, literal);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_CompileEffect(ID3DXEffectCompiler *iface, DWORD flags,
        LPD3DXBUFFER *effect, LPD3DXBUFFER *error_msgs)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);

    FIXME("iface %p, flags %#x, effect %p, error_msgs %p stub\n", This, flags, effect, error_msgs);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_CompileShader(ID3DXEffectCompiler *iface, D3DXHANDLE function,
        LPCSTR target, DWORD flags, LPD3DXBUFFER *shader, LPD3DXBUFFER *error_msgs, LPD3DXCONSTANTTABLE *constant_table)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);

    FIXME("iface %p, function %p, target %p, flags %#x, shader %p, error_msgs %p, constant_table %p stub\n",
            This, function, target, flags, shader, error_msgs, constant_table);

    return E_NOTIMPL;
}

static const struct ID3DXEffectCompilerVtbl ID3DXEffectCompiler_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXEffectCompilerImpl_QueryInterface,
    ID3DXEffectCompilerImpl_AddRef,
    ID3DXEffectCompilerImpl_Release,
    /*** ID3DXBaseEffect methods ***/
    ID3DXEffectCompilerImpl_GetDesc,
    ID3DXEffectCompilerImpl_GetParameterDesc,
    ID3DXEffectCompilerImpl_GetTechniqueDesc,
    ID3DXEffectCompilerImpl_GetPassDesc,
    ID3DXEffectCompilerImpl_GetFunctionDesc,
    ID3DXEffectCompilerImpl_GetParameter,
    ID3DXEffectCompilerImpl_GetParameterByName,
    ID3DXEffectCompilerImpl_GetParameterBySemantic,
    ID3DXEffectCompilerImpl_GetParameterElement,
    ID3DXEffectCompilerImpl_GetTechnique,
    ID3DXEffectCompilerImpl_GetTechniqueByName,
    ID3DXEffectCompilerImpl_GetPass,
    ID3DXEffectCompilerImpl_GetPassByName,
    ID3DXEffectCompilerImpl_GetFunction,
    ID3DXEffectCompilerImpl_GetFunctionByName,
    ID3DXEffectCompilerImpl_GetAnnotation,
    ID3DXEffectCompilerImpl_GetAnnotationByName,
    ID3DXEffectCompilerImpl_SetValue,
    ID3DXEffectCompilerImpl_GetValue,
    ID3DXEffectCompilerImpl_SetBool,
    ID3DXEffectCompilerImpl_GetBool,
    ID3DXEffectCompilerImpl_SetBoolArray,
    ID3DXEffectCompilerImpl_GetBoolArray,
    ID3DXEffectCompilerImpl_SetInt,
    ID3DXEffectCompilerImpl_GetInt,
    ID3DXEffectCompilerImpl_SetIntArray,
    ID3DXEffectCompilerImpl_GetIntArray,
    ID3DXEffectCompilerImpl_SetFloat,
    ID3DXEffectCompilerImpl_GetFloat,
    ID3DXEffectCompilerImpl_SetFloatArray,
    ID3DXEffectCompilerImpl_GetFloatArray,
    ID3DXEffectCompilerImpl_SetVector,
    ID3DXEffectCompilerImpl_GetVector,
    ID3DXEffectCompilerImpl_SetVectorArray,
    ID3DXEffectCompilerImpl_GetVectorArray,
    ID3DXEffectCompilerImpl_SetMatrix,
    ID3DXEffectCompilerImpl_GetMatrix,
    ID3DXEffectCompilerImpl_SetMatrixArray,
    ID3DXEffectCompilerImpl_GetMatrixArray,
    ID3DXEffectCompilerImpl_SetMatrixPointerArray,
    ID3DXEffectCompilerImpl_GetMatrixPointerArray,
    ID3DXEffectCompilerImpl_SetMatrixTranspose,
    ID3DXEffectCompilerImpl_GetMatrixTranspose,
    ID3DXEffectCompilerImpl_SetMatrixTransposeArray,
    ID3DXEffectCompilerImpl_GetMatrixTransposeArray,
    ID3DXEffectCompilerImpl_SetMatrixTransposePointerArray,
    ID3DXEffectCompilerImpl_GetMatrixTransposePointerArray,
    ID3DXEffectCompilerImpl_SetString,
    ID3DXEffectCompilerImpl_GetString,
    ID3DXEffectCompilerImpl_SetTexture,
    ID3DXEffectCompilerImpl_GetTexture,
    ID3DXEffectCompilerImpl_GetPixelShader,
    ID3DXEffectCompilerImpl_GetVertexShader,
    ID3DXEffectCompilerImpl_SetArrayRange,
    /*** ID3DXEffectCompiler methods ***/
    ID3DXEffectCompilerImpl_SetLiteral,
    ID3DXEffectCompilerImpl_GetLiteral,
    ID3DXEffectCompilerImpl_CompileEffect,
    ID3DXEffectCompilerImpl_CompileShader,
};

static HRESULT d3dx9_parse_value(struct d3dx_parameter *param, void *value, const char **ptr)
{
    unsigned int i;
    HRESULT hr;
    UINT old_size = 0;
    DWORD id;

    if (param->element_count)
    {
        param->data = value;

        for (i = 0; i < param->element_count; ++i)
        {
            struct d3dx_parameter *member = get_parameter_struct(param->member_handles[i]);

            hr = d3dx9_parse_value(member, (char *)value + old_size, ptr);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse value\n");
                return hr;
            }

            old_size += member->bytes;
        }

        return D3D_OK;
    }

    switch(param->class)
    {
        case D3DXPC_SCALAR:
        case D3DXPC_VECTOR:
        case D3DXPC_MATRIX_ROWS:
        case D3DXPC_MATRIX_COLUMNS:
            param->data = value;
            break;

        case D3DXPC_STRUCT:
            param->data = value;

            for (i = 0; i < param->member_count; ++i)
            {
                struct d3dx_parameter *member = get_parameter_struct(param->member_handles[i]);

                hr = d3dx9_parse_value(member, (char *)value + old_size, ptr);
                if (hr != D3D_OK)
                {
                    WARN("Failed to parse value\n");
                    return hr;
                }

                old_size += member->bytes;
            }
            break;

        case D3DXPC_OBJECT:
            switch (param->type)
            {
                case D3DXPT_STRING:
                case D3DXPT_TEXTURE:
                case D3DXPT_TEXTURE1D:
                case D3DXPT_TEXTURE2D:
                case D3DXPT_TEXTURE3D:
                case D3DXPT_TEXTURECUBE:
                case D3DXPT_PIXELSHADER:
                case D3DXPT_VERTEXSHADER:
                    read_dword(ptr, &id);
                    TRACE("Id: %u\n", id);
                    param->base->objects[id] = get_parameter_handle(param);
                    param->data = value;
                    break;

                default:
                    FIXME("Unhandled type %s\n", debug_d3dxparameter_type(param->type));
                    break;
            }
            break;

        default:
            FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
            break;
    }

    return D3D_OK;
}

static HRESULT d3dx9_parse_init_value(struct d3dx_parameter *param, const char *ptr)
{
    UINT size = param->bytes;
    HRESULT hr;
    void *value = NULL;

    TRACE("param size: %u\n", size);

    value = HeapAlloc(GetProcessHeap(), 0, size);
    if (!value)
    {
        ERR("Failed to allocate data memory.\n");
        return E_OUTOFMEMORY;
    }

    TRACE("Data: %s.\n", debugstr_an(ptr, size));
    memcpy(value, ptr, size);

    hr = d3dx9_parse_value(param, value, &ptr);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse value\n");
        HeapFree(GetProcessHeap(), 0, value);
        return hr;
    }

    param->data = value;

    return D3D_OK;
}

static HRESULT d3dx9_parse_name(char **name, const char *ptr)
{
    DWORD size;

    read_dword(&ptr, &size);
    TRACE("Name size: %#x\n", size);

    if (!size)
    {
        return D3D_OK;
    }

    *name = HeapAlloc(GetProcessHeap(), 0, size);
    if (!*name)
    {
        ERR("Failed to allocate name memory.\n");
        return E_OUTOFMEMORY;
    }

    TRACE("Name: %s.\n", debugstr_an(ptr, size));
    memcpy(*name, ptr, size);

    return D3D_OK;
}

static HRESULT d3dx9_parse_data(struct d3dx_parameter *param, const char **ptr)
{
    DWORD size;
    HRESULT hr;

    TRACE("Parse data for parameter %s, type %s\n", debugstr_a(param->name), debug_d3dxparameter_type(param->type));

    read_dword(ptr, &size);
    TRACE("Data size: %#x\n", size);

    if (!size)
    {
        TRACE("Size is 0\n");
        *(void **)param->data = NULL;
        return D3D_OK;
    }

    switch (param->type)
    {
        case D3DXPT_STRING:
            /* re-read with size (sizeof(DWORD) = 4) */
            hr = d3dx9_parse_name((LPSTR *)param->data, *ptr - 4);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse string data\n");
                return hr;
            }
            break;

        case D3DXPT_VERTEXSHADER:
            hr = IDirect3DDevice9_CreateVertexShader(param->base->effect->device, (DWORD *)*ptr, (LPDIRECT3DVERTEXSHADER9 *)param->data);
            if (hr != D3D_OK)
            {
                WARN("Failed to create vertex shader\n");
                return hr;
            }
            break;

        case D3DXPT_PIXELSHADER:
            hr = IDirect3DDevice9_CreatePixelShader(param->base->effect->device, (DWORD *)*ptr, (LPDIRECT3DPIXELSHADER9 *)param->data);
            if (hr != D3D_OK)
            {
                WARN("Failed to create pixel shader\n");
                return hr;
            }
            break;

        default:
            FIXME("Unhandled type %s\n", debug_d3dxparameter_type(param->type));
            break;
    }


    *ptr += ((size + 3) & ~3);

    return D3D_OK;
}

static HRESULT d3dx9_parse_effect_typedef(struct d3dx_parameter *param, const char *data, const char **ptr,
        struct d3dx_parameter *parent, UINT flags)
{
    DWORD offset;
    HRESULT hr;
    D3DXHANDLE *member_handles = NULL;
    UINT i;

    param->flags = flags;

    if (!parent)
    {
        read_dword(ptr, &param->type);
        TRACE("Type: %s\n", debug_d3dxparameter_type(param->type));

        read_dword(ptr, &param->class);
        TRACE("Class: %s\n", debug_d3dxparameter_class(param->class));

        read_dword(ptr, &offset);
        TRACE("Type name offset: %#x\n", offset);
        hr = d3dx9_parse_name(&param->name, data + offset);
        if (hr != D3D_OK)
        {
            WARN("Failed to parse name\n");
            goto err_out;
        }

        read_dword(ptr, &offset);
        TRACE("Type semantic offset: %#x\n", offset);
        hr = d3dx9_parse_name(&param->semantic, data + offset);
        if (hr != D3D_OK)
        {
            WARN("Failed to parse semantic\n");
            goto err_out;
        }

        read_dword(ptr, &param->element_count);
        TRACE("Elements: %u\n", param->element_count);

        switch (param->class)
        {
            case D3DXPC_VECTOR:
                read_dword(ptr, &param->columns);
                TRACE("Columns: %u\n", param->columns);

                read_dword(ptr, &param->rows);
                TRACE("Rows: %u\n", param->rows);

                /* sizeof(DWORD) * rows * columns */
                param->bytes = 4 * param->rows * param->columns;
                break;

            case D3DXPC_SCALAR:
            case D3DXPC_MATRIX_ROWS:
            case D3DXPC_MATRIX_COLUMNS:
                read_dword(ptr, &param->rows);
                TRACE("Rows: %u\n", param->rows);

                read_dword(ptr, &param->columns);
                TRACE("Columns: %u\n", param->columns);

                /* sizeof(DWORD) * rows * columns */
                param->bytes = 4 * param->rows * param->columns;
                break;

            case D3DXPC_STRUCT:
                read_dword(ptr, &param->member_count);
                TRACE("Members: %u\n", param->member_count);
                break;

            case D3DXPC_OBJECT:
                switch (param->type)
                {
                    case D3DXPT_STRING:
                        param->bytes = sizeof(LPCSTR);
                        break;

                    case D3DXPT_PIXELSHADER:
                        param->bytes = sizeof(LPDIRECT3DPIXELSHADER9);
                        break;

                    case D3DXPT_VERTEXSHADER:
                        param->bytes = sizeof(LPDIRECT3DVERTEXSHADER9);
                        break;

                    case D3DXPT_TEXTURE:
                    case D3DXPT_TEXTURE1D:
                    case D3DXPT_TEXTURE2D:
                    case D3DXPT_TEXTURE3D:
                    case D3DXPT_TEXTURECUBE:
                        param->bytes = sizeof(LPDIRECT3DBASETEXTURE9);
                        break;

                    default:
                        FIXME("Unhandled type %s\n", debug_d3dxparameter_type(param->type));
                        break;
                }
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }
    else
    {
        /* elements */
        param->type = parent->type;
        param->class = parent->class;
        param->name = parent->name;
        param->semantic = parent->semantic;
        param->element_count = 0;
        param->annotation_count = 0;
        param->member_count = parent->member_count;
        param->bytes = parent->bytes;
        param->rows = parent->rows;
        param->columns = parent->columns;
    }

    if (param->element_count)
    {
        unsigned int param_bytes = 0;
        const char *save_ptr = *ptr;

        member_handles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*member_handles) * param->element_count);
        if (!member_handles)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < param->element_count; ++i)
        {
            struct d3dx_parameter *member;
            *ptr = save_ptr;

            member = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*member));
            if (!member)
            {
                ERR("Out of memory\n");
                hr = E_OUTOFMEMORY;
                goto err_out;
            }

            member_handles[i] = get_parameter_handle(member);
            member->base = param->base;

            hr = d3dx9_parse_effect_typedef(member, data, ptr, param, flags);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse member\n");
                goto err_out;
            }

            param_bytes += member->bytes;
        }

        param->bytes = param_bytes;
    }
    else if (param->member_count)
    {
        member_handles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*member_handles) * param->member_count);
        if (!member_handles)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < param->member_count; ++i)
        {
            struct d3dx_parameter *member;

            member = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*member));
            if (!member)
            {
                ERR("Out of memory\n");
                hr = E_OUTOFMEMORY;
                goto err_out;
            }

            member_handles[i] = get_parameter_handle(member);
            member->base = param->base;

            hr = d3dx9_parse_effect_typedef(member, data, ptr, NULL, flags);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse member\n");
                goto err_out;
            }

            param->bytes += member->bytes;
        }
    }

    param->member_handles = member_handles;

    return D3D_OK;

err_out:

    if (member_handles)
    {
        unsigned int count;

        if (param->element_count) count = param->element_count;
        else count = param->member_count;

        for (i = 0; i < count; ++i)
        {
            free_parameter(member_handles[i], param->element_count != 0, TRUE);
        }
        HeapFree(GetProcessHeap(), 0, member_handles);
    }

    if (!parent)
    {
        HeapFree(GetProcessHeap(), 0, param->name);
        HeapFree(GetProcessHeap(), 0, param->semantic);
    }
    param->name = NULL;
    param->semantic = NULL;

    return hr;
}

static HRESULT d3dx9_parse_effect_annotation(struct d3dx_parameter *anno, const char *data, const char **ptr)
{
    DWORD offset;
    const char *ptr2;
    HRESULT hr;

    anno->flags = D3DX_PARAMETER_ANNOTATION;

    read_dword(ptr, &offset);
    TRACE("Typedef offset: %#x\n", offset);
    ptr2 = data + offset;
    hr = d3dx9_parse_effect_typedef(anno, data, &ptr2, NULL, D3DX_PARAMETER_ANNOTATION);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse type definition\n");
        return hr;
    }

    read_dword(ptr, &offset);
    TRACE("Value offset: %#x\n", offset);
    hr = d3dx9_parse_init_value(anno, data + offset);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse value\n");
        return hr;
    }

    return D3D_OK;
}

static HRESULT d3dx9_parse_effect_parameter(struct d3dx_parameter *param, const char *data, const char **ptr)
{
    DWORD offset;
    HRESULT hr;
    unsigned int i;
    D3DXHANDLE *annotation_handles = NULL;
    const char *ptr2;

    read_dword(ptr, &offset);
    TRACE("Typedef offset: %#x\n", offset);
    ptr2 = data + offset;

    read_dword(ptr, &offset);
    TRACE("Value offset: %#x\n", offset);

    read_dword(ptr, &param->flags);
    TRACE("Flags: %#x\n", param->flags);

    read_dword(ptr, &param->annotation_count);
    TRACE("Annotation count: %u\n", param->annotation_count);

    hr = d3dx9_parse_effect_typedef(param, data, &ptr2, NULL, param->flags);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse type definition\n");
        return hr;
    }

    hr = d3dx9_parse_init_value(param, data + offset);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse value\n");
        return hr;
    }

    if (param->annotation_count)
    {
        annotation_handles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*annotation_handles) * param->annotation_count);
        if (!annotation_handles)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < param->annotation_count; ++i)
        {
            struct d3dx_parameter *annotation;

            annotation = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*annotation));
            if (!annotation)
            {
                ERR("Out of memory\n");
                hr = E_OUTOFMEMORY;
                goto err_out;
            }

            annotation_handles[i] = get_parameter_handle(annotation);
            annotation->base = param->base;

            hr = d3dx9_parse_effect_annotation(annotation, data, ptr);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse annotation\n");
                goto err_out;
            }
        }
    }

    param->annotation_handles = annotation_handles;

    return D3D_OK;

err_out:

    if (annotation_handles)
    {
        for (i = 0; i < param->annotation_count; ++i)
        {
            free_parameter(annotation_handles[i], FALSE, FALSE);
        }
        HeapFree(GetProcessHeap(), 0, annotation_handles);
    }

    return hr;
}

static HRESULT d3dx9_parse_effect_pass(struct d3dx_pass *pass, const char *data, const char **ptr)
{
    DWORD offset;
    HRESULT hr;
    unsigned int i;
    D3DXHANDLE *annotation_handles = NULL;
    char *name = NULL;

    read_dword(ptr, &offset);
    TRACE("Pass name offset: %#x\n", offset);
    hr = d3dx9_parse_name(&name, data + offset);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse name\n");
        goto err_out;
    }

    read_dword(ptr, &pass->annotation_count);
    TRACE("Annotation count: %u\n", pass->annotation_count);

    read_dword(ptr, &pass->state_count);
    TRACE("State count: %u\n", pass->state_count);

    if (pass->annotation_count)
    {
        annotation_handles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*annotation_handles) * pass->annotation_count);
        if (!annotation_handles)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < pass->annotation_count; ++i)
        {
            struct d3dx_parameter *annotation;

            annotation = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*annotation));
            if (!annotation)
            {
                ERR("Out of memory\n");
                hr = E_OUTOFMEMORY;
                goto err_out;
            }

            annotation_handles[i] = get_parameter_handle(annotation);
            annotation->base = pass->base;

            hr = d3dx9_parse_effect_annotation(annotation, data, ptr);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse annotations\n");
                goto err_out;
            }
        }
    }

    if (pass->state_count)
    {
        for (i = 0; i < pass->state_count; ++i)
        {
            skip_dword_unknown(ptr, 4);
        }
    }

    pass->name = name;
    pass->annotation_handles = annotation_handles;

    return D3D_OK;

err_out:

    if (annotation_handles)
    {
        for (i = 0; i < pass->annotation_count; ++i)
        {
            free_parameter(annotation_handles[i], FALSE, FALSE);
        }
        HeapFree(GetProcessHeap(), 0, annotation_handles);
    }

    HeapFree(GetProcessHeap(), 0, name);

    return hr;
}

static HRESULT d3dx9_parse_effect_technique(struct d3dx_technique *technique, const char *data, const char **ptr)
{
    DWORD offset;
    HRESULT hr;
    unsigned int i;
    D3DXHANDLE *annotation_handles = NULL;
    D3DXHANDLE *pass_handles = NULL;
    char *name = NULL;

    read_dword(ptr, &offset);
    TRACE("Technique name offset: %#x\n", offset);
    hr = d3dx9_parse_name(&name, data + offset);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse name\n");
        goto err_out;
    }

    read_dword(ptr, &technique->annotation_count);
    TRACE("Annotation count: %u\n", technique->annotation_count);

    read_dword(ptr, &technique->pass_count);
    TRACE("Pass count: %u\n", technique->pass_count);

    if (technique->annotation_count)
    {
        annotation_handles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*annotation_handles) * technique->annotation_count);
        if (!annotation_handles)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < technique->annotation_count; ++i)
        {
            struct d3dx_parameter *annotation;

            annotation = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*annotation));
            if (!annotation)
            {
                ERR("Out of memory\n");
                hr = E_OUTOFMEMORY;
                goto err_out;
            }

            annotation_handles[i] = get_parameter_handle(annotation);
            annotation->base = technique->base;

            hr = d3dx9_parse_effect_annotation(annotation, data, ptr);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse annotations\n");
                goto err_out;
            }
        }
    }

    if (technique->pass_count)
    {
        pass_handles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pass_handles) * technique->pass_count);
        if (!pass_handles)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < technique->pass_count; ++i)
        {
            struct d3dx_pass *pass;

            pass = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pass));
            if (!pass)
            {
                ERR("Out of memory\n");
                hr = E_OUTOFMEMORY;
                goto err_out;
            }

            pass_handles[i] = get_pass_handle(pass);
            pass->base = technique->base;

            hr = d3dx9_parse_effect_pass(pass, data, ptr);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse passes\n");
                goto err_out;
            }
        }
    }

    technique->name = name;
    technique->pass_handles = pass_handles;
    technique->annotation_handles = annotation_handles;

    return D3D_OK;

err_out:

    if (pass_handles)
    {
        for (i = 0; i < technique->pass_count; ++i)
        {
            free_pass(pass_handles[i]);
        }
        HeapFree(GetProcessHeap(), 0, pass_handles);
    }

    if (annotation_handles)
    {
        for (i = 0; i < technique->annotation_count; ++i)
        {
            free_parameter(annotation_handles[i], FALSE, FALSE);
        }
        HeapFree(GetProcessHeap(), 0, annotation_handles);
    }

    HeapFree(GetProcessHeap(), 0, name);

    return hr;
}

static HRESULT d3dx9_parse_effect(struct ID3DXBaseEffectImpl *base, const char *data, UINT data_size, DWORD start)
{
    const char *ptr = data + start;
    D3DXHANDLE *parameter_handles = NULL;
    D3DXHANDLE *technique_handles = NULL;
    D3DXHANDLE *objects = NULL;
    unsigned int stringcount;
    HRESULT hr;
    unsigned int i;

    read_dword(&ptr, &base->parameter_count);
    TRACE("Parameter count: %u\n", base->parameter_count);

    read_dword(&ptr, &base->technique_count);
    TRACE("Technique count: %u\n", base->technique_count);

    skip_dword_unknown(&ptr, 1);

    read_dword(&ptr, &base->object_count);
    TRACE("Object count: %u\n", base->object_count);

    objects = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*objects) * base->object_count);
    if (!objects)
    {
        ERR("Out of memory\n");
        hr = E_OUTOFMEMORY;
        goto err_out;
    }

    base->objects = objects;

    if (base->parameter_count)
    {
        parameter_handles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*parameter_handles) * base->parameter_count);
        if (!parameter_handles)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < base->parameter_count; ++i)
        {
            struct d3dx_parameter *parameter;

            parameter = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*parameter));
            if (!parameter)
            {
                ERR("Out of memory\n");
                hr = E_OUTOFMEMORY;
                goto err_out;
            }

            parameter_handles[i] = get_parameter_handle(parameter);
            parameter->base = base;

            hr = d3dx9_parse_effect_parameter(parameter, data, &ptr);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse parameter\n");
                goto err_out;
            }
        }
    }

    if (base->technique_count)
    {
        technique_handles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*technique_handles) * base->technique_count);
        if (!technique_handles)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < base->technique_count; ++i)
        {
            struct d3dx_technique *technique;

            technique = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*technique));
            if (!technique)
            {
                ERR("Out of memory\n");
                hr = E_OUTOFMEMORY;
                goto err_out;
            }

            technique_handles[i] = get_technique_handle(technique);
            technique->base = base;

            hr = d3dx9_parse_effect_technique(technique, data, &ptr);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse technique\n");
                goto err_out;
            }
        }
    }

    read_dword(&ptr, &stringcount);
    TRACE("String count: %u\n", stringcount);

    skip_dword_unknown(&ptr, 1);

    for (i = 0; i < stringcount; ++i)
    {
        DWORD id;
        struct d3dx_parameter *param;

        read_dword(&ptr, &id);
        TRACE("Id: %u\n", id);

        param = get_parameter_struct(base->objects[id]);

        hr = d3dx9_parse_data(param, &ptr);
        if (hr != D3D_OK)
        {
            WARN("Failed to parse data\n");
            goto err_out;
        }
    }

    HeapFree(GetProcessHeap(), 0, objects);
    base->objects = NULL;

    base->technique_handles = technique_handles;
    base->parameter_handles = parameter_handles;

    return D3D_OK;

err_out:

    if (technique_handles)
    {
        for (i = 0; i < base->technique_count; ++i)
        {
            free_technique(technique_handles[i]);
        }
        HeapFree(GetProcessHeap(), 0, technique_handles);
    }

    if (parameter_handles)
    {
        for (i = 0; i < base->parameter_count; ++i)
        {
            free_parameter(parameter_handles[i], FALSE, FALSE);
        }
        HeapFree(GetProcessHeap(), 0, parameter_handles);
    }

    HeapFree(GetProcessHeap(), 0, objects);

    return hr;
}

static HRESULT d3dx9_base_effect_init(struct ID3DXBaseEffectImpl *base,
        const char *data, SIZE_T data_size, struct ID3DXEffectImpl *effect)
{
    DWORD tag, offset;
    const char *ptr = data;
    HRESULT hr;

    TRACE("base %p, data %p, data_size %lu, effect %p\n", base, data, data_size, effect);

    base->ID3DXBaseEffect_iface.lpVtbl = &ID3DXBaseEffect_Vtbl;
    base->ref = 1;
    base->effect = effect;

    read_dword(&ptr, &tag);
    TRACE("Tag: %x\n", tag);

    if (tag != d3dx9_effect_version(9, 1))
    {
        /* todo: compile hlsl ascii code */
        FIXME("HLSL ascii effects not supported, yet\n");

        /* Show the start of the shader for debugging info. */
        TRACE("effect:\n%s\n", debugstr_an(data, data_size > 40 ? 40 : data_size));
    }
    else
    {
        read_dword(&ptr, &offset);
        TRACE("Offset: %x\n", offset);

        hr = d3dx9_parse_effect(base, ptr, data_size, offset);
        if (hr != D3D_OK)
        {
            FIXME("Failed to parse effect.\n");
            return hr;
        }
    }

    return D3D_OK;
}

static HRESULT d3dx9_effect_init(struct ID3DXEffectImpl *effect, LPDIRECT3DDEVICE9 device,
        const char *data, SIZE_T data_size, LPD3DXEFFECTPOOL pool)
{
    HRESULT hr;
    struct ID3DXBaseEffectImpl *object = NULL;

    TRACE("effect %p, device %p, data %p, data_size %lu, pool %p\n", effect, device, data, data_size, pool);

    effect->ID3DXEffect_iface.lpVtbl = &ID3DXEffect_Vtbl;
    effect->ref = 1;

    if (pool) pool->lpVtbl->AddRef(pool);
    effect->pool = pool;

    IDirect3DDevice9_AddRef(device);
    effect->device = device;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        hr = E_OUTOFMEMORY;
        goto err_out;
    }

    hr = d3dx9_base_effect_init(object, data, data_size, effect);
    if (hr != D3D_OK)
    {
        FIXME("Failed to parse effect.\n");
        goto err_out;
    }

    effect->base_effect = &object->ID3DXBaseEffect_iface;

    return D3D_OK;

err_out:

    HeapFree(GetProcessHeap(), 0, object);
    free_effect(effect);

    return hr;
}

HRESULT WINAPI D3DXCreateEffectEx(LPDIRECT3DDEVICE9 device,
                                  LPCVOID srcdata,
                                  UINT srcdatalen,
                                  CONST D3DXMACRO* defines,
                                  LPD3DXINCLUDE include,
                                  LPCSTR skip_constants,
                                  DWORD flags,
                                  LPD3DXEFFECTPOOL pool,
                                  LPD3DXEFFECT* effect,
                                  LPD3DXBUFFER* compilation_errors)
{
    struct ID3DXEffectImpl *object;
    HRESULT hr;

    FIXME("(%p, %p, %u, %p, %p, %p, %#x, %p, %p, %p): semi-stub\n", device, srcdata, srcdatalen, defines, include,
        skip_constants, flags, pool, effect, compilation_errors);

    if (!device || !srcdata)
        return D3DERR_INVALIDCALL;

    if (!srcdatalen)
        return E_FAIL;

    /* Native dll allows effect to be null so just return D3D_OK after doing basic checks */
    if (!effect)
        return D3D_OK;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    hr = d3dx9_effect_init(object, device, srcdata, srcdatalen, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize shader reflection\n");
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *effect = &object->ID3DXEffect_iface;

    TRACE("Created ID3DXEffect %p\n", object);

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateEffect(LPDIRECT3DDEVICE9 device,
                                LPCVOID srcdata,
                                UINT srcdatalen,
                                CONST D3DXMACRO* defines,
                                LPD3DXINCLUDE include,
                                DWORD flags,
                                LPD3DXEFFECTPOOL pool,
                                LPD3DXEFFECT* effect,
                                LPD3DXBUFFER* compilation_errors)
{
    TRACE("(%p, %p, %u, %p, %p, %#x, %p, %p, %p): Forwarded to D3DXCreateEffectEx\n", device, srcdata, srcdatalen, defines,
        include, flags, pool, effect, compilation_errors);

    return D3DXCreateEffectEx(device, srcdata, srcdatalen, defines, include, NULL, flags, pool, effect, compilation_errors);
}

static HRESULT d3dx9_effect_compiler_init(struct ID3DXEffectCompilerImpl *compiler, const char *data, SIZE_T data_size)
{
    HRESULT hr;
    struct ID3DXBaseEffectImpl *object = NULL;

    TRACE("effect %p, data %p, data_size %lu\n", compiler, data, data_size);

    compiler->ID3DXEffectCompiler_iface.lpVtbl = &ID3DXEffectCompiler_Vtbl;
    compiler->ref = 1;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        hr = E_OUTOFMEMORY;
        goto err_out;
    }

    hr = d3dx9_base_effect_init(object, data, data_size, NULL);
    if (hr != D3D_OK)
    {
        FIXME("Failed to parse effect.\n");
        goto err_out;
    }

    compiler->base_effect = &object->ID3DXBaseEffect_iface;

    return D3D_OK;

err_out:

    HeapFree(GetProcessHeap(), 0, object);
    free_effect_compiler(compiler);

    return hr;
}

HRESULT WINAPI D3DXCreateEffectCompiler(LPCSTR srcdata,
                                        UINT srcdatalen,
                                        CONST D3DXMACRO *defines,
                                        LPD3DXINCLUDE include,
                                        DWORD flags,
                                        LPD3DXEFFECTCOMPILER *compiler,
                                        LPD3DXBUFFER *parse_errors)
{
    struct ID3DXEffectCompilerImpl *object;
    HRESULT hr;

    TRACE("srcdata %p, srcdatalen %u, defines %p, include %p, flags %#x, compiler %p, parse_errors %p\n",
            srcdata, srcdatalen, defines, include, flags, compiler, parse_errors);

    if (!srcdata || !compiler)
    {
        WARN("Invalid arguments supplied\n");
        return D3DERR_INVALIDCALL;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    hr = d3dx9_effect_compiler_init(object, srcdata, srcdatalen);
    if (FAILED(hr))
    {
        WARN("Failed to initialize effect compiler\n");
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *compiler = &object->ID3DXEffectCompiler_iface;

    TRACE("Created ID3DXEffectCompiler %p\n", object);

    return D3D_OK;
}

static const struct ID3DXEffectPoolVtbl ID3DXEffectPool_Vtbl;

struct ID3DXEffectPoolImpl
{
    ID3DXEffectPool ID3DXEffectPool_iface;
    LONG ref;
};

static inline struct ID3DXEffectPoolImpl *impl_from_ID3DXEffectPool(ID3DXEffectPool *iface)
{
    return CONTAINING_RECORD(iface, struct ID3DXEffectPoolImpl, ID3DXEffectPool_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXEffectPoolImpl_QueryInterface(ID3DXEffectPool *iface, REFIID riid, void **object)
{
    struct ID3DXEffectPoolImpl *This = impl_from_ID3DXEffectPool(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXEffectPool))
    {
        This->ID3DXEffectPool_iface.lpVtbl->AddRef(iface);
        *object = This;
        return S_OK;
    }

    WARN("Interface %s not found\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXEffectPoolImpl_AddRef(ID3DXEffectPool *iface)
{
    struct ID3DXEffectPoolImpl *This = impl_from_ID3DXEffectPool(iface);

    TRACE("(%p)->(): AddRef from %u\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXEffectPoolImpl_Release(ID3DXEffectPool *iface)
{
    struct ID3DXEffectPoolImpl *This = impl_from_ID3DXEffectPool(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Release from %u\n", This, ref + 1);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static const struct ID3DXEffectPoolVtbl ID3DXEffectPool_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXEffectPoolImpl_QueryInterface,
    ID3DXEffectPoolImpl_AddRef,
    ID3DXEffectPoolImpl_Release
};

HRESULT WINAPI D3DXCreateEffectPool(LPD3DXEFFECTPOOL *pool)
{
    struct ID3DXEffectPoolImpl *object;

    TRACE("(%p)\n", pool);

    if (!pool)
        return D3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->ID3DXEffectPool_iface.lpVtbl = &ID3DXEffectPool_Vtbl;
    object->ref = 1;

    *pool = &object->ID3DXEffectPool_iface;

    return S_OK;
}

HRESULT WINAPI D3DXCreateEffectFromFileExW(LPDIRECT3DDEVICE9 device, LPCWSTR srcfile,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, LPCSTR skipconstants, DWORD flags,
    LPD3DXEFFECTPOOL pool, LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    LPVOID buffer;
    HRESULT ret;
    DWORD size;

    TRACE("(%s): relay\n", debugstr_w(srcfile));

    if (!device || !srcfile || !defines)
        return D3DERR_INVALIDCALL;

    ret = map_view_of_file(srcfile, &buffer, &size);

    if (FAILED(ret))
        return D3DXERR_INVALIDDATA;

    ret = D3DXCreateEffectEx(device, buffer, size, defines, include, skipconstants, flags, pool, effect, compilationerrors);
    UnmapViewOfFile(buffer);

    return ret;
}

HRESULT WINAPI D3DXCreateEffectFromFileExA(LPDIRECT3DDEVICE9 device, LPCSTR srcfile,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, LPCSTR skipconstants, DWORD flags,
    LPD3DXEFFECTPOOL pool, LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    LPWSTR srcfileW;
    HRESULT ret;
    DWORD len;

    TRACE("(void): relay\n");

    if (!srcfile || !defines)
        return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, srcfile, -1, NULL, 0);
    srcfileW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(*srcfileW));
    MultiByteToWideChar(CP_ACP, 0, srcfile, -1, srcfileW, len);

    ret = D3DXCreateEffectFromFileExW(device, srcfileW, defines, include, skipconstants, flags, pool, effect, compilationerrors);
    HeapFree(GetProcessHeap(), 0, srcfileW);

    return ret;
}

HRESULT WINAPI D3DXCreateEffectFromFileW(LPDIRECT3DDEVICE9 device, LPCWSTR srcfile,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, DWORD flags, LPD3DXEFFECTPOOL pool,
    LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    TRACE("(void): relay\n");
    return D3DXCreateEffectFromFileExW(device, srcfile, defines, include, NULL, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectFromFileA(LPDIRECT3DDEVICE9 device, LPCSTR srcfile,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, DWORD flags, LPD3DXEFFECTPOOL pool,
    LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    TRACE("(void): relay\n");
    return D3DXCreateEffectFromFileExA(device, srcfile, defines, include, NULL, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectFromResourceExW(LPDIRECT3DDEVICE9 device, HMODULE srcmodule, LPCWSTR srcresource,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, LPCSTR skipconstants, DWORD flags,
    LPD3DXEFFECTPOOL pool, LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    HRSRC resinfo;

    TRACE("(%p, %s): relay\n", srcmodule, debugstr_w(srcresource));

    if (!device || !defines)
        return D3DERR_INVALIDCALL;

    resinfo = FindResourceW(srcmodule, srcresource, (LPCWSTR) RT_RCDATA);

    if (resinfo)
    {
        LPVOID buffer;
        HRESULT ret;
        DWORD size;

        ret = load_resource_into_memory(srcmodule, resinfo, &buffer, &size);

        if (FAILED(ret))
            return D3DXERR_INVALIDDATA;

        return D3DXCreateEffectEx(device, buffer, size, defines, include, skipconstants, flags, pool, effect, compilationerrors);
    }

    return D3DXERR_INVALIDDATA;
}

HRESULT WINAPI D3DXCreateEffectFromResourceExA(LPDIRECT3DDEVICE9 device, HMODULE srcmodule, LPCSTR srcresource,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, LPCSTR skipconstants, DWORD flags,
    LPD3DXEFFECTPOOL pool, LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    HRSRC resinfo;

    TRACE("(%p, %s): relay\n", srcmodule, debugstr_a(srcresource));

    if (!device || !defines)
        return D3DERR_INVALIDCALL;

    resinfo = FindResourceA(srcmodule, srcresource, (LPCSTR) RT_RCDATA);

    if (resinfo)
    {
        LPVOID buffer;
        HRESULT ret;
        DWORD size;

        ret = load_resource_into_memory(srcmodule, resinfo, &buffer, &size);

        if (FAILED(ret))
            return D3DXERR_INVALIDDATA;

        return D3DXCreateEffectEx(device, buffer, size, defines, include, skipconstants, flags, pool, effect, compilationerrors);
    }

    return D3DXERR_INVALIDDATA;
}

HRESULT WINAPI D3DXCreateEffectFromResourceW(LPDIRECT3DDEVICE9 device, HMODULE srcmodule, LPCWSTR srcresource,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, DWORD flags, LPD3DXEFFECTPOOL pool,
    LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    TRACE("(void): relay\n");
    return D3DXCreateEffectFromResourceExW(device, srcmodule, srcresource, defines, include, NULL, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectFromResourceA(LPDIRECT3DDEVICE9 device, HMODULE srcmodule, LPCSTR srcresource,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, DWORD flags, LPD3DXEFFECTPOOL pool,
    LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    TRACE("(void): relay\n");
    return D3DXCreateEffectFromResourceExA(device, srcmodule, srcresource, defines, include, NULL, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectCompilerFromFileW(LPCWSTR srcfile, const D3DXMACRO *defines, LPD3DXINCLUDE include,
    DWORD flags, LPD3DXEFFECTCOMPILER *effectcompiler, LPD3DXBUFFER *parseerrors)
{
    LPVOID buffer;
    HRESULT ret;
    DWORD size;

    TRACE("(%s): relay\n", debugstr_w(srcfile));

    if (!srcfile || !defines)
        return D3DERR_INVALIDCALL;

    ret = map_view_of_file(srcfile, &buffer, &size);

    if (FAILED(ret))
        return D3DXERR_INVALIDDATA;

    ret = D3DXCreateEffectCompiler(buffer, size, defines, include, flags, effectcompiler, parseerrors);
    UnmapViewOfFile(buffer);

    return ret;
}

HRESULT WINAPI D3DXCreateEffectCompilerFromFileA(LPCSTR srcfile, const D3DXMACRO *defines, LPD3DXINCLUDE include,
    DWORD flags, LPD3DXEFFECTCOMPILER *effectcompiler, LPD3DXBUFFER *parseerrors)
{
    LPWSTR srcfileW;
    HRESULT ret;
    DWORD len;

    TRACE("(void): relay\n");

    if (!srcfile || !defines)
        return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, srcfile, -1, NULL, 0);
    srcfileW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(*srcfileW));
    MultiByteToWideChar(CP_ACP, 0, srcfile, -1, srcfileW, len);

    ret = D3DXCreateEffectCompilerFromFileW(srcfileW, defines, include, flags, effectcompiler, parseerrors);
    HeapFree(GetProcessHeap(), 0, srcfileW);

    return ret;
}

HRESULT WINAPI D3DXCreateEffectCompilerFromResourceA(HMODULE srcmodule, LPCSTR srcresource, const D3DXMACRO *defines,
    LPD3DXINCLUDE include, DWORD flags, LPD3DXEFFECTCOMPILER *effectcompiler, LPD3DXBUFFER *parseerrors)
{
    HRSRC resinfo;

    TRACE("(%p, %s): relay\n", srcmodule, debugstr_a(srcresource));

    resinfo = FindResourceA(srcmodule, srcresource, (LPCSTR) RT_RCDATA);

    if (resinfo)
    {
        LPVOID buffer;
        HRESULT ret;
        DWORD size;

        ret = load_resource_into_memory(srcmodule, resinfo, &buffer, &size);

        if (FAILED(ret))
            return D3DXERR_INVALIDDATA;

        return D3DXCreateEffectCompiler(buffer, size, defines, include, flags, effectcompiler, parseerrors);
    }

    return D3DXERR_INVALIDDATA;
}

HRESULT WINAPI D3DXCreateEffectCompilerFromResourceW(HMODULE srcmodule, LPCWSTR srcresource, const D3DXMACRO *defines,
    LPD3DXINCLUDE include, DWORD flags, LPD3DXEFFECTCOMPILER *effectcompiler, LPD3DXBUFFER *parseerrors)
{
    HRSRC resinfo;

    TRACE("(%p, %s): relay\n", srcmodule, debugstr_w(srcresource));

    resinfo = FindResourceW(srcmodule, srcresource, (LPCWSTR) RT_RCDATA);

    if (resinfo)
    {
        LPVOID buffer;
        HRESULT ret;
        DWORD size;

        ret = load_resource_into_memory(srcmodule, resinfo, &buffer, &size);

        if (FAILED(ret))
            return D3DXERR_INVALIDDATA;

        return D3DXCreateEffectCompiler(buffer, size, defines, include, flags, effectcompiler, parseerrors);
    }

    return D3DXERR_INVALIDDATA;
}
