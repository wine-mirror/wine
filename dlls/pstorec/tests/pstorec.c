/*
 * Protected Storage Tests
 *
 * Copyright 2017 Nikolay Sivov
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

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "initguid.h"
#include "ole2.h"
#include "pstore.h"

#include "wine/test.h"

static HRESULT (WINAPI *pPStoreCreateInstance)(IPStore **store, PST_PROVIDERID *id, void *reserved, DWORD flags);

static void test_PStoreCreateInstance(void)
{
    IPStore *store;
    IUnknown *unk;
    HRESULT hr;

    if (!pPStoreCreateInstance)
    {
        win_skip("PStoreCreateInstance is not available\n");
        return;
    }

    hr = pPStoreCreateInstance(&store, NULL, NULL, 0);
    if (hr == E_NOTIMPL)
    {
        /* this function does not work starting with Win8 */
        win_skip("PStoreCreateInstance is not implemented on this system\n");
        return;
    }
    ok(hr == S_OK, "Unexpected return value %#lx.\n", hr);

    hr = IPStore_QueryInterface(store, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected return value %#lx.\n", hr);
    IUnknown_Release(unk);

    hr = IPStore_QueryInterface(store, &IID_IPStore, (void **)&unk);
    ok(hr == S_OK, "Unexpected return value %#lx.\n", hr);
    IUnknown_Release(unk);

    IPStore_Release(store);
}

START_TEST(pstorec)
{
    HMODULE module = LoadLibraryA("pstorec");

    pPStoreCreateInstance = (void *)GetProcAddress(module, "PStoreCreateInstance");

    test_PStoreCreateInstance();
}
