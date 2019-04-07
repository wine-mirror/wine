/*
 * AVI compressor filter unit tests
 *
 * Copyright 2018 Zebediah Figura
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
#include "dshow.h"
#include "vfw.h"
#include "wine/test.h"

static const DWORD test_fourcc = mmioFOURCC('w','t','s','t');

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static void test_interfaces(IBaseFilter *filter)
{
    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IPersistPropertyBag, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);

    check_interface(filter, &IID_IAMFilterMiscFlags, FALSE);
    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IKsPropertySet, FALSE);
    check_interface(filter, &IID_IMediaPosition, FALSE);
    check_interface(filter, &IID_IMediaSeeking, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IQualityControl, FALSE);
    check_interface(filter, &IID_IQualProp, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVideoWindow, FALSE);
}

static LRESULT CALLBACK driver_proc(DWORD_PTR id, HDRVR driver, UINT msg,
        LPARAM lparam1, LPARAM lparam2)
{
    static const WCHAR nameW[] = {'f','o','o',0};
    if (winetest_debug > 1) trace("msg %#x, lparam1 %#lx, lparam2 %#lx.\n", msg, lparam1, lparam2);

    switch (msg)
    {
    case DRV_LOAD:
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_FREE:
        return 1;
    case ICM_GETINFO:
    {
        ICINFO *info = (ICINFO *)lparam1;
        info->fccType = ICTYPE_VIDEO;
        info->fccHandler = test_fourcc;
        info->dwFlags = VIDCF_TEMPORAL;
        info->dwVersion = 0x10101;
        info->dwVersionICM = ICVERSION;
        lstrcpyW(info->szName, nameW);
        lstrcpyW(info->szDescription, nameW);
        return sizeof(ICINFO);
    }
    }

    return 0;
}

START_TEST(avico)
{
    static const WCHAR test_display_name[] = {'@','d','e','v','i','c','e',':',
            'c','m',':','{','3','3','D','9','A','7','6','0','-','9','0','C','8',
            '-','1','1','D','0','-','B','D','4','3','-','0','0','A','0','C','9',
            '1','1','C','E','8','6','}','\\','w','t','s','t',0};
    ICreateDevEnum *devenum;
    IEnumMoniker *enummon;
    IBaseFilter *filter;
    IMoniker *mon;
    WCHAR *name;
    HRESULT hr;
    ULONG ref;
    BOOL ret;

    ret = ICInstall(ICTYPE_VIDEO, test_fourcc, (LPARAM)driver_proc, NULL, ICINSTALL_FUNCTION);
    ok(ret, "Failed to install test VFW driver.\n");

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
            &IID_ICreateDevEnum, (void **)&devenum);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = ICreateDevEnum_CreateClassEnumerator(devenum, &CLSID_VideoCompressorCategory, &enummon, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    while (IEnumMoniker_Next(enummon, 1, &mon, NULL) == S_OK)
    {
        hr = IMoniker_GetDisplayName(mon, NULL, NULL, &name);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        if (!lstrcmpW(name, test_display_name))
        {
            hr = IMoniker_BindToObject(mon, NULL, NULL, &IID_IBaseFilter, (void **)&filter);
            ok(hr == S_OK, "Got hr %#x.\n", hr);

            test_interfaces(filter);

            ref = IBaseFilter_Release(filter);
            ok(!ref, "Got outstanding refcount %d.\n", ref);
        }
        CoTaskMemFree(name);

        IMoniker_Release(mon);
    }

    IEnumMoniker_Release(enummon);
    ICreateDevEnum_Release(devenum);

    ret = ICRemove(ICTYPE_VIDEO, test_fourcc, 0);
    ok(ret, "Failed to remove test driver.\n");

    CoUninitialize();
}
