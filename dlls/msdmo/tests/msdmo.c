/*
 *    MSDMO tests
 *
 * Copyright 2014 Nikolay Sivov for CodeWeavers
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

#include "initguid.h"
#include "dmo.h"
#include "wine/test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
static const GUID GUID_unknowndmo = {0x14d99047,0x441f,0x4cd3,{0xbc,0xa8,0x3e,0x67,0x99,0xaf,0x34,0x75}};
static const GUID GUID_unknowncategory = {0x14d99048,0x441f,0x4cd3,{0xbc,0xa8,0x3e,0x67,0x99,0xaf,0x34,0x75}};
static const GUID GUID_wmp1 = {0x13a7995e,0x7d8f,0x45b4,{0x9c,0x77,0x81,0x92,0x65,0x22,0x57,0x63}};

static void test_DMOUnregister(void)
{
    static const WCHAR testdmoW[] = {'t','e','s','t','d','m','o',0};
    HRESULT hr;

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_unknowncategory);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_NULL);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);

    /* can't register for all categories */
    hr = DMORegister(testdmoW, &GUID_unknowndmo, &GUID_NULL, 0, 0, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = DMORegister(testdmoW, &GUID_unknowndmo, &GUID_unknowncategory, 0, 0, NULL, 0, NULL);
    if (hr != S_OK) {
        win_skip("Failed to register DMO. Probably user doesn't have persmissions to do so.\n");
        return;
    }

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_NULL);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
}

static void test_DMOGetName(void)
{
    WCHAR name[80];
    HRESULT hr;

    hr = DMOGetName(&GUID_unknowndmo, NULL);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);

    /* no such DMO */
    name[0] = 'a';
    hr = DMOGetName(&GUID_wmp1, name);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(name[0] == 'a', "got %x\n", name[0]);
}

START_TEST(msdmo)
{
    test_DMOUnregister();
    test_DMOGetName();
}
