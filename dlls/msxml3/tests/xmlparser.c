/*
 * XML Parser implementation
 *
 * Copyright 2011 Alistair Leslie-Hughes
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
#define CONST_VTABLE

#include <stdio.h>
#include <assert.h>

#include "windows.h"
#include "ole2.h"
#include "initguid.h"

#include "wine/test.h"

DEFINE_GUID(IID_IXMLParser, 0xd242361e, 0x51a0, 0x11d2, 0x9c,0xaf, 0x00,0x60,0xb0,0xec,0x3d,0x39);
DEFINE_GUID(CLSID_XMLParser30, 0xf5078f31, 0xc551, 0x11d3, 0x89,0xb9, 0x00,0x00,0xf8,0x1f,0xe2,0x21);

static void create_test(void)
{
    HRESULT hr;
    IUnknown *parser;

    hr = CoCreateInstance(&CLSID_XMLParser30, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLParser, (void**)&parser);
    if (FAILED(hr))
    {
        win_skip("IXMLParser is not available (0x%08x)\n", hr);
        return;
    }

    IUnknown_Release(parser);
}

START_TEST(xmlparser)
{
    HRESULT hr;

    hr = CoInitialize( NULL );
    ok( hr == S_OK, "failed to init com\n");
    if (hr != S_OK)
        return;

    create_test();

    CoUninitialize();
}
