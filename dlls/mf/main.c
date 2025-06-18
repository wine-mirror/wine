/*
 * Copyright (C) 2015 Austin English
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
#include "mfidl.h"
#include "rpcproxy.h"

#include "mf_private.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/mfinternal.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

extern const GUID CLSID_FileSchemePlugin;

struct activate_object
{
    IMFActivate IMFActivate_iface;
    LONG refcount;
    IMFAttributes *attributes;
    IUnknown *object;
    const struct activate_funcs *funcs;
    void *context;
};

static struct activate_object *impl_from_IMFActivate(IMFActivate *iface)
{
    return CONTAINING_RECORD(iface, struct activate_object, IMFActivate_iface);
}

static HRESULT WINAPI activate_object_QueryInterface(IMFActivate *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFActivate) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFActivate_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activate_object_AddRef(IMFActivate *iface)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);
    ULONG refcount = InterlockedIncrement(&activate->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI activate_object_Release(IMFActivate *iface)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);
    ULONG refcount = InterlockedDecrement(&activate->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (activate->funcs->free_private)
            activate->funcs->free_private(activate->context);
        if (activate->object)
            IUnknown_Release(activate->object);
        IMFAttributes_Release(activate->attributes);
        free(activate);
    }

    return refcount;
}

static HRESULT WINAPI activate_object_GetItem(IMFActivate *iface, REFGUID key, PROPVARIANT *value)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetItem(activate->attributes, key, value);
}

static HRESULT WINAPI activate_object_GetItemType(IMFActivate *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), type);

    return IMFAttributes_GetItemType(activate->attributes, key, type);
}

static HRESULT WINAPI activate_object_CompareItem(IMFActivate *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, result);

    return IMFAttributes_CompareItem(activate->attributes, key, value, result);
}

static HRESULT WINAPI activate_object_Compare(IMFActivate *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
        BOOL *result)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, type, result);

    return IMFAttributes_Compare(activate->attributes, theirs, type, result);
}

static HRESULT WINAPI activate_object_GetUINT32(IMFActivate *iface, REFGUID key, UINT32 *value)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT32(activate->attributes, key, value);
}

static HRESULT WINAPI activate_object_GetUINT64(IMFActivate *iface, REFGUID key, UINT64 *value)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT64(activate->attributes, key, value);
}

static HRESULT WINAPI activate_object_GetDouble(IMFActivate *iface, REFGUID key, double *value)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetDouble(activate->attributes, key, value);
}

static HRESULT WINAPI activate_object_GetGUID(IMFActivate *iface, REFGUID key, GUID *value)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetGUID(activate->attributes, key, value);
}

static HRESULT WINAPI activate_object_GetStringLength(IMFActivate *iface, REFGUID key, UINT32 *length)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), length);

    return IMFAttributes_GetStringLength(activate->attributes, key, length);
}

static HRESULT WINAPI activate_object_GetString(IMFActivate *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), value, size, length);

    return IMFAttributes_GetString(activate->attributes, key, value, size, length);
}

static HRESULT WINAPI activate_object_GetAllocatedString(IMFActivate *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, length);

    return IMFAttributes_GetAllocatedString(activate->attributes, key, value, length);
}

static HRESULT WINAPI activate_object_GetBlobSize(IMFActivate *iface, REFGUID key, UINT32 *size)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), size);

    return IMFAttributes_GetBlobSize(activate->attributes, key, size);
}

static HRESULT WINAPI activate_object_GetBlob(IMFActivate *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), buf, bufsize, blobsize);

    return IMFAttributes_GetBlob(activate->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI activate_object_GetAllocatedBlob(IMFActivate *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_GetAllocatedBlob(activate->attributes, key, buf, size);
}

static HRESULT WINAPI activate_object_GetUnknown(IMFActivate *iface, REFGUID key, REFIID riid, void **ppv)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(key), debugstr_guid(riid), ppv);

    return IMFAttributes_GetUnknown(activate->attributes, key, riid, ppv);
}

static HRESULT WINAPI activate_object_SetItem(IMFActivate *iface, REFGUID key, REFPROPVARIANT value)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetItem(activate->attributes, key, value);
}

static HRESULT WINAPI activate_object_DeleteItem(IMFActivate *iface, REFGUID key)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s.\n", iface, debugstr_guid(key));

    return IMFAttributes_DeleteItem(activate->attributes, key);
}

static HRESULT WINAPI activate_object_DeleteAllItems(IMFActivate *iface)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_DeleteAllItems(activate->attributes);
}

static HRESULT WINAPI activate_object_SetUINT32(IMFActivate *iface, REFGUID key, UINT32 value)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %d.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetUINT32(activate->attributes, key, value);
}

static HRESULT WINAPI activate_object_SetUINT64(IMFActivate *iface, REFGUID key, UINT64 value)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), wine_dbgstr_longlong(value));

    return IMFAttributes_SetUINT64(activate->attributes, key, value);
}

static HRESULT WINAPI activate_object_SetDouble(IMFActivate *iface, REFGUID key, double value)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetDouble(activate->attributes, key, value);
}

static HRESULT WINAPI activate_object_SetGUID(IMFActivate *iface, REFGUID key, REFGUID value)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_guid(value));

    return IMFAttributes_SetGUID(activate->attributes, key, value);
}

static HRESULT WINAPI activate_object_SetString(IMFActivate *iface, REFGUID key, const WCHAR *value)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_w(value));

    return IMFAttributes_SetString(activate->attributes, key, value);
}

static HRESULT WINAPI activate_object_SetBlob(IMFActivate *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %s, %p, %d.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_SetBlob(activate->attributes, key, buf, size);
}

static HRESULT WINAPI activate_object_SetUnknown(IMFActivate *iface, REFGUID key, IUnknown *unknown)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(key), unknown);

    return IMFAttributes_SetUnknown(activate->attributes, key, unknown);
}

static HRESULT WINAPI activate_object_LockStore(IMFActivate *iface)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_LockStore(activate->attributes);
}

static HRESULT WINAPI activate_object_UnlockStore(IMFActivate *iface)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_UnlockStore(activate->attributes);
}

static HRESULT WINAPI activate_object_GetCount(IMFActivate *iface, UINT32 *count)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %p.\n", iface, count);

    return IMFAttributes_GetCount(activate->attributes, count);
}

static HRESULT WINAPI activate_object_GetItemByIndex(IMFActivate *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return IMFAttributes_GetItemByIndex(activate->attributes, index, key, value);
}

static HRESULT WINAPI activate_object_CopyAllItems(IMFActivate *iface, IMFAttributes *dest)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);

    TRACE("%p, %p.\n", iface, dest);

    return IMFAttributes_CopyAllItems(activate->attributes, dest);
}

static HRESULT WINAPI activate_object_ActivateObject(IMFActivate *iface, REFIID riid, void **obj)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);
    IUnknown *object;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (!activate->object)
    {
        if (FAILED(hr = activate->funcs->create_object((IMFAttributes *)iface, activate->context, &object)))
            return hr;

        if (InterlockedCompareExchangePointer((void **)&activate->object, object, NULL))
            IUnknown_Release(object);
    }

    return IUnknown_QueryInterface(activate->object, riid, obj);
}

static HRESULT WINAPI activate_object_ShutdownObject(IMFActivate *iface)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);
    IUnknown *object;

    TRACE("%p.\n", iface);

    if ((object = InterlockedCompareExchangePointer((void **)&activate->object, NULL, activate->object)))
    {
        activate->funcs->shutdown_object(activate->context, object);
        IUnknown_Release(object);
    }

    return S_OK;
}

static HRESULT WINAPI activate_object_DetachObject(IMFActivate *iface)
{
    TRACE("%p.\n", iface);

    return E_NOTIMPL;
}

static const IMFActivateVtbl activate_object_vtbl =
{
    activate_object_QueryInterface,
    activate_object_AddRef,
    activate_object_Release,
    activate_object_GetItem,
    activate_object_GetItemType,
    activate_object_CompareItem,
    activate_object_Compare,
    activate_object_GetUINT32,
    activate_object_GetUINT64,
    activate_object_GetDouble,
    activate_object_GetGUID,
    activate_object_GetStringLength,
    activate_object_GetString,
    activate_object_GetAllocatedString,
    activate_object_GetBlobSize,
    activate_object_GetBlob,
    activate_object_GetAllocatedBlob,
    activate_object_GetUnknown,
    activate_object_SetItem,
    activate_object_DeleteItem,
    activate_object_DeleteAllItems,
    activate_object_SetUINT32,
    activate_object_SetUINT64,
    activate_object_SetDouble,
    activate_object_SetGUID,
    activate_object_SetString,
    activate_object_SetBlob,
    activate_object_SetUnknown,
    activate_object_LockStore,
    activate_object_UnlockStore,
    activate_object_GetCount,
    activate_object_GetItemByIndex,
    activate_object_CopyAllItems,
    activate_object_ActivateObject,
    activate_object_ShutdownObject,
    activate_object_DetachObject,
};

HRESULT create_activation_object(void *context, const struct activate_funcs *funcs, IMFActivate **ret)
{
    struct activate_object *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFActivate_iface.lpVtbl = &activate_object_vtbl;
    object->refcount = 1;
    if (FAILED(hr = MFCreateAttributes(&object->attributes, 0)))
    {
        free(object);
        return hr;
    }
    object->funcs = funcs;
    object->context = context;

    *ret = &object->IMFActivate_iface;

    return S_OK;
}

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_instance)(REFIID riid, void **obj);
};

static inline struct class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct class_factory, IClassFactory_iface);
}

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IClassFactory) ||
            IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("%s is not supported.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI class_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);

    TRACE("%p, %p, %s, %p.\n", iface, outer, debugstr_guid(riid), obj);

    if (outer)
    {
        *obj = NULL;
        return CLASS_E_NOAGGREGATION;
    }

    return factory->create_instance(riid, obj);
}

static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("%d.\n", dolock);

    return S_OK;
}

static const IClassFactoryVtbl class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer,
};

static struct class_factory file_scheme_handler_factory = { { &class_factory_vtbl }, file_scheme_handler_construct };
static struct class_factory urlmon_scheme_handler_factory = { { &class_factory_vtbl }, urlmon_scheme_handler_construct };

static const struct class_object
{
    const GUID *clsid;
    IClassFactory *factory;
}
class_objects[] =
{
    { &CLSID_FileSchemePlugin, &file_scheme_handler_factory.IClassFactory_iface },
    { &CLSID_UrlmonSchemePlugin, &urlmon_scheme_handler_factory.IClassFactory_iface },
};

/*******************************************************************************
 *      DllGetClassObject (mf.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **obj)
{
    unsigned int i;

    TRACE("%s, %s, %p.\n", debugstr_guid(rclsid), debugstr_guid(riid), obj);

    for (i = 0; i < ARRAY_SIZE(class_objects); ++i)
    {
        if (IsEqualGUID(class_objects[i].clsid, rclsid))
            return IClassFactory_QueryInterface(class_objects[i].factory, riid, obj);
    }

    WARN("%s: class not found.\n", debugstr_guid(rclsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

static HRESULT prop_string_vector_append(PROPVARIANT *vector, unsigned int *capacity, BOOL unique, const WCHAR *str)
{
    WCHAR *ptrW;
    int len, i;

    if (unique)
    {
        for (i = 0; i < vector->calpwstr.cElems; ++i)
        {
            if (!lstrcmpW(vector->calpwstr.pElems[i], str))
                return S_OK;
        }
    }

    if (!*capacity || *capacity - 1 < vector->calpwstr.cElems)
    {
        unsigned int new_count;
        WCHAR **ptr;

        new_count = *capacity ? *capacity * 2 : 10;
        ptr = CoTaskMemRealloc(vector->calpwstr.pElems, new_count * sizeof(*vector->calpwstr.pElems));
        if (!ptr)
            return E_OUTOFMEMORY;
        vector->calpwstr.pElems = ptr;
        *capacity = new_count;
    }

    len = lstrlenW(str);
    if (!(vector->calpwstr.pElems[vector->calpwstr.cElems] = ptrW = CoTaskMemAlloc((len + 1) * sizeof(WCHAR))))
        return E_OUTOFMEMORY;

    lstrcpyW(ptrW, str);
    vector->calpwstr.cElems++;

    return S_OK;
}

static int __cdecl qsort_string_compare(const void *a, const void *b)
{
    const WCHAR *left = *(const WCHAR **)a, *right = *(const WCHAR **)b;
    return lstrcmpW(left, right);
}

static HRESULT mf_get_handler_strings(const WCHAR *path, WCHAR filter, unsigned int maxlen, PROPVARIANT *dst)
{
    static const HKEY hkey_roots[2] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    unsigned int capacity = 0, count;
    HRESULT hr = S_OK;
    int i, index;
    WCHAR *buffW;
    DWORD size;

    if (!(buffW = calloc(maxlen, sizeof(*buffW))))
        return E_OUTOFMEMORY;

    memset(dst, 0, sizeof(*dst));
    dst->vt = VT_VECTOR | VT_LPWSTR;

    for (i = 0; i < ARRAY_SIZE(hkey_roots); ++i)
    {
        HKEY hkey;

        if (RegOpenKeyW(hkey_roots[i], path, &hkey))
            continue;

        index = 0;
        size = maxlen;
        count = dst->calpwstr.cElems;
        while (!RegEnumKeyExW(hkey, index++, buffW, &size, NULL, NULL, NULL, NULL))
        {
            if (filter && !wcschr(buffW, filter))
                continue;

            if (FAILED(hr = prop_string_vector_append(dst, &capacity, i > 0, buffW)))
                break;
            size = maxlen;
        }

        /* Sort last pass results. */
        qsort(&dst->calpwstr.pElems[count], dst->calpwstr.cElems - count, sizeof(*dst->calpwstr.pElems),
                qsort_string_compare);

        RegCloseKey(hkey);
    }

    if (FAILED(hr))
        PropVariantClear(dst);

    free(buffW);

    return hr;
}

