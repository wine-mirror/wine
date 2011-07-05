/*
 * Copyright (c) 2011 Andrew Nguyen
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

#define DIRECTINPUT_VERSION 0x0700

#define COBJMACROS
#include <windows.h>
#include <dinput.h>

#include "wine/test.h"

HINSTANCE hInstance;

static void test_RunControlPanel(void)
{
    IDirectInputA *pDI;
    HRESULT hr;

    hr = DirectInputCreateA(hInstance, DIRECTINPUT_VERSION, &pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    if (winetest_interactive)
    {
        hr = IDirectInput_RunControlPanel(pDI, NULL, 0);
        ok(hr == S_OK, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);

        hr = IDirectInput_RunControlPanel(pDI, GetDesktopWindow(), 0);
        ok(hr == S_OK, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);
    }

    hr = IDirectInput_RunControlPanel(pDI, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput_RunControlPanel(pDI, (HWND)0xdeadbeef, 0);
    ok(hr == E_HANDLE, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput_RunControlPanel(pDI, (HWND)0xdeadbeef, ~0u);
    ok(hr == E_HANDLE, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);

    IDirectInput_Release(pDI);
}

START_TEST(dinput)
{
    hInstance = GetModuleHandleA(NULL);

    CoInitialize(NULL);
    test_RunControlPanel();
    CoUninitialize();
}
