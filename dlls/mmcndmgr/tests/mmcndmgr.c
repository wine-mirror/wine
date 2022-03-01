/*
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
#include <stdio.h>

#include "windows.h"
#include "ole2.h"
#include "dispex.h"

#include "wine/test.h"

#include <initguid.h>
#include <mmc.h>

static void test_get_version(void)
{
    IMMCVersionInfo * ver_info = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MMCVersionInfo, NULL, CLSCTX_INPROC_SERVER, &IID_IMMCVersionInfo, (void**)&ver_info);
    if (hr != S_OK)
    {
        trace("MMCVersionInfo interface not registered.\n");
    }
    else
    {
        LONG lMajor, lMinor;

        hr = IMMCVersionInfo_GetMMCVersion(ver_info, &lMajor, &lMinor);
        ok(hr == S_OK, "got 0x%08lx, expected 0x%08lx\n", hr, S_OK);
        if (hr == S_OK)
            trace("MMC Version is %ld.%ld\n", lMajor, lMinor);
    }

    if (ver_info)
    {
        IMMCVersionInfo_Release(ver_info);
    }
}


START_TEST(mmcndmgr)
{
    CoInitialize(NULL);

    test_get_version();

    CoUninitialize();
}
