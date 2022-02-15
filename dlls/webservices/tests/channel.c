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
#define COBJMACROS
#include "windows.h"
#include "webservices.h"
#include "initguid.h"
#include "netfw.h"
#include "wine/test.h"

static void test_WsCreateChannel(void)
{
    HRESULT hr;
    WS_CHANNEL *channel;
    WS_CHANNEL_STATE state;
    WS_CHANNEL_PROPERTY prop;
    WS_ENCODING encoding;
    WS_ENVELOPE_VERSION env_version;
    WS_ADDRESSING_VERSION addr_version;
    ULONG size;

    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    channel = NULL;
    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( channel != NULL, "channel not set\n" );

    size = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_MAX_BUFFERED_MESSAGE_SIZE, &size, sizeof(size), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( size == 65536, "got %lu\n", size );

    encoding = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_ENCODING, &encoding, sizeof(encoding), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( encoding == WS_ENCODING_XML_UTF8, "got %u\n", encoding );

    env_version = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_ENVELOPE_VERSION, &env_version, sizeof(env_version),
                               NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( env_version == WS_ENVELOPE_VERSION_SOAP_1_2, "got %u\n", env_version );

    addr_version = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_ADDRESSING_VERSION, &addr_version, sizeof(addr_version),
                               NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( addr_version == WS_ADDRESSING_VERSION_1_0, "got %u\n", addr_version );

    /* read-only property */
    state = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_CHANNEL_STATE_CREATED, "got %u\n", state );

    state = WS_CHANNEL_STATE_CREATED;
    hr = WsSetChannelProperty( channel, WS_CHANNEL_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    encoding = WS_ENCODING_XML_UTF8;
    hr = WsSetChannelProperty( channel, WS_CHANNEL_PROPERTY_ENCODING, &encoding, sizeof(encoding), NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    WsFreeChannel( channel );

    encoding = WS_ENCODING_XML_UTF16LE;
    prop.id        = WS_CHANNEL_PROPERTY_ENCODING;
    prop.value     = &encoding;
    prop.valueSize = sizeof(encoding);
    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, &prop, 1, NULL, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    encoding = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_ENCODING, &encoding, sizeof(encoding), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( encoding == WS_ENCODING_XML_UTF16LE, "got %u\n", encoding );
    WsFreeChannel( channel );

    hr = WsCreateChannel( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    encoding = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_ENCODING, &encoding, sizeof(encoding), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( encoding == WS_ENCODING_XML_BINARY_SESSION_1, "got %u\n", encoding );

    env_version = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_ENVELOPE_VERSION, &env_version, sizeof(env_version),
                               NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( env_version == WS_ENVELOPE_VERSION_SOAP_1_2, "got %u\n", env_version );

    addr_version = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_ADDRESSING_VERSION, &addr_version, sizeof(addr_version),
                               NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( addr_version == WS_ADDRESSING_VERSION_1_0, "got %u\n", addr_version );
    WsFreeChannel( channel );
}

static void test_WsOpenChannel(void)
{
    HRESULT hr;
    WS_CHANNEL *channel;
    WS_CHANNEL_STATE state;
    WS_ENDPOINT_ADDRESS addr;

    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    WsFreeChannel( channel );

    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsOpenChannel( channel, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    memset( &addr, 0, sizeof(addr) );
    addr.url.length = ARRAY_SIZE( L"http://localhost" ) - 1;
    addr.url.chars  = (WCHAR *)L"http://localhost";
    hr = WsOpenChannel( NULL, &addr, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    state = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_CHANNEL_STATE_OPEN, "got %u\n", state );

    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    state = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_CHANNEL_STATE_CLOSED, "got %u\n", state );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCloseChannel( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    WsFreeChannel( channel );
}

static void test_WsResetChannel(void)
{
    HRESULT hr;
    WS_CHANNEL *channel;
    WS_CHANNEL_STATE state;
    WS_CHANNEL_TYPE type;
    WS_ENDPOINT_ADDRESS addr;
    ULONG size, timeout;

    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsResetChannel( channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    timeout = 5000;
    size = sizeof(timeout);
    hr = WsSetChannelProperty( channel, WS_CHANNEL_PROPERTY_RESOLVE_TIMEOUT, &timeout, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &addr, 0, sizeof(addr) );
    addr.url.length = ARRAY_SIZE( L"http://localhost" ) - 1;
    addr.url.chars  = (WCHAR *)L"http://localhost";
    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsResetChannel( channel, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsResetChannel( channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    state = 0xdeadbeef;
    size = sizeof(state);
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_STATE, &state, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_CHANNEL_STATE_CREATED, "got %u\n", state );

    type = 0xdeadbeef;
    size = sizeof(type);
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_CHANNEL_TYPE, &type, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( type == WS_CHANNEL_TYPE_REQUEST, "got %u\n", type );

    timeout = 0xdeadbeef;
    size = sizeof(timeout);
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_RESOLVE_TIMEOUT, &timeout, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( timeout == 5000, "got %lu\n", timeout );

    WsFreeChannel( channel );
}

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
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    listener = NULL;
    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( listener != NULL, "listener not set\n" );

    backlog = 1000;
    size = sizeof(backlog);
    hr = WsSetListenerProperty( listener, WS_LISTENER_PROPERTY_LISTEN_BACKLOG, &backlog, size, NULL );
    todo_wine ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    version = WS_IP_VERSION_4;
    size = sizeof(version);
    hr = WsSetListenerProperty( listener, WS_LISTENER_PROPERTY_IP_VERSION, &version, size, NULL );
    todo_wine ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    type = 0xdeadbeef;
    hr = WsGetListenerProperty( listener, WS_LISTENER_PROPERTY_CHANNEL_TYPE, &type, sizeof(type), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( type == WS_CHANNEL_TYPE_DUPLEX_SESSION, "got %u\n", type );

    binding = 0xdeadbeef;
    hr = WsGetListenerProperty( listener, WS_LISTENER_PROPERTY_CHANNEL_BINDING, &binding, sizeof(binding), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( binding == WS_TCP_CHANNEL_BINDING, "got %u\n", binding );

    version = 0;
    size = sizeof(version);
    hr = WsGetListenerProperty( listener, WS_LISTENER_PROPERTY_IP_VERSION, &version, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    todo_wine ok( version == WS_IP_VERSION_AUTO, "got %u\n", version );

    state = 0xdeadbeef;
    size = sizeof(state);
    hr = WsGetListenerProperty( listener, WS_LISTENER_PROPERTY_STATE, &state, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_LISTENER_STATE_CREATED, "got %u\n", state );

    state = WS_LISTENER_STATE_CREATED;
    size = sizeof(state);
    hr = WsSetListenerProperty( listener, WS_LISTENER_PROPERTY_STATE, &state, size, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    WsFreeListener( listener );
}

static void test_WsOpenListener(void)
{
    WS_STRING url;
    WS_LISTENER *listener;
    HRESULT hr;

    hr = WsOpenListener( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCloseListener( listener, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    WsFreeListener( listener );

    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsOpenListener( listener, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );
    url.length = ARRAY_SIZE( L"net.tcp://+:2017/path" ) - 1;
    url.chars  = (WCHAR *)L"net.tcp://+:2017/path";
    hr = WsOpenListener( NULL, &url, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsOpenListener( listener, &url, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsOpenListener( listener, &url, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsCloseListener( listener, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    WsFreeListener( listener );

    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    url.length = ARRAY_SIZE( L"net.tcp://localhost:2017" ) - 1;
    url.chars  = (WCHAR *)L"net.tcp://localhost:2017";
    hr = WsOpenListener( listener, &url, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCloseListener( listener, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    WsFreeListener( listener );

    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    url.length = ARRAY_SIZE( L"net.tcp://127.0.0.1:2017" ) - 1;
    url.chars  = (WCHAR *)L"net.tcp://127.0.0.1:2017";
    hr = WsOpenListener( listener, &url, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCloseListener( listener, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCloseListener( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    WsFreeListener( listener );
}

static void test_WsCreateChannelForListener(void)
{
    WS_LISTENER *listener;
    WS_CHANNEL *channel;
    WS_CHANNEL_TYPE type;
    WS_CHANNEL_STATE state;
    HRESULT hr;

    hr = WsCreateChannelForListener( NULL, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateChannelForListener( NULL, NULL, 0, &channel, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    channel = NULL;
    hr = WsCreateChannelForListener( listener, NULL, 0, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( channel != NULL, "channel not set\n" );

    type = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_CHANNEL_TYPE, &type, sizeof(type), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( type == WS_CHANNEL_TYPE_DUPLEX_SESSION, "got %u\n", type );

    state = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_CHANNEL_STATE_CREATED, "got %u\n", state );

    WsFreeChannel( channel );
    WsFreeListener( listener );
}

static void test_WsResetListener(void)
{
    WS_STRING url = { ARRAY_SIZE( L"net.tcp://+:2017/path" ) - 1, (WCHAR *)L"net.tcp://+:2017/path" };
    WS_LISTENER *listener;
    WS_LISTENER_STATE state;
    WS_LISTENER_PROPERTY prop;
    ULONG size, timeout = 1000;
    HRESULT hr;

    hr = WsResetListener( NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    prop.id        = WS_LISTENER_PROPERTY_CONNECT_TIMEOUT;
    prop.value     = &timeout;
    prop.valueSize = sizeof(timeout);
    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, &prop, 1, NULL, &listener, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsResetListener( listener, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsOpenListener( listener, &url, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsResetListener( listener, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsCloseListener( listener, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsResetListener( listener, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    state = 0xdeadbeef;
    size = sizeof(state);
    hr = WsGetListenerProperty( listener, WS_LISTENER_PROPERTY_STATE, &state, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_LISTENER_STATE_CREATED, "got %u\n", state );

    timeout = 0xdeadbeef;
    size = sizeof(timeout);
    hr = WsGetListenerProperty( listener, WS_LISTENER_PROPERTY_CONNECT_TIMEOUT, &timeout, size, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( timeout == 1000, "got %lu\n", timeout );

    WsFreeListener( listener );
}

struct listener_info
{
    int                 port;
    HANDLE              ready;
    HANDLE              done;
    WS_CHANNEL_BINDING  binding;
    WS_CHANNEL_TYPE     type;
    void                (*server_func)( WS_CHANNEL * );
};

static void server_message_read_write( WS_CHANNEL *channel )
{
    WS_MESSAGE *msg;
    HRESULT hr;

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadMessageStart( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadMessageStart( channel, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadMessageStart( NULL, msg, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadMessageStart( channel, msg, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsReadMessageEnd( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadMessageEnd( channel, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadMessageEnd( NULL, msg, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsReadMessageEnd( channel, msg, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    WsFreeMessage( msg );
}

static void client_message_read_write( const struct listener_info *info )
{
    WS_ENDPOINT_ADDRESS addr;
    WCHAR buf[64];
    WS_CHANNEL *channel;
    WS_MESSAGE *msg;
    HRESULT hr;
    DWORD err;

    err = WaitForSingleObject( info->ready, 3000 );
    ok( err == WAIT_OBJECT_0, "wait failed %lu\n", err );

    hr = WsCreateChannel( info->type, info->binding, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &addr, 0, sizeof(addr) );
    addr.url.length = wsprintfW( buf, L"soap.udp://localhost:%u", info->port );
    addr.url.chars  = buf;
    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsShutdownSessionChannel( channel, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteMessageStart( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteMessageStart( channel, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteMessageStart( NULL, msg, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteMessageStart( channel, msg, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsWriteMessageEnd( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteMessageEnd( channel, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteMessageEnd( NULL, msg, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsWriteMessageEnd( channel, msg, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    err = WaitForSingleObject( info->done, 3000 );
    ok( err == WAIT_OBJECT_0, "wait failed %lu\n", err );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    WsFreeMessage( msg );
    WsFreeChannel( channel );
}

struct async_test
{
    ULONG  call_count;
    HANDLE wait;
};

static void CALLBACK async_callback( HRESULT hr, WS_CALLBACK_MODEL model, void *state )
{
    struct async_test *test = state;

    ok( hr == S_OK, "got %#lx\n", hr );
    ok( model == WS_LONG_CALLBACK, "got %u\n", model );
    test->call_count++;
    SetEvent( test->wait );
}

static void server_duplex_session( WS_CHANNEL *channel )
{
    WS_XML_STRING action = {6, (BYTE *)"action"}, localname = {9, (BYTE *)"localname"}, ns = {2, (BYTE *)"ns"};
    WS_ELEMENT_DESCRIPTION desc_body;
    const WS_MESSAGE_DESCRIPTION *desc[1];
    WS_MESSAGE_DESCRIPTION desc_msg;
    struct async_test test;
    WS_ASYNC_CONTEXT ctx;
    WS_MESSAGE *msg, *msg2;
    INT32 val = 0;
    HRESULT hr;

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    desc_body.elementLocalName = &localname;
    desc_body.elementNs        = &ns;
    desc_body.type             = WS_INT32_TYPE;
    desc_body.typeDescription  = NULL;
    desc_msg.action                 = &action;
    desc_msg.bodyElementDescription = &desc_body;
    desc[0] = &desc_msg;

    test.call_count = 0;
    test.wait       = CreateEventW( NULL, FALSE, FALSE, NULL );
    ctx.callback      = async_callback;
    ctx.callbackState = &test;

    hr = WsReceiveMessage( channel, msg, desc, 1, WS_RECEIVE_OPTIONAL_MESSAGE, WS_READ_REQUIRED_VALUE,
                           NULL, &val, sizeof(val), NULL, &ctx, NULL );
    ok( hr == WS_S_ASYNC || hr == S_OK, "got %#lx\n", hr );
    if (hr == WS_S_ASYNC)
    {
        WaitForSingleObject( test.wait, INFINITE );
        ok( test.call_count == 1, "got %lu\n", test.call_count );
    }
    else ok( !test.call_count, "got %lu\n", test.call_count );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    test.call_count = 0;
    hr = WsReceiveMessage( channel, msg2, desc, 1, WS_RECEIVE_OPTIONAL_MESSAGE, WS_READ_REQUIRED_VALUE,
                           NULL, &val, sizeof(val), NULL, &ctx, NULL );
    ok( hr == WS_S_ASYNC || hr == S_OK, "got %#lx\n", hr );
    if (hr == WS_S_ASYNC)
    {
        WaitForSingleObject( test.wait, INFINITE );
        ok( test.call_count == 1, "got %lu\n", test.call_count );
    }
    else ok( !test.call_count, "got %lu\n", test.call_count );

    CloseHandle( test.wait );
    WsFreeMessage( msg );
    WsFreeMessage( msg2 );
}

static void client_duplex_session( const struct listener_info *info )
{
    WS_XML_STRING action = {6, (BYTE *)"action"}, localname = {9, (BYTE *)"localname"}, ns = {2, (BYTE *)"ns"};
    WS_ELEMENT_DESCRIPTION desc_body;
    WS_MESSAGE_DESCRIPTION desc;
    WS_ENDPOINT_ADDRESS addr;
    WCHAR buf[64];
    WS_CHANNEL *channel;
    WS_MESSAGE *msg, *msg2;
    INT32 val = -1;
    HRESULT hr;
    DWORD err;

    err = WaitForSingleObject( info->ready, 3000 );
    ok( err == WAIT_OBJECT_0, "wait failed %lu\n", err );

    hr = WsCreateChannel( info->type, info->binding, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsShutdownSessionChannel( channel, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    memset( &addr, 0, sizeof(addr) );
    addr.url.length = wsprintfW( buf, L"net.tcp://localhost:%u", info->port );
    addr.url.chars  = buf;
    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    desc_body.elementLocalName = &localname;
    desc_body.elementNs        = &ns;
    desc_body.type             = WS_INT32_TYPE;
    desc_body.typeDescription  = NULL;
    desc.action                 = &action;
    desc.bodyElementDescription = &desc_body;

    hr = WsSendMessage( channel, msg, &desc, WS_WRITE_REQUIRED_VALUE, &val, sizeof(val), NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsSendMessage( channel, msg2, &desc, WS_WRITE_REQUIRED_VALUE, &val, sizeof(val), NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    err = WaitForSingleObject( info->done, 3000 );
    ok( err == WAIT_OBJECT_0, "wait failed %lu\n", err );

    hr = WsShutdownSessionChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsShutdownSessionChannel( channel, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    WsFreeMessage( msg );
    WsFreeMessage( msg2 );
    WsFreeChannel( channel );
}

static void client_duplex_session_async( const struct listener_info *info )
{
    WS_XML_STRING action = {6, (BYTE *)"action"}, localname = {9, (BYTE *)"localname"}, ns = {2, (BYTE *)"ns"};
    WS_ELEMENT_DESCRIPTION desc_body;
    WS_MESSAGE_DESCRIPTION desc;
    WS_ENDPOINT_ADDRESS addr;
    WS_ASYNC_CONTEXT ctx;
    struct async_test test;
    WCHAR buf[64];
    WS_CHANNEL *channel;
    WS_MESSAGE *msg, *msg2;
    INT32 val = -1;
    HRESULT hr;
    DWORD err;

    err = WaitForSingleObject( info->ready, 3000 );
    ok( err == WAIT_OBJECT_0, "wait failed %lu\n", err );

    hr = WsCreateChannel( info->type, info->binding, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsShutdownSessionChannel( channel, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    test.call_count = 0;
    test.wait       = CreateEventW( NULL, FALSE, FALSE, NULL );
    ctx.callback      = async_callback;
    ctx.callbackState = &test;

    memset( &addr, 0, sizeof(addr) );
    addr.url.length = wsprintfW( buf, L"net.tcp://localhost:%u", info->port );
    addr.url.chars  = buf;
    hr = WsOpenChannel( channel, &addr, &ctx, NULL );
    ok( hr == WS_S_ASYNC || hr == S_OK, "got %#lx\n", hr );
    if (hr == WS_S_ASYNC)
    {
        WaitForSingleObject( test.wait, INFINITE );
        ok( test.call_count == 1, "got %lu\n", test.call_count );
    }
    else ok( !test.call_count, "got %lu\n", test.call_count );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    desc_body.elementLocalName = &localname;
    desc_body.elementNs        = &ns;
    desc_body.type             = WS_INT32_TYPE;
    desc_body.typeDescription  = NULL;
    desc.action                 = &action;
    desc.bodyElementDescription = &desc_body;

    /* asynchronous call */
    test.call_count = 0;
    hr = WsSendMessage( channel, msg, &desc, WS_WRITE_REQUIRED_VALUE, &val, sizeof(val), &ctx, NULL );
    ok( hr == WS_S_ASYNC || hr == S_OK, "got %#lx\n", hr );
    if (hr == WS_S_ASYNC)
    {
        WaitForSingleObject( test.wait, INFINITE );
        ok( test.call_count == 1, "got %lu\n", test.call_count );
    }
    else ok( !test.call_count, "got %lu\n", test.call_count );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg2, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    /* synchronous call */
    hr = WsSendMessage( channel, msg2, &desc, WS_WRITE_REQUIRED_VALUE, &val, sizeof(val), NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    test.call_count = 0;
    hr = WsShutdownSessionChannel( channel, &ctx, NULL );
    ok( hr == WS_S_ASYNC || hr == S_OK, "got %#lx\n", hr );
    if (hr == WS_S_ASYNC)
    {
        WaitForSingleObject( test.wait, INFINITE );
        ok( test.call_count == 1, "got %lu\n", test.call_count );
    }
    else ok( !test.call_count, "got %lu\n", test.call_count );

    hr = WsShutdownSessionChannel( channel, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );

    test.call_count = 0;
    hr = WsCloseChannel( channel, &ctx, NULL );
    ok( hr == WS_S_ASYNC || hr == S_OK, "got %#lx\n", hr );
    if (hr == WS_S_ASYNC)
    {
        WaitForSingleObject( test.wait, INFINITE );
        ok( test.call_count == 1, "got %lu\n", test.call_count );
    }
    else ok( !test.call_count, "got %lu\n", test.call_count );

    err = WaitForSingleObject( info->done, 3000 );
    ok( err == WAIT_OBJECT_0, "wait failed %lu\n", err );

    CloseHandle( test.wait );
    WsFreeMessage( msg );
    WsFreeMessage( msg2 );
    WsFreeChannel( channel );
}

static void server_accept_channel( WS_CHANNEL *channel )
{
    WS_XML_STRING localname = {9, (BYTE *)"localname"}, ns = {2, (BYTE *)"ns"}, action = {6, (BYTE *)"action"};
    WS_ELEMENT_DESCRIPTION body;
    WS_MESSAGE_DESCRIPTION desc_resp;
    const WS_MESSAGE_DESCRIPTION *desc[1];
    WS_MESSAGE *msg;
    INT32 val = 0;
    HRESULT hr;

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    body.elementLocalName = &localname;
    body.elementNs        = &ns;
    body.type             = WS_INT32_TYPE;
    body.typeDescription  = NULL;

    desc_resp.action                 = &action;
    desc_resp.bodyElementDescription = &body;
    desc[0] = &desc_resp;

    hr = WsReceiveMessage( channel, msg, desc, 1, WS_RECEIVE_REQUIRED_MESSAGE, WS_READ_REQUIRED_VALUE,
                           NULL, &val, sizeof(val), NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val == -1, "got %d\n", val );
    WsFreeMessage( msg );
}

static void client_accept_channel( const struct listener_info *info )
{
    const WCHAR *fmt = (info->binding == WS_TCP_CHANNEL_BINDING) ?  L"net.tcp://localhost:%u" : L"soap.udp://localhost:%u";
    WS_XML_STRING localname = {9, (BYTE *)"localname"}, ns = {2, (BYTE *)"ns"}, action = {6, (BYTE *)"action"};
    WCHAR buf[64];
    WS_LISTENER *listener;
    WS_CHANNEL *channel;
    WS_MESSAGE *msg;
    WS_ENDPOINT_ADDRESS addr;
    WS_ELEMENT_DESCRIPTION body;
    WS_MESSAGE_DESCRIPTION desc;
    INT32 val = -1;
    HRESULT hr;
    DWORD err;

    err = WaitForSingleObject( info->ready, 3000 );
    ok( err == WAIT_OBJECT_0, "wait failed %lu\n", err );

    hr = WsAcceptChannel( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateListener( info->type, info->binding, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsAcceptChannel( listener, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsCreateChannelForListener( listener, NULL, 0, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsAcceptChannel( listener, channel, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %#lx\n", hr );
    WsFreeChannel( channel );
    WsFreeListener( listener );

    hr = WsCreateChannel( info->type, info->binding, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &addr, 0, sizeof(addr) );
    addr.url.length = wsprintfW( buf, fmt, info->port );
    addr.url.chars  = buf;
    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    body.elementLocalName = &localname;
    body.elementNs        = &ns;
    body.type             = WS_INT32_TYPE;
    body.typeDescription  = NULL;

    desc.action                 = &action;
    desc.bodyElementDescription = &body;

    hr = WsSendMessage( channel, msg, &desc, WS_WRITE_REQUIRED_VALUE, &val, sizeof(val), NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    err = WaitForSingleObject( info->done, 3000 );
    ok( err == WAIT_OBJECT_0, "wait failed %lu\n", err );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    WsFreeMessage( msg );
    WsFreeChannel( channel );
}

static void server_request_reply( WS_CHANNEL *channel )
{
    WS_XML_STRING localname = {9, (BYTE *)"localname"}, ns = {2, (BYTE *)"ns"};
    WS_XML_STRING req_action = {7, (BYTE *)"request"}, reply_action= {5, (BYTE *)"reply"};
    WS_ELEMENT_DESCRIPTION body;
    WS_MESSAGE_DESCRIPTION in_desc, out_desc;
    const WS_MESSAGE_DESCRIPTION *desc[1];
    WS_MESSAGE *msg;
    INT32 val = 0;
    HRESULT hr;

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    body.elementLocalName = &localname;
    body.elementNs        = &ns;
    body.type             = WS_INT32_TYPE;
    body.typeDescription  = NULL;

    in_desc.action                 = &req_action;
    in_desc.bodyElementDescription = &body;
    desc[0] = &in_desc;

    hr = WsReceiveMessage( channel, msg, desc, 1, WS_RECEIVE_REQUIRED_MESSAGE, WS_READ_REQUIRED_VALUE,
                           NULL, &val, sizeof(val), NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( val == -1, "got %d\n", val );
    WsFreeMessage( msg );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    out_desc.action                 = &reply_action;
    out_desc.bodyElementDescription = &body;

    hr = WsSendMessage( channel, msg, &out_desc, WS_WRITE_REQUIRED_VALUE, &val, sizeof(val), NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    WsFreeMessage( msg );
}

static void client_request_reply( const struct listener_info *info )
{
    WS_XML_STRING localname = {9, (BYTE *)"localname"}, ns = {2, (BYTE *)"ns"};
    WS_XML_STRING req_action = {7, (BYTE *)"request"}, reply_action= {5, (BYTE *)"reply"};
    WCHAR buf[64];
    WS_CHANNEL *channel;
    WS_MESSAGE *req, *reply;
    WS_ENDPOINT_ADDRESS addr;
    WS_ELEMENT_DESCRIPTION body;
    WS_MESSAGE_DESCRIPTION req_desc, reply_desc;
    INT32 val_in = -1, val_out = 0;
    HRESULT hr;
    DWORD err;

    err = WaitForSingleObject( info->ready, 3000 );
    ok( err == WAIT_OBJECT_0, "wait failed %lu\n", err );

    hr = WsCreateChannel( info->type, info->binding, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    memset( &addr, 0, sizeof(addr) );
    addr.url.length = wsprintfW( buf, L"net.tcp://localhost:%u", info->port );
    addr.url.chars  = buf;
    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &req, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &reply, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsInitializeMessage( req, WS_BLANK_MESSAGE, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    body.elementLocalName = &localname;
    body.elementNs        = &ns;
    body.type             = WS_INT32_TYPE;
    body.typeDescription  = NULL;

    req_desc.action                 = &req_action;
    req_desc.bodyElementDescription = &body;

    reply_desc.action                 = &reply_action;
    reply_desc.bodyElementDescription = &body;

    hr = WsRequestReply( channel, req, &req_desc, WS_WRITE_REQUIRED_VALUE, &val_in, sizeof(val_in), reply,
                         &reply_desc, WS_READ_REQUIRED_VALUE, NULL, &val_out, sizeof(val_out), NULL, NULL );
    ok( val_out == -1, "got %d\n", val_out );

    err = WaitForSingleObject( info->done, 3000 );
    ok( err == WAIT_OBJECT_0, "wait failed %lu\n", err );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    WsFreeMessage( req );
    WsFreeMessage( reply );
    WsFreeChannel( channel );
}

static DWORD CALLBACK listener_proc( void *arg )
{
    struct listener_info *info = arg;
    const WCHAR *fmt = (info->binding == WS_TCP_CHANNEL_BINDING) ? L"net.tcp://localhost:%u" : L"soap.udp://localhost:%u";
    WS_LISTENER *listener;
    WS_CHANNEL *channel;
    WS_CHANNEL_STATE state;
    WCHAR buf[64];
    WS_STRING url;
    HRESULT hr;

    hr = WsCreateListener( info->type, info->binding, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    url.length = wsprintfW( buf, fmt, info->port );
    url.chars  = buf;
    hr = WsOpenListener( listener, &url, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsCreateChannelForListener( listener, NULL, 0, &channel, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    SetEvent( info->ready );

    hr = WsAcceptChannel( listener, channel, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    state = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( state == WS_CHANNEL_STATE_OPEN, "got %u\n", state );

    info->server_func( channel );

    SetEvent( info->done );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    WsFreeChannel( channel );

    hr = WsCloseListener( listener, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    WsFreeListener( listener );

    return 0;
}

static HANDLE start_listener( struct listener_info *info )
{
    HANDLE thread = CreateThread( NULL, 0, listener_proc, info, 0, NULL );
    ok( thread != NULL, "failed to create listener thread %lu\n", GetLastError() );
    return thread;
}

enum firewall_op
{
    APP_ADD,
    APP_REMOVE
};

static BOOL is_process_elevated(void)
{
    HANDLE token;
    if (OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token ))
    {
        TOKEN_ELEVATION_TYPE type;
        DWORD size;
        BOOL ret;

        ret = GetTokenInformation( token, TokenElevationType, &type, sizeof(type), &size );
        CloseHandle( token );
        return (ret && type == TokenElevationTypeFull);
    }
    return FALSE;
}

static BOOL is_firewall_enabled(void)
{
    HRESULT hr, init;
    INetFwMgr *mgr = NULL;
    INetFwPolicy *policy = NULL;
    INetFwProfile *profile = NULL;
    VARIANT_BOOL enabled = VARIANT_FALSE;

    init = CoInitializeEx( 0, COINIT_APARTMENTTHREADED );

    hr = CoCreateInstance( &CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwMgr,
                           (void **)&mgr );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_FirewallEnabled( profile, &enabled );
    ok( hr == S_OK, "got %#lx\n", hr );

done:
    if (policy) INetFwPolicy_Release( policy );
    if (profile) INetFwProfile_Release( profile );
    if (mgr) INetFwMgr_Release( mgr );
    if (SUCCEEDED( init )) CoUninitialize();
    return (enabled == VARIANT_TRUE);
}

static HRESULT set_firewall( enum firewall_op op )
{
    HRESULT hr, init;
    INetFwMgr *mgr = NULL;
    INetFwPolicy *policy = NULL;
    INetFwProfile *profile = NULL;
    INetFwAuthorizedApplication *app = NULL;
    INetFwAuthorizedApplications *apps = NULL;
    BSTR name, image = SysAllocStringLen( NULL, MAX_PATH );

    if (!GetModuleFileNameW( NULL, image, MAX_PATH ))
    {
        SysFreeString( image );
        return E_FAIL;
    }
    init = CoInitializeEx( 0, COINIT_APARTMENTTHREADED );

    hr = CoCreateInstance( &CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwMgr,
                           (void **)&mgr );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_AuthorizedApplications( profile, &apps );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (hr != S_OK) goto done;

    hr = CoCreateInstance( &CLSID_NetFwAuthorizedApplication, NULL, CLSCTX_INPROC_SERVER,
                           &IID_INetFwAuthorizedApplication, (void **)&app );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwAuthorizedApplication_put_ProcessImageFileName( app, image );
    if (hr != S_OK) goto done;

    name = SysAllocString( L"webservices_test" );
    hr = INetFwAuthorizedApplication_put_Name( app, name );
    SysFreeString( name );
    ok( hr == S_OK, "got %#lx\n", hr );
    if (hr != S_OK) goto done;

    if (op == APP_ADD)
        hr = INetFwAuthorizedApplications_Add( apps, app );
    else if (op == APP_REMOVE)
        hr = INetFwAuthorizedApplications_Remove( apps, image );
    else
        hr = E_INVALIDARG;

done:
    if (app) INetFwAuthorizedApplication_Release( app );
    if (apps) INetFwAuthorizedApplications_Release( apps );
    if (policy) INetFwPolicy_Release( policy );
    if (profile) INetFwProfile_Release( profile );
    if (mgr) INetFwMgr_Release( mgr );
    if (SUCCEEDED( init )) CoUninitialize();
    SysFreeString( image );
    return hr;
}

START_TEST(channel)
{
    BOOL firewall_enabled = is_firewall_enabled();
    struct listener_info info;
    HANDLE thread;
    HRESULT hr;
    unsigned int i;
    static const struct test
    {
        WS_CHANNEL_BINDING binding;
        WS_CHANNEL_TYPE    type;
        void               (*server_func)( WS_CHANNEL * );
        void               (*client_func)( const struct listener_info * );
    }
    tests[] =
    {
        { WS_UDP_CHANNEL_BINDING, WS_CHANNEL_TYPE_DUPLEX, server_message_read_write, client_message_read_write },
        { WS_TCP_CHANNEL_BINDING, WS_CHANNEL_TYPE_DUPLEX_SESSION, server_duplex_session, client_duplex_session },
        { WS_UDP_CHANNEL_BINDING, WS_CHANNEL_TYPE_DUPLEX, server_accept_channel, client_accept_channel },
        { WS_TCP_CHANNEL_BINDING, WS_CHANNEL_TYPE_DUPLEX_SESSION, server_accept_channel, client_accept_channel },
        { WS_TCP_CHANNEL_BINDING, WS_CHANNEL_TYPE_DUPLEX_SESSION, server_request_reply, client_request_reply },
        { WS_TCP_CHANNEL_BINDING, WS_CHANNEL_TYPE_DUPLEX_SESSION, server_duplex_session, client_duplex_session_async },
    };

    if (firewall_enabled)
    {
        if (!is_process_elevated())
        {
            skip( "no privileges, skipping tests to avoid firewall dialog\n" );
            return;
        }
        if ((hr = set_firewall( APP_ADD )) != S_OK)
        {
            skip( "can't authorize app in firewall %#lx\n", hr );
            return;
        }
    }

    test_WsCreateChannel();
    test_WsOpenChannel();
    test_WsResetChannel();
    test_WsCreateListener();
    test_WsOpenListener();
    test_WsCreateChannelForListener();
    test_WsResetListener();

    info.port = 7533;
    info.ready = CreateEventW( NULL, 0, 0, NULL );
    info.done = CreateEventW( NULL, 0, 0, NULL );

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        info.binding     = tests[i].binding;
        info.type        = tests[i].type;
        info.server_func = tests[i].server_func;

        thread = start_listener( &info );
        tests[i].client_func( &info );
        WaitForSingleObject( thread, 3000 );
        CloseHandle( thread );
    }

    if (firewall_enabled) set_firewall( APP_REMOVE );
}
