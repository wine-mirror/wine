/*
 * XMLLite IXmlReader tests
 *
 * Copyright 2010 (C) Nikolay Sivov
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "xmllite.h"
#include "initguid.h"
#include "wine/test.h"

DEFINE_GUID(IID_IXmlReader, 0x7279fc81, 0x709d, 0x4095, 0xb6, 0x3d, 0x69,
                            0xfe, 0x4b, 0x0d, 0x90, 0x30);

HRESULT WINAPI (*pCreateXmlReader)(REFIID riid, void **ppvObject, IMalloc *pMalloc);

static BOOL init_pointers(void)
{
    /* don't free module here, it's to be unloaded on exit */
    HMODULE mod = LoadLibraryA("xmllite.dll");

    if (!mod)
    {
        win_skip("xmllite library not available\n");
        return FALSE;
    }

    pCreateXmlReader = (void*)GetProcAddress(mod, "CreateXmlReader");
    if (!pCreateXmlReader) return FALSE;

    return TRUE;
}

static void test_reader_create(void)
{
    HRESULT hr;
    IXmlReader *reader;
    IMalloc *imalloc;

    /* crashes native */
    if (0)
    {
        hr = pCreateXmlReader(&IID_IXmlReader, NULL, NULL);
        hr = pCreateXmlReader(NULL, (LPVOID*)&reader, NULL);
    }

    hr = pCreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader, NULL);
    todo_wine ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    if (hr != S_OK)
    {
        skip("Failed to create IXmlReader instance\n");
        return;
    }

    hr = CoGetMalloc(1, &imalloc);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IMalloc_DidAlloc(imalloc, reader);
    ok(hr != 1, "Expected 0 or -1, got %08x\n", hr);

    IXmlReader_Release(reader);
}

START_TEST(reader)
{
    HRESULT r;

    r = CoInitialize( NULL );
    ok( r == S_OK, "failed to init com\n");

    if (!init_pointers())
    {
       CoUninitialize();
       return;
    }

    test_reader_create();

    CoUninitialize();
}
