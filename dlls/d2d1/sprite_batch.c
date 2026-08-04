/*
 * Copyright 2026 Santino Mazza for CodeWeavers
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

static inline struct d2d_sprite_batch *impl_from_ID2D1SpriteBatch(ID2D1SpriteBatch *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_sprite_batch, ID2D1SpriteBatch_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_sprite_batch_QueryInterface(ID2D1SpriteBatch *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1SpriteBatch)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1SpriteBatch_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_sprite_batch_AddRef(ID2D1SpriteBatch *iface)
{
    struct d2d_sprite_batch *batch = impl_from_ID2D1SpriteBatch(iface);
    ULONG refcount = InterlockedIncrement(&batch->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_sprite_batch_Release(ID2D1SpriteBatch *iface)
{
    struct d2d_sprite_batch *batch = impl_from_ID2D1SpriteBatch(iface);
    ULONG refcount = InterlockedDecrement(&batch->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        ID2D1Factory_Release(batch->factory);
        free(batch->sprites);
        free(batch);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_sprite_batch_GetFactory(ID2D1SpriteBatch *iface, ID2D1Factory **factory)
{
    struct d2d_sprite_batch *batch = impl_from_ID2D1SpriteBatch(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    *factory = batch->factory;
    ID2D1Factory_AddRef(*factory);
}

static HRESULT STDMETHODCALLTYPE d2d_sprite_batch_AddSprites(ID2D1SpriteBatch *iface,
        UINT32 count, const D2D1_RECT_F *dests, const D2D1_RECT_U *sources, const D2D1_COLOR_F *colors,
        const D2D1_MATRIX_3X2_F *transforms, UINT32 dests_stride, UINT32 sources_stride,
        UINT32 colors_stride, UINT32 transforms_stride)
{
    struct d2d_sprite_batch *batch = impl_from_ID2D1SpriteBatch(iface);

    TRACE("iface %p, count %u, dests %p, sources %p, colors %p, transforms %p, dests_stride %u, "
              "sources_stride %u, colors_stride %u, transforms_stride %u.\n",
          iface, count, dests, sources, colors, transforms, dests_stride, sources_stride,
          colors_stride, transforms_stride);

    if (!dests)
        return S_OK;

    if (!d2d_array_reserve((void **)&batch->sprites, &batch->sprites_size, batch->sprite_count + count,
            sizeof(*batch->sprites)))
    {
        return E_OUTOFMEMORY;
    }

    for (unsigned int i = batch->sprite_count; i < batch->sprite_count + count; ++i)
    {
        struct d2d_sprite *sprite = &batch->sprites[i];

        sprite->dest = *dests;
        dests = (const D2D1_RECT_F *)((BYTE *)dests + dests_stride);

        if (sources)
        {
            sprite->source = *sources;
            sources = (const D2D1_RECT_U *)((BYTE *)sources + sources_stride);
        }
        else
        {
            sprite->source = (D2D1_RECT_U){ 0, 0, UINT_MAX, UINT_MAX };
        }

        if (colors)
        {
            sprite->color = *colors;
            colors = (const D2D1_COLOR_F *)((BYTE *)colors + colors_stride);
        }
        else
        {
            sprite->color = (D2D1_COLOR_F){ 1.0f, 1.0f, 1.0f, 1.0f };
        }

        if (transforms)
        {
            sprite->transform = *transforms;
            transforms = (const D2D1_MATRIX_3X2_F *)((BYTE *)transforms + transforms_stride);
        }
        else
        {
            memset(&sprite->transform, 0, sizeof(sprite->transform));
            sprite->transform._11 = sprite->transform._22 = 1.0f;
        }
    }

    batch->sprite_count += count;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_sprite_batch_SetSprites(ID2D1SpriteBatch *iface, UINT32 start,
        UINT32 count, const D2D1_RECT_F *dests, const D2D1_RECT_U *sources, const D2D1_COLOR_F *colors,
        const D2D1_MATRIX_3X2_F *transforms, UINT32 dests_stride, UINT32 sources_stride,
        UINT32 colors_stride, UINT32 transforms_stride)
{
    struct d2d_sprite_batch *batch = impl_from_ID2D1SpriteBatch(iface);

    TRACE("iface %p, start %u, count %u, dests %p, sources %p, colors %p, transforms %p, "
            "dests_stride %u, sources_stride %u, colors_stride %u, transforms_stride %u.\n",
            iface, start, count, dests, sources, colors, transforms, dests_stride,
            sources_stride, colors_stride, transforms_stride);

    if (start >= batch->sprite_count || count > batch->sprite_count - start)
        return E_INVALIDARG;

    for (unsigned int i = start; i < start + count; ++i)
    {
        struct d2d_sprite *sprite = &batch->sprites[i];

        if (dests)
        {
            sprite->dest = *dests;
            dests = (const D2D1_RECT_F *)((BYTE *)dests + dests_stride);
        }

        if (sources)
        {
            sprite->source = *sources;
            sources = (const D2D1_RECT_U *)((BYTE *)sources + sources_stride);
        }

        if (colors)
        {
            sprite->color = *colors;
            colors = (const D2D1_COLOR_F *)((BYTE *)colors + colors_stride);
        }

        if (transforms)
        {
            sprite->transform = *transforms;
            transforms = (const D2D1_MATRIX_3X2_F *)((BYTE *)transforms + transforms_stride);
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_sprite_batch_GetSprites(ID2D1SpriteBatch *iface,
        UINT32 start, UINT32 count, D2D1_RECT_F *dests, D2D1_RECT_U *sources, D2D1_COLOR_F *colors,
        D2D1_MATRIX_3X2_F *transforms)
{
    struct d2d_sprite_batch *batch = impl_from_ID2D1SpriteBatch(iface);

    TRACE("iface %p, start %u, count %u, dests %p, sources %p, colors %p, transforms %p.\n",
            iface, start, count, dests, sources, colors, transforms);

    if (!count)
        return S_OK;

    if (start >= batch->sprite_count || count > batch->sprite_count - start)
        return E_INVALIDARG;

    for (unsigned int i = 0; i < count; ++i)
    {
        struct d2d_sprite *sprite = &batch->sprites[i + start];

        if (dests)
            dests[i] = sprite->dest;

        if (sources)
            sources[i] = sprite->source;

        if (colors)
            colors[i] = sprite->color;

        if (transforms)
            transforms[i] = sprite->transform;
    }

    return S_OK;
}

static UINT32 STDMETHODCALLTYPE d2d_sprite_batch_GetSpritesCount(ID2D1SpriteBatch *iface)
{
    struct d2d_sprite_batch *batch = impl_from_ID2D1SpriteBatch(iface);

    TRACE("iface %p.\n", iface);

    return batch->sprite_count;
}

static void STDMETHODCALLTYPE d2d_sprite_batch_Clear(ID2D1SpriteBatch *iface)
{
    struct d2d_sprite_batch *batch = impl_from_ID2D1SpriteBatch(iface);

    TRACE("iface %p.\n", iface);

    batch->sprite_count = 0;
}

static const struct ID2D1SpriteBatchVtbl d2d_sprite_batch_vtbl =
{
    d2d_sprite_batch_QueryInterface,
    d2d_sprite_batch_AddRef,
    d2d_sprite_batch_Release,
    d2d_sprite_batch_GetFactory,
    d2d_sprite_batch_AddSprites,
    d2d_sprite_batch_SetSprites,
    d2d_sprite_batch_GetSprites,
    d2d_sprite_batch_GetSpritesCount,
    d2d_sprite_batch_Clear,
};

HRESULT d2d_sprite_batch_create(ID2D1Factory *factory, struct d2d_sprite_batch **batch)
{
    if (!(*batch = calloc(1, sizeof(**batch))))
        return E_OUTOFMEMORY;

    (*batch)->ID2D1SpriteBatch_iface.lpVtbl = &d2d_sprite_batch_vtbl;
    (*batch)->refcount = 1;
    ID2D1Factory_AddRef((*batch)->factory = factory);

    TRACE("Created sprite batch %p.\n", *batch);

    return S_OK;
}
