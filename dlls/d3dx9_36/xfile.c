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

static HRESULT error_dxfile_to_d3dxfile(HRESULT error)
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

typedef struct {
    ID3DXFileEnumObject ID3DXFileEnumObject_iface;
    LONG ref;
    ULONG nb_children;
    ID3DXFileData **children;
} ID3DXFileEnumObjectImpl;

typedef struct {
    ID3DXFileData ID3DXFileData_iface;
    LONG ref;
    IDirectXFileData *dxfile_data;
} ID3DXFileDataImpl;


static inline ID3DXFileImpl* impl_from_ID3DXFile(ID3DXFile *iface)
{
    return CONTAINING_RECORD(iface, ID3DXFileImpl, ID3DXFile_iface);
}

static inline ID3DXFileEnumObjectImpl* impl_from_ID3DXFileEnumObject(ID3DXFileEnumObject *iface)
{
    return CONTAINING_RECORD(iface, ID3DXFileEnumObjectImpl, ID3DXFileEnumObject_iface);
}

static inline ID3DXFileDataImpl* impl_from_ID3DXFileData(ID3DXFileData *iface)
{
    return CONTAINING_RECORD(iface, ID3DXFileDataImpl, ID3DXFileData_iface);
}

/*** IUnknown methods ***/

static HRESULT WINAPI ID3DXFileDataImpl_QueryInterface(ID3DXFileData *iface, REFIID riid, void **ret_iface)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXFileData))
    {
        iface->lpVtbl->AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    WARN("(%p)->(%s, %p), not found\n", iface, debugstr_guid(riid), ret_iface);
    *ret_iface = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXFileDataImpl_AddRef(ID3DXFileData *iface)
{
    ID3DXFileDataImpl *This = impl_from_ID3DXFileData(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI ID3DXFileDataImpl_Release(ID3DXFileData *iface)
{
    ID3DXFileDataImpl *This = impl_from_ID3DXFileData(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
    {
        IDirectXFileData_Release(This->dxfile_data);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}


/*** ID3DXFileData methods ***/

static HRESULT WINAPI ID3DXFileDataImpl_GetEnum(ID3DXFileData *iface, ID3DXFileEnumObject **enum_object)
{
    FIXME("(%p)->(%p): stub\n", iface, enum_object);

    return E_NOTIMPL;
}


static HRESULT WINAPI ID3DXFileDataImpl_GetName(ID3DXFileData *iface, char *name, SIZE_T *size)
{
    ID3DXFileDataImpl *This = impl_from_ID3DXFileData(iface);
    DWORD dxfile_size;
    HRESULT ret;

    TRACE("(%p)->(%p, %p)\n", iface, name, size);

    if (!name || !size)
        return E_POINTER;

    dxfile_size = *size;

    ret = IDirectXFileData_GetName(This->dxfile_data, name, &dxfile_size);
    if (ret != DXFILE_OK)
        return error_dxfile_to_d3dxfile(ret);

    *size = dxfile_size;

    return S_OK;
}


static HRESULT WINAPI ID3DXFileDataImpl_GetId(ID3DXFileData *iface, GUID *guid)
{
    ID3DXFileDataImpl *This = impl_from_ID3DXFileData(iface);
    HRESULT ret;

    TRACE("(%p)->(%p)\n", iface, guid);

    if (!guid)
        return E_POINTER;

    ret = IDirectXFileData_GetId(This->dxfile_data, guid);
    if (ret != DXFILE_OK)
        return error_dxfile_to_d3dxfile(ret);

    return S_OK;
}


static HRESULT WINAPI ID3DXFileDataImpl_Lock(ID3DXFileData *iface, SIZE_T *size, const void **data)
{
    ID3DXFileDataImpl *This = impl_from_ID3DXFileData(iface);
    DWORD dxfile_size;
    HRESULT ret;

    TRACE("(%p)->(%p, %p)\n", iface, size, data);

    if (!size || !data)
        return E_POINTER;

    ret = IDirectXFileData_GetData(This->dxfile_data, NULL, &dxfile_size, (void**)data);
    if (ret != DXFILE_OK)
        return error_dxfile_to_d3dxfile(ret);

    *size = dxfile_size;

    return S_OK;
}


static HRESULT WINAPI ID3DXFileDataImpl_Unlock(ID3DXFileData *iface)
{
    TRACE("(%p)->()\n", iface);

    /* Nothing to do */

    return S_OK;
}


static HRESULT WINAPI ID3DXFileDataImpl_GetType(ID3DXFileData *iface, GUID *guid)
{
    ID3DXFileDataImpl *This = impl_from_ID3DXFileData(iface);
    const GUID *dxfile_guid;
    HRESULT ret;

    TRACE("(%p)->(%p)\n", iface, guid);

    ret = IDirectXFileData_GetType(This->dxfile_data, &dxfile_guid);
    if (ret != DXFILE_OK)
        return error_dxfile_to_d3dxfile(ret);

    *guid = *dxfile_guid;

    return S_OK;
}


static BOOL WINAPI ID3DXFileDataImpl_IsReference(ID3DXFileData *iface)
{
    TRACE("(%p)->(): stub\n", iface);

    return E_NOTIMPL;
}


static HRESULT WINAPI ID3DXFileDataImpl_GetChildren(ID3DXFileData *iface, SIZE_T *children)
{
    TRACE("(%p)->(%p): stub\n", iface, children);

    return E_NOTIMPL;
}


static HRESULT WINAPI ID3DXFileDataImpl_GetChild(ID3DXFileData *iface, SIZE_T id, ID3DXFileData **object)
{
    TRACE("(%p)->(%lu, %p): stub\n", iface, id, object);

    return E_NOTIMPL;
}


static const ID3DXFileDataVtbl ID3DXFileData_Vtbl =
{
    ID3DXFileDataImpl_QueryInterface,
    ID3DXFileDataImpl_AddRef,
    ID3DXFileDataImpl_Release,
    ID3DXFileDataImpl_GetEnum,
    ID3DXFileDataImpl_GetName,
    ID3DXFileDataImpl_GetId,
    ID3DXFileDataImpl_Lock,
    ID3DXFileDataImpl_Unlock,
    ID3DXFileDataImpl_GetType,
    ID3DXFileDataImpl_IsReference,
    ID3DXFileDataImpl_GetChildren,
    ID3DXFileDataImpl_GetChild
};


static HRESULT ID3DXFileDataImpl_Create(IDirectXFileData *dxfile_data, ID3DXFileData **ret_iface)
{
    ID3DXFileDataImpl *object;

    TRACE("(%p, %p)\n", dxfile_data, ret_iface);

    *ret_iface = NULL;

    object = HeapAlloc(GetProcessHeap(), 0, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXFileData_iface.lpVtbl = &ID3DXFileData_Vtbl;
    object->ref = 1;
    object->dxfile_data = dxfile_data;

    *ret_iface = &object->ID3DXFileData_iface;

    return S_OK;
}


/*** IUnknown methods ***/

static HRESULT WINAPI ID3DXFileEnumObjectImpl_QueryInterface(ID3DXFileEnumObject *iface, REFIID riid, void **ret_iface)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXFileEnumObject))
    {
        iface->lpVtbl->AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    WARN("(%p)->(%s, %p), not found\n", iface, debugstr_guid(riid), ret_iface);
    *ret_iface = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXFileEnumObjectImpl_AddRef(ID3DXFileEnumObject *iface)
{
    ID3DXFileEnumObjectImpl *This = impl_from_ID3DXFileEnumObject(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI ID3DXFileEnumObjectImpl_Release(ID3DXFileEnumObject *iface)
{
    ID3DXFileEnumObjectImpl *This = impl_from_ID3DXFileEnumObject(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
    {
        ULONG i;

        for (i = 0; i < This->nb_children; i++)
            (This->children[i])->lpVtbl->Release(This->children[i]);
        HeapFree(GetProcessHeap(), 0, This->children);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}


/*** ID3DXFileEnumObject methods ***/

static HRESULT WINAPI ID3DXFileEnumObjectImpl_GetFile(ID3DXFileEnumObject *iface, ID3DXFile **file)
{
    FIXME("(%p)->(%p): stub\n", iface, file);

    return E_NOTIMPL;
}


static HRESULT WINAPI ID3DXFileEnumObjectImpl_GetChildren(ID3DXFileEnumObject *iface, SIZE_T *children)
{
    ID3DXFileEnumObjectImpl *This = impl_from_ID3DXFileEnumObject(iface);

    TRACE("(%p)->(%p)\n", iface, children);

    if (!children)
        return E_POINTER;

    *children = This->nb_children;

    return S_OK;
}


static HRESULT WINAPI ID3DXFileEnumObjectImpl_GetChild(ID3DXFileEnumObject *iface, SIZE_T id, ID3DXFileData **object)
{
    ID3DXFileEnumObjectImpl *This = impl_from_ID3DXFileEnumObject(iface);

    TRACE("(%p)->(%lu, %p)\n", iface, id, object);

    if (!object)
        return E_POINTER;

    *object = This->children[id];
    (*object)->lpVtbl->AddRef(*object);

    return S_OK;
}


static HRESULT WINAPI ID3DXFileEnumObjectImpl_GetDataObjectById(ID3DXFileEnumObject *iface, REFGUID guid, ID3DXFileData **object)
{
    FIXME("(%p)->(%s, %p): stub\n", iface, debugstr_guid(guid), object);

    return E_NOTIMPL;
}


static HRESULT WINAPI ID3DXFileEnumObjectImpl_GetDataObjectByName(ID3DXFileEnumObject *iface, const char *name, ID3DXFileData **object)
{
    FIXME("(%p)->(%s, %p): stub\n", iface, debugstr_a(name), object);

    return E_NOTIMPL;
}


static const ID3DXFileEnumObjectVtbl ID3DXFileEnumObject_Vtbl =
{
    ID3DXFileEnumObjectImpl_QueryInterface,
    ID3DXFileEnumObjectImpl_AddRef,
    ID3DXFileEnumObjectImpl_Release,
    ID3DXFileEnumObjectImpl_GetFile,
    ID3DXFileEnumObjectImpl_GetChildren,
    ID3DXFileEnumObjectImpl_GetChild,
    ID3DXFileEnumObjectImpl_GetDataObjectById,
    ID3DXFileEnumObjectImpl_GetDataObjectByName
};


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

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}


static ULONG WINAPI ID3DXFileImpl_Release(ID3DXFile *iface)
{
    ID3DXFileImpl *This = impl_from_ID3DXFile(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

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
    ID3DXFileImpl *This = impl_from_ID3DXFile(iface);
    ID3DXFileEnumObjectImpl *object;
    IDirectXFileEnumObject *dxfile_enum_object;
    void *dxfile_source;
    DXFILELOADOPTIONS dxfile_options;
    DXFILELOADRESOURCE dxfile_resource;
    DXFILELOADMEMORY dxfile_memory;
    IDirectXFileData *data_object;
    HRESULT ret;

    TRACE("(%p)->(%p, %x, %p)\n", iface, source, options, enum_object);

    if (!enum_object)
        return E_POINTER;

    *enum_object = NULL;

    if (options == D3DXF_FILELOAD_FROMFILE)
    {
        dxfile_source = (void*)source;
        dxfile_options = DXFILELOAD_FROMFILE;
    }
    else if (options == D3DXF_FILELOAD_FROMRESOURCE)
    {
        D3DXF_FILELOADRESOURCE *resource = (D3DXF_FILELOADRESOURCE*)source;

        dxfile_resource.hModule = resource->hModule;
        dxfile_resource.lpName = resource->lpName;
        dxfile_resource.lpType = resource->lpType;
        dxfile_source = &dxfile_resource;
        dxfile_options = DXFILELOAD_FROMRESOURCE;
    }
    else if (options == D3DXF_FILELOAD_FROMMEMORY)
    {
        D3DXF_FILELOADMEMORY *memory = (D3DXF_FILELOADMEMORY*)source;

        dxfile_memory.lpMemory = memory->lpMemory;
        dxfile_memory.dSize = memory->dSize;
        dxfile_source = &dxfile_memory;
        dxfile_options = DXFILELOAD_FROMMEMORY;
    }
    else
    {
        FIXME("Source type %u is not handled yet\n", options);
        return E_NOTIMPL;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXFileEnumObject_iface.lpVtbl = &ID3DXFileEnumObject_Vtbl;
    object->ref = 1;

    ret = IDirectXFile_CreateEnumObject(This->dxfile, dxfile_source, dxfile_options, &dxfile_enum_object);

    if (ret != S_OK)
    {
        HeapFree(GetProcessHeap(), 0, object);
        return ret;
    }

    /* Fill enum object with top level data objects */
    while (SUCCEEDED(ret = IDirectXFileEnumObject_GetNextDataObject(dxfile_enum_object, &data_object)))
    {
        if (object->children)
            object->children = HeapReAlloc(GetProcessHeap(), 0, object->children, sizeof(*object->children) * (object->nb_children + 1));
        else
            object->children = HeapAlloc(GetProcessHeap(), 0, sizeof(*object->children));
        if (!object->children)
        {
            ret = E_OUTOFMEMORY;
            break;
        }
        ret = ID3DXFileDataImpl_Create(data_object, &object->children[object->nb_children]);
        if (ret != S_OK)
            break;
        object->nb_children++;
    }

    IDirectXFileEnumObject_Release(dxfile_enum_object);

    if (ret != DXFILEERR_NOMOREOBJECTS)
        WARN("Cannot get all top level data objects\n");

    TRACE("Found %u children\n", object->nb_children);

    *enum_object = &object->ID3DXFileEnumObject_iface;

    return S_OK;
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
