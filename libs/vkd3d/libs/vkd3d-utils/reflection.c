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
#include <d3d10.h>
#include <d3d10_1shader.h>
#include <d3d11shader.h>
#include <d3d12shader.h>

#define RDEF_SHADER_TYPE_CS 0x4353u /* "CS" */

struct d3d12_type
{
    ID3D12ShaderReflectionType ID3D12ShaderReflectionType_iface;
    uint32_t id;
    D3D12_SHADER_TYPE_DESC desc;
    size_t desc_size;

    struct d3d12_field *fields;
};

struct d3d12_field
{
    char *name;
    struct d3d12_type type;
};

struct d3d12_variable
{
    ID3D12ShaderReflectionVariable ID3D12ShaderReflectionVariable_iface;
    D3D12_SHADER_VARIABLE_DESC desc;
    size_t desc_size;
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
    ID3D10ShaderReflection1 ID3D10ShaderReflection1_iface;
    unsigned int refcount;
    unsigned int version;

    struct vkd3d_shader_scan_signature_info signature_info;
    struct vkd3d_shader_scan_thread_group_size_info thread_group_size_info;

    D3D12_SHADER_DESC desc;
    size_t desc_size, bind_desc_size;

    uint32_t mov_count;
    uint32_t movc_count;
    uint32_t type_conversion_count;
    uint32_t bitwise_count;
    uint32_t sample_frequency;
    uint32_t interface_slot_count;

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

    memcpy(desc, &type->desc, type->desc_size);

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
    struct d3d12_type *type = impl_from_ID3D12ShaderReflectionType(iface);
    unsigned int i;

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("NULL name specified.\n");
        return &null_type.ID3D12ShaderReflectionType_iface;
    }

    for (i = 0; i < type->desc.Members; ++i)
    {
        struct d3d12_field *field = &type->fields[i];

        if (!strcmp(field->name, name))
        {
            TRACE("Returning member type %p at index %u.\n", &field->type, i);
            return &field->type.ID3D12ShaderReflectionType_iface;
        }
    }

    WARN("Member type %s not found.\n", debugstr_a(name));

    return &null_type.ID3D12ShaderReflectionType_iface;
}

static const char * STDMETHODCALLTYPE d3d12_type_GetMemberTypeName(
        ID3D12ShaderReflectionType *iface, UINT index)
{
    struct d3d12_type *type = impl_from_ID3D12ShaderReflectionType(iface);

    TRACE("iface %p, index %u.\n", iface, index);

    if (type == &null_type)
    {
        WARN("Null type, returning \"$Invalid\".\n");
        return "$Invalid";
    }

    if (index >= type->desc.Members)
    {
        WARN("Invalid index %u.\n", index);
        return NULL;
    }

    return type->fields[index].name;
}

static HRESULT STDMETHODCALLTYPE d3d12_type_IsEqual(
        ID3D12ShaderReflectionType *iface, ID3D12ShaderReflectionType *other)
{
    struct d3d12_type *type1 = impl_from_ID3D12ShaderReflectionType(iface);
    struct d3d12_type *type2 = impl_from_ID3D12ShaderReflectionType(other);

    TRACE("iface %p, other %p.\n", iface, other);

    if (type1 == &null_type)
    {
        WARN("Null type, returning E_FAIL.\n");
        return E_FAIL;
    }

    return type1->id == type2->id ? S_OK : S_FALSE;
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

    memcpy(desc, &variable->desc, variable->desc_size);

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

    if (variable == &null_variable)
        return &null_buffer.ID3D12ShaderReflectionConstantBuffer_iface;

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
    struct d3d12_buffer *buffer = impl_from_ID3D12ShaderReflectionConstantBuffer(iface);
    unsigned int i;

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("NULL name specified.\n");
        return &null_variable.ID3D12ShaderReflectionVariable_iface;
    }

    for (i = 0; i < buffer->desc.Variables; ++i)
    {
        struct d3d12_variable *variable = &buffer->variables[i];

        if (!strcmp(variable->desc.Name, name))
        {
            TRACE("Returning variable %p at index %u.\n", variable, i);
            return &variable->ID3D12ShaderReflectionVariable_iface;
        }
    }

    WARN("Variable %s not found.\n", debugstr_a(name));

    return &null_variable.ID3D12ShaderReflectionVariable_iface;
}

static const struct ID3D12ShaderReflectionConstantBufferVtbl d3d12_buffer_vtbl =
{
    d3d12_buffer_GetDesc,
    d3d12_buffer_GetVariableByIndex,
    d3d12_buffer_GetVariableByName,
};

static struct d3d12_buffer null_buffer = {{&d3d12_buffer_vtbl}};

static bool reflection_iid_supported(const IID *iid, unsigned int version)
{
    if (version < 33)
        return IsEqualGUID(iid, &IID_ID3D10ShaderReflection);
    if (version < 40)
        return IsEqualGUID(iid, &IID_ID3D10ShaderReflection)
                || IsEqualGUID(iid, &IID_ID3D10ShaderReflection1);
    if (version < 43)
        return IsEqualGUID(iid, &IID_ID3D11ShaderReflection_v40);
    if (version < 46)
        return IsEqualGUID(iid, &IID_ID3D11ShaderReflection_v43);
    if (version < 47)
        return IsEqualGUID(iid, &IID_ID3D11ShaderReflection_v46);
    return IsEqualGUID(iid, &IID_ID3D11ShaderReflection_v46)
            || IsEqualGUID(iid, &IID_ID3D12ShaderReflection);
}

