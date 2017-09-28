/*
 * Speech API (SAPI) token tests.
 *
 * Copyright (C) 2017 Huw Davies
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

#define COBJMACROS

#include "initguid.h"
#include "sapiddk.h"
#include "sperror.h"

#include "wine/test.h"

static void test_data_key(void)
{
    ISpRegDataKey *data_key;
    HRESULT hr;
    HKEY key;
    LONG res;

    hr = CoCreateInstance( &CLSID_SpDataKey, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpRegDataKey, (void **)&data_key );
    ok( hr == S_OK, "got %08x\n", hr );

    res = RegCreateKeyExA( HKEY_CURRENT_USER, "Software\\Winetest\\sapi", 0, NULL, 0, KEY_ALL_ACCESS,
                           NULL, &key, NULL );
    ok( res == ERROR_SUCCESS, "got %d\n", res );

    hr = ISpRegDataKey_SetKey( data_key, key, FALSE );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = ISpRegDataKey_SetKey( data_key, key, FALSE );
    ok( hr == SPERR_ALREADY_INITIALIZED, "got %08x\n", hr );

    ISpRegDataKey_Release( data_key );
}

START_TEST(token)
{
    CoInitialize( NULL );
    test_data_key();
    CoUninitialize();
}