/***********************************************************************
 *      MFGetSupportedMimeTypes (mf.@)
 */
HRESULT WINAPI MFGetSupportedMimeTypes(PROPVARIANT *dst)
{
    unsigned int maxlen;

    TRACE("%p.\n", dst);

    if (!dst)
        return E_POINTER;

    /* According to RFC4288 it's 127/127 characters. */
    maxlen = 127 /* type */ + 1 /* / */ + 127 /* subtype */ + 1;
    return mf_get_handler_strings(L"Software\\Microsoft\\Windows Media Foundation\\ByteStreamHandlers", '/',
            maxlen,  dst);
}

/***********************************************************************
 *      MFGetSupportedSchemes (mf.@)
 */
HRESULT WINAPI MFGetSupportedSchemes(PROPVARIANT *dst)
{
    TRACE("%p.\n", dst);

    if (!dst)
        return E_POINTER;

    return mf_get_handler_strings(L"Software\\Microsoft\\Windows Media Foundation\\SchemeHandlers", 0, 64, dst);
}

/***********************************************************************
 *      MFGetService (mf.@)
 */
HRESULT WINAPI MFGetService(IUnknown *object, REFGUID service, REFIID riid, void **obj)
{
    IMFGetService *gs;
    HRESULT hr;

    TRACE("(%p, %s, %s, %p)\n", object, debugstr_guid(service), debugstr_guid(riid), obj);

    if (!object)
        return E_POINTER;

    if (FAILED(hr = IUnknown_QueryInterface(object, &IID_IMFGetService, (void **)&gs)))
        return hr;

    hr = IMFGetService_GetService(gs, service, riid, obj);
    IMFGetService_Release(gs);
    return hr;
}

