/* WinRT Windows.Devices.Enumeration.Pnp implementation
 *
 * Copyright 2024 Onni Kukkonen
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

#include "initguid.h"
#include "private.h"
#include <assert.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(enumerationpnp);

struct pnpstatic
{
    IActivationFactory IActivationFactory_iface;
    IPnpObjectStatics IPnpObjectStatics_iface;
    LONG ref;
};

static inline struct pnpstatic *impl_from_IPnpObjectStatics( IPnpObjectStatics *iface )
{
    return CONTAINING_RECORD( iface, struct pnpstatic, IPnpObjectStatics_iface );
}

static HRESULT WINAPI pnpstatic_QueryInterface( IPnpObjectStatics *iface, REFIID iid, void **out )
{
    struct pnpstatic *impl = impl_from_IPnpObjectStatics( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IPnpObjectStatics ))
    {
        *out = &impl->IPnpObjectStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IPnpObjectStatics ))
    {
        *out = &impl->IPnpObjectStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI pnpstatic_AddRef( IPnpObjectStatics *iface )
{
    struct pnpstatic *impl = impl_from_IPnpObjectStatics( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI pnpstatic_Release( IPnpObjectStatics *iface )
{
    struct pnpstatic *impl = impl_from_IPnpObjectStatics( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI pnpstatic_GetIids( IPnpObjectStatics *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI pnpstatic_GetRuntimeClassName( IPnpObjectStatics *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI pnpstatic_GetTrustLevel( IPnpObjectStatics *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

struct pnpobj_impl {
    PnpObjectType type;
    HSTRING id;
    IPnpObject IPnpObject_iface;
    IMapView_HSTRING_IInspectable IMapView_iface;
    LONG ref;
};

DEFINE_IINSPECTABLE( mapview, IMapView_HSTRING_IInspectable, struct pnpobj_impl, IMapView_iface );

static HRESULT WINAPI mapview_Lookup( IMapView_HSTRING_IInspectable *iface, HSTRING key, IInspectable **value)
{
    FIXME( "iface %p, key %s, value %p stub!\n", iface, debugstr_hstring(key), value );
    return E_NOTIMPL;
}

static HRESULT WINAPI mapview_get_Size( IMapView_HSTRING_IInspectable *iface, unsigned int *size)
{
    FIXME( "iface %p, size %p stub!\n", iface, size );
    *size = 0;
    return S_OK;
}

static HRESULT WINAPI mapview_HasKey( IMapView_HSTRING_IInspectable *iface, HSTRING key, boolean *found)
{
    FIXME( "iface %p, key %s, found %p stub!\n", iface, debugstr_hstring(key), found );
    *found = FALSE;
    return S_OK;
}

static HRESULT WINAPI mapview_Split( IMapView_HSTRING_IInspectable *iface, IMapView_HSTRING_IInspectable **first, IMapView_HSTRING_IInspectable **second)
{
    FIXME( "iface %p, first %p, second %p stub!\n", iface, first, second );
    return E_NOTIMPL;
}

static const struct IMapView_HSTRING_IInspectableVtbl mapview_vtbl =
{
    mapview_QueryInterface,
    mapview_AddRef,
    mapview_Release,
    /* IInspectable methods */
    mapview_GetIids,
    mapview_GetRuntimeClassName,
    mapview_GetTrustLevel,
    /* IMapView_HSTRING_IInspectable methods */
    mapview_Lookup,
    mapview_get_Size,
    mapview_HasKey,
    mapview_Split
};

static inline struct pnpobj_impl *impl_from_IPnpObject( IPnpObject *iface )
{
    return CONTAINING_RECORD( iface, struct pnpobj_impl, IPnpObject_iface );
}

