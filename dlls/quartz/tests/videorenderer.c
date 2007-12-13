/*
 * Unit tests for Video Renderer functions
 *
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

#include "wine/test.h"
#include "dshow.h"

#define QI_SUCCEED(iface, riid, ppv) hr = IUnknown_QueryInterface(iface, &riid, (LPVOID*)&ppv); \
    ok(hr == S_OK, "IUnknown_QueryInterface returned %x\n", hr); \
    ok(ppv != NULL, "Pointer is NULL\n");

#define QI_FAIL(iface, riid, ppv) hr = IUnknown_QueryInterface(iface, &riid, (LPVOID*)&ppv); \
    ok(hr == E_NOINTERFACE, "IUnknown_QueryInterface returned %x\n", hr); \
    ok(ppv == NULL, "Pointer is %p\n", ppv);

#define ADDREF_EXPECT(iface, num) if (iface) { \
    hr = IUnknown_AddRef(iface); \
    ok(hr == num, "IUnknown_AddRef should return %d, got %d\n", num, hr); \
}

#define RELEASE_EXPECT(iface, num) if (iface) { \
    hr = IUnknown_Release(iface); \
    ok(hr == num, "IUnknown_Release should return %d, got %d\n", num, hr); \
}

static IUnknown *pVideoRenderer = NULL;

static void test_aggregation(const CLSID clsidOuter, const CLSID clsidInner,
                             const IID iidOuter, const IID iidInner)
{
    HRESULT hr;
    IUnknown *pUnkOuter = NULL;
    IUnknown *pUnkInner = NULL;
    IUnknown *pUnkInnerFail = NULL;
    IUnknown *pUnkOuterTest = NULL;
    IUnknown *pUnkInnerTest = NULL;
    IUnknown *pUnkAggregatee = NULL;
    IUnknown *pUnkAggregator = NULL;
    IUnknown *pUnkTest = NULL;

    hr = CoCreateInstance(&clsidOuter, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (LPVOID*)&pUnkOuter);
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    ok(pUnkOuter != NULL, "pUnkOuter is NULL\n");

    if (!pUnkOuter)
    {
        skip("pUnkOuter is NULL\n");
        return;
    }

    /* for aggregation, we should only be able to request IUnknown */
    hr = CoCreateInstance(&clsidInner, pUnkOuter, CLSCTX_INPROC_SERVER,
                          &iidInner, (LPVOID*)&pUnkInnerFail);
    ok(hr == E_NOINTERFACE, "CoCreateInstance returned %x\n", hr);
    ok(pUnkInnerFail == NULL, "pUnkInnerFail is not NULL\n");

    /* aggregation, request IUnknown */
    hr = CoCreateInstance(&clsidInner, pUnkOuter, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (LPVOID*)&pUnkInner);
    ok(hr == S_OK, "CoCreateInstance returned %x\n", hr);
    ok(pUnkInner != NULL, "pUnkInner is NULL\n");

    if (!pUnkInner)
    {
        skip("pUnkInner is NULL\n");
        return;
    }

    ADDREF_EXPECT(pUnkOuter, 2);
    ADDREF_EXPECT(pUnkInner, 2);
    RELEASE_EXPECT(pUnkOuter, 1);
    RELEASE_EXPECT(pUnkInner, 1);

    QI_FAIL(pUnkOuter, iidInner, pUnkAggregatee);
    QI_FAIL(pUnkInner, iidOuter, pUnkAggregator);

    /* these QueryInterface calls should work */
    QI_SUCCEED(pUnkOuter, iidOuter, pUnkAggregator);
    QI_SUCCEED(pUnkOuter, IID_IUnknown, pUnkOuterTest);
    QI_SUCCEED(pUnkInner, iidInner, pUnkAggregatee);
    QI_SUCCEED(pUnkInner, IID_IUnknown, pUnkInnerTest);

    if (!pUnkAggregator || !pUnkOuterTest || !pUnkAggregatee \
                    || !pUnkInnerTest)
    {
        skip("One of the required interfaces is NULL\n");
        return;
    }

    ADDREF_EXPECT(pUnkAggregator, 5);
    ADDREF_EXPECT(pUnkOuterTest, 6);
    ADDREF_EXPECT(pUnkAggregatee, 7);
    ADDREF_EXPECT(pUnkInnerTest, 3);
    RELEASE_EXPECT(pUnkAggregator, 6);
    RELEASE_EXPECT(pUnkOuterTest, 5);
    RELEASE_EXPECT(pUnkAggregatee, 4);
    RELEASE_EXPECT(pUnkInnerTest, 2);

    QI_SUCCEED(pUnkAggregator, IID_IUnknown, pUnkTest);
    QI_SUCCEED(pUnkOuterTest, IID_IUnknown, pUnkTest);
    QI_SUCCEED(pUnkAggregatee, IID_IUnknown, pUnkTest);
    QI_SUCCEED(pUnkInnerTest, IID_IUnknown, pUnkTest);

    QI_FAIL(pUnkAggregator, iidInner, pUnkTest);
    QI_FAIL(pUnkOuterTest, iidInner, pUnkTest);
    QI_FAIL(pUnkAggregatee, iidInner, pUnkTest);
    QI_SUCCEED(pUnkInnerTest, iidInner, pUnkTest);

    QI_SUCCEED(pUnkAggregator, iidOuter, pUnkTest);
    QI_SUCCEED(pUnkOuterTest, iidOuter, pUnkTest);
    QI_SUCCEED(pUnkAggregatee, iidOuter, pUnkTest);
    QI_FAIL(pUnkInnerTest, iidOuter, pUnkTest);

    RELEASE_EXPECT(pUnkAggregator, 10);
    RELEASE_EXPECT(pUnkOuterTest, 9);
    RELEASE_EXPECT(pUnkAggregatee, 8);
    RELEASE_EXPECT(pUnkInnerTest, 2);
    RELEASE_EXPECT(pUnkOuter, 7);
    RELEASE_EXPECT(pUnkInner, 1);
}

