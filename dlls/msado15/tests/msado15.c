/*
 * Copyright 2019 Hans Leidekker for CodeWeavers
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
#include <initguid.h>
#include <oledb.h>
#include <msado15_backcompat.h>
#include "wine/test.h"

static void test_Stream(void)
{
    _Stream *stream;
    StreamTypeEnum type;
    LONG refs;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_Stream, NULL, CLSCTX_INPROC_SERVER, &IID__Stream, (void **)&stream );
    ok( hr == S_OK, "got %08x\n", hr );

    /* check default type */
    type = 0;
    hr = _Stream_get_Type( stream, &type );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( type == adTypeText, "got %u\n", type );

    hr = _Stream_put_Type( stream, adTypeBinary );
    ok( hr == S_OK, "got %08x\n", hr );

    type = 0;
    hr = _Stream_get_Type( stream, &type );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( type == adTypeBinary, "got %u\n", type );

    /* revert */
    hr = _Stream_put_Type( stream, adTypeText );
    ok( hr == S_OK, "got %08x\n", hr );

    refs = _Stream_Release( stream );
    ok( !refs, "got %d\n", refs );
}

START_TEST(msado15)
{
    CoInitialize( NULL );
    test_Stream();
    CoUninitialize();
}