static struct d3d12_reflection *impl_from_ID3D12ShaderReflection(ID3D12ShaderReflection *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_reflection, ID3D12ShaderReflection_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_QueryInterface(
        ID3D12ShaderReflection *iface, REFIID iid, void **out)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (reflection_iid_supported(iid, reflection->version) || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID3D12ShaderReflection_AddRef(iface);
        if (IsEqualGUID(iid, &IID_ID3D10ShaderReflection1))
            *out = &reflection->ID3D10ShaderReflection1_iface;
        else
            *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_reflection_AddRef(ID3D12ShaderReflection *iface)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&reflection->refcount);

    TRACE("%p increasing refcount to %u.\n", reflection, refcount);

    return refcount;
}

static void d3d12_reflection_free_bindings(struct d3d12_reflection *reflection)
{
    unsigned int i;

    for (i = 0; i < reflection->desc.BoundResources; ++i)
    {
        vkd3d_free((void *)reflection->bindings[i].Name);
    }
    vkd3d_free(reflection->bindings);
}

static void d3d12_type_cleanup(struct d3d12_type *type)
{
    unsigned int i;

    for (i = 0; i < type->desc.Members; ++i)
    {
        d3d12_type_cleanup(&type->fields[i].type);
        vkd3d_free(type->fields[i].name);
    }
    vkd3d_free(type->fields);
    vkd3d_free((void *)type->desc.Name);
}

static void d3d12_variable_cleanup(struct d3d12_variable *variable)
{
    d3d12_type_cleanup(&variable->type);
    vkd3d_free(variable->desc.DefaultValue);
    vkd3d_free((void *)variable->desc.Name);
}

static void d3d12_buffer_cleanup(struct d3d12_buffer *buffer)
{
    unsigned int i;

    for (i = 0; i < buffer->desc.Variables; ++i)
    {
        d3d12_variable_cleanup(&buffer->variables[i]);
    }
    vkd3d_free(buffer->variables);
    vkd3d_free((void *)buffer->desc.Name);
}

static void d3d12_reflection_free_buffers(struct d3d12_reflection *reflection)
{
    unsigned int i;

    for (i = 0; i < reflection->desc.ConstantBuffers; ++i)
    {
        d3d12_buffer_cleanup(&reflection->buffers[i]);
    }
    vkd3d_free(reflection->buffers);
}

static void d3d12_reflection_cleanup(struct d3d12_reflection *reflection)
{
    vkd3d_free((void *)reflection->desc.Creator);
    d3d12_reflection_free_bindings(reflection);
    d3d12_reflection_free_buffers(reflection);
    vkd3d_shader_free_scan_signature_info(&reflection->signature_info);
}

static void d3d12_reflection_destroy(struct d3d12_reflection *reflection)
{
    d3d12_reflection_cleanup(reflection);
    vkd3d_free(reflection);
}

static ULONG STDMETHODCALLTYPE d3d12_reflection_Release(ID3D12ShaderReflection *iface)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&reflection->refcount);

    TRACE("%p decreasing refcount to %u.\n", reflection, refcount);

    if (!refcount)
        d3d12_reflection_destroy(reflection);

    return refcount;
}

/* ID3D12ShaderReflection methods */

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetDesc(ID3D12ShaderReflection *iface, D3D12_SHADER_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("NULL desc specified.\n");
        return E_FAIL;
    }

    memcpy(desc, &reflection->desc, reflection->desc_size);

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
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);
    unsigned int i;

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("NULL name specified.\n");
        return &null_buffer.ID3D12ShaderReflectionConstantBuffer_iface;
    }

    for (i = 0; i < reflection->desc.ConstantBuffers; ++i)
    {
        struct d3d12_buffer *buffer = &reflection->buffers[i];

        if (!strcmp(buffer->desc.Name, name))
        {
            TRACE("Returning buffer %p at index %u.\n", buffer, i);
            return &buffer->ID3D12ShaderReflectionConstantBuffer_iface;
        }
    }

    WARN("Constant buffer %s not found.\n", debugstr_a(name));

    return &null_buffer.ID3D12ShaderReflectionConstantBuffer_iface;
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetResourceBindingDesc(
        ID3D12ShaderReflection *iface, UINT index, D3D12_SHADER_INPUT_BIND_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    if (!desc)
    {
        WARN("NULL desc specified.\n");
        return E_INVALIDARG;
    }

    if (index >= reflection->desc.BoundResources)
    {
        WARN("Invalid index %u.\n", index);
        return E_INVALIDARG;
    }

    memcpy(desc, &reflection->bindings[index], reflection->bind_desc_size);

    return S_OK;
}

