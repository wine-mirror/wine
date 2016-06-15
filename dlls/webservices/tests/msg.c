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

static void test_WsCreateMessage(void)
{
    HRESULT hr;
    WS_MESSAGE *msg;
    WS_ENVELOPE_VERSION env_version;
    WS_ADDRESSING_VERSION addr_version;
    WS_MESSAGE_PROPERTY prop;

    hr = WsCreateMessage( 0, 0, NULL, 0, NULL, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateMessage( 0, 0, NULL, 0, &msg, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateMessage( WS_ADDRESSING_VERSION_0_9, 0, NULL, 0, &msg, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateMessage( 0, WS_ENVELOPE_VERSION_SOAP_1_1, NULL, 0, &msg, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    env_version = WS_ENVELOPE_VERSION_SOAP_1_1;
    prop.id = WS_MESSAGE_PROPERTY_ENVELOPE_VERSION;
    prop.value = &env_version;
    prop.valueSize = sizeof(env_version);
    hr = WsCreateMessage( WS_ADDRESSING_VERSION_0_9, WS_ENVELOPE_VERSION_SOAP_1_1, &prop,
                          1, &msg, NULL ) ;
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = WsCreateMessage( WS_ADDRESSING_VERSION_0_9, WS_ENVELOPE_VERSION_SOAP_1_1, NULL,
                          0, &msg, NULL ) ;
    ok( hr == S_OK, "got %08x\n", hr );

    env_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ENVELOPE_VERSION, &env_version,
                               sizeof(env_version), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( env_version == WS_ENVELOPE_VERSION_SOAP_1_1, "got %u\n", env_version );

    addr_version = 0xdeadbeef;
    hr = WsGetMessageProperty( msg, WS_MESSAGE_PROPERTY_ADDRESSING_VERSION, &addr_version,
                               sizeof(addr_version), NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( addr_version == WS_ADDRESSING_VERSION_0_9, "got %u\n", addr_version );

    WsFreeMessage( msg );
}

START_TEST(msg)
{
    test_WsCreateMessage();
}
