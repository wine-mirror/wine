/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XGameActivation
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

struct x_game_activation
{
    IXGameActivationImpl IXGameActivationImpl_iface;
    LONG ref;
};

static inline struct x_game_activation *impl_from_IXGameActivationImpl( IXGameActivationImpl *iface )
{
    return CONTAINING_RECORD( iface, struct x_game_activation, IXGameActivationImpl_iface );
}

static HRESULT WINAPI x_game_activation_QueryInterface( IXGameActivationImpl *iface, REFIID iid, void **out )
{
    struct x_game_activation *impl = impl_from_IXGameActivationImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown             ) ||
        IsEqualGUID( iid, &IID_IXGameActivationImpl ))
    {
        IXGameActivationImpl_AddRef( *out = &impl->IXGameActivationImpl_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_game_activation_AddRef( IXGameActivationImpl *iface )
{
    struct x_game_activation *impl = impl_from_IXGameActivationImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_game_activation_Release( IXGameActivationImpl *iface )
{
    struct x_game_activation *impl = impl_from_IXGameActivationImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_game_activation_XGameActivationRegisterForEvent( IXGameActivationImpl *iface, XTaskQueueHandle queue, void *context, XGameActivationCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, context %p, callback %p, token %p stub!\n", iface, queue, context, callback, token );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_game_activation_XGameActivationUnregisterForEvent( IXGameActivationImpl *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    FIXME( "iface %p, token %p, wait %d stub!\n", iface, &token, wait );
    return FALSE;
}

static HRESULT WINAPI x_game_activation_XGameActivationAcceptPendingInvite( IXGameActivationImpl *iface, const char *inviteUri )
{
    FIXME( "iface %p, inviteUri %s stub!\n", iface, debugstr_a( inviteUri ) );
    return E_NOTIMPL;
}

static const struct IXGameActivationImplVtbl x_game_activation_vtbl =
{
    x_game_activation_QueryInterface,
    x_game_activation_AddRef,
    x_game_activation_Release,
    /* IXGameActivationImpl methods */
    x_game_activation_XGameActivationRegisterForEvent,
    x_game_activation_XGameActivationUnregisterForEvent,
    x_game_activation_XGameActivationAcceptPendingInvite,
};

static struct x_game_activation x_game_activation =
{
    {&x_game_activation_vtbl},
    0,
};

IXGameActivationImpl *x_game_activation_impl = &x_game_activation.IXGameActivationImpl_iface;
