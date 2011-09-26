/*
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

#include "windows.h"
#include "ole2.h"
#include "corerror.h"
#include "mscoree.h"
#include "corhdr.h"

#include "wine/test.h"

#include "initguid.h"
#include "cordebug.h"

static HMODULE hmscoree;

static HRESULT (WINAPI *pCreateDebuggingInterfaceFromVersion)(int, LPCWSTR, IUnknown **);

const WCHAR v2_0[] = {'v','2','.','0','.','5','0','7','2','7',0};

static BOOL init_functionpointers(void)
{
    hmscoree = LoadLibraryA("mscoree.dll");

    if (!hmscoree)
    {
        win_skip("mscoree.dll not available\n");
        return FALSE;
    }

    pCreateDebuggingInterfaceFromVersion = (void *)GetProcAddress(hmscoree, "CreateDebuggingInterfaceFromVersion");

    if (!pCreateDebuggingInterfaceFromVersion)
    {
        win_skip("functions not available\n");
        FreeLibrary(hmscoree);
        return FALSE;
    }

    return TRUE;
}

static void test_createDebugger(void)
{
    HRESULT hr;
    IUnknown *pUnk;
    ICorDebug *pCorDebug;

    hr = pCreateDebuggingInterfaceFromVersion(0, v2_0, &pUnk);
    ok(hr == E_INVALIDARG, "CreateDebuggingInterfaceFromVersion returned %08x\n", hr);

    hr = pCreateDebuggingInterfaceFromVersion(1, v2_0, &pUnk);
    ok(hr == E_INVALIDARG, "CreateDebuggingInterfaceFromVersion returned %08x\n", hr);

    hr = pCreateDebuggingInterfaceFromVersion(2, v2_0, &pUnk);
    ok(hr == E_INVALIDARG, "CreateDebuggingInterfaceFromVersion returned %08x\n", hr);

    hr = pCreateDebuggingInterfaceFromVersion(4, v2_0, &pUnk);
    ok(hr == E_INVALIDARG, "CreateDebuggingInterfaceFromVersion returned %08x\n", hr);

    hr = pCreateDebuggingInterfaceFromVersion(3, v2_0, NULL);
    ok(hr == E_INVALIDARG, "CreateDebuggingInterfaceFromVersion returned %08x\n", hr);

    hr = pCreateDebuggingInterfaceFromVersion(3, v2_0, &pUnk);
    if(hr == S_OK)
    {
        hr = IUnknown_QueryInterface(pUnk, &IID_ICorDebug, (void**)&pCorDebug);
        todo_wine ok(hr == S_OK, "expected S_OK got %08x\n", hr);
        if(hr == S_OK)
        {
            ICorDebug_Release(pCorDebug);
        }
        IUnknown_Release(pUnk);
    }
    else
    {
        skip(".NET 2.0 or mono not installed.\n");
    }
}

START_TEST(debugging)
{
    if (!init_functionpointers())
        return;

    test_createDebugger();

    FreeLibrary(hmscoree);
}
