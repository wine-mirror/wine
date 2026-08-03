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
 */

#include "vkd3d_utils_private.h"
#include <d3dcommon.h>
#include <d3d12shader.h>

struct d3d12_type
{
    ID3D12ShaderReflectionType ID3D12ShaderReflectionType_iface;
    D3D12_SHADER_TYPE_DESC desc;

    struct d3d12_field *fields;
};

struct d3d12_field
{
    struct d3d12_type type;
};

struct d3d12_variable
{
    ID3D12ShaderReflectionVariable ID3D12ShaderReflectionVariable_iface;
    D3D12_SHADER_VARIABLE_DESC desc;
    struct d3d12_buffer *buffer;

    struct d3d12_type type;
};

struct d3d12_buffer
{
    ID3D12ShaderReflectionConstantBuffer ID3D12ShaderReflectionConstantBuffer_iface;
    D3D12_SHADER_BUFFER_DESC desc;

    struct d3d12_variable *variables;
};

struct d3d12_reflection
{
    ID3D12ShaderReflection ID3D12ShaderReflection_iface;
    unsigned int refcount;

    struct vkd3d_shader_scan_signature_info signature_info;

    D3D12_SHADER_DESC desc;

    struct d3d12_buffer *buffers;

    D3D12_SHADER_INPUT_BIND_DESC *bindings;
};

static struct d3d12_buffer null_buffer;
static struct d3d12_variable null_variable;
static struct d3d12_type null_type;

