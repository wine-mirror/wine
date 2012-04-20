/*
 *
 * Copyright 2012 Alistair Leslie-Hughes
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

#include "initguid.h"
#include "scrrun.h"

static void test_interfaces(void)
{
    HRESULT hr;
    IDispatch *disp;
    IDispatchEx *dispex;
    IFileSystem3 *fs3;
    IObjectWithSite *site;

    hr = CoCreateInstance(&CLSID_FileSystemObject, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDispatch, (void**)&disp);
    if(FAILED(hr)) {
        win_skip("Could not create FileSystem object: %08x\n", hr);
        return;
    }

    hr = IDispatch_QueryInterface(disp, &IID_IFileSystem3, (void**)&fs3);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    IFileSystem3_Release(fs3);

    hr = IDispatch_QueryInterface(disp, &IID_IObjectWithSite, (void**)&site);
    ok(hr == E_NOINTERFACE, "got 0x%08x, expected 0x%08x\n", hr, E_NOINTERFACE);

    hr = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == E_NOINTERFACE, "got 0x%08x, expected 0x%08x\n", hr, E_NOINTERFACE);

    IDispatch_Release(disp);
}

START_TEST(filesystem)
{
    CoInitialize(NULL);

    test_interfaces();


    CoUninitialize();
}
