/*
 * Copyright (c) 2020 Alistair Leslie-Hughes
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

#include <windows.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#define COBJMACROS
#include "wine/test.h"

#include "x3daudio.h"

#include "initguid.h"
#include "xact3.h"

DEFINE_GUID(IID_IXACT3Engine30, 0x9e33f661, 0x2d07, 0x43ec, 0x97, 0x04, 0xbb, 0xcb, 0x71, 0xa5, 0x49, 0x72);
DEFINE_GUID(IID_IXACT3Engine31, 0xe72c1b9a, 0xd717, 0x41c0, 0x81, 0xa6, 0x50, 0xeb, 0x56, 0xe8, 0x06, 0x49);

DEFINE_GUID(CLSID_XACTEngine30, 0x3b80ee2a, 0xb0f5, 0x4780, 0x9e, 0x30, 0x90, 0xcb, 0x39, 0x68, 0x5b, 0x03);
DEFINE_GUID(CLSID_XACTEngine31, 0x962f5027, 0x99be, 0x4692, 0xa4, 0x68, 0x85, 0x80, 0x2c, 0xf8, 0xde, 0x61);
DEFINE_GUID(CLSID_XACTEngine32, 0xd3332f02, 0x3dd0, 0x4de9, 0x9a, 0xec, 0x20, 0xd8, 0x5c, 0x41, 0x11, 0xb6);
DEFINE_GUID(CLSID_XACTEngine33, 0x94c1affa, 0x66e7, 0x4961, 0x95, 0x21, 0xcf, 0xde, 0xf3, 0x12, 0x8d, 0x4f);
DEFINE_GUID(CLSID_XACTEngine34, 0x0977d092, 0x2d95, 0x4e43, 0x8d, 0x42, 0x9d, 0xdc, 0xc2, 0x54, 0x5e, 0xd5);
DEFINE_GUID(CLSID_XACTEngine35, 0x074b110f, 0x7f58, 0x4743, 0xae, 0xa5, 0x12, 0xf1, 0x5b, 0x50, 0x74, 0xed);
DEFINE_GUID(CLSID_XACTEngine36, 0x248d8a3b, 0x6256, 0x44d3, 0xa0, 0x18, 0x2a, 0xc9, 0x6c, 0x45, 0x9f, 0x47);

struct xact_interfaces
{
    REFGUID clsid;
    REFIID iid;
    HRESULT expected;
} xact_interfaces[] =
{
    {&CLSID_XACTEngine30, &IID_IXACT3Engine30, S_OK },
    {&CLSID_XACTEngine30, &IID_IXACT3Engine31, E_NOINTERFACE},
    {&CLSID_XACTEngine30, &IID_IXACT3Engine,   E_NOINTERFACE },

    /* Version 3.1 to 3.4 use the same interface */
    {&CLSID_XACTEngine31, &IID_IXACT3Engine31, S_OK },
    {&CLSID_XACTEngine32, &IID_IXACT3Engine31, S_OK },
    {&CLSID_XACTEngine33, &IID_IXACT3Engine31, S_OK },
    {&CLSID_XACTEngine34, &IID_IXACT3Engine31, S_OK },

    /* Version 3.5 to 3.7 use the same interface */
    {&CLSID_XACTEngine35, &IID_IXACT3Engine31, E_NOINTERFACE },
    {&CLSID_XACTEngine35, &IID_IXACT3Engine,   S_OK },

    {&CLSID_XACTEngine36, &IID_IXACT3Engine31, E_NOINTERFACE },
    {&CLSID_XACTEngine36, &IID_IXACT3Engine,   S_OK },

    {&CLSID_XACTEngine,   &IID_IXACT3Engine31, E_NOINTERFACE },
    {&CLSID_XACTEngine,   &IID_IXACT3Engine,   S_OK },
    {&CLSID_XACTEngine,   &IID_IUnknown,       S_OK },
};

static void test_interfaces(void)
{
    IUnknown *unk;
    HRESULT hr;
    int i;

    for (i = 0; i < ARRAY_SIZE(xact_interfaces); i++)
    {
        hr = CoCreateInstance(xact_interfaces[i].clsid, NULL, CLSCTX_INPROC_SERVER,
                    xact_interfaces[i].iid, (void**)&unk);
        if (hr == REGDB_E_CLASSNOTREG)
        {
            trace("%d %s not registered. Skipping\n", i, wine_dbgstr_guid(xact_interfaces[i].clsid) );
            continue;
        }
        ok(hr == xact_interfaces[i].expected, "%d, Unexpected value 0x%08lx\n", i, hr);
        if (hr == S_OK)
            IUnknown_Release(unk);
    }
}

START_TEST(xact3)
{
    CoInitialize(NULL);

    test_interfaces();

    CoUninitialize();
}
