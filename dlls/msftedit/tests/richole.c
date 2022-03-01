/*
 * Tests for IRichEditOle and friends.
 *
 * Copyright 2008 Google (Dan Hipschman)
 * Copyright 2018 Jactry Zeng for CodeWeavers
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

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <initguid.h>
#include <ole2.h>
#include <richedit.h>
#include <richole.h>
#include <tom.h>
#include <wine/test.h>

static HMODULE msftedit_hmodule;

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %ld, got %ld.\n", ref, rc);
}

static HWND new_window(LPCWSTR classname, DWORD dwstyle, HWND parent)
{
    HWND hwnd = CreateWindowW(classname, NULL,
                              dwstyle | WS_POPUP | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE,
                              0, 0, 200, 60, parent, NULL, msftedit_hmodule, NULL);
    ok(hwnd != NULL, "class: %s, error: %d.\n", wine_dbgstr_w(classname), (int) GetLastError());
    return hwnd;
}

static void test_Interfaces(void)
{
    IRichEditOle *reole = NULL, *reole1 = NULL;
    ITextDocument *txtdoc = NULL;
    ITextDocument2Old *txtdoc2old = NULL;
    ITextDocument2 *txtdoc2 = NULL;
    ITextSelection *txtsel = NULL, *txtsel2;
    IUnknown *punk;
    HRESULT hres;
    LRESULT res;
    HWND hwnd;
    ULONG refcount;

    hwnd = new_window(MSFTEDIT_CLASS, ES_MULTILINE, NULL);
    if (!hwnd)
    {
        skip("Couldn't create window.\n");
        return;
    }

    res = SendMessageA(hwnd, EM_GETOLEINTERFACE, 0, (LPARAM)&reole);
    ok(res, "SendMessage\n");
    ok(reole != NULL, "EM_GETOLEINTERFACE\n");
    EXPECT_REF(reole, 2);

    res = SendMessageA(hwnd, EM_GETOLEINTERFACE, 0, (LPARAM)&reole1);
    ok(res == 1, "SendMessage\n");
    ok(reole1 == reole, "Should not return a new IRichEditOle interface.\n");
    EXPECT_REF(reole, 3);

    hres = IRichEditOle_QueryInterface(reole, &IID_ITextDocument, (void **)&txtdoc);
    ok(hres == S_OK, "IRichEditOle_QueryInterface failed: 0x%08lx.\n", hres);
    ok(txtdoc != NULL, "IRichEditOle_QueryInterface\n");

    hres = ITextDocument_GetSelection(txtdoc, NULL);
    ok(hres == E_INVALIDARG, "ITextDocument_GetSelection: 0x%08lx.\n", hres);

    EXPECT_REF(txtdoc, 4);

    hres = ITextDocument_GetSelection(txtdoc, &txtsel);
    ok(hres == S_OK, "ITextDocument_GetSelection failed 0x%08lx.\n", hres);

    EXPECT_REF(txtdoc, 4);
    EXPECT_REF(txtsel, 2);

    hres = ITextDocument_GetSelection(txtdoc, &txtsel2);
    ok(hres == S_OK, "ITextDocument_GetSelection failed: 0x%08lx.\n", hres);
    ok(txtsel2 == txtsel, "got %p, %p\n", txtsel, txtsel2);

    EXPECT_REF(txtdoc, 4);
    EXPECT_REF(txtsel, 3);

    ITextSelection_Release(txtsel2);

    punk = NULL;
    hres = ITextSelection_QueryInterface(txtsel, &IID_ITextSelection, (void **)&punk);
    ok(hres == S_OK, "ITextSelection_QueryInterface failed: 0x%08lx.\n", hres);
    ok(punk != NULL, "ITextSelection_QueryInterface\n");
    IUnknown_Release(punk);

    punk = NULL;
    hres = ITextSelection_QueryInterface(txtsel, &IID_ITextRange, (void **)&punk);
    ok(hres == S_OK, "ITextSelection_QueryInterface failed: 0x%08lx.\n", hres);
    ok(punk != NULL, "ITextSelection_QueryInterface\n");
    IUnknown_Release(punk);

    punk = NULL;
    hres = ITextSelection_QueryInterface(txtsel, &IID_IDispatch, (void **)&punk);
    ok(hres == S_OK, "ITextSelection_QueryInterface failed: 0x%08lx.\n", hres);
    ok(punk != NULL, "ITextSelection_QueryInterface\n");
    IUnknown_Release(punk);

    punk = NULL;
    hres = IRichEditOle_QueryInterface(reole, &IID_IOleClientSite, (void **)&punk);
    ok(hres == E_NOINTERFACE, "IRichEditOle_QueryInterface: 0x%08lx.\n", hres);

    punk = NULL;
    hres = IRichEditOle_QueryInterface(reole, &IID_IOleWindow, (void **)&punk);
    ok(hres == E_NOINTERFACE, "IRichEditOle_QueryInterface: 0x%08lx.\n", hres);

    punk = NULL;
    hres = IRichEditOle_QueryInterface(reole, &IID_IOleInPlaceSite, (void **)&punk);
    ok(hres == E_NOINTERFACE, "IRichEditOle_QueryInterface: 0x%08lx.\n", hres);

    /* ITextDocument2 is implemented on msftedit after win8 for superseding ITextDocument2Old  */
    hres = IRichEditOle_QueryInterface(reole, &IID_ITextDocument2, (void **)&txtdoc2);
    ok(hres == S_OK ||
       hres == E_NOINTERFACE /* before win8 */, "IRichEditOle_QueryInterface: 0x%08lx.\n", hres);
    if (hres != E_NOINTERFACE)
    {
        ok(txtdoc2 != NULL, "IRichEditOle_QueryInterface\n");
        ok((ITextDocument *)txtdoc2 == txtdoc, "Interface pointer isn't equal.\n");
        EXPECT_REF(txtdoc2, 5);
        EXPECT_REF(reole, 5);

        hres = ITextDocument2_QueryInterface(txtdoc2, &IID_ITextDocument2Old, (void **)&txtdoc2old);
        ok(hres == S_OK, "ITextDocument2_QueryInterface failed: 0x%08lx.\n", hres);
        ok((ITextDocument *)txtdoc2old != txtdoc, "Interface pointer is equal.\n");
        EXPECT_REF(txtdoc2, 5);
        EXPECT_REF(reole, 5);
        EXPECT_REF(txtdoc2old, 1);
        ITextDocument2Old_Release(txtdoc2old);
        ITextDocument2_Release(txtdoc2);
    }
    else
    {
        hres = IRichEditOle_QueryInterface(reole, &IID_ITextDocument2Old, (void **)&txtdoc2old);
        ok(hres == S_OK, "IRichEditOle_QueryInterface failed: 0x%08lx.\n", hres);
        ok(txtdoc2old != NULL, "IRichEditOle_QueryInterface\n");
        ok((ITextDocument *)txtdoc2old == txtdoc, "Interface pointer is equal.\n");
        EXPECT_REF(txtdoc2old, 5);
        EXPECT_REF(reole, 5);
        ITextDocument2Old_Release(txtdoc2old);
    }

    ITextDocument_Release(txtdoc);
    IRichEditOle_Release(reole);
    refcount = IRichEditOle_Release(reole);
    ok(refcount == 1, "Got wrong ref count: %ld.\n", refcount);
    DestroyWindow(hwnd);

    /* Methods should return CO_E_RELEASED if the backing document has
       been released.  One test should suffice.  */
    hres = ITextSelection_CanEdit(txtsel, NULL);
    ok(hres == CO_E_RELEASED, "ITextSelection after ITextDocument destroyed\n");

    ITextSelection_Release(txtsel);
}

START_TEST(richole)
{
    msftedit_hmodule = LoadLibraryA("msftedit.dll");
    ok(msftedit_hmodule != NULL, "error: %d\n", (int) GetLastError());

    test_Interfaces();
}
