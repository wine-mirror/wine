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
#include "wine/heap.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static HINSTANCE mf_instance;
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

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI activate_object_Release(IMFActivate *iface)
{
    struct activate_object *activate = impl_from_IMFActivate(iface);
    ULONG refcount = InterlockedDecrement(&activate->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (activate->funcs->free_private)
            activate->funcs->free_private(activate->context);
        if (activate->object)
            IUnknown_Release(activate->object);
        IMFAttributes_Release(activate->attributes);
        heap_free(activate);
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

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFActivate_iface.lpVtbl = &activate_object_vtbl;
    object->refcount = 1;
    if (FAILED(hr = MFCreateAttributes(&object->attributes, 0)))
    {
        heap_free(object);
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

struct file_scheme_handler_result
{
    struct list entry;
    IMFAsyncResult *result;
    MF_OBJECT_TYPE obj_type;
    IUnknown *object;
};

struct file_scheme_handler
{
    IMFSchemeHandler IMFSchemeHandler_iface;
    IMFAsyncCallback IMFAsyncCallback_iface;
    LONG refcount;
    IMFSourceResolver *resolver;
    struct list results;
    CRITICAL_SECTION cs;
};

static struct file_scheme_handler *impl_from_IMFSchemeHandler(IMFSchemeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct file_scheme_handler, IMFSchemeHandler_iface);
}

static struct file_scheme_handler *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct file_scheme_handler, IMFAsyncCallback_iface);
}

static HRESULT WINAPI file_scheme_handler_QueryInterface(IMFSchemeHandler *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFSchemeHandler) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFSchemeHandler_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI file_scheme_handler_AddRef(IMFSchemeHandler *iface)
{
    struct file_scheme_handler *handler = impl_from_IMFSchemeHandler(iface);
    ULONG refcount = InterlockedIncrement(&handler->refcount);

    TRACE("%p, refcount %u.\n", handler, refcount);

    return refcount;
}

static ULONG WINAPI file_scheme_handler_Release(IMFSchemeHandler *iface)
{
    struct file_scheme_handler *handler = impl_from_IMFSchemeHandler(iface);
    ULONG refcount = InterlockedDecrement(&handler->refcount);
    struct file_scheme_handler_result *result, *next;

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        LIST_FOR_EACH_ENTRY_SAFE(result, next, &handler->results, struct file_scheme_handler_result, entry)
        {
            list_remove(&result->entry);
            IMFAsyncResult_Release(result->result);
            if (result->object)
                IUnknown_Release(result->object);
            heap_free(result);
        }
        DeleteCriticalSection(&handler->cs);
        if (handler->resolver)
            IMFSourceResolver_Release(handler->resolver);
        heap_free(handler);
    }

    return refcount;
}

struct create_object_context
{
    IUnknown IUnknown_iface;
    LONG refcount;

    IPropertyStore *props;
    WCHAR *url;
    DWORD flags;
};

static struct create_object_context *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct create_object_context, IUnknown_iface);
}

static HRESULT WINAPI create_object_context_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI create_object_context_AddRef(IUnknown *iface)
{
    struct create_object_context *context = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&context->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI create_object_context_Release(IUnknown *iface)
{
    struct create_object_context *context = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&context->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (context->props)
            IPropertyStore_Release(context->props);
        heap_free(context->url);
        heap_free(context);
    }

    return refcount;
}

static const IUnknownVtbl create_object_context_vtbl =
{
    create_object_context_QueryInterface,
    create_object_context_AddRef,
    create_object_context_Release,
};

static WCHAR *heap_strdupW(const WCHAR *str)
{
    WCHAR *ret = NULL;

    if (str)
    {
        unsigned int size;

        size = (lstrlenW(str) + 1) * sizeof(WCHAR);
        ret = heap_alloc(size);
        if (ret)
            memcpy(ret, str, size);
    }

    return ret;
}

