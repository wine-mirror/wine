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
        case DXFILEERR_BADVALUE:
            return D3DXFERR_BADVALUE;
        default:
            FIXME("Cannot map error %#x\n", error);
            return E_FAIL;
    }
}

struct d3dx9_file
{
    ID3DXFile ID3DXFile_iface;
    LONG ref;
    IDirectXFile *dxfile;
};

struct d3dx9_file_enum_object
{
    ID3DXFileEnumObject ID3DXFileEnumObject_iface;
    LONG ref;
    ULONG nb_children;
    ID3DXFileData **children;
};

typedef struct {
    ID3DXFileData ID3DXFileData_iface;
    LONG ref;
    BOOL reference;
    IDirectXFileData *dxfile_data;
    ULONG nb_children;
    ID3DXFileData **children;
} ID3DXFileDataImpl;


static inline struct d3dx9_file *impl_from_ID3DXFile(ID3DXFile *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx9_file, ID3DXFile_iface);
}

static inline struct d3dx9_file_enum_object *impl_from_ID3DXFileEnumObject(ID3DXFileEnumObject *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx9_file_enum_object, ID3DXFileEnumObject_iface);
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
        ULONG i;

        for (i = 0; i < This->nb_children; i++)
            (This->children[i])->lpVtbl->Release(This->children[i]);
        HeapFree(GetProcessHeap(), 0, This->children);
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

    if (!size)
        return D3DXFERR_BADVALUE;

    dxfile_size = *size;

    ret = IDirectXFileData_GetName(This->dxfile_data, name, &dxfile_size);
    if (ret != DXFILE_OK)
        return error_dxfile_to_d3dxfile(ret);

    if (!dxfile_size)
    {
        /* Contrary to d3dxof, d3dx9_36 returns an empty string with a null byte when no name is available.
         * If the input size is 0, it returns a length of 1 without touching the buffer */
        dxfile_size = 1;
        if (name && *size)
            name[0] = 0;
    }

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
    ID3DXFileDataImpl *This = impl_from_ID3DXFileData(iface);

    TRACE("(%p)->()\n", iface);

    return This->reference;
}


static HRESULT WINAPI ID3DXFileDataImpl_GetChildren(ID3DXFileData *iface, SIZE_T *children)
{
    ID3DXFileDataImpl *This = impl_from_ID3DXFileData(iface);

    TRACE("(%p)->(%p)\n", iface, children);

    if (!children)
        return E_POINTER;

    *children = This->nb_children;

    return S_OK;
}


