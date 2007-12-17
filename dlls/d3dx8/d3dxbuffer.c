/*
 * ID3DXBuffer implementation
 *
 * Copyright 2002 Raphael Junqueira
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
#include "wine/port.h"

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d8.h"
#include "d3dx8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* ID3DXBuffer IUnknown parts follow: */
static HRESULT WINAPI ID3DXBufferImpl_QueryInterface(LPD3DXBUFFER iface, REFIID riid, LPVOID* ppobj) {
  ID3DXBufferImpl *This = (ID3DXBufferImpl *)iface;
  
  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_ID3DXBuffer)) {
    IUnknown_AddRef(iface);
    *ppobj = This;
    return D3D_OK;
  }

  WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
  return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXBufferImpl_AddRef(LPD3DXBUFFER iface) {
  ID3DXBufferImpl *This = (ID3DXBufferImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p) : AddRef from %d\n", This, ref - 1);

  return ref;
}

static ULONG WINAPI ID3DXBufferImpl_Release(LPD3DXBUFFER iface) {
  ID3DXBufferImpl *This = (ID3DXBufferImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p) : ReleaseRef to %d\n", This, ref);

  if (ref == 0) {
    HeapFree(GetProcessHeap(), 0, This->buffer);
    HeapFree(GetProcessHeap(), 0, This);
  }
  return ref;
}

/* ID3DXBuffer Interface follow: */
static LPVOID WINAPI ID3DXBufferImpl_GetBufferPointer(LPD3DXBUFFER iface) {
  ID3DXBufferImpl *This = (ID3DXBufferImpl *)iface;
  return This->buffer;
}

static DWORD WINAPI ID3DXBufferImpl_GetBufferSize(LPD3DXBUFFER iface) {
  ID3DXBufferImpl *This = (ID3DXBufferImpl *)iface;
  return This->bufferSize;
}

const ID3DXBufferVtbl D3DXBuffer_Vtbl =
{
    ID3DXBufferImpl_QueryInterface,
    ID3DXBufferImpl_AddRef,
    ID3DXBufferImpl_Release,
    ID3DXBufferImpl_GetBufferPointer,
    ID3DXBufferImpl_GetBufferSize
};
