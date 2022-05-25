/*
 * Copyright 2018 Nikolay Sivov for CodeWeavers
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

#include "d2d1_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

static const struct d2d_effect_info builtin_effects[] =
{
    {&CLSID_D2D12DAffineTransform,      1, 1, 1},
    {&CLSID_D2D13DPerspectiveTransform, 1, 1, 1},
    {&CLSID_D2D1Composite,              2, 1, 0xffffffff},
    {&CLSID_D2D1Crop,                   1, 1, 1},
    {&CLSID_D2D1Shadow,                 1, 1, 1},
    {&CLSID_D2D1Grayscale,              1, 1, 1},
};

static inline struct d2d_effect_context *impl_from_ID2D1EffectContext(ID2D1EffectContext *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_effect_context, ID2D1EffectContext_iface);
}

static void d2d_effect_context_cleanup(struct d2d_effect_context *effect_context)
{
    size_t i;

    for (i = 0; i < effect_context->shader_count; ++i)
        IUnknown_Release(effect_context->shaders[i].shader);
    heap_free(effect_context->shaders);

    ID2D1DeviceContext_Release(&effect_context->device_context->ID2D1DeviceContext_iface);
}

static HRESULT d2d_effect_context_add_shader(struct d2d_effect_context *effect_context,
        REFGUID shader_id, void *shader)
{
    struct d2d_shader *entry;

    if (!d2d_array_reserve((void **)&effect_context->shaders, &effect_context->shaders_size,
            effect_context->shader_count + 1, sizeof(*effect_context->shaders)))
    {
        WARN("Failed to resize shaders array.\n");
        return E_OUTOFMEMORY;
    }

    entry = &effect_context->shaders[effect_context->shader_count++];
    entry->id = *shader_id;
    entry->shader = shader;
    IUnknown_AddRef(entry->shader);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_QueryInterface(ID2D1EffectContext *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1EffectContext)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1EffectContext_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_effect_context_AddRef(ID2D1EffectContext *iface)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    ULONG refcount = InterlockedIncrement(&effect_context->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_effect_context_Release(ID2D1EffectContext *iface)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    ULONG refcount = InterlockedDecrement(&effect_context->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        d2d_effect_context_cleanup(effect_context);
        heap_free(effect_context);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_effect_context_GetDpi(ID2D1EffectContext *iface, float *dpi_x, float *dpi_y)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);

    TRACE("iface %p, dpi_x %p, dpi_y %p.\n", iface, dpi_x, dpi_y);

    ID2D1DeviceContext_GetDpi(&effect_context->device_context->ID2D1DeviceContext_iface, dpi_x, dpi_y);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateEffect(ID2D1EffectContext *iface,
        REFCLSID clsid, ID2D1Effect **effect)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    struct d2d_effect *object;
    HRESULT hr;

    TRACE("iface %p, clsid %s, effect %p.\n", iface, debugstr_guid(clsid), effect);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d2d_effect_init(object, effect_context, clsid)))
    {
        WARN("Failed to initialise effect, hr %#lx.\n", hr);
        heap_free(object);
        return hr;
    }

    TRACE("Created effect %p.\n", object);
    *effect = &object->ID2D1Effect_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_GetMaximumSupportedFeatureLevel(ID2D1EffectContext *iface,
        const D3D_FEATURE_LEVEL *levels, UINT32 level_count, D3D_FEATURE_LEVEL *max_level)
{
    FIXME("iface %p, levels %p, level_count %u, max_level %p stub!\n", iface, levels, level_count, max_level);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateTransformNodeFromEffect(ID2D1EffectContext *iface,
        ID2D1Effect *effect, ID2D1TransformNode **node)
{
    FIXME("iface %p, effect %p, node %p stub!\n", iface, effect, node);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateBlendTransform(ID2D1EffectContext *iface,
        UINT32 num_inputs, const D2D1_BLEND_DESCRIPTION *description, ID2D1BlendTransform **transform)
{
    FIXME("iface %p, num_inputs %u, description %p, transform %p stub!\n", iface, num_inputs, description, transform);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateBorderTransform(ID2D1EffectContext *iface,
        D2D1_EXTEND_MODE mode_x, D2D1_EXTEND_MODE mode_y, ID2D1BorderTransform **transform)
{
    FIXME("iface %p, mode_x %#x, mode_y %#x, transform %p stub!\n", iface, mode_x, mode_y, transform);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateOffsetTransform(ID2D1EffectContext *iface,
        D2D1_POINT_2L offset, ID2D1OffsetTransform **transform)
{
    FIXME("iface %p, offset %s, transform %p stub!\n", iface, debug_d2d_point_2l(&offset), transform);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateBoundsAdjustmentTransform(ID2D1EffectContext *iface,
        const D2D1_RECT_L *output_rect, ID2D1BoundsAdjustmentTransform **transform)
{
    FIXME("iface %p, output_rect %s, transform %p stub!\n", iface, debug_d2d_rect_l(output_rect), transform);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_LoadPixelShader(ID2D1EffectContext *iface,
        REFGUID shader_id, const BYTE *buffer, UINT32 buffer_size)
{
    FIXME("iface %p, shader_id %s, buffer %p, buffer_size %u stub!\n",
            iface, debugstr_guid(shader_id), buffer, buffer_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_LoadVertexShader(ID2D1EffectContext *iface,
        REFGUID shader_id, const BYTE *buffer, UINT32 buffer_size)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    ID3D11VertexShader *vertex_shader;
    HRESULT hr;

    TRACE("iface %p, shader_id %s, buffer %p, buffer_size %u.\n",
            iface, debugstr_guid(shader_id), buffer, buffer_size);

    if (ID2D1EffectContext_IsShaderLoaded(iface, shader_id))
        return S_OK;

    if (FAILED(hr = ID3D11Device1_CreateVertexShader(effect_context->device_context->d3d_device,
            buffer, buffer_size, NULL, &vertex_shader)))
    {
        WARN("Failed to create vertex shader, hr %#lx.\n", hr);
        return hr;
    }

    hr = d2d_effect_context_add_shader(effect_context, shader_id, vertex_shader);
    ID3D11VertexShader_Release(vertex_shader);

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_LoadComputeShader(ID2D1EffectContext *iface,
        REFGUID shader_id, const BYTE *buffer, UINT32 buffer_size)
{
    FIXME("iface %p, shader_id %s, buffer %p, buffer_size %u stub!\n",
            iface, debugstr_guid(shader_id), buffer, buffer_size);

    return E_NOTIMPL;
}

static BOOL STDMETHODCALLTYPE d2d_effect_context_IsShaderLoaded(ID2D1EffectContext *iface, REFGUID shader_id)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    size_t i;

    TRACE("iface %p, shader_id %s.\n", iface, debugstr_guid(shader_id));

    for (i = 0; i < effect_context->shader_count; ++i)
    {
        if (IsEqualGUID(shader_id, &effect_context->shaders[i].id))
            return TRUE;
    }

    return FALSE;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateResourceTexture(ID2D1EffectContext *iface,
        const GUID *id,  const D2D1_RESOURCE_TEXTURE_PROPERTIES *texture_properties,
        const BYTE *data, const UINT32 *strides, UINT32 data_size, ID2D1ResourceTexture **texture)
{
    FIXME("iface %p, id %s, texture_properties %p, data %p, strides %p, data_size %u, texture %p stub!\n",
            iface, debugstr_guid(id), texture_properties, data, strides, data_size, texture);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_FindResourceTexture(ID2D1EffectContext *iface,
        const GUID *id, ID2D1ResourceTexture **texture)
{
    FIXME("iface %p, id %s, texture %p stub!\n", iface, debugstr_guid(id), texture);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateVertexBuffer(ID2D1EffectContext *iface,
        const D2D1_VERTEX_BUFFER_PROPERTIES *buffer_properties, const GUID *id,
        const D2D1_CUSTOM_VERTEX_BUFFER_PROPERTIES *custom_buffer_properties, ID2D1VertexBuffer **buffer)
{
    FIXME("iface %p, buffer_properties %p, id %s, custom_buffer_properties %p, buffer %p stub!\n",
            iface, buffer_properties, debugstr_guid(id), custom_buffer_properties, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_FindVertexBuffer(ID2D1EffectContext *iface,
        const GUID *id, ID2D1VertexBuffer **buffer)
{
    FIXME("iface %p, id %s, buffer %p stub!\n", iface, debugstr_guid(id), buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateColorContext(ID2D1EffectContext *iface,
        D2D1_COLOR_SPACE space, const BYTE *profile, UINT32 profile_size, ID2D1ColorContext **color_context)
{
    FIXME("iface %p, space %#x, profile %p, profile_size %u, color_context %p stub!\n",
            iface, space, profile, profile_size, color_context);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateColorContextFromFilename(ID2D1EffectContext *iface,
        const WCHAR *filename, ID2D1ColorContext **color_context)
{
    FIXME("iface %p, filename %s, color_context %p stub!\n", iface, debugstr_w(filename), color_context);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateColorContextFromWicColorContext(ID2D1EffectContext *iface,
        IWICColorContext *wic_color_context, ID2D1ColorContext **color_context)
{
    FIXME("iface %p, wic_color_context %p, color_context %p stub!\n", iface, wic_color_context, color_context);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CheckFeatureSupport(ID2D1EffectContext *iface,
        D2D1_FEATURE feature, void *data, UINT32 data_size)
{
    FIXME("iface %p, feature %#x, data %p, data_size %u stub!\n", iface, feature, data, data_size);

    return E_NOTIMPL;
}

static BOOL STDMETHODCALLTYPE d2d_effect_context_IsBufferPrecisionSupported(ID2D1EffectContext *iface,
        D2D1_BUFFER_PRECISION precision)
{
    FIXME("iface %p, precision %#x stub!\n", iface, precision);

    return FALSE;
}

static const ID2D1EffectContextVtbl d2d_effect_context_vtbl =
{
    d2d_effect_context_QueryInterface,
    d2d_effect_context_AddRef,
    d2d_effect_context_Release,
    d2d_effect_context_GetDpi,
    d2d_effect_context_CreateEffect,
    d2d_effect_context_GetMaximumSupportedFeatureLevel,
    d2d_effect_context_CreateTransformNodeFromEffect,
    d2d_effect_context_CreateBlendTransform,
    d2d_effect_context_CreateBorderTransform,
    d2d_effect_context_CreateOffsetTransform,
    d2d_effect_context_CreateBoundsAdjustmentTransform,
    d2d_effect_context_LoadPixelShader,
    d2d_effect_context_LoadVertexShader,
    d2d_effect_context_LoadComputeShader,
    d2d_effect_context_IsShaderLoaded,
    d2d_effect_context_CreateResourceTexture,
    d2d_effect_context_FindResourceTexture,
    d2d_effect_context_CreateVertexBuffer,
    d2d_effect_context_FindVertexBuffer,
    d2d_effect_context_CreateColorContext,
    d2d_effect_context_CreateColorContextFromFilename,
    d2d_effect_context_CreateColorContextFromWicColorContext,
    d2d_effect_context_CheckFeatureSupport,
    d2d_effect_context_IsBufferPrecisionSupported,
};

void d2d_effect_context_init(struct d2d_effect_context *effect_context, struct d2d_device_context *device_context)
{
    effect_context->ID2D1EffectContext_iface.lpVtbl = &d2d_effect_context_vtbl;
    effect_context->refcount = 1;
    effect_context->device_context = device_context;
    ID2D1DeviceContext_AddRef(&device_context->ID2D1DeviceContext_iface);
}

static inline struct d2d_effect *impl_from_ID2D1Effect(ID2D1Effect *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_effect, ID2D1Effect_iface);
}

static void d2d_effect_cleanup(struct d2d_effect *effect)
{
    unsigned int i;

    for (i = 0; i < effect->input_count; ++i)
    {
        if (effect->inputs[i])
            ID2D1Image_Release(effect->inputs[i]);
    }
    heap_free(effect->inputs);
    ID2D1EffectContext_Release(&effect->effect_context->ID2D1EffectContext_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_QueryInterface(ID2D1Effect *iface, REFIID iid, void **out)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1Effect)
            || IsEqualGUID(iid, &IID_ID2D1Properties)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1Effect_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ID2D1Image)
            || IsEqualGUID(iid, &IID_ID2D1Resource))
    {
        ID2D1Image_AddRef(&effect->ID2D1Image_iface);
        *out = &effect->ID2D1Image_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_effect_AddRef(ID2D1Effect *iface)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);
    ULONG refcount = InterlockedIncrement(&effect->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_effect_Release(ID2D1Effect *iface)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);
    ULONG refcount = InterlockedDecrement(&effect->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        d2d_effect_cleanup(effect);
        heap_free(effect);
    }

    return refcount;
}

static UINT32 STDMETHODCALLTYPE d2d_effect_GetPropertyCount(ID2D1Effect *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_GetPropertyName(ID2D1Effect *iface, UINT32 index,
        WCHAR *name, UINT32 name_count)
{
    FIXME("iface %p, index %u, name %p, name_count %u stub!\n", iface, index, name, name_count);

    return E_NOTIMPL;
}

static UINT32 STDMETHODCALLTYPE d2d_effect_GetPropertyNameLength(ID2D1Effect *iface, UINT32 index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return 0;
}

static D2D1_PROPERTY_TYPE STDMETHODCALLTYPE d2d_effect_GetType(ID2D1Effect *iface, UINT32 index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return 0;
}

static UINT32 STDMETHODCALLTYPE d2d_effect_GetPropertyIndex(ID2D1Effect *iface, const WCHAR *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_w(name));

    return 0;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_SetValueByName(ID2D1Effect *iface, const WCHAR *name,
        D2D1_PROPERTY_TYPE type, const BYTE *value, UINT32 value_size)
{
    FIXME("iface %p, name %s, type %#x, value %p, value_size %u stub!\n", iface, debugstr_w(name),
            type, value, value_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_SetValue(ID2D1Effect *iface, UINT32 index, D2D1_PROPERTY_TYPE type,
        const BYTE *value, UINT32 value_size)
{
    FIXME("iface %p, index %u, type %#x, value %p, value_size %u stub!\n", iface, index, type, value, value_size);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_GetValueByName(ID2D1Effect *iface, const WCHAR *name,
        D2D1_PROPERTY_TYPE type, BYTE *value, UINT32 value_size)
{
    FIXME("iface %p, name %s, type %#x, value %p, value_size %u stub!\n", iface, debugstr_w(name), type,
            value, value_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_GetValue(ID2D1Effect *iface, UINT32 index, D2D1_PROPERTY_TYPE type,
        BYTE *value, UINT32 value_size)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);
    const void *src;

    TRACE("iface %p, index %u, type %#x, value %p, value_size %u.\n", iface, index, type, value, value_size);

    switch (index)
    {
        case D2D1_PROPERTY_CLSID:
            if ((type != D2D1_PROPERTY_TYPE_UNKNOWN && type != D2D1_PROPERTY_TYPE_CLSID)
                    || value_size != sizeof(*effect->info->clsid))
                return E_INVALIDARG;
            src = effect->info->clsid;
            break;
        case D2D1_PROPERTY_MIN_INPUTS:
            if ((type != D2D1_PROPERTY_TYPE_UNKNOWN && type != D2D1_PROPERTY_TYPE_UINT32)
                    || value_size != sizeof(effect->info->min_inputs))
                return E_INVALIDARG;
            src = &effect->info->min_inputs;
            break;
        case D2D1_PROPERTY_MAX_INPUTS:
            if ((type != D2D1_PROPERTY_TYPE_UNKNOWN && type != D2D1_PROPERTY_TYPE_UINT32)
                    || value_size != sizeof(effect->info->max_inputs))
                return E_INVALIDARG;
            src = &effect->info->max_inputs;
            break;
        default:
            if (index < D2D1_PROPERTY_CLSID)
                FIXME("Custom properties are not supported.\n");
            else if (index <= D2D1_PROPERTY_MAX_INPUTS)
                FIXME("Standard property %#x is not supported.\n", index);
            return D2DERR_INVALID_PROPERTY;
    }

    memcpy(value, src, value_size);

    return S_OK;
}

static UINT32 STDMETHODCALLTYPE d2d_effect_GetValueSize(ID2D1Effect *iface, UINT32 index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_GetSubProperties(ID2D1Effect *iface, UINT32 index, ID2D1Properties **props)
{
    FIXME("iface %p, index %u, props %p stub!\n", iface, index, props);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_effect_SetInput(ID2D1Effect *iface, UINT32 index, ID2D1Image *input, BOOL invalidate)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, index %u, input %p, invalidate %#x.\n", iface, index, input, invalidate);

    if (index >= effect->input_count)
        return;

    ID2D1Image_AddRef(input);
    if (effect->inputs[index])
        ID2D1Image_Release(effect->inputs[index]);
    effect->inputs[index] = input;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_SetInputCount(ID2D1Effect *iface, UINT32 count)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);
    unsigned int i;

    TRACE("iface %p, count %u.\n", iface, count);

    if (count < effect->info->min_inputs || count > effect->info->max_inputs)
        return E_INVALIDARG;
    if (count == effect->input_count)
        return S_OK;

    if (count < effect->input_count)
    {
        for (i = count; i < effect->input_count; ++i)
        {
            if (effect->inputs[i])
                ID2D1Image_Release(effect->inputs[i]);
        }
        effect->input_count = count;
        return S_OK;
    }

    if (!d2d_array_reserve((void **)&effect->inputs, &effect->inputs_size,
            count, sizeof(*effect->inputs)))
    {
        ERR("Failed to resize inputs array.\n");
        return E_OUTOFMEMORY;
    }

    memset(&effect->inputs[effect->input_count], 0, sizeof(*effect->inputs) * (count - effect->input_count));
    effect->input_count = count;

    return S_OK;
}

static void STDMETHODCALLTYPE d2d_effect_GetInput(ID2D1Effect *iface, UINT32 index, ID2D1Image **input)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, index %u, input %p.\n", iface, index, input);

    if (index < effect->input_count && effect->inputs[index])
        ID2D1Image_AddRef(*input = effect->inputs[index]);
    else
        *input = NULL;
}

static UINT32 STDMETHODCALLTYPE d2d_effect_GetInputCount(ID2D1Effect *iface)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p.\n", iface);

    return effect->input_count;
}

static void STDMETHODCALLTYPE d2d_effect_GetOutput(ID2D1Effect *iface, ID2D1Image **output)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, output %p.\n", iface, output);

    ID2D1Image_AddRef(*output = &effect->ID2D1Image_iface);
}

static const ID2D1EffectVtbl d2d_effect_vtbl =
{
    d2d_effect_QueryInterface,
    d2d_effect_AddRef,
    d2d_effect_Release,
    d2d_effect_GetPropertyCount,
    d2d_effect_GetPropertyName,
    d2d_effect_GetPropertyNameLength,
    d2d_effect_GetType,
    d2d_effect_GetPropertyIndex,
    d2d_effect_SetValueByName,
    d2d_effect_SetValue,
    d2d_effect_GetValueByName,
    d2d_effect_GetValue,
    d2d_effect_GetValueSize,
    d2d_effect_GetSubProperties,
    d2d_effect_SetInput,
    d2d_effect_SetInputCount,
    d2d_effect_GetInput,
    d2d_effect_GetInputCount,
    d2d_effect_GetOutput,
};

static inline struct d2d_effect *impl_from_ID2D1Image(ID2D1Image *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_effect, ID2D1Image_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_image_QueryInterface(ID2D1Image *iface, REFIID iid, void **out)
{
    struct d2d_effect *effect = impl_from_ID2D1Image(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    return d2d_effect_QueryInterface(&effect->ID2D1Effect_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE d2d_effect_image_AddRef(ID2D1Image *iface)
{
    struct d2d_effect *effect = impl_from_ID2D1Image(iface);

    TRACE("iface %p.\n", iface);

    return d2d_effect_AddRef(&effect->ID2D1Effect_iface);
}

static ULONG STDMETHODCALLTYPE d2d_effect_image_Release(ID2D1Image *iface)
{
    struct d2d_effect *effect = impl_from_ID2D1Image(iface);

    TRACE("iface %p.\n", iface);

    return d2d_effect_Release(&effect->ID2D1Effect_iface);
}

static void STDMETHODCALLTYPE d2d_effect_image_GetFactory(ID2D1Image *iface, ID2D1Factory **factory)
{
    struct d2d_effect *effect = impl_from_ID2D1Image(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = effect->effect_context->device_context->factory);
}

static const ID2D1ImageVtbl d2d_effect_image_vtbl =
{
    d2d_effect_image_QueryInterface,
    d2d_effect_image_AddRef,
    d2d_effect_image_Release,
    d2d_effect_image_GetFactory,
};

HRESULT d2d_effect_init(struct d2d_effect *effect, struct d2d_effect_context *effect_context, const CLSID *effect_id)
{
    unsigned int i;

    effect->ID2D1Effect_iface.lpVtbl = &d2d_effect_vtbl;
    effect->ID2D1Image_iface.lpVtbl = &d2d_effect_image_vtbl;
    effect->refcount = 1;

    for (i = 0; i < ARRAY_SIZE(builtin_effects); ++i)
    {
        if (IsEqualGUID(effect_id, builtin_effects[i].clsid))
        {
            effect->info = &builtin_effects[i];
            d2d_effect_SetInputCount(&effect->ID2D1Effect_iface, effect->info->default_input_count);
            effect->effect_context = effect_context;
            ID2D1EffectContext_AddRef(&effect_context->ID2D1EffectContext_iface);
            return S_OK;
        }
    }

    WARN("Unsupported effect clsid %s.\n", debugstr_guid(effect_id));
    return E_FAIL;
}
