/*
 * Copyright 2025 Zhiyi Zhang for CodeWeavers
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
#define COBJMACROS
#include "wine/debug.h"
#include "d3dcompiler_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dcompiler);

struct d3d11_linker
{
    ID3D11Linker ID3D11Linker_iface;
    LONG refcount;
};

static inline struct d3d11_linker *impl_from_ID3D11Linker(ID3D11Linker *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_linker, ID3D11Linker_iface);
}

static HRESULT WINAPI d3d11_linker_QueryInterface(ID3D11Linker *iface, REFIID riid, void **object)
{
    struct d3d11_linker *linker = impl_from_ID3D11Linker(iface);

    TRACE("linker %p, riid %s, object %p.\n", linker, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11Linker)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d11_linker_AddRef(ID3D11Linker *iface)
{
    struct d3d11_linker *linker = impl_from_ID3D11Linker(iface);
    ULONG refcount = InterlockedIncrement(&linker->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3d11_linker_Release(ID3D11Linker *iface)
{
    struct d3d11_linker *linker = impl_from_ID3D11Linker(iface);
    ULONG refcount = InterlockedDecrement(&linker->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        free(linker);

    return refcount;
}

static HRESULT WINAPI d3d11_linker_Link(ID3D11Linker *iface, ID3D11ModuleInstance *instance,
        LPCSTR instance_name, LPCSTR target_name, UINT flags, ID3DBlob **shader, ID3DBlob **error)
{
    FIXME("iface %p, instance %p, instance_name %s, target_name %s, flags %#x, shader %p, error %p stub!\n",
            iface, instance, wine_dbgstr_a(instance_name), wine_dbgstr_a(target_name), flags, shader, error);
    return E_NOTIMPL;
}

static HRESULT WINAPI d3d11_linker_UseLibrary(ID3D11Linker *iface, ID3D11ModuleInstance *instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static HRESULT WINAPI d3d11_linker_AddClipPlaneFromCBuffer(ID3D11Linker *iface, UINT buffer_slot,
        UINT buffer_entry)
{
    FIXME("iface %p, buffer_slot %u, buffer_entry %u stub!\n", iface, buffer_slot, buffer_entry);
    return E_NOTIMPL;
}

static const struct ID3D11LinkerVtbl d3d11_linker_vtbl =
{
    d3d11_linker_QueryInterface,
    d3d11_linker_AddRef,
    d3d11_linker_Release,
    d3d11_linker_Link,
    d3d11_linker_UseLibrary,
    d3d11_linker_AddClipPlaneFromCBuffer,
};

HRESULT WINAPI D3DCreateLinker(ID3D11Linker **linker)
{
    struct d3d11_linker *object;

    TRACE("linker %p.\n", linker);

    if (!linker)
        return E_INVALIDARG;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID3D11Linker_iface.lpVtbl = &d3d11_linker_vtbl;
    object->refcount = 1;

    *linker = &object->ID3D11Linker_iface;
    return S_OK;
}