static HRESULT WINAPI file_scheme_handler_BeginCreateObject(IMFSchemeHandler *iface, const WCHAR *url, DWORD flags,
        IPropertyStore *props, IUnknown **cancel_cookie, IMFAsyncCallback *callback, IUnknown *state)
{
    struct file_scheme_handler *handler = impl_from_IMFSchemeHandler(iface);
    struct create_object_context *context;
    IMFAsyncResult *caller, *item;
    HRESULT hr;

    TRACE("%p, %s, %#x, %p, %p, %p, %p.\n", iface, debugstr_w(url), flags, props, cancel_cookie, callback, state);

    if (cancel_cookie)
        *cancel_cookie = NULL;

    if (FAILED(hr = MFCreateAsyncResult(NULL, callback, state, &caller)))
        return hr;

    context = heap_alloc(sizeof(*context));
    if (!context)
    {
        IMFAsyncResult_Release(caller);
        return E_OUTOFMEMORY;
    }

    context->IUnknown_iface.lpVtbl = &create_object_context_vtbl;
    context->refcount = 1;
    context->props = props;
    if (context->props)
        IPropertyStore_AddRef(context->props);
    context->flags = flags;
    context->url = heap_strdupW(url);
    if (!context->url)
    {
        IMFAsyncResult_Release(caller);
        IUnknown_Release(&context->IUnknown_iface);
        return E_OUTOFMEMORY;
    }

    hr = MFCreateAsyncResult(&context->IUnknown_iface, &handler->IMFAsyncCallback_iface, (IUnknown *)caller, &item);
    IUnknown_Release(&context->IUnknown_iface);
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = MFPutWorkItemEx(MFASYNC_CALLBACK_QUEUE_IO, item)))
        {
            if (cancel_cookie)
            {
                *cancel_cookie = (IUnknown *)caller;
                IUnknown_AddRef(*cancel_cookie);
            }
        }

        IMFAsyncResult_Release(item);
    }
    IMFAsyncResult_Release(caller);

    return hr;
}

static HRESULT WINAPI file_scheme_handler_EndCreateObject(IMFSchemeHandler *iface, IMFAsyncResult *result,
        MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    struct file_scheme_handler *handler = impl_from_IMFSchemeHandler(iface);
    struct file_scheme_handler_result *found = NULL, *cur;
    HRESULT hr;

    TRACE("%p, %p, %p, %p.\n", iface, result, obj_type, object);

    EnterCriticalSection(&handler->cs);

    LIST_FOR_EACH_ENTRY(cur, &handler->results, struct file_scheme_handler_result, entry)
    {
        if (result == cur->result)
        {
            list_remove(&cur->entry);
            found = cur;
            break;
        }
    }

    LeaveCriticalSection(&handler->cs);

    if (found)
    {
        *obj_type = found->obj_type;
        *object = found->object;
        hr = IMFAsyncResult_GetStatus(found->result);
        IMFAsyncResult_Release(found->result);
        heap_free(found);
    }
    else
    {
        *obj_type = MF_OBJECT_INVALID;
        *object = NULL;
        hr = MF_E_UNEXPECTED;
    }

    return hr;
}

static HRESULT WINAPI file_scheme_handler_CancelObjectCreation(IMFSchemeHandler *iface, IUnknown *cancel_cookie)
{
    struct file_scheme_handler *handler = impl_from_IMFSchemeHandler(iface);
    struct file_scheme_handler_result *found = NULL, *cur;

    TRACE("%p, %p.\n", iface, cancel_cookie);

    EnterCriticalSection(&handler->cs);

    LIST_FOR_EACH_ENTRY(cur, &handler->results, struct file_scheme_handler_result, entry)
    {
        if (cancel_cookie == (IUnknown *)cur->result)
        {
            list_remove(&cur->entry);
            found = cur;
            break;
        }
    }

    LeaveCriticalSection(&handler->cs);

    if (found)
    {
        IMFAsyncResult_Release(found->result);
        if (found->object)
            IUnknown_Release(found->object);
        heap_free(found);
    }

    return found ? S_OK : MF_E_UNEXPECTED;
}

