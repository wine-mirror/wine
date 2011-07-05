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
#include <initguid.h>
#include <windows.h>
#include <dinput.h>

#include "wine/test.h"

HINSTANCE hInstance;

static void test_QueryInterface(void)
{
    static const REFIID iid_list[] = {&IID_IUnknown, &IID_IDirectInputA, &IID_IDirectInputW,
                                      &IID_IDirectInput2A, &IID_IDirectInput2W,
                                      &IID_IDirectInput7A, &IID_IDirectInput7W};

    static const struct
    {
        REFIID riid;
        int test_todo;
    } no_interface_list[] =
    {
        {&IID_IDirectInput8A, 1},
        {&IID_IDirectInput8W, 1},
        {&IID_IDirectInputDeviceA},
        {&IID_IDirectInputDeviceW},
        {&IID_IDirectInputDevice2A},
        {&IID_IDirectInputDevice2W},
        {&IID_IDirectInputDevice7A},
        {&IID_IDirectInputDevice7W},
        {&IID_IDirectInputDevice8A},
        {&IID_IDirectInputDevice8W},
        {&IID_IDirectInputEffect},
    };

    IDirectInputA *pDI;
    HRESULT hr;
    IUnknown *pUnk;
    int i;

    hr = DirectInputCreateA(hInstance, DIRECTINPUT_VERSION, &pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput_QueryInterface(pDI, NULL, NULL);
    ok(hr == E_POINTER, "IDirectInput_QueryInterface returned 0x%08x\n", hr);

    pUnk = (void *)0xdeadbeef;
    hr = IDirectInput_QueryInterface(pDI, NULL, (void **)&pUnk);
    ok(hr == E_POINTER, "IDirectInput_QueryInterface returned 0x%08x\n", hr);
    ok(pUnk == (void *)0xdeadbeef, "Output interface pointer is %p\n", pUnk);

    hr = IDirectInput_QueryInterface(pDI, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "IDirectInput_QueryInterface returned 0x%08x\n", hr);

    for (i = 0; i < sizeof(iid_list)/sizeof(iid_list[0]); i++)
    {
        pUnk = NULL;
        hr = IDirectInput_QueryInterface(pDI, iid_list[i], (void **)&pUnk);
        ok(hr == S_OK, "[%d] IDirectInput_QueryInterface returned 0x%08x\n", i, hr);
        ok(pUnk != NULL, "[%d] Output interface pointer is NULL\n", i);
        if (pUnk) IUnknown_Release(pUnk);
    }

    for (i = 0; i < sizeof(no_interface_list)/sizeof(no_interface_list[0]); i++)
    {
        pUnk = (void *)0xdeadbeef;
        hr = IDirectInput_QueryInterface(pDI, no_interface_list[i].riid, (void **)&pUnk);
        if (no_interface_list[i].test_todo)
        {
            todo_wine
            ok(hr == E_NOINTERFACE, "[%d] IDirectInput_QueryInterface returned 0x%08x\n", i, hr);
            todo_wine
            ok(pUnk == NULL, "[%d] Output interface pointer is %p\n", i, pUnk);

            if (pUnk) IUnknown_Release(pUnk);
        }
        else
        {
            ok(hr == E_NOINTERFACE, "[%d] IDirectInput_QueryInterface returned 0x%08x\n", i, hr);
            ok(pUnk == NULL, "[%d] Output interface pointer is %p\n", i, pUnk);
        }
    }

    IDirectInput_Release(pDI);
}

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
    test_QueryInterface();
    test_RunControlPanel();
    CoUninitialize();
}
