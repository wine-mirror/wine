/*
 * IWineD3D implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);

UINT     WINAPI  IWineD3DImpl_GetAdapterCount (IWineD3D *iface) {
    IWineD3DImpl *This = (IWineD3DImpl *)iface;

    /* FIXME: Set to one for now to imply the display */
    TRACE_(d3d_caps)("(%p): Mostly stub, only returns primary display\n", This);
    return 1;
}


/* IUnknown parts follow: */
HRESULT WINAPI IWineD3DImpl_QueryInterface(IWineD3D *iface,REFIID riid,LPVOID *ppobj)
{
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DImpl_AddRef(IWineD3D *iface) {
    return 1;
}

ULONG WINAPI IWineD3DImpl_Release(IWineD3D *iface) {
    return 0;
}

/* VTbl definition */
IWineD3DVtbl IWineD3D_Vtbl =
{
    IWineD3DImpl_QueryInterface,
    IWineD3DImpl_AddRef,
    IWineD3DImpl_Release,
    IWineD3DImpl_GetAdapterCount
};
