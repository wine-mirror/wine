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

#define ADDREF_EXPECT(iface, num) if (iface) { \
    hr = IUnknown_AddRef(iface); \
    ok(hr == num, "IUnknown_AddRef should return %d, got %d\n", num, hr); \
}

#define RELEASE_EXPECT(iface, num) if (iface) { \
    hr = IUnknown_Release(iface); \
    ok(hr == num, "IUnknown_Release should return %d, got %d\n", num, hr); \
}

static IUnknown *pVideoRenderer = NULL;

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

START_TEST(videorenderer)
{
    CoInitialize(NULL);
    if (!create_video_renderer())
        return;

    test_query_interface();

    release_video_renderer();
}
