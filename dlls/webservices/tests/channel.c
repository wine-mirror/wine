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

    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    channel = NULL;
    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, &channel, NULL );
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

    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    WsFreeChannel( channel );

    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsOpenChannel( channel, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    memset( &addr, 0, sizeof(addr) );
    addr.url.length = sizeof(url)/sizeof(url[0]);
    addr.url.chars  = url;
    hr = WsOpenChannel( NULL, &addr, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCloseChannel( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    WsFreeChannel( channel );
}

static void test_WsResetChannel(void)
{
    WCHAR url[] = {'h','t','t','p',':','/','/','l','o','c','a','l','h','o','s','t'};
    HRESULT hr;
    WS_CHANNEL *channel;
    WS_CHANNEL_STATE state;
    WS_CHANNEL_TYPE type;
    WS_ENDPOINT_ADDRESS addr;
    ULONG size, timeout;

    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsResetChannel( channel, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    timeout = 5000;
    size = sizeof(timeout);
    hr = WsSetChannelProperty( channel, WS_CHANNEL_PROPERTY_RESOLVE_TIMEOUT, &timeout, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    memset( &addr, 0, sizeof(addr) );
    addr.url.length = sizeof(url)/sizeof(url[0]);
    addr.url.chars  = url;
    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsResetChannel( channel, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsResetChannel( channel, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    state = 0xdeadbeef;
    size = sizeof(state);
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_STATE, &state, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == WS_CHANNEL_STATE_CREATED, "got %u\n", state );

    type = 0xdeadbeef;
    size = sizeof(type);
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_CHANNEL_TYPE, &type, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( type == WS_CHANNEL_TYPE_REQUEST, "got %u\n", type );

    timeout = 0xdeadbeef;
    size = sizeof(timeout);
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_RESOLVE_TIMEOUT, &timeout, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( timeout == 5000, "got %u\n", timeout );

    WsFreeChannel( channel );
}

struct listener_info
{
    int    port;
    HANDLE wait;
};

static DWORD CALLBACK listener_proc( void *arg )
{
    static const WCHAR fmt[] =
        {'s','o','a','p','.','u','d','p',':','/','/','l','o','c','a','l','h','o','s','t',':','%','u',0};
    struct listener_info *info = arg;
    WS_LISTENER *listener;
    WS_CHANNEL *channel;
    WS_MESSAGE *msg;
    WCHAR buf[64];
    WS_STRING url;
    HRESULT hr;

    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX, WS_UDP_CHANNEL_BINDING, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    url.length = wsprintfW( buf, fmt, info->port );
    url.chars  = buf;
    hr = WsOpenListener( listener, &url, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateChannelForListener( listener, NULL, 0, &channel, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    SetEvent( info->wait );

    hr = WsAcceptChannel( listener, channel, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadMessageStart( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsReadMessageStart( channel, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsReadMessageStart( NULL, msg, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsReadMessageStart( channel, msg, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsReadMessageEnd( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsReadMessageEnd( channel, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsReadMessageEnd( NULL, msg, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsReadMessageEnd( channel, msg, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    WsFreeMessage( msg );

    SetEvent( info->wait );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    WsFreeChannel( channel );

    hr = WsCloseListener( listener, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    WsFreeListener( listener );

    return 0;
}

static void test_message_read_write( const struct listener_info *info )
{
    static const WCHAR fmt[] =
        {'s','o','a','p','.','u','d','p',':','/','/','l','o','c','a','l','h','o','s','t',':','%','u',0};
    WS_ENDPOINT_ADDRESS addr;
    WCHAR buf[64];
    WS_CHANNEL *channel;
    WS_MESSAGE *msg;
    HRESULT hr;
    DWORD err;

    hr = WsCreateChannel( WS_CHANNEL_TYPE_DUPLEX, WS_UDP_CHANNEL_BINDING, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    memset( &addr, 0, sizeof(addr) );
    addr.url.length = wsprintfW( buf, fmt, info->port );
    addr.url.chars  = buf;
    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsInitializeMessage( msg,  WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteMessageStart( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteMessageStart( channel, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteMessageStart( NULL, msg, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteMessageStart( channel, msg, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsWriteMessageEnd( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteMessageEnd( channel, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteMessageEnd( NULL, msg, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsWriteMessageEnd( channel, msg, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    err = WaitForSingleObject( info->wait, 3000 );
    ok( err == WAIT_OBJECT_0, "wait failed %u\n", err );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    WsFreeMessage( msg );
    WsFreeChannel( channel );
}

static HANDLE start_listener( struct listener_info *info )
{
    DWORD err;
    HANDLE thread = CreateThread( NULL, 0, listener_proc, info, 0, NULL );
    ok( thread != NULL, "failed to create listener thread %u\n", GetLastError() );

    err = WaitForSingleObject( info->wait, 3000 );
    ok( err == WAIT_OBJECT_0, "failed to start listener %u\n", err );
    return thread;
}

START_TEST(channel)
{
    struct listener_info info;
    HANDLE thread;

    test_WsCreateChannel();
    test_WsOpenChannel();
    test_WsResetChannel();

    info.port = 7533;
    info.wait = CreateEventW( NULL, 0, 0, NULL );
    thread = start_listener( &info );

    test_message_read_write( &info );

    WaitForSingleObject( thread, 3000 );
}
