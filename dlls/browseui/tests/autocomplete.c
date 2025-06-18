/* Unit tests for autocomplete
 *
 * Copyright 2007 Mikolaj Zalewski
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

#include <initguid.h>
#include <windows.h>
#include <shlobj.h>
#include <shldisp.h>
#include <shlwapi.h>
#include <shlguid.h>

#include "wine/test.h"

#define ole_ok(exp) \
{ \
    HRESULT res = (exp); \
    if (res != S_OK) \
        ok(FALSE, #exp " failed: %#lx\n", res); \
}

typedef struct
{
    IEnumString IEnumString_iface;
    IACList IACList_iface;
    LONG ref;
    HRESULT expret;
    INT expcount;
    INT pos;
    INT limit;
    const WCHAR **data;
} TestACL;

extern IEnumStringVtbl TestACLVtbl;
extern IACListVtbl TestACL_ACListVtbl;

static inline TestACL *impl_from_IEnumString(IEnumString *iface)
{
    return CONTAINING_RECORD(iface, TestACL, IEnumString_iface);
}

static TestACL *impl_from_IACList(IACList *iface)
{
    return CONTAINING_RECORD(iface, TestACL, IACList_iface);
}

static TestACL *TestACL_Constructor(int limit, const WCHAR **strings)
{
    TestACL *This = CoTaskMemAlloc(sizeof(TestACL));
    ZeroMemory(This, sizeof(*This));
    This->IEnumString_iface.lpVtbl = &TestACLVtbl;
    This->IACList_iface.lpVtbl = &TestACL_ACListVtbl;
    This->ref = 1;
    This->expret = S_OK;
    This->limit = limit;
    This->data = strings;
    return This;
}

static ULONG STDMETHODCALLTYPE TestACL_AddRef(IEnumString *iface)
{
    TestACL *This = impl_from_IEnumString(iface);
    trace("ACL(%p): addref (%ld)\n", This, This->ref+1);
    return InterlockedIncrement(&This->ref);
}

static ULONG STDMETHODCALLTYPE TestACL_Release(IEnumString *iface)
{
    TestACL *This = impl_from_IEnumString(iface);
    ULONG res;

    res = InterlockedDecrement(&This->ref);
    trace("ACL(%p): release (%ld)\n", This, res);
    return res;
}

static HRESULT STDMETHODCALLTYPE TestACL_QueryInterface(IEnumString *iface, REFIID iid, LPVOID *ppvOut)
{
    TestACL *This = impl_from_IEnumString(iface);
    *ppvOut = NULL;
    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IEnumString))
    {
        *ppvOut = iface;
    }
    else if (IsEqualGUID(iid, &IID_IACList))
    {
        *ppvOut = &This->IACList_iface;
    }

    if (*ppvOut)
    {
        IEnumString_AddRef(iface);
        return S_OK;
    }

    if (!IsEqualGUID(iid, &IID_IEnumACString))
        trace("unknown interface queried\n");
    return E_NOINTERFACE;
}

static HRESULT STDMETHODCALLTYPE TestACL_Next(IEnumString *iface, ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    TestACL *This = impl_from_IEnumString(iface);
    int size;
    ULONG i;

    trace("ACL(%p): read %ld item(s)\n", This, celt);
    for (i = 0; i < celt; i++)
    {
        if (This->pos >= This->limit)
            break;

        size = wcslen(This->data[This->pos]);
        rgelt[i] = CoTaskMemAlloc((size + 1) * sizeof(WCHAR));
        wcscpy(rgelt[i], This->data[This->pos]);
        This->pos++;
    }

    if (pceltFetched)
        *pceltFetched = i;
    if (i == celt)
        return S_OK;
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE TestACL_Skip(IEnumString *iface, ULONG celt)
{
    ok(FALSE, "Unexpected call to TestACL_Skip\n");
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE TestACL_Clone(IEnumString *iface, IEnumString **out)
{
    ok(FALSE, "Unexpected call to TestACL_Clone\n");
    return E_OUTOFMEMORY;
}

static HRESULT STDMETHODCALLTYPE TestACL_Reset(IEnumString *iface)
{
    TestACL *This = impl_from_IEnumString(iface);
    trace("ACL(%p): Reset\n", This);
    This->pos = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE TestACL_Expand(IACList *iface, LPCOLESTR str)
{
    TestACL *This = impl_from_IACList(iface);
    trace("ACL(%p): Expand\n", This);
    This->expcount++;
    return This->expret;
}

IEnumStringVtbl TestACLVtbl =
{
    TestACL_QueryInterface,
    TestACL_AddRef,
    TestACL_Release,

    TestACL_Next,
    TestACL_Skip,
    TestACL_Reset,
    TestACL_Clone
};

static ULONG STDMETHODCALLTYPE TestACL_ACList_AddRef(IACList *iface)
{
    TestACL *This = impl_from_IACList(iface);
    return TestACL_AddRef(&This->IEnumString_iface);
}

static ULONG STDMETHODCALLTYPE TestACL_ACList_Release(IACList *iface)
{
    TestACL *This = impl_from_IACList(iface);
    return TestACL_Release(&This->IEnumString_iface);
}

static HRESULT STDMETHODCALLTYPE TestACL_ACList_QueryInterface(IACList *iface, REFIID iid, LPVOID *ppvout)
{
    TestACL *This = impl_from_IACList(iface);
    return TestACL_QueryInterface(&This->IEnumString_iface, iid, ppvout);
}

IACListVtbl TestACL_ACListVtbl =
{
    TestACL_ACList_QueryInterface,
    TestACL_ACList_AddRef,
    TestACL_ACList_Release,

    TestACL_Expand
};

#define expect_str(obj, str)  \
{ \
    ole_ok(IEnumString_Next(obj, 1, &wstr, &i)); \
    ok(i == 1, "Expected i == 1, got %ld\n", i); \
    ok(str[0] == wstr[0], "String mismatch\n"); \
    CoTaskMemFree(wstr); \
}

static void test_ACLMulti(void)
{
    const WCHAR *strings1[] = { L"a", L"c", L"e" };
    const WCHAR *strings2[] = { L"a", L"b", L"d" };
    const WCHAR exp[] = L"ABC";
    IEnumString *obj;
    IEnumACString *unk;
    HRESULT hr;
    TestACL *acl1, *acl2;
    IACList *acl;
    IObjMgr *mgr;
    LPWSTR wstr;
    LPWSTR wstrtab[15];
    ULONG ref, i;
    LPVOID tmp;

    hr = CoCreateInstance(&CLSID_ACLMulti, NULL, CLSCTX_INPROC, &IID_IEnumString, (void**)&obj);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr != S_OK) return;

    hr = IEnumString_QueryInterface(obj, &IID_IACList, (void**)&acl);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumString_QueryInterface(obj, &IID_IACList2, &tmp);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);
    hr = IEnumString_QueryInterface(obj, &IID_IObjMgr, (void**)&mgr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IEnumString_QueryInterface(obj, &IID_IEnumACString, (LPVOID*)&unk);
    if (hr == E_NOINTERFACE)
        todo_wine win_skip("IEnumACString is not supported, skipping tests\n");
    else
    {
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (unk != NULL)
            IEnumACString_Release(unk);
    }

    i = -1;
    hr = IEnumString_Next(obj, 1, (LPOLESTR *)&tmp, &i);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(i == 0, "Unexpected fetched value %ld\n", i);
    hr = IEnumString_Next(obj, 44, (LPOLESTR *)&tmp, &i);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    hr = IEnumString_Skip(obj, 1);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    hr = IEnumString_Clone(obj, (IEnumString **)&tmp);
    ok(hr == E_OUTOFMEMORY, "Unexpected hr %#lx.\n", hr);
    hr = IACList_Expand(acl, exp);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    acl1 = TestACL_Constructor(3, strings1);
    acl2 = TestACL_Constructor(3, strings2);
    hr = IObjMgr_Append(mgr, (IUnknown *)&acl1->IACList_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IObjMgr_Append(mgr, (IUnknown *)&acl2->IACList_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IObjMgr_Append(mgr, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    expect_str(obj, "a");
    expect_str(obj, "c");
    expect_str(obj, "e");
    expect_str(obj, "a");
    expect_str(obj, "b");
    expect_str(obj, "d");
    hr = IEnumString_Next(obj, 1, &wstr, &i);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IEnumString_Reset(obj);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(acl1->pos == 0, "acl1 not reset\n");
    ok(acl2->pos == 0, "acl2 not reset\n");

    hr = IACList_Expand(acl, exp);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(acl1->expcount == 1, "expcount - expected 1, got %d\n", acl1->expcount);
    ok(acl2->expcount == 0 /* XP */ || acl2->expcount == 1 /* Vista */,
        "expcount - expected 0 or 1, got %d\n", acl2->expcount);

    hr = IEnumString_Next(obj, 15, wstrtab, &i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(i == 1, "Expected i == 1, got %ld\n", i);
    CoTaskMemFree(wstrtab[0]);

    hr = IEnumString_Next(obj, 15, wstrtab, &i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CoTaskMemFree(wstrtab[0]);

    hr = IEnumString_Next(obj, 15, wstrtab, &i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CoTaskMemFree(wstrtab[0]);

    hr = IEnumString_Next(obj, 15, wstrtab, &i);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CoTaskMemFree(wstrtab[0]);

    hr = IACList_Expand(acl, exp);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(acl1->expcount == 2, "expcount - expected 1, got %d\n", acl1->expcount);
    ok(acl2->expcount == 0 /* XP */ || acl2->expcount == 2 /* Vista */,
        "expcount - expected 0 or 2, got %d\n", acl2->expcount);
    acl1->expret = S_FALSE;
    hr = IACList_Expand(acl, exp);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(acl1->expcount == 3, "expcount - expected 1, got %d\n", acl1->expcount);
    ok(acl2->expcount == 1 /* XP */ || acl2->expcount == 3 /* Vista */,
        "expcount - expected 0 or 3, got %d\n", acl2->expcount);
    acl1->expret = E_NOTIMPL;
    hr = IACList_Expand(acl, exp);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(acl1->expcount == 4, "expcount - expected 1, got %d\n", acl1->expcount);
    ok(acl2->expcount == 2 /* XP */ || acl2->expcount == 4 /* Vista */,
        "expcount - expected 0 or 4, got %d\n", acl2->expcount);
    acl2->expret = E_OUTOFMEMORY;
    hr = IACList_Expand(acl, exp);
    ok(hr == E_OUTOFMEMORY, "Unexpected hr %#lx.\n", hr);
    acl2->expret = E_FAIL;
    hr = IACList_Expand(acl, exp);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IObjMgr_Remove(mgr, (IUnknown *)&acl1->IACList_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(acl1->ref == 1, "acl1 not released\n");
    hr = IEnumString_Next(obj, 1, &wstr, &i);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    IEnumString_Reset(obj);
    expect_str(obj, "a");
    expect_str(obj, "b");
    expect_str(obj, "d");
    hr = IEnumString_Next(obj, 1, &wstr, &i);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    IEnumString_Release(obj);
    IACList_Release(acl);
    ref = IObjMgr_Release(mgr);
    ok(ref == 0, "Unexpected references\n");
    ok(acl1->ref == 1, "acl1 not released\n");
    ok(acl2->ref == 1, "acl2 not released\n");

    CoTaskMemFree(acl1);
    CoTaskMemFree(acl2);
}

static void test_ACListISF(void)
{
    IEnumString *enumstring;
    IACList *list, *list2;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ACListISF, NULL, CLSCTX_INPROC, &IID_IACList, (void**)&list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IACList_QueryInterface(list, &IID_IEnumString, (void**)&enumstring);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IEnumString_QueryInterface(enumstring, &IID_IACList, (void**)&list2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(list == list2, "got %p, %p\n", list, list2);
    IACList_Release(list2);

    IEnumString_Release(enumstring);
    IACList_Release(list);
}

START_TEST(autocomplete)
{
    CoInitialize(NULL);

    test_ACLMulti();
    test_ACListISF();

    CoUninitialize();
}