static HRESULT get_signature_parameter(const struct vkd3d_shader_signature *signature,
        unsigned int index, D3D12_SIGNATURE_PARAMETER_DESC *desc, bool output, unsigned int version)
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
    if (version < 35 && e->sysval_semantic >= VKD3D_SHADER_SV_TARGET)
        desc->SystemValueType = D3D_NAME_UNDEFINED;
    else
        desc->SystemValueType = (D3D_NAME)e->sysval_semantic;
    desc->ComponentType = (D3D_REGISTER_COMPONENT_TYPE)e->component_type;
    desc->Mask = e->mask;
    desc->ReadWriteMask = output ? (0xf ^ e->used_mask) : e->used_mask;
    if (version >= 40)
        desc->Stream = e->stream_index;
    if (version >= 46)
        desc->MinPrecision = (D3D_MIN_PRECISION)e->min_precision;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetInputParameterDesc(
        ID3D12ShaderReflection *iface, UINT index, D3D12_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    return get_signature_parameter(&reflection->signature_info.input, index, desc, false, reflection->version);
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetOutputParameterDesc(
        ID3D12ShaderReflection *iface, UINT index, D3D12_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    return get_signature_parameter(&reflection->signature_info.output, index, desc, true, reflection->version);
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetPatchConstantParameterDesc(
        ID3D12ShaderReflection *iface, UINT index, D3D12_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);
    bool output = ((reflection->desc.Version & 0xffff0000) >> 16) == D3D12_SHVER_HULL_SHADER;

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    return get_signature_parameter(&reflection->signature_info.patch_constant,
            index, desc, output, reflection->version);
}

static struct ID3D12ShaderReflectionVariable * STDMETHODCALLTYPE d3d12_reflection_GetVariableByName(
        ID3D12ShaderReflection *iface, const char *name)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);
    unsigned int i, j;

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("NULL name specified.\n");
        return &null_variable.ID3D12ShaderReflectionVariable_iface;
    }

    for (i = 0; i < reflection->desc.ConstantBuffers; ++i)
    {
        struct d3d12_buffer *buffer = &reflection->buffers[i];

        for (j = 0; j < buffer->desc.Variables; ++j)
        {
            struct d3d12_variable *variable = &buffer->variables[j];

            if (!strcmp(variable->desc.Name, name))
            {
                TRACE("Returning variable %p at index %u of buffer %u.\n", variable, j, i);
                return &variable->ID3D12ShaderReflectionVariable_iface;
            }
        }
    }

    WARN("Variable %s not found.\n", debugstr_a(name));

    return &null_variable.ID3D12ShaderReflectionVariable_iface;
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetResourceBindingDescByName(
        ID3D12ShaderReflection *iface, const char *name, D3D12_SHADER_INPUT_BIND_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);
    unsigned int i;

    TRACE("iface %p, name %s, desc %p.\n", iface, debugstr_a(name), desc);

    if (!desc)
    {
        WARN("NULL desc specified.\n");
        return E_INVALIDARG;
    }

    if (!name)
    {
        WARN("NULL name specified.\n");
        return E_INVALIDARG;
    }

    for (i = 0; i < reflection->desc.BoundResources; ++i)
    {
        D3D12_SHADER_INPUT_BIND_DESC *binding = &reflection->bindings[i];

        if (!strcmp(binding->Name, name))
        {
            TRACE("Returning binding %p at index %u.\n", binding, i);
            memcpy(desc, binding, reflection->bind_desc_size);
            return S_OK;
        }
    }

    WARN("Binding %s not found.\n", debugstr_a(name));

    return E_INVALIDARG;
}

static UINT STDMETHODCALLTYPE d3d12_reflection_GetMovInstructionCount(ID3D12ShaderReflection *iface)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p.\n", iface);

    return reflection->mov_count;
}

static UINT STDMETHODCALLTYPE d3d12_reflection_GetMovcInstructionCount(ID3D12ShaderReflection *iface)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p.\n", iface);

    return reflection->movc_count;
}

static UINT STDMETHODCALLTYPE d3d12_reflection_GetConversionInstructionCount(ID3D12ShaderReflection *iface)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p.\n", iface);

    return reflection->type_conversion_count;
}

static UINT STDMETHODCALLTYPE d3d12_reflection_GetBitwiseInstructionCount(ID3D12ShaderReflection *iface)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p.\n", iface);

    return reflection->bitwise_count;
}

static D3D_PRIMITIVE STDMETHODCALLTYPE d3d12_reflection_GetGSInputPrimitive(ID3D12ShaderReflection *iface)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p.\n", iface);

    return reflection->desc.InputPrimitive;
}

static BOOL STDMETHODCALLTYPE d3d12_reflection_IsSampleFrequencyShader(ID3D12ShaderReflection *iface)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p.\n", iface);

    return reflection->sample_frequency;
}

static UINT STDMETHODCALLTYPE d3d12_reflection_GetNumInterfaceSlots(ID3D12ShaderReflection *iface)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p.\n", iface);

    return reflection->interface_slot_count;
}

static HRESULT STDMETHODCALLTYPE d3d12_reflection_GetMinFeatureLevel(
        ID3D12ShaderReflection *iface, D3D_FEATURE_LEVEL *level)
{
    FIXME("iface %p, level %p stub!\n", iface, level);

    return E_NOTIMPL;
}

