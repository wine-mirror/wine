/*
 * Copyright 2016 Hans Leidekker for CodeWeavers
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

static void test_WsCreateChannel(void)
{
    HRESULT hr;
    WS_CHANNEL *channel;
    WS_CHANNEL_STATE state;
    ULONG size, value;

    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, NULL, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    channel = NULL;
    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, &channel, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );
    ok( channel != NULL, "channel not set\n" );

    value = 0xdeadbeef;
    size = sizeof(value);
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_MAX_BUFFERED_MESSAGE_SIZE, &value, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( value == 65536, "got %u\n", value );

    /* read-only property */
    state = 0xdeadbeef;
    size = sizeof(state);
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_STATE, &state, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == WS_CHANNEL_STATE_CREATED, "got %u\n", state );

    state = WS_CHANNEL_STATE_CREATED;
    size = sizeof(state);
    hr = WsSetChannelProperty( channel, WS_CHANNEL_PROPERTY_STATE, &state, size, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    WsFreeChannel( channel );
}

static void test_WsOpenChannel(void)
{
    WCHAR url[] = {'h','t','t','p',':','/','/','l','o','c','a','l','h','o','s','t'};
    HRESULT hr;
    WS_CHANNEL *channel;
    WS_ENDPOINT_ADDRESS addr;

    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, &channel, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    WsFreeChannel( channel );

    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, &channel, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsOpenChannel( channel, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    addr.url.length = sizeof(url)/sizeof(url[0]);
    addr.url.chars  = url;
    addr.headers    = NULL;
    addr.extensions = NULL;
    addr.identity   = NULL;
    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    WsFreeChannel( channel );
}

START_TEST(channel)
{
    test_WsCreateChannel();
    test_WsOpenChannel();
}
