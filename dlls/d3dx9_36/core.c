/*
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

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct ID3DXBufferImpl
{
    ID3DXBuffer ID3DXBuffer_iface;
    LONG ref;

    void *buffer;
    DWORD size;
};

static inline struct ID3DXBufferImpl *impl_from_ID3DXBuffer(ID3DXBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct ID3DXBufferImpl, ID3DXBuffer_iface);
}

static HRESULT WINAPI ID3DXBufferImpl_QueryInterface(ID3DXBuffer *iface, REFIID riid, void **ppobj)
{
    struct ID3DXBufferImpl *This = impl_from_ID3DXBuffer(iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_ID3DXBuffer))
    {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXBufferImpl_AddRef(ID3DXBuffer *iface)
{
    struct ID3DXBufferImpl *This = impl_from_ID3DXBuffer(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI ID3DXBufferImpl_Release(ID3DXBuffer *iface)
{
    struct ID3DXBufferImpl *This = impl_from_ID3DXBuffer(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This->buffer);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static LPVOID WINAPI ID3DXBufferImpl_GetBufferPointer(ID3DXBuffer *iface)
{
    struct ID3DXBufferImpl *This = impl_from_ID3DXBuffer(iface);
    return This->buffer;
}

static DWORD WINAPI ID3DXBufferImpl_GetBufferSize(ID3DXBuffer *iface)
{
    struct ID3DXBufferImpl *This = impl_from_ID3DXBuffer(iface);
    return This->size;
}

static const struct ID3DXBufferVtbl ID3DXBufferImpl_Vtbl =
{
    /* IUnknown methods */
    ID3DXBufferImpl_QueryInterface,
    ID3DXBufferImpl_AddRef,
    ID3DXBufferImpl_Release,
    /* ID3DXBuffer methods */
    ID3DXBufferImpl_GetBufferPointer,
    ID3DXBufferImpl_GetBufferSize
};

HRESULT WINAPI D3DXCreateBuffer(DWORD size, LPD3DXBUFFER *buffer)
{
    struct ID3DXBufferImpl *object;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (object == NULL)
    {
        *buffer = NULL;
        return E_OUTOFMEMORY;
    }
    object->ID3DXBuffer_iface.lpVtbl = &ID3DXBufferImpl_Vtbl;
    object->ref = 1;
    object->size = size;
    object->buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (object->buffer == NULL)
    {
        HeapFree(GetProcessHeap(), 0, object);
        *buffer = NULL;
        return E_OUTOFMEMORY;
    }

    *buffer = &object->ID3DXBuffer_iface;
    return D3D_OK;
}
