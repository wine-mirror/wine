/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XGameProtocol
 *
 * Written by LukasPAH
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

struct x_game_protocol
{
    IXGameProtocolImpl IXGameProtocolImpl_iface;
    LONG ref;
};

static inline struct x_game_protocol *impl_from_IXGameProtocolImpl( IXGameProtocolImpl *iface )
{
    return CONTAINING_RECORD( iface, struct x_game_protocol, IXGameProtocolImpl_iface );
}

static HRESULT WINAPI x_game_protocol_QueryInterface( IXGameProtocolImpl *iface, REFIID iid, void **out )
{
    struct x_game_protocol *impl = impl_from_IXGameProtocolImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown           ) ||
        IsEqualGUID( iid, &IID_IXGameProtocolImpl ))
    {
        IXGameProtocolImpl_AddRef( *out = &impl->IXGameProtocolImpl_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_game_protocol_AddRef( IXGameProtocolImpl *iface )
{
    struct x_game_protocol *impl = impl_from_IXGameProtocolImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_game_protocol_Release( IXGameProtocolImpl *iface )
{
    struct x_game_protocol *impl = impl_from_IXGameProtocolImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_game_protocol_XGameProtocolRegisterForActivation( IXGameProtocolImpl *iface, XTaskQueueHandle queue, void *context, XGameProtocolActivationCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, context %p, callback %p, token %p stub!\n", iface, queue, context, callback, token );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_game_protocol_XGameProtocolUnregisterForActivation( IXGameProtocolImpl *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    FIXME( "iface %p, token %p, wait %d stub!\n", iface, &token, wait );
    return TRUE;
}

static const struct IXGameProtocolImplVtbl x_game_protocol_vtbl =
{
    x_game_protocol_QueryInterface,
    x_game_protocol_AddRef,
    x_game_protocol_Release,
    /* IXGameProtocolImpl methods */
    x_game_protocol_XGameProtocolRegisterForActivation,
    x_game_protocol_XGameProtocolUnregisterForActivation,
};

static struct x_game_protocol x_game_protocol =
{
    {&x_game_protocol_vtbl},
    0,
};

IXGameProtocolImpl *x_game_protocol_impl = &x_game_protocol.IXGameProtocolImpl_iface;