static UINT STDMETHODCALLTYPE d3d12_reflection_GetThreadGroupSize(
        ID3D12ShaderReflection *iface, UINT *size_x, UINT *size_y, UINT *size_z)
{
    struct d3d12_reflection *reflection = impl_from_ID3D12ShaderReflection(iface);

    TRACE("iface %p, size_x %p, size_y %p, size_z %p.\n", iface, size_x, size_y, size_z);

    if (!size_x || !size_y || !size_z)
    {
        WARN("NULL size_x/size_y/size_z specified.\n");
        return E_INVALIDARG;
    }

    *size_x = reflection->thread_group_size_info.x;
    *size_y = reflection->thread_group_size_info.y;
    *size_z = reflection->thread_group_size_info.z;

    return *size_x * *size_y * *size_z;
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

static struct d3d12_reflection *impl_from_ID3D10ShaderReflection1(ID3D10ShaderReflection1 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_reflection, ID3D10ShaderReflection1_iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_1reflection_QueryInterface(
        ID3D10ShaderReflection1 *iface, REFIID iid, void **out)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID3D10ShaderReflection1) || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID3D10ShaderReflection1_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    return d3d12_reflection_QueryInterface(&reflection->ID3D12ShaderReflection_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE d3d10_1reflection_AddRef(ID3D10ShaderReflection1 *iface)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p.\n", iface);

    return d3d12_reflection_AddRef(&reflection->ID3D12ShaderReflection_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_1reflection_Release(ID3D10ShaderReflection1 *iface)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p.\n", iface);

    return d3d12_reflection_Release(&reflection->ID3D12ShaderReflection_iface);
}

/* ID3D10ShaderReflection1 methods. */

static HRESULT STDMETHODCALLTYPE d3d10_1reflection_GetDesc(ID3D10ShaderReflection1 *iface, D3D10_SHADER_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    return d3d12_reflection_GetDesc(&reflection->ID3D12ShaderReflection_iface, (D3D12_SHADER_DESC *)desc);
}

static ID3D10ShaderReflectionConstantBuffer * STDMETHODCALLTYPE d3d10_1reflection_GetConstantBufferByIndex(
        ID3D10ShaderReflection1 *iface, unsigned int index)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, index %u.\n", iface, index);

    return (ID3D10ShaderReflectionConstantBuffer *)d3d12_reflection_GetConstantBufferByIndex(
            &reflection->ID3D12ShaderReflection_iface, index);
}

static ID3D10ShaderReflectionConstantBuffer * STDMETHODCALLTYPE d3d10_1reflection_GetConstantBufferByName(
        ID3D10ShaderReflection1 *iface, const char *name)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return (ID3D10ShaderReflectionConstantBuffer *)d3d12_reflection_GetConstantBufferByName(
            &reflection->ID3D12ShaderReflection_iface, name);
}

static HRESULT STDMETHODCALLTYPE d3d10_1reflection_GetResourceBindingDesc(ID3D10ShaderReflection1 *iface,
        unsigned int index, D3D10_SHADER_INPUT_BIND_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    return d3d12_reflection_GetResourceBindingDesc(&reflection->ID3D12ShaderReflection_iface,
            index, (D3D12_SHADER_INPUT_BIND_DESC *)desc);
}

static HRESULT STDMETHODCALLTYPE d3d10_1reflection_GetInputParameterDesc(ID3D10ShaderReflection1 *iface,
        unsigned int index, D3D10_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    return d3d12_reflection_GetInputParameterDesc(&reflection->ID3D12ShaderReflection_iface,
            index, (D3D12_SIGNATURE_PARAMETER_DESC *)desc);
}

static HRESULT STDMETHODCALLTYPE d3d10_1reflection_GetOutputParameterDesc(ID3D10ShaderReflection1 *iface,
        unsigned int index, D3D10_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    return d3d12_reflection_GetOutputParameterDesc(&reflection->ID3D12ShaderReflection_iface,
            index, (D3D12_SIGNATURE_PARAMETER_DESC *)desc);
}

static ID3D10ShaderReflectionVariable * STDMETHODCALLTYPE d3d10_1reflection_GetVariableByName(
        ID3D10ShaderReflection1 *iface, const char *name)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return (ID3D10ShaderReflectionVariable *)d3d12_reflection_GetVariableByName(
            &reflection->ID3D12ShaderReflection_iface, name);
}

static HRESULT STDMETHODCALLTYPE d3d10_1reflection_GetResourceBindingDescByName(ID3D10ShaderReflection1 *iface,
        const char *name, D3D10_SHADER_INPUT_BIND_DESC *desc)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, name %s, desc %p.\n", iface, debugstr_a(name), desc);

    return d3d12_reflection_GetResourceBindingDescByName(&reflection->ID3D12ShaderReflection_iface,
            name, (D3D12_SHADER_INPUT_BIND_DESC *)desc);
}

