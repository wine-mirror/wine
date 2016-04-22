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

static void test_WsCreateServiceProxy(void)
{
    HRESULT hr;
    WS_SERVICE_PROXY *proxy;
    WS_SERVICE_PROXY_STATE state;
    ULONG size, value;

    hr = WsCreateServiceProxy( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, NULL,
                               0, NULL, 0, NULL, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    proxy = NULL;
    hr = WsCreateServiceProxy( WS_CHANNEL_TYPE_REQUEST, WS_HTTP_CHANNEL_BINDING, NULL, NULL,
                               0, NULL, 0, &proxy, NULL ) ;
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

START_TEST(proxy)
{
    test_WsCreateServiceProxy();
    test_WsCreateServiceProxyFromTemplate();
    test_WsOpenServiceProxy();
}