/***********************************************************************
 *      MFShutdownObject (mf.@)
 */
HRESULT WINAPI MFShutdownObject(IUnknown *object)
{
    IMFShutdown *shutdown;

    TRACE("%p.\n", object);

    if (object && SUCCEEDED(IUnknown_QueryInterface(object, &IID_IMFShutdown, (void **)&shutdown)))
    {
        IMFShutdown_Shutdown(shutdown);
        IMFShutdown_Release(shutdown);
    }

    return S_OK;
}

/***********************************************************************
 *      MFEnumDeviceSources (mf.@)
 */
HRESULT WINAPI MFEnumDeviceSources(IMFAttributes *attributes, IMFActivate ***sources, UINT32 *ret_count)
{
    GUID source_type;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", attributes, sources, ret_count);

    if (!attributes || !sources || !ret_count)
        return E_INVALIDARG;

    if (FAILED(hr = IMFAttributes_GetGUID(attributes, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &source_type)))
        return hr;

    if (IsEqualGUID(&source_type, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID))
    {
        FIXME("Not implemented for video capture devices.\n");
        *ret_count = 0;
        return S_OK;
    }
    if (IsEqualGUID(&source_type, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID))
        return enum_audio_capture_sources(attributes, sources, ret_count);

    return E_INVALIDARG;
}

