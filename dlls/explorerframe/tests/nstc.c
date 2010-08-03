/*
 *    Unit tests for the NamespaceTree Control
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

static HWND hwnd;

/* "Intended for internal use" */
#define TVS_EX_NOSINGLECOLLAPSE 0x1

/* Returns FALSE if the NamespaceTreeControl failed to be instantiated. */
static BOOL test_initialization(void)
{
    INameSpaceTreeControl *pnstc;
    IOleWindow *pow;
    IUnknown *punk;
    HWND hwnd_host1;
    LONG lres;
    HRESULT hr;
    RECT rc;

    hr = CoCreateInstance(&CLSID_NamespaceTreeControl, NULL, CLSCTX_INPROC_SERVER,
                          &IID_INameSpaceTreeControl, (void**)&pnstc);
    ok(hr == S_OK || hr == REGDB_E_CLASSNOTREG, "Got 0x%08x\n", hr);
    if(FAILED(hr))
    {
        return FALSE;
    }

    hr = INameSpaceTreeControl_Initialize(pnstc, NULL, NULL, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_TLW_WITH_WSCHILD), "Got (0x%08x)\n", hr);

    hr = INameSpaceTreeControl_Initialize(pnstc, (HWND)0xDEADBEEF, NULL, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE), "Got (0x%08x)\n", hr);

    ZeroMemory(&rc, sizeof(RECT));
    hr = INameSpaceTreeControl_Initialize(pnstc, NULL, &rc, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_TLW_WITH_WSCHILD), "Got (0x%08x)\n", hr);

    hr = INameSpaceTreeControl_Initialize(pnstc, (HWND)0xDEADBEEF, &rc, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE), "Got (0x%08x)\n", hr);

    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleWindow, (void**)&pow);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IOleWindow_GetWindow(pow, &hwnd_host1);
        ok(hr == S_OK, "Got (0x%08x)\n", hr);
        ok(hwnd_host1 == NULL, "hwnd is not null.\n");

        hr = IOleWindow_ContextSensitiveHelp(pow, TRUE);
        ok(hr == E_NOTIMPL, "Got (0x%08x)\n", hr);
        hr = IOleWindow_ContextSensitiveHelp(pow, FALSE);
        ok(hr == E_NOTIMPL, "Got (0x%08x)\n", hr);
        IOleWindow_Release(pow);
    }

    hr = INameSpaceTreeControl_Initialize(pnstc, hwnd, NULL, 0);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleWindow, (void**)&pow);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        static const CHAR namespacetree[] = "NamespaceTreeControl";
        char buf[1024];
        LONG style, expected_style;
        HWND hwnd_tv;
        hr = IOleWindow_GetWindow(pow, &hwnd_host1);
        ok(hr == S_OK, "Got (0x%08x)\n", hr);
        ok(hwnd_host1 != NULL, "hwnd_host1 is null.\n");
        buf[0] = '\0';
        GetClassNameA(hwnd_host1, buf, 1024);
        ok(!lstrcmpA(namespacetree, buf), "Class name was %s\n", buf);

        expected_style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        style = GetWindowLongPtrW(hwnd_host1, GWL_STYLE);
        ok(style == expected_style, "Got style %08x\n", style);

        expected_style = 0;
        style = GetWindowLongPtrW(hwnd_host1, GWL_EXSTYLE);
        ok(style == expected_style, "Got style %08x\n", style);

        expected_style = 0;
        style = SendMessageW(hwnd_host1, TVM_GETEXTENDEDSTYLE, 0, 0);
        ok(style == expected_style, "Got 0x%08x\n", style);

        hwnd_tv = FindWindowExW(hwnd_host1, NULL, WC_TREEVIEWW, NULL);
        ok(hwnd_tv != NULL, "Failed to get treeview hwnd.\n");
        if(hwnd_tv)
        {
            expected_style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
                WS_CLIPCHILDREN | WS_TABSTOP | TVS_NOHSCROLL |
                TVS_NONEVENHEIGHT | TVS_INFOTIP | TVS_TRACKSELECT | TVS_EDITLABELS;
            style = GetWindowLongPtrW(hwnd_tv, GWL_STYLE);
            ok(style == expected_style, "Got style %08x\n", style);

            expected_style = 0;
            style = GetWindowLongPtrW(hwnd_tv, GWL_EXSTYLE);
            ok(style == expected_style, "Got style %08x\n", style);

            expected_style = TVS_EX_NOSINGLECOLLAPSE | TVS_EX_DOUBLEBUFFER |
                TVS_EX_RICHTOOLTIP | TVS_EX_DRAWIMAGEASYNC;
            style = SendMessageW(hwnd_tv, TVM_GETEXTENDEDSTYLE, 0, 0);
            todo_wine ok(style == expected_style, "Got 0x%08x\n", style);
        }

        IOleWindow_Release(pow);
    }

    if(0)
    {
        /* The control can be initialized again without crashing, but
         * the reference counting will break. */
        hr = INameSpaceTreeControl_Initialize(pnstc, hwnd, &rc, 0);
        ok(hr == S_OK, "Got (0x%08x)\n", hr);
        hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleWindow, (void**)&pow);
        if(SUCCEEDED(hr))
        {
            HWND hwnd_host2;
            hr = IOleWindow_GetWindow(pow, &hwnd_host2);
            ok(hr == S_OK, "Got (0x%08x)\n", hr);
            ok(hwnd_host1 != hwnd_host2, "Same hwnd.\n");
            IOleWindow_Release(pow);
        }
    }

    /* Some "random" interfaces */
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceObject, (void**)&punk);
    ok(hr == E_NOINTERFACE || hr == S_OK /* vista, w2k8 */, "Got (0x%08x)\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceActiveObject, (void**)&punk);
    ok(hr == E_NOINTERFACE || hr == S_OK /* vista, w2k8 */, "Got (0x%08x)\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceObjectWindowless, (void**)&punk);
    ok(hr == E_NOINTERFACE || hr == S_OK /* vista, w2k8 */, "Got (0x%08x)\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceUIWindow, (void**)&punk);
    ok(hr == E_NOINTERFACE, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceFrame, (void**)&punk);
    ok(hr == E_NOINTERFACE, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceSite, (void**)&punk);
    ok(hr == E_NOINTERFACE, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceSiteEx, (void**)&punk);
    ok(hr == E_NOINTERFACE, "Got (0x%08x)\n", hr);
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleInPlaceSiteWindowless, (void**)&punk);
    ok(hr == E_NOINTERFACE, "Got (0x%08x)\n", hr);

    /* On windows, the reference count won't go to zero until the
     * window is destroyed. */
    INameSpaceTreeControl_AddRef(pnstc);
    lres = INameSpaceTreeControl_Release(pnstc);
    ok(lres > 1, "Reference count was (%d).\n", lres);

    DestroyWindow(hwnd_host1);
    lres = INameSpaceTreeControl_Release(pnstc);
    ok(!lres, "lres was %d\n", lres);

    return TRUE;
}

static void test_basics(void)
{
    INameSpaceTreeControl *pnstc;
    INameSpaceTreeControl2 *pnstc2;
    IOleWindow *pow;
    HRESULT hr;
    UINT i, res;
    RECT rc;

    hr = CoCreateInstance(&CLSID_NamespaceTreeControl, NULL, CLSCTX_INPROC_SERVER,
                          &IID_INameSpaceTreeControl, (void**)&pnstc);
    ok(hr == S_OK, "Failed to initialize control (0x%08x)\n", hr);

    /* Initialize the control */
    rc.top = rc.left = 0; rc.right = rc.bottom = 200;
    hr = INameSpaceTreeControl_Initialize(pnstc, hwnd, &rc, 0);
    ok(hr == S_OK, "Got (0x%08x)\n", hr);


    /* Set/GetControlStyle(2) */
    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_INameSpaceTreeControl2, (void**)&pnstc2);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE), "Got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        DWORD tmp;
        NSTCSTYLE style;
        NSTCSTYLE2 style2;
        static const NSTCSTYLE2 styles2[] =
            { NSTCS2_INTERRUPTNOTIFICATIONS,NSTCS2_SHOWNULLSPACEMENU,
              NSTCS2_DISPLAYPADDING,NSTCS2_DISPLAYPINNEDONLY,
              NTSCS2_NOSINGLETONAUTOEXPAND,NTSCS2_NEVERINSERTNONENUMERATED, 0};


        /* We can use this to differentiate between two versions of
         * this interface. Windows 7 returns hr == S_OK. */
        hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, 0, 0);
        ok(hr == S_OK || broken(hr == E_FAIL), "Got 0x%08x\n", hr);
        if(hr == S_OK)
        {
            static const NSTCSTYLE styles_setable[] =
                { NSTCS_HASEXPANDOS,NSTCS_HASLINES,NSTCS_SINGLECLICKEXPAND,
                  NSTCS_FULLROWSELECT,NSTCS_HORIZONTALSCROLL,
                  NSTCS_ROOTHASEXPANDO,NSTCS_SHOWSELECTIONALWAYS,NSTCS_NOINFOTIP,
                  NSTCS_EVENHEIGHT,NSTCS_NOREPLACEOPEN,NSTCS_DISABLEDRAGDROP,
                  NSTCS_NOORDERSTREAM,NSTCS_BORDER,NSTCS_NOEDITLABELS,
                  NSTCS_TABSTOP,NSTCS_FAVORITESMODE,NSTCS_EMPTYTEXT,NSTCS_CHECKBOXES,
                  NSTCS_ALLOWJUNCTIONS,NSTCS_SHOWTABSBUTTON,NSTCS_SHOWDELETEBUTTON,
                  NSTCS_SHOWREFRESHBUTTON, 0};
            static const NSTCSTYLE styles_nonsetable[] =
                { NSTCS_SPRINGEXPAND, NSTCS_RICHTOOLTIP, NSTCS_AUTOHSCROLL,
                  NSTCS_FADEINOUTEXPANDOS,
                  NSTCS_PARTIALCHECKBOXES, NSTCS_EXCLUSIONCHECKBOXES,
                  NSTCS_DIMMEDCHECKBOXES, NSTCS_NOINDENTCHECKS,0};

            /* Set/GetControlStyle */
            style = style2 = 0xdeadbeef;
            hr = INameSpaceTreeControl2_GetControlStyle(pnstc2, 0xFFFFFFFF, &style);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style == 0, "Got style %x\n", style);

            hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, 0, 0xFFFFFFF);
            ok(hr == S_OK, "Got 0x%08x\n", hr);

            hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, 0xFFFFFFFF, 0);
            ok(hr == E_FAIL, "Got 0x%08x\n", hr);
            hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, 0xFFFFFFFF, 0xFFFFFFFF);
            ok(hr == E_FAIL, "Got 0x%08x\n", hr);

            tmp = 0;
            for(i = 0; styles_setable[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, styles_setable[i], styles_setable[i]);
                ok(hr == S_OK, "Got 0x%08x (%x)\n", hr, styles_setable[i]);
                if(SUCCEEDED(hr)) tmp |= styles_setable[i];
            }
            for(i = 0; styles_nonsetable[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, styles_nonsetable[i], styles_nonsetable[i]);
                ok(hr == E_FAIL, "Got 0x%08x (%x)\n", hr, styles_nonsetable[i]);
            }

            hr = INameSpaceTreeControl2_GetControlStyle(pnstc2, 0xFFFFFFFF, &style);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style == tmp, "Got style %x (expected %x)\n", style, tmp);
            if(SUCCEEDED(hr))
            {
                DWORD tmp2;
                for(i = 0; styles_setable[i] != 0; i++)
                {
                    hr = INameSpaceTreeControl2_GetControlStyle(pnstc2, styles_setable[i], &tmp2);
                    ok(hr == S_OK, "Got 0x%08x\n", hr);
                    ok(tmp2 == (style & styles_setable[i]), "Got %x\n", tmp2);
                }
            }

            for(i = 0; styles_setable[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, styles_setable[i], 0);
                ok(hr == S_OK, "Got 0x%08x (%x)\n", hr, styles_setable[i]);
            }
            for(i = 0; styles_nonsetable[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, styles_nonsetable[i], 0);
                ok(hr == E_FAIL, "Got 0x%08x (%x)\n", hr, styles_nonsetable[i]);
            }
            hr = INameSpaceTreeControl2_GetControlStyle(pnstc2, 0xFFFFFFFF, &style);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style == 0, "Got style %x\n", style);

            /* Set/GetControlStyle2 */
            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFFFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == 0, "Got style %x\n", style2);

            hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, 0, 0);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, 0, 0xFFFFFFFF);
            ok(hr == S_OK, "Got 0x%08x\n", hr);

            hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, 0xFFFFFFFF, 0);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, 0xFFFFFFFF, 0xFFFFFFFF);
            ok(hr == S_OK, "Got 0x%08x\n", hr);

            hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, 0xFFFFFFFF, 0);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFFFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == 0x00000000, "Got style %x\n", style2);

            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == 0, "Got style %x\n", style2);

            tmp = 0;
            for(i = 0; styles2[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, styles2[i], styles2[i]);
                ok(hr == S_OK, "Got 0x%08x (%x)\n", hr, styles2[i]);
                if(SUCCEEDED(hr)) tmp |= styles2[i];
            }

            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == tmp, "Got style %x (expected %x)\n", style2, tmp);
            if(SUCCEEDED(hr))
            {
                DWORD tmp2;
                for(i = 0; styles2[i] != 0; i++)
                {
                    hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, styles2[i], &tmp2);
                    ok(hr == S_OK, "Got 0x%08x\n", hr);
                    ok(tmp2 == (style2 & styles2[i]), "Got %x\n", tmp2);
                }
            }

            for(i = 0; styles2[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, styles2[i], 0);
                ok(hr == S_OK, "Got 0x%08x (%x)\n", hr, styles2[i]);
            }
            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == 0, "Got style %x (expected 0)\n", style2);
        }
        else
        {
            /* 64-bit Windows Vista (others?) seems to have a somewhat
             * different idea of how the methods of this interface
             * should behave. */

            static const NSTCSTYLE styles[] =
                { NSTCS_HASEXPANDOS,NSTCS_HASLINES,NSTCS_SINGLECLICKEXPAND,
                  NSTCS_FULLROWSELECT,NSTCS_SPRINGEXPAND,NSTCS_HORIZONTALSCROLL,
                  NSTCS_RICHTOOLTIP, NSTCS_AUTOHSCROLL,
                  NSTCS_FADEINOUTEXPANDOS,
                  NSTCS_PARTIALCHECKBOXES,NSTCS_EXCLUSIONCHECKBOXES,
                  NSTCS_DIMMEDCHECKBOXES, NSTCS_NOINDENTCHECKS,
                  NSTCS_ROOTHASEXPANDO,NSTCS_SHOWSELECTIONALWAYS,NSTCS_NOINFOTIP,
                  NSTCS_EVENHEIGHT,NSTCS_NOREPLACEOPEN,NSTCS_DISABLEDRAGDROP,
                  NSTCS_NOORDERSTREAM,NSTCS_BORDER,NSTCS_NOEDITLABELS,
                  NSTCS_TABSTOP,NSTCS_FAVORITESMODE,NSTCS_EMPTYTEXT,NSTCS_CHECKBOXES,
                  NSTCS_ALLOWJUNCTIONS,NSTCS_SHOWTABSBUTTON,NSTCS_SHOWDELETEBUTTON,
                  NSTCS_SHOWREFRESHBUTTON, 0};
            trace("Detected broken INameSpaceTreeControl2.\n");

            style = 0xdeadbeef;
            hr = INameSpaceTreeControl2_GetControlStyle(pnstc2, 0xFFFFFFFF, &style);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style == 0xdeadbeef, "Got style %x\n", style);

            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFFFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == 0, "Got style %x\n", style2);

            tmp = 0;
            for(i = 0; styles[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle(pnstc2, styles[i], styles[i]);
                ok(hr == E_FAIL || ((styles[i] & NSTCS_SPRINGEXPAND) && hr == S_OK),
                   "Got 0x%08x (%x)\n", hr, styles[i]);
                if(SUCCEEDED(hr)) tmp |= styles[i];
            }

            style = 0xdeadbeef;
            hr = INameSpaceTreeControl2_GetControlStyle(pnstc2, tmp, &style);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style == 0xdeadbeef, "Got style %x\n", style);

            tmp = 0;
            for(i = 0; styles2[i] != 0; i++)
            {
                hr = INameSpaceTreeControl2_SetControlStyle2(pnstc2, styles2[i], styles2[i]);
                ok(hr == S_OK, "Got 0x%08x (%x)\n", hr, styles2[i]);
                if(SUCCEEDED(hr)) tmp |= styles2[i];
            }

            style2 = 0xdeadbeef;
            hr = INameSpaceTreeControl2_GetControlStyle2(pnstc2, 0xFFFFFFFF, &style2);
            ok(hr == S_OK, "Got 0x%08x\n", hr);
            ok(style2 == tmp, "Got style %x\n", style2);

        }

        INameSpaceTreeControl2_Release(pnstc2);
    }
    else
    {
        skip("INameSpaceTreeControl2 missing.\n");
    }

    hr = INameSpaceTreeControl_QueryInterface(pnstc, &IID_IOleWindow, (void**)&pow);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        HWND hwnd_nstc;
        hr = IOleWindow_GetWindow(pow, &hwnd_nstc);
        ok(hr == S_OK, "Got 0x%08x\n", hr);
        DestroyWindow(hwnd_nstc);
        IOleWindow_Release(pow);
    }

    res = INameSpaceTreeControl_Release(pnstc);
    ok(!res, "res was %d!\n", res);
}

static void setup_window(void)
{
    WNDCLASSA wc;
    static const char nstctest_wnd_name[] = "nstctest_wnd";

    ZeroMemory(&wc, sizeof(WNDCLASSA));
    wc.lpfnWndProc      = DefWindowProcA;
    wc.lpszClassName    = nstctest_wnd_name;
    RegisterClassA(&wc);
    hwnd = CreateWindowA(nstctest_wnd_name, NULL, WS_TABSTOP,
                         0, 0, 200, 200, NULL, 0, 0, NULL);
    ok(hwnd != NULL, "Failed to create window for test (lasterror: %d).\n",
       GetLastError());
}

static void destroy_window(void)
{
    DestroyWindow(hwnd);
}

START_TEST(nstc)
{
    OleInitialize(NULL);
    setup_window();

    if(test_initialization())
    {
        test_basics();
    }
    else
    {
        win_skip("No NamespaceTreeControl (or instantiation failed).\n");
    }

    destroy_window();
    OleUninitialize();
}