static HRESULT WINAPI ID3DXFileDataImpl_GetChild(ID3DXFileData *iface, SIZE_T id, ID3DXFileData **object)
{
    ID3DXFileDataImpl *This = impl_from_ID3DXFileData(iface);

    TRACE("(%p)->(%lu, %p)\n", iface, id, object);

    if (!object)
        return E_POINTER;

    *object = This->children[id];
    (*object)->lpVtbl->AddRef(*object);

    return S_OK;
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


static HRESULT ID3DXFileDataImpl_Create(IDirectXFileObject *dxfile_object, ID3DXFileData **ret_iface)
{
    ID3DXFileDataImpl *object;
    IDirectXFileObject *data_object;
    HRESULT ret;

    TRACE("(%p, %p)\n", dxfile_object, ret_iface);

    *ret_iface = NULL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXFileData_iface.lpVtbl = &ID3DXFileData_Vtbl;
    object->ref = 1;

    ret = IDirectXFileObject_QueryInterface(dxfile_object, &IID_IDirectXFileData, (void **)&object->dxfile_data);
    if (FAILED(ret))
    {
        IDirectXFileDataReference *reference;

        ret = IDirectXFileObject_QueryInterface(dxfile_object, &IID_IDirectXFileDataReference, (void **)&reference);
        if (SUCCEEDED(ret))
        {
            ret = IDirectXFileDataReference_Resolve(reference, &object->dxfile_data);
            if (FAILED(ret))
            {
                HeapFree(GetProcessHeap(), 0, object);
                return E_FAIL;
            }
            object->reference = TRUE;
        }
        else
        {
            FIXME("Don't known what to do with binary object\n");
            HeapFree(GetProcessHeap(), 0, object);
            return E_FAIL;
        }
    }

    while (SUCCEEDED(ret = IDirectXFileData_GetNextObject(object->dxfile_data, &data_object)))
    {
        if (object->children)
            object->children = HeapReAlloc(GetProcessHeap(), 0, object->children, sizeof(ID3DXFileData*) * (object->nb_children + 1));
        else
            object->children = HeapAlloc(GetProcessHeap(), 0, sizeof(ID3DXFileData*));
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

    if (ret != DXFILEERR_NOMOREOBJECTS)
    {
        (&object->ID3DXFileData_iface)->lpVtbl->Release(&object->ID3DXFileData_iface);
        return ret;
    }

    TRACE("Found %u children\n", object->nb_children);

    *ret_iface = &object->ID3DXFileData_iface;

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_enum_object_QueryInterface(ID3DXFileEnumObject *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXFileEnumObject)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        iface->lpVtbl->AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx9_file_enum_object_AddRef(ID3DXFileEnumObject *iface)
{
    struct d3dx9_file_enum_object *file_enum = impl_from_ID3DXFileEnumObject(iface);
    ULONG refcount = InterlockedIncrement(&file_enum->ref);

    TRACE("%p increasing refcount to %u.\n", file_enum, refcount);

    return refcount;
}

static ULONG WINAPI d3dx9_file_enum_object_Release(ID3DXFileEnumObject *iface)
{
    struct d3dx9_file_enum_object *file_enum = impl_from_ID3DXFileEnumObject(iface);
    ULONG refcount = InterlockedDecrement(&file_enum->ref);

    TRACE("%p decreasing refcount to %u.\n", file_enum, refcount);

    if (!refcount)
    {
        ULONG i;

        for (i = 0; i < file_enum->nb_children; ++i)
        {
            ID3DXFileData *child = file_enum->children[i];
            child->lpVtbl->Release(child);
        }
        HeapFree(GetProcessHeap(), 0, file_enum->children);
        HeapFree(GetProcessHeap(), 0, file_enum);
    }

    return refcount;
}

static HRESULT WINAPI d3dx9_file_enum_object_GetFile(ID3DXFileEnumObject *iface, ID3DXFile **file)
{
    FIXME("iface %p, file %p stub!\n", iface, file);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_file_enum_object_GetChildren(ID3DXFileEnumObject *iface, SIZE_T *children)
{
    struct d3dx9_file_enum_object *file_enum = impl_from_ID3DXFileEnumObject(iface);

    TRACE("iface %p, children %p.\n", iface, children);

    if (!children)
        return E_POINTER;

    *children = file_enum->nb_children;

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_enum_object_GetChild(ID3DXFileEnumObject *iface, SIZE_T id, ID3DXFileData **object)
{
    struct d3dx9_file_enum_object *file_enum = impl_from_ID3DXFileEnumObject(iface);

    TRACE("iface %p, id %#lx, object %p.\n", iface, id, object);

    if (!object)
        return E_POINTER;

    *object = file_enum->children[id];
    (*object)->lpVtbl->AddRef(*object);

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_enum_object_GetDataObjectById(ID3DXFileEnumObject *iface,
        REFGUID guid, ID3DXFileData **object)
{
    FIXME("iface %p, guid %s, object %p stub!\n", iface, debugstr_guid(guid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_file_enum_object_GetDataObjectByName(ID3DXFileEnumObject *iface,
        const char *name, ID3DXFileData **object)
{
    FIXME("iface %p, name %s, object %p stub!\n", iface, debugstr_a(name), object);

    return E_NOTIMPL;
}

static const ID3DXFileEnumObjectVtbl d3dx9_file_enum_object_vtbl =
{
    d3dx9_file_enum_object_QueryInterface,
    d3dx9_file_enum_object_AddRef,
    d3dx9_file_enum_object_Release,
    d3dx9_file_enum_object_GetFile,
    d3dx9_file_enum_object_GetChildren,
    d3dx9_file_enum_object_GetChild,
    d3dx9_file_enum_object_GetDataObjectById,
    d3dx9_file_enum_object_GetDataObjectByName,
};

static HRESULT WINAPI d3dx9_file_QueryInterface(ID3DXFile *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXFile)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        iface->lpVtbl->AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx9_file_AddRef(ID3DXFile *iface)
{
    struct d3dx9_file *file = impl_from_ID3DXFile(iface);
    ULONG refcount = InterlockedIncrement(&file->ref);

    TRACE("%p increasing refcount to %u.\n", file, refcount);

    return refcount;
}

static ULONG WINAPI d3dx9_file_Release(ID3DXFile *iface)
{
    struct d3dx9_file *file = impl_from_ID3DXFile(iface);
    ULONG refcount = InterlockedDecrement(&file->ref);

    TRACE("%p decreasing refcount to %u.\n", file, refcount);

    if (!refcount)
    {
        IDirectXFile_Release(file->dxfile);
        HeapFree(GetProcessHeap(), 0, file);
    }

    return refcount;
}

static HRESULT WINAPI d3dx9_file_CreateEnumObject(ID3DXFile *iface, const void *source,
        D3DXF_FILELOADOPTIONS options, ID3DXFileEnumObject **enum_object)
{
    struct d3dx9_file *file = impl_from_ID3DXFile(iface);
    struct d3dx9_file_enum_object *object;
    IDirectXFileEnumObject *dxfile_enum_object;
    void *dxfile_source;
    DXFILELOADOPTIONS dxfile_options;
    DXFILELOADRESOURCE dxfile_resource;
    DXFILELOADMEMORY dxfile_memory;
    IDirectXFileData *data_object;
    HRESULT ret;

    TRACE("iface %p, source %p, options %#x, enum_object %p.\n", iface, source, options, enum_object);

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

    object->ID3DXFileEnumObject_iface.lpVtbl = &d3dx9_file_enum_object_vtbl;
    object->ref = 1;

    ret = IDirectXFile_CreateEnumObject(file->dxfile, dxfile_source, dxfile_options, &dxfile_enum_object);

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
        ret = ID3DXFileDataImpl_Create((IDirectXFileObject*)data_object, &object->children[object->nb_children]);
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

static HRESULT WINAPI d3dx9_file_CreateSaveObject(ID3DXFile *iface, const void *data,
        D3DXF_FILESAVEOPTIONS options, D3DXF_FILEFORMAT format, ID3DXFileSaveObject **save_object)
{
    FIXME("iface %p, data %p, options %#x, format %#x, save_object %p stub!\n",
            iface, data, options, format, save_object);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_file_RegisterTemplates(ID3DXFile *iface, const void *data, SIZE_T size)
{
    struct d3dx9_file *file = impl_from_ID3DXFile(iface);
    HRESULT ret;

    TRACE("iface %p, data %p, size %lu.\n", iface, data, size);

    ret = IDirectXFile_RegisterTemplates(file->dxfile, (void *)data, size);
    if (ret != DXFILE_OK)
    {
        WARN("Error %#x\n", ret);
        return error_dxfile_to_d3dxfile(ret);
    }

    return S_OK;
}

static HRESULT WINAPI d3dx9_file_RegisterEnumTemplates(ID3DXFile *iface, ID3DXFileEnumObject *enum_object)
{
    FIXME("iface %p, enum_object %p stub!\n", iface, enum_object);

    return E_NOTIMPL;
}

static const ID3DXFileVtbl d3dx9_file_vtbl =
{
    d3dx9_file_QueryInterface,
    d3dx9_file_AddRef,
    d3dx9_file_Release,
    d3dx9_file_CreateEnumObject,
    d3dx9_file_CreateSaveObject,
    d3dx9_file_RegisterTemplates,
    d3dx9_file_RegisterEnumTemplates,
};

HRESULT WINAPI D3DXFileCreate(ID3DXFile **d3dxfile)
{
    struct d3dx9_file *object;
    HRESULT ret;

    TRACE("d3dxfile %p.\n", d3dxfile);

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

    object->ID3DXFile_iface.lpVtbl = &d3dx9_file_vtbl;
    object->ref = 1;

    *d3dxfile = &object->ID3DXFile_iface;

    return S_OK;
}
