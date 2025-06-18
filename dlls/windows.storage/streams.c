/* WinRT Windows.Storage.Streams Implementation
 *
 * Copyright (C) 2025 Mohamad Al-Jaf
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

WINE_DEFAULT_DEBUG_CHANNEL(storage);

struct random_access_stream_reference_statics
{
    IActivationFactory IActivationFactory_iface;
    IRandomAccessStreamReferenceStatics IRandomAccessStreamReferenceStatics_iface;
    LONG ref;
};

static inline struct random_access_stream_reference_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct random_access_stream_reference_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct random_access_stream_reference_statics *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_IRandomAccessStreamReferenceStatics ))
    {
        *out = &impl->IRandomAccessStreamReferenceStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct random_access_stream_reference_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct random_access_stream_reference_statics *impl = impl_from_IActivationFactory( iface );
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
    return S_OK;
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

DEFINE_IINSPECTABLE( random_access_stream_reference_statics, IRandomAccessStreamReferenceStatics, struct random_access_stream_reference_statics, IActivationFactory_iface )

static HRESULT WINAPI random_access_stream_reference_statics_CreateFromFile( IRandomAccessStreamReferenceStatics *iface, IStorageFile *file,
                                                                             IRandomAccessStreamReference **stream_reference )
{
    FIXME( "iface %p, file %p, stream_reference %p stub!\n", iface, file, stream_reference );
    return E_NOTIMPL;
}

static HRESULT WINAPI random_access_stream_reference_statics_CreateFromUri( IRandomAccessStreamReferenceStatics *iface, IUriRuntimeClass *uri,
                                                                            IRandomAccessStreamReference **stream_reference )
{
    FIXME( "iface %p, uri %p, stream_reference %p stub!\n", iface, uri, stream_reference );
    return E_NOTIMPL;
}

static HRESULT WINAPI random_access_stream_reference_statics_CreateFromStream( IRandomAccessStreamReferenceStatics *iface, IRandomAccessStream *stream,
                                                                               IRandomAccessStreamReference **stream_reference )
{
    FIXME( "iface %p, stream %p, stream_reference %p stub!\n", iface, stream, stream_reference );

    *stream_reference = NULL;

    if (!stream) return E_POINTER;
    return E_NOTIMPL;
}

static const struct IRandomAccessStreamReferenceStaticsVtbl random_access_stream_reference_statics_vtbl =
{
    random_access_stream_reference_statics_QueryInterface,
    random_access_stream_reference_statics_AddRef,
    random_access_stream_reference_statics_Release,
    /* IInspectable methods */
    random_access_stream_reference_statics_GetIids,
    random_access_stream_reference_statics_GetRuntimeClassName,
    random_access_stream_reference_statics_GetTrustLevel,
    /* IRandomAccessStreamReferenceStatics methods */
    random_access_stream_reference_statics_CreateFromFile,
    random_access_stream_reference_statics_CreateFromUri,
    random_access_stream_reference_statics_CreateFromStream,
};

static struct random_access_stream_reference_statics random_access_stream_reference_statics =
{
    {&factory_vtbl},
    {&random_access_stream_reference_statics_vtbl},
    0,
};

IActivationFactory *random_access_stream_reference_factory = &random_access_stream_reference_statics.IActivationFactory_iface;
