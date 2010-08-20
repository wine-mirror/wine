/*
 *    Unit tests for the Explorer Browser control
 *
 * Copyright 2010 David Hedberg
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

#include "shlobj.h"

#include "wine/test.h"

/*********************************************************************
 * Some simple helpers
 */
static HRESULT ebrowser_instantiate(IExplorerBrowser **peb)
{
    return CoCreateInstance(&CLSID_ExplorerBrowser, NULL, CLSCTX_INPROC_SERVER,
                            &IID_IExplorerBrowser, (void**)peb);
}

static void test_QueryInterface(void)
{
    IExplorerBrowser *peb;
    IUnknown *punk;
    HRESULT hr;
    LONG lres;

    hr = ebrowser_instantiate(&peb);
    ok(hr == S_OK, "Got 0x%08x\n", hr);

#define test_qinterface(iid, exp)                                       \
    do {                                                                \
        hr = IExplorerBrowser_QueryInterface(peb, &iid, (void**)&punk); \
        ok(hr == exp, "(%s:)Expected (0x%08x), got (0x%08x)\n",         \
           #iid, exp, hr);                                              \
        if(SUCCEEDED(hr)) IUnknown_Release(punk);                       \
    } while(0)

    test_qinterface(IID_IUnknown, S_OK);
    test_qinterface(IID_IExplorerBrowser, S_OK);
    test_qinterface(IID_IShellBrowser, S_OK);
    todo_wine test_qinterface(IID_IOleWindow, S_OK);
    todo_wine test_qinterface(IID_ICommDlgBrowser, S_OK);
    todo_wine test_qinterface(IID_ICommDlgBrowser2, S_OK);
    todo_wine test_qinterface(IID_ICommDlgBrowser3, S_OK);
    todo_wine test_qinterface(IID_IServiceProvider, S_OK);
    todo_wine test_qinterface(IID_IObjectWithSite, S_OK);
    todo_wine test_qinterface(IID_IConnectionPointContainer, S_OK);
    test_qinterface(IID_IOleObject, E_NOINTERFACE);
    test_qinterface(IID_IViewObject, E_NOINTERFACE);
    test_qinterface(IID_IViewObject2, E_NOINTERFACE);
    test_qinterface(IID_IViewObjectEx, E_NOINTERFACE);
    test_qinterface(IID_IConnectionPoint, E_NOINTERFACE);
    test_qinterface(IID_IShellView, E_NOINTERFACE);
    test_qinterface(IID_INameSpaceTreeControlEvents, E_NOINTERFACE);

#undef test_qinterface

    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got %d\n", lres);
}

static void test_SB_misc(void)
{
    IExplorerBrowser *peb;
    IShellBrowser *psb;
    HRESULT hr;
    HWND retHwnd;
    LONG lres;

    ebrowser_instantiate(&peb);
    hr = IExplorerBrowser_QueryInterface(peb, &IID_IShellBrowser, (void**)&psb);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    if(FAILED(hr))
    {
        skip("Failed to get IShellBrowser interface.\n");
        return;
    }

    /* Some unimplemented methods */
    retHwnd = (HWND)0xDEADBEEF;
    hr = IShellBrowser_GetControlWindow(psb, FCW_TOOLBAR, &retHwnd);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);
    ok(retHwnd == (HWND)0xDEADBEEF, "HWND overwritten\n");

    hr = IShellBrowser_GetControlWindow(psb, FCW_STATUS, &retHwnd);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);
    ok(retHwnd == (HWND)0xDEADBEEF, "HWND overwritten\n");

    hr = IShellBrowser_GetControlWindow(psb, FCW_TREE, &retHwnd);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);
    ok(retHwnd == (HWND)0xDEADBEEF, "HWND overwritten\n");

    hr = IShellBrowser_GetControlWindow(psb, FCW_PROGRESS, &retHwnd);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);
    ok(retHwnd == (HWND)0xDEADBEEF, "HWND overwritten\n");

    /* ::InsertMenuSB */
    hr = IShellBrowser_InsertMenusSB(psb, NULL, NULL);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);

    /* ::RemoveMenusSB */
    hr = IShellBrowser_RemoveMenusSB(psb, NULL);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);

    /* ::SetMenuSB */
    hr = IShellBrowser_SetMenuSB(psb, NULL, NULL, NULL);
    ok(hr == E_NOTIMPL, "got (0x%08x)\n", hr);

    IShellBrowser_Release(psb);
    lres = IExplorerBrowser_Release(peb);
    ok(lres == 0, "Got %d\n", lres);
}

static BOOL test_instantiate_control(void)
{
    IExplorerBrowser *peb;
    HRESULT hr;

    hr = ebrowser_instantiate(&peb);
    ok(hr == S_OK || hr == REGDB_E_CLASSNOTREG, "Got (0x%08x)\n", hr);
    if(FAILED(hr))
        return FALSE;

    IExplorerBrowser_Release(peb);
    return TRUE;
}

START_TEST(ebrowser)
{
    OleInitialize(NULL);

    if(!test_instantiate_control())
    {
        win_skip("No ExplorerBrowser control..\n");
        OleUninitialize();
        return;
    }

    test_QueryInterface();
    test_SB_misc();

    OleUninitialize();
}
