/* WinRT Windows.Globalization implementation
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(locale);

struct geographic_region
{
    IGeographicRegion IGeographicRegion_iface;
    LONG ref;
};

static inline struct geographic_region *impl_from_IGeographicRegion( IGeographicRegion *iface )
{
    return CONTAINING_RECORD( iface, struct geographic_region, IGeographicRegion_iface );
}

static HRESULT WINAPI geographic_region_QueryInterface( IGeographicRegion *iface, REFIID iid, void **out )
{
    struct geographic_region *impl = impl_from_IGeographicRegion( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IGeographicRegion ))
    {
        IInspectable_AddRef( (*out = &impl->IGeographicRegion_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI geographic_region_AddRef( IGeographicRegion *iface )
{
    struct geographic_region *impl = impl_from_IGeographicRegion( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI geographic_region_Release( IGeographicRegion *iface )
{
    struct geographic_region *impl = impl_from_IGeographicRegion( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI geographic_region_GetIids( IGeographicRegion *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI geographic_region_GetRuntimeClassName( IGeographicRegion *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI geographic_region_GetTrustLevel( IGeographicRegion *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI geographic_region_get_Code( IGeographicRegion *iface, HSTRING *value )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI geographic_region_get_CodeTwoLetter( IGeographicRegion *iface, HSTRING *value )
{
    WCHAR buffer[LOCALE_NAME_MAX_LENGTH];

    FIXME( "iface %p semi-stub!\n", iface );

    if (!GetLocaleInfoEx( LOCALE_NAME_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, buffer, ARRAY_SIZE(buffer) )) return E_INVALIDARG;
    return WindowsCreateString( buffer, wcslen( buffer ), value );
}

static HRESULT WINAPI geographic_region_get_CodeThreeLetter( IGeographicRegion *iface, HSTRING *value )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI geographic_region_get_CodeThreeDigit( IGeographicRegion *iface, HSTRING *value )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI geographic_region_get_DisplayName( IGeographicRegion *iface, HSTRING *value )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI geographic_region_get_NativeName( IGeographicRegion *iface, HSTRING *value )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI geographic_region_get_CurrenciesInUse( IGeographicRegion *iface, IVectorView_HSTRING **value )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static const struct IGeographicRegionVtbl geographic_region_vtbl =
{
    geographic_region_QueryInterface,
    geographic_region_AddRef,
    geographic_region_Release,
    /* IInspectable methods */
    geographic_region_GetIids,
    geographic_region_GetRuntimeClassName,
    geographic_region_GetTrustLevel,
    /* IGeographicRegion methods */
    geographic_region_get_Code,
    geographic_region_get_CodeTwoLetter,
    geographic_region_get_CodeThreeLetter,
    geographic_region_get_CodeThreeDigit,
    geographic_region_get_DisplayName,
    geographic_region_get_NativeName,
    geographic_region_get_CurrenciesInUse,
};

struct geographic_region_factory
{
    IActivationFactory IActivationFactory_iface;
    IGeographicRegionFactory IGeographicRegionFactory_iface;
    LONG ref;
};

static inline struct geographic_region_factory *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct geographic_region_factory, IActivationFactory_iface );
}

static HRESULT WINAPI activation_factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct geographic_region_factory *factory = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IActivationFactory_AddRef( (*out = &factory->IActivationFactory_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IGeographicRegionFactory ))
    {
        IGeographicRegionFactory_AddRef( (*out = &factory->IGeographicRegionFactory_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activation_factory_AddRef( IActivationFactory *iface )
{
    struct geographic_region_factory *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI activation_factory_Release( IActivationFactory *iface )
{
    struct geographic_region_factory *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI activation_factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_ActivateInstance( IActivationFactory *iface, IInspectable **out )
{
    struct geographic_region *region;

    TRACE( "iface %p, out %p.\n", iface, out );

    if (!(region = calloc( 1, sizeof(*region) ))) return E_OUTOFMEMORY;
    region->IGeographicRegion_iface.lpVtbl = &geographic_region_vtbl;
    region->ref = 1;

    *out = (IInspectable *)&region->IGeographicRegion_iface;
    return S_OK;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    activation_factory_QueryInterface,
    activation_factory_AddRef,
    activation_factory_Release,
    /* IInspectable methods */
    activation_factory_GetIids,
    activation_factory_GetRuntimeClassName,
    activation_factory_GetTrustLevel,
    /* IActivationFactory methods */
    activation_factory_ActivateInstance,
};

DEFINE_IINSPECTABLE( geographic_region_factory, IGeographicRegionFactory, struct geographic_region_factory, IActivationFactory_iface )

static HRESULT WINAPI geographic_region_factory_CreateGeographicRegion( IGeographicRegionFactory *iface, HSTRING code,
                                                                        IGeographicRegion **value )
{
    FIXME( "iface %p, code %p, value %p stub!\n", iface, code, value );
    return E_NOTIMPL;
}

static const struct IGeographicRegionFactoryVtbl geographic_region_factory_vtbl =
{
    geographic_region_factory_QueryInterface,
    geographic_region_factory_AddRef,
    geographic_region_factory_Release,
    /* IInspectable methods */
    geographic_region_factory_GetIids,
    geographic_region_factory_GetRuntimeClassName,
    geographic_region_factory_GetTrustLevel,
    /* IGeographicRegionFactory methods */
    geographic_region_factory_CreateGeographicRegion,
};

static struct geographic_region_factory factory =
{
    {&activation_factory_vtbl},
    {&geographic_region_factory_vtbl},
    1,
};

IActivationFactory *geographic_region_factory = &factory.IActivationFactory_iface;
