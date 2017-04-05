/*
 * Copyright 2017 Hans Leidekker for CodeWeavers
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

#include <stdio.h>
#include "windows.h"
#include "webservices.h"
#include "wine/test.h"

static void test_WsCreateListener(void)
{
    HRESULT hr;
    WS_LISTENER *listener;
    WS_CHANNEL_TYPE type;
    WS_CHANNEL_BINDING binding;
    WS_LISTENER_STATE state;
    WS_IP_VERSION version;
    ULONG size, backlog;

    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    listener = NULL;
    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( listener != NULL, "listener not set\n" );

    backlog = 1000;
    size = sizeof(backlog);
    hr = WsSetListenerProperty( listener, WS_LISTENER_PROPERTY_LISTEN_BACKLOG, &backlog, size, NULL );
    todo_wine ok( hr == E_INVALIDARG, "got %08x\n", hr );

    version = WS_IP_VERSION_4;
    size = sizeof(version);
    hr = WsSetListenerProperty( listener, WS_LISTENER_PROPERTY_IP_VERSION, &version, size, NULL );
    todo_wine ok( hr == E_INVALIDARG, "got %08x\n", hr );

    type = 0xdeadbeef;
    hr = WsGetListenerProperty( listener, WS_LISTENER_PROPERTY_CHANNEL_TYPE, &type, sizeof(type), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( type == WS_CHANNEL_TYPE_DUPLEX_SESSION, "got %u\n", type );

    binding = 0xdeadbeef;
    hr = WsGetListenerProperty( listener, WS_LISTENER_PROPERTY_CHANNEL_BINDING, &binding, sizeof(binding), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( binding == WS_TCP_CHANNEL_BINDING, "got %u\n", binding );

    version = 0;
    size = sizeof(version);
    hr = WsGetListenerProperty( listener, WS_LISTENER_PROPERTY_IP_VERSION, &version, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    todo_wine ok( version == WS_IP_VERSION_AUTO, "got %u\n", version );

    state = 0xdeadbeef;
    size = sizeof(state);
    hr = WsGetListenerProperty( listener, WS_LISTENER_PROPERTY_STATE, &state, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == WS_LISTENER_STATE_CREATED, "got %u\n", state );

    state = WS_LISTENER_STATE_CREATED;
    size = sizeof(state);
    hr = WsSetListenerProperty( listener, WS_LISTENER_PROPERTY_STATE, &state, size, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    WsFreeListener( listener );
}

START_TEST(listener)
{
    test_WsCreateListener();
}
