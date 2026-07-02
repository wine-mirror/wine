/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2010 Rico Schüller
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

#include "initguid.h"
#include "d3dcompiler_private.h"
#include "d3d10.h"

#if !D3D_COMPILER_VERSION
#define ID3D11ShaderReflection ID3D10ShaderReflection
#define ID3D11ShaderReflectionVtbl ID3D10ShaderReflectionVtbl
#define ID3D11ShaderReflectionConstantBuffer ID3D10ShaderReflectionConstantBuffer
#define ID3D11ShaderReflectionConstantBufferVtbl ID3D10ShaderReflectionConstantBufferVtbl
#define ID3D11ShaderReflectionType ID3D10ShaderReflectionType
#define ID3D11ShaderReflectionTypeVtbl ID3D10ShaderReflectionTypeVtbl
#define ID3D11ShaderReflectionVariable ID3D10ShaderReflectionVariable
#define ID3D11ShaderReflectionVariableVtbl ID3D10ShaderReflectionVariableVtbl
#define IID_ID3D11ShaderReflection IID_ID3D10ShaderReflection
#define D3D11_SHADER_BUFFER_DESC D3D10_SHADER_BUFFER_DESC
#define D3D11_SHADER_DESC D3D10_SHADER_DESC
#define D3D11_SHADER_INPUT_BIND_DESC D3D10_SHADER_INPUT_BIND_DESC
#define D3D11_SHADER_TYPE_DESC D3D10_SHADER_TYPE_DESC
#define D3D11_SHADER_VARIABLE_DESC D3D10_SHADER_VARIABLE_DESC
#define D3D11_SIGNATURE_PARAMETER_DESC D3D10_SIGNATURE_PARAMETER_DESC
#endif

WINE_DEFAULT_DEBUG_CHANNEL(d3dcompiler);

enum D3DCOMPILER_SIGNATURE_ELEMENT_SIZE
{
    D3DCOMPILER_SIGNATURE_ELEMENT_SIZE6 = 6,
    D3DCOMPILER_SIGNATURE_ELEMENT_SIZE7 = 7,
};

#define D3DCOMPILER_SHADER_TARGET_VERSION_MASK 0xffff
#define D3DCOMPILER_SHADER_TARGET_SHADERTYPE_MASK 0xffff0000
#define D3DCOMPILER_SHADER_TARGET_SHADERTYPE_SHIFT 16

#define D3DCOMPILER_SHDR_SHADER_TYPE_CS 0x4353

struct d3dcompiler_shader_reflection_type
{
    ID3D11ShaderReflectionType ID3D11ShaderReflectionType_iface;

    uint32_t id;
    struct wine_rb_entry entry;

    struct d3dcompiler_shader_reflection *reflection;

    D3D11_SHADER_TYPE_DESC desc;
    struct d3dcompiler_shader_reflection_type_member *members;
    char *name;
};

struct d3dcompiler_shader_reflection_type_member
{
    char *name;
    uint32_t offset;
    struct d3dcompiler_shader_reflection_type *type;
};

struct d3dcompiler_shader_reflection_variable
{
    ID3D11ShaderReflectionVariable ID3D11ShaderReflectionVariable_iface;

    struct d3dcompiler_shader_reflection_constant_buffer *constant_buffer;
    struct d3dcompiler_shader_reflection_type *type;

    char *name;
    UINT start_offset;
    UINT size;
    UINT flags;
    void *default_value;
};

struct d3dcompiler_shader_reflection_constant_buffer
{
    ID3D11ShaderReflectionConstantBuffer ID3D11ShaderReflectionConstantBuffer_iface;

    struct d3dcompiler_shader_reflection *reflection;

    char *name;
    D3D_CBUFFER_TYPE type;
    UINT variable_count;
    UINT size;
    UINT flags;

    struct d3dcompiler_shader_reflection_variable *variables;
};

enum D3DCOMPILER_REFLECTION_VERSION
{
    D3DCOMPILER_REFLECTION_VERSION_D3D10,
    D3DCOMPILER_REFLECTION_VERSION_D3D11,
    D3DCOMPILER_REFLECTION_VERSION_D3D12,
};

/* ID3D11ShaderReflection */
struct d3dcompiler_shader_reflection
{
    ID3D11ShaderReflection ID3D11ShaderReflection_iface;
    LONG refcount;

    enum D3DCOMPILER_REFLECTION_VERSION interface_version;

    struct vkd3d_shader_scan_signature_info signature_info;
    struct vkd3d_shader_scan_thread_group_size_info thread_group_size_info;

    D3D11_SHADER_DESC desc;

    uint32_t target;

    UINT mov_instruction_count;
    UINT conversion_instruction_count;

    char *resource_string;
    D3D12_SHADER_INPUT_BIND_DESC *bound_resources;
    struct d3dcompiler_shader_reflection_constant_buffer *constant_buffers;
    struct wine_rb_tree types;
};

static struct d3dcompiler_shader_reflection_type *get_reflection_type(struct d3dcompiler_shader_reflection *reflection, const char *data, uint32_t offset);

static const struct ID3D11ShaderReflectionConstantBufferVtbl d3dcompiler_shader_reflection_constant_buffer_vtbl;
static const struct ID3D11ShaderReflectionVariableVtbl d3dcompiler_shader_reflection_variable_vtbl;
static const struct ID3D11ShaderReflectionTypeVtbl d3dcompiler_shader_reflection_type_vtbl;

/* null objects - needed for invalid calls */
static struct d3dcompiler_shader_reflection_constant_buffer null_constant_buffer =
{
    {&d3dcompiler_shader_reflection_constant_buffer_vtbl},
};
static struct d3dcompiler_shader_reflection_type null_type =
{
    {&d3dcompiler_shader_reflection_type_vtbl},
};
static struct d3dcompiler_shader_reflection_variable null_variable =
{
    {&d3dcompiler_shader_reflection_variable_vtbl},
    &null_constant_buffer,
    &null_type
};

static BOOL copy_name(const char *ptr, char **name)
{
    if (!ptr || !ptr[0]) return TRUE;

    *name = strdup(ptr);
    if (!*name)
    {
        ERR("Failed to allocate name memory.\n");
        return FALSE;
    }

    return TRUE;
}

static BOOL copy_value(const char *ptr, void **value, uint32_t size)
{
    if (!ptr || !size) return TRUE;

    *value = malloc(size);
    if (!*value)
    {
        ERR("Failed to allocate value memory.\n");
        return FALSE;
    }

    memcpy(*value, ptr, size);

    return TRUE;
}

static int d3dcompiler_shader_reflection_type_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct d3dcompiler_shader_reflection_type *t = WINE_RB_ENTRY_VALUE(entry, const struct d3dcompiler_shader_reflection_type, entry);
    const uint32_t *id = key;

    return *id - t->id;
}

static void free_type_member(struct d3dcompiler_shader_reflection_type_member *member)
{
    if (member)
    {
        free(member->name);
    }
}

