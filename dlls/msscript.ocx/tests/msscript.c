/*
 * Copyright 2016 Nikolay Sivov for CodeWeavers
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
#define CONST_VTABLE

#include <initguid.h>
#include <ole2.h>
#include <olectl.h>

#include "msscript.h"
#include "wine/test.h"

static HRESULT WINAPI OleClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IOleClientSite) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IOleClientSite_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI OleClientSite_AddRef(IOleClientSite *iface)
{
    return 2;
}

static ULONG WINAPI OleClientSite_Release(IOleClientSite *iface)
{
    return 1;
}

static HRESULT WINAPI OleClientSite_SaveObject(IOleClientSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_GetMoniker(IOleClientSite *iface, DWORD assign,
    DWORD which, IMoniker **moniker)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_GetContainer(IOleClientSite *iface, IOleContainer **container)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_ShowObject(IOleClientSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_OnShowWindow(IOleClientSite *iface, BOOL show)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl OleClientSiteVtbl = {
    OleClientSite_QueryInterface,
    OleClientSite_AddRef,
    OleClientSite_Release,
    OleClientSite_SaveObject,
    OleClientSite_GetMoniker,
    OleClientSite_GetContainer,
    OleClientSite_ShowObject,
    OleClientSite_OnShowWindow,
    OleClientSite_RequestNewObjectLayout
};

static IOleClientSite testclientsite = { &OleClientSiteVtbl };

static void test_oleobject(void)
{
    DWORD status, dpi_x, dpi_y;
    IOleClientSite *site;
    IOleObject *obj;
    SIZEL extent;
    HRESULT hr;
    HDC hdc;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IOleObject, (void**)&obj);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    if (0) /* crashes on w2k3 */
        hr = IOleObject_GetMiscStatus(obj, DVASPECT_CONTENT, NULL);

    status = 0;
    hr = IOleObject_GetMiscStatus(obj, DVASPECT_CONTENT, &status);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(status != 0, "got 0x%08x\n", status);

    hr = IOleObject_SetClientSite(obj, &testclientsite);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    if (0) /* crashes on w2k3 */
        hr = IOleObject_GetClientSite(obj, NULL);

    hr = IOleObject_GetClientSite(obj, &site);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(site == &testclientsite, "got %p, %p\n", site, &testclientsite);
    IOleClientSite_Release(site);

    hr = IOleObject_SetClientSite(obj, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IOleObject_GetClientSite(obj, &site);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(site == NULL, "got %p\n", site);

    /* extents */
    hdc = GetDC(0);
    dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(0, hdc);

    memset(&extent, 0, sizeof(extent));
    hr = IOleObject_GetExtent(obj, DVASPECT_CONTENT, &extent);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(extent.cx == MulDiv(38, 2540, dpi_x), "got %d\n", extent.cx);
    ok(extent.cy == MulDiv(38, 2540, dpi_y), "got %d\n", extent.cy);

    extent.cx = extent.cy = 0xdeadbeef;
    hr = IOleObject_GetExtent(obj, DVASPECT_THUMBNAIL, &extent);
    ok(hr == DV_E_DVASPECT, "got 0x%08x\n", hr);
    ok(extent.cx == 0xdeadbeef, "got %d\n", extent.cx);
    ok(extent.cy == 0xdeadbeef, "got %d\n", extent.cy);

    extent.cx = extent.cy = 0xdeadbeef;
    hr = IOleObject_GetExtent(obj, DVASPECT_ICON, &extent);
    ok(hr == DV_E_DVASPECT, "got 0x%08x\n", hr);
    ok(extent.cx == 0xdeadbeef, "got %d\n", extent.cx);
    ok(extent.cy == 0xdeadbeef, "got %d\n", extent.cy);

    extent.cx = extent.cy = 0xdeadbeef;
    hr = IOleObject_GetExtent(obj, DVASPECT_DOCPRINT, &extent);
    ok(hr == DV_E_DVASPECT, "got 0x%08x\n", hr);
    ok(extent.cx == 0xdeadbeef, "got %d\n", extent.cx);
    ok(extent.cy == 0xdeadbeef, "got %d\n", extent.cy);

    IOleObject_Release(obj);
}

static void test_persiststreaminit(void)
{
    IPersistStreamInit *init;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IPersistStreamInit, (void**)&init);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IPersistStreamInit_Release(init);
}

static void test_olecontrol(void)
{
    IOleControl *olecontrol;
    CONTROLINFO info;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IOleControl, (void**)&olecontrol);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    memset(&info, 0xab, sizeof(info));
    info.cb = sizeof(info);
    hr = IOleControl_GetControlInfo(olecontrol, &info);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(info.hAccel == NULL, "got %p\n", info.hAccel);
    ok(info.cAccel == 0, "got %d\n", info.cAccel);
    ok(info.dwFlags == 0xabababab, "got %x\n", info.dwFlags);

    memset(&info, 0xab, sizeof(info));
    info.cb = sizeof(info) - 1;
    hr = IOleControl_GetControlInfo(olecontrol, &info);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(info.hAccel == NULL, "got %p\n", info.hAccel);
    ok(info.cAccel == 0, "got %d\n", info.cAccel);
    ok(info.dwFlags == 0xabababab, "got %x\n", info.dwFlags);

    if (0) /* crashes on win2k3 */
    {
        hr = IOleControl_GetControlInfo(olecontrol, NULL);
        ok(hr == E_POINTER, "got 0x%08x\n", hr);
    }

    IOleControl_Release(olecontrol);
}

START_TEST(msscript)
{
    IUnknown *unk;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    if (FAILED(hr)) {
        win_skip("Could not create ScriptControl object: %08x\n", hr);
        return;
    }
    IUnknown_Release(unk);

    test_oleobject();
    test_persiststreaminit();
    test_olecontrol();

    CoUninitialize();
}
