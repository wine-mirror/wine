/* CryptoWinRT Credentials Implementation
 *
 * Copyright (C) 2022 Mohamad Al-Jaf
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

#include "private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypto);

struct credentials_statics
{
    IActivationFactory IActivationFactory_iface;
    IKeyCredentialManagerStatics IKeyCredentialManagerStatics_iface;
    LONG ref;
};

static inline struct credentials_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct credentials_statics, IActivationFactory_iface );
}

static HRESULT WINAPI credentials_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct credentials_statics *factory = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IUnknown_AddRef( iface );
        *out = &factory->IActivationFactory_iface;
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IKeyCredentialManagerStatics ))
    {
        IUnknown_AddRef( iface );
        *out = &factory->IKeyCredentialManagerStatics_iface;
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI credentials_AddRef( IActivationFactory *iface )
{
    struct credentials_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI credentials_Release( IActivationFactory *iface )
{
    struct credentials_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI credentials_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI credentials_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI credentials_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI credentials_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "iface %p, instance %p stub!\n", iface, instance );
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    credentials_QueryInterface,
    credentials_AddRef,
    credentials_Release,
    /* IInspectable methods */
    credentials_GetIids,
    credentials_GetRuntimeClassName,
    credentials_GetTrustLevel,
    /* IActivationFactory methods */
    credentials_ActivateInstance,
};

DEFINE_IINSPECTABLE( credentials_statics, IKeyCredentialManagerStatics, struct credentials_statics, IActivationFactory_iface );

static HRESULT is_supported_async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result )
{
    result->vt = VT_BOOL;
    result->boolVal = FALSE;
    return S_OK;
}

static HRESULT WINAPI credentials_statics_IsSupportedAsync( IKeyCredentialManagerStatics *iface, IAsyncOperation_boolean **value )
{
    TRACE( "iface %p, value %p.\n", iface, value );
    return async_operation_boolean_create( (IUnknown *)iface, NULL, is_supported_async, value );
}

static HRESULT WINAPI credentials_statics_RenewAttestationAsync( IKeyCredentialManagerStatics *iface, IAsyncAction **operation )
{
    FIXME( "iface %p, operation %p stub!\n", iface, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI credentials_statics_RequestCreateAsync( IKeyCredentialManagerStatics *iface,
        HSTRING name, KeyCredentialCreationOption option, IAsyncOperation_KeyCredentialRetrievalResult **value )
{
    FIXME( "iface %p, name %s, %d option, %p value stub!\n", iface, debugstr_hstring(name), option, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI credentials_statics_OpenAsync(IKeyCredentialManagerStatics *iface,
        HSTRING name, IAsyncOperation_KeyCredentialRetrievalResult **value)
{
    FIXME( "iface %p, name %s, %p value stub!\n", iface, debugstr_hstring(name), value );
    return E_NOTIMPL;
}

static HRESULT WINAPI credentials_statics_DeleteAsync( IKeyCredentialManagerStatics *iface, HSTRING name, IAsyncAction **operation )
{
    FIXME( "iface %p, name %s, %p operation stub!\n", iface, debugstr_hstring(name), operation );
    return E_NOTIMPL;
}

static const struct IKeyCredentialManagerStaticsVtbl credentials_statics_vtbl =
{
    credentials_statics_QueryInterface,
    credentials_statics_AddRef,
    credentials_statics_Release,
    /* IInspectable methods */
    credentials_statics_GetIids,
    credentials_statics_GetRuntimeClassName,
    credentials_statics_GetTrustLevel,
    /* IKeyCredentialManagerStatics methods */
    credentials_statics_IsSupportedAsync,
    credentials_statics_RenewAttestationAsync,
    credentials_statics_RequestCreateAsync,
    credentials_statics_OpenAsync,
    credentials_statics_DeleteAsync,
};

static struct credentials_statics credentials_statics =
{
    {&factory_vtbl},
    {&credentials_statics_vtbl},
    1,
};

IActivationFactory *credentials_activation_factory = &credentials_statics.IActivationFactory_iface;