static void d3dcompiler_shader_reflection_type_destroy(struct wine_rb_entry *entry, void *context)
{
    struct d3dcompiler_shader_reflection_type *t = WINE_RB_ENTRY_VALUE(entry, struct d3dcompiler_shader_reflection_type, entry);
    unsigned int i;

    TRACE("reflection type %p.\n", t);

    if (t->members)
    {
        for (i = 0; i < t->desc.Members; ++i)
        {
            free_type_member(&t->members[i]);
        }
        free(t->members);
    }

    free(t->name);
    free(t);
}

static void free_variable(struct d3dcompiler_shader_reflection_variable *var)
{
    if (var)
    {
        free(var->name);
        free(var->default_value);
    }
}

static void free_constant_buffer(struct d3dcompiler_shader_reflection_constant_buffer *cb)
{
    if (cb->variables)
    {
        unsigned int i;

        for (i = 0; i < cb->variable_count; ++i)
        {
            free_variable(&cb->variables[i]);
        }
        free(cb->variables);
    }

    free(cb->name);
}

static void reflection_cleanup(struct d3dcompiler_shader_reflection *ref)
{
    TRACE("Cleanup %p\n", ref);

    if (ref->constant_buffers)
    {
        unsigned int i;

        for (i = 0; i < ref->desc.ConstantBuffers; ++i)
        {
            free_constant_buffer(&ref->constant_buffers[i]);
        }
    }

    wine_rb_destroy(&ref->types, d3dcompiler_shader_reflection_type_destroy, NULL);
    free(ref->constant_buffers);
    free(ref->bound_resources);
    free(ref->resource_string);
    free((void *)ref->desc.Creator);

    vkd3d_shader_free_scan_signature_info(&ref->signature_info);
}

/* IUnknown methods */

static inline struct d3dcompiler_shader_reflection *impl_from_ID3D11ShaderReflection(ID3D11ShaderReflection *iface)
{
    return CONTAINING_RECORD(iface, struct d3dcompiler_shader_reflection, ID3D11ShaderReflection_iface);
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_QueryInterface(ID3D11ShaderReflection *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11ShaderReflection)
            || IsEqualGUID(riid, &IID_IUnknown)
            || (D3D_COMPILER_VERSION >= 47 && IsEqualGUID(riid, &IID_ID3D12ShaderReflection)))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3dcompiler_shader_reflection_AddRef(ID3D11ShaderReflection *iface)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %lu.\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3dcompiler_shader_reflection_Release(ID3D11ShaderReflection *iface)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %lu.\n", This, refcount);

    if (!refcount)
    {
        reflection_cleanup(This);
        free(This);
    }

    return refcount;
}

/* ID3D11ShaderReflection methods */

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetDesc(ID3D11ShaderReflection *iface, D3D11_SHADER_DESC *desc)
{
    struct d3dcompiler_shader_reflection *reflection = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid argument specified\n");
        return E_FAIL;
    }

    *desc = reflection->desc;

    return S_OK;
}

static struct ID3D11ShaderReflectionConstantBuffer * STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetConstantBufferByIndex(
        ID3D11ShaderReflection *iface, UINT index)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->desc.ConstantBuffers)
    {
        WARN("Invalid argument specified\n");
        return &null_constant_buffer.ID3D11ShaderReflectionConstantBuffer_iface;
    }

    return &This->constant_buffers[index].ID3D11ShaderReflectionConstantBuffer_iface;
}

static struct ID3D11ShaderReflectionConstantBuffer * STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetConstantBufferByName(
        ID3D11ShaderReflection *iface, const char *name)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);
    unsigned int i;

    TRACE("iface %p, name %s\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid argument specified\n");
        return &null_constant_buffer.ID3D11ShaderReflectionConstantBuffer_iface;
    }

    for (i = 0; i < This->desc.ConstantBuffers; ++i)
    {
        struct d3dcompiler_shader_reflection_constant_buffer *d = &This->constant_buffers[i];

        if (!strcmp(d->name, name))
        {
            TRACE("Returning ID3D11ShaderReflectionConstantBuffer %p.\n", d);
            return &d->ID3D11ShaderReflectionConstantBuffer_iface;
        }
    }

    WARN("Invalid name specified\n");

    return &null_constant_buffer.ID3D11ShaderReflectionConstantBuffer_iface;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetResourceBindingDesc(
        ID3D11ShaderReflection *iface, UINT index, D3D11_SHADER_INPUT_BIND_DESC *desc)
{
    struct d3dcompiler_shader_reflection *reflection = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p, index %u, desc %p\n", iface, index, desc);

    if (!desc || index >= reflection->desc.BoundResources)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    memcpy(desc, &reflection->bound_resources[index],
            reflection->interface_version == D3DCOMPILER_REFLECTION_VERSION_D3D12
            ? sizeof(D3D12_SHADER_INPUT_BIND_DESC) : sizeof(D3D11_SHADER_INPUT_BIND_DESC));

    return S_OK;
}

