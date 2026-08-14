/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XDisplay and XLauncher
 *
 * Copyright 2026 Olivia Ryan
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

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

struct display
{
    IXDisplayImpl IXDisplayImpl_iface;
    IXLauncherImpl IXLauncherImpl_iface;
    LONG ref;
};

static inline struct display *impl_from_IXDisplayImpl( IXDisplayImpl *iface )
{
    return CONTAINING_RECORD( iface, struct display, IXDisplayImpl_iface );
}

static HRESULT WINAPI x_display_QueryInterface( IXDisplayImpl *iface, REFIID iid, void **out )
{
    struct display *impl = impl_from_IXDisplayImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown      ) ||
        IsEqualGUID( iid, &IID_IXDisplayImpl ))
    {
        IXDisplayImpl_AddRef( *out = &impl->IXDisplayImpl_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_display_AddRef( IXDisplayImpl *iface )
{
    struct display *impl = impl_from_IXDisplayImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_display_Release( IXDisplayImpl *iface )
{
    struct display *impl = impl_from_IXDisplayImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI __PADDING__( IXDisplayImpl *iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does.\n", iface );
    return E_NOTIMPL;
}

static XDisplayHdrModeResult WINAPI x_display_XDisplayTryEnableHdrMode( IXDisplayImpl *iface, XDisplayHdrModePreference displayModePreference, XDisplayHdrModeInfo *displayHdrModeInfo )
{
    FIXME( "iface %p, displayModePreference %d, displayHdrModeInfo %p stub!\n", iface, displayModePreference, displayHdrModeInfo );
    return XDisplayHdrModeResult_Unknown;
}

static const struct IXDisplayImplVtbl x_display_vtbl =
{
    x_display_QueryInterface,
    x_display_AddRef,
    x_display_Release,
    /* IXDisplayImpl methods */
    __PADDING__,
    x_display_XDisplayTryEnableHdrMode,
};

static inline struct display *impl_from_IXLauncherImpl( IXLauncherImpl *iface )
{
    return CONTAINING_RECORD( iface, struct display, IXLauncherImpl_iface );
}

static HRESULT WINAPI x_launcher_QueryInterface( IXLauncherImpl *iface, REFIID iid, void **out )
{
    struct display *impl = impl_from_IXLauncherImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown       ) ||
        IsEqualGUID( iid, &IID_IXLauncherImpl ))
    {
        IXLauncherImpl_AddRef( *out = &impl->IXLauncherImpl_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_launcher_AddRef( IXLauncherImpl *iface )
{
    struct display *impl = impl_from_IXLauncherImpl( iface );
    return IXDisplayImpl_AddRef( &impl->IXDisplayImpl_iface );
}

static ULONG WINAPI x_launcher_Release( IXLauncherImpl *iface )
{
    struct display *impl = impl_from_IXLauncherImpl( iface );
    return IXDisplayImpl_Release( &impl->IXDisplayImpl_iface );
}

static HRESULT WINAPI x_launcher_XLaunchUri( IXLauncherImpl *iface, XUserHandle user, const char *uri )
{
    FIXME( "iface %p, user %p uri %s stub!\n", iface, user, debugstr_a( uri ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_launcher_XDisplayAcquireTimeoutDeferral( IXLauncherImpl *iface, XDisplayTimeoutDeferralHandle *handle )
{
    FIXME( "iface %p, handle %p stub!\n", iface, handle );
    return E_NOTIMPL;
}

static void WINAPI x_launcher_XDisplayCloseTimeoutDeferralHandle( IXLauncherImpl *iface, XDisplayTimeoutDeferralHandle handle )
{
    FIXME( "iface %p, handle %p stub!\n", iface, handle );
}

static const struct IXLauncherImplVtbl x_launcher_vtbl =
{
    x_launcher_QueryInterface,
    x_launcher_AddRef,
    x_launcher_Release,
    /* IXLauncherImpl methods */
    x_launcher_XLaunchUri,
    x_launcher_XDisplayAcquireTimeoutDeferral,
    x_launcher_XDisplayCloseTimeoutDeferralHandle,
};

static struct display display =
{
    {&x_display_vtbl},
    {&x_launcher_vtbl},
    0,
};

IXDisplayImpl *x_display_impl = &display.IXDisplayImpl_iface;
IXLauncherImpl *x_launcher_impl = &display.IXLauncherImpl_iface;
