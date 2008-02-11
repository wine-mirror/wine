/*
 * Unit test suite for bits functions
 *
 * Copyright 2007 Google (Roy Shea)
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

#include "wine/test.h"
#include "initguid.h"
#include "bits.h"

static void
test_CreateInstance(void)
{
    HRESULT hres;
    ULONG res;
    IBackgroundCopyManager *manager = NULL;

    /* Creating BITS instance */
    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL, CLSCTX_LOCAL_SERVER,
                            &IID_IBackgroundCopyManager, (void **) &manager);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);
    if(hres != S_OK) {
        skip("Unable to create bits instance.\n");
        return;
    }

    /* Releasing bits manager */
    res = IBackgroundCopyManager_Release(manager);
    ok(res == 0, "Bad ref count on release: %u\n", res);

}

START_TEST(qmgr)
{
    CoInitialize(NULL);
    test_CreateInstance();
    CoUninitialize();
}
