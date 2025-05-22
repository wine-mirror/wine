/* WinRT Windows.UI Implementation
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

#include "private.h"
#include "initguid.h"
#include "weakreference.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ui);

static EventRegistrationToken dummy_cookie = {.value = 0xdeadbeef};

struct uisettings
{
    IUISettings IUISettings_iface;
    IUISettings2 IUISettings2_iface;
    IUISettings3 IUISettings3_iface;
    IUISettings4 IUISettings4_iface;
    IUISettings5 IUISettings5_iface;
    IWeakReferenceSource IWeakReferenceSource_iface;
    IWeakReference IWeakReference_iface;
    LONG ref_strong;
    LONG ref_weak;
};

static inline struct uisettings *impl_from_IUISettings( IUISettings *iface )
{
    return CONTAINING_RECORD( iface, struct uisettings, IUISettings_iface );
}

static HRESULT WINAPI uisettings_QueryInterface( IUISettings *iface, REFIID iid, void **out )
{
    struct uisettings *impl = impl_from_IUISettings( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    *out = NULL;

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IUISettings ))
    {
        *out = &impl->IUISettings_iface;
    }
    else if (IsEqualGUID( iid, &IID_IUISettings2 ))
    {
        *out = &impl->IUISettings2_iface;
    }
    else if (IsEqualGUID( iid, &IID_IUISettings3 ))
    {
        *out = &impl->IUISettings3_iface;
    }
    else if (IsEqualGUID( iid, &IID_IUISettings4 ))
    {
        *out = &impl->IUISettings4_iface;
    }
    else if (IsEqualGUID( iid, &IID_IUISettings5 ))
    {
        *out = &impl->IUISettings5_iface;
    }
    else if (IsEqualGUID( iid, &IID_IWeakReferenceSource ))
    {
        *out = &impl->IWeakReferenceSource_iface;
    }

    if (!*out)
    {
        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown*)*out );
    return S_OK;
}

static ULONG WINAPI uisettings_AddRef( IUISettings *iface )
{
    struct uisettings *impl = impl_from_IUISettings( iface );
    ULONG ref = InterlockedIncrement( &impl->ref_strong );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    IWeakReference_AddRef( &impl->IWeakReference_iface );
    return ref;
}

static ULONG WINAPI uisettings_Release( IUISettings *iface )
{
    struct uisettings *impl = impl_from_IUISettings( iface );
    ULONG ref = InterlockedDecrement( &impl->ref_strong );

    TRACE( "iface %p, ref %lu.\n", iface, ref );

    IWeakReference_Release( &impl->IWeakReference_iface );
    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI uisettings_GetIids( IUISettings *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_GetRuntimeClassName( IUISettings *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_GetTrustLevel( IUISettings *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_get_HandPreference( IUISettings *iface, enum HandPreference *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_get_CursorSize( IUISettings *iface, struct Size *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_get_ScrollBarSize( IUISettings *iface, struct Size *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_get_ScrollBarArrowSize( IUISettings *iface, struct Size *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_get_ScrollBarThumbBoxSize( IUISettings *iface, struct Size *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_get_MessageDuration( IUISettings *iface, UINT32 *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_get_AnimationsEnabled( IUISettings *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_get_CaretBrowsingEnabled( IUISettings *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_get_CaretBlinkRate( IUISettings *iface, UINT32 *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_get_CaretWidth( IUISettings *iface, UINT32 *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_get_DoubleClickTime( IUISettings *iface, UINT32 *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_get_MouseHoverTime( IUISettings *iface, UINT32 *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings_UIElementColor( IUISettings *iface, enum UIElementType element, struct Color *value )
{
    FIXME( "iface %p, element %d value %p stub!\n", iface, element, value );
    return E_NOTIMPL;
}

static const struct IUISettingsVtbl uisettings_vtbl =
{
    uisettings_QueryInterface,
    uisettings_AddRef,
    uisettings_Release,

    /* IInspectable methods */
    uisettings_GetIids,
    uisettings_GetRuntimeClassName,
    uisettings_GetTrustLevel,

    /* IUISettings methods */
    uisettings_get_HandPreference,
    uisettings_get_CursorSize,
    uisettings_get_ScrollBarSize,
    uisettings_get_ScrollBarArrowSize,
    uisettings_get_ScrollBarThumbBoxSize,
    uisettings_get_MessageDuration,
    uisettings_get_AnimationsEnabled,
    uisettings_get_CaretBrowsingEnabled,
    uisettings_get_CaretBlinkRate,
    uisettings_get_CaretWidth,
    uisettings_get_DoubleClickTime,
    uisettings_get_MouseHoverTime,
    uisettings_UIElementColor
};