static HRESULT STDMETHODCALLTYPE d3d10_1reflection_GetMovInstructionCount(ID3D10ShaderReflection1 *iface,
        unsigned int *count)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, count %p.\n", iface, count);

    *count = d3d12_reflection_GetMovInstructionCount(&reflection->ID3D12ShaderReflection_iface);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_1reflection_GetMovcInstructionCount(ID3D10ShaderReflection1 *iface,
        unsigned int *count)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, count %p.\n", iface, count);

    *count = d3d12_reflection_GetMovcInstructionCount(&reflection->ID3D12ShaderReflection_iface);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_1reflection_GetConversionInstructionCount(ID3D10ShaderReflection1 *iface,
        unsigned int *count)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, count %p.\n", iface, count);

    *count = d3d12_reflection_GetConversionInstructionCount(&reflection->ID3D12ShaderReflection_iface);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_1reflection_GetBitwiseInstructionCount(ID3D10ShaderReflection1 *iface,
        unsigned int *count)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, count %p.\n", iface, count);

    *count = d3d12_reflection_GetBitwiseInstructionCount(&reflection->ID3D12ShaderReflection_iface);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_1reflection_GetGSInputPrimitive(ID3D10ShaderReflection1 *iface,
        D3D_PRIMITIVE *primitive)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, primitive %p.\n", iface, primitive);

    *primitive = d3d12_reflection_GetGSInputPrimitive(&reflection->ID3D12ShaderReflection_iface);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_1reflection_IsLevel9Shader(ID3D10ShaderReflection1 *iface,
        BOOL *level9_shader)
{
    FIXME("iface %p, level9_shader %p stub!\n", iface, level9_shader);

    *level9_shader = FALSE;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_1reflection_IsSampleFrequencyShader(ID3D10ShaderReflection1 *iface,
        BOOL *sample_frequency)
{
    struct d3d12_reflection *reflection = impl_from_ID3D10ShaderReflection1(iface);

    TRACE("iface %p, sample_frequency %p.\n", iface, sample_frequency);

    *sample_frequency = d3d12_reflection_IsSampleFrequencyShader(&reflection->ID3D12ShaderReflection_iface);

    return S_OK;
}

static const struct ID3D10ShaderReflection1Vtbl d3d10_1reflection_vtbl =
{
    /* IUnknown methods. */
    d3d10_1reflection_QueryInterface,
    d3d10_1reflection_AddRef,
    d3d10_1reflection_Release,
    /* ID3D10ShaderReflection1 methods. */
    d3d10_1reflection_GetDesc,
    d3d10_1reflection_GetConstantBufferByIndex,
    d3d10_1reflection_GetConstantBufferByName,
    d3d10_1reflection_GetResourceBindingDesc,
    d3d10_1reflection_GetInputParameterDesc,
    d3d10_1reflection_GetOutputParameterDesc,
    d3d10_1reflection_GetVariableByName,
    d3d10_1reflection_GetResourceBindingDescByName,
    d3d10_1reflection_GetMovInstructionCount,
    d3d10_1reflection_GetMovcInstructionCount,
    d3d10_1reflection_GetConversionInstructionCount,
    d3d10_1reflection_GetBitwiseInstructionCount,
    d3d10_1reflection_GetGSInputPrimitive,
    d3d10_1reflection_IsLevel9Shader,
    d3d10_1reflection_IsSampleFrequencyShader,
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
    uint32_t interface_slot_count;
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

struct stat
{
    uint32_t instruction_count;
    uint32_t temp_count;
    uint32_t def_count;
    uint32_t dcl_count;
    uint32_t float_count;
    uint32_t int_count;
    uint32_t uint_count;
    uint32_t static_flow_control_count;
    uint32_t dynamic_flow_control_count;
    uint32_t macro_instruction_count;
    uint32_t temp_array_count;
    uint32_t array_instruction_count;
    uint32_t cut_count;
    uint32_t emit_count;
    uint32_t sample_count;
    uint32_t load_count;
    uint32_t sample_compare_count;
    uint32_t sample_bias_count;
    uint32_t sample_grad_count;
    uint32_t mov_count;
    uint32_t movc_count;
    uint32_t type_conversion_count;
    uint32_t bitwise_count;
    uint32_t input_primitive;
    uint32_t gs_output_topology;
    uint32_t gs_max_output_vertex_count;
    uint32_t gather_count;
    uint32_t lod_count;
    uint32_t sample_frequency;
    uint32_t gs_instance_count;
    uint32_t control_point_count;
    uint32_t hs_output_primitive;
    uint32_t hs_partitioning;
    uint32_t tessellator_domain;
    uint32_t barrier_count;
    uint32_t atomic_count;
    uint32_t store_count;
};

static HRESULT d3d12_type_init(struct d3d12_type *type, struct d3d12_reflection *reflection,
        uint32_t type_offset, uint32_t type_size, const struct vkd3d_shader_code *section, uint32_t field_offset);

static HRESULT d3d12_field_init(struct d3d12_field *field, struct d3d12_reflection *reflection,
        uint32_t type_size, const struct vkd3d_shader_code *section, const struct rdef_field *rdef_field)
{
    HRESULT hr;

    if (FAILED(hr = get_string(section, rdef_field->name_offset, &field->name)))
        return hr;

    if (FAILED(hr = d3d12_type_init(&field->type, reflection,
            rdef_field->type_offset, type_size, section, rdef_field->offset)))
    {
        WARN("Failed to parse field type.\n");
        vkd3d_free(field->name);
        return hr;
    }

    return S_OK;
}

static HRESULT d3d12_type_init(struct d3d12_type *type, struct d3d12_reflection *reflection,
        uint32_t type_offset, uint32_t type_size, const struct vkd3d_shader_code *section, uint32_t field_offset)
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
    type->id = type_offset;

    type->desc.Class = normalized_type.class;
    type->desc.Type = normalized_type.base_type;
    type->desc.Rows = normalized_type.row_count;
    type->desc.Columns = normalized_type.column_count;
    type->desc.Elements = normalized_type.element_count;
    type->desc.Members = normalized_type.field_count;
    type->desc.Offset = field_offset;
    type->desc.Name = name;

    if (reflection->version >= 40)
        type->desc_size = sizeof(D3D12_SHADER_TYPE_DESC);
    else
        type->desc_size = sizeof(D3D10_SHADER_TYPE_DESC);

    if (normalized_type.field_count)
    {
        const struct rdef_field *rdef_fields;

        if (!(rdef_fields = get_data_ptr(section, normalized_type.fields_offset,
                normalized_type.field_count, sizeof(*rdef_fields))))
        {
            vkd3d_free(name);
            return E_INVALIDARG;
        }

        if (!(type->fields = vkd3d_calloc(normalized_type.field_count, sizeof(*type->fields))))
        {
            vkd3d_free(name);
            return E_OUTOFMEMORY;
        }

        for (uint32_t i = 0; i < normalized_type.field_count; ++i)
        {
            if (FAILED(hr = d3d12_field_init(&type->fields[i], reflection, type_size, section, &rdef_fields[i])))
            {
                type->desc.Members = i;
                d3d12_type_cleanup(type);
                return hr;
            }
        }
    }

    return S_OK;
}

static HRESULT d3d12_variable_init(struct d3d12_variable *variable, struct d3d12_reflection *reflection,
        struct d3d12_buffer *buffer, const struct rdef_variable *rdef_variable,
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
        {
            vkd3d_free(name);
            return E_INVALIDARG;
        }

        if (!(variable->desc.DefaultValue = vkd3d_memdup(default_value, rdef_variable->size)))
        {
            vkd3d_free(name);
            return E_OUTOFMEMORY;
        }
    }

    if (reflection->version >= 41)
        variable->desc_size = sizeof(D3D12_SHADER_VARIABLE_DESC);
    else
        variable->desc_size = sizeof(D3D10_SHADER_VARIABLE_DESC);
    variable->buffer = buffer;

    if (FAILED(hr = d3d12_type_init(&variable->type, reflection, rdef_variable->type_offset, type_size, section, 0)))
    {
        vkd3d_free(variable->desc.DefaultValue);
        vkd3d_free(name);
    }

    return hr;
}