static HRESULT get_signature_parameter(const struct vkd3d_shader_signature *signature,
        unsigned int index, D3D11_SIGNATURE_PARAMETER_DESC *desc, bool output)
{
    const struct vkd3d_shader_signature_element *e;

    if (!desc || index >= signature->element_count)
    {
        WARN("Invalid argument specified.\n");
        return E_INVALIDARG;
    }
    e = &signature->elements[index];

    desc->SemanticName = e->semantic_name;
    desc->SemanticIndex = e->semantic_index;
    desc->Register = e->register_index;
    desc->SystemValueType = (D3D_NAME)e->sysval_semantic;
    desc->ComponentType = (D3D_REGISTER_COMPONENT_TYPE)e->component_type;
    desc->Mask = e->mask;
    desc->ReadWriteMask = output ? (0xf ^ e->used_mask) : e->used_mask;
#if D3D_COMPILER_VERSION
    desc->Stream = e->stream_index;
#endif
#if D3D_COMPILER_VERSION >= 46
    desc->MinPrecision = (D3D_MIN_PRECISION)e->min_precision;
#endif

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetInputParameterDesc(
        ID3D11ShaderReflection *iface, UINT index, D3D11_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3dcompiler_shader_reflection *reflection = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    return get_signature_parameter(&reflection->signature_info.input, index, desc, false);
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetOutputParameterDesc(
        ID3D11ShaderReflection *iface, UINT index, D3D11_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3dcompiler_shader_reflection *reflection = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    return get_signature_parameter(&reflection->signature_info.output, index, desc, true);
}

#if D3D_COMPILER_VERSION
static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetPatchConstantParameterDesc(
        ID3D11ShaderReflection *iface, UINT index, D3D11_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3dcompiler_shader_reflection *reflection = impl_from_ID3D11ShaderReflection(iface);
    bool output = ((reflection->desc.Version & 0xffff0000) >> 16) == D3D12_SHVER_HULL_SHADER;

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    return get_signature_parameter(&reflection->signature_info.patch_constant, index, desc, output);
}

static struct ID3D11ShaderReflectionVariable * STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetVariableByName(
        ID3D11ShaderReflection *iface, const char *name)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);
    unsigned int i, k;

    TRACE("iface %p, name %s\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid name specified\n");
        return &null_variable.ID3D11ShaderReflectionVariable_iface;
    }

    for (i = 0; i < This->desc.ConstantBuffers; ++i)
    {
        struct d3dcompiler_shader_reflection_constant_buffer *cb = &This->constant_buffers[i];

        for (k = 0; k < cb->variable_count; ++k)
        {
            struct d3dcompiler_shader_reflection_variable *v = &cb->variables[k];

            if (!strcmp(v->name, name))
            {
                TRACE("Returning ID3D11ShaderReflectionVariable %p.\n", v);
                return &v->ID3D11ShaderReflectionVariable_iface;
            }
        }
    }

    WARN("Invalid name specified\n");

    return &null_variable.ID3D11ShaderReflectionVariable_iface;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetResourceBindingDescByName(
        ID3D11ShaderReflection *iface, const char *name, D3D11_SHADER_INPUT_BIND_DESC *desc)
{
    struct d3dcompiler_shader_reflection *reflection = impl_from_ID3D11ShaderReflection(iface);
    unsigned int i;

    TRACE("iface %p, name %s, desc %p\n", iface, debugstr_a(name), desc);

    if (!desc || !name)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    for (i = 0; i < reflection->desc.BoundResources; ++i)
    {
        D3D12_SHADER_INPUT_BIND_DESC *d = &reflection->bound_resources[i];

        if (!strcmp(d->Name, name))
        {
            TRACE("Returning D3D11_SHADER_INPUT_BIND_DESC %p.\n", d);
            memcpy(desc, d, reflection->interface_version == D3DCOMPILER_REFLECTION_VERSION_D3D12
                    ? sizeof(D3D12_SHADER_INPUT_BIND_DESC) : sizeof(D3D11_SHADER_INPUT_BIND_DESC));
            return S_OK;
        }
    }

    WARN("Invalid name specified\n");

    return E_INVALIDARG;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetMovInstructionCount(
        ID3D11ShaderReflection *iface)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p\n", iface);

    return This->mov_instruction_count;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetMovcInstructionCount(
        ID3D11ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetConversionInstructionCount(
        ID3D11ShaderReflection *iface)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p\n", iface);

    return This->conversion_instruction_count;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetBitwiseInstructionCount(
        ID3D11ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static D3D_PRIMITIVE STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetGSInputPrimitive(
        ID3D11ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static BOOL STDMETHODCALLTYPE d3dcompiler_shader_reflection_IsSampleFrequencyShader(
        ID3D11ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetNumInterfaceSlots(
        ID3D11ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetMinFeatureLevel(
        ID3D11ShaderReflection *iface, D3D_FEATURE_LEVEL *level)
{
    FIXME("iface %p, level %p stub!\n", iface, level);

    return E_NOTIMPL;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetThreadGroupSize(
        ID3D11ShaderReflection *iface, UINT *sizex, UINT *sizey, UINT *sizez)
{
    struct d3dcompiler_shader_reflection *reflection = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p, sizex %p, sizey %p, sizez %p.\n", iface, sizex, sizey, sizez);

    if (!sizex || !sizey || !sizez)
    {
        WARN("Invalid argument specified.\n");
        return E_INVALIDARG;
    }

    *sizex = reflection->thread_group_size_info.x;
    *sizey = reflection->thread_group_size_info.y;
    *sizez = reflection->thread_group_size_info.z;

    return *sizex * *sizey * *sizez;
}

static UINT64 STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetRequiresFlags(
        ID3D11ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}
#endif

static const struct ID3D11ShaderReflectionVtbl d3dcompiler_shader_reflection_vtbl =
{
    /* IUnknown methods */
    d3dcompiler_shader_reflection_QueryInterface,
    d3dcompiler_shader_reflection_AddRef,
    d3dcompiler_shader_reflection_Release,
    /* ID3D11ShaderReflection methods */
    d3dcompiler_shader_reflection_GetDesc,
    d3dcompiler_shader_reflection_GetConstantBufferByIndex,
    d3dcompiler_shader_reflection_GetConstantBufferByName,
    d3dcompiler_shader_reflection_GetResourceBindingDesc,
    d3dcompiler_shader_reflection_GetInputParameterDesc,
    d3dcompiler_shader_reflection_GetOutputParameterDesc,
#if D3D_COMPILER_VERSION
    d3dcompiler_shader_reflection_GetPatchConstantParameterDesc,
    d3dcompiler_shader_reflection_GetVariableByName,
    d3dcompiler_shader_reflection_GetResourceBindingDescByName,
    d3dcompiler_shader_reflection_GetMovInstructionCount,
    d3dcompiler_shader_reflection_GetMovcInstructionCount,
    d3dcompiler_shader_reflection_GetConversionInstructionCount,
    d3dcompiler_shader_reflection_GetBitwiseInstructionCount,
    d3dcompiler_shader_reflection_GetGSInputPrimitive,
    d3dcompiler_shader_reflection_IsSampleFrequencyShader,
    d3dcompiler_shader_reflection_GetNumInterfaceSlots,
    d3dcompiler_shader_reflection_GetMinFeatureLevel,
    d3dcompiler_shader_reflection_GetThreadGroupSize,
    d3dcompiler_shader_reflection_GetRequiresFlags,
#endif
};

/* ID3D11ShaderReflectionConstantBuffer methods */

static inline struct d3dcompiler_shader_reflection_constant_buffer *impl_from_ID3D11ShaderReflectionConstantBuffer(ID3D11ShaderReflectionConstantBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct d3dcompiler_shader_reflection_constant_buffer, ID3D11ShaderReflectionConstantBuffer_iface);
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_constant_buffer_GetDesc(
        ID3D11ShaderReflectionConstantBuffer *iface, D3D11_SHADER_BUFFER_DESC *desc)
{
    struct d3dcompiler_shader_reflection_constant_buffer *This = impl_from_ID3D11ShaderReflectionConstantBuffer(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    if (This == &null_constant_buffer)
    {
        WARN("Null constant buffer specified\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("Invalid argument specified\n");
        return E_FAIL;
    }

    desc->Name = This->name;
    desc->Type = This->type;
    desc->Variables = This->variable_count;
    desc->Size = This->size;
    desc->uFlags = This->flags;

    return S_OK;
}

static ID3D11ShaderReflectionVariable * STDMETHODCALLTYPE d3dcompiler_shader_reflection_constant_buffer_GetVariableByIndex(
        ID3D11ShaderReflectionConstantBuffer *iface, UINT index)
{
    struct d3dcompiler_shader_reflection_constant_buffer *This = impl_from_ID3D11ShaderReflectionConstantBuffer(iface);

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->variable_count)
    {
        WARN("Invalid index specified\n");
        return &null_variable.ID3D11ShaderReflectionVariable_iface;
    }

    return &This->variables[index].ID3D11ShaderReflectionVariable_iface;
}

static ID3D11ShaderReflectionVariable * STDMETHODCALLTYPE d3dcompiler_shader_reflection_constant_buffer_GetVariableByName(
        ID3D11ShaderReflectionConstantBuffer *iface, const char *name)
{
    struct d3dcompiler_shader_reflection_constant_buffer *This = impl_from_ID3D11ShaderReflectionConstantBuffer(iface);
    unsigned int i;

    TRACE("iface %p, name %s\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid argument specified\n");
        return &null_variable.ID3D11ShaderReflectionVariable_iface;
    }

    for (i = 0; i < This->variable_count; ++i)
    {
        struct d3dcompiler_shader_reflection_variable *v = &This->variables[i];

        if (!strcmp(v->name, name))
        {
            TRACE("Returning ID3D11ShaderReflectionVariable %p.\n", v);
            return &v->ID3D11ShaderReflectionVariable_iface;
        }
    }

    WARN("Invalid name specified\n");

    return &null_variable.ID3D11ShaderReflectionVariable_iface;
}

static const struct ID3D11ShaderReflectionConstantBufferVtbl d3dcompiler_shader_reflection_constant_buffer_vtbl =
{
    /* ID3D11ShaderReflectionConstantBuffer methods */
    d3dcompiler_shader_reflection_constant_buffer_GetDesc,
    d3dcompiler_shader_reflection_constant_buffer_GetVariableByIndex,
    d3dcompiler_shader_reflection_constant_buffer_GetVariableByName,
};

/* ID3D11ShaderReflectionVariable methods */

static inline struct d3dcompiler_shader_reflection_variable *impl_from_ID3D11ShaderReflectionVariable(ID3D11ShaderReflectionVariable *iface)
{
    return CONTAINING_RECORD(iface, struct d3dcompiler_shader_reflection_variable, ID3D11ShaderReflectionVariable_iface);
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_variable_GetDesc(
        ID3D11ShaderReflectionVariable *iface, D3D11_SHADER_VARIABLE_DESC *desc)
{
    struct d3dcompiler_shader_reflection_variable *This = impl_from_ID3D11ShaderReflectionVariable(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    if (This == &null_variable)
    {
        WARN("Null variable specified\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("Invalid argument specified\n");
        return E_FAIL;
    }

    desc->Name = This->name;
    desc->StartOffset = This->start_offset;
    desc->Size = This->size;
    desc->uFlags = This->flags;
    desc->DefaultValue = This->default_value;

#if D3D_COMPILER_VERSION
    /* TODO test and set proper values for texture. */
    desc->StartTexture = 0xffffffff;
    desc->TextureSize = 0;
    desc->StartSampler = 0xffffffff;
    desc->SamplerSize = 0;
#endif

    return S_OK;
}

static ID3D11ShaderReflectionType * STDMETHODCALLTYPE d3dcompiler_shader_reflection_variable_GetType(
        ID3D11ShaderReflectionVariable *iface)
{
    struct d3dcompiler_shader_reflection_variable *This = impl_from_ID3D11ShaderReflectionVariable(iface);

    TRACE("iface %p\n", iface);

    return &This->type->ID3D11ShaderReflectionType_iface;
}

#if D3D_COMPILER_VERSION
static ID3D11ShaderReflectionConstantBuffer * STDMETHODCALLTYPE d3dcompiler_shader_reflection_variable_GetBuffer(
        ID3D11ShaderReflectionVariable *iface)
{
    struct d3dcompiler_shader_reflection_variable *This = impl_from_ID3D11ShaderReflectionVariable(iface);

    TRACE("iface %p\n", iface);

    return &This->constant_buffer->ID3D11ShaderReflectionConstantBuffer_iface;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_variable_GetInterfaceSlot(
        ID3D11ShaderReflectionVariable *iface, UINT index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return 0;
}
#endif

static const struct ID3D11ShaderReflectionVariableVtbl d3dcompiler_shader_reflection_variable_vtbl =
{
    /* ID3D11ShaderReflectionVariable methods */
    d3dcompiler_shader_reflection_variable_GetDesc,
    d3dcompiler_shader_reflection_variable_GetType,
#if D3D_COMPILER_VERSION
    d3dcompiler_shader_reflection_variable_GetBuffer,
    d3dcompiler_shader_reflection_variable_GetInterfaceSlot,
#endif
};

/* ID3D11ShaderReflectionType methods */

static inline struct d3dcompiler_shader_reflection_type *impl_from_ID3D11ShaderReflectionType(ID3D11ShaderReflectionType *iface)
{
    return CONTAINING_RECORD(iface, struct d3dcompiler_shader_reflection_type, ID3D11ShaderReflectionType_iface);
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetDesc(
        ID3D11ShaderReflectionType *iface, D3D11_SHADER_TYPE_DESC *desc)
{
    struct d3dcompiler_shader_reflection_type *This = impl_from_ID3D11ShaderReflectionType(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    if (This == &null_type)
    {
        WARN("Null type specified\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("Invalid argument specified\n");
        return E_FAIL;
    }

    *desc = This->desc;

    return S_OK;
}

static ID3D11ShaderReflectionType * STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetMemberTypeByIndex(
        ID3D11ShaderReflectionType *iface, UINT index)
{
    struct d3dcompiler_shader_reflection_type *This = impl_from_ID3D11ShaderReflectionType(iface);

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->desc.Members)
    {
        WARN("Invalid index specified\n");
        return &null_type.ID3D11ShaderReflectionType_iface;
    }

    return &This->members[index].type->ID3D11ShaderReflectionType_iface;
}

static ID3D11ShaderReflectionType * STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetMemberTypeByName(
        ID3D11ShaderReflectionType *iface, const char *name)
{
    struct d3dcompiler_shader_reflection_type *This = impl_from_ID3D11ShaderReflectionType(iface);
    unsigned int i;

    TRACE("iface %p, name %s\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid argument specified\n");
        return &null_type.ID3D11ShaderReflectionType_iface;
    }

    for (i = 0; i < This->desc.Members; ++i)
    {
        struct d3dcompiler_shader_reflection_type_member *member = &This->members[i];

        if (!strcmp(member->name, name))
        {
            TRACE("Returning ID3D11ShaderReflectionType %p.\n", member->type);
            return &member->type->ID3D11ShaderReflectionType_iface;
        }
    }

    WARN("Invalid name specified\n");

    return &null_type.ID3D11ShaderReflectionType_iface;
}

static const char * STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetMemberTypeName(
        ID3D11ShaderReflectionType *iface, UINT index)
{
    struct d3dcompiler_shader_reflection_type *This = impl_from_ID3D11ShaderReflectionType(iface);

    TRACE("iface %p, index %u\n", iface, index);

    if (This == &null_type)
    {
        WARN("Null type specified\n");
        return "$Invalid";
    }

    if (index >= This->desc.Members)
    {
        WARN("Invalid index specified\n");
        return NULL;
    }

    return This->members[index].name;
}

#if D3D_COMPILER_VERSION
static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_IsEqual(
        ID3D11ShaderReflectionType *iface, ID3D11ShaderReflectionType *type)
{
    struct d3dcompiler_shader_reflection_type *This = impl_from_ID3D11ShaderReflectionType(iface);

    TRACE("iface %p, type %p\n", iface, type);

    if (This == &null_type)
    {
        WARN("Null type specified\n");
        return E_FAIL;
    }

    if (iface == type)
        return S_OK;

    return S_FALSE;
}

static ID3D11ShaderReflectionType * STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetSubType(
        ID3D11ShaderReflectionType *iface)
{
    FIXME("iface %p stub!\n", iface);

    return NULL;
}

static ID3D11ShaderReflectionType * STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetBaseClass(
        ID3D11ShaderReflectionType *iface)
{
    FIXME("iface %p stub!\n", iface);

    return NULL;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetNumInterfaces(
        ID3D11ShaderReflectionType *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static ID3D11ShaderReflectionType * STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetInterfaceByIndex(
        ID3D11ShaderReflectionType *iface, UINT index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return NULL;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_IsOfType(
        ID3D11ShaderReflectionType *iface, ID3D11ShaderReflectionType *type)
{
    FIXME("iface %p, type %p stub!\n", iface, type);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_ImplementsInterface(
        ID3D11ShaderReflectionType *iface, ID3D11ShaderReflectionType *base)
{
    FIXME("iface %p, base %p stub!\n", iface, base);

    return E_NOTIMPL;
}
#endif

static const struct ID3D11ShaderReflectionTypeVtbl d3dcompiler_shader_reflection_type_vtbl =
{
    /* ID3D11ShaderReflectionType methods */
    d3dcompiler_shader_reflection_type_GetDesc,
    d3dcompiler_shader_reflection_type_GetMemberTypeByIndex,
    d3dcompiler_shader_reflection_type_GetMemberTypeByName,
    d3dcompiler_shader_reflection_type_GetMemberTypeName,
#if D3D_COMPILER_VERSION
    d3dcompiler_shader_reflection_type_IsEqual,
    d3dcompiler_shader_reflection_type_GetSubType,
    d3dcompiler_shader_reflection_type_GetBaseClass,
    d3dcompiler_shader_reflection_type_GetNumInterfaces,
    d3dcompiler_shader_reflection_type_GetInterfaceByIndex,
    d3dcompiler_shader_reflection_type_IsOfType,
    d3dcompiler_shader_reflection_type_ImplementsInterface,
#endif
};

static HRESULT d3dcompiler_parse_stat(struct d3dcompiler_shader_reflection *r, const char *data, size_t data_size)
{
    const char *ptr = data;
    size_t size = data_size >> 2;

    TRACE("Size %Iu.\n", size);

    r->desc.InstructionCount = read_u32(&ptr);
    TRACE("InstructionCount: %u.\n", r->desc.InstructionCount);

    r->desc.TempRegisterCount = read_u32(&ptr);
    TRACE("TempRegisterCount: %u.\n", r->desc.TempRegisterCount);

    r->desc.DefCount = read_u32(&ptr);
    TRACE("DefCount: %u.\n", r->desc.DefCount);

    r->desc.DclCount = read_u32(&ptr);
    TRACE("DclCount: %u.\n", r->desc.DclCount);

    r->desc.FloatInstructionCount = read_u32(&ptr);
    TRACE("FloatInstructionCount: %u.\n", r->desc.FloatInstructionCount);

    r->desc.IntInstructionCount = read_u32(&ptr);
    TRACE("IntInstructionCount: %u.\n", r->desc.IntInstructionCount);

    r->desc.UintInstructionCount = read_u32(&ptr);
    TRACE("UintInstructionCount: %u.\n", r->desc.UintInstructionCount);

    r->desc.StaticFlowControlCount = read_u32(&ptr);
    TRACE("StaticFlowControlCount: %u.\n", r->desc.StaticFlowControlCount);

    r->desc.DynamicFlowControlCount = read_u32(&ptr);
    TRACE("DynamicFlowControlCount: %u.\n", r->desc.DynamicFlowControlCount);

    r->desc.MacroInstructionCount = read_u32(&ptr);
    TRACE("MacroInstructionCount: %u.\n", r->desc.MacroInstructionCount);

    r->desc.TempArrayCount = read_u32(&ptr);
    TRACE("TempArrayCount: %u.\n", r->desc.TempArrayCount);

    r->desc.ArrayInstructionCount = read_u32(&ptr);
    TRACE("ArrayInstructionCount: %u.\n", r->desc.ArrayInstructionCount);

    r->desc.CutInstructionCount = read_u32(&ptr);
    TRACE("CutInstructionCount: %u.\n", r->desc.CutInstructionCount);

    r->desc.EmitInstructionCount = read_u32(&ptr);
    TRACE("EmitInstructionCount: %u.\n", r->desc.EmitInstructionCount);

    r->desc.TextureNormalInstructions = read_u32(&ptr);
    TRACE("TextureNormalInstructions: %u.\n", r->desc.TextureNormalInstructions);

    r->desc.TextureLoadInstructions = read_u32(&ptr);
    TRACE("TextureLoadInstructions: %u.\n", r->desc.TextureLoadInstructions);

    r->desc.TextureCompInstructions = read_u32(&ptr);
    TRACE("TextureCompInstructions: %u.\n", r->desc.TextureCompInstructions);

    r->desc.TextureBiasInstructions = read_u32(&ptr);
    TRACE("TextureBiasInstructions: %u.\n", r->desc.TextureBiasInstructions);

    r->desc.TextureGradientInstructions = read_u32(&ptr);
    TRACE("TextureGradientInstructions: %u.\n", r->desc.TextureGradientInstructions);

    r->mov_instruction_count = read_u32(&ptr);
    TRACE("MovInstructionCount: %u.\n", r->mov_instruction_count);

    skip_u32_unknown(&ptr, 1);

    r->conversion_instruction_count = read_u32(&ptr);
    TRACE("ConversionInstructionCount: %u.\n", r->conversion_instruction_count);

    skip_u32_unknown(&ptr, 1);

#if D3D_COMPILER_VERSION
    r->desc.InputPrimitive = read_u32(&ptr);
    TRACE("InputPrimitive: %x.\n", r->desc.InputPrimitive);

    r->desc.GSOutputTopology = read_u32(&ptr);
    TRACE("GSOutputTopology: %x.\n", r->desc.GSOutputTopology);

    r->desc.GSMaxOutputVertexCount = read_u32(&ptr);
    TRACE("GSMaxOutputVertexCount: %u.\n", r->desc.GSMaxOutputVertexCount);

    skip_u32_unknown(&ptr, 2);

    /* old dx10 stat size */
    if (size == 28) return S_OK;

    skip_u32_unknown(&ptr, 1);

    /* dx10 stat size */
    if (size == 29) return S_OK;

    skip_u32_unknown(&ptr, 1);

    r->desc.cControlPoints = read_u32(&ptr);
    TRACE("cControlPoints: %u.\n", r->desc.cControlPoints);

    r->desc.HSOutputPrimitive = read_u32(&ptr);
    TRACE("HSOutputPrimitive: %x.\n", r->desc.HSOutputPrimitive);

    r->desc.HSPartitioning = read_u32(&ptr);
    TRACE("HSPartitioning: %x.\n", r->desc.HSPartitioning);

    r->desc.TessellatorDomain = read_u32(&ptr);
    TRACE("TessellatorDomain: %x.\n", r->desc.TessellatorDomain);

    skip_u32_unknown(&ptr, 3);

    /* dx11 stat size */
    if (size == 37) return S_OK;
#else
    if (size == 28 || size == 29 || size == 37)
        return S_OK;
#endif

    FIXME("Unhandled size %Iu.\n", size);

    return E_FAIL;
}

static HRESULT d3dcompiler_parse_type_members(struct d3dcompiler_shader_reflection *ref,
        struct d3dcompiler_shader_reflection_type_member *member, const char *data, const char **ptr)
{
    uint32_t offset;

    offset = read_u32(ptr);
    if (!copy_name(data + offset, &member->name))
    {
        ERR("Failed to copy name.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Member name: %s.\n", debugstr_a(member->name));

    offset = read_u32(ptr);
    TRACE("Member type offset: %x.\n", offset);

    member->type = get_reflection_type(ref, data, offset);
    if (!member->type)
    {
        ERR("Failed to get member type\n");
        free(member->name);
        return E_FAIL;
    }

    member->offset = read_u32(ptr);
    TRACE("Member offset %x.\n", member->offset);

    return S_OK;
}

static HRESULT d3dcompiler_parse_type(struct d3dcompiler_shader_reflection_type *type, const char *data, uint32_t offset)
{
    const char *ptr = data + offset;
    uint32_t temp;
    D3D11_SHADER_TYPE_DESC *desc;
    unsigned int i;
    struct d3dcompiler_shader_reflection_type_member *members = NULL;
    HRESULT hr;
    uint32_t member_offset;

    desc = &type->desc;

    temp = read_u32(&ptr);
    desc->Class = temp & 0xffff;
    desc->Type = temp >> 16;
    TRACE("Class %s, Type %s\n", debug_d3dcompiler_shader_variable_class(desc->Class),
            debug_d3dcompiler_shader_variable_type(desc->Type));

    temp = read_u32(&ptr);
    desc->Rows = temp & 0xffff;
    desc->Columns = temp >> 16;
    TRACE("Rows %u, Columns %u\n", desc->Rows, desc->Columns);

    temp = read_u32(&ptr);
    desc->Elements = temp & 0xffff;
    desc->Members = temp >> 16;
    TRACE("Elements %u, Members %u\n", desc->Elements, desc->Members);

    member_offset = read_u32(&ptr);
    TRACE("Member Offset %u.\n", member_offset);

    if ((type->reflection->target & D3DCOMPILER_SHADER_TARGET_VERSION_MASK) >= 0x500)
        skip_u32_unknown(&ptr, 4);

    if (desc->Members)
    {
        const char *ptr2 = data + member_offset;

        members = calloc(desc->Members, sizeof(*members));
        if (!members)
        {
            ERR("Failed to allocate type memory.\n");
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < desc->Members; ++i)
        {
            hr = d3dcompiler_parse_type_members(type->reflection, &members[i], data, &ptr2);
            if (hr != S_OK)
            {
                FIXME("Failed to parse type members.\n");
                goto err_out;
            }
        }
    }

#if D3D_COMPILER_VERSION
    if ((type->reflection->target & D3DCOMPILER_SHADER_TARGET_VERSION_MASK) >= 0x500)
    {
        offset = read_u32(&ptr);
        if (!copy_name(data + offset, &type->name))
        {
            ERR("Failed to copy name.\n");
            free(members);
            return E_OUTOFMEMORY;
        }
        desc->Name = type->name;
        TRACE("Type name: %s.\n", debugstr_a(type->name));
    }
#endif

    type->members = members;

    return S_OK;

err_out:
    for (i = 0; i < desc->Members; ++i)
    {
        free_type_member(&members[i]);
    }
    free(members);
    return hr;
}

static struct d3dcompiler_shader_reflection_type *get_reflection_type(struct d3dcompiler_shader_reflection *reflection, const char *data, uint32_t offset)
{
    struct d3dcompiler_shader_reflection_type *type;
    struct wine_rb_entry *entry;
    HRESULT hr;

    entry = wine_rb_get(&reflection->types, &offset);
    if (entry)
    {
        TRACE("Returning existing type.\n");
        return WINE_RB_ENTRY_VALUE(entry, struct d3dcompiler_shader_reflection_type, entry);
    }

    type = calloc(1, sizeof(*type));
    if (!type)
        return NULL;

    type->ID3D11ShaderReflectionType_iface.lpVtbl = &d3dcompiler_shader_reflection_type_vtbl;
    type->id = offset;
    type->reflection = reflection;

    hr = d3dcompiler_parse_type(type, data, offset);
    if (FAILED(hr))
    {
        ERR("Failed to parse type info, hr %#lx.\n", hr);
        free(type);
        return NULL;
    }

    if (wine_rb_put(&reflection->types, &offset, &type->entry) == -1)
    {
        ERR("Failed to insert type entry.\n");
        free(type);
        return NULL;
    }

    return type;
}

static HRESULT d3dcompiler_parse_variables(struct d3dcompiler_shader_reflection_constant_buffer *cb,
        const char *data, size_t data_size, const char *ptr)
{
    struct d3dcompiler_shader_reflection_variable *variables;
    unsigned int i;
    HRESULT hr;

    variables = calloc(cb->variable_count, sizeof(*variables));
    if (!variables)
    {
        ERR("Failed to allocate variables memory.\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < cb->variable_count; i++)
    {
        struct d3dcompiler_shader_reflection_variable *v = &variables[i];
        uint32_t offset;

        v->ID3D11ShaderReflectionVariable_iface.lpVtbl = &d3dcompiler_shader_reflection_variable_vtbl;
        v->constant_buffer = cb;

        offset = read_u32(&ptr);
        if (!copy_name(data + offset, &v->name))
        {
            ERR("Failed to copy name.\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }
        TRACE("Variable name: %s.\n", debugstr_a(v->name));

        v->start_offset = read_u32(&ptr);
        TRACE("Variable offset: %u\n", v->start_offset);

        v->size = read_u32(&ptr);
        TRACE("Variable size: %u\n", v->size);

        v->flags = read_u32(&ptr);
        TRACE("Variable flags: %u\n", v->flags);

        offset = read_u32(&ptr);
        TRACE("Variable type offset: %x.\n", offset);
        v->type = get_reflection_type(cb->reflection, data, offset);
        if (!v->type)
        {
            ERR("Failed to get type.\n");
            hr = E_FAIL;
            goto err_out;
        }

        offset = read_u32(&ptr);
        TRACE("Variable default value offset: %x.\n", offset);
        if (!copy_value(data + offset, &v->default_value, offset ? v->size : 0))
        {
            ERR("Failed to copy name.\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        if ((cb->reflection->target & D3DCOMPILER_SHADER_TARGET_VERSION_MASK) >= 0x500)
            skip_u32_unknown(&ptr, 4);
    }

    cb->variables = variables;

    return S_OK;

err_out:
    for (i = 0; i < cb->variable_count; i++)
    {
        free_variable(&variables[i]);
    }
    free(variables);
    return hr;
}

static HRESULT d3dcompiler_parse_rdef(struct d3dcompiler_shader_reflection *r, const char *data, size_t data_size)
{
    struct d3dcompiler_shader_reflection_constant_buffer *constant_buffers = NULL;
    uint32_t offset, cbuffer_offset, resource_offset, creator_offset;
    unsigned int i, string_data_offset, string_data_size;
    D3D12_SHADER_INPUT_BIND_DESC *bound_resources = NULL;
    char *string_data = NULL, *creator = NULL;
    size_t size = data_size >> 2;
    uint32_t target_version;
    const char *ptr = data;
#if D3D_COMPILER_VERSION < 47
    uint32_t shader_type;
#endif
    HRESULT hr;

    TRACE("Size %Iu.\n", size);

    r->desc.ConstantBuffers = read_u32(&ptr);
    TRACE("Constant buffer count: %u.\n", r->desc.ConstantBuffers);

    cbuffer_offset = read_u32(&ptr);
    TRACE("Constant buffer offset: %#x.\n", cbuffer_offset);

    r->desc.BoundResources = read_u32(&ptr);
    TRACE("Bound resource count: %u.\n", r->desc.BoundResources);

    resource_offset = read_u32(&ptr);
    TRACE("Bound resource offset: %#x.\n", resource_offset);

    r->target = read_u32(&ptr);
    TRACE("Target: %#x.\n", r->target);

    target_version = r->target & D3DCOMPILER_SHADER_TARGET_VERSION_MASK;
#if D3D_COMPILER_VERSION < 47
    shader_type = (r->target & D3DCOMPILER_SHADER_TARGET_SHADERTYPE_MASK)
            >> D3DCOMPILER_SHADER_TARGET_SHADERTYPE_SHIFT;
    if ((target_version >= 0x501 && shader_type != D3DCOMPILER_SHDR_SHADER_TYPE_CS)
            || (!D3D_COMPILER_VERSION && shader_type == D3DCOMPILER_SHDR_SHADER_TYPE_CS))
    {
        WARN("Target version %#x is not supported in d3dcompiler %u.\n", target_version, D3D_COMPILER_VERSION);
        return E_INVALIDARG;
    }
#endif

    r->desc.Flags = read_u32(&ptr);
    TRACE("Flags: %#x.\n", r->desc.Flags);

    creator_offset = read_u32(&ptr);
    TRACE("Creator at offset %#x.\n", creator_offset);

    if (!copy_name(data + creator_offset, &creator))
    {
        ERR("Failed to copy name.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Creator: %s.\n", debugstr_a(creator));

    /* todo: Parse RD11 */
    if (target_version >= 0x500)
    {
        skip_u32_unknown(&ptr, 8);
    }

    if (r->desc.BoundResources)
    {
        /* 8 for each bind desc */
        string_data_offset = resource_offset + r->desc.BoundResources * 8 * sizeof(uint32_t);
        string_data_size = (cbuffer_offset ? cbuffer_offset : creator_offset) - string_data_offset;

        string_data = malloc(string_data_size);
        if (!string_data)
        {
            ERR("Failed to allocate string data memory.\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }
        memcpy(string_data, data + string_data_offset, string_data_size);

        bound_resources = calloc(r->desc.BoundResources, sizeof(*bound_resources));
        if (!bound_resources)
        {
            ERR("Failed to allocate resources memory.\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        ptr = data + resource_offset;
        for (i = 0; i < r->desc.BoundResources; i++)
        {
            D3D12_SHADER_INPUT_BIND_DESC *desc = &bound_resources[i];

            offset = read_u32(&ptr);
            desc->Name = string_data + (offset - string_data_offset);
            TRACE("Input bind Name: %s.\n", debugstr_a(desc->Name));

            desc->Type = read_u32(&ptr);
            TRACE("Input bind Type: %#x.\n", desc->Type);

            desc->ReturnType = read_u32(&ptr);
            TRACE("Input bind ReturnType: %#x.\n", desc->ReturnType);

            desc->Dimension = read_u32(&ptr);
            TRACE("Input bind Dimension: %#x.\n", desc->Dimension);

            desc->NumSamples = read_u32(&ptr);
            TRACE("Input bind NumSamples: %u.\n", desc->NumSamples);

            desc->BindPoint = read_u32(&ptr);
            TRACE("Input bind BindPoint: %u.\n", desc->BindPoint);

            desc->BindCount = read_u32(&ptr);
            TRACE("Input bind BindCount: %u.\n", desc->BindCount);

            desc->uFlags = read_u32(&ptr);
            TRACE("Input bind uFlags: %u.\n", desc->uFlags);

            if (target_version >= 0x501)
            {
                desc->Space = read_u32(&ptr);
                TRACE("Input bind Space %u.\n", desc->Space);
                desc->uID = read_u32(&ptr);
                TRACE("Input bind uID %u.\n", desc->uID);
            }
            else
            {
                desc->uID = desc->BindPoint;
            }
        }
    }

    if (r->desc.ConstantBuffers)
    {
        constant_buffers = calloc(r->desc.ConstantBuffers, sizeof(*constant_buffers));
        if (!constant_buffers)
        {
            ERR("Failed to allocate constant buffer memory.\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        ptr = data + cbuffer_offset;
        for (i = 0; i < r->desc.ConstantBuffers; i++)
        {
            struct d3dcompiler_shader_reflection_constant_buffer *cb = &constant_buffers[i];

            cb->ID3D11ShaderReflectionConstantBuffer_iface.lpVtbl = &d3dcompiler_shader_reflection_constant_buffer_vtbl;
            cb->reflection = r;

            offset = read_u32(&ptr);
            if (!copy_name(data + offset, &cb->name))
            {
                ERR("Failed to copy name.\n");
                hr = E_OUTOFMEMORY;
                goto err_out;
            }
            TRACE("Name: %s.\n", debugstr_a(cb->name));

            cb->variable_count = read_u32(&ptr);
            TRACE("Variable count: %u.\n", cb->variable_count);

            offset = read_u32(&ptr);
            TRACE("Variable offset: %x.\n", offset);

            hr = d3dcompiler_parse_variables(cb, data, data_size, data + offset);
            if (hr != S_OK)
            {
                FIXME("Failed to parse variables.\n");
                goto err_out;
            }

            cb->size = read_u32(&ptr);
            TRACE("Cbuffer size: %u.\n", cb->size);

            cb->flags = read_u32(&ptr);
            TRACE("Cbuffer flags: %u.\n", cb->flags);

            cb->type = read_u32(&ptr);
            TRACE("Cbuffer type: %#x.\n", cb->type);
        }
    }

    r->desc.Creator = creator;
    r->resource_string = string_data;
    r->bound_resources = bound_resources;
    r->constant_buffers = constant_buffers;

    return S_OK;

err_out:
    for (i = 0; i < r->desc.ConstantBuffers; ++i)
    {
        free_constant_buffer(&constant_buffers[i]);
    }
    free(constant_buffers);
    free(bound_resources);
    free(string_data);
    free(creator);

    return hr;
}

static HRESULT d3dcompiler_shader_reflection_init(struct d3dcompiler_shader_reflection *reflection,
        const void *data, SIZE_T data_size)
{
    struct vkd3d_shader_compile_info compile_info = {.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO};
    struct vkd3d_shader_dxbc_desc src_dxbc_desc;
    HRESULT hr = S_OK;
    unsigned int i;
    int ret;

    compile_info.next = &reflection->signature_info;
    compile_info.source.code = data;
    compile_info.source.size = data_size;
    compile_info.source_type = VKD3D_SHADER_SOURCE_DXBC_TPF;

    reflection->signature_info.type = VKD3D_SHADER_STRUCTURE_TYPE_SCAN_SIGNATURE_INFO;
    reflection->signature_info.next = &reflection->thread_group_size_info;

    reflection->thread_group_size_info.type = VKD3D_SHADER_STRUCTURE_TYPE_SCAN_THREAD_GROUP_SIZE_INFO;

    if ((ret = vkd3d_shader_scan(&compile_info, NULL)) < 0)
    {
        WARN("Failed to scan shader, ret %d.\n", ret);
        return E_FAIL;
    }

    wine_rb_init(&reflection->types, d3dcompiler_shader_reflection_type_compare);

    if ((ret = vkd3d_shader_parse_dxbc(&compile_info.source, 0, &src_dxbc_desc, NULL)) < 0)
    {
        WARN("Failed to parse reflection, ret %d.\n", ret);
        vkd3d_shader_free_scan_signature_info(&reflection->signature_info);
        return E_FAIL;
    }

    for (i = 0; i < src_dxbc_desc.section_count; ++i)
    {
        const struct vkd3d_shader_dxbc_section_desc *section = &src_dxbc_desc.sections[i];

        if (section->tag == TAG_RDEF)
        {
            if (FAILED(hr = d3dcompiler_parse_rdef(reflection, section->data.code, section->data.size)))
            {
                WARN("Failed to parse RDEF section.\n");
                goto err_out;
            }
        }
        else if (section->tag == TAG_SHDR || section->tag == TAG_SHEX)
        {
            const char *ptr = section->data.code;

            reflection->desc.Version = read_u32(&ptr);
            TRACE("Shader version: 0x%08x.\n", reflection->desc.Version);
        }
        else if (section->tag == TAG_STAT)
        {
            if (FAILED(hr = d3dcompiler_parse_stat(reflection, section->data.code, section->data.size)))
            {
                WARN("Failed to parse STAT section.\n");
                goto err_out;
            }
        }
    }

    reflection->desc.InputParameters = reflection->signature_info.input.element_count;
    reflection->desc.OutputParameters = reflection->signature_info.output.element_count;
#if D3D_COMPILER_VERSION
    reflection->desc.PatchConstantParameters = reflection->signature_info.patch_constant.element_count;
#endif

    vkd3d_shader_free_dxbc(&src_dxbc_desc);

    return hr;

err_out:
    reflection_cleanup(reflection);
    vkd3d_shader_free_dxbc(&src_dxbc_desc);

    return hr;
}

/* d3d10 reflection methods. */
#if !D3D_COMPILER_VERSION
HRESULT WINAPI D3D10ReflectShader(const void *data, SIZE_T data_size, ID3D10ShaderReflection **reflector)
{
    struct d3dcompiler_shader_reflection *object;
    HRESULT hr;

    TRACE("data %p, data_size %Iu, reflector %p.\n", data, data_size, reflector);

    if (!(object = calloc(1, sizeof(*object))))
    {
        ERR("Failed to allocate D3D10 shader reflection object memory.\n");
        return E_OUTOFMEMORY;
    }

    object->ID3D11ShaderReflection_iface.lpVtbl = &d3dcompiler_shader_reflection_vtbl;
    object->interface_version = D3DCOMPILER_REFLECTION_VERSION_D3D10;
    object->refcount = 1;

    hr = d3dcompiler_shader_reflection_init(object, data, data_size);
    if (FAILED(hr))
    {
        WARN("Failed to initialize shader reflection.\n");
        free(object);
        return hr;
    }

    *reflector = (ID3D10ShaderReflection *)&object->ID3D11ShaderReflection_iface;

    TRACE("Created ID3D10ShaderReflection %p.\n", object);

    return S_OK;
}
#else
HRESULT WINAPI D3DReflect(const void *data, SIZE_T data_size, REFIID riid, void **reflector)
{
    struct d3dcompiler_shader_reflection *object;
    const uint32_t *temp = data;
    HRESULT hr;

    TRACE("data %p, data_size %Iu, riid %s, blob %p.\n", data, data_size, debugstr_guid(riid), reflector);

    if (!data || data_size < 32)
    {
        WARN("Invalid argument supplied.\n");
        if (D3D_COMPILER_VERSION >= 43)
            return D3DERR_INVALIDCALL;
        if (D3D_COMPILER_VERSION)
            return E_INVALIDARG;
        return E_FAIL;
    }

    if (temp[6] != data_size)
    {
        WARN("Wrong size supplied.\n");
        if (D3D_COMPILER_VERSION >= 46)
            return D3DERR_INVALIDCALL;
        if (D3D_COMPILER_VERSION == 42)
            return E_INVALIDARG;
        return E_FAIL;
    }

    if (!IsEqualGUID(riid, &IID_ID3D11ShaderReflection)
            && (D3D_COMPILER_VERSION < 47 || !IsEqualGUID(riid, &IID_ID3D12ShaderReflection)))
    {
        WARN("Wrong riid %s, accept only %s!\n", debugstr_guid(riid), debugstr_guid(&IID_ID3D11ShaderReflection));
#if D3D_COMPILER_VERSION >= 46
        return E_INVALIDARG;
#else
        return E_NOINTERFACE;
#endif
    }

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3D11ShaderReflection_iface.lpVtbl = &d3dcompiler_shader_reflection_vtbl;
    object->refcount = 1;
    object->interface_version = IsEqualGUID(riid, &IID_ID3D12ShaderReflection)
            ? D3DCOMPILER_REFLECTION_VERSION_D3D12 : D3DCOMPILER_REFLECTION_VERSION_D3D11;

    hr = d3dcompiler_shader_reflection_init(object, data, data_size);
    if (FAILED(hr))
    {
        WARN("Failed to initialize shader reflection\n");
        free(object);
        return hr;
    }

    *reflector = object;
    TRACE("Created ID3D11ShaderReflection %p\n", object);

    return S_OK;
}
#endif