struct simple_type_handler
{
    IMFMediaTypeHandler IMFMediaTypeHandler_iface;
    LONG refcount;
    IMFMediaType *media_type;
    CRITICAL_SECTION cs;
};

static struct simple_type_handler *impl_from_IMFMediaTypeHandler(IMFMediaTypeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct simple_type_handler, IMFMediaTypeHandler_iface);
}

static HRESULT WINAPI simple_type_handler_QueryInterface(IMFMediaTypeHandler *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaTypeHandler) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFMediaTypeHandler_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI simple_type_handler_AddRef(IMFMediaTypeHandler *iface)
{
    struct simple_type_handler *handler = impl_from_IMFMediaTypeHandler(iface);
    ULONG refcount = InterlockedIncrement(&handler->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI simple_type_handler_Release(IMFMediaTypeHandler *iface)
{
    struct simple_type_handler *handler = impl_from_IMFMediaTypeHandler(iface);
    ULONG refcount = InterlockedDecrement(&handler->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (handler->media_type)
            IMFMediaType_Release(handler->media_type);
        DeleteCriticalSection(&handler->cs);
        free(handler);
    }

    return refcount;
}

static HRESULT WINAPI simple_type_handler_IsMediaTypeSupported(IMFMediaTypeHandler *iface, IMFMediaType *in_type,
        IMFMediaType **out_type)
{
    struct simple_type_handler *handler = impl_from_IMFMediaTypeHandler(iface);
    DWORD flags = 0;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, in_type, out_type);

    if (out_type)
        *out_type = NULL;

    EnterCriticalSection(&handler->cs);
    if (!handler->media_type)
        hr = MF_E_UNEXPECTED;
    else
    {
        if (SUCCEEDED(hr = IMFMediaType_IsEqual(handler->media_type, in_type, &flags)))
            hr = (flags & (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES)) ==
                    (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES) ? S_OK : E_FAIL;
    }
    LeaveCriticalSection(&handler->cs);

    return hr;
}

static HRESULT WINAPI simple_type_handler_GetMediaTypeCount(IMFMediaTypeHandler *iface, DWORD *count)
{
    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    *count = 1;

    return S_OK;
}

static HRESULT WINAPI simple_type_handler_GetMediaTypeByIndex(IMFMediaTypeHandler *iface, DWORD index,
        IMFMediaType **type)
{
    struct simple_type_handler *handler = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %lu, %p.\n", iface, index, type);

    if (index > 0)
        return MF_E_NO_MORE_TYPES;

    EnterCriticalSection(&handler->cs);
    *type = handler->media_type;
    if (*type)
        IMFMediaType_AddRef(*type);
    LeaveCriticalSection(&handler->cs);

    return S_OK;
}

static HRESULT WINAPI simple_type_handler_SetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType *media_type)
{
    struct simple_type_handler *handler = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p.\n", iface, media_type);

    EnterCriticalSection(&handler->cs);
    if (handler->media_type)
        IMFMediaType_Release(handler->media_type);
    handler->media_type = media_type;
    if (handler->media_type)
        IMFMediaType_AddRef(handler->media_type);
    LeaveCriticalSection(&handler->cs);

    return S_OK;
}

