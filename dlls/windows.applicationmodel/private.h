/* WinRT Windows.ApplicationModel Implementation
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

#ifndef __WINE_WINDOWS_APPLICATIONMODEL_PRIVATE_H
#define __WINE_WINDOWS_APPLICATIONMODEL_PRIVATE_H

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winstring.h"

#include "activation.h"

#define WIDL_using_Windows_System
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_UI_Input
#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.ui.core.h"
#include "windows.system.h"
#include "windows.foundation.h"
#define WIDL_using_Windows_ApplicationModel
#define WIDL_using_Windows_ApplicationModel_Core
#define WIDL_using_Windows_Storage
#include "windows.applicationmodel.core.h"
#include "windows.applicationmodel.h"

#include "wine/list.h"

struct corewindow_impl {
    ICoreWindow ICoreWindow_iface;
    ICoreWindow4 ICoreWindow4_iface;
    ICoreWindowInterop ICoreWindowInterop_iface;
    HWND window_handle;
    IFrameworkView *current_view;
    struct dispatcher_impl *dispatcher;
    LONG ref;
};

struct dispatcher_impl {
    ICoreDispatcher ICoreDispatcher_iface;
    struct corewindow_impl *for_window;
    struct list queued_tasks;
    LONG ref;
};

struct corewindow_tls {
    struct corewindow_impl *window;
};

extern struct corewindow_impl *create_corewindow(IFrameworkView *for_view, char *identity_name, char *display_name);
extern struct dispatcher_impl *create_dispatcher(struct corewindow_impl *for_window, char *identity_name, char *display_name);
extern ICoreCursor *create_cursor(UINT32 id, CoreCursorType type);    

typedef HRESULT (*dispatcher_func)( IInspectable *invoker );

extern void *dispatcher_run_and_wait(dispatcher_func func);
extern void dispatcher_add_queue(dispatcher_func func);

extern DWORD corewindow_tls;
extern IActivationFactory *package_factory;
extern IActivationFactory *coreapplication_factory;
extern IActivationFactory *corewindow_factory;
extern IActivationFactory *sysnav_factory;
extern IActivationFactory *cursor_factory;
extern IActivationFactory *ptrvissettings_factory;

#define DEFINE_IINSPECTABLE_( pfx, iface_type, impl_type, impl_from, iface_mem, expr )             \
    static inline impl_type *impl_from( iface_type *iface )                                        \
    {                                                                                              \
        return CONTAINING_RECORD( iface, impl_type, iface_mem );                                   \
    }                                                                                              \
    static HRESULT WINAPI pfx##_QueryInterface( iface_type *iface, REFIID iid, void **out )        \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_QueryInterface( (IInspectable *)(expr), iid, out );                    \
    }                                                                                              \
    static ULONG WINAPI pfx##_AddRef( iface_type *iface )                                          \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_AddRef( (IInspectable *)(expr) );                                      \
    }                                                                                              \
    static ULONG WINAPI pfx##_Release( iface_type *iface )                                         \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_Release( (IInspectable *)(expr) );                                     \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetIids( iface_type *iface, ULONG *iid_count, IID **iids )         \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_GetIids( (IInspectable *)(expr), iid_count, iids );                    \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetRuntimeClassName( iface_type *iface, HSTRING *class_name )      \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_GetRuntimeClassName( (IInspectable *)(expr), class_name );             \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetTrustLevel( iface_type *iface, TrustLevel *trust_level )        \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_GetTrustLevel( (IInspectable *)(expr), trust_level );                  \
    }
#define DEFINE_IINSPECTABLE( pfx, iface_type, impl_type, base_iface )                              \
    DEFINE_IINSPECTABLE_( pfx, iface_type, impl_type, impl_from_##iface_type, iface_type##_iface, &impl->base_iface )

#define DECLARE_EVENT(pfx, evt)                              \
    pfx##_add_##evt,                                         \
    pfx##_remove_##evt

#define DEFINE_EVENT_STUB(type, pfx, evt, event_args)                                                                                       \
static HRESULT WINAPI pfx##_add_##evt( I##type *iface, ITypedEventHandler_##type##_##event_args *handler, EventRegistrationToken *token )                                  \
{                                                                                                                                           \
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );                                                                                \
    return S_OK;                                                                                                                            \
}                                                                                                                                           \
                                                                                                                                            \
static HRESULT WINAPI pfx##_remove_##evt( I##type *iface, EventRegistrationToken cookie)                                                    \
{                                                                                                                                           \
    FIXME( "iface %p token %#I64x stub!\n", iface, cookie.value );                                                                          \
    return E_NOTIMPL;                                                                                                                       \
}

#define IPropertySet __x_ABI_CWindows_CFoundation_CCollections_CIPropertySet

#endif
