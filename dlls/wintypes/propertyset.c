/*
 * Copyright 2024 Vibhav Pant
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
#include <winstring.h>
#include <objbase.h>
#include <wine/debug.h>
#include <activation.h>

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL( wintypes );

struct propertyset
{
    IPropertySet IPropertySet_iface;
    IObservableMap_HSTRING_IInspectable IObservableMap_HSTRING_IInspectable_iface;
    LONG ref;
};

static inline struct propertyset *impl_from_IPropertySet( IPropertySet *iface )
{
    return CONTAINING_RECORD( iface, struct propertyset, IPropertySet_iface );
}

static HRESULT STDMETHODCALLTYPE propertyset_QueryInterface( IPropertySet *iface, REFIID iid, void **out )
{
    struct propertyset *impl = impl_from_IPropertySet( iface );

    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );

    *out = NULL;
    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IPropertySet ))
    {
        *out = iface;
        IUnknown_AddRef( (IUnknown *)*out );
        return S_OK;
    }
    else if (IsEqualGUID( iid, &IID_IObservableMap_HSTRING_IInspectable ))
    {
        *out = &impl->IObservableMap_HSTRING_IInspectable_iface;
        IUnknown_AddRef( (IUnknown *)*out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE propertyset_AddRef( IPropertySet *iface )
{
    struct propertyset *impl = impl_from_IPropertySet( iface );

    TRACE( "(%p)\n", iface );

    return InterlockedIncrement( &impl->ref );
}

static ULONG STDMETHODCALLTYPE propertyset_Release( IPropertySet *iface )
{
    struct propertyset *impl = impl_from_IPropertySet( iface );
    ULONG ref;

    TRACE( "(%p)\n", iface );

    ref = InterlockedDecrement( &impl->ref );
    if (!ref)
        free( impl );
    return ref;
}

static HRESULT STDMETHODCALLTYPE propertyset_GetIids( IPropertySet *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "(%p, %p, %p) stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE propertyset_GetRuntimeClassName( IPropertySet *iface, HSTRING *class_name )
{
    FIXME( "(%p, %p) stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE propertyset_GetTrustLevel( IPropertySet *iface, TrustLevel *trust_level )
{
    FIXME( "(%p, %p) stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static const IPropertySetVtbl propertyset_vtbl =
{
    /* IUnknown */
    propertyset_QueryInterface,
    propertyset_AddRef,
    propertyset_Release,
    /* IInspectable */
    propertyset_GetIids,
    propertyset_GetRuntimeClassName,
    propertyset_GetTrustLevel,
};

DEFINE_IINSPECTABLE( propertyset_IObservableMap, IObservableMap_HSTRING_IInspectable, struct propertyset, IPropertySet_iface );

static HRESULT STDMETHODCALLTYPE propertyset_IObservableMap_add_MapChanged( IObservableMap_HSTRING_IInspectable *iface,
                                                                            IMapChangedEventHandler_HSTRING_IInspectable *handler,
                                                                            EventRegistrationToken *token )
{
    FIXME( "(%p, %p, %p) stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE propertyset_IObservableMap_remove_MapChanged( IObservableMap_HSTRING_IInspectable *iface, EventRegistrationToken token )
{
    FIXME( "(%p, %I64d) stub!\n", iface, token.value );
    return E_NOTIMPL;
}

const static IObservableMap_HSTRING_IInspectableVtbl propertyset_IObservableMap_vtbl =
{
    /* IUnknown */
    propertyset_IObservableMap_QueryInterface,
    propertyset_IObservableMap_AddRef,
    propertyset_IObservableMap_Release,
    /* IInspectable */
    propertyset_IObservableMap_GetIids,
    propertyset_IObservableMap_GetRuntimeClassName,
    propertyset_IObservableMap_GetTrustLevel,
    /* IObservableMap<HSTRING, IInspectable*> */
    propertyset_IObservableMap_add_MapChanged,
    propertyset_IObservableMap_remove_MapChanged,
};

struct propertyset_factory
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct propertyset_factory *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct propertyset_factory, IActivationFactory_iface );
}

static HRESULT STDMETHODCALLTYPE factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );
    *out = NULL;
    if (IsEqualGUID( &IID_IUnknown, iid ) ||
        IsEqualGUID( &IID_IInspectable, iid ) ||
        IsEqualGUID( &IID_IAgileObject, iid ) ||
        IsEqualGUID( &IID_IActivationFactory, iid ))
    {
        *out = iface;
        IUnknown_AddRef( (IUnknown *)*out );
        return S_OK;
    }

    FIXME( "%s not implemented, return E_NOINTERFACE.\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE factory_AddRef( IActivationFactory *iface )
{
    struct propertyset_factory *impl = impl_from_IActivationFactory( iface );
    TRACE( "(%p)\n", iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG STDMETHODCALLTYPE factory_Release( IActivationFactory *iface )
{
    struct propertyset_factory *impl = impl_from_IActivationFactory( iface );
    TRACE( "(%p)\n", iface );
    return InterlockedDecrement( &impl->ref );
}

static HRESULT STDMETHODCALLTYPE factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "(%p, %p, %p) stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "(%p, %p) stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "(%p, %p) stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    struct propertyset *impl;

    TRACE( "(%p, %p)\n", iface, instance );

    impl = calloc( 1, sizeof( *impl ) );
    if (!impl)
        return E_OUTOFMEMORY;

    impl->IPropertySet_iface.lpVtbl = &propertyset_vtbl;
    impl->IObservableMap_HSTRING_IInspectable_iface.lpVtbl = &propertyset_IObservableMap_vtbl;
    impl->ref = 1;
    *instance = (IInspectable *)&impl->IPropertySet_iface;
    return S_OK;
}

static const IActivationFactoryVtbl propertyset_factory_vtbl =
{
    /* IUnknown */
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IActivationFactory */
    factory_ActivateInstance,
};

static struct propertyset_factory propertyset_factory =
{
    {&propertyset_factory_vtbl},
    1
};

IActivationFactory *property_set_factory = &propertyset_factory.IActivationFactory_iface;
