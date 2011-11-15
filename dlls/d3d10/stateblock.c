/*
 * Copyright 2011 Henri Verbeet for CodeWeavers
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

#include "config.h"
#include "wine/port.h"

#include "d3d10_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10);

struct d3d10_stateblock
{
    ID3D10StateBlock ID3D10StateBlock_iface;
    LONG refcount;
};

static inline struct d3d10_stateblock *impl_from_ID3D10StateBlock(ID3D10StateBlock *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_stateblock, ID3D10StateBlock_iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_stateblock_QueryInterface(ID3D10StateBlock *iface, REFIID iid, void **object)
{
    struct d3d10_stateblock *stateblock;

    TRACE("iface %p, iid %s, object %p.\n", iface, debugstr_guid(iid), object);

    stateblock = impl_from_ID3D10StateBlock(iface);

    if (IsEqualGUID(iid, &IID_ID3D10StateBlock)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        IUnknown_AddRef(&stateblock->ID3D10StateBlock_iface);
        *object = &stateblock->ID3D10StateBlock_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *object = NULL;

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d10_stateblock_AddRef(ID3D10StateBlock *iface)
{
    struct d3d10_stateblock *stateblock = impl_from_ID3D10StateBlock(iface);
    ULONG refcount = InterlockedIncrement(&stateblock->refcount);

    TRACE("%p increasing refcount to %u.\n", stateblock, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d10_stateblock_Release(ID3D10StateBlock *iface)
{
    struct d3d10_stateblock *stateblock = impl_from_ID3D10StateBlock(iface);
    ULONG refcount = InterlockedDecrement(&stateblock->refcount);

    TRACE("%p decreasing refcount to %u.\n", stateblock, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, stateblock);

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d3d10_stateblock_Capture(ID3D10StateBlock *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_stateblock_Apply(ID3D10StateBlock *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_stateblock_ReleaseAllDeviceObjects(ID3D10StateBlock *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_stateblock_GetDevice(ID3D10StateBlock *iface, ID3D10Device **device)
{
    FIXME("iface %p, device %p stub!\n", iface, device);

    return E_NOTIMPL;
}

static const struct ID3D10StateBlockVtbl d3d10_stateblock_vtbl =
{
    /* IUnknown methods */
    d3d10_stateblock_QueryInterface,
    d3d10_stateblock_AddRef,
    d3d10_stateblock_Release,
    /* ID3D10StateBlock methods */
    d3d10_stateblock_Capture,
    d3d10_stateblock_Apply,
    d3d10_stateblock_ReleaseAllDeviceObjects,
    d3d10_stateblock_GetDevice,
};

HRESULT WINAPI D3D10CreateStateBlock(ID3D10Device *device,
        D3D10_STATE_BLOCK_MASK *mask, ID3D10StateBlock **stateblock)
{
    struct d3d10_stateblock *object;

    FIXME("device %p, mask %p, stateblock %p stub!\n", device, mask, stateblock);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate D3D10 stateblock object memory.\n");
        return E_OUTOFMEMORY;
    }

    object->ID3D10StateBlock_iface.lpVtbl = &d3d10_stateblock_vtbl;
    object->refcount = 1;

    TRACE("Created stateblock %p.\n", object);
    *stateblock = &object->ID3D10StateBlock_iface;

    return S_OK;
}

HRESULT WINAPI D3D10StateBlockMaskDifference(D3D10_STATE_BLOCK_MASK *mask_x,
        D3D10_STATE_BLOCK_MASK *mask_y, D3D10_STATE_BLOCK_MASK *result)
{
    UINT count = sizeof(*result) / sizeof(DWORD);
    UINT i;

    TRACE("mask_x %p, mask_y %p, result %p.\n", mask_x, mask_y, result);

    if (!mask_x || !mask_y || !result)
        return E_INVALIDARG;

    for (i = 0; i < count; ++i)
    {
        ((DWORD *)result)[i] = ((DWORD *)mask_x)[i] ^ ((DWORD *)mask_y)[i];
    }
    for (i = count * sizeof(DWORD); i < sizeof(*result); ++i)
    {
        ((BYTE *)result)[i] = ((BYTE *)mask_x)[i] ^ ((BYTE *)mask_y)[i];
    }

    return S_OK;
}

HRESULT WINAPI D3D10StateBlockMaskDisableAll(D3D10_STATE_BLOCK_MASK *mask)
{
    TRACE("mask %p.\n", mask);

    if (!mask)
        return E_INVALIDARG;

    memset(mask, 0, sizeof(*mask));

    return S_OK;
}
