/*
 * state block implementation
 *
 * Copyright 2002 Raphael Junqueira
 *           2004 Jason Edmeades   
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

HRESULT WINAPI IWineD3DStateBlockImpl_InitStartupStateBlock(IWineD3DStateBlock* iface) {
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;

    /* Note this may have a large overhead but it should only be executed
       once, in order to initialize the complete state of the device and 
       all opengl equivalents                                            */
    TRACE("-----------------------> Setting up device defaults...\n");
    This->blockType = D3DSBT_ALL;

    TRACE("-----------------------> Device defaults now set up...\n");
    return D3D_OK;
}

/**********************************************************
 * IUnknown parts follows
 **********************************************************/
HRESULT WINAPI IWineD3DStateBlockImpl_QueryInterface(IWineD3DStateBlock *iface,REFIID riid,LPVOID *ppobj)
{
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DStateBlockImpl_AddRef(IWineD3DStateBlock *iface) {
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI IWineD3DStateBlockImpl_Release(IWineD3DStateBlock *iface) {
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

/**********************************************************
 * IWineD3DStateBlock VTbl follows
 **********************************************************/

IWineD3DStateBlockVtbl IWineD3DStateBlock_Vtbl =
{
    IWineD3DStateBlockImpl_QueryInterface,
    IWineD3DStateBlockImpl_AddRef,
    IWineD3DStateBlockImpl_Release,
    IWineD3DStateBlockImpl_InitStartupStateBlock
};
