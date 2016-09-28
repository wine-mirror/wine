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
#include "winsock2.h"
#include "webservices.h"
#include "wine/test.h"

static void test_WsCreateServiceProxy(void)
{
    HRESULT hr;
    WS_SERVICE_PROXY *proxy;
    WS_SERVICE_PROXY_STATE state;
    ULONG size, value;

    hr = WsCreateServiceProxy( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, NULL,
                               0, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    proxy = NULL;
    hr = WsCreateServiceProxy( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, NULL,
                               0, NULL, 0, &proxy, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( proxy != NULL, "proxy not set\n" );

    /* write-only property */
    value = 0xdeadbeef;
    size = sizeof(value);
    hr = WsGetServiceProxyProperty( proxy, WS_PROXY_PROPERTY_CALL_TIMEOUT, &value, size, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    state = 0xdeadbeef;
    size = sizeof(state);
    hr = WsGetServiceProxyProperty( proxy, WS_PROXY_PROPERTY_STATE, &state, size, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == WS_SERVICE_PROXY_STATE_CREATED, "got %u\n", state );

    WsFreeServiceProxy( proxy );
}

static void test_WsCreateServiceProxyFromTemplate(void)
{
    HRESULT hr;
    WS_SERVICE_PROXY *proxy;
    WS_HTTP_POLICY_DESCRIPTION policy;

    hr = WsCreateServiceProxyFromTemplate( WS_CHANNEL_TYPE_REQUEST, NULL, 0, WS_HTTP_BINDING_TEMPLATE_TYPE,
                                           NULL, 0, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateServiceProxyFromTemplate( WS_CHANNEL_TYPE_REQUEST, NULL, 0, WS_HTTP_BINDING_TEMPLATE_TYPE,
                                           NULL, 0, NULL, 0, &proxy, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    memset( &policy, 0, sizeof(policy) );
    proxy = NULL;
    hr = WsCreateServiceProxyFromTemplate( WS_CHANNEL_TYPE_REQUEST, NULL, 0, WS_HTTP_BINDING_TEMPLATE_TYPE,
                                           NULL, 0, &policy, sizeof(policy), &proxy, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( proxy != NULL, "proxy not set\n" );

    WsFreeServiceProxy( proxy );
}

static void test_WsOpenServiceProxy(void)
{
    WCHAR url[] = {'h','t','t','p',':','/','/','l','o','c','a','l','h','o','s','t','/'};
    HRESULT hr;
    WS_SERVICE_PROXY *proxy;
    WS_HTTP_POLICY_DESCRIPTION policy;
    WS_ENDPOINT_ADDRESS addr;

    memset( &policy, 0, sizeof(policy) );
    hr = WsCreateServiceProxyFromTemplate( WS_CHANNEL_TYPE_REQUEST, NULL, 0, WS_HTTP_BINDING_TEMPLATE_TYPE,
                                           NULL, 0, &policy, sizeof(policy), &proxy, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    addr.url.length = sizeof(url)/sizeof(url[0]);
    addr.url.chars  = url;
    addr.headers    = NULL;
    addr.extensions = NULL;
    addr.identity   = NULL;
    hr = WsOpenServiceProxy( proxy, &addr, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCloseServiceProxy( proxy , NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    WsFreeServiceProxy( proxy );
}

static HRESULT create_channel( int port, WS_CHANNEL **ret )
{
    static const WCHAR fmt[] =
        {'h','t','t','p',':','/','/','1','2','7','.','0','.','0','.','1',':','%','u',0};
    WS_CHANNEL_PROPERTY prop[2];
    WS_ENVELOPE_VERSION env_version = WS_ENVELOPE_VERSION_SOAP_1_1;
    WS_ADDRESSING_VERSION addr_version = WS_ADDRESSING_VERSION_TRANSPORT;
    WS_CHANNEL *channel;
    WS_ENDPOINT_ADDRESS addr;
    WCHAR buf[64];
    HRESULT hr;

    prop[0].id        = WS_CHANNEL_PROPERTY_ENVELOPE_VERSION;
    prop[0].value     = &env_version;
    prop[0].valueSize = sizeof(env_version);

    prop[1].id        = WS_CHANNEL_PROPERTY_ADDRESSING_VERSION;
    prop[1].value     = &addr_version;
    prop[1].valueSize = sizeof(addr_version);

    *ret = NULL;
    hr = WsCreateChannel( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, prop, 2, NULL, &channel, NULL );
    if (hr != S_OK) return hr;

    addr.url.length = wsprintfW( buf, fmt, port );
    addr.url.chars  = buf;
    addr.headers    = NULL;
    addr.extensions = NULL;
    addr.identity   = NULL;
    hr = WsOpenChannel( channel, &addr, NULL, NULL );
    if (hr == S_OK) *ret = channel;
    else WsFreeChannel( channel );
    return hr;
}

static const char req_test1[] =
    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
    "<req_test1 xmlns=\"ns\">-1</req_test1>"
    "</s:Body></s:Envelope>";

static const char resp_test1[] =
    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
    "<resp_test1 xmlns=\"ns\">-2</resp_test1>"
    "</s:Body></s:Envelope>";

static void test_WsSendMessage( int port, WS_XML_STRING *action )
{
    WS_XML_STRING req = {9, (BYTE *)"req_test1"}, ns = {2, (BYTE *)"ns"};
    WS_CHANNEL *channel;
    WS_MESSAGE *msg;
    WS_ELEMENT_DESCRIPTION body;
    WS_MESSAGE_DESCRIPTION desc;
    INT32 val = -1;
    HRESULT hr;

    hr = create_channel( port, &channel );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    body.elementLocalName = &req;
    body.elementNs        = &ns;
    body.type             = WS_INT32_TYPE;
    body.typeDescription  = NULL;
    desc.action                 = action;
    desc.bodyElementDescription = &body;
    hr = WsSendMessage( NULL, msg, &desc, WS_WRITE_REQUIRED_VALUE, &val, sizeof(val), NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsSendMessage( channel, NULL, &desc, WS_WRITE_REQUIRED_VALUE, &val, sizeof(val), NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsSendMessage( channel, msg, NULL, WS_WRITE_REQUIRED_VALUE, &val, sizeof(val), NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsSendMessage( channel, msg, &desc, WS_WRITE_REQUIRED_VALUE, &val, sizeof(val), NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    WsFreeChannel( channel );
    WsFreeMessage( msg );
}

static void test_WsReceiveMessage( int port )
{
    WS_XML_STRING req = {9, (BYTE *)"req_test1"}, resp = {10, (BYTE *)"resp_test1"}, ns = {2, (BYTE *)"ns"};
    WS_CHANNEL *channel;
    WS_MESSAGE *msg;
    WS_ELEMENT_DESCRIPTION body;
    WS_MESSAGE_DESCRIPTION desc_req, desc_resp;
    const WS_MESSAGE_DESCRIPTION *desc[1];
    INT32 val = -1;
    HRESULT hr;

    hr = create_channel( port, &channel );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    body.elementLocalName = &req;
    body.elementNs        = &ns;
    body.type             = WS_INT32_TYPE;
    body.typeDescription  = NULL;
    desc_req.action                 = &req;
    desc_req.bodyElementDescription = &body;
    hr = WsSendMessage( channel, msg, &desc_req, WS_WRITE_REQUIRED_VALUE, &val, sizeof(val), NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    WsFreeMessage( msg );

    hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    body.elementLocalName = &resp;
    desc_resp.action                 = &resp;
    desc_resp.bodyElementDescription = &body;
    desc[0] = &desc_resp;
    hr = WsReceiveMessage( NULL, msg, desc, 1, WS_RECEIVE_REQUIRED_MESSAGE, WS_READ_REQUIRED_VALUE,
                           NULL, &val, sizeof(val), NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsReceiveMessage( channel, NULL, desc, 1, WS_RECEIVE_REQUIRED_MESSAGE, WS_READ_REQUIRED_VALUE,
                           NULL, &val, sizeof(val), NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsReceiveMessage( channel, msg, NULL, 1, WS_RECEIVE_REQUIRED_MESSAGE, WS_READ_REQUIRED_VALUE,
                           NULL, &val, sizeof(val), NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsReceiveMessage( channel, msg, desc, 1, WS_RECEIVE_REQUIRED_MESSAGE, WS_READ_REQUIRED_VALUE,
                           NULL, &val, sizeof(val), NULL, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( val == -2, "got %d\n", val );

    hr = WsCloseChannel( channel, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    WsFreeChannel( channel );
    WsFreeMessage( msg );
}

static const struct
{
    const char   *req_action;
    const char   *req_data;
    unsigned int  req_len;
    const char   *resp_action;
    const char   *resp_data;
    unsigned int  resp_len;
}
tests[] =
{
    { "req_test1", req_test1, sizeof(req_test1)-1, "resp_test1", resp_test1, sizeof(resp_test1)-1 },
};

static void send_response( int c, const char *action, const char *data, unsigned int len )
{
    static const char headers[] =
        "HTTP/1.1 200 OK\r\nContent-Type: text/xml; charset=utf-8\r\nConnection: close\r\n";
    static const char fmt[] =
        "SOAPAction: \"%s\"\r\nContent-Length: %u\r\n\r\n";
    char buf[128];

    send( c, headers, sizeof(headers) - 1, 0 );
    sprintf( buf, fmt, action, len );
    send( c, buf, strlen(buf), 0 );
    send( c, data, len, 0 );
}

struct server_info
{
    HANDLE event;
    int    port;
};

static DWORD CALLBACK server_proc( void *arg )
{
    struct server_info *info = arg;
    int len, res, c = -1, i, j, on = 1, quit;
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in sa;
    char buf[1024];
    const char *p;

    WSAStartup( MAKEWORD(1,1), &wsa );
    if ((s = socket( AF_INET, SOCK_STREAM, 0 )) == INVALID_SOCKET) return 1;
    setsockopt( s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on) );

    memset( &sa, 0, sizeof(sa) );
    sa.sin_family           = AF_INET;
    sa.sin_port             = htons( info->port );
    sa.sin_addr.S_un.S_addr = inet_addr( "127.0.0.1" );
    if (bind( s, (struct sockaddr *)&sa, sizeof(sa) ) < 0) return 1;

    listen( s, 0 );
    SetEvent( info->event );
    for (;;)
    {
        c = accept( s, NULL, NULL );

        buf[0] = 0;
        for (i = 0; i < sizeof(buf) - 1; i++)
        {
            if ((res = recv( c, &buf[i], 1, 0 )) != 1) break;
            if (i < 4) continue;
            if (buf[i - 2] == '\n' && buf[i] == '\n' && buf[i - 3] == '\r' && buf[i - 1] == '\r')
                break;
        }
        buf[i] = 0;
        quit = strstr( buf, "SOAPAction: \"quit\"" ) != NULL;

        len = 0;
        if ((p = strstr( buf, "Content-Length: " )))
        {
            p += strlen( "Content-Length: " );
            while (isdigit( *p ))
            {
                len *= 10;
                len += *p++ - '0';
            }
        }
        for (i = 0; i < len; i++)
        {
            if ((res = recv( c, &buf[i], 1, 0 )) != 1) break;
        }
        buf[i] = 0;

        for (j = 0; j < sizeof(tests)/sizeof(tests[0]); j++)
        {
            if (strstr( buf, tests[j].req_action ))
            {
                if (tests[j].req_data)
                {
                    int data_len = strlen( buf );
                    ok( tests[j].req_len == data_len, "%u: unexpected data length %u %u\n",
                        j, data_len, tests[j].req_len );
                    if (tests[j].req_len == data_len)
                        ok( !memcmp( tests[j].req_data, buf, tests[j].req_len ), "%u: unexpected data\n", j );
                }
                send_response( c, tests[j].resp_action, tests[j].resp_data, tests[j].resp_len );
            }
        }

        shutdown( c, 2 );
        closesocket( c );
        if (quit) break;
    }

    return 0;
}

START_TEST(proxy)
{
    WS_XML_STRING test1 = {9, (BYTE *)"req_test1"};
    WS_XML_STRING quit = {4, (BYTE *)"quit"};
    struct server_info info;
    HANDLE thread;
    DWORD ret;

    test_WsCreateServiceProxy();
    test_WsCreateServiceProxyFromTemplate();
    test_WsOpenServiceProxy();

    info.port  = 7533;
    info.event = CreateEventW( NULL, 0, 0, NULL );
    thread = CreateThread( NULL, 0, server_proc, &info, 0, NULL );
    ok( thread != NULL, "failed to create server thread %u\n", GetLastError() );

    ret = WaitForSingleObject( info.event, 3000 );
    ok(ret == WAIT_OBJECT_0, "failed to start test server %u\n", GetLastError());
    if (ret != WAIT_OBJECT_0) return;

    test_WsSendMessage( info.port, &test1 );
    test_WsReceiveMessage( info.port );

    test_WsSendMessage( info.port, &quit );
    WaitForSingleObject( thread, 3000 );
}
