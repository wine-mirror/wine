/* WinRT Windows.UI.Input.PointerVisualizationSettings Implementation
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

#include "private.h"
#include "initguid.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ui);

struct ptrvissettings
{
    IPointerVisualizationSettings IPointerVisualizationSettings_iface;
    LONG ref;
};

static inline struct ptrvissettings *impl_from_IPointerVisualizationSettings( IPointerVisualizationSettings *iface )
{
    return CONTAINING_RECORD( iface, struct ptrvissettings, IPointerVisualizationSettings_iface );
}

static HRESULT WINAPI ptrvissettings_QueryInterface( IPointerVisualizationSettings *iface, REFIID iid, void **out )
{
    struct ptrvissettings *impl = impl_from_IPointerVisualizationSettings( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    *out = NULL;

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IPointerVisualizationSettings ))
    {
        *out = &impl->IPointerVisualizationSettings_iface;
    }

    if (!*out)
    {
        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown *)*out );
    return S_OK;
}

static ULONG WINAPI ptrvissettings_AddRef( IPointerVisualizationSettings *iface )
{
    struct ptrvissettings *impl = impl_from_IPointerVisualizationSettings( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI ptrvissettings_Release( IPointerVisualizationSettings *iface )
{
    struct ptrvissettings *impl = impl_from_IPointerVisualizationSettings( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI ptrvissettings_GetIids( IPointerVisualizationSettings *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI ptrvissettings_GetRuntimeClassName( IPointerVisualizationSettings *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI ptrvissettings_GetTrustLevel( IPointerVisualizationSettings *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI ptrvissettings_set_IsContactFeedbackEnabled( IPointerVisualizationSettings *iface, boolean value)
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return S_OK;
}

static HRESULT WINAPI ptrvissettings_get_IsContactFeedbackEnabled( IPointerVisualizationSettings *iface, boolean *value)
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = FALSE;
    return S_OK;
}

static HRESULT WINAPI ptrvissettings_set_IsBarrelButtonFeedbackEnabled( IPointerVisualizationSettings *iface, boolean value)
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return S_OK;
}

static HRESULT WINAPI ptrvissettings_get_IsBarrelButtonFeedbackEnabled( IPointerVisualizationSettings *iface, boolean *value)
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = FALSE;
    return S_OK;
}

static const struct IPointerVisualizationSettingsVtbl ptrvissettings_vtbl =
{
    ptrvissettings_QueryInterface,
    ptrvissettings_AddRef,
    ptrvissettings_Release,

    /* IInspectable methods */
    ptrvissettings_GetIids,
    ptrvissettings_GetRuntimeClassName,
    ptrvissettings_GetTrustLevel,

    /* IPointerVisualizationSettings methods */
    ptrvissettings_set_IsContactFeedbackEnabled,
    ptrvissettings_get_IsContactFeedbackEnabled,
    ptrvissettings_set_IsBarrelButtonFeedbackEnabled,
    ptrvissettings_get_IsBarrelButtonFeedbackEnabled,
};

struct ptrvissettings_statics
{
    IActivationFactory IActivationFactory_iface;
    IPointerVisualizationSettingsStatics IPointerVisualizationSettingsStatics_iface;
    LONG ref;
};

static inline struct ptrvissettings_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct ptrvissettings_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct ptrvissettings_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }
    else if (IsEqualGUID( iid, &IID_IPointerVisualizationSettingsStatics ))
    {
        *out = &impl->IPointerVisualizationSettingsStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct ptrvissettings_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct ptrvissettings_statics *impl = impl_from_IActivationFactory( iface );
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
    struct ptrvissettings *impl;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    if (!(impl = calloc( 1, sizeof(*impl) )))
    {
        *instance = NULL;
        return E_OUTOFMEMORY;
    }

    impl->IPointerVisualizationSettings_iface.lpVtbl = &ptrvissettings_vtbl;
    impl->ref = 1;

    *instance = (IInspectable *)&impl->IPointerVisualizationSettings_iface;
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

DEFINE_IINSPECTABLE( ptrvissettingsstatics, IPointerVisualizationSettingsStatics, struct ptrvissettings_statics, IActivationFactory_iface );

static HRESULT WINAPI ptrvissettingsstatics_GetForCurrentView( IPointerVisualizationSettingsStatics *iface, IPointerVisualizationSettings **ptrvissettings )
{
    struct ptrvissettings_statics *impl = impl_from_IPointerVisualizationSettingsStatics( iface );

    TRACE( "(iface %p, ptrvissettings %p)\n", iface, ptrvissettings );

    factory_ActivateInstance( &impl->IActivationFactory_iface, (IInspectable **)ptrvissettings );
    return S_OK;
}

static const struct IPointerVisualizationSettingsStaticsVtbl ptrvissettingsstatics_vtbl =
{
    ptrvissettingsstatics_QueryInterface,
    ptrvissettingsstatics_AddRef,
    ptrvissettingsstatics_Release,

    /* IInspectable methods */
    ptrvissettingsstatics_GetIids,
    ptrvissettingsstatics_GetRuntimeClassName,
    ptrvissettingsstatics_GetTrustLevel,

    /* IPointerVisualizationSettingsStatics methods */
    ptrvissettingsstatics_GetForCurrentView,
};

static struct ptrvissettings_statics ptrvissettings_statics =
{
    {&factory_vtbl},
    {&ptrvissettingsstatics_vtbl},
    1,
};

IActivationFactory *ptrvissettings_factory = &ptrvissettings_statics.IActivationFactory_iface;
