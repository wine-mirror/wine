/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XGameEvent
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

struct x_game_event
{
    IXGameEventImpl IXGameEventImpl_iface;
    LONG ref;
};

static inline struct x_game_event *impl_from_IXGameEventImpl( IXGameEventImpl *iface )
{
    return CONTAINING_RECORD( iface, struct x_game_event, IXGameEventImpl_iface );
}

static HRESULT WINAPI x_game_event_QueryInterface( IXGameEventImpl *iface, REFIID iid, void **out )
{
    struct x_game_event *impl = impl_from_IXGameEventImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown        ) ||
        IsEqualGUID( iid, &IID_IXGameEventImpl ))
    {
        IXGameEventImpl_AddRef( *out = &impl->IXGameEventImpl_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_game_event_AddRef( IXGameEventImpl *iface )
{
    struct x_game_event *impl = impl_from_IXGameEventImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_game_event_Release( IXGameEventImpl *iface )
{
    struct x_game_event *impl = impl_from_IXGameEventImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_game_event_XGameEventWrite( IXGameEventImpl *iface, XUserHandle user, const char *serviceConfigId, const char *playSessionId, const char *eventName, const char *dimensionsJson, const char *measurementsJson )
{
    FIXME( "iface %p, user %p, serviceConfigId %s, playSessionId %s, eventName %s, dimensionsJson %s, measurementsJson %s stub!\n", iface, user, debugstr_a( serviceConfigId ), debugstr_a( playSessionId ), debugstr_a( eventName ), debugstr_a( dimensionsJson ), debugstr_a( measurementsJson ) );
    return E_NOTIMPL;
}

static const struct IXGameEventImplVtbl x_game_event_vtbl =
{
    x_game_event_QueryInterface,
    x_game_event_AddRef,
    x_game_event_Release,
    /* IXGameEventImpl methods */
    x_game_event_XGameEventWrite,
};

static struct x_game_event x_game_event =
{
    {&x_game_event_vtbl},
    0,
};

IXGameEventImpl *x_game_event_impl = &x_game_event.IXGameEventImpl_iface;