static HRESULT d3d12_buffer_init(struct d3d12_buffer *buffer,
        struct d3d12_reflection *reflection, const struct rdef_buffer *rdef_buffer,
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
    {
        vkd3d_free(name);
        return E_OUTOFMEMORY;
    }

    for (uint32_t i = 0; i < rdef_buffer->var_count; ++i)
    {
        struct rdef_variable normalized_variable = {0};
        const struct rdef_variable *rdef_variable;

        if (!(rdef_variable = get_data_ptr(section, rdef_buffer->vars_offset + (i * variable_size), 1, variable_size)))
        {
            buffer->desc.Variables = i;
            d3d12_buffer_cleanup(buffer);
            return E_INVALIDARG;
        }

        normalized_variable.resource_binding = ~0u;
        normalized_variable.sampler_binding = ~0u;
        memcpy(&normalized_variable, rdef_variable, variable_size);

        if (FAILED(hr = d3d12_variable_init(&buffer->variables[i], reflection,
                buffer, &normalized_variable, section, type_size)))
        {
            buffer->desc.Variables = i;
            d3d12_buffer_cleanup(buffer);
            return hr;
        }
    }

    return S_OK;
}

static bool rdef_header_version_ge(const struct rdef_header *header, unsigned int major, unsigned int minor)
{
    return header->major_version > major || (header->major_version == major && header->minor_version >= minor);
}

