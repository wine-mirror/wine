/*
 * Copyright 2015 Hans Leidekker for CodeWeavers
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

static void test_WsCreateError(void)
{
    HRESULT hr;
    WS_ERROR *error;
    WS_ERROR_PROPERTY prop;
    ULONG size, code, count;
    LANGID langid;

    hr = WsCreateError( NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    error = NULL;
    hr = WsCreateError( NULL, 0, &error );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( error != NULL, "error not set\n" );

    count = 0xdeadbeef;
    size = sizeof(count);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_STRING_COUNT, &count, size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !count, "got %u\n", count );

    hr = WsSetErrorProperty( error, WS_ERROR_PROPERTY_STRING_COUNT, &count, size );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    code = 0xdeadbeef;
    size = sizeof(code);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !code, "got %u\n", code );

    code = 0xdeadbeef;
    hr = WsSetErrorProperty( error, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, size );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( code == 0xdeadbeef, "got %u\n", code );

    langid = 0xdead;
    size = sizeof(langid);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_LANGID, &langid, size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( langid == GetUserDefaultUILanguage(), "got %u\n", langid );

    langid = MAKELANGID( LANG_DUTCH, SUBLANG_DEFAULT );
    hr = WsSetErrorProperty( error, WS_ERROR_PROPERTY_LANGID, &langid, size );
    ok( hr == WS_E_INVALID_OPERATION, "got %08x\n", hr );

    count = 0xdeadbeef;
    size = sizeof(count);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_LANGID + 1, &count, size );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
    ok( count == 0xdeadbeef, "got %u\n", count );
    WsFreeError( error );

    count = 1;
    prop.id = WS_ERROR_PROPERTY_STRING_COUNT;
    prop.value = &count;
    prop.valueSize = sizeof(count);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    code = 0xdeadbeef;
    prop.id = WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE;
    prop.value = &code;
    prop.valueSize = sizeof(code);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    langid = MAKELANGID( LANG_DUTCH, SUBLANG_DEFAULT );
    prop.id = WS_ERROR_PROPERTY_LANGID;
    prop.value = &langid;
    prop.valueSize = sizeof(langid);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == S_OK, "got %08x\n", hr );

    langid = 0xdead;
    size = sizeof(langid);
    hr = WsGetErrorProperty( error, WS_ERROR_PROPERTY_LANGID, &langid, size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( langid == MAKELANGID( LANG_DUTCH, SUBLANG_DEFAULT ), "got %u\n", langid );
    WsFreeError( error );

    count = 0xdeadbeef;
    prop.id = WS_ERROR_PROPERTY_LANGID + 1;
    prop.value = &count;
    prop.valueSize = sizeof(count);
    hr = WsCreateError( &prop, 1, &error );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
}

START_TEST(reader)
{
    test_WsCreateError();
}
