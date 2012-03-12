/*
 * Copyright 2012 Piotr Caban for CodeWeavers
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
#include "mshtml.h"
#include "initguid.h"
#include "perhist.h"
#include "docobj.h"

#include "wine/test.h"

DEFINE_GUID(CLSID_XMLView, 0x48123bc4, 0x99d9, 0x11d1, 0xa6,0xb3, 0x00,0xc0,0x4f,0xd9,0x15,0x55);

static void test_QueryInterface(void)
{
    IUnknown *xmlview, *unk;
    IHTMLDocument *htmldoc;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_XMLView, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&xmlview);
    if(hres == REGDB_E_CLASSNOTREG) {
        win_skip("XMLView class not registered\n");
        return;
    }
    ok(hres == S_OK, "CoCreateInstance returned %x, expected S_OK\n", hres);

    hres = IUnknown_QueryInterface(xmlview, &IID_IPersistMoniker, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IPersistMoniker) returned %x, expected S_OK\n", hres);
    IUnknown_Release(unk);

    hres = IUnknown_QueryInterface(xmlview, &IID_IPersistHistory, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IPersistHistory) returned %x, expected S_OK\n", hres);
    IUnknown_Release(unk);

    hres = IUnknown_QueryInterface(xmlview, &IID_IOleCommandTarget, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IOleCommandTarget) returned %x, expected S_OK\n", hres);
    IUnknown_Release(unk);

    hres = IUnknown_QueryInterface(xmlview, &IID_IOleObject, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) returned %x, expected S_OK\n", hres);
    IUnknown_Release(unk);

    hres = IUnknown_QueryInterface(xmlview, &IID_IHTMLDocument, (void**)&htmldoc);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLDocument) returned %x, expected S_OK\n", hres);
    hres = IHTMLDocument_QueryInterface(htmldoc, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IUnknown) returned %x, expected S_OK\n", hres);
    todo_wine ok(unk == xmlview, "Aggregation is not working as expected\n");
    IUnknown_Release(unk);
    IHTMLDocument_Release(htmldoc);

    IUnknown_Release(xmlview);
}

START_TEST(xmlview)
{
    CoInitialize(NULL);
    test_QueryInterface();
    CoUninitialize();
}
