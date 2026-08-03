/* WinRT Windows.System.Profile.HardwareIdentification Implementation
 *
 * Copyright (C) 2024 Onni Kukkonen
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

WINE_DEFAULT_DEBUG_CHANNEL(hwid);

struct hwid_statics
{
    IActivationFactory IActivationFactory_iface;
    IHardwareIdentificationStatics IHardwareIdentificationStatics_iface;
    LONG ref;
};

static inline struct hwid_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct hwid_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct hwid_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IHardwareIdentificationStatics ))
    {
        *out = &impl->IHardwareIdentificationStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct hwid_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct hwid_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
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

DEFINE_IINSPECTABLE( statics, IHardwareIdentificationStatics, struct hwid_statics, IActivationFactory_iface )

struct hardware_token {
    IHardwareToken IHardwareToken_iface;
    LONG ref;
};

static inline struct hardware_token *impl_from_IHardwareToken( IHardwareToken *iface )
{
    return CONTAINING_RECORD( iface, struct hardware_token, IHardwareToken_iface );
}

static HRESULT WINAPI hardware_token_QueryInterface( IHardwareToken *iface, REFIID iid, void **out )
{
    struct hardware_token *impl = impl_from_IHardwareToken( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IHardwareToken ))
    {
        *out = &impl->IHardwareToken_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI hardware_token_AddRef( IHardwareToken *iface )
{
    struct hardware_token *impl = impl_from_IHardwareToken( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI hardware_token_Release( IHardwareToken *iface )
{
    struct hardware_token *impl = impl_from_IHardwareToken( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI hardware_token_GetIids( IHardwareToken *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI hardware_token_GetRuntimeClassName( IHardwareToken *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI hardware_token_GetTrustLevel( IHardwareToken *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI hardware_token_get_Id( IHardwareToken *iface, IBuffer** value) {
    struct buffer_impl *buf = alloc_buffer(12);

    FIXME("iface: %p, value: %p stub!\n", iface, value);
    buf->dataptr[0] = '\1';
    buf->dataptr[1] = '\0';
    buf->dataptr[2] = '\4';
    buf->dataptr[3] = '\0';

    buf->dataptr[4] = '\5';
    buf->dataptr[5] = '\0';
    buf->dataptr[6] = '\1';
    buf->dataptr[7] = '\2';
    
    buf->dataptr[8] = '\3';
    buf->dataptr[9] = '\0';
    buf->dataptr[10] = '\24';
    buf->dataptr[11] = '\211';
    *value = &buf->IBuffer_iface;
    return S_OK;
}

static HRESULT WINAPI hardware_token_get_Signature( IHardwareToken *iface, IBuffer** value) {
    FIXME("iface: %p, value: %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI hardware_token_get_Certificate( IHardwareToken *iface, IBuffer** value) {
    FIXME("iface: %p, value: %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static const struct IHardwareTokenVtbl hardware_token_vtbl =
{
    hardware_token_QueryInterface,
    hardware_token_AddRef,
    hardware_token_Release,
    /* IInspectable methods */
    hardware_token_GetIids,
    hardware_token_GetRuntimeClassName,
    hardware_token_GetTrustLevel,
    /* IHardwareToken methods */
    hardware_token_get_Id,
    hardware_token_get_Signature,
    hardware_token_get_Certificate
};


static HRESULT create_hardware_token(IHardwareToken** value) {
    struct hardware_token *impl;

    if (!value) return E_INVALIDARG;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IHardwareToken_iface.lpVtbl = &hardware_token_vtbl;
    impl->ref = 1;

    *value = &impl->IHardwareToken_iface;
    TRACE( "created IHardwareToken %p.\n", *value );
    return S_OK;
}

static HRESULT WINAPI statics_GetPackageSpecificToken( IHardwareIdentificationStatics *iface, IBuffer *nonce, IHardwareToken** packageSpecificHardwareToken)
{
    FIXME( "iface %p, nonce %p, packageSpecificHardwareToken %p stub!\n", iface, nonce, packageSpecificHardwareToken );
    if (nonce != NULL) {
        ERR("Nonce not supported!\n");
    }
    
    return create_hardware_token(packageSpecificHardwareToken);
}

static const struct IHardwareIdentificationStaticsVtbl statics_vtbl =
{
    statics_QueryInterface,
    statics_AddRef,
    statics_Release,
    /* IInspectable methods */
    statics_GetIids,
    statics_GetRuntimeClassName,
    statics_GetTrustLevel,
    /* IHardwareIdentificationStatics methods */
    statics_GetPackageSpecificToken,
};

static struct hwid_statics hwid_statics =
{
    {&factory_vtbl},
    {&statics_vtbl},
    1,
};

static IActivationFactory *smbios_factory = &hwid_statics.IActivationFactory_iface;

HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID riid, void **out )
{
    FIXME( "clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out );
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory( HSTRING classid, IActivationFactory **factory )
{
    const WCHAR *name = WindowsGetStringRawBuffer( classid, NULL );

    TRACE( "classid %s, factory %p.\n", debugstr_hstring(classid), factory );

    *factory = NULL;

    if (!wcscmp( name, RuntimeClass_Windows_System_Profile_HardwareIdentification ))
        IActivationFactory_QueryInterface( smbios_factory, &IID_IActivationFactory, (void **)factory );

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}