static HRESULT WINAPI simple_type_handler_GetCurrentMediaType(IMFMediaTypeHandler *iface, IMFMediaType **media_type)
{
    struct simple_type_handler *handler = impl_from_IMFMediaTypeHandler(iface);

    TRACE("%p, %p.\n", iface, media_type);

    if (!media_type)
        return E_POINTER;

    EnterCriticalSection(&handler->cs);
    *media_type = handler->media_type;
    if (*media_type)
        IMFMediaType_AddRef(*media_type);
    LeaveCriticalSection(&handler->cs);

    return S_OK;
}

static HRESULT WINAPI simple_type_handler_GetMajorType(IMFMediaTypeHandler *iface, GUID *type)
{
    struct simple_type_handler *handler = impl_from_IMFMediaTypeHandler(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, type);

    EnterCriticalSection(&handler->cs);
    if (handler->media_type)
        hr = IMFMediaType_GetGUID(handler->media_type, &MF_MT_MAJOR_TYPE, type);
    else
        hr = MF_E_NOT_INITIALIZED;
    LeaveCriticalSection(&handler->cs);

    return hr;
}

static const IMFMediaTypeHandlerVtbl simple_type_handler_vtbl =
{
    simple_type_handler_QueryInterface,
    simple_type_handler_AddRef,
    simple_type_handler_Release,
    simple_type_handler_IsMediaTypeSupported,
    simple_type_handler_GetMediaTypeCount,
    simple_type_handler_GetMediaTypeByIndex,
    simple_type_handler_SetCurrentMediaType,
    simple_type_handler_GetCurrentMediaType,
    simple_type_handler_GetMajorType,
};

/*******************************************************************************
 *      MFCreateSimpleTypeHandler (mf.@)
 */
