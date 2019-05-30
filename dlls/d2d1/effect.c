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

static inline struct d2d_effect *impl_from_ID2D1Effect(ID2D1Effect *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_effect, ID2D1Effect_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_QueryInterface(ID2D1Effect *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1Effect)
            || IsEqualGUID(iid, &IID_ID2D1Properties)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1Effect_AddRef(iface);
        *out = iface;
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

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_effect_Release(ID2D1Effect *iface)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);
    ULONG refcount = InterlockedDecrement(&effect->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        heap_free(effect);

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
    FIXME("iface %p, index %u, type %#x, value %p, value_size %u stub!\n", iface, index, type,
            value, value_size);

    return E_NOTIMPL;
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
    FIXME("iface %p, index %u, input %p, invalidate %d stub!\n", iface, index, input, invalidate);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_SetInputCount(ID2D1Effect *iface, UINT32 count)
{
    FIXME("iface %p, count %u stub!\n", iface, count);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_effect_GetInput(ID2D1Effect *iface, UINT32 index, ID2D1Image **input)
{
    FIXME("iface %p, index %u, input %p stub!\n", iface, index, input);
}

static UINT32 STDMETHODCALLTYPE d2d_effect_GetInputCount(ID2D1Effect *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static void STDMETHODCALLTYPE d2d_effect_GetOutput(ID2D1Effect *iface, ID2D1Image **output)
{
    FIXME("iface %p, output %p stub!\n", iface, output);
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

void d2d_effect_init(struct d2d_effect *effect)
{
    effect->ID2D1Effect_iface.lpVtbl = &d2d_effect_vtbl;
    effect->refcount = 1;
}