static struct d3d12_type *impl_from_ID3D12ShaderReflectionType(ID3D12ShaderReflectionType *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_type, ID3D12ShaderReflectionType_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_type_GetDesc(
        ID3D12ShaderReflectionType *iface, D3D12_SHADER_TYPE_DESC *desc)
{
    struct d3d12_type *type = impl_from_ID3D12ShaderReflectionType(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (type == &null_type)
    {
        WARN("Null type, returning E_FAIL.\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("NULL pointer, returning E_FAIL.\n");
        return E_FAIL;
    }

    *desc = type->desc;
    return S_OK;
}

static ID3D12ShaderReflectionType * STDMETHODCALLTYPE d3d12_type_GetMemberTypeByIndex(
        ID3D12ShaderReflectionType *iface, UINT index)
{
    struct d3d12_type *type = impl_from_ID3D12ShaderReflectionType(iface);

    TRACE("iface %p, index %u.\n", iface, index);

    if (index >= type->desc.Members)
    {
        WARN("Invalid index %u.\n", index);
        return &null_type.ID3D12ShaderReflectionType_iface;
    }

    return &type->fields[index].type.ID3D12ShaderReflectionType_iface;
}

static ID3D12ShaderReflectionType * STDMETHODCALLTYPE d3d12_type_GetMemberTypeByName(
        ID3D12ShaderReflectionType *iface, const char *name)
{
    FIXME("iface %p, name %s, stub!\n", iface, debugstr_a(name));

    return NULL;
}

static const char * STDMETHODCALLTYPE d3d12_type_GetMemberTypeName(
        ID3D12ShaderReflectionType *iface, UINT index)
{
    FIXME("iface %p, index %u, stub!\n", iface, index);

    return NULL;
}

static HRESULT STDMETHODCALLTYPE d3d12_type_IsEqual(
        ID3D12ShaderReflectionType *iface, ID3D12ShaderReflectionType *other)
{
    FIXME("iface %p, other %p, stub!\n", iface, other);
    return E_NOTIMPL;
}

static ID3D12ShaderReflectionType * STDMETHODCALLTYPE d3d12_type_GetSubType(
        ID3D12ShaderReflectionType *iface)
{
    FIXME("iface %p stub!\n", iface);

    return NULL;
}

static ID3D12ShaderReflectionType * STDMETHODCALLTYPE d3d12_type_GetBaseClass(
        ID3D12ShaderReflectionType *iface)
{
    FIXME("iface %p stub!\n", iface);

    return NULL;
}

static UINT STDMETHODCALLTYPE d3d12_type_GetNumInterfaces(
        ID3D12ShaderReflectionType *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static ID3D12ShaderReflectionType * STDMETHODCALLTYPE d3d12_type_GetInterfaceByIndex(
        ID3D12ShaderReflectionType *iface, UINT index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return NULL;
}

static HRESULT STDMETHODCALLTYPE d3d12_type_IsOfType(
        ID3D12ShaderReflectionType *iface, ID3D12ShaderReflectionType *type)
{
    FIXME("iface %p, type %p stub!\n", iface, type);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_type_ImplementsInterface(
        ID3D12ShaderReflectionType *iface, ID3D12ShaderReflectionType *base)
{
    FIXME("iface %p, base %p stub!\n", iface, base);

    return E_NOTIMPL;
}

static const struct ID3D12ShaderReflectionTypeVtbl d3d12_type_vtbl =
{
    d3d12_type_GetDesc,
    d3d12_type_GetMemberTypeByIndex,
    d3d12_type_GetMemberTypeByName,
    d3d12_type_GetMemberTypeName,
    d3d12_type_IsEqual,
    d3d12_type_GetSubType,
    d3d12_type_GetBaseClass,
    d3d12_type_GetNumInterfaces,
    d3d12_type_GetInterfaceByIndex,
    d3d12_type_IsOfType,
    d3d12_type_ImplementsInterface,
};

static struct d3d12_type null_type = {{&d3d12_type_vtbl}};

static struct d3d12_variable *impl_from_ID3D12ShaderReflectionVariable(ID3D12ShaderReflectionVariable *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_variable, ID3D12ShaderReflectionVariable_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_variable_GetDesc(
        ID3D12ShaderReflectionVariable *iface, D3D12_SHADER_VARIABLE_DESC *desc)
{
    struct d3d12_variable *variable = impl_from_ID3D12ShaderReflectionVariable(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (variable == &null_variable)
    {
        WARN("Null variable, returning E_FAIL.\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("NULL pointer, returning E_FAIL.\n");
        return E_FAIL;
    }

    *desc = variable->desc;
    return S_OK;
}

static ID3D12ShaderReflectionType * STDMETHODCALLTYPE d3d12_variable_GetType(
        ID3D12ShaderReflectionVariable *iface)
{
    struct d3d12_variable *variable = impl_from_ID3D12ShaderReflectionVariable(iface);

    TRACE("iface %p.\n", iface);

    if (variable == &null_variable)
        return &null_type.ID3D12ShaderReflectionType_iface;

    return &variable->type.ID3D12ShaderReflectionType_iface;
}

static ID3D12ShaderReflectionConstantBuffer * STDMETHODCALLTYPE d3d12_variable_GetBuffer(
        ID3D12ShaderReflectionVariable *iface)
{
    struct d3d12_variable *variable = impl_from_ID3D12ShaderReflectionVariable(iface);

    TRACE("iface %p.\n", iface);

    return &variable->buffer->ID3D12ShaderReflectionConstantBuffer_iface;
}

static UINT STDMETHODCALLTYPE d3d12_variable_GetInterfaceSlot(
        ID3D12ShaderReflectionVariable *iface, UINT index)
{
    FIXME("iface %p, index %u, stub!\n", iface, index);

    return 0;
}

static const struct ID3D12ShaderReflectionVariableVtbl d3d12_variable_vtbl =
{
    d3d12_variable_GetDesc,
    d3d12_variable_GetType,
    d3d12_variable_GetBuffer,
    d3d12_variable_GetInterfaceSlot,
};

static struct d3d12_variable null_variable = {{&d3d12_variable_vtbl}};

static struct d3d12_buffer *impl_from_ID3D12ShaderReflectionConstantBuffer(ID3D12ShaderReflectionConstantBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_buffer, ID3D12ShaderReflectionConstantBuffer_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_buffer_GetDesc(
        ID3D12ShaderReflectionConstantBuffer *iface, D3D12_SHADER_BUFFER_DESC *desc)
{
    struct d3d12_buffer *buffer = impl_from_ID3D12ShaderReflectionConstantBuffer(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (buffer == &null_buffer)
    {
        WARN("Null constant buffer, returning E_FAIL.\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("NULL pointer, returning E_FAIL.\n");
        return E_FAIL;
    }

    *desc = buffer->desc;
    return S_OK;
}

static ID3D12ShaderReflectionVariable * STDMETHODCALLTYPE d3d12_buffer_GetVariableByIndex(
        ID3D12ShaderReflectionConstantBuffer *iface, UINT index)
{
    struct d3d12_buffer *buffer = impl_from_ID3D12ShaderReflectionConstantBuffer(iface);

    TRACE("iface %p, index %u.\n", iface, index);

    if (index >= buffer->desc.Variables)
    {
        WARN("Invalid index %u.\n", index);
        return &null_variable.ID3D12ShaderReflectionVariable_iface;
    }

    return &buffer->variables[index].ID3D12ShaderReflectionVariable_iface;
}

static ID3D12ShaderReflectionVariable * STDMETHODCALLTYPE d3d12_buffer_GetVariableByName(
        ID3D12ShaderReflectionConstantBuffer *iface, const char *name)
{
    FIXME("iface %p, name %s, stub!\n", iface, debugstr_a(name));

    return NULL;
}

static const struct ID3D12ShaderReflectionConstantBufferVtbl d3d12_buffer_vtbl =
{
    d3d12_buffer_GetDesc,
    d3d12_buffer_GetVariableByIndex,
    d3d12_buffer_GetVariableByName,
};

static struct d3d12_buffer null_buffer = {{&d3d12_buffer_vtbl}};

static struct d3d12_reflection *impl_from_ID3D12ShaderReflection(ID3D12ShaderReflection *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_reflection, ID3D12ShaderReflection_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_QueryInterface(
        ID3D12ShaderReflection *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D12ShaderReflection)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D12ShaderReflection_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_reflection_AddRef(ID3D12ShaderReflection *iface)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&reflection->refcount);

    TRACE("%p increasing refcount to %u.\n", reflection, refcount);

    return refcount;
}

static void free_type(struct d3d12_type *type)
{
    for (UINT i = 0; i < type->desc.Members; ++i)
        free_type(&type->fields[i].type);
    vkd3d_free(type->fields);
    vkd3d_free((void *)type->desc.Name);
}

static ULONG STDMETHODCALLTYPE d3d12_reflection_Release(ID3D12ShaderReflection *iface)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&reflection->refcount);

    TRACE("%p decreasing refcount to %u.\n", reflection, refcount);

    if (!refcount)
    {
        for (UINT i = 0; i < reflection->desc.ConstantBuffers; ++i)
        {
            struct d3d12_buffer *buffer = &reflection->buffers[i];

            for (UINT j = 0; j < buffer->desc.Variables; ++j)
            {
                struct d3d12_variable *variable = &buffer->variables[j];

                free_type(&variable->type);
                vkd3d_free((void *)variable->desc.DefaultValue);
                vkd3d_free((void *)variable->desc.Name);
            }
            vkd3d_free(buffer->variables);
            vkd3d_free((void *)buffer->desc.Name);
        }
        vkd3d_free(reflection->buffers);

        for (UINT i = 0; i < reflection->desc.BoundResources; ++i)
            vkd3d_free((void *)reflection->bindings[i].Name);
        vkd3d_free(reflection->bindings);

        vkd3d_shader_free_scan_signature_info(&reflection->signature_info);
        free(reflection);
    }

    return refcount;
}

/* ID3D12ShaderReflection methods */

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetDesc(ID3D12ShaderReflection *iface, D3D12_SHADER_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    FIXME("iface %p, desc %p partial stub!\n", iface, desc);

    *desc = reflection->desc;

    return S_OK;
}

static struct ID3D12ShaderReflectionConstantBuffer * STDMETHODCALLTYPE d3d12_reflection_GetConstantBufferByIndex(
        ID3D12ShaderReflection *iface, UINT index)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p, index %u.\n", iface, index);

    if (index >= reflection->desc.ConstantBuffers)
    {
        WARN("Invalid index %u.\n", index);
        return &null_buffer.ID3D12ShaderReflectionConstantBuffer_iface;
    }

    return &reflection->buffers[index].ID3D12ShaderReflectionConstantBuffer_iface;
}

static struct ID3D12ShaderReflectionConstantBuffer * STDMETHODCALLTYPE d3d12_reflection_GetConstantBufferByName(
        ID3D12ShaderReflection *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return NULL;
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetResourceBindingDesc(
        ID3D12ShaderReflection *iface, UINT index, D3D12_SHADER_INPUT_BIND_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    if (index >= reflection->desc.BoundResources)
    {
        WARN("Invalid index %u.\n", index);
        return E_INVALIDARG;
    }

    *desc = reflection->bindings[index];
    return S_OK;
}

static HRESULT get_signature_parameter(const struct vkd3d_shader_signature *signature,
        unsigned int index, D3D12_SIGNATURE_PARAMETER_DESC *desc, bool output)
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
    desc->Stream = e->stream_index;
    desc->MinPrecision = (D3D_MIN_PRECISION)e->min_precision;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetInputParameterDesc(
        ID3D12ShaderReflection *iface, UINT index, D3D12_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    return get_signature_parameter(&reflection->signature_info.input, index, desc, false);
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetOutputParameterDesc(
        ID3D12ShaderReflection *iface, UINT index, D3D12_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    return get_signature_parameter(&reflection->signature_info.output, index, desc, true);
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetPatchConstantParameterDesc(
        ID3D12ShaderReflection *iface, UINT index, D3D12_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);
    bool output = ((reflection->desc.Version & 0xffff0000) >> 16) == D3D12_SHVER_HULL_SHADER;

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    return get_signature_parameter(&reflection->signature_info.patch_constant, index, desc, output);
}

static struct ID3D12ShaderReflectionVariable * STDMETHODCALLTYPE d3d12_reflection_GetVariableByName(
        ID3D12ShaderReflection *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return NULL;
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetResourceBindingDescByName(
        ID3D12ShaderReflection *iface, const char *name, D3D12_SHADER_INPUT_BIND_DESC *desc)
{
    FIXME("iface %p, name %s, desc %p stub!\n", iface, debugstr_a(name), desc);

    return E_NOTIMPL;
}

static UINT STDMETHODCALLTYPE d3d12_reflection_GetMovInstructionCount(ID3D12ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static UINT STDMETHODCALLTYPE d3d12_reflection_GetMovcInstructionCount(ID3D12ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static UINT STDMETHODCALLTYPE d3d12_reflection_GetConversionInstructionCount(ID3D12ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static UINT STDMETHODCALLTYPE d3d12_reflection_GetBitwiseInstructionCount(ID3D12ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static D3D_PRIMITIVE STDMETHODCALLTYPE d3d12_reflection_GetGSInputPrimitive(ID3D12ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static BOOL STDMETHODCALLTYPE d3d12_reflection_IsSampleFrequencyShader(ID3D12ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static UINT STDMETHODCALLTYPE d3d12_reflection_GetNumInterfaceSlots(ID3D12ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetMinFeatureLevel(
        ID3D12ShaderReflection *iface, D3D_FEATURE_LEVEL *level)
{
    FIXME("iface %p, level %p stub!\n", iface, level);

    return E_NOTIMPL;
}

static UINT STDMETHODCALLTYPE d3d12_reflection_GetThreadGroupSize(
        ID3D12ShaderReflection *iface, UINT *sizex, UINT *sizey, UINT *sizez)
{
    FIXME("iface %p, sizex %p, sizey %p, sizez %p stub!\n", iface, sizex, sizey, sizez);

    return 0;
}

static UINT64 STDMETHODCALLTYPE d3d12_reflection_GetRequiresFlags(ID3D12ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static const struct ID3D12ShaderReflectionVtbl d3d12_reflection_vtbl =
{
    /* IUnknown methods */
    d3d12_reflection_QueryInterface,
    d3d12_reflection_AddRef,
    d3d12_reflection_Release,
    /* ID3D12ShaderReflection methods */
    d3d12_reflection_GetDesc,
    d3d12_reflection_GetConstantBufferByIndex,
    d3d12_reflection_GetConstantBufferByName,
    d3d12_reflection_GetResourceBindingDesc,
    d3d12_reflection_GetInputParameterDesc,
    d3d12_reflection_GetOutputParameterDesc,
    d3d12_reflection_GetPatchConstantParameterDesc,
    d3d12_reflection_GetVariableByName,
    d3d12_reflection_GetResourceBindingDescByName,
    d3d12_reflection_GetMovInstructionCount,
    d3d12_reflection_GetMovcInstructionCount,
    d3d12_reflection_GetConversionInstructionCount,
    d3d12_reflection_GetBitwiseInstructionCount,
    d3d12_reflection_GetGSInputPrimitive,
    d3d12_reflection_IsSampleFrequencyShader,
    d3d12_reflection_GetNumInterfaceSlots,
    d3d12_reflection_GetMinFeatureLevel,
    d3d12_reflection_GetThreadGroupSize,
    d3d12_reflection_GetRequiresFlags,
};

static bool require_space(size_t offset, size_t count, size_t size, size_t data_size)
{
    return !count || (data_size - offset) / count >= size;
}

/* Return a pointer to data in a code blob, with bounds checking. */
static const void *get_data_ptr(const struct vkd3d_shader_code *code,
        uint32_t offset, uint32_t count, uint32_t size)
{
    if (!require_space(offset, count, size, code->size))
    {
        WARN("Offset %#x and size %#x exceeds section size %#zx.\n", offset, size, code->size);
        return NULL;
    }

    return (const uint8_t *)code->code + offset;
}

static HRESULT get_string(const struct vkd3d_shader_code *code, uint32_t offset, char **ret)
{
    const char *str;
    char *end;

    if (offset >= code->size)
    {
        WARN("Offset %#x exceeds size %#zx.\n", offset, code->size);
        return E_INVALIDARG;
    }

    str = (const char *)code->code + offset;
    if (!(end = memchr(str, 0, code->size - offset)))
    {
        WARN("String at %#x is not properly zero-terminated.\n", offset);
        return E_INVALIDARG;
    }

    if (!(*ret = vkd3d_memdup(str, end + 1 - str)))
        return E_OUTOFMEMORY;
    return S_OK;
}

struct rdef_header
{
    uint32_t buffer_count;
    uint32_t buffers_offset;
    uint32_t binding_count;
    uint32_t bindings_offset;
    uint8_t minor_version;
    uint8_t major_version;
    uint16_t type;
    uint32_t compile_flags;
    uint32_t creator_offset;
};

struct rdef_rd11
{
    uint32_t magic;
    uint32_t header_size;
    uint32_t buffer_size;
    uint32_t binding_size;
    uint32_t variable_size;
    uint32_t type_size;
    uint32_t field_size;
    /* Always zero. Possibly either padding or a null terminator? */
    uint32_t zero;
};

struct rdef_buffer
{
    uint32_t name_offset;
    uint32_t var_count;
    uint32_t vars_offset;
    uint32_t size;
    uint32_t flags;
    uint32_t type;
};

struct rdef_variable
{
    uint32_t name_offset;
    uint32_t offset;
    uint32_t size;
    uint32_t flags;
    uint32_t type_offset;
    uint32_t default_value_offset;
    uint32_t resource_binding;
    uint32_t resource_count;
    uint32_t sampler_binding;
    uint32_t sampler_count;
};

struct rdef_type
{
    uint16_t class;
    uint16_t base_type;
    uint16_t row_count;
    uint16_t column_count;
    uint16_t element_count;
    uint16_t field_count;
    uint32_t fields_offset;
    /* Probably related to interfaces. */
    uint32_t unknown[4];
    uint32_t name_offset;
};

struct rdef_field
{
    uint32_t name_offset;
    uint32_t type_offset;
    uint32_t offset;
};

struct rdef_binding
{
    uint32_t name_offset;
    uint32_t type;
    uint32_t resource_format;
    uint32_t dimension;
    uint32_t multisample_count;
    uint32_t index;
    uint32_t count;
    uint32_t flags;
    uint32_t space;
    uint32_t id;
};

static HRESULT d3d12_type_init(struct d3d12_type *type, uint32_t type_offset, uint32_t type_size,
        const struct vkd3d_shader_code *section, uint32_t field_offset)
{
    struct rdef_type normalized_type = {0};
    const struct rdef_type *rdef_type;
    char *name = NULL;
    HRESULT hr;

    if (!(rdef_type = get_data_ptr(section, type_offset, 1, type_size)))
        return E_INVALIDARG;
    memcpy(&normalized_type, rdef_type, type_size);

    if (normalized_type.name_offset && FAILED(hr = get_string(section, normalized_type.name_offset, &name)))
        return hr;

    type->ID3D12ShaderReflectionType_iface.lpVtbl = &d3d12_type_vtbl;

    type->desc.Class = normalized_type.class;
    type->desc.Type = normalized_type.base_type;
    type->desc.Rows = normalized_type.row_count;
    type->desc.Columns = normalized_type.column_count;
    type->desc.Elements = normalized_type.element_count;
    type->desc.Members = normalized_type.field_count;
    type->desc.Offset = field_offset;
    type->desc.Name = name;

    if (normalized_type.field_count)
    {
        const struct rdef_field *rdef_fields;

        if (!(rdef_fields = get_data_ptr(section, normalized_type.fields_offset,
                normalized_type.field_count, sizeof(*rdef_fields))))
            return E_INVALIDARG;

        if (!(type->fields = vkd3d_calloc(normalized_type.field_count, sizeof(*type->fields))))
            return false;

        for (uint32_t i = 0; i < normalized_type.field_count; ++i)
        {
            const struct rdef_field *rdef_field = &rdef_fields[i];

            if ((hr = d3d12_type_init(&type->fields[i].type, rdef_field->type_offset,
                    type_size, section, rdef_field->offset)))
                return hr;
        }
    }

    return S_OK;
}

static HRESULT d3d12_variable_init(struct d3d12_variable *variable, const struct rdef_variable *rdef_variable,
        const struct vkd3d_shader_code *section, uint32_t type_size)
{
    HRESULT hr;
    char *name;

    if (FAILED(hr = get_string(section, rdef_variable->name_offset, &name)))
        return hr;

    variable->ID3D12ShaderReflectionVariable_iface.lpVtbl = &d3d12_variable_vtbl;

    variable->desc.Name = name;
    variable->desc.StartOffset = rdef_variable->offset;
    variable->desc.Size = rdef_variable->size;
    variable->desc.uFlags = rdef_variable->flags;
    variable->desc.StartTexture = rdef_variable->resource_binding;
    variable->desc.TextureSize = rdef_variable->resource_count;
    variable->desc.StartSampler = rdef_variable->sampler_binding;
    variable->desc.SamplerSize = rdef_variable->sampler_count;

    if (rdef_variable->default_value_offset)
    {
        const void *default_value;

        if (!(default_value = get_data_ptr(section, rdef_variable->default_value_offset, 1, rdef_variable->size)))
            return E_INVALIDARG;

        if (!(variable->desc.DefaultValue = vkd3d_memdup(default_value, rdef_variable->size)))
            return E_OUTOFMEMORY;
    }

    return d3d12_type_init(&variable->type, rdef_variable->type_offset, type_size, section, 0);
}

static HRESULT d3d12_buffer_init(struct d3d12_buffer *buffer, const struct rdef_buffer *rdef_buffer,
        const struct vkd3d_shader_code *section, uint32_t variable_size, uint32_t type_size)
{
    HRESULT hr;
    char *name;

    if ((FAILED(hr = get_string(section, rdef_buffer->name_offset, &name))))
        return hr;

    buffer->ID3D12ShaderReflectionConstantBuffer_iface.lpVtbl = &d3d12_buffer_vtbl;

    buffer->desc.Type = rdef_buffer->type;
    buffer->desc.Variables = rdef_buffer->var_count;
    buffer->desc.Size = rdef_buffer->size;
    buffer->desc.uFlags = rdef_buffer->flags;
    buffer->desc.Name = name;

    if (!(buffer->variables = vkd3d_calloc(rdef_buffer->var_count, sizeof(*buffer->variables))))
        return E_OUTOFMEMORY;

    for (uint32_t i = 0; i < rdef_buffer->var_count; ++i)
    {
        struct rdef_variable normalized_variable = {0};
        const struct rdef_variable *rdef_variable;

        if (!(rdef_variable = get_data_ptr(section, rdef_buffer->vars_offset + (i * variable_size), 1, variable_size)))
            return E_INVALIDARG;

        normalized_variable.resource_binding = ~0u;
        normalized_variable.sampler_binding = ~0u;
        memcpy(&normalized_variable, rdef_variable, variable_size);

        if ((hr = d3d12_variable_init(&buffer->variables[i], &normalized_variable, section, type_size)))
            return hr;
    }

    return S_OK;
}

static HRESULT parse_rdef(struct d3d12_reflection *reflection, const struct vkd3d_shader_code *section)
{
    uint32_t variable_size = offsetof(struct rdef_variable, resource_binding);
    uint32_t binding_size = offsetof(struct rdef_binding, space);
    uint32_t type_size = offsetof(struct rdef_type, unknown);
    const struct rdef_header *header;
    const struct rdef_rd11 *rd11;
    HRESULT hr;

    if (!(header = get_data_ptr(section, 0, 1, sizeof(*header))))
        return E_INVALIDARG;

    if (header->major_version >= 5)
    {
        if (!(rd11 = get_data_ptr(section, sizeof(*header), 1, sizeof(*rd11))))
            return E_INVALIDARG;

        /* RD11 is emitted for 5.0, the reversed version for 5.1 and 6.0.
         * This corresponds to a difference in the binding_size member, but
         * it's not clear why the magic also changed there. */
        if (rd11->magic != TAG_RD11 && rd11->magic != TAG_RD11_REVERSE)
        {
            FIXME("Unknown tag %#x.\n", rd11->magic);
            return E_INVALIDARG;
        }

        if (rd11->header_size != sizeof(struct rdef_header) + sizeof(struct rdef_rd11))
        {
            FIXME("Unexpected header size %#x.\n", rd11->header_size);
            return E_INVALIDARG;
        }

        if (rd11->buffer_size != sizeof(struct rdef_buffer))
        {
            FIXME("Unexpected buffer size %#x.\n", rd11->buffer_size);
            return E_INVALIDARG;
        }

        if (rd11->variable_size != sizeof(struct rdef_variable))
        {
            FIXME("Unexpected variable size %#x.\n", rd11->variable_size);
            return E_INVALIDARG;
        }
        variable_size = rd11->variable_size;

        if (rd11->binding_size != sizeof(struct rdef_binding)
                && rd11->binding_size != offsetof(struct rdef_binding, space))
        {
            FIXME("Unexpected binding size %#x.\n", rd11->binding_size);
            return E_INVALIDARG;
        }
        binding_size = rd11->binding_size;

        if (rd11->type_size != sizeof(struct rdef_type))
        {
            FIXME("Unexpected type size %#x.\n", rd11->type_size);
            return E_INVALIDARG;
        }
        type_size = rd11->type_size;

        if (rd11->zero)
        {
            FIXME("Unexpected field %#x.\n", rd11->zero);
            return E_INVALIDARG;
        }
    }

    reflection->desc.ConstantBuffers = header->buffer_count;

    if (header->buffer_count)
    {
        const struct rdef_buffer *rdef_buffers;

        if (!(rdef_buffers = get_data_ptr(section, header->buffers_offset,
                header->buffer_count, sizeof(*rdef_buffers))))
            return E_INVALIDARG;

        if (!(reflection->buffers = vkd3d_calloc(header->buffer_count, sizeof(*reflection->buffers))))
            return E_OUTOFMEMORY;

        for (uint32_t i = 0; i < header->buffer_count; ++i)
        {
            if ((hr = d3d12_buffer_init(&reflection->buffers[i], &rdef_buffers[i], section, variable_size, type_size)))
                return hr;
        }
    }

    reflection->desc.BoundResources = header->binding_count;

    if (header->binding_count)
    {
        if (!(reflection->bindings = vkd3d_calloc(header->binding_count, sizeof(*reflection->bindings))))
            return E_OUTOFMEMORY;

        for (uint32_t i = 0; i < header->binding_count; ++i)
        {
            const struct rdef_binding *rdef_binding;
            D3D12_SHADER_INPUT_BIND_DESC *binding;
            char *name;

            if (!(rdef_binding = get_data_ptr(section, header->bindings_offset + (i * binding_size), 1, binding_size)))
                return E_INVALIDARG;

            if (FAILED(hr = get_string(section, rdef_binding->name_offset, &name)))
                return hr;

            binding = &reflection->bindings[i];

            binding->Name = name;
            binding->Type = rdef_binding->type;
            binding->BindPoint = rdef_binding->index;
            binding->BindCount = rdef_binding->count;
            binding->uFlags = rdef_binding->flags;
            binding->ReturnType = rdef_binding->resource_format;
            binding->Dimension = rdef_binding->dimension;
            binding->NumSamples = rdef_binding->multisample_count;
            if (binding_size == sizeof(*rdef_binding))
            {
                binding->Space = rdef_binding->space;
                binding->uID = rdef_binding->id;
            }
            else
            {
                binding->Space = 0;
                binding->uID = rdef_binding->index;
            }
        }
    }

    return S_OK;
}

static HRESULT d3d12_reflection_init(struct d3d12_reflection *reflection, const void *data, size_t data_size)
{
    struct vkd3d_shader_compile_info compile_info = {.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO};
    struct vkd3d_shader_dxbc_desc dxbc_desc;
    bool found_rdef = false;
    enum vkd3d_result ret;
    HRESULT hr;

    reflection->ID3D12ShaderReflection_iface.lpVtbl = &d3d12_reflection_vtbl;
    reflection->refcount = 1;

    compile_info.source.code = data;
    compile_info.source.size = data_size;
    compile_info.source_type = VKD3D_SHADER_SOURCE_DXBC_TPF;

    compile_info.next = &reflection->signature_info;
    reflection->signature_info.type = VKD3D_SHADER_STRUCTURE_TYPE_SCAN_SIGNATURE_INFO;

    if (FAILED(hr = hresult_from_vkd3d_result(vkd3d_shader_scan(&compile_info, NULL))))
        return hr;

    if ((ret = vkd3d_shader_parse_dxbc(&compile_info.source, 0, &dxbc_desc, NULL)))
    {
        vkd3d_shader_free_scan_signature_info(&reflection->signature_info);
        return hresult_from_vkd3d_result(ret);
    }

    for (unsigned int i = 0; i < dxbc_desc.section_count; ++i)
    {
        const struct vkd3d_shader_dxbc_section_desc *section = &dxbc_desc.sections[i];

        if (section->tag == TAG_RDEF)
        {
            if (found_rdef)
            {
                FIXME("Multiple RDEF chunks.\n");
                continue;
            }

            if (FAILED(hr = parse_rdef(reflection, &section->data)))
                goto fail;
            found_rdef = true;
        }
        else if (section->tag == TAG_SHDR || section->tag == TAG_SHEX)
        {
            const uint32_t *version;

            if (!(version = get_data_ptr(&section->data, 0, 1, sizeof(*version))))
            {
                hr = E_INVALIDARG;
                goto fail;
            }
            reflection->desc.Version = *version;
        }
    }

    reflection->desc.InputParameters = reflection->signature_info.input.element_count;
    reflection->desc.OutputParameters = reflection->signature_info.output.element_count;
    reflection->desc.PatchConstantParameters = reflection->signature_info.patch_constant.element_count;

    vkd3d_shader_free_dxbc(&dxbc_desc);

    return S_OK;

fail:
    vkd3d_shader_free_scan_signature_info(&reflection->signature_info);
    vkd3d_shader_free_dxbc(&dxbc_desc);
    return hr;
}

HRESULT WINAPI D3DReflect(const void *data, SIZE_T data_size, REFIID iid, void **reflection)
{
    struct d3d12_reflection *object;
    HRESULT hr;

    TRACE("data %p, data_size %"PRIuPTR", iid %s, reflection %p.\n",
            data, (uintptr_t)data_size, debugstr_guid(iid), reflection);

    if (!IsEqualGUID(iid, &IID_ID3D12ShaderReflection))
    {
        WARN("Invalid iid %s.\n", debugstr_guid(iid));
        return E_INVALIDARG;
    }

    if (!(object = vkd3d_calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d12_reflection_init(object, data, data_size)))
    {
        free(object);
        return hr;
    }

    *reflection = &object->ID3D12ShaderReflection_iface;
    TRACE("Created reflection %p.\n", object);
    return S_OK;
}