static HRESULT parse_rdef(struct d3d12_reflection *reflection, const struct vkd3d_shader_code *section)
{
    uint32_t variable_size = offsetof(struct rdef_variable, resource_binding);
    uint32_t binding_size = offsetof(struct rdef_binding, space);
    uint32_t type_size = offsetof(struct rdef_type, unknown);
    const struct rdef_header *header;
    const struct rdef_rd11 *rd11;
    char *creator;
    HRESULT hr;

    if (!(header = get_data_ptr(section, 0, 1, sizeof(*header))))
        return E_INVALIDARG;

    if ((reflection->version < 40 && (rdef_header_version_ge(header, 5, 0) || header->type == RDEF_SHADER_TYPE_CS))
            || (reflection->version < 47 && rdef_header_version_ge(header, 5, 1)
                    && header->type != RDEF_SHADER_TYPE_CS))
    {
        WARN("Shader version %u.%u, type %#x is not supported by compiler version %u.\n",
                header->major_version, header->minor_version, header->type, reflection->version);
        return E_INVALIDARG;
    }

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

        if (rd11->field_size != sizeof(struct rdef_field))
        {
            FIXME("Unexpected field size %#x.\n", rd11->field_size);
            return E_INVALIDARG;
        }

        reflection->interface_slot_count = rd11->interface_slot_count;
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
            if (FAILED(hr = d3d12_buffer_init(&reflection->buffers[i], reflection,
                    &rdef_buffers[i], section, variable_size, type_size)))
            {
                reflection->desc.ConstantBuffers = i;
                d3d12_reflection_free_buffers(reflection);
                return hr;
            }
        }
    }

    reflection->desc.BoundResources = header->binding_count;

    if (header->binding_count)
    {
        if (!(reflection->bindings = vkd3d_calloc(header->binding_count, sizeof(*reflection->bindings))))
        {
            d3d12_reflection_free_buffers(reflection);
            return E_OUTOFMEMORY;
        }

        for (uint32_t i = 0; i < header->binding_count; ++i)
        {
            const struct rdef_binding *rdef_binding;
            D3D12_SHADER_INPUT_BIND_DESC *binding;
            char *name;

            if (!(rdef_binding = get_data_ptr(section, header->bindings_offset + (i * binding_size), 1, binding_size)))
            {
                reflection->desc.BoundResources = i;
                hr = E_INVALIDARG;
                goto fail;
            }

            if (FAILED(hr = get_string(section, rdef_binding->name_offset, &name)))
            {
                reflection->desc.BoundResources = i;
                goto fail;
            }

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

    reflection->desc.Flags = header->compile_flags;
    if (FAILED(hr = get_string(section, header->creator_offset, &creator)))
        goto fail;
    reflection->desc.Creator = creator;

    return S_OK;

fail:
    d3d12_reflection_free_bindings(reflection);
    d3d12_reflection_free_buffers(reflection);

    return hr;
}

static HRESULT parse_stat(struct d3d12_reflection *r, const struct vkd3d_shader_code *section)
{
    struct stat normalised_stat = {0};
    size_t stat_size = section->size;
    const struct stat *stat;

    if (stat_size != sizeof(struct stat)
            && stat_size != offsetof(struct stat, gs_instance_count)
            && stat_size != offsetof(struct stat, sample_frequency))
    {
        FIXME("Unexpected STAT size %#zx.\n", stat_size);
        return E_FAIL;
    }

    if (!(stat = get_data_ptr(section, 0, 1, stat_size)))
        return E_INVALIDARG;
    memcpy(&normalised_stat, stat, stat_size);

    r->desc.InstructionCount = normalised_stat.instruction_count;
    r->desc.TempRegisterCount = normalised_stat.temp_count;
    r->desc.DefCount = normalised_stat.def_count;
    r->desc.DclCount = normalised_stat.dcl_count;
    r->desc.FloatInstructionCount = normalised_stat.float_count;
    r->desc.IntInstructionCount = normalised_stat.int_count;
    r->desc.UintInstructionCount = normalised_stat.uint_count;
    r->desc.StaticFlowControlCount = normalised_stat.static_flow_control_count;
    r->desc.DynamicFlowControlCount = normalised_stat.dynamic_flow_control_count;
    r->desc.MacroInstructionCount = normalised_stat.macro_instruction_count;
    r->desc.TempArrayCount = normalised_stat.temp_array_count;
    r->desc.ArrayInstructionCount = normalised_stat.array_instruction_count;
    r->desc.CutInstructionCount = normalised_stat.cut_count;
    r->desc.EmitInstructionCount = normalised_stat.emit_count;
    r->desc.TextureNormalInstructions = normalised_stat.sample_count;
    r->desc.TextureLoadInstructions = normalised_stat.load_count;
    r->desc.TextureCompInstructions = normalised_stat.sample_compare_count;
    r->desc.TextureBiasInstructions = normalised_stat.sample_bias_count;
    r->desc.TextureGradientInstructions = normalised_stat.sample_grad_count;
    r->mov_count = normalised_stat.mov_count;
    r->movc_count = normalised_stat.movc_count;
    r->type_conversion_count = normalised_stat.type_conversion_count;
    r->bitwise_count = normalised_stat.bitwise_count;
    r->desc.InputPrimitive = normalised_stat.input_primitive;
    r->desc.GSOutputTopology = normalised_stat.gs_output_topology;
    r->desc.GSMaxOutputVertexCount = normalised_stat.gs_max_output_vertex_count;
    r->sample_frequency = normalised_stat.sample_frequency;
    r->desc.cGSInstanceCount = normalised_stat.gs_instance_count;
    r->desc.cControlPoints = normalised_stat.control_point_count;
    r->desc.HSOutputPrimitive = normalised_stat.hs_output_primitive;
    r->desc.HSPartitioning = normalised_stat.hs_partitioning;
    r->desc.TessellatorDomain = normalised_stat.tessellator_domain;
    r->desc.cBarrierInstructions = normalised_stat.barrier_count;
    r->desc.cInterlockedInstructions = normalised_stat.atomic_count;
    r->desc.cTextureStoreInstructions = normalised_stat.store_count;

    return S_OK;
}

static HRESULT d3d12_reflection_init(struct d3d12_reflection *reflection,
        const IID *iid, const void *data, size_t data_size, unsigned int version)
{
    struct vkd3d_shader_compile_info compile_info = {.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO};
    bool found_rdef = false, found_stat = false;
    struct vkd3d_shader_dxbc_desc dxbc_desc;
    enum vkd3d_result ret;
    HRESULT hr;

    reflection->ID3D12ShaderReflection_iface.lpVtbl = &d3d12_reflection_vtbl;
    reflection->ID3D10ShaderReflection1_iface.lpVtbl = &d3d10_1reflection_vtbl;
    reflection->refcount = 1;
    reflection->version = version;

    compile_info.next = &reflection->signature_info;
    compile_info.source.code = data;
    compile_info.source.size = data_size;
    compile_info.source_type = VKD3D_SHADER_SOURCE_DXBC_TPF;

    reflection->signature_info.type = VKD3D_SHADER_STRUCTURE_TYPE_SCAN_SIGNATURE_INFO;
    reflection->signature_info.next = &reflection->thread_group_size_info;

    reflection->thread_group_size_info.type = VKD3D_SHADER_STRUCTURE_TYPE_SCAN_THREAD_GROUP_SIZE_INFO;

    if (!data || (ret = vkd3d_shader_parse_dxbc(&compile_info.source, 0, &dxbc_desc, NULL)) < 0)
    {
        if (version < 43)
            return E_INVALIDARG;
        if (version < 44 && data && data_size >= 8 * sizeof(uint32_t))
            return E_FAIL;
        return D3DERR_INVALIDCALL;
    }

    if ((ret = vkd3d_shader_scan(&compile_info, NULL)) < 0)
    {
        vkd3d_shader_free_dxbc(&dxbc_desc);
        return hresult_from_vkd3d_result(ret);
    }

    if (reflection->version >= 40)
        reflection->desc_size = sizeof(D3D12_SHADER_DESC);
    else
        reflection->desc_size = sizeof(D3D10_SHADER_DESC);

    if (IsEqualGUID(iid, &IID_ID3D12ShaderReflection))
        reflection->bind_desc_size = sizeof(D3D12_SHADER_INPUT_BIND_DESC);
    else
        reflection->bind_desc_size = sizeof(D3D10_SHADER_INPUT_BIND_DESC);

    for (unsigned int i = 0; i < dxbc_desc.section_count; ++i)
    {
        const struct vkd3d_shader_dxbc_section_desc *section = &dxbc_desc.sections[i];

        if (section->tag == TAG_RDEF)
        {
            if (found_rdef)
            {
                FIXME("Multiple RDEF sections.\n");
                continue;
            }

            if (FAILED(hr = parse_rdef(reflection, &section->data)))
                goto fail;
            found_rdef = true;
        }
        else if (section->tag == TAG_SHDR || section->tag == TAG_SHEX)
        {
            const uint32_t *shader_version;

            if (!(shader_version = get_data_ptr(&section->data, 0, 1, sizeof(*shader_version))))
            {
                hr = E_INVALIDARG;
                goto fail;
            }
            reflection->desc.Version = *shader_version;
        }
        else if (section->tag == TAG_STAT)
        {
            if (found_stat)
            {
                FIXME("Multiple STAT sections.\n");
                continue;
            }

            if (FAILED(hr = parse_stat(reflection, &section->data)))
                goto fail;
            found_stat = true;
        }
    }

    reflection->desc.InputParameters = reflection->signature_info.input.element_count;
    reflection->desc.OutputParameters = reflection->signature_info.output.element_count;
    reflection->desc.PatchConstantParameters = reflection->signature_info.patch_constant.element_count;

    if (!reflection_iid_supported(iid, version))
    {
        WARN("Unhandled iid %s, version %u.\n", debugstr_guid(iid), version);
        hr = version == 47 ? E_INVALIDARG : E_NOINTERFACE;
        goto fail;
    }

    vkd3d_shader_free_dxbc(&dxbc_desc);

    return S_OK;

fail:
    d3d12_reflection_cleanup(reflection);
    vkd3d_shader_free_dxbc(&dxbc_desc);
    return hr;
}

HRESULT WINAPI D3DReflectVKD3D(const void *data, SIZE_T data_size, REFIID iid, void **reflection, unsigned int version)
{
    struct d3d12_reflection *object;
    HRESULT hr;

    TRACE("data %p, data_size %"PRIuPTR", iid %s, reflection %p, version %u.\n",
            data, (uintptr_t)data_size, debugstr_guid(iid), reflection, version);

    if (!(object = vkd3d_calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d12_reflection_init(object, iid, data, data_size, version)))
    {
        vkd3d_free(object);
        return hr;
    }

    if (IsEqualGUID(iid, &IID_ID3D10ShaderReflection1))
        *reflection = &object->ID3D10ShaderReflection1_iface;
    else
        *reflection = &object->ID3D12ShaderReflection_iface;
    TRACE("Created reflection %p.\n", object);

    return S_OK;
}

HRESULT WINAPI D3DReflect(const void *data, SIZE_T data_size, REFIID iid, void **reflection)
{
    TRACE("data %p, data_size %"PRIuPTR", iid %s, reflection %p.\n",
            data, (uintptr_t)data_size, debugstr_guid(iid), reflection);

    return D3DReflectVKD3D(data, data_size, iid, reflection, 47);
}

HRESULT WINAPI D3D10ReflectShader(const void *data, SIZE_T data_size, ID3D10ShaderReflection **reflection)
{
    TRACE("data %p, data_size %"PRIuPTR", reflection %p.\n",
            data, (uintptr_t)data_size, reflection);

    return D3DReflectVKD3D(data, data_size, &IID_ID3D10ShaderReflection, (void **)reflection, 0);
}
