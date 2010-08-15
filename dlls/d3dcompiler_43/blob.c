/*
 * Direct3D blob file
 *
 * Copyright 2010 Rico Schüller
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

#include "d3dcompiler_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dcompiler);

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3dcompiler_blob_QueryInterface(ID3DBlob *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D10Blob)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3dcompiler_blob_AddRef(ID3DBlob *iface)
{
    struct d3dcompiler_blob *blob = (struct d3dcompiler_blob *)iface;
    ULONG refcount = InterlockedIncrement(&blob->refcount);

    TRACE("%p increasing refcount to %u\n", blob, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3dcompiler_blob_Release(ID3DBlob *iface)
{
    struct d3dcompiler_blob *blob = (struct d3dcompiler_blob *)iface;
    ULONG refcount = InterlockedDecrement(&blob->refcount);

    TRACE("%p decreasing refcount to %u\n", blob, refcount);

    if (!refcount)
    {
        HeapFree(GetProcessHeap(), 0, blob->data);
        HeapFree(GetProcessHeap(), 0, blob);
    }

    return refcount;
}

/* ID3DBlob methods */

static void * STDMETHODCALLTYPE d3dcompiler_blob_GetBufferPointer(ID3DBlob *iface)
{
    struct d3dcompiler_blob *blob = (struct d3dcompiler_blob *)iface;

    TRACE("iface %p\n", iface);

    return blob->data;
}

static SIZE_T STDMETHODCALLTYPE d3dcompiler_blob_GetBufferSize(ID3DBlob *iface)
{
    struct d3dcompiler_blob *blob = (struct d3dcompiler_blob *)iface;

    TRACE("iface %p\n", iface);

    return blob->size;
}

const struct ID3D10BlobVtbl d3dcompiler_blob_vtbl =
{
    /* IUnknown methods */
    d3dcompiler_blob_QueryInterface,
    d3dcompiler_blob_AddRef,
    d3dcompiler_blob_Release,
    /* ID3DBlob methods */
    d3dcompiler_blob_GetBufferPointer,
    d3dcompiler_blob_GetBufferSize,
};

HRESULT d3dcompiler_blob_init(struct d3dcompiler_blob *blob, SIZE_T data_size)
{
    blob->vtbl = &d3dcompiler_blob_vtbl;
    blob->refcount = 1;
    blob->size = data_size;

    blob->data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, data_size);
    if (!blob->data)
    {
        ERR("Failed to allocate D3D blob data memory\n");
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
