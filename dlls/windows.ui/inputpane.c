/* WinRT Windows.UI Implementation
 *
 * Copyright (C) 2024 Fabian Maurer
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
#include "inputpaneinterop.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ui);

struct inputpane
{
    IInputPane IInputPane_iface;
    IInputPane2 IInputPane2_iface;
    LONG ref;
};

static inline struct inputpane *impl_from_IInputPane( IInputPane *iface )
{
    return CONTAINING_RECORD( iface, struct inputpane, IInputPane_iface );
}

static HRESULT WINAPI inputpane_QueryInterface( IInputPane *iface, REFIID iid, void **out )
{
    struct inputpane *impl = impl_from_IInputPane( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    *out = NULL;

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IInputPane ))
    {
        *out = &impl->IInputPane_iface;
    }
    else if (IsEqualGUID( iid, &IID_IInputPane2))
    {
        *out = &impl->IInputPane2_iface;
    }

    if (!*out)
    {
        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown *)*out );
    return S_OK;
}

static ULONG WINAPI inputpane_AddRef( IInputPane *iface )
{
    struct inputpane *impl = impl_from_IInputPane( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI inputpane_Release( IInputPane *iface )
{
    struct inputpane *impl = impl_from_IInputPane( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI inputpane_GetIids( IInputPane *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI inputpane_GetRuntimeClassName( IInputPane *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI inputpane_GetTrustLevel( IInputPane *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI inputpane_add_Showing( IInputPane *iface, ITypedEventHandler_InputPane_InputPaneVisibilityEventArgs *handler, EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI inputpane_remove_Showing( IInputPane *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static HRESULT WINAPI inputpane_add_Hiding( IInputPane *iface, ITypedEventHandler_InputPane_InputPaneVisibilityEventArgs *handler, EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI inputpane_remove_Hiding( IInputPane *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static HRESULT WINAPI inputpane_OccludedRect( IInputPane *iface, Rect *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static const struct IInputPaneVtbl inputpane_vtbl =
{
    inputpane_QueryInterface,
    inputpane_AddRef,
    inputpane_Release,

    /* IInspectable methods */
    inputpane_GetIids,
    inputpane_GetRuntimeClassName,
    inputpane_GetTrustLevel,

    /* IInputPane methods */
    inputpane_add_Showing,
    inputpane_remove_Showing,
    inputpane_add_Hiding,
    inputpane_remove_Hiding,
    inputpane_OccludedRect,
};

DEFINE_IINSPECTABLE( inputpane2, IInputPane2, struct inputpane, IInputPane_iface );

static HRESULT WINAPI inputpane2_TryShow( IInputPane2 *iface, boolean *result )
{
    FIXME( "iface %p, result %p stub!\n", iface, result );
    return E_NOTIMPL;
}

static HRESULT WINAPI inputpane2_TryHide( IInputPane2 *iface, boolean *result )
{
    FIXME( "iface %p, result %p stub!\n", iface, result );
    return S_OK;
}

static const struct IInputPane2Vtbl inputpane2_vtbl =
{
    inputpane2_QueryInterface,
    inputpane2_AddRef,
    inputpane2_Release,

    /* IInspectable methods */
    inputpane2_GetIids,
    inputpane2_GetRuntimeClassName,
    inputpane2_GetTrustLevel,

    /* IInputPane2 methods */
    inputpane2_TryShow,
    inputpane2_TryHide,
};

struct inputpane_statics
{
    IActivationFactory IActivationFactory_iface;
    IInputPaneInterop IInputPaneInterop_iface;
    LONG ref;
};

static inline struct inputpane_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct inputpane_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct inputpane_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }
    else if (IsEqualGUID( iid, &IID_IInputPaneInterop ))
    {
        *out = &impl->IInputPaneInterop_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct inputpane_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct inputpane_statics *impl = impl_from_IActivationFactory( iface );
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
    struct inputpane *impl;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    if (!(impl = calloc( 1, sizeof(*impl) )))
    {
        *instance = NULL;
        return E_OUTOFMEMORY;
    }

    impl->IInputPane_iface.lpVtbl = &inputpane_vtbl;
    impl->IInputPane2_iface.lpVtbl = &inputpane2_vtbl;
    impl->ref = 1;

    *instance = (IInspectable *)&impl->IInputPane_iface;
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

DEFINE_IINSPECTABLE( inputpane_interop, IInputPaneInterop, struct inputpane_statics, IActivationFactory_iface );

static HRESULT WINAPI inputpane_interop_GetForWindow( IInputPaneInterop *iface, HWND window, REFIID riid, void **inputpane )
{
    struct inputpane_statics *impl = impl_from_IInputPaneInterop( iface );

    TRACE( "(window %p, riid %s, inputpane %p)\n", window, debugstr_guid( riid ), inputpane );

    factory_ActivateInstance( &impl->IActivationFactory_iface, (IInspectable **)inputpane );
    return S_OK;
}

static const struct IInputPaneInteropVtbl inputpane_interop_vtbl =
{
    inputpane_interop_QueryInterface,
    inputpane_interop_AddRef,
    inputpane_interop_Release,

    /* IInspectable methods */
    inputpane_interop_GetIids,
    inputpane_interop_GetRuntimeClassName,
    inputpane_interop_GetTrustLevel,

    /* IInputPaneInteropt methods */
    inputpane_interop_GetForWindow,
};

static struct inputpane_statics inputpane_statics =
{
    {&factory_vtbl},
    {&inputpane_interop_vtbl},
    1,
};

IActivationFactory *inputpane_factory = &inputpane_statics.IActivationFactory_iface;