HRESULT WINAPI MFCreateSimpleTypeHandler(IMFMediaTypeHandler **handler)
{
    struct simple_type_handler *object;

    TRACE("%p.\n", handler);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFMediaTypeHandler_iface.lpVtbl = &simple_type_handler_vtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);

    *handler = &object->IMFMediaTypeHandler_iface;

    return S_OK;
}

/*******************************************************************************
 *      MFRequireProtectedEnvironment (mf.@)
 */
HRESULT WINAPI MFRequireProtectedEnvironment(IMFPresentationDescriptor *pd)
{
    BOOL selected, protected = FALSE;
    unsigned int i = 0, value;
    IMFStreamDescriptor *sd;

    TRACE("%p.\n", pd);

    while (SUCCEEDED(IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, i++, &selected, &sd)))
    {
        value = 0;
        protected = SUCCEEDED(IMFStreamDescriptor_GetUINT32(sd, &MF_SD_PROTECTED, &value)) && value;
        IMFStreamDescriptor_Release(sd);
        if (protected) break;
    }

    return protected ? S_OK : S_FALSE;
}

static HRESULT create_media_sink(const CLSID *clsid, IMFByteStream *stream, IMFMediaType *video_type,
        IMFMediaType *audio_type, IMFMediaSink **sink)
{
    IMFSinkClassFactory *factory;
    HRESULT hr;

    if (FAILED(hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IMFSinkClassFactory, (void **)&factory)))
        return hr;

    hr = IMFSinkClassFactory_CreateMediaSink(factory, stream, video_type, audio_type, sink);
    IMFSinkClassFactory_Release(factory);

    return hr;
}

/*******************************************************************************
 *      MFCreate3GPMediaSink (mf.@)
 */
HRESULT WINAPI MFCreate3GPMediaSink(IMFByteStream *stream, IMFMediaType *video_type,
         IMFMediaType *audio_type, IMFMediaSink **sink)
{
    TRACE("%p, %p, %p, %p.\n", stream, video_type, audio_type, sink);

    return create_media_sink(&CLSID_MF3GPSinkClassFactory, stream, video_type, audio_type, sink);
}

/*******************************************************************************
 *      MFCreateAC3MediaSink (mf.@)
 */
HRESULT WINAPI MFCreateAC3MediaSink(IMFByteStream *stream, IMFMediaType *audio_type, IMFMediaSink **sink)
{
    TRACE("%p, %p, %p.\n", stream, audio_type, sink);

    return create_media_sink(&CLSID_MFAC3SinkClassFactory, stream, NULL, audio_type, sink);
}

/*******************************************************************************
 *      MFCreateADTSMediaSink (mf.@)
 */
HRESULT WINAPI MFCreateADTSMediaSink(IMFByteStream *stream, IMFMediaType *audio_type, IMFMediaSink **sink)
{
    TRACE("%p, %p, %p.\n", stream, audio_type, sink);

    return create_media_sink(&CLSID_MFADTSSinkClassFactory, stream, NULL, audio_type, sink);
}

/*******************************************************************************
 *      MFCreateMP3MediaSink (mf.@)
 */
HRESULT WINAPI MFCreateMP3MediaSink(IMFByteStream *stream, IMFMediaSink **sink)
{
    TRACE("%p, %p.\n", stream, sink);

    return create_media_sink(&CLSID_MFMP3SinkClassFactory, stream, NULL, NULL, sink);
}

/*******************************************************************************
 *      MFCreateMPEG4MediaSink (mf.@)
 */
HRESULT WINAPI MFCreateMPEG4MediaSink(IMFByteStream *stream, IMFMediaType *video_type,
         IMFMediaType *audio_type, IMFMediaSink **sink)
{
    TRACE("%p, %p, %p, %p.\n", stream, video_type, audio_type, sink);

    return create_media_sink(&CLSID_MFMPEG4SinkClassFactory, stream, video_type, audio_type, sink);
}

/*******************************************************************************
 *      MFCreateFMPEG4MediaSink (mf.@)
 */
HRESULT WINAPI MFCreateFMPEG4MediaSink(IMFByteStream *stream, IMFMediaType *video_type,
         IMFMediaType *audio_type, IMFMediaSink **sink)
{
    TRACE("%p, %p, %p, %p.\n", stream, video_type, audio_type, sink);

    return create_media_sink(&CLSID_MFFMPEG4SinkClassFactory, stream, video_type, audio_type, sink);
}
