/* WinRT Windows.ApplicationModel.Core.CoreApplication implementation
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

#include "private.h"
#include <assert.h>
#include <winuser.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(corewnd);

struct corewindowstatics_impl {
    IActivationFactory IActivationFactory_iface;
    ICoreWindowStatic ICoreWindowStatic_iface;
    LONG ref;
};

static inline struct corewindowstatics_impl *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct corewindowstatics_impl, IActivationFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct corewindowstatics_impl *factory = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IActivationFactory_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ICoreWindowStatic))
    {
        IUnknown_AddRef(iface);
        *out = &factory->ICoreWindowStatic_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IActivationFactory *iface)
{
    struct corewindowstatics_impl *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI factory_Release(IActivationFactory *iface)
{
    struct corewindowstatics_impl *factory = impl_from_IActivationFactory(iface);
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

static inline struct corewindowstatics_impl *impl_from_ICoreWindowStatic(ICoreWindowStatic *iface)
{
    return CONTAINING_RECORD(iface, struct corewindowstatics_impl, ICoreWindowStatic_iface);
}

static HRESULT WINAPI corewindowstatic_QueryInterface(
        ICoreWindowStatic *iface, REFIID iid, void **out)
{
    struct corewindowstatics_impl *factory = impl_from_ICoreWindowStatic(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_ICoreWindowStatic))
    {
        IUnknown_AddRef(iface);
        *out = &factory->ICoreWindowStatic_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI corewindowstatic_AddRef(ICoreWindowStatic *iface)
{
    struct corewindowstatics_impl *factory = impl_from_ICoreWindowStatic(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI corewindowstatic_Release(ICoreWindowStatic *iface)
{
    struct corewindowstatics_impl *factory = impl_from_ICoreWindowStatic(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI corewindowstatic_GetIids(
        ICoreWindowStatic *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindowstatic_GetRuntimeClassName(
        ICoreWindowStatic *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindowstatic_GetTrustLevel(
        ICoreWindowStatic *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindowstatic_GetForCurrentThread(
        ICoreWindowStatic *iface, ICoreWindow **windows)
{
    struct corewindow_tls *tls;
    tls = TlsGetValue(corewindow_tls); 
    if ((tls == NULL) && (GetLastError() != ERROR_SUCCESS)) 
    {
        ERR("TLS failed!\n");
        return E_FAIL;
    }

    *windows = &tls->window->ICoreWindow_iface;
    if (*windows == NULL) {
        ERR("NULL window!\n");
        return E_FAIL;
    }
    
    return S_OK;
}

static const struct ICoreWindowStaticVtbl corewindowstatic_vtbl =
{
    corewindowstatic_QueryInterface,
    corewindowstatic_AddRef,
    corewindowstatic_Release,
    /* IInspectable methods */
    corewindowstatic_GetIids,
    corewindowstatic_GetRuntimeClassName,
    corewindowstatic_GetTrustLevel,
    /* ICoreWindowStatic methods */
    corewindowstatic_GetForCurrentThread,
};

static struct corewindowstatics_impl corewindowstatics_impl_global =
{
    .IActivationFactory_iface.lpVtbl = &factory_vtbl,
    .ICoreWindowStatic_iface.lpVtbl = &corewindowstatic_vtbl,
    .ref = 1,
};

IActivationFactory *corewindow_factory = &corewindowstatics_impl_global.IActivationFactory_iface;

static inline struct corewindow_impl *impl_from_ICoreWindow(ICoreWindow *iface)
{
    return CONTAINING_RECORD(iface, struct corewindow_impl, ICoreWindow_iface);
}

static inline struct corewindow_impl *impl_from_ICoreWindowInterop(ICoreWindowInterop *iface)
{
    return CONTAINING_RECORD(iface, struct corewindow_impl, ICoreWindowInterop_iface);
}

