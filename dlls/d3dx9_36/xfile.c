/*
 * Copyright (C) 2012 Christian Costa
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

#include "wine/debug.h"

#include "d3dx9.h"
#include "d3dx9xof.h"
#undef MAKE_DDHRESULT
#include "dxfile.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

HRESULT error_dxfile_to_d3dxfile(HRESULT error)
{
    switch (error)
    {
        case DXFILEERR_BADFILETYPE:
            return D3DXFERR_BADFILETYPE;
        case DXFILEERR_BADFILEVERSION:
            return D3DXFERR_BADFILEVERSION;
        case DXFILEERR_BADFILEFLOATSIZE:
            return D3DXFERR_BADFILEFLOATSIZE;
        case DXFILEERR_PARSEERROR:
            return D3DXFERR_PARSEERROR;
        default:
            FIXME("Cannot map error %#x\n", error);
            return E_FAIL;
    }
}

typedef struct {
    ID3DXFile ID3DXFile_iface;
    LONG ref;
    IDirectXFile *dxfile;
} ID3DXFileImpl;


static inline ID3DXFileImpl* impl_from_ID3DXFile(ID3DXFile *iface)
{
    return CONTAINING_RECORD(iface, ID3DXFileImpl, ID3DXFile_iface);
}


/*** IUnknown methods ***/

static HRESULT WINAPI ID3DXFileImpl_QueryInterface(ID3DXFile *iface, REFIID riid, void **ret_iface)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXFile))
    {
        iface->lpVtbl->AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    WARN("(%p)->(%s, %p), not found\n", iface, debugstr_guid(riid), ret_iface);
    *ret_iface = NULL;
    return E_NOINTERFACE;
}


static ULONG WINAPI ID3DXFileImpl_AddRef(ID3DXFile *iface)
{
    ID3DXFileImpl *This = impl_from_ID3DXFile(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref %d\n", iface, ref);

    return ref;
}


static ULONG WINAPI ID3DXFileImpl_Release(ID3DXFile *iface)
{
    ID3DXFileImpl *This = impl_from_ID3DXFile(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref %d\n", iface, ref);

    if (!ref)
    {
        IDirectXFile_Release(This->dxfile);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}


/*** ID3DXFile methods ***/

static HRESULT WINAPI ID3DXFileImpl_CreateEnumObject(ID3DXFile *iface, const void *source, D3DXF_FILELOADOPTIONS options, ID3DXFileEnumObject **enum_object)
{
    FIXME("(%p)->(%p, %x, %p): stub\n", iface, source, options, enum_object);

    return E_NOTIMPL;
}


static HRESULT WINAPI ID3DXFileImpl_CreateSaveObject(ID3DXFile *iface, const void *data, D3DXF_FILESAVEOPTIONS options, D3DXF_FILEFORMAT format, ID3DXFileSaveObject **save_object)
{
    FIXME("(%p)->(%p, %x, %u, %p): stub\n", iface, data, options, format, save_object);

    return E_NOTIMPL;
}


static HRESULT WINAPI ID3DXFileImpl_RegisterTemplates(ID3DXFile *iface, const void *data, SIZE_T size)
{
    ID3DXFileImpl *This = impl_from_ID3DXFile(iface);
    HRESULT ret;

    TRACE("(%p)->(%p, %lu)\n", iface, data, size);

    ret = IDirectXFile_RegisterTemplates(This->dxfile, (void*)data, size);
    if (ret != DXFILE_OK)
    {
        WARN("Error %#x\n", ret);
        return error_dxfile_to_d3dxfile(ret);
    }

    return S_OK;
}


static HRESULT WINAPI ID3DXFileImpl_RegisterEnumTemplates(ID3DXFile *iface, ID3DXFileEnumObject *enum_object)
{
    FIXME("(%p)->(%p): stub\n", iface, enum_object);

    return E_NOTIMPL;
}


static const ID3DXFileVtbl ID3DXFile_Vtbl =
{
    ID3DXFileImpl_QueryInterface,
    ID3DXFileImpl_AddRef,
    ID3DXFileImpl_Release,
    ID3DXFileImpl_CreateEnumObject,
    ID3DXFileImpl_CreateSaveObject,
    ID3DXFileImpl_RegisterTemplates,
    ID3DXFileImpl_RegisterEnumTemplates
};

HRESULT WINAPI D3DXFileCreate(ID3DXFile **d3dxfile)
{
    ID3DXFileImpl *object;
    HRESULT ret;

    TRACE("(%p)\n", d3dxfile);

    if (!d3dxfile)
        return E_POINTER;

    *d3dxfile = NULL;

    object = HeapAlloc(GetProcessHeap(), 0, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    ret = DirectXFileCreate(&object->dxfile);
    if (ret != S_OK)
    {
        HeapFree(GetProcessHeap(), 0, object);
        if (ret == E_OUTOFMEMORY)
            return ret;
        return E_FAIL;
    }

    object->ID3DXFile_iface.lpVtbl = &ID3DXFile_Vtbl;
    object->ref = 1;

    *d3dxfile = &object->ID3DXFile_iface;

    return S_OK;
}
