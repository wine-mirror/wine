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

static void test_WsOpenListener(void)
{
    WCHAR str[] =
        {'n','e','t','.','t','c','p',':','/','/','+',':','2','0','1','7','/','p','a','t','h'};
    WCHAR str2[] =
        {'n','e','t','.','t','c','p',':','/','/','l','o','c','a','l','h','o','s','t',':','2','0','1','7'};
    WCHAR str3[] =
        {'n','e','t','.','t','c','p',':','/','/','1','2','7','.','0','.','0','.','1',':','2','0','1','7'};
    WS_STRING url;
    WS_LISTENER *listener;
    HRESULT hr;

    hr = WsOpenListener( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCloseListener( listener, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    WsFreeListener( listener );

    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsOpenListener( listener, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    url.length = sizeof(str)/sizeof(str[0]);
    url.chars  = str;
    hr = WsOpenListener( NULL, &url, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsOpenListener( listener, &url, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsOpenListener( listener, &url, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = WsCloseListener( listener, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    WsFreeListener( listener );

    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    url.length = sizeof(str2)/sizeof(str2[0]);
    url.chars  = str2;
    hr = WsOpenListener( listener, &url, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCloseListener( listener, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    WsFreeListener( listener );

    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    url.length = sizeof(str3)/sizeof(str3[0]);
    url.chars  = str3;
    hr = WsOpenListener( listener, &url, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCloseListener( listener, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCloseListener( NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

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
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateChannelForListener( NULL, NULL, 0, &channel, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    channel = NULL;
    hr = WsCreateChannelForListener( listener, NULL, 0, &channel, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( channel != NULL, "channel not set\n" );

    type = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_CHANNEL_TYPE, &type, sizeof(type), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( type == WS_CHANNEL_TYPE_DUPLEX_SESSION, "got %u\n", type );

    state = 0xdeadbeef;
    hr = WsGetChannelProperty( channel, WS_CHANNEL_PROPERTY_STATE, &state, sizeof(state), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == WS_CHANNEL_STATE_CREATED, "got %u\n", state );

    WsFreeChannel( channel );
    WsFreeListener( listener );
}

static void test_WsResetListener(void)
{
    WCHAR str[] =
        {'n','e','t','.','t','c','p',':','/','/','+',':','2','0','1','7','/','p','a','t','h'};
    WS_STRING url = { sizeof(str)/sizeof(str[0]), str };
    WS_LISTENER *listener;
    WS_LISTENER_STATE state;
    WS_LISTENER_PROPERTY prop;
    ULONG size, timeout = 1000;
    HRESULT hr;

    hr = WsResetListener( NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    prop.id        = WS_LISTENER_PROPERTY_CONNECT_TIMEOUT;
    prop.value     = &timeout;
    prop.valueSize = sizeof(timeout);
    hr = WsCreateListener( WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, &prop, 1, NULL, &listener, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsResetListener( listener, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsOpenListener( listener, &url, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsResetListener( listener, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = WsCloseListener( listener, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsResetListener( listener, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    state = 0xdeadbeef;
    size = sizeof(state);
    hr = WsGetListenerProperty( listener, WS_LISTENER_PROPERTY_STATE, &state, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == WS_LISTENER_STATE_CREATED, "got %u\n", state );

    timeout = 0xdeadbeef;
    size = sizeof(timeout);
    hr = WsGetListenerProperty( listener, WS_LISTENER_PROPERTY_CONNECT_TIMEOUT, &timeout, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( timeout == 1000, "got %u\n", timeout );

    WsFreeListener( listener );
}

struct listener_info
{
    int                port;
    HANDLE             wait;
    WS_CHANNEL_BINDING binding;
    WS_CHANNEL_TYPE    type;
};

static DWORD CALLBACK listener_proc( void *arg )
{
    static const WCHAR fmt_tcp[] = {'n','e','t','.','t','c','p',':','/','/','+',':','%','u',0};
    static const WCHAR fmt_udp[] = {'s','o','a','p','.','u','d','p',':','/','/','+',':','%','u',0};
    struct listener_info *info = arg;
    const WCHAR *fmt = (info->binding == WS_TCP_CHANNEL_BINDING) ? fmt_tcp : fmt_udp;
    WS_XML_STRING localname = {9, (BYTE *)"localname"}, ns = {2, (BYTE *)"ns"}, action = {6, (BYTE *)"action"};
    WS_ELEMENT_DESCRIPTION body;
    WS_MESSAGE_DESCRIPTION desc_resp;
    const WS_MESSAGE_DESCRIPTION *desc[1];
    INT32 val = 0;
    WS_LISTENER *listener;
    WS_CHANNEL *channel;
    WS_MESSAGE *msg;
    WCHAR buf[64];
    WS_STRING url;
    HRESULT hr;

    hr = WsCreateListener( info->type, info->binding, NULL, 0, NULL, &listener, NULL );
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

    hr = WsAcceptChannel( listener, channel, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    body.elementLocalName = &localname;
    body.elementNs        = &ns;
    body.type             = WS_INT32_TYPE;
    body.typeDescription  = NULL;

    desc_resp.action                 = &action;
    desc_resp.bodyElementDescription = &body;
    desc[0] = &desc_resp;

    hr = WsReceiveMessage( channel, msg, desc, 1, WS_RECEIVE_REQUIRED_MESSAGE, WS_READ_REQUIRED_VALUE,
                           NULL, &val, sizeof(val), NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val == -1, "got %d\n", val );
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

static void test_WsAcceptChannel( const struct listener_info *info )
{
    static const WCHAR fmt_tcp[] =
        {'n','e','t','.','t','c','p',':','/','/','l','o','c','a','l','h','o','s','t',':','%','u',0};
    static const WCHAR fmt_udp[] =
        {'s','o','a','p','.','u','d','p',':','/','/','l','o','c','a','l','h','o','s','t',':','%','u',0};
    const WCHAR *fmt = (info->binding == WS_TCP_CHANNEL_BINDING) ? fmt_tcp : fmt_udp;
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

    hr = WsAcceptChannel( NULL, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateListener( info->type, info->binding, NULL, 0, NULL, &listener, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsAcceptChannel( listener, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateChannelForListener( listener, NULL, 0, &channel, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsAcceptChannel( listener, channel, NULL, NULL );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );
    WsFreeChannel( channel );
    WsFreeListener( listener );

    hr = WsCreateChannel( info->type, info->binding, NULL, 0, NULL, &channel, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    memset( &addr, 0, sizeof(addr) );
    addr.url.length = wsprintfW( buf, fmt, info->port );
    addr.url.chars  = buf;
    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    body.elementLocalName = &localname;
    body.elementNs        = &ns;
    body.type             = WS_INT32_TYPE;
    body.typeDescription  = NULL;

    desc.action                 = &action;
    desc.bodyElementDescription = &body;

    hr = WsSendMessage( channel, msg, &desc, WS_WRITE_REQUIRED_VALUE, &val, sizeof(val), NULL, NULL );
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

START_TEST(listener)
{
    struct listener_info info;
    HANDLE thread;

    test_WsCreateListener();
    test_WsOpenListener();
    test_WsCreateChannelForListener();
    test_WsResetListener();

    info.port    = 7533;
    info.wait    = CreateEventW( NULL, 0, 0, NULL );
    info.type    = WS_CHANNEL_TYPE_DUPLEX;
    info.binding = WS_UDP_CHANNEL_BINDING;

    thread = start_listener( &info );
    test_WsAcceptChannel( &info );
    WaitForSingleObject( thread, 3000 );

    info.type    = WS_CHANNEL_TYPE_DUPLEX_SESSION;
    info.binding = WS_TCP_CHANNEL_BINDING;

    thread = start_listener( &info );
    test_WsAcceptChannel( &info );
    WaitForSingleObject( thread, 3000 );
}