static HRESULT WINAPI pnpobj_QueryInterface( IPnpObject *iface, REFIID iid, void **out )
{
    struct pnpobj_impl *impl = impl_from_IPnpObject( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IPnpObject ))
    {
        *out = &impl->IPnpObject_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IPnpObject ))
    {
        *out = &impl->IPnpObject_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI pnpobj_AddRef( IPnpObject *iface )
{
    struct pnpobj_impl *impl = impl_from_IPnpObject( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI pnpobj_Release( IPnpObject *iface )
{
    struct pnpobj_impl *impl = impl_from_IPnpObject( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI pnpobj_GetIids( IPnpObject *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI pnpobj_GetRuntimeClassName( IPnpObject *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI pnpobj_GetTrustLevel( IPnpObject *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI pnpobj_get_Type( IPnpObject *iface, PnpObjectType* value) 
{
    struct pnpobj_impl *impl = impl_from_IPnpObject( iface );
    *value = impl->type;
    return S_OK;
}

static HRESULT WINAPI pnpobj_get_Id( IPnpObject *iface, HSTRING* value) 
{
    struct pnpobj_impl *impl = impl_from_IPnpObject( iface );
    *value = impl->id;
    return S_OK;
}

static HRESULT WINAPI pnpobj_get_Properties( IPnpObject *iface, IMapView_HSTRING_IInspectable** value) 
{
    struct pnpobj_impl *impl = impl_from_IPnpObject( iface );
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = &impl->IMapView_iface;
    return S_OK;
}

static HRESULT WINAPI pnpobj_Update( IPnpObject *iface, __x_ABI_CWindows_CDevices_CEnumeration_CPnp_CIPnpObjectUpdate *updateInfo) 
{
    FIXME( "iface %p, updateInfo %p stub!\n", iface, updateInfo );
    return E_NOTIMPL;
}

static const struct IPnpObjectVtbl pnpobj_vtbl =
{
    pnpobj_QueryInterface,
    pnpobj_AddRef,
    pnpobj_Release,
    /* IInspectable methods */
    pnpobj_GetIids,
    pnpobj_GetRuntimeClassName,
    pnpobj_GetTrustLevel,
    /* IPnpObject methods */
    pnpobj_get_Type,
    pnpobj_get_Id,
    pnpobj_get_Properties,
    pnpobj_Update
};

static HRESULT create_from_id_async( IInspectable *invoker, IInspectable **result )
{
    struct pnpobj_impl *impl;
    HRESULT hr;

    FIXME("invoker %p, result %p semi-stub!\n", invoker, result);

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        *result = NULL;
        return E_OUTOFMEMORY;
    }

    impl->IPnpObject_iface.lpVtbl = &pnpobj_vtbl;
    impl->IMapView_iface.lpVtbl = &mapview_vtbl;
    impl->ref = 1;
    hr = WindowsCreateString(NULL, 0, &impl->id);
    if (hr != S_OK) {
        free(impl);
        ERR("WindowsCreateString failed\n");
        return hr;
    }

    TRACE("created IPnpObject %p.\n", impl);
    *result = (IInspectable*)&impl->IPnpObject_iface;
    return S_OK;
}

static HRESULT find_all_async( IInspectable *invoker, IInspectable **result )
{
    static const struct vector_iids iids =
    {
        .vector = &IID_IVector_PnpObject,
        .view = &IID_IVectorView_PnpObject,
        .iterable = &IID_IIterable_PnpObject,
        .iterator = &IID_IIterator_PnpObject,
    };

    FIXME("invoker %p, result %p semi-stub!\n", invoker, result);
    return vector_create( &iids, (void **)result );
}

static HRESULT WINAPI pnpstatic_CreateFromIdAsync( IPnpObjectStatics *iface, PnpObjectType type, HSTRING id, IIterable_HSTRING* requestedProperties, IAsyncOperation_PnpObject** asyncOp)
{
    FIXME( "iface %p, type %04x, id %s, requestedProperties %p, asyncOp %p semi-stub!\n", iface, type, debugstr_hstring(id), requestedProperties, asyncOp );
    return async_operation_inspectable_create(&IID_IAsyncOperation_PnpObject, NULL, create_from_id_async, (IAsyncOperation_IInspectable **)asyncOp);
}

static HRESULT WINAPI pnpstatic_FindAllAsync( IPnpObjectStatics *iface, PnpObjectType type, IIterable_HSTRING* requestedProperties, IAsyncOperation_PnpObjectCollection** asyncOp ) 
{
    FIXME( "iface %p, type %04x, requestedProperties %p, asyncOp %p stub!\n", iface, type, requestedProperties, asyncOp );
    return async_operation_inspectable_create(&IID_IAsyncOperation_PnpObjectCollection, NULL, find_all_async, (IAsyncOperation_IInspectable **)asyncOp);
}

static HRESULT WINAPI pnpstatic_FindAllAsyncAqsFilter( IPnpObjectStatics *iface, PnpObjectType type, IIterable_HSTRING* requestedProperties, HSTRING aqsFilter, IAsyncOperation_PnpObjectCollection** asyncOp ) 
{
    FIXME( "iface %p, type %04x, requestedProperties %p, aqsFilter %s, asyncOp %p stub!\n", iface, type, requestedProperties, debugstr_hstring(aqsFilter), asyncOp );
    //TODO: Filter not implemented
    return async_operation_inspectable_create(&IID_IAsyncOperation_PnpObjectCollection, NULL, find_all_async, (IAsyncOperation_IInspectable **)asyncOp);
}

static HRESULT WINAPI pnpstatic_CreateWatcher( IPnpObjectStatics *iface, PnpObjectType type, IIterable_HSTRING* requestedProperties, __x_ABI_CWindows_CDevices_CEnumeration_CPnp_CIPnpObjectWatcher** watcher)
{
    FIXME( "iface %p, type %04x, requestedProperties %p, watcher %p stub!\n", iface, type, requestedProperties, watcher);
    return E_NOTIMPL;
}

static HRESULT WINAPI pnpstatic_CreateWatcherAqsFilter( IPnpObjectStatics *iface, PnpObjectType type, IIterable_HSTRING* requestedProperties, HSTRING aqsFilter, __x_ABI_CWindows_CDevices_CEnumeration_CPnp_CIPnpObjectWatcher** watcher)
{
    FIXME( "iface %p, type %04x, requestedProperties %p, aqsFilter %s, watcher %p stub!\n", iface, type, requestedProperties, debugstr_hstring(aqsFilter), watcher);
    return E_NOTIMPL;
}

//DEFINE_IINSPECTABLE(pnpstatic, IPnpObjectStatics, struct pnpstatic, IActivationFactory_iface);

static const struct IPnpObjectStaticsVtbl pnpstatic_vtbl =
{
    pnpstatic_QueryInterface,
    pnpstatic_AddRef,
    pnpstatic_Release,
    /* IInspectable methods */
    pnpstatic_GetIids,
    pnpstatic_GetRuntimeClassName,
    pnpstatic_GetTrustLevel,
    /* IPnpObjectStatics methods */
    pnpstatic_CreateFromIdAsync,
    pnpstatic_FindAllAsync,
    pnpstatic_FindAllAsyncAqsFilter,
    pnpstatic_CreateWatcher,
    pnpstatic_CreateWatcherAqsFilter
};

static inline struct pnpstatic *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct pnpstatic, IActivationFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct pnpstatic *factory = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IActivationFactory_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IPnpObjectStatics))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IPnpObjectStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IActivationFactory *iface)
{
    struct pnpstatic *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI factory_Release(IActivationFactory *iface)
{
    struct pnpstatic *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI factory_GetIids(
        IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName(
        IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel(
        IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance(
        IActivationFactory *iface, IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable methods */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IActivationFactory methods */
    factory_ActivateInstance,
};

static struct pnpstatic pnpstatic_global =
{
    .IActivationFactory_iface.lpVtbl = &factory_vtbl,
    .IPnpObjectStatics_iface.lpVtbl = &pnpstatic_vtbl,
    .ref = 1,
};

IActivationFactory *activation_factory = &pnpstatic_global.IActivationFactory_iface;

HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID riid, void **out )
{
    FIXME( "clsid %s, riid %s, out %p stub!\n", debugstr_guid( clsid ), debugstr_guid( riid ), out );
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory( HSTRING classid, IActivationFactory **factory )
{
    const WCHAR *buffer = WindowsGetStringRawBuffer( classid, NULL );

    TRACE( "class %s, factory %p.\n", debugstr_hstring(classid), factory );

    *factory = NULL;

    if (!wcscmp(buffer, RuntimeClass_Windows_Devices_Enumeration_Pnp_PnpObject))
        IActivationFactory_QueryInterface( activation_factory, &IID_IActivationFactory, (void **)factory );

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}