static HRESULT WINAPI corewindow_interop_impl_QueryInterface( ICoreWindowInterop *iface, REFIID iid, void **out )
{
    struct corewindow_impl *impl = impl_from_ICoreWindowInterop( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_ICoreWindowInterop ))
    {
        *out = &impl->ICoreWindowInterop_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI corewindow_interop_impl_AddRef( ICoreWindowInterop *iface )
{
    struct corewindow_impl *impl = impl_from_ICoreWindowInterop( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI corewindow_interop_impl_Release( ICoreWindowInterop *iface )
{
    struct corewindow_impl *impl = impl_from_ICoreWindowInterop( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI corewindow_interop_impl_get_WindowHandle( ICoreWindowInterop *iface, HWND *value)
{
    struct corewindow_impl *impl = impl_from_ICoreWindowInterop( iface );
    *value = impl->window_handle;
    return S_OK;
}

static HRESULT WINAPI corewindow_interop_impl_set_MessageHandled( ICoreWindowInterop *iface, boolean value)
{
    FIXME("iface %p, value %d stub.\n", iface, value);
    return S_OK;
}

static const struct ICoreWindowInteropVtbl corewindow_interop_impl_vtbl =
{
    corewindow_interop_impl_QueryInterface,
    corewindow_interop_impl_AddRef,
    corewindow_interop_impl_Release,
    /* ICoreWindowInterop methods */
    corewindow_interop_impl_get_WindowHandle,
    corewindow_interop_impl_set_MessageHandled,
};

static HRESULT WINAPI corewindow_impl_QueryInterface( ICoreWindow *iface, REFIID iid, void **out )
{
    struct corewindow_impl *impl = impl_from_ICoreWindow( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_ICoreWindow ))
    {
        *out = &impl->ICoreWindow_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_ICoreWindowInterop ))
    {
        *out = &impl->ICoreWindowInterop_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_ICoreWindow4 ))
    {
        *out = &impl->ICoreWindow4_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI corewindow_impl_AddRef( ICoreWindow *iface )
{
    struct corewindow_impl *impl = impl_from_ICoreWindow( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI corewindow_impl_Release( ICoreWindow *iface )
{
    struct corewindow_impl *impl = impl_from_ICoreWindow( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI corewindow_impl_GetIids( ICoreWindow *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindow_impl_GetRuntimeClassName( ICoreWindow *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindow_impl_GetTrustLevel( ICoreWindow *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindow_impl_get_AutomationHostProvider( ICoreWindow *iface, IInspectable **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindow_impl_get_Bounds( ICoreWindow *iface, Rect *value )
{
    RECT rect;

    struct corewindow_impl *impl = impl_from_ICoreWindow( iface );

    if(GetWindowRect(impl->window_handle, &rect))
    {
        value->X = rect.left;
        value->Y = rect.top;
        value->Width = rect.right - rect.left;
        value->Height = rect.bottom - rect.top;
        return S_OK;
    }

    ERR( "GetWindowRect failed!\n");
    return E_FAIL;
}

static HRESULT WINAPI corewindow_impl_get_CustomProperties( ICoreWindow *iface, IPropertySet **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindow_impl_get_Dispatcher( ICoreWindow *iface, ICoreDispatcher **value )
{
    struct corewindow_impl *impl = impl_from_ICoreWindow( iface );
    *value = &impl->dispatcher->ICoreDispatcher_iface;
    return S_OK;
}

static HRESULT WINAPI corewindow_impl_get_FlowDirection( ICoreWindow *iface, CoreWindowFlowDirection *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = CoreWindowFlowDirection_LeftToRight;
    return S_OK;
}

static HRESULT WINAPI corewindow_impl_set_FlowDirection( ICoreWindow *iface, CoreWindowFlowDirection value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return S_OK;
}

static HRESULT WINAPI corewindow_impl_get_IsInputEnabled( ICoreWindow *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = TRUE;
    return S_OK;
}

static HRESULT WINAPI corewindow_impl_set_IsInputEnabled( ICoreWindow *iface, boolean value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return S_OK;
}

static HRESULT WINAPI corewindow_impl_get_PointerCursor( ICoreWindow *iface, ICoreCursor **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindow_impl_set_PointerCursor( ICoreWindow *iface, ICoreCursor* value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return S_OK;
}

static HRESULT WINAPI corewindow_impl_get_PointerPosition( ICoreWindow *iface, Point* value )
{
    POINT p;
    struct corewindow_impl *impl = impl_from_ICoreWindow( iface );

    FIXME( "iface %p, value %p stub!\n", iface, value );
    
    if (GetCursorPos(&p) && ScreenToClient(impl->window_handle, &p)) {
        value->X = p.x;
        value->Y = p.y;
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI corewindow_impl_get_Visible( ICoreWindow *iface, boolean* value )
{
    struct corewindow_impl *impl = impl_from_ICoreWindow( iface );

    FIXME( "iface %p, value %p stub!\n", iface, value );
    if (IsIconic(impl->window_handle)) {
        *value = FALSE;
    } else {
        *value = TRUE;
    }
    
    return S_OK;
}

static HRESULT WINAPI corewindow_impl_Activate( ICoreWindow *iface )
{
    struct corewindow_impl *impl = impl_from_ICoreWindow( iface );

    FIXME( "iface %p stub!\n", iface );
    if(ShowWindow(impl->window_handle, SW_SHOW)) {
        return S_OK;
    }

    ERR( "ShowWindow failed!\n");
    
    return E_FAIL;
}

static HRESULT WINAPI corewindow_impl_Close( ICoreWindow *iface )
{
    struct corewindow_impl *impl = impl_from_ICoreWindow( iface );

    if(CloseWindow(impl->window_handle))
    {
        return S_OK;
    }

    ERR( "CloseWindow failed!\n");
    return E_FAIL;
}

static HRESULT WINAPI corewindow_impl_GetAsyncKeyState( ICoreWindow *iface, VirtualKey key, CoreVirtualKeyStates *state)
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindow_impl_GetKeyState( ICoreWindow *iface, VirtualKey key, CoreVirtualKeyStates *state)
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindow_impl_ReleasePointerCapture( ICoreWindow *iface )
{
    FIXME( "iface %p stub!\n", iface );
    if(ReleaseCapture()) {
        return S_OK;
    }

    ERR( "ReleaseCapture failed!\n");
    return E_FAIL;
}

static HRESULT WINAPI corewindow_impl_SetPointerCapture( ICoreWindow *iface )
{
    struct corewindow_impl *impl = impl_from_ICoreWindow( iface );

    FIXME( "iface %p stub!\n", iface );
    if(SetCapture(impl->window_handle)) {
        return S_OK;
    }

    ERR( "SetCapture failed!\n");
    return E_FAIL;
}

DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, Activated, WindowActivatedEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, AutomationProviderRequested, AutomationProviderRequestedEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, CharacterReceived, CharacterReceivedEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, Closed, CoreWindowEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, InputEnabled, InputEnabledEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, KeyDown, KeyEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, KeyUp, KeyEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, PointerCaptureLost, PointerEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, PointerEntered, PointerEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, PointerExited, PointerEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, PointerMoved, PointerEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, PointerPressed, PointerEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, PointerReleased, PointerEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, TouchHitTesting, TouchHitTestingEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, PointerWheelChanged, PointerEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, SizeChanged, WindowSizeChangedEventArgs)
DEFINE_EVENT_STUB(CoreWindow, corewindow_impl, VisibilityChanged, VisibilityChangedEventArgs)

static const struct ICoreWindowVtbl corewindow_impl_vtbl =
{
    corewindow_impl_QueryInterface,
    corewindow_impl_AddRef,
    corewindow_impl_Release,
    /* IInspectable methods */
    corewindow_impl_GetIids,
    corewindow_impl_GetRuntimeClassName,
    corewindow_impl_GetTrustLevel,
    /* ICoreWindow methods */
    corewindow_impl_get_AutomationHostProvider,
    corewindow_impl_get_Bounds,
    corewindow_impl_get_CustomProperties,
    corewindow_impl_get_Dispatcher,
    corewindow_impl_get_FlowDirection,
    corewindow_impl_set_FlowDirection,
    corewindow_impl_get_IsInputEnabled,
    corewindow_impl_set_IsInputEnabled,
    corewindow_impl_get_PointerCursor,
    corewindow_impl_set_PointerCursor,
    corewindow_impl_get_PointerPosition,
    corewindow_impl_get_Visible,
    corewindow_impl_Activate,
    corewindow_impl_Close,
    corewindow_impl_GetAsyncKeyState,
    corewindow_impl_GetKeyState,
    corewindow_impl_ReleasePointerCapture,
    corewindow_impl_SetPointerCapture,
    DECLARE_EVENT(corewindow_impl, Activated),
    DECLARE_EVENT(corewindow_impl, AutomationProviderRequested),
    DECLARE_EVENT(corewindow_impl, CharacterReceived),
    DECLARE_EVENT(corewindow_impl, Closed),
    DECLARE_EVENT(corewindow_impl, InputEnabled),
    DECLARE_EVENT(corewindow_impl, KeyDown),
    DECLARE_EVENT(corewindow_impl, KeyUp),
    DECLARE_EVENT(corewindow_impl, PointerCaptureLost),
    DECLARE_EVENT(corewindow_impl, PointerEntered),
    DECLARE_EVENT(corewindow_impl, PointerExited),
    DECLARE_EVENT(corewindow_impl, PointerMoved),
    DECLARE_EVENT(corewindow_impl, PointerPressed),
    DECLARE_EVENT(corewindow_impl, PointerReleased),
    DECLARE_EVENT(corewindow_impl, TouchHitTesting),
    DECLARE_EVENT(corewindow_impl, PointerWheelChanged),
    DECLARE_EVENT(corewindow_impl, SizeChanged),
    DECLARE_EVENT(corewindow_impl, VisibilityChanged)
};

static inline struct corewindow_impl *impl_from_ICoreWindow4( ICoreWindow4 *iface )
{
    return CONTAINING_RECORD( iface, struct corewindow_impl, ICoreWindow4_iface );
}

static HRESULT WINAPI corewindow4_impl_QueryInterface( ICoreWindow4 *iface, REFIID iid, void **out )
{
    struct corewindow_impl *impl = impl_from_ICoreWindow4( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_ICoreWindow4 ))
    {
        *out = &impl->ICoreWindow4_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI corewindow4_impl_AddRef( ICoreWindow4 *iface )
{
    struct corewindow_impl *impl = impl_from_ICoreWindow4( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI corewindow4_impl_Release( ICoreWindow4 *iface )
{
    struct corewindow_impl *impl = impl_from_ICoreWindow4( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI corewindow4_impl_GetIids( ICoreWindow4 *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindow4_impl_GetRuntimeClassName( ICoreWindow4 *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindow4_impl_GetTrustLevel( ICoreWindow4 *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI corewindow4_impl_add_ResizeStarted( ICoreWindow4 *iface, ITypedEventHandler_CoreWindow_IInspectable *handler, EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    return S_OK;
}

static HRESULT WINAPI corewindow4_impl_remove_ResizeStarted( ICoreWindow4 *iface, EventRegistrationToken token )
{
    FIXME("iface %p, token %#I64x stub.\n", iface, token.value);
    return S_OK;
}

static HRESULT WINAPI corewindow4_impl_add_ResizeCompleted( ICoreWindow4 *iface, ITypedEventHandler_CoreWindow_IInspectable *handler, EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    return S_OK;
}

static HRESULT WINAPI corewindow4_impl_remove_ResizeCompleted( ICoreWindow4 *iface, EventRegistrationToken token )
{
    FIXME("iface %p, token %#I64x stub.\n", iface, token.value);
    return S_OK;
}

static const struct ICoreWindow4Vtbl corewindow4_impl_vtbl =
{
    corewindow4_impl_QueryInterface,
    corewindow4_impl_AddRef,
    corewindow4_impl_Release,
    /* IInspectable methods */
    corewindow4_impl_GetIids,
    corewindow4_impl_GetRuntimeClassName,
    corewindow4_impl_GetTrustLevel,
    /* ICoreWindow4 methods */
    corewindow4_impl_add_ResizeStarted,
    corewindow4_impl_remove_ResizeStarted,
    corewindow4_impl_add_ResizeCompleted,
    corewindow4_impl_remove_ResizeCompleted
};

struct corewindow_impl *create_corewindow(IFrameworkView *for_view, char *identity_name, char *display_name) {
    struct corewindow_impl *object;
    struct corewindow_tls *tls;

    TRACE("for_view %p.\n", for_view);

    if (!(object = calloc(1, sizeof(*object))))
        return NULL;

    object->ICoreWindow_iface.lpVtbl = &corewindow_impl_vtbl;
    object->ICoreWindow4_iface.lpVtbl = &corewindow4_impl_vtbl;
    object->ICoreWindowInterop_iface.lpVtbl = &corewindow_interop_impl_vtbl;
    object->current_view = for_view;
    object->current_view->lpVtbl->AddRef(object->current_view);
    object->ref = 1;
    object->dispatcher = create_dispatcher(object, identity_name, display_name);

    tls = LocalAlloc(LPTR, sizeof(*tls)); 
    tls->window = object;
    if (!TlsSetValue(corewindow_tls, (LPVOID)tls)) 
    {
        ERR("TlsSetValue failed!\n");
    }

    return object;
}