DEFINE_IINSPECTABLE( uisettings2, IUISettings2, struct uisettings, IUISettings_iface );

static HRESULT WINAPI uisettings2_get_TextScaleFactor( IUISettings2 *iface, DOUBLE *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings2_add_TextScaleFactorChanged( IUISettings2 *iface, ITypedEventHandler_UISettings_IInspectable *handler,
        EventRegistrationToken *cookie )
{
    FIXME( "iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie );
    return E_NOTIMPL;
}

static HRESULT WINAPI uisettings2_remove_TextScaleFactorChanged( IUISettings2 *iface, EventRegistrationToken cookie )
{
    FIXME( "iface %p, cookie %#I64x stub!\n", iface, cookie.value );
    return E_NOTIMPL;
}

static const struct IUISettings2Vtbl uisettings2_vtbl =
{
    uisettings2_QueryInterface,
    uisettings2_AddRef,
    uisettings2_Release,

    /* IInspectable methods */
    uisettings2_GetIids,
    uisettings2_GetRuntimeClassName,
    uisettings2_GetTrustLevel,

    /* IUISettings2 methods */
    uisettings2_get_TextScaleFactor,
    uisettings2_add_TextScaleFactorChanged,
    uisettings2_remove_TextScaleFactorChanged
};

DEFINE_IINSPECTABLE( uisettings3, IUISettings3, struct uisettings, IUISettings_iface );

static DWORD get_app_theme(void)
{
    static const WCHAR *subkey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    static const WCHAR *name = L"AppsUseLightTheme";
    static const HKEY root = HKEY_CURRENT_USER;
    DWORD ret = 0, len = sizeof(ret), type;
    HKEY hkey;

    if (RegOpenKeyExW( root, subkey, 0, KEY_QUERY_VALUE, &hkey )) return 1;
    if (RegQueryValueExW( hkey, name, NULL, &type, (BYTE *)&ret, &len ) || type != REG_DWORD) ret = 1;
    RegCloseKey( hkey );
    return ret;
}

static DWORD initialize_accent_palette( HKEY hkey, DWORD offset )
{
    DWORD palette[7] = { 0x00ffd8a6, 0x00edb976, 0x00e39c42, 0x00d77800, 0x009e5a00, 0x00754200, 0x00422600 };
    DWORD len = sizeof(palette);

    RegSetValueExW( hkey, L"AccentPalette", 0, REG_BINARY, (const BYTE *)&palette, len );
    return palette[offset];
}

static void dword_to_color( DWORD color, Color *out )
{
    out->R = color & 0xFF;
    out->G = (color >> 8) & 0xFF;
    out->B = (color >> 16) & 0xFF;
    out->A = 0xFF;
}

static void get_accent_palette_color( DWORD offset, Color *out )
{
    DWORD palette[7];
    DWORD ret = 0, len = sizeof(palette), type;
    HKEY hkey;

    if (!RegCreateKeyW( HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent", &hkey ))
    {
        if (RegQueryValueExW( hkey, L"AccentPalette", NULL, &type, (BYTE *)&palette, &len ) || type != REG_BINARY)
            ret = initialize_accent_palette( hkey, offset );
        else
            ret = palette[offset];
        RegCloseKey( hkey );
    }
    dword_to_color( ret, out );
}

static void set_color_value( BYTE a, BYTE r, BYTE g, BYTE b, Color *out )
{
    out->A = a;
    out->R = r;
    out->G = g;
    out->B = b;
}

static HRESULT WINAPI uisettings3_GetColorValue( IUISettings3 *iface, UIColorType type, Color *value )
{
    DWORD theme;

    TRACE( "iface %p, type %d, value %p.\n", iface, type, value );

    switch (type)
    {
    case UIColorType_Foreground:
        theme = get_app_theme();
        set_color_value( 255, theme ? 0 : 255, theme ? 0 : 255, theme ? 0 : 255, value );
        break;
    case UIColorType_Background:
        theme = get_app_theme();
        set_color_value( 255, theme ? 255 : 0, theme ? 255 : 0, theme ? 255 : 0, value );
        break;
    case UIColorType_Accent:
        get_accent_palette_color( 3, value );
        break;
    case UIColorType_AccentDark1:
        get_accent_palette_color( 4, value );
        break;
    case UIColorType_AccentDark2:
        get_accent_palette_color( 5, value );
        break;
    case UIColorType_AccentDark3:
        get_accent_palette_color( 6, value );
        break;
    case UIColorType_AccentLight1:
        get_accent_palette_color( 2, value );
        break;
    case UIColorType_AccentLight2:
        get_accent_palette_color( 1, value );
        break;
    case UIColorType_AccentLight3:
        get_accent_palette_color( 0, value );
        break;
    default:
        FIXME( "type %d not implemented.\n", type );
        return E_NOTIMPL;
    }

    TRACE( "Returning value.A = %d, value.R = %d, value.G = %d, value.B = %d\n", value->A, value->R, value->G, value->B );
    return S_OK;
}

static HRESULT WINAPI uisettings3_add_ColorValuesChanged( IUISettings3 *iface, ITypedEventHandler_UISettings_IInspectable *handler, EventRegistrationToken *cookie )
{
    FIXME( "iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie );
    *cookie = dummy_cookie;
    return S_OK;
}

static HRESULT WINAPI uisettings3_remove_ColorValuesChanged( IUISettings3 *iface, EventRegistrationToken cookie )
{
    FIXME( "iface %p, cookie %#I64x stub!\n", iface, cookie.value );
    return S_OK;
}

static const struct IUISettings3Vtbl uisettings3_vtbl =
{
    uisettings3_QueryInterface,
    uisettings3_AddRef,
    uisettings3_Release,
    /* IInspectable methods */
    uisettings3_GetIids,
    uisettings3_GetRuntimeClassName,
    uisettings3_GetTrustLevel,
    /* IUISettings3 methods */
    uisettings3_GetColorValue,
    uisettings3_add_ColorValuesChanged,
    uisettings3_remove_ColorValuesChanged,
};

DEFINE_IINSPECTABLE( uisettings4, IUISettings4, struct uisettings, IUISettings_iface );

static HRESULT WINAPI uisettings4_get_AdvancedEffectsEnabled( IUISettings4 *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!.\n", iface, value );
    *value = TRUE;
    return S_OK;
}

static HRESULT WINAPI uisettings4_add_AdvancedEffectsEnabledChanged( IUISettings4 *iface, ITypedEventHandler_UISettings_IInspectable *handler, EventRegistrationToken *cookie )
{
    FIXME( "iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie );
    *cookie = dummy_cookie;
    return S_OK;
}

static HRESULT WINAPI uisettings4_remove_AdvancedEffectsEnabledChanged( IUISettings4 *iface, EventRegistrationToken cookie )
{
    FIXME( "iface %p, cookie %#I64x stub!\n", iface, cookie.value );
    return S_OK;
}

static const struct IUISettings4Vtbl uisettings4_vtbl =
{
    uisettings4_QueryInterface,
    uisettings4_AddRef,
    uisettings4_Release,
    /* IInspectable methods */
    uisettings4_GetIids,
    uisettings4_GetRuntimeClassName,
    uisettings4_GetTrustLevel,
    /* IUISettings4 methods */
    uisettings4_get_AdvancedEffectsEnabled,
    uisettings4_add_AdvancedEffectsEnabledChanged,
    uisettings4_remove_AdvancedEffectsEnabledChanged,
};

DEFINE_IINSPECTABLE( uisettings5, IUISettings5, struct uisettings, IUISettings_iface );

static HRESULT WINAPI uisettings5_get_AutoHideScrollBars( IUISettings5 *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!.\n", iface, value );
    *value = FALSE;
    return S_OK;
}

static HRESULT WINAPI uisettings5_add_AutoHideScrollBarsChanged( IUISettings5 *iface, ITypedEventHandler_UISettings_UISettingsAutoHideScrollBarsChangedEventArgs *handler,
                                                                 EventRegistrationToken *cookie )
{
    FIXME( "iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie );
    *cookie = dummy_cookie;
    return S_OK;
}

static HRESULT WINAPI uisettings5_remove_AutoHideScrollBarsChanged( IUISettings5 *iface, EventRegistrationToken cookie )
{
    FIXME( "iface %p, cookie %#I64x stub!\n", iface, cookie.value );
    return S_OK;
}

static const struct IUISettings5Vtbl uisettings5_vtbl =
{
    uisettings5_QueryInterface,
    uisettings5_AddRef,
    uisettings5_Release,
    /* IInspectable methods */
    uisettings5_GetIids,
    uisettings5_GetRuntimeClassName,
    uisettings5_GetTrustLevel,
    /* IUISettings5 methods */
    uisettings5_get_AutoHideScrollBars,
    uisettings5_add_AutoHideScrollBarsChanged,
    uisettings5_remove_AutoHideScrollBarsChanged,
};

static inline struct uisettings *impl_from_IWeakReference( IWeakReference *iface )
{
    return CONTAINING_RECORD( iface, struct uisettings, IWeakReference_iface );
}

static HRESULT WINAPI weak_reference_QueryInterface( IWeakReference *iface, REFIID iid, void **out )
{
    struct uisettings *impl = impl_from_IWeakReference( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IWeakReference ))
    {
        *out = &impl->IWeakReference_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI weak_reference_AddRef( IWeakReference *iface )
{
    struct uisettings *impl = impl_from_IWeakReference( iface );
    ULONG ref = InterlockedIncrement( &impl->ref_weak );
    TRACE("iface %p increasing refcount to %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI weak_reference_Release( IWeakReference *iface )
{
    struct uisettings *impl = impl_from_IWeakReference( iface );
    ULONG ref = InterlockedDecrement( &impl->ref_weak );
    if (!ref)
        free( impl );
    return ref;
}

static HRESULT WINAPI weak_reference_Resolve( IWeakReference *iface, REFIID iid, IInspectable **out )
{
    struct uisettings *impl = impl_from_IWeakReference( iface );
    HRESULT hr;
    LONG ref;

    TRACE( "iface %p, iid %s, out %p stub.\n", iface, debugstr_guid(iid), out );

    *out = NULL;

    do
    {
        if (!(ref = ReadNoFence( &impl->ref_strong )))
            return S_OK;
    } while (ref != InterlockedCompareExchange( &impl->ref_strong, ref + 1, ref ));

    hr = IUISettings_QueryInterface( &impl->IUISettings_iface, iid, (void **)out );
    InterlockedDecrement( &impl->ref_strong );
    return hr;
}

static const struct IWeakReferenceVtbl weak_reference_vtbl =
{
    weak_reference_QueryInterface,
    weak_reference_AddRef,
    weak_reference_Release,
    /* IWeakReference methods */
    weak_reference_Resolve,
};

static inline struct uisettings *impl_from_IWeakReferenceSource( IWeakReferenceSource *iface )
{
    return CONTAINING_RECORD( iface, struct uisettings, IWeakReferenceSource_iface );
}

static HRESULT WINAPI weak_reference_source_QueryInterface( IWeakReferenceSource *iface, REFIID iid, void **out )
{
    struct uisettings *impl = impl_from_IWeakReferenceSource( iface );
    return uisettings_QueryInterface( &impl->IUISettings_iface, iid, out );
}

static ULONG WINAPI weak_reference_source_AddRef( IWeakReferenceSource *iface )
{
    struct uisettings *impl = impl_from_IWeakReferenceSource( iface );
    return uisettings_AddRef( &impl->IUISettings_iface );
}

static ULONG WINAPI weak_reference_source_Release( IWeakReferenceSource *iface )
{
    struct uisettings *impl = impl_from_IWeakReferenceSource( iface );
    return uisettings_Release( &impl->IUISettings_iface );
}

static HRESULT WINAPI weak_reference_source_GetWeakReference( IWeakReferenceSource *iface, IWeakReference **ref )
{
    struct uisettings *impl = impl_from_IWeakReferenceSource(iface);

    TRACE("iface %p, ref %p stub.\n", iface, ref);

    *ref = &impl->IWeakReference_iface;
    IWeakReference_AddRef( *ref );
    return S_OK;
}

static const struct IWeakReferenceSourceVtbl weak_reference_source_vtbl =
{
    weak_reference_source_QueryInterface,
    weak_reference_source_AddRef,
    weak_reference_source_Release,
    /* IWeakReferenceSource methods */
    weak_reference_source_GetWeakReference,
};

struct uisettings_statics
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct uisettings_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct uisettings_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct uisettings_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct uisettings_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct uisettings_statics *impl = impl_from_IActivationFactory( iface );
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
    struct uisettings *impl;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    if (!(impl = calloc( 1, sizeof(*impl) )))
    {
        *instance = NULL;
        return E_OUTOFMEMORY;
    }

    impl->IUISettings_iface.lpVtbl = &uisettings_vtbl;
    impl->IUISettings2_iface.lpVtbl = &uisettings2_vtbl;
    impl->IUISettings3_iface.lpVtbl = &uisettings3_vtbl;
    impl->IUISettings4_iface.lpVtbl = &uisettings4_vtbl;
    impl->IUISettings5_iface.lpVtbl = &uisettings5_vtbl;
    impl->IWeakReferenceSource_iface.lpVtbl = &weak_reference_source_vtbl;
    impl->IWeakReference_iface.lpVtbl = &weak_reference_vtbl;
    impl->ref_strong = 1;
    impl->ref_weak = 1;

    *instance = (IInspectable *)&impl->IUISettings5_iface;
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

static struct uisettings_statics uisettings_statics =
{
    {&factory_vtbl},
    1,
};

IActivationFactory *uisettings_factory = &uisettings_statics.IActivationFactory_iface;
