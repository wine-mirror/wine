/* Tests for Thread and SHGlobalCounter functions
 *
 * Copyright 2010 Detlef Riekenberg
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
#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "ole2.h"
#include "shlwapi.h"

#include "wine/test.h"

static HRESULT (WINAPI *pSHGetThreadRef)(IUnknown**);
static HRESULT (WINAPI *pSHSetThreadRef)(IUnknown*);

static DWORD AddRef_called;

typedef struct
{
  void* lpVtbl;
  LONG  *ref;
} threadref;

static HRESULT WINAPI threadref_QueryInterface(threadref *This, REFIID riid, LPVOID *ppvObj)
{
    trace("unexpected QueryInterface(%p, %p, %p) called\n", This, riid, ppvObj);
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI threadref_AddRef(threadref *This)
{
    AddRef_called++;
    return InterlockedIncrement(This->ref);
}

static ULONG WINAPI threadref_Release(threadref *This)
{
    trace("unexpected Release(%p) called\n", This);
    return InterlockedDecrement(This->ref);
}

/* VTable */
static void* threadref_vt[] =
{
  threadref_QueryInterface,
  threadref_AddRef,
  threadref_Release
};

static void init_threadref(threadref* iface, LONG *refcount)
{
  iface->lpVtbl = (void*)threadref_vt;
  iface->ref = refcount;
}

/* ##### */

static void test_SHGetThreadRef(void)
{
    IUnknown *punk;
    HRESULT hr;

    /* Not present before IE 5 */
    if (!pSHGetThreadRef) {
        win_skip("SHGetThreadRef not found\n");
        return;
    }

    punk = NULL;
    hr = pSHGetThreadRef(&punk);
    ok( (hr == E_NOINTERFACE) && (punk == NULL),
        "got 0x%x and %p (expected E_NOINTERFACE and NULL)\n", hr, punk);

    if (0) {
        /* this crash on Windows */
        hr = pSHGetThreadRef(NULL);
    }
}

static void test_SHSetThreadRef(void)
{
    threadref ref;
    IUnknown *punk;
    HRESULT hr;
    LONG refcount;

    /* Not present before IE 5 */
    if (!pSHSetThreadRef) {
        win_skip("SHSetThreadRef not found\n");
        return;
    }

    /* start with a clean state */
    hr = pSHSetThreadRef(NULL);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);

    /* build and set out object */
    init_threadref(&ref, &refcount);
    AddRef_called = 0;
    refcount = 1;
    hr = pSHSetThreadRef( (IUnknown *)&ref);
    ok( (hr == S_OK) && (refcount == 1) && (!AddRef_called),
        "got 0x%x with %d, %d (expected S_OK with 1, 0)\n",
        hr, refcount, AddRef_called);

    /* read back our object */
    AddRef_called = 0;
    refcount = 1;
    punk = NULL;
    hr = pSHGetThreadRef(&punk);
    ok( (hr == S_OK) && (punk == (IUnknown *)&ref) && (refcount == 2) && (AddRef_called == 1),
        "got 0x%x and %p with %d, %d (expected S_OK and %p with 2, 1)\n",
        hr, punk, refcount, AddRef_called, &ref);

    /* clear the onject pointer */
    hr = pSHSetThreadRef(NULL);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);

    /* verify, that our object is no longer known as ThreadRef */
    hr = pSHGetThreadRef(&punk);
    ok( (hr == E_NOINTERFACE) && (punk == NULL),
        "got 0x%x and %p (expected E_NOINTERFACE and NULL)\n", hr, punk);

}

START_TEST(thread)
{
    HMODULE hshlwapi = GetModuleHandleA("shlwapi.dll");

    pSHGetThreadRef = (void *) GetProcAddress(hshlwapi, "SHGetThreadRef");
    pSHSetThreadRef = (void *) GetProcAddress(hshlwapi, "SHSetThreadRef");

    test_SHGetThreadRef();
    test_SHSetThreadRef();

}