static const IMFSchemeHandlerVtbl file_scheme_handler_vtbl =
{
    file_scheme_handler_QueryInterface,
    file_scheme_handler_AddRef,
    file_scheme_handler_Release,
    file_scheme_handler_BeginCreateObject,
    file_scheme_handler_EndCreateObject,
    file_scheme_handler_CancelObjectCreation,
};

static HRESULT WINAPI file_scheme_handler_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI file_scheme_handler_callback_AddRef(IMFAsyncCallback *iface)
{
    struct file_scheme_handler *handler = impl_from_IMFAsyncCallback(iface);
    return IMFSchemeHandler_AddRef(&handler->IMFSchemeHandler_iface);
}

static ULONG WINAPI file_scheme_handler_callback_Release(IMFAsyncCallback *iface)
{
    struct file_scheme_handler *handler = impl_from_IMFAsyncCallback(iface);
    return IMFSchemeHandler_Release(&handler->IMFSchemeHandler_iface);
}

static HRESULT WINAPI file_scheme_handler_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT file_scheme_handler_get_resolver(struct file_scheme_handler *handler, IMFSourceResolver **resolver)
{
    HRESULT hr;

    if (!handler->resolver)
    {
        IMFSourceResolver *resolver;

        if (FAILED(hr = MFCreateSourceResolver(&resolver)))
            return hr;

        if (InterlockedCompareExchangePointer((void **)&handler->resolver, resolver, NULL))
            IMFSourceResolver_Release(resolver);
    }

    *resolver = handler->resolver;
    IMFSourceResolver_AddRef(*resolver);

    return S_OK;
}

static HRESULT WINAPI file_scheme_handler_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    static const WCHAR schemeW[] = {'f','i','l','e',':','/','/'};
    struct file_scheme_handler *handler = impl_from_IMFAsyncCallback(iface);
    struct file_scheme_handler_result *handler_result;
    MF_OBJECT_TYPE obj_type = MF_OBJECT_INVALID;
    IUnknown *object = NULL, *context_object;
    struct create_object_context *context;
    IMFSourceResolver *resolver;
    IMFAsyncResult *caller;
    IMFByteStream *stream;
    const WCHAR *url;
    HRESULT hr;

    caller = (IMFAsyncResult *)IMFAsyncResult_GetStateNoAddRef(result);

    if (FAILED(hr = IMFAsyncResult_GetObject(result, &context_object)))
    {
        WARN("Expected context set for callee result.\n");
        return hr;
    }

    context = impl_from_IUnknown(context_object);

    /* Strip from scheme, MFCreateFile() won't be expecting it. */
    url = context->url;
    if (!wcsnicmp(context->url, schemeW, ARRAY_SIZE(schemeW)))
        url += ARRAY_SIZE(schemeW);

    hr = MFCreateFile(context->flags & MF_RESOLUTION_WRITE ? MF_ACCESSMODE_READWRITE : MF_ACCESSMODE_READ,
            MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, url, &stream);
    if (SUCCEEDED(hr))
    {
        if (context->flags & MF_RESOLUTION_MEDIASOURCE)
        {
            if (SUCCEEDED(hr = file_scheme_handler_get_resolver(handler, &resolver)))
            {
                hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, context->url, context->flags,
                        context->props, &obj_type, &object);
                IMFSourceResolver_Release(resolver);
                IMFByteStream_Release(stream);
            }
        }
        else
        {
            object = (IUnknown *)stream;
            obj_type = MF_OBJECT_BYTESTREAM;
        }
    }

    handler_result = heap_alloc(sizeof(*handler_result));
    if (handler_result)
    {
        handler_result->result = caller;
        IMFAsyncResult_AddRef(handler_result->result);
        handler_result->obj_type = obj_type;
        handler_result->object = object;

        EnterCriticalSection(&handler->cs);
        list_add_tail(&handler->results, &handler_result->entry);
        LeaveCriticalSection(&handler->cs);
    }
    else
    {
        if (object)
            IUnknown_Release(object);
        hr = E_OUTOFMEMORY;
    }

    IUnknown_Release(&context->IUnknown_iface);

    IMFAsyncResult_SetStatus(caller, hr);
    MFInvokeCallback(caller);

    return S_OK;
}

