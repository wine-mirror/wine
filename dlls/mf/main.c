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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "mfidl.h"
#include "rpcproxy.h"

#include "initguid.h"
#include "mf.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static HINSTANCE mf_instance;

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

struct file_scheme_handler
{
    IMFSchemeHandler IMFSchemeHandler_iface;
    LONG refcount;
};

static struct file_scheme_handler *impl_from_IMFSchemeHandler(IMFSchemeHandler *iface)
{
    return CONTAINING_RECORD(iface, struct file_scheme_handler, IMFSchemeHandler_iface);
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

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
        heap_free(handler);

    return refcount;
}

static HRESULT WINAPI file_scheme_handler_BeginCreateObject(IMFSchemeHandler *iface, const WCHAR *url, DWORD flags,
        IPropertyStore *props, IUnknown **cancel_cookie, IMFAsyncCallback *callback, IUnknown *state)
{
    FIXME("%p, %s, %#x, %p, %p, %p, %p.\n", iface, debugstr_w(url), flags, props, cancel_cookie, callback, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_scheme_handler_EndCreateObject(IMFSchemeHandler *iface, IMFAsyncResult *result,
        MF_OBJECT_TYPE *obj_type, IUnknown **object)
{
    FIXME("%p, %p, %p, %p.\n", iface, result, obj_type, object);

    return E_NOTIMPL;
}

static HRESULT WINAPI file_scheme_handler_CancelObjectCreation(IMFSchemeHandler *iface, IUnknown *cancel_cookie)
{
    FIXME("%p, %p.\n", iface, cancel_cookie);

    return E_NOTIMPL;
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

static HRESULT file_scheme_handler_construct(REFIID riid, void **obj)
{
    struct file_scheme_handler *handler;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(riid), obj);

    handler = heap_alloc(sizeof(*handler));
    if (!handler)
        return E_OUTOFMEMORY;

    handler->IMFSchemeHandler_iface.lpVtbl = &file_scheme_handler_vtbl;
    handler->refcount = 1;

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
    { &CLSID_FileSchemeHandler, &file_scheme_handler_factory.IClassFactory_iface },
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

/***********************************************************************
 *      MFGetSupportedMimeTypes (mf.@)
 */
HRESULT WINAPI MFGetSupportedMimeTypes(PROPVARIANT *array)
{
    FIXME("(%p) stub\n", array);

    return E_NOTIMPL;
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
