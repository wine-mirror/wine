/*
 * DirectSound renderer filter unit tests
 *
 * Copyright (C) 2010 Maarten Lankhorst for CodeWeavers
 * Copyright (C) 2007 Google (Lei Zhang)
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
#include "initguid.h"
#include "dsound.h"
#include "amaudio.h"
#include "wine/test.h"

#define QI_SUCCEED(iface, riid, ppv) hr = IUnknown_QueryInterface(iface, &riid, (LPVOID*)&ppv); \
    ok(hr == S_OK, "IUnknown_QueryInterface returned %x\n", hr); \
    ok(ppv != NULL, "Pointer is NULL\n");

#define RELEASE_EXPECT(iface, num) if (iface) { \
    hr = IUnknown_Release((IUnknown*)iface); \
    ok(hr == num, "IUnknown_Release should return %d, got %d\n", num, hr); \
}

static IUnknown *pDSRender = NULL;

static BOOL create_dsound_renderer(void)
{
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (LPVOID*)&pDSRender);
    return (hr == S_OK && pDSRender != NULL);
}

static void release_dsound_renderer(void)
{
    HRESULT hr;

    hr = IUnknown_Release(pDSRender);
    ok(hr == 0, "IUnknown_Release failed with %x\n", hr);
}

static void test_property_bag(void)
{
    IPersistPropertyBag *ppb;
    ICreateDevEnum *devenum;
    IEnumMoniker *enummon;
    IPropertyBag *propbag;
    IMoniker *mon;
    HRESULT hr;

    CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
            &IID_ICreateDevEnum, (void **)&devenum);
    ICreateDevEnum_CreateClassEnumerator(devenum, &CLSID_AudioRendererCategory, &enummon, 0);

    while (IEnumMoniker_Next(enummon, 1, &mon, NULL) == S_OK)
    {
        hr = CoCreateInstance(&CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER,
                &IID_IPersistPropertyBag, (void **)&ppb);
        todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
        if (hr != S_OK) break;

        IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&propbag);

        hr = IPersistPropertyBag_InitNew(ppb);
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IPersistPropertyBag_Load(ppb, propbag, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        IPersistPropertyBag_Release(ppb);
        IPropertyBag_Release(propbag);
        IMoniker_Release(mon);
    }

    IEnumMoniker_Release(enummon);
    ICreateDevEnum_Release(devenum);
}

static void test_query_interface(void)
{
    HRESULT hr;
    IBaseFilter *pBaseFilter = NULL;
    IBasicAudio *pBasicAudio = NULL;
    IMediaPosition *pMediaPosition = NULL;
    IMediaSeeking *pMediaSeeking = NULL;
    IQualityControl *pQualityControl = NULL;
    IPersistPropertyBag *ppb = NULL;
    IDirectSound3DBuffer *ds3dbuf = NULL;
    IReferenceClock *clock = NULL;
    IAMDirectSound *pAMDirectSound = NULL;

    QI_SUCCEED(pDSRender, IID_IBaseFilter, pBaseFilter);
    RELEASE_EXPECT(pBaseFilter, 1);
    QI_SUCCEED(pDSRender, IID_IBasicAudio, pBasicAudio);
    RELEASE_EXPECT(pBasicAudio, 1);
    QI_SUCCEED(pDSRender, IID_IMediaSeeking, pMediaSeeking);
    RELEASE_EXPECT(pMediaSeeking, 1);
    QI_SUCCEED(pDSRender, IID_IReferenceClock, clock);
    RELEASE_EXPECT(clock, 1);
    QI_SUCCEED(pDSRender, IID_IAMDirectSound, pAMDirectSound);
    RELEASE_EXPECT( pAMDirectSound, 1);
    todo_wine {
    QI_SUCCEED(pDSRender, IID_IDirectSound3DBuffer, ds3dbuf);
    RELEASE_EXPECT(ds3dbuf, 1);
    QI_SUCCEED(pDSRender, IID_IPersistPropertyBag, ppb);
    RELEASE_EXPECT(ppb, 1);
    }
    QI_SUCCEED(pDSRender, IID_IMediaPosition, pMediaPosition);
    RELEASE_EXPECT(pMediaPosition, 1);
    QI_SUCCEED(pDSRender, IID_IQualityControl, pQualityControl);
    RELEASE_EXPECT(pQualityControl, 1);
}

static void test_pin(IPin *pin)
{
    IMemInputPin *mpin = NULL;

    IPin_QueryInterface(pin, &IID_IMemInputPin, (void **)&mpin);

    ok(mpin != NULL, "No IMemInputPin found!\n");
    if (mpin)
    {
        ok(IMemInputPin_ReceiveCanBlock(mpin) == S_OK, "Receive can't block for pin!\n");
        ok(IMemInputPin_NotifyAllocator(mpin, NULL, 0) == E_POINTER, "NotifyAllocator likes a NULL pointer argument\n");
        IMemInputPin_Release(mpin);
    }
    /* TODO */
}

static void test_basefilter(void)
{
    IEnumPins *pin_enum = NULL;
    IBaseFilter *base = NULL;
    IPin *pins[2];
    ULONG ref;
    HRESULT hr;

    IUnknown_QueryInterface(pDSRender, &IID_IBaseFilter, (void **)&base);
    if (base == NULL)
    {
        /* test_query_interface handles this case */
        skip("No IBaseFilter\n");
        return;
    }

    hr = IBaseFilter_EnumPins(base, NULL);
    ok(hr == E_POINTER, "hr = %08x and not E_POINTER\n", hr);

    hr= IBaseFilter_EnumPins(base, &pin_enum);
    ok(hr == S_OK, "hr = %08x and not S_OK\n", hr);

    hr = IEnumPins_Next(pin_enum, 1, NULL, NULL);
    ok(hr == E_POINTER, "hr = %08x and not E_POINTER\n", hr);

    hr = IEnumPins_Next(pin_enum, 2, pins, NULL);
    ok(hr == E_INVALIDARG, "hr = %08x and not E_INVALIDARG\n", hr);

    pins[0] = (void *)0xdead;
    pins[1] = (void *)0xdeed;

    hr = IEnumPins_Next(pin_enum, 2, pins, &ref);
    ok(hr == S_FALSE, "hr = %08x instead of S_FALSE\n", hr);
    ok(pins[0] != (void *)0xdead && pins[0] != NULL, "pins[0] = %p\n", pins[0]);
    if (pins[0] != (void *)0xdead && pins[0] != NULL)
    {
        test_pin(pins[0]);
        IPin_Release(pins[0]);
    }

    ok(pins[1] == (void *)0xdeed, "pins[1] = %p\n", pins[1]);

    ref = IEnumPins_Release(pin_enum);
    ok(ref == 0, "ref is %u and not 0!\n", ref);

    IBaseFilter_Release(base);
}

START_TEST(dsoundrender)
{
    CoInitialize(NULL);
    if (!create_dsound_renderer())
        return;

    test_property_bag();
    test_query_interface();
    test_basefilter();

    release_dsound_renderer();

    CoUninitialize();
}
