/* Unit test suite for various shell Association objects
 *
 * Copyright 2012 Detlef Riekenberg
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

#include "shlwapi.h"
#include "shlguid.h"

#include "wine/test.h"


static void test_IQueryAssociations_QueryInterface(void)
{
    IQueryAssociations *qa;
    IQueryAssociations *qa2;
    IUnknown *unk;
    HRESULT hr;

    /* this works since XP */
    hr = CoCreateInstance(&CLSID_QueryAssociations, NULL, CLSCTX_INPROC_SERVER, &IID_IQueryAssociations, (void*)&qa);

    if (FAILED(hr)) {
        win_skip("CoCreateInstance for IQueryAssociations returned 0x%x\n", hr);
        return;
    }

    hr = IUnknown_QueryInterface(qa, &IID_IQueryAssociations, (void**)&qa2);
    ok(hr == S_OK, "QueryInterface (IQueryAssociations) returned 0x%x\n", hr);
    if (SUCCEEDED(hr)) {
        IUnknown_Release(qa2);
    }

    hr = IUnknown_QueryInterface(qa, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface (IUnknown) returned 0x%x\n", hr);
    if (SUCCEEDED(hr)) {
        IUnknown_Release(unk);
    }

    IQueryAssociations_Release(qa);
}


START_TEST(assoc)
{
    CoInitialize(NULL);

    test_IQueryAssociations_QueryInterface();

    CoUninitialize();
}
