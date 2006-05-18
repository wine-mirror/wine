/*		DirectDraw - IDirectPalette base interface
 *
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2006 Stefan Dösinger for CodeWeavers
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
#include "winerror.h"
#include "wine/debug.h"

#include <assert.h>
#include <string.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

#define SIZE_BITS (DDPCAPS_1BIT | DDPCAPS_2BIT | DDPCAPS_4BIT | DDPCAPS_8BIT)

HRESULT WINAPI IWineD3DPaletteImpl_QueryInterface(IWineD3DPalette *iface, REFIID refiid, void **obj) {
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);

    if (IsEqualGUID(refiid, &IID_IUnknown)
        || IsEqualGUID(refiid, &IID_IWineD3DPalette)) {
        *obj = iface;
        IWineD3DPalette_AddRef(iface);
        return S_OK;
    }
    else {
        *obj = NULL;
        return E_NOINTERFACE;
    }
}

ULONG WINAPI IWineD3DPaletteImpl_AddRef(IWineD3DPalette *iface) {
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %lu.\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IWineD3DPaletteImpl_Release(IWineD3DPalette *iface) {
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %lu.\n", This, ref + 1);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
        return 0;
    }

    return ref;
}

/* Not called from the vtable */
DWORD IWineD3DPaletteImpl_Size(DWORD dwFlags) {
    switch (dwFlags & SIZE_BITS) {
        case DDPCAPS_1BIT: return 2;
        case DDPCAPS_2BIT: return 4;
        case DDPCAPS_4BIT: return 16;
        case DDPCAPS_8BIT: return 256;
        default: assert(0); return 256;
    }
}

HRESULT WINAPI IWineD3DPaletteImpl_GetEntries(IWineD3DPalette *iface, DWORD Flags, DWORD Start, DWORD Count, PALETTEENTRY *PalEnt) {
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;

    TRACE("(%p)->(%08lx,%ld,%ld,%p)\n",This,Flags,Start,Count,PalEnt);

    if (Flags != 0) return WINED3DERR_INVALIDCALL; /* unchecked */
    if (Start + Count > IWineD3DPaletteImpl_Size(This->Flags))
        return WINED3DERR_INVALIDCALL;

    if (This->Flags & DDPCAPS_8BITENTRIES)
    {
        unsigned int i;
        LPBYTE entry = (LPBYTE)PalEnt;

        for (i=Start; i < Count+Start; i++)
            *entry++ = This->palents[i].peRed;
    }
    else
        memcpy(PalEnt, This->palents+Start, Count * sizeof(PALETTEENTRY));

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DPaletteImpl_SetEntries(IWineD3DPalette *iface, DWORD Flags, DWORD Start, DWORD Count, PALETTEENTRY *PalEnt)
{
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;
    ResourceList *res;

    TRACE("(%p)->(%08lx,%ld,%ld,%p)\n",This,Flags,Start,Count,PalEnt);

    if (This->Flags & DDPCAPS_8BITENTRIES) {
        unsigned int i;
        const BYTE* entry = (const BYTE*)PalEnt;

        for (i=Start; i < Count+Start; i++)
            This->palents[i].peRed = *entry++;
    }
    else {
        memcpy(This->palents+Start, PalEnt, Count * sizeof(PALETTEENTRY));

        if (This->hpal)
            SetPaletteEntries(This->hpal, Start, Count, This->palents+Start);
    }

#if 0
    /* Now, if we are in 'depth conversion mode', update the screen palette */
    /* FIXME: we need to update the image or we won't get palette fading. */
    if (This->ddraw->d->palette_convert != NULL)
        This->ddraw->d->palette_convert(palent,This->screen_palents,start,count);
#endif

    /* If the palette is attached to the render target, update all render targets */

    for(res = This->wineD3DDevice->resources; res != NULL; res=res->next) {
        if(IWineD3DResource_GetType(res->resource) == D3DRTYPE_SURFACE) {
            IWineD3DSurfaceImpl *impl = (IWineD3DSurfaceImpl *) res->resource;
            if(impl->palette == This)
                IWineD3DSurface_RealizePalette( (IWineD3DSurface *) res->resource);
        }
    }

    /* If the palette is the primary palette, set the entries to the device */
    if(This->Flags & DDPCAPS_PRIMARYSURFACE) {
        unsigned int i;
        IWineD3DDeviceImpl *device = This->wineD3DDevice;
        PALETTEENTRY *entry = PalEnt;

        for(i = Start; i < Start+Count; i++) {
            device->palettes[device->currentPalette][i] = *entry++;
        }
    }

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DPaletteImpl_GetCaps(IWineD3DPalette *iface, DWORD *Caps) {
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;
    TRACE("(%p)->(%p)\n", This, Caps);

    *Caps = This->Flags;
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DPaletteImpl_GetParent(IWineD3DPalette *iface, IUnknown **Parent) {
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;
    TRACE("(%p)->(%p)\n", This, Parent);

    *Parent = (IUnknown *) This->parent;
    IUnknown_AddRef( (IUnknown *) This->parent);
    return WINED3D_OK;
}

const IWineD3DPaletteVtbl IWineD3DPalette_Vtbl =
{
    /*** IUnknown ***/
    IWineD3DPaletteImpl_QueryInterface,
    IWineD3DPaletteImpl_AddRef,
    IWineD3DPaletteImpl_Release,
    /*** IWineD3DPalette ***/
    IWineD3DPaletteImpl_GetParent,
    IWineD3DPaletteImpl_GetEntries,
    IWineD3DPaletteImpl_GetCaps,
    IWineD3DPaletteImpl_SetEntries
};
