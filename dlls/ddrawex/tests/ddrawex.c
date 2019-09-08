/*
 * Unit tests for ddrawex specific things
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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "ddraw.h"
#include "ddrawex.h"
#include "unknwn.h"

static IDirectDrawFactory *factory;
static HRESULT (WINAPI *pDllGetClassObject)(REFCLSID rclsid, REFIID riid, void **out);

static IDirectDraw *createDDraw(void)
{
    HRESULT hr;
    IDirectDraw *dd;

    hr = IDirectDrawFactory_CreateDirectDraw(factory, NULL, NULL, DDSCL_NORMAL, 0,
                                             0, &dd);
    ok(hr == DD_OK, "Failed to create an IDirectDraw interface, hr = 0x%08x\n", hr);
    return SUCCEEDED(hr) ? dd : NULL;
}

static ULONG get_ref(IUnknown *o)
{
    IUnknown_AddRef(o);
    return IUnknown_Release(o);
}

static void RefCountTest(void)
{
    IDirectDraw *dd1 = createDDraw();
    IDirectDraw2 *dd2;
    IDirectDraw3 *dd3;
    IDirectDraw4 *dd4;
    HRESULT hr;
    ULONG ref;

    ref = get_ref((IUnknown *) dd1);
    ok(ref == 1, "A new ddraw object's refcount is %u, expected 1\n", ref);

    IDirectDraw_AddRef(dd1);
    ref = get_ref((IUnknown *) dd1);
    if (ref == 1)
    {
        win_skip("Refcounting is broken\n");
        IDirectDraw_Release(dd1);
        return;
    }
    ok(ref == 2, "After AddRef the refcount is %u, expected 2\n", ref);
    IDirectDraw_Release(dd1);
    ref = get_ref((IUnknown *) dd1);
    ok(ref == 1, "After Release the refcount is %u, expected 1\n", ref);

    IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw2, (void **) &dd2);
    ref = get_ref((IUnknown *) dd2);
    ok(ref == 2, "IDirectDraw2 refcount is %u, expected 2\n", ref);

    IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw3, (void **) &dd3);
    ref = get_ref((IUnknown *) dd3);
    ok(ref == 3, "IDirectDraw3 refcount is %u, expected 3\n", ref);

    hr = IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw4, (void **) &dd4);
    if (FAILED(hr))
    {
        win_skip("Failed to query IDirectDraw4\n");
        IDirectDraw_Release(dd1);
        IDirectDraw2_Release(dd2);
        IDirectDraw3_Release(dd3);
        return;
    }
    ref = get_ref((IUnknown *) dd4);
    ok(ref == 4, "IDirectDraw4 refcount is %u, expected 4\n", ref);

    IDirectDraw_Release(dd1);
    IDirectDraw2_Release(dd2);
    IDirectDraw3_Release(dd3);

    ref = get_ref((IUnknown *) dd4);
    ok(ref == 1, "IDirectDraw4 refcount is %u, expected 1\n", ref);

    IDirectDraw4_Release(dd4);
}

START_TEST(ddrawex)
{
    IClassFactory *classfactory = NULL;
    ULONG ref;
    HRESULT hr;
    HMODULE hmod = LoadLibraryA("ddrawex.dll");
    if(hmod == NULL) {
        skip("Failed to load ddrawex.dll\n");
        return;
    }
    pDllGetClassObject = (void*)GetProcAddress(hmod, "DllGetClassObject");
    if(pDllGetClassObject == NULL) {
        skip("Failed to get DllGetClassObject\n");
        return;
    }

    hr = pDllGetClassObject(&CLSID_DirectDrawFactory, &IID_IClassFactory, (void **) &classfactory);
    ok(hr == S_OK, "Failed to create a IClassFactory\n");
    hr = IClassFactory_CreateInstance(classfactory, NULL, &IID_IDirectDrawFactory, (void **) &factory);
    ok(hr == S_OK, "Failed to create a IDirectDrawFactory\n");

    RefCountTest();

    if(factory) {
        ref = IDirectDrawFactory_Release(factory);
        ok(ref == 0, "IDirectDrawFactory not cleanly released\n");
    }
    if(classfactory) {
        ref = IClassFactory_Release(classfactory);
        todo_wine ok(ref == 1, "IClassFactory refcount wrong, ref = %u\n", ref);
    }
}