static int create_video_renderer(void)
{
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (LPVOID*)&pVideoRenderer);
    return (hr == S_OK && pVideoRenderer != NULL);
}

static void release_video_renderer(void)
{
    HRESULT hr;

    hr = IUnknown_Release(pVideoRenderer);
    ok(hr == 0, "IUnknown_Release failed with %x\n", hr);
}

static void test_query_interface(void)
{
    HRESULT hr;
    IBaseFilter *pBaseFilter = NULL;
    IBasicVideo *pBasicVideo = NULL;
    IDirectDrawVideo *pDirectDrawVideo = NULL;
    IKsPropertySet *pKsPropertySet = NULL;
    IMediaPosition *pMediaPosition = NULL;
    IMediaSeeking *pMediaSeeking = NULL;
    IQualityControl *pQualityControl = NULL;
    IQualProp *pQualProp = NULL;
    IVideoWindow *pVideoWindow = NULL;

    QI_SUCCEED(pVideoRenderer, IID_IBaseFilter, pBaseFilter);
    RELEASE_EXPECT(pBaseFilter, 1);
    QI_SUCCEED(pVideoRenderer, IID_IBasicVideo, pBasicVideo);
    RELEASE_EXPECT(pBasicVideo, 1);
    todo_wine {
    QI_SUCCEED(pVideoRenderer, IID_IDirectDrawVideo, pDirectDrawVideo);
    RELEASE_EXPECT(pDirectDrawVideo, 1);
    QI_SUCCEED(pVideoRenderer, IID_IKsPropertySet, pKsPropertySet);
    RELEASE_EXPECT(pKsPropertySet, 1);
    QI_SUCCEED(pVideoRenderer, IID_IMediaPosition, pMediaPosition);
    RELEASE_EXPECT(pMediaPosition, 1);
    QI_SUCCEED(pVideoRenderer, IID_IMediaSeeking, pMediaSeeking);
    RELEASE_EXPECT(pMediaSeeking, 1);
    QI_SUCCEED(pVideoRenderer, IID_IQualityControl, pQualityControl);
    RELEASE_EXPECT(pQualityControl, 1);
    QI_SUCCEED(pVideoRenderer, IID_IQualProp, pQualProp);
    RELEASE_EXPECT(pQualProp, 1);
    }
    QI_SUCCEED(pVideoRenderer, IID_IVideoWindow, pVideoWindow);
    RELEASE_EXPECT(pVideoWindow, 1);
}

static void test_all_aggregations(void)
{
    IID iids[] = {
        IID_IMediaFilter, IID_IBaseFilter, IID_IBasicVideo, IID_IVideoWindow
    };
    int i;

    for (i = 0; i < sizeof(iids) / sizeof(iids[0]); i++)
    {
        test_aggregation(CLSID_SystemClock, CLSID_VideoRenderer,
                         IID_IReferenceClock, iids[i]);
    }
}

START_TEST(videorenderer)
{
    CoInitialize(NULL);
    if (!create_video_renderer())
        return;

    test_query_interface();
    test_all_aggregations();

    release_video_renderer();
}
