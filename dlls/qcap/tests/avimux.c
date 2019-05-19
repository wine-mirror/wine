/*
 * AVI muxer filter unit tests
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

static const GUID testguid = {0xfacade};

static IBaseFilter *create_avi_mux(void)
{
    IBaseFilter *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_AviDest, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    return filter;
}

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

static void test_interfaces(void)
{
    IBaseFilter *filter = create_avi_mux();

    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IConfigAviMux, TRUE);
    check_interface(filter, &IID_IConfigInterleaving, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IMediaSeeking, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IPersistMediaPropertyBag, TRUE);
    check_interface(filter, &IID_ISpecifyPropertyPages, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);

    check_interface(filter, &IID_IAMFilterMiscFlags, FALSE);
    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IKsPropertySet, FALSE);
    check_interface(filter, &IID_IMediaPosition, FALSE);
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IQualityControl, FALSE);
    check_interface(filter, &IID_IQualProp, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVideoWindow, FALSE);

    IBaseFilter_Release(filter);
}

static void test_seeking(void)
{
    IBaseFilter *filter = create_avi_mux();
    LONGLONG time, current, stop;
    IMediaSeeking *seeking;
    unsigned int i;
    GUID format;
    HRESULT hr;
    DWORD caps;
    ULONG ref;

    static const struct
    {
        const GUID *guid;
        HRESULT hr;
    }
    format_tests[] =
    {
        {&TIME_FORMAT_MEDIA_TIME, S_OK},
        {&TIME_FORMAT_BYTE, S_OK},

        {&TIME_FORMAT_NONE, S_FALSE},
        {&TIME_FORMAT_FRAME, S_FALSE},
        {&TIME_FORMAT_SAMPLE, S_FALSE},
        {&TIME_FORMAT_FIELD, S_FALSE},
        {&testguid, S_FALSE},
    };

    IBaseFilter_QueryInterface(filter, &IID_IMediaSeeking, (void **)&seeking);

    hr = IMediaSeeking_GetCapabilities(seeking, &caps);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(caps == (AM_SEEKING_CanGetCurrentPos | AM_SEEKING_CanGetDuration), "Got caps %#x.\n", caps);

    caps = AM_SEEKING_CanGetCurrentPos;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(caps == AM_SEEKING_CanGetCurrentPos, "Got caps %#x.\n", caps);

    caps = AM_SEEKING_CanDoSegments | AM_SEEKING_CanGetCurrentPos;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(caps == AM_SEEKING_CanGetCurrentPos, "Got caps %#x.\n", caps);

    caps = AM_SEEKING_CanDoSegments;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(!caps, "Got caps %#x.\n", caps);

    for (i = 0; i < ARRAY_SIZE(format_tests); ++i)
    {
        hr = IMediaSeeking_IsFormatSupported(seeking, format_tests[i].guid);
        todo_wine ok(hr == format_tests[i].hr, "Got hr %#x for format %s.\n", hr, wine_dbgstr_guid(format_tests[i].guid));
    }

    hr = IMediaSeeking_QueryPreferredFormat(seeking, &format);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_BYTE);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_SAMPLE);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_BYTE);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_QueryPreferredFormat(seeking, &format);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(IsEqualGUID(&format, &TIME_FORMAT_BYTE), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_BYTE);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_BYTE);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, NULL, 0x123456789a, NULL);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_MEDIA_TIME, 0x123456789a, &TIME_FORMAT_BYTE);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    current = 0x123;
    stop = 0x321;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_NoPositioning,
            &stop, AM_SEEKING_NoPositioning);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetPositions(seeking, NULL, NULL);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_GetPositions(seeking, NULL, &stop);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_GetPositions(seeking, &current, NULL);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetDuration(seeking, &time);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(!time, "Got duration %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(!time, "Got duration %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    IMediaSeeking_Release(seeking);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got unexpected refcount %d.\n", ref);
}

START_TEST(avimux)
{
    CoInitialize(NULL);

    test_interfaces();
    test_seeking();

    CoUninitialize();
}
