/*
 *
 * Copyright 2019 Alistair Leslie-Hughes
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
 *
 */
#define COBJMACROS

#include <stdarg.h>

#include "windows.h"
#include "directmanipulation.h"

#include "wine/test.h"

static void test_IDirectManipulationManager2(void)
{
    IDirectManipulationManager2 *manager2;
    IDirectManipulationUpdateManager *update;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_DirectManipulationManager, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDirectManipulationManager2, (void**)&manager2);
    if(FAILED(hres))
    {
        win_skip("Failed to create XMLView instance\n");
        return;
    }
    ok(hres == S_OK, "CoCreateInstance returned %x, expected S_OK\n", hres);

    hres = IDirectManipulationManager2_GetUpdateManager(manager2, &IID_IDirectManipulationUpdateManager, (void**)&update);
    ok(hres == S_OK, "returned %x, expected S_OK\n", hres);

    if(update)
        IDirectManipulationUpdateManager_Release(update);

    IDirectManipulationManager2_Release(manager2);
}

START_TEST(manipulation)
{
    CoInitialize(NULL);

    test_IDirectManipulationManager2();

    CoUninitialize();
}
