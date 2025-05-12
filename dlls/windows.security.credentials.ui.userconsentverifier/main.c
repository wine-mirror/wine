/* WinRT Windows.Security.Credentials.UI Implementation
 *
 * Copyright (C) 2023 Mohamad Al-Jaf
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(credentials);

struct user_consent_verifier_statics
{
    IActivationFactory IActivationFactory_iface;
    IUserConsentVerifierStatics IUserConsentVerifierStatics_iface;
    LONG ref;
};

static inline struct user_consent_verifier_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct user_consent_verifier_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct user_consent_verifier_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IUserConsentVerifierStatics ))
    {
        *out = &impl->IUserConsentVerifierStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct user_consent_verifier_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct user_consent_verifier_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "iface %p, instance %p stub!\n", iface, instance );
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

DEFINE_IINSPECTABLE( user_consent_verifier_statics, IUserConsentVerifierStatics, struct user_consent_verifier_statics, IActivationFactory_iface )

static HRESULT check_availability_async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result )
{
    result->vt = VT_UI4;
    result->ulVal = UserConsentVerifierAvailability_DeviceNotPresent;
    return S_OK;
}

static HRESULT WINAPI user_consent_verifier_statics_CheckAvailabilityAsync( IUserConsentVerifierStatics *iface, IAsyncOperation_UserConsentVerifierAvailability **result )
{
    TRACE( "iface %p, result %p\n", iface, result );
    return async_operation_user_consent_verifier_availability_create( (IUnknown *)iface, NULL, check_availability_async, result );
}

static HRESULT WINAPI user_consent_verifier_statics_RequestVerificationAsync( IUserConsentVerifierStatics *iface, HSTRING message,
    IAsyncOperation_UserConsentVerificationResult **result )
{
    FIXME( "iface %p, message %s, result %p stub!\n", iface, debugstr_hstring( message ), result );
    return E_NOTIMPL;
}

static const struct IUserConsentVerifierStaticsVtbl user_consent_verifier_statics_vtbl =
{
    user_consent_verifier_statics_QueryInterface,
    user_consent_verifier_statics_AddRef,
    user_consent_verifier_statics_Release,
    /* IInspectable methods */
    user_consent_verifier_statics_GetIids,
    user_consent_verifier_statics_GetRuntimeClassName,
    user_consent_verifier_statics_GetTrustLevel,
    /* IUserConsentVerifierStatics methods */
    user_consent_verifier_statics_CheckAvailabilityAsync,
    user_consent_verifier_statics_RequestVerificationAsync,
};

static struct user_consent_verifier_statics user_consent_verifier_statics =
{
    {&factory_vtbl},
    {&user_consent_verifier_statics_vtbl},
    1,
};

static IActivationFactory *user_consent_verifier_factory = &user_consent_verifier_statics.IActivationFactory_iface;

HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID riid, void **out )
{
    FIXME( "clsid %s, riid %s, out %p stub!\n", debugstr_guid( clsid ), debugstr_guid( riid ), out );
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory( HSTRING classid, IActivationFactory **factory )
{
    const WCHAR *buffer = WindowsGetStringRawBuffer( classid, NULL );

    TRACE( "class %s, factory %p.\n", debugstr_hstring( classid ), factory );

    *factory = NULL;

    if (!wcscmp( buffer, RuntimeClass_Windows_Security_Credentials_UI_UserConsentVerifier ))
        IActivationFactory_QueryInterface( user_consent_verifier_factory, &IID_IActivationFactory, (void **)factory );

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}
