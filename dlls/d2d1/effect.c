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
};

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
    ID2D1Factory_Release(effect->factory);
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

    ID2D1Factory_AddRef(*factory = effect->factory);
}

static const ID2D1ImageVtbl d2d_effect_image_vtbl =
{
    d2d_effect_image_QueryInterface,
    d2d_effect_image_AddRef,
    d2d_effect_image_Release,
    d2d_effect_image_GetFactory,
};

HRESULT d2d_effect_init(struct d2d_effect *effect, ID2D1Factory *factory, const CLSID *effect_id)
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
            ID2D1Factory_AddRef(effect->factory = factory);
            return S_OK;
        }
    }

    WARN("Unsupported effect clsid %s.\n", debugstr_guid(effect_id));
    return E_FAIL;
}
