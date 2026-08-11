/*
 * Copyright 2021 Nikolay Sivov for CodeWeavers
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

#define COBJMACROS
#include "d3dx10.h"
#include <stdbool.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

#define D3DERR_INVALIDCALL 0x8876086c
#define D3DX10_SPRITE_READY 0x10000000

struct d3dx10_sprite
{
    ID3DX10Sprite ID3DX10Sprite_iface;
    LONG refcount;

    struct
    {
        D3DX10_SPRITE *sprites;
        size_t count;
        size_t capacity;
    } buffer;
    D3DXMATRIX projection;
    ID3D10Device *device;
    ID3D10StateBlock *state_block;
    unsigned int flags;
};

static bool d3dx_array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    size_t new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return true;

    max_capacity = ~(size_t)0 / size;
    if (count > max_capacity)
        return false;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return false;

    *elements = new_elements;
    *capacity = new_capacity;
    return true;
}

static void d3dx10_sprite_clear_batch(struct d3dx10_sprite *sprite)
{
    if (sprite->flags & D3DX10_SPRITE_ADDREF_TEXTURES)
    {
        for (size_t i = 0; i < sprite->buffer.count; ++i)
        {
            if (sprite->buffer.sprites[i].pTexture)
                ID3D10ShaderResourceView_Release(sprite->buffer.sprites[i].pTexture);
        }
    }

    sprite->buffer.count = 0;
}

static void d3dx10_sprite_flush(struct d3dx10_sprite *sprite)
{
    /* TODO: draw batched sprites */
    d3dx10_sprite_clear_batch(sprite);
}

static inline struct d3dx10_sprite *impl_from_ID3DX10Sprite(ID3DX10Sprite *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx10_sprite, ID3DX10Sprite_iface);
}

static HRESULT WINAPI d3dx10_sprite_QueryInterface(ID3DX10Sprite *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DX10Sprite)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx10_sprite_AddRef(ID3DX10Sprite *iface)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);
    ULONG refcount = InterlockedIncrement(&sprite->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3dx10_sprite_Release(ID3DX10Sprite *iface)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);
    ULONG refcount = InterlockedDecrement(&sprite->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        ID3D10Device_Release(sprite->device);
        if (sprite->state_block)
            IUnknown_Release(sprite->state_block);
        d3dx10_sprite_clear_batch(sprite);
        free(sprite->buffer.sprites);
        free(sprite);
    }

    return refcount;
}

static HRESULT WINAPI d3dx10_sprite_Begin(ID3DX10Sprite *iface, UINT flags)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    if (sprite->flags & D3DX10_SPRITE_READY)
        return E_FAIL;

    sprite->flags = flags | D3DX10_SPRITE_READY;
    if (sprite->flags & D3DX10_SPRITE_SAVE_STATE)
        sprite->state_block->lpVtbl->Capture(sprite->state_block);

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_DrawSpritesBuffered(ID3DX10Sprite *iface,
        D3DX10_SPRITE *sprites, UINT count)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, sprites %p, count %u.\n", iface, sprites, count);

    if (!(sprite->flags & D3DX10_SPRITE_READY))
        return E_FAIL;

    if (!d3dx_array_reserve((void **)&sprite->buffer.sprites, &sprite->buffer.capacity,
            sprite->buffer.count + count, sizeof(*sprite->buffer.sprites)))
    {
        return E_OUTOFMEMORY;
    }

    for (unsigned int i = 0; i < count; ++i)
    {
        sprite->buffer.sprites[sprite->buffer.count++] = sprites[i];
        if (sprite->flags & D3DX10_SPRITE_ADDREF_TEXTURES)
            ID3D10ShaderResourceView_AddRef(sprites[i].pTexture);
    }

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_Flush(ID3DX10Sprite *iface)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    FIXME("iface %p stub!\n", iface);

    if (!(sprite->flags & D3DX10_SPRITE_READY))
        return E_FAIL;

    d3dx10_sprite_flush(sprite);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx10_sprite_DrawSpritesImmediate(ID3DX10Sprite *iface,
        D3DX10_SPRITE *sprites, UINT count, UINT size, UINT flags)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    FIXME("iface %p, sprites %p, count %u, size %u, flags %#x stub!\n",
            iface, sprites, count, size, flags);

    if (!(sprite->flags & D3DX10_SPRITE_READY))
        return E_FAIL;

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx10_sprite_End(ID3DX10Sprite *iface)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    FIXME("iface %p stub!\n", iface);

    if (!(sprite->flags & D3DX10_SPRITE_READY))
        return E_FAIL;

    d3dx10_sprite_flush(sprite);

    if (sprite->flags & D3DX10_SPRITE_SAVE_STATE)
        sprite->state_block->lpVtbl->Apply(sprite->state_block);
    sprite->flags = 0;

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_GetViewTransform(ID3DX10Sprite *iface, D3DXMATRIX *transform)
{
    FIXME("iface %p, transform %p stub!\n", iface, transform);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx10_sprite_SetViewTransform(ID3DX10Sprite *iface, D3DXMATRIX *transform)
{
    FIXME("iface %p, transform %p stub!\n", iface, transform);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx10_sprite_GetProjectionTransform(ID3DX10Sprite *iface,
        D3DXMATRIX *transform)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    if (!transform)
        return E_FAIL;

    *transform = sprite->projection;

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_SetProjectionTransform(ID3DX10Sprite *iface, D3DXMATRIX *transform)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    if (!transform)
        return E_FAIL;

    sprite->projection = *transform;

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_GetDevice(ID3DX10Sprite *iface, ID3D10Device **device)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    if (!device)
        return E_FAIL;

    *device = sprite->device;
    ID3D10Device_AddRef(*device);

    return S_OK;
}

static const ID3DX10SpriteVtbl d3dx10_sprite_vtbl =
{
    d3dx10_sprite_QueryInterface,
    d3dx10_sprite_AddRef,
    d3dx10_sprite_Release,
    d3dx10_sprite_Begin,
    d3dx10_sprite_DrawSpritesBuffered,
    d3dx10_sprite_Flush,
    d3dx10_sprite_DrawSpritesImmediate,
    d3dx10_sprite_End,
    d3dx10_sprite_GetViewTransform,
    d3dx10_sprite_SetViewTransform,
    d3dx10_sprite_GetProjectionTransform,
    d3dx10_sprite_SetProjectionTransform,
    d3dx10_sprite_GetDevice,
};

HRESULT WINAPI D3DX10CreateSprite(ID3D10Device *device, UINT size, ID3DX10Sprite **sprite)
{
    struct d3dx10_sprite *object;
    D3D10_STATE_BLOCK_MASK mask;
    HRESULT hr;

    TRACE("device %p, size %u, sprite %p.\n", device, size, sprite);

    if (!device || !sprite)
        return D3DERR_INVALIDCALL;

    *sprite = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID3DX10Sprite_iface.lpVtbl = &d3dx10_sprite_vtbl;
    object->refcount = 1;
    object->device = device;
    ID3D10Device_AddRef(device);
    object->projection._11 = 1.0f;
    object->projection._22 = 1.0f;
    object->projection._33 = 1.0f;
    object->projection._44 = 1.0f;

    /* TODO: we shouldn't be capturing entire state */
    D3D10StateBlockMaskEnableAll(&mask);
    if (FAILED(hr = D3D10CreateStateBlock(device, &mask, &object->state_block)))
    {
        ID3DX10Sprite_Release(&object->ID3DX10Sprite_iface);
        return hr;
    }

    *sprite = &object->ID3DX10Sprite_iface;

    return S_OK;
}
