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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "config.h"
#include "winerror.h"
#include "wine/debug.h"

#include <assert.h>
#include <string.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

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

HRESULT WINAPI IWineD3DPaletteImpl_GetEntries(IWineD3DPalette *iface, DWORD Flags, DWORD Start, DWORD Count, PALETTEENTRY *PalEnt) {
    FIXME("This is unimplemented for now(d3d7 merge)\n");
    return DDERR_INVALIDPARAMS;
}

HRESULT WINAPI IWineD3DPaletteImpl_SetEntries(IWineD3DPalette *iface, DWORD Flags, DWORD Start, DWORD Count, PALETTEENTRY *PalEnt) {
    FIXME("This is unimplemented for now(d3d7 merge)\n");
    return DDERR_INVALIDPARAMS;
}

HRESULT WINAPI IWineD3DPaletteImpl_GetCaps(IWineD3DPalette *iface, DWORD *Caps) {
    FIXME("This is unimplemented for now(d3d7 merge)\n");
    return DDERR_INVALIDPARAMS;
}

HRESULT WINAPI IWineD3DPaletteImpl_GetParent(IWineD3DPalette *iface, IUnknown **Parent) {
    FIXME("This is unimplemented for now(d3d7 merge)\n");
    return DDERR_INVALIDPARAMS;
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
