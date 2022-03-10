/*
 * Copyright 2016 Jacek Caban for CodeWeavers
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

#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include "initguid.h"
#include "wpcapi.h"

#include "wine/test.h"

static void test_wpc(void)
{
    IWindowsParentalControls *wpc;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_WindowsParentalControls, NULL, CLSCTX_INPROC_SERVER, &IID_IWindowsParentalControls, (void**)&wpc);
    if(hres == REGDB_E_CLASSNOTREG)
        win_skip("CLSID_WindowsParentalControls not registered\n");
    else
        ok(hres == S_OK, "Could not create CLSID_WindowsParentalControls instance: %08lx\n", hres);
    if(FAILED(hres))
        return;

    IWindowsParentalControls_Release(wpc);
}

START_TEST(wpc)
{
    CoInitialize(NULL);

    test_wpc();

    CoUninitialize();
}
