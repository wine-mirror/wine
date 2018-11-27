/*
 * Copyright 2018 Nikolay Sivov for CodeWeavers
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

#include <windows.h>
#include "initguid.h"
#include "objidl.h"

#include "wine/test.h"

static HRESULT (WINAPI *pGetProcessReference)(IUnknown **);
static void (WINAPI *pSetProcessReference)(IUnknown *);
static HRESULT (WINAPI *pSHGetInstanceExplorer)(IUnknown **);

static void init(HMODULE hshcore)
{
#define X(f) p##f = (void*)GetProcAddress(hshcore, #f)
    X(GetProcessReference);
    X(SetProcessReference);
#undef X
}

static HRESULT WINAPI unk_QI(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

struct test_unk
{
    IUnknown IUnknown_iface;
    LONG refcount;
};

static struct test_unk *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct test_unk, IUnknown_iface);
}

static ULONG WINAPI unk_AddRef(IUnknown *iface)
{
    struct test_unk *obj = impl_from_IUnknown(iface);
    return InterlockedIncrement(&obj->refcount);
}

static ULONG WINAPI unk_Release(IUnknown *iface)
{
    struct test_unk *obj = impl_from_IUnknown(iface);
    return InterlockedDecrement(&obj->refcount);
}

static const IUnknownVtbl testunkvtbl =
{
    unk_QI,
    unk_AddRef,
    unk_Release,
};

static void test_unk_init(struct test_unk *testunk)
{
    testunk->IUnknown_iface.lpVtbl = &testunkvtbl;
    testunk->refcount = 1;
}

static void test_process_reference(void)
{
    struct test_unk test_unk, test_unk2;
    IUnknown *obj;
    HMODULE hmod;
    HRESULT hr;

    obj = (void *)0xdeadbeef;
    hr = pGetProcessReference(&obj);
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);
    ok(obj == NULL, "Unexpected pointer.\n");

    test_unk_init(&test_unk);
    test_unk_init(&test_unk2);

    pSetProcessReference(&test_unk.IUnknown_iface);
    ok(test_unk.refcount == 1, "Unexpected refcount %u.\n", test_unk.refcount);
    pSetProcessReference(&test_unk2.IUnknown_iface);
    ok(test_unk.refcount == 1, "Unexpected refcount %u.\n", test_unk.refcount);
    ok(test_unk2.refcount == 1, "Unexpected refcount %u.\n", test_unk2.refcount);

    hr = pGetProcessReference(&obj);
    ok(hr == S_OK, "Failed to get reference, hr %#x.\n", hr);
    ok(obj == &test_unk2.IUnknown_iface, "Unexpected pointer.\n");
    ok(test_unk2.refcount == 2, "Unexpected refcount %u.\n", test_unk2.refcount);

    hmod = LoadLibraryA("shell32.dll");

    pSHGetInstanceExplorer = (void *)GetProcAddress(hmod, "SHGetInstanceExplorer");
    hr = pSHGetInstanceExplorer(&obj);
todo_wine {
    ok(hr == S_OK, "Failed to get reference, hr %#x.\n", hr);
    ok(obj == &test_unk2.IUnknown_iface, "Unexpected pointer.\n");
    ok(test_unk2.refcount == 3, "Unexpected refcount %u.\n", test_unk2.refcount);
}
}

START_TEST(shcore)
{
    HMODULE hshcore = LoadLibraryA("shcore.dll");

    if (!hshcore)
    {
        win_skip("Shcore.dll is not available.\n");
        return;
    }

    init(hshcore);

    test_process_reference();
}