static const IMFAsyncCallbackVtbl file_scheme_handler_callback_vtbl =
{
    file_scheme_handler_callback_QueryInterface,
    file_scheme_handler_callback_AddRef,
    file_scheme_handler_callback_Release,
    file_scheme_handler_callback_GetParameters,
    file_scheme_handler_callback_Invoke,
};

static HRESULT file_scheme_handler_construct(REFIID riid, void **obj)
{
    struct file_scheme_handler *handler;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(riid), obj);

    handler = heap_alloc_zero(sizeof(*handler));
    if (!handler)
        return E_OUTOFMEMORY;

    handler->IMFSchemeHandler_iface.lpVtbl = &file_scheme_handler_vtbl;
    handler->IMFAsyncCallback_iface.lpVtbl = &file_scheme_handler_callback_vtbl;
    handler->refcount = 1;
    list_init(&handler->results);
    InitializeCriticalSection(&handler->cs);

    hr = IMFSchemeHandler_QueryInterface(&handler->IMFSchemeHandler_iface, riid, obj);
    IMFSchemeHandler_Release(&handler->IMFSchemeHandler_iface);

    return hr;
}

static struct class_factory file_scheme_handler_factory = { { &class_factory_vtbl }, file_scheme_handler_construct };

static const struct class_object
{
    const GUID *clsid;
    IClassFactory *factory;
}
class_objects[] =
{
    { &CLSID_FileSchemePlugin, &file_scheme_handler_factory.IClassFactory_iface },
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

/******************************************************************
 *              DllCanUnloadNow (mf.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/***********************************************************************
 *          DllRegisterServer (mf.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( mf_instance );
}

/***********************************************************************
 *          DllUnregisterServer (mf.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( mf_instance );
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            mf_instance = instance;
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
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
    unsigned int capacity = 0, count, size;
    HRESULT hr = S_OK;
    int i, index;
    WCHAR *buffW;

    buffW = heap_calloc(maxlen, sizeof(*buffW));
    if (!buffW)
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

    heap_free(buffW);

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
HRESULT WINAPI MFEnumDeviceSources(IMFAttributes *attributes, IMFActivate ***sources, UINT32 *count)
{
    FIXME("%p, %p, %p.\n", attributes, sources, count);

    if (!attributes || !sources || !count)
        return E_INVALIDARG;

    *count = 0;

    return S_OK;
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

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI simple_type_handler_Release(IMFMediaTypeHandler *iface)
{
    struct simple_type_handler *handler = impl_from_IMFMediaTypeHandler(iface);
    ULONG refcount = InterlockedDecrement(&handler->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (handler->media_type)
            IMFMediaType_Release(handler->media_type);
        DeleteCriticalSection(&handler->cs);
        heap_free(handler);
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

    TRACE("%p, %u, %p.\n", iface, index, type);

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

HRESULT WINAPI MFCreateSimpleTypeHandler(IMFMediaTypeHandler **handler)
{
    struct simple_type_handler *object;

    TRACE("%p.\n", handler);

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFMediaTypeHandler_iface.lpVtbl = &simple_type_handler_vtbl;
    object->refcount = 1;
    InitializeCriticalSection(&object->cs);

    *handler = &object->IMFMediaTypeHandler_iface;

    return S_OK;
}
