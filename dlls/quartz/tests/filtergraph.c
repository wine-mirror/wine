/*
 * Unit tests for Direct Show functions
 *
 * Copyright (C) 2004 Christian Costa
 * Copyright (C) 2008 Alexander Dorofeyev
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

#include "dshow.h"
#include "wine/heap.h"
#include "wine/test.h"

static const GUID testguid = {0xabbccdde};

typedef struct TestFilterImpl
{
    IBaseFilter IBaseFilter_iface;

    LONG refCount;
    CRITICAL_SECTION csFilter;
    FILTER_STATE state;
    FILTER_INFO filterInfo;
    CLSID clsid;
    IPin **ppPins;
    UINT nPins;
} TestFilterImpl;

static BOOL compare_time(ULONGLONG x, ULONGLONG y, unsigned int max_diff)
{
    ULONGLONG diff = x > y ? x - y : y - x;
    return diff <= max_diff;
}

static WCHAR *create_file(const WCHAR *name, const char *data, DWORD size)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    wcscat(pathW, name);
    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create file %s, error %u.\n",
            wine_dbgstr_w(pathW), GetLastError());
    WriteFile(file, data, size, &written, NULL);
    ok(written = size, "Failed to write file data, error %u.\n", GetLastError());
    CloseHandle(file);

    return pathW;
}

static WCHAR *load_resource(const WCHAR *name)
{
    HRSRC res;
    void *ptr;

    res = FindResourceW(NULL, name, (const WCHAR *)RT_RCDATA);
    ok(!!res, "Failed to find resource %s, error %u.\n", wine_dbgstr_w(name), GetLastError());
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    return create_file(name, ptr, SizeofResource(GetModuleHandleA(NULL), res));
}

static IFilterGraph2 *create_graph(void)
{
    IFilterGraph2 *ret;
    HRESULT hr;
    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterGraph2, (void **)&ret);
    ok(hr == S_OK, "Failed to create FilterGraph: %#x\n", hr);
    return ret;
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
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
    IFilterGraph2 *graph = create_graph();

    check_interface(graph, &IID_IBasicAudio, TRUE);
    check_interface(graph, &IID_IBasicVideo2, TRUE);
    check_interface(graph, &IID_IFilterGraph2, TRUE);
    check_interface(graph, &IID_IFilterMapper, TRUE);
    check_interface(graph, &IID_IFilterMapper3, TRUE);
    check_interface(graph, &IID_IGraphConfig, TRUE);
    check_interface(graph, &IID_IGraphVersion, TRUE);
    check_interface(graph, &IID_IMediaControl, TRUE);
    check_interface(graph, &IID_IMediaEvent, TRUE);
    check_interface(graph, &IID_IMediaFilter, TRUE);
    check_interface(graph, &IID_IMediaEventSink, TRUE);
    check_interface(graph, &IID_IMediaPosition, TRUE);
    check_interface(graph, &IID_IMediaSeeking, TRUE);
    check_interface(graph, &IID_IObjectWithSite, TRUE);
    check_interface(graph, &IID_IVideoFrameStep, TRUE);
    check_interface(graph, &IID_IVideoWindow, TRUE);

    check_interface(graph, &IID_IBaseFilter, FALSE);

    IFilterGraph2_Release(graph);
}

static void test_basic_video(IFilterGraph2 *graph)
{
    IBasicVideo* pbv;
    LONG video_width, video_height, window_width;
    LONG left, top, width, height;
    HRESULT hr;

    hr = IFilterGraph2_QueryInterface(graph, &IID_IBasicVideo, (void **)&pbv);
    ok(hr==S_OK, "Cannot get IBasicVideo interface returned: %x\n", hr);

    /* test get video size */
    hr = IBasicVideo_GetVideoSize(pbv, NULL, NULL);
    ok(hr==E_POINTER, "IBasicVideo_GetVideoSize returned: %x\n", hr);
    hr = IBasicVideo_GetVideoSize(pbv, &video_width, NULL);
    ok(hr==E_POINTER, "IBasicVideo_GetVideoSize returned: %x\n", hr);
    hr = IBasicVideo_GetVideoSize(pbv, NULL, &video_height);
    ok(hr==E_POINTER, "IBasicVideo_GetVideoSize returned: %x\n", hr);
    hr = IBasicVideo_GetVideoSize(pbv, &video_width, &video_height);
    ok(hr==S_OK, "Cannot get video size returned: %x\n", hr);

    /* test source position */
    hr = IBasicVideo_GetSourcePosition(pbv, NULL, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IBasicVideo_GetSourcePosition returned: %x\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, &left, &top, NULL, NULL);
    ok(hr == E_POINTER, "IBasicVideo_GetSourcePosition returned: %x\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, NULL, NULL, &width, &height);
    ok(hr == E_POINTER, "IBasicVideo_GetSourcePosition returned: %x\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Cannot get source position returned: %x\n", hr);
    ok(left == 0, "expected 0, got %d\n", left);
    ok(top == 0, "expected 0, got %d\n", top);
    ok(width == video_width, "expected %d, got %d\n", video_width, width);
    ok(height == video_height, "expected %d, got %d\n", video_height, height);

    hr = IBasicVideo_SetSourcePosition(pbv, 0, 0, 0, 0);
    ok(hr==E_INVALIDARG, "IBasicVideo_SetSourcePosition returned: %x\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, 0, 0, video_width*2, video_height*2);
    ok(hr==E_INVALIDARG, "IBasicVideo_SetSourcePosition returned: %x\n", hr);
    hr = IBasicVideo_put_SourceTop(pbv, -1);
    ok(hr==E_INVALIDARG, "IBasicVideo_put_SourceTop returned: %x\n", hr);
    hr = IBasicVideo_put_SourceTop(pbv, 0);
    ok(hr==S_OK, "Cannot put source top returned: %x\n", hr);
    hr = IBasicVideo_put_SourceTop(pbv, 1);
    ok(hr==E_INVALIDARG, "IBasicVideo_put_SourceTop returned: %x\n", hr);

    hr = IBasicVideo_SetSourcePosition(pbv, video_width, 0, video_width, video_height);
    ok(hr==E_INVALIDARG, "IBasicVideo_SetSourcePosition returned: %x\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, 0, video_height, video_width, video_height);
    ok(hr==E_INVALIDARG, "IBasicVideo_SetSourcePosition returned: %x\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, -1, 0, video_width, video_height);
    ok(hr==E_INVALIDARG, "IBasicVideo_SetSourcePosition returned: %x\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, 0, -1, video_width, video_height);
    ok(hr==E_INVALIDARG, "IBasicVideo_SetSourcePosition returned: %x\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, video_width/2, video_height/2, video_width, video_height);
    ok(hr==E_INVALIDARG, "IBasicVideo_SetSourcePosition returned: %x\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, video_width/2, video_height/2, video_width, video_height);
    ok(hr==E_INVALIDARG, "IBasicVideo_SetSourcePosition returned: %x\n", hr);

    hr = IBasicVideo_SetSourcePosition(pbv, 0, 0, video_width, video_height+1);
    ok(hr==E_INVALIDARG, "IBasicVideo_SetSourcePosition returned: %x\n", hr);
    hr = IBasicVideo_SetSourcePosition(pbv, 0, 0, video_width+1, video_height);
    ok(hr==E_INVALIDARG, "IBasicVideo_SetSourcePosition returned: %x\n", hr);

    hr = IBasicVideo_SetSourcePosition(pbv, video_width/2, video_height/2, video_width/3+1, video_height/3+1);
    ok(hr==S_OK, "Cannot set source position returned: %x\n", hr);

    hr = IBasicVideo_get_SourceLeft(pbv, &left);
    ok(hr==S_OK, "Cannot get source left returned: %x\n", hr);
    ok(left==video_width/2, "expected %d, got %d\n", video_width/2, left);
    hr = IBasicVideo_get_SourceTop(pbv, &top);
    ok(hr==S_OK, "Cannot get source top returned: %x\n", hr);
    ok(top==video_height/2, "expected %d, got %d\n", video_height/2, top);
    hr = IBasicVideo_get_SourceWidth(pbv, &width);
    ok(hr==S_OK, "Cannot get source width returned: %x\n", hr);
    ok(width==video_width/3+1, "expected %d, got %d\n", video_width/3+1, width);
    hr = IBasicVideo_get_SourceHeight(pbv, &height);
    ok(hr==S_OK, "Cannot get source height returned: %x\n", hr);
    ok(height==video_height/3+1, "expected %d, got %d\n", video_height/3+1, height);

    hr = IBasicVideo_put_SourceLeft(pbv, video_width/3);
    ok(hr==S_OK, "Cannot put source left returned: %x\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Cannot get source position returned: %x\n", hr);
    ok(left == video_width/3, "expected %d, got %d\n", video_width/3, left);
    ok(width == video_width/3+1, "expected %d, got %d\n", video_width/3+1, width);

    hr = IBasicVideo_put_SourceTop(pbv, video_height/3);
    ok(hr==S_OK, "Cannot put source top returned: %x\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Cannot get source position returned: %x\n", hr);
    ok(top == video_height/3, "expected %d, got %d\n", video_height/3, top);
    ok(height == video_height/3+1, "expected %d, got %d\n", video_height/3+1, height);

    hr = IBasicVideo_put_SourceWidth(pbv, video_width/4+1);
    ok(hr==S_OK, "Cannot put source width returned: %x\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Cannot get source position returned: %x\n", hr);
    ok(left == video_width/3, "expected %d, got %d\n", video_width/3, left);
    ok(width == video_width/4+1, "expected %d, got %d\n", video_width/4+1, width);

    hr = IBasicVideo_put_SourceHeight(pbv, video_height/4+1);
    ok(hr==S_OK, "Cannot put source height returned: %x\n", hr);
    hr = IBasicVideo_GetSourcePosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Cannot get source position returned: %x\n", hr);
    ok(top == video_height/3, "expected %d, got %d\n", video_height/3, top);
    ok(height == video_height/4+1, "expected %d, got %d\n", video_height/4+1, height);

    /* test destination rectangle */
    window_width = max(video_width, GetSystemMetrics(SM_CXMIN) - 2 * GetSystemMetrics(SM_CXFRAME));

    hr = IBasicVideo_GetDestinationPosition(pbv, NULL, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IBasicVideo_GetDestinationPosition returned: %x\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, &left, &top, NULL, NULL);
    ok(hr == E_POINTER, "IBasicVideo_GetDestinationPosition returned: %x\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, NULL, NULL, &width, &height);
    ok(hr == E_POINTER, "IBasicVideo_GetDestinationPosition returned: %x\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Cannot get destination position returned: %x\n", hr);
    ok(left == 0, "expected 0, got %d\n", left);
    ok(top == 0, "expected 0, got %d\n", top);
    ok(width == window_width, "expected %d, got %d\n", window_width, width);
    ok(height == video_height, "expected %d, got %d\n", video_height, height);

    hr = IBasicVideo_SetDestinationPosition(pbv, 0, 0, 0, 0);
    ok(hr==E_INVALIDARG, "IBasicVideo_SetDestinationPosition returned: %x\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, 0, 0, video_width*2, video_height*2);
    ok(hr==S_OK, "Cannot put destination position returned: %x\n", hr);

    hr = IBasicVideo_put_DestinationLeft(pbv, -1);
    ok(hr==S_OK, "Cannot put destination left returned: %x\n", hr);
    hr = IBasicVideo_put_DestinationLeft(pbv, 0);
    ok(hr==S_OK, "Cannot put destination left returned: %x\n", hr);
    hr = IBasicVideo_put_DestinationLeft(pbv, 1);
    ok(hr==S_OK, "Cannot put destination left returned: %x\n", hr);

    hr = IBasicVideo_SetDestinationPosition(pbv, video_width, 0, video_width, video_height);
    ok(hr==S_OK, "Cannot set destinaiton position returned: %x\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, 0, video_height, video_width, video_height);
    ok(hr==S_OK, "Cannot set destinaiton position returned: %x\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, -1, 0, video_width, video_height);
    ok(hr==S_OK, "Cannot set destination position returned: %x\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, 0, -1, video_width, video_height);
    ok(hr==S_OK, "Cannot set destination position returned: %x\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, video_width/2, video_height/2, video_width, video_height);
    ok(hr==S_OK, "Cannot set destination position returned: %x\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, video_width/2, video_height/2, video_width, video_height);
    ok(hr==S_OK, "Cannot set destination position returned: %x\n", hr);

    hr = IBasicVideo_SetDestinationPosition(pbv, 0, 0, video_width, video_height+1);
    ok(hr==S_OK, "Cannot set destination position returned: %x\n", hr);
    hr = IBasicVideo_SetDestinationPosition(pbv, 0, 0, video_width+1, video_height);
    ok(hr==S_OK, "Cannot set destination position returned: %x\n", hr);

    hr = IBasicVideo_SetDestinationPosition(pbv, video_width/2, video_height/2, video_width/3+1, video_height/3+1);
    ok(hr==S_OK, "Cannot set destination position returned: %x\n", hr);

    hr = IBasicVideo_get_DestinationLeft(pbv, &left);
    ok(hr==S_OK, "Cannot get destination left returned: %x\n", hr);
    ok(left==video_width/2, "expected %d, got %d\n", video_width/2, left);
    hr = IBasicVideo_get_DestinationTop(pbv, &top);
    ok(hr==S_OK, "Cannot get destination top returned: %x\n", hr);
    ok(top==video_height/2, "expected %d, got %d\n", video_height/2, top);
    hr = IBasicVideo_get_DestinationWidth(pbv, &width);
    ok(hr==S_OK, "Cannot get destination width returned: %x\n", hr);
    ok(width==video_width/3+1, "expected %d, got %d\n", video_width/3+1, width);
    hr = IBasicVideo_get_DestinationHeight(pbv, &height);
    ok(hr==S_OK, "Cannot get destination height returned: %x\n", hr);
    ok(height==video_height/3+1, "expected %d, got %d\n", video_height/3+1, height);

    hr = IBasicVideo_put_DestinationLeft(pbv, video_width/3);
    ok(hr==S_OK, "Cannot put destination left returned: %x\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Cannot get source position returned: %x\n", hr);
    ok(left == video_width/3, "expected %d, got %d\n", video_width/3, left);
    ok(width == video_width/3+1, "expected %d, got %d\n", video_width/3+1, width);

    hr = IBasicVideo_put_DestinationTop(pbv, video_height/3);
    ok(hr==S_OK, "Cannot put destination top returned: %x\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Cannot get source position returned: %x\n", hr);
    ok(top == video_height/3, "expected %d, got %d\n", video_height/3, top);
    ok(height == video_height/3+1, "expected %d, got %d\n", video_height/3+1, height);

    hr = IBasicVideo_put_DestinationWidth(pbv, video_width/4+1);
    ok(hr==S_OK, "Cannot put destination width returned: %x\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Cannot get source position returned: %x\n", hr);
    ok(left == video_width/3, "expected %d, got %d\n", video_width/3, left);
    ok(width == video_width/4+1, "expected %d, got %d\n", video_width/4+1, width);

    hr = IBasicVideo_put_DestinationHeight(pbv, video_height/4+1);
    ok(hr==S_OK, "Cannot put destination height returned: %x\n", hr);
    hr = IBasicVideo_GetDestinationPosition(pbv, &left, &top, &width, &height);
    ok(hr == S_OK, "Cannot get source position returned: %x\n", hr);
    ok(top == video_height/3, "expected %d, got %d\n", video_height/3, top);
    ok(height == video_height/4+1, "expected %d, got %d\n", video_height/4+1, height);

    /* reset source rectangle */
    hr = IBasicVideo_SetDefaultSourcePosition(pbv);
    ok(hr==S_OK, "IBasicVideo_SetDefaultSourcePosition returned: %x\n", hr);

    /* reset destination position */
    hr = IBasicVideo_SetDestinationPosition(pbv, 0, 0, video_width, video_height);
    ok(hr==S_OK, "Cannot set destination position returned: %x\n", hr);

    IBasicVideo_Release(pbv);
}

static void test_media_seeking(IFilterGraph2 *graph)
{
    IMediaSeeking *seeking;
    IMediaFilter *filter;
    LONGLONG pos, stop, duration;
    GUID format;
    HRESULT hr;

    IFilterGraph2_SetDefaultSyncSource(graph);
    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaSeeking, (void **)&seeking);
    ok(hr == S_OK, "QueryInterface(IMediaControl) failed: %08x\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);
    ok(hr == S_OK, "QueryInterface(IMediaFilter) failed: %08x\n", hr);

    format = GUID_NULL;
    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "GetTimeFormat failed: %#x\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "got %s\n", wine_dbgstr_guid(&format));

    pos = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &pos, NULL, 0x123456789a, NULL);
    ok(hr == S_OK, "ConvertTimeFormat failed: %#x\n", hr);
    ok(pos == 0x123456789a, "got %s\n", wine_dbgstr_longlong(pos));

    pos = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &pos, &TIME_FORMAT_MEDIA_TIME, 0x123456789a, NULL);
    ok(hr == S_OK, "ConvertTimeFormat failed: %#x\n", hr);
    ok(pos == 0x123456789a, "got %s\n", wine_dbgstr_longlong(pos));

    pos = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &pos, NULL, 0x123456789a, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "ConvertTimeFormat failed: %#x\n", hr);
    ok(pos == 0x123456789a, "got %s\n", wine_dbgstr_longlong(pos));

    hr = IMediaSeeking_GetCurrentPosition(seeking, &pos);
    ok(hr == S_OK, "GetCurrentPosition failed: %#x\n", hr);
    ok(pos == 0, "got %s\n", wine_dbgstr_longlong(pos));

    hr = IMediaSeeking_GetDuration(seeking, &duration);
    ok(hr == S_OK, "GetDuration failed: %#x\n", hr);
    ok(duration > 0, "got %s\n", wine_dbgstr_longlong(duration));

    hr = IMediaSeeking_GetStopPosition(seeking, &stop);
    ok(hr == S_OK, "GetCurrentPosition failed: %08x\n", hr);
    ok(stop == duration || stop == duration + 1, "expected %s, got %s\n",
        wine_dbgstr_longlong(duration), wine_dbgstr_longlong(stop));

    hr = IMediaSeeking_SetPositions(seeking, NULL, AM_SEEKING_ReturnTime, NULL, AM_SEEKING_NoPositioning);
    ok(hr == S_OK, "SetPositions failed: %#x\n", hr);
    hr = IMediaSeeking_SetPositions(seeking, NULL, AM_SEEKING_NoPositioning, NULL, AM_SEEKING_ReturnTime);
    ok(hr == S_OK, "SetPositions failed: %#x\n", hr);

    pos = 0;
    hr = IMediaSeeking_SetPositions(seeking, &pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
    ok(hr == S_OK, "SetPositions failed: %08x\n", hr);

    IMediaFilter_SetSyncSource(filter, NULL);
    pos = 0xdeadbeef;
    hr = IMediaSeeking_GetCurrentPosition(seeking, &pos);
    ok(hr == S_OK, "GetCurrentPosition failed: %08x\n", hr);
    ok(pos == 0, "Position != 0 (%s)\n", wine_dbgstr_longlong(pos));
    IFilterGraph2_SetDefaultSyncSource(graph);

    IMediaSeeking_Release(seeking);
    IMediaFilter_Release(filter);
}

static void test_state_change(IFilterGraph2 *graph)
{
    IMediaControl *control;
    OAFilterState state;
    HRESULT hr;

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    ok(hr == S_OK, "QueryInterface(IMediaControl) failed: %x\n", hr);

    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "GetState() failed: %x\n", hr);
    ok(state == State_Stopped, "wrong state %d\n", state);

    hr = IMediaControl_Run(control);
    ok(SUCCEEDED(hr), "Run() failed: %x\n", hr);
    hr = IMediaControl_GetState(control, INFINITE, &state);
    ok(SUCCEEDED(hr), "GetState() failed: %x\n", hr);
    ok(state == State_Running, "wrong state %d\n", state);

    hr = IMediaControl_Stop(control);
    ok(SUCCEEDED(hr), "Stop() failed: %x\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "GetState() failed: %x\n", hr);
    ok(state == State_Stopped, "wrong state %d\n", state);

    hr = IMediaControl_Pause(control);
    ok(SUCCEEDED(hr), "Pause() failed: %x\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "GetState() failed: %x\n", hr);
    ok(state == State_Paused, "wrong state %d\n", state);

    hr = IMediaControl_Run(control);
    ok(SUCCEEDED(hr), "Run() failed: %x\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "GetState() failed: %x\n", hr);
    ok(state == State_Running, "wrong state %d\n", state);

    hr = IMediaControl_Pause(control);
    ok(SUCCEEDED(hr), "Pause() failed: %x\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "GetState() failed: %x\n", hr);
    ok(state == State_Paused, "wrong state %d\n", state);

    hr = IMediaControl_Stop(control);
    ok(SUCCEEDED(hr), "Stop() failed: %x\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "GetState() failed: %x\n", hr);
    ok(state == State_Stopped, "wrong state %d\n", state);

    IMediaControl_Release(control);
}

static void test_media_event(IFilterGraph2 *graph)
{
    IMediaEvent *media_event;
    IMediaSeeking *seeking;
    IMediaControl *control;
    IMediaFilter *filter;
    LONG_PTR lparam1, lparam2;
    LONGLONG current, stop;
    OAFilterState state;
    int got_eos = 0;
    HANDLE event;
    HRESULT hr;
    LONG code;

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);
    ok(hr == S_OK, "QueryInterface(IMediaFilter) failed: %#x\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    ok(hr == S_OK, "QueryInterface(IMediaControl) failed: %#x\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaEvent, (void **)&media_event);
    ok(hr == S_OK, "QueryInterface(IMediaEvent) failed: %#x\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IMediaSeeking, (void **)&seeking);
    ok(hr == S_OK, "QueryInterface(IMediaEvent) failed: %#x\n", hr);

    hr = IMediaControl_Stop(control);
    ok(SUCCEEDED(hr), "Stop() failed: %#x\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "GetState() timed out\n");

    hr = IMediaSeeking_GetDuration(seeking, &stop);
    ok(hr == S_OK, "GetDuration() failed: %#x\n", hr);
    current = 0;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning, &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "SetPositions() failed: %#x\n", hr);

    hr = IMediaFilter_SetSyncSource(filter, NULL);
    ok(hr == S_OK, "SetSyncSource() failed: %#x\n", hr);

    hr = IMediaEvent_GetEventHandle(media_event, (OAEVENT *)&event);
    ok(hr == S_OK, "GetEventHandle() failed: %#x\n", hr);

    /* flush existing events */
    while ((hr = IMediaEvent_GetEvent(media_event, &code, &lparam1, &lparam2, 0)) == S_OK);

    ok(WaitForSingleObject(event, 0) == WAIT_TIMEOUT, "event should not be signaled\n");

    hr = IMediaControl_Run(control);
    ok(SUCCEEDED(hr), "Run() failed: %#x\n", hr);

    while (!got_eos)
    {
        if (WaitForSingleObject(event, 5000) == WAIT_TIMEOUT)
            break;

        while ((hr = IMediaEvent_GetEvent(media_event, &code, &lparam1, &lparam2, 0)) == S_OK)
        {
            if (code == EC_COMPLETE)
            {
                got_eos = 1;
                break;
            }
        }
    }
    ok(got_eos, "didn't get EOS\n");

    hr = IMediaSeeking_GetCurrentPosition(seeking, &current);
    ok(hr == S_OK, "GetCurrentPosition() failed: %#x\n", hr);
    ok(current == stop, "expected %s, got %s\n", wine_dbgstr_longlong(stop), wine_dbgstr_longlong(current));

    hr = IMediaControl_Stop(control);
    ok(SUCCEEDED(hr), "Run() failed: %#x\n", hr);
    hr = IMediaControl_GetState(control, 1000, &state);
    ok(hr == S_OK, "GetState() timed out\n");

    hr = IFilterGraph2_SetDefaultSyncSource(graph);
    ok(hr == S_OK, "SetDefaultSinkSource() failed: %#x\n", hr);

    IMediaSeeking_Release(seeking);
    IMediaEvent_Release(media_event);
    IMediaControl_Release(control);
    IMediaFilter_Release(filter);
}

static void rungraph(IFilterGraph2 *graph, BOOL video)
{
    if (video)
        test_basic_video(graph);
    test_media_seeking(graph);
    test_state_change(graph);
    test_media_event(graph);
}

static HRESULT test_graph_builder_connect_file(WCHAR *filename, BOOL audio, BOOL video)
{
    IBaseFilter *source_filter, *renderer;
    IPin *pin_in, *pin_out;
    IFilterGraph2 *graph;
    IEnumPins *enumpins;
    HRESULT hr;

    if (video)
        hr = CoCreateInstance(&CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER,
                &IID_IBaseFilter, (void **)&renderer);
    else
        hr = CoCreateInstance(&CLSID_AudioRender, NULL, CLSCTX_INPROC_SERVER,
                &IID_IBaseFilter, (void **)&renderer);
    if (hr == VFW_E_NO_AUDIO_HARDWARE)
        return VFW_E_CANNOT_CONNECT;
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    graph = create_graph();

    IBaseFilter_EnumPins(renderer, &enumpins);
    IEnumPins_Next(enumpins, 1, &pin_in, NULL);
    IEnumPins_Release(enumpins);

    hr = IFilterGraph2_AddSourceFilter(graph, filename, NULL, &source_filter);
    ok(hr == S_OK, "AddSourceFilter failed: %#x\n", hr);

    hr = IFilterGraph2_AddFilter(graph, renderer, NULL);
    ok(hr == S_OK, "AddFilter failed: %#x\n", hr);

    hr = IBaseFilter_FindPin(source_filter, L"Output", &pin_out);
    ok(hr == S_OK, "FindPin failed: %#x\n", hr);
    hr = IFilterGraph2_Connect(graph, pin_out, pin_in);

    if (SUCCEEDED(hr))
        rungraph(graph, video);

    IPin_Release(pin_in);
    IPin_Release(pin_out);
    IBaseFilter_Release(source_filter);
    IBaseFilter_Release(renderer);
    IFilterGraph2_Release(graph);

    return hr;
}

static void test_render_run(const WCHAR *file, BOOL audio, BOOL video)
{
    IFilterGraph2 *graph;
    HANDLE h;
    HRESULT hr;
    LONG refs;
    WCHAR *filename = load_resource(file);

    h = CreateFileW(filename, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        skip("Could not read test file %s, skipping test\n", wine_dbgstr_w(file));
        DeleteFileW(filename);
        return;
    }
    CloseHandle(h);

    trace("running %s\n", wine_dbgstr_w(file));

    graph = create_graph();

    hr = IFilterGraph2_RenderFile(graph, filename, NULL);
    if (FAILED(hr))
    {
        skip("%s: codec not supported; skipping test\n", wine_dbgstr_w(file));

        refs = IFilterGraph2_Release(graph);
        ok(!refs, "Graph has %u references\n", refs);

        hr = test_graph_builder_connect_file(filename, audio, video);
        ok(hr == VFW_E_CANNOT_CONNECT, "got %#x\n", hr);
    }
    else
    {
        if (audio)
            ok(hr == S_OK || hr == VFW_S_AUDIO_NOT_RENDERED, "Got hr %#x.\n", hr);
        else
            ok(hr == S_OK, "Got hr %#x.\n", hr);
        rungraph(graph, video);

        refs = IFilterGraph2_Release(graph);
        ok(!refs, "Graph has %u references\n", refs);

        hr = test_graph_builder_connect_file(filename, audio, video);
        if (audio && video)
            todo_wine ok(hr == VFW_S_PARTIAL_RENDER, "Got hr %#x.\n", hr);
        else
            ok(hr == S_OK, "Got hr %#x.\n", hr);
    }

    /* check reference leaks */
    h = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(h != INVALID_HANDLE_VALUE, "CreateFile failed: err=%d\n", GetLastError());
    CloseHandle(h);

    DeleteFileW(filename);
}

static void test_enum_filters(void)
{
    IBaseFilter *filter1, *filter2, *filters[2];
    IFilterGraph2 *graph = create_graph();
    IEnumFilters *enum1, *enum2;
    ULONG count, ref;
    HRESULT hr;

    CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter1);
    CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter2);

    hr = IFilterGraph2_EnumFilters(graph, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumFilters_Next(enum1, 1, filters, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IFilterGraph2_AddFilter(graph, filter1, NULL);
    IFilterGraph2_AddFilter(graph, filter2, NULL);

    hr = IEnumFilters_Next(enum1, 1, filters, NULL);
    ok(hr == VFW_E_ENUM_OUT_OF_SYNC, "Got hr %#x.\n", hr);

    hr = IEnumFilters_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumFilters_Next(enum1, 1, filters, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(filters[0] == filter2, "Got filter %p.\n", filters[0]);
    IBaseFilter_Release(filters[0]);

    hr = IEnumFilters_Next(enum1, 1, filters, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    ok(filters[0] == filter1, "Got filter %p.\n", filters[0]);
    IBaseFilter_Release(filters[0]);

    hr = IEnumFilters_Next(enum1, 1, filters, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(count == 0, "Got count %u.\n", count);

    hr = IEnumFilters_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumFilters_Next(enum1, 2, filters, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 2, "Got count %u.\n", count);
    ok(filters[0] == filter2, "Got filter %p.\n", filters[0]);
    ok(filters[1] == filter1, "Got filter %p.\n", filters[1]);
    IBaseFilter_Release(filters[0]);
    IBaseFilter_Release(filters[1]);

    IFilterGraph2_RemoveFilter(graph, filter1);
    IFilterGraph2_AddFilter(graph, filter1, NULL);

    hr = IEnumFilters_Next(enum1, 2, filters, &count);
    ok(hr == VFW_E_ENUM_OUT_OF_SYNC, "Got hr %#x.\n", hr);

    hr = IEnumFilters_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumFilters_Next(enum1, 2, filters, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 2, "Got count %u.\n", count);
    ok(filters[0] == filter1, "Got filter %p.\n", filters[0]);
    ok(filters[1] == filter2, "Got filter %p.\n", filters[1]);
    IBaseFilter_Release(filters[0]);
    IBaseFilter_Release(filters[1]);

    hr = IEnumFilters_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumFilters_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumFilters_Skip(enum2, 1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumFilters_Next(enum2, 2, filters, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    ok(filters[0] == filter2, "Got filter %p.\n", filters[0]);
    IBaseFilter_Release(filters[0]);

    hr = IEnumFilters_Skip(enum1, 3);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumFilters_Release(enum2);
    IEnumFilters_Release(enum1);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static DWORD WINAPI call_RenderFile_multithread(LPVOID lParam)
{
    WCHAR *filename = load_resource(L"test.avi");
    IFilterGraph2 *graph = lParam;
    HRESULT hr;

    hr = IFilterGraph2_RenderFile(graph, filename, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    rungraph(graph, TRUE);

    return 0;
}

static void test_render_with_multithread(void)
{
    IFilterGraph2 *graph;
    HANDLE thread;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    graph = create_graph();

    thread = CreateThread(NULL, 0, call_RenderFile_multithread, graph, 0, NULL);

    ok(!WaitForSingleObject(thread, 10000), "Wait timed out.\n");
    IFilterGraph2_Release(graph);
    CloseHandle(thread);
    CoUninitialize();
}

struct testpin
{
    IPin IPin_iface;
    LONG ref;
    PIN_DIRECTION dir;
    struct testfilter *filter;
    IPin *peer;
    AM_MEDIA_TYPE *mt;
    WCHAR name[10];
    WCHAR id[10];

    IEnumMediaTypes IEnumMediaTypes_iface;
    const AM_MEDIA_TYPE *types;
    unsigned int type_count, enum_idx;
    AM_MEDIA_TYPE *request_mt, *accept_mt;
    const struct testpin *require_connected_pin;

    HRESULT Connect_hr;
    HRESULT EnumMediaTypes_hr;
    HRESULT QueryInternalConnections_hr;
};

struct testfilter
{
    IBaseFilter IBaseFilter_iface;
    LONG ref;
    IFilterGraph *graph;
    WCHAR *name;
    IReferenceClock *clock;

    IEnumPins IEnumPins_iface;
    struct testpin *pins;
    unsigned int pin_count, enum_idx;

    FILTER_STATE state;
    REFERENCE_TIME start_time;
    HRESULT state_hr, GetState_hr, seek_hr;
    FILTER_STATE expect_stop_prev, expect_run_prev;

    IAMFilterMiscFlags IAMFilterMiscFlags_iface;
    ULONG misc_flags;

    IMediaSeeking IMediaSeeking_iface;
    LONG seeking_ref;
    DWORD seek_caps;
    BOOL support_testguid, support_media_time;
    GUID time_format;
    LONGLONG seek_duration, seek_current, seek_stop;
    double seek_rate;

    IReferenceClock IReferenceClock_iface;

    IFileSourceFilter IFileSourceFilter_iface;
    WCHAR filename[MAX_PATH];
};

static inline struct testpin *impl_from_IEnumMediaTypes(IEnumMediaTypes *iface)
{
    return CONTAINING_RECORD(iface, struct testpin, IEnumMediaTypes_iface);
}

static HRESULT WINAPI testenummt_QueryInterface(IEnumMediaTypes *iface, REFIID iid, void **out)
{
    struct testpin *pin = impl_from_IEnumMediaTypes(iface);
    if (winetest_debug > 1) trace("%p->QueryInterface(%s)\n", pin, wine_dbgstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testenummt_AddRef(IEnumMediaTypes *iface)
{
    struct testpin *pin = impl_from_IEnumMediaTypes(iface);
    return InterlockedIncrement(&pin->ref);
}

static ULONG WINAPI testenummt_Release(IEnumMediaTypes *iface)
{
    struct testpin *pin = impl_from_IEnumMediaTypes(iface);
    return InterlockedDecrement(&pin->ref);
}

static HRESULT WINAPI testenummt_Next(IEnumMediaTypes *iface, ULONG count, AM_MEDIA_TYPE **out, ULONG *fetched)
{
    struct testpin *pin = impl_from_IEnumMediaTypes(iface);
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        if (pin->enum_idx + i >= pin->type_count)
            break;

        out[i] = CoTaskMemAlloc(sizeof(*out[i]));
        *out[i] = pin->types[pin->enum_idx + i];
    }

    if (fetched)
        *fetched = i;
    pin->enum_idx += i;

    return (i == count) ? S_OK : S_FALSE;
}

static HRESULT WINAPI testenummt_Skip(IEnumMediaTypes *iface, ULONG count)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testenummt_Reset(IEnumMediaTypes *iface)
{
    struct testpin *pin = impl_from_IEnumMediaTypes(iface);
    pin->enum_idx = 0;
    return S_OK;
}

static HRESULT WINAPI testenummt_Clone(IEnumMediaTypes *iface, IEnumMediaTypes **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IEnumMediaTypesVtbl testenummt_vtbl =
{
    testenummt_QueryInterface,
    testenummt_AddRef,
    testenummt_Release,
    testenummt_Next,
    testenummt_Skip,
    testenummt_Reset,
    testenummt_Clone,
};

static inline struct testpin *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, struct testpin, IPin_iface);
}

static HRESULT WINAPI testpin_QueryInterface(IPin *iface, REFIID iid, void **out)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->QueryInterface(%s)\n", pin, wine_dbgstr_guid(iid));

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IPin))
    {
        *out = &pin->IPin_iface;
        IPin_AddRef(*out);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testpin_AddRef(IPin *iface)
{
    struct testpin *pin = impl_from_IPin(iface);
    return InterlockedIncrement(&pin->ref);
}

static ULONG WINAPI testpin_Release(IPin *iface)
{
    struct testpin *pin = impl_from_IPin(iface);
    return InterlockedDecrement(&pin->ref);
}

static HRESULT WINAPI testpin_Disconnect(IPin *iface)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->Disconnect()\n", pin);

    if (!pin->peer)
        return S_FALSE;

    IPin_Release(pin->peer);
    pin->peer = NULL;
    return S_OK;
}

static HRESULT WINAPI testpin_ConnectedTo(IPin *iface, IPin **peer)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->ConnectedTo()\n", pin);

    *peer = pin->peer;
    if (*peer)
    {
        IPin_AddRef(*peer);
        return S_OK;
    }
    return VFW_E_NOT_CONNECTED;
}

static HRESULT WINAPI testpin_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpin_QueryPinInfo(IPin *iface, PIN_INFO *info)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->QueryPinInfo()\n", pin);

    info->pFilter = &pin->filter->IBaseFilter_iface;
    IBaseFilter_AddRef(info->pFilter);
    info->dir = pin->dir;
    wcscpy(info->achName, pin->name);
    return S_OK;
}


static HRESULT WINAPI testpin_QueryDirection(IPin *iface, PIN_DIRECTION *dir)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->QueryDirection()\n", pin);

    *dir = pin->dir;
    return S_OK;
}

static HRESULT WINAPI testpin_QueryId(IPin *iface, WCHAR **id)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->QueryId()\n", iface);
    *id = CoTaskMemAlloc(11);
    wcscpy(*id, pin->id);
    return S_OK;
}

static HRESULT WINAPI testpin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **out)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->EnumMediaTypes()\n", pin);

    if (FAILED(pin->EnumMediaTypes_hr))
        return pin->EnumMediaTypes_hr;

    *out = &pin->IEnumMediaTypes_iface;
    IEnumMediaTypes_AddRef(*out);
    pin->enum_idx = 0;
    return S_OK;
}

static HRESULT WINAPI testpin_QueryInternalConnections(IPin *iface, IPin **out, ULONG *count)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->QueryInternalConnections()\n", pin);

    *count = 0;
    return pin->QueryInternalConnections_hr;
}

static HRESULT WINAPI testpin_BeginFlush(IPin *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpin_EndFlush(IPin * iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpin_NewSegment(IPin *iface, REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testpin_EndOfStream(IPin *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI no_Connect(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI no_ReceiveConnection(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI no_EnumMediaTypes(IPin *iface, IEnumMediaTypes **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsink_ReceiveConnection(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->ReceiveConnection(%p)\n", pin, peer);

    if (pin->accept_mt && memcmp(pin->accept_mt, mt, sizeof(*mt)))
        return VFW_E_TYPE_NOT_ACCEPTED;

    if (pin->require_connected_pin && !pin->require_connected_pin->peer)
        return VFW_E_TYPE_NOT_ACCEPTED;

    pin->peer = peer;
    IPin_AddRef(peer);
    return S_OK;
}

static const IPinVtbl testsink_vtbl =
{
    testpin_QueryInterface,
    testpin_AddRef,
    testpin_Release,
    no_Connect,
    testsink_ReceiveConnection,
    testpin_Disconnect,
    testpin_ConnectedTo,
    testpin_ConnectionMediaType,
    testpin_QueryPinInfo,
    testpin_QueryDirection,
    testpin_QueryId,
    testpin_QueryAccept,
    no_EnumMediaTypes,
    testpin_QueryInternalConnections,
    testpin_EndOfStream,
    testpin_BeginFlush,
    testpin_EndFlush,
    testpin_NewSegment
};

static void testpin_init(struct testpin *pin, const IPinVtbl *vtbl, PIN_DIRECTION dir)
{
    memset(pin, 0, sizeof(*pin));
    pin->IPin_iface.lpVtbl = vtbl;
    pin->IEnumMediaTypes_iface.lpVtbl = &testenummt_vtbl;
    pin->ref = 1;
    pin->dir = dir;
    pin->Connect_hr = S_OK;
    pin->EnumMediaTypes_hr = S_OK;
    pin->QueryInternalConnections_hr = E_NOTIMPL;
}

static void testsink_init(struct testpin *pin)
{
    testpin_init(pin, &testsink_vtbl, PINDIR_INPUT);
}

static HRESULT WINAPI testsource_Connect(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct testpin *pin = impl_from_IPin(iface);
    HRESULT hr;
    if (winetest_debug > 1) trace("%p->Connect(%p)\n", pin, peer);

    if (FAILED(pin->Connect_hr))
        return pin->Connect_hr;

    if (pin->require_connected_pin && !pin->require_connected_pin->peer)
        return VFW_E_NO_ACCEPTABLE_TYPES;

    ok(!mt, "Got media type %p.\n", mt);

    if (SUCCEEDED(hr = IPin_ReceiveConnection(peer, &pin->IPin_iface, pin->request_mt)))
    {
        pin->peer = peer;
        IPin_AddRef(peer);
        return pin->Connect_hr;
    }
    return hr;
}

static const IPinVtbl testsource_vtbl =
{
    testpin_QueryInterface,
    testpin_AddRef,
    testpin_Release,
    testsource_Connect,
    no_ReceiveConnection,
    testpin_Disconnect,
    testpin_ConnectedTo,
    testpin_ConnectionMediaType,
    testpin_QueryPinInfo,
    testpin_QueryDirection,
    testpin_QueryId,
    testpin_QueryAccept,
    testpin_EnumMediaTypes,
    testpin_QueryInternalConnections,
    testpin_EndOfStream,
    testpin_BeginFlush,
    testpin_EndFlush,
    testpin_NewSegment
};

static void testsource_init(struct testpin *pin, const AM_MEDIA_TYPE *types, int type_count)
{
    testpin_init(pin, &testsource_vtbl, PINDIR_OUTPUT);
    pin->types = types;
    pin->type_count = type_count;
}

static inline struct testfilter *impl_from_IEnumPins(IEnumPins *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IEnumPins_iface);
}

static HRESULT WINAPI testenumpins_QueryInterface(IEnumPins *iface, REFIID iid, void **out)
{
    ok(0, "Unexpected iid %s.\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI testenumpins_AddRef(IEnumPins * iface)
{
    struct testfilter *filter = impl_from_IEnumPins(iface);
    return InterlockedIncrement(&filter->ref);
}

static ULONG WINAPI testenumpins_Release(IEnumPins * iface)
{
    struct testfilter *filter = impl_from_IEnumPins(iface);
    return InterlockedDecrement(&filter->ref);
}

static HRESULT WINAPI testenumpins_Next(IEnumPins *iface, ULONG count, IPin **out, ULONG *fetched)
{
    struct testfilter *filter = impl_from_IEnumPins(iface);
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        if (filter->enum_idx + i >= filter->pin_count)
            break;

        out[i] = &filter->pins[filter->enum_idx + i].IPin_iface;
        IPin_AddRef(out[i]);
    }

    if (fetched)
        *fetched = i;
    filter->enum_idx += i;

    return (i == count) ? S_OK : S_FALSE;
}

static HRESULT WINAPI testenumpins_Skip(IEnumPins *iface, ULONG count)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testenumpins_Reset(IEnumPins *iface)
{
    struct testfilter *filter = impl_from_IEnumPins(iface);
    filter->enum_idx = 0;
    return S_OK;
}

static HRESULT WINAPI testenumpins_Clone(IEnumPins *iface, IEnumPins **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IEnumPinsVtbl testenumpins_vtbl =
{
    testenumpins_QueryInterface,
    testenumpins_AddRef,
    testenumpins_Release,
    testenumpins_Next,
    testenumpins_Skip,
    testenumpins_Reset,
    testenumpins_Clone,
};

static inline struct testfilter *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IBaseFilter_iface);
}

static HRESULT WINAPI testfilter_QueryInterface(IBaseFilter *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->QueryInterface(%s)\n", filter, wine_dbgstr_guid(iid));

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IPersist)
            || IsEqualGUID(iid, &IID_IMediaFilter)
            || IsEqualGUID(iid, &IID_IBaseFilter))
    {
        *out = &filter->IBaseFilter_iface;
    }
    else if (IsEqualGUID(iid, &IID_IAMFilterMiscFlags) && filter->IAMFilterMiscFlags_iface.lpVtbl)
    {
        *out = &filter->IAMFilterMiscFlags_iface;
    }
    else if (IsEqualGUID(iid, &IID_IMediaSeeking) && filter->IMediaSeeking_iface.lpVtbl)
    {
        *out = &filter->IMediaSeeking_iface;
    }
    else if (IsEqualGUID(iid, &IID_IReferenceClock) && filter->IReferenceClock_iface.lpVtbl)
    {
        *out = &filter->IReferenceClock_iface;
    }
    else if (IsEqualGUID(iid, &IID_IFileSourceFilter) && filter->IFileSourceFilter_iface.lpVtbl)
    {
        *out = &filter->IFileSourceFilter_iface;
    }
    else
    {
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI testfilter_AddRef(IBaseFilter *iface)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    return InterlockedIncrement(&filter->ref);
}

static ULONG WINAPI testfilter_Release(IBaseFilter *iface)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    return InterlockedDecrement(&filter->ref);
}

static HRESULT WINAPI testfilter_GetClassID(IBaseFilter *iface, CLSID *clsid)
{
    if (winetest_debug > 1) trace("%p->GetClassID()\n", iface);
    memset(clsid, 0xde, sizeof(*clsid));
    return S_OK;
}

/* Downstream filters are always stopped before any filters they are connected
 * to upstream. Native actually implements this by topologically sorting filters
 * as they are connected. */
static void check_state_transition(struct testfilter *filter, FILTER_STATE expect)
{
    FILTER_STATE state;
    unsigned int i;
    PIN_INFO info;

    for (i = 0; i < filter->pin_count; ++i)
    {
        if (filter->pins[i].peer)
        {
            IPin_QueryPinInfo(filter->pins[i].peer, &info);
            IBaseFilter_GetState(info.pFilter, 0, &state);
            if (filter->pins[i].dir == PINDIR_OUTPUT)
                ok(state == expect, "Expected state %d for downstream filter %p, got %d.\n",
                        expect, info.pFilter, state);
            else
                ok(state == filter->state, "Expected state %d for upstream filter %p, got %d.\n",
                        filter->state, info.pFilter, state);
            IBaseFilter_Release(info.pFilter);
        }
    }
}

static HRESULT WINAPI testfilter_Stop(IBaseFilter *iface)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->Stop()\n", filter);

    todo_wine_if (filter->expect_stop_prev == State_Running)
        ok(filter->state == filter->expect_stop_prev, "Expected previous state %#x, got %#x.\n",
                filter->expect_stop_prev, filter->state);

    check_state_transition(filter, State_Stopped);

    filter->state = State_Stopped;
    return filter->state_hr;
}

static HRESULT WINAPI testfilter_Pause(IBaseFilter *iface)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->Pause()\n", filter);

    check_state_transition(filter, State_Paused);

    filter->state = State_Paused;
    return filter->state_hr;
}

static HRESULT WINAPI testfilter_Run(IBaseFilter *iface, REFERENCE_TIME start)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->Run(%s)\n", filter, wine_dbgstr_longlong(start));

    ok(filter->state == filter->expect_run_prev, "Expected previous state %#x, got %#x.\n",
            filter->expect_run_prev, filter->state);

    check_state_transition(filter, State_Running);

    filter->state = State_Running;
    filter->start_time = start;
    return filter->state_hr;
}

static HRESULT WINAPI testfilter_GetState(IBaseFilter *iface, DWORD timeout, FILTER_STATE *state)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->GetState(%u)\n", filter, timeout);

    *state = filter->state;
    return filter->GetState_hr;
}

static HRESULT WINAPI testfilter_SetSyncSource(IBaseFilter *iface, IReferenceClock *clock)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->SetSyncSource(%p)\n", filter, clock);

    if (filter->clock)
        IReferenceClock_Release(filter->clock);
    if (clock)
        IReferenceClock_AddRef(clock);
    filter->clock = clock;
    return S_OK;
}

static HRESULT WINAPI testfilter_GetSyncSource(IBaseFilter *iface, IReferenceClock **clock)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testfilter_EnumPins(IBaseFilter *iface, IEnumPins **out)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->EnumPins()\n", filter);

    *out = &filter->IEnumPins_iface;
    IEnumPins_AddRef(*out);
    filter->enum_idx = 0;
    return S_OK;
}

static HRESULT WINAPI testfilter_FindPin(IBaseFilter *iface, const WCHAR *id, IPin **pin)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testfilter_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *info)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->QueryFilterInfo()\n", filter);

    info->pGraph = filter->graph;
    if (filter->graph)
        IFilterGraph_AddRef(filter->graph);
    if (filter->name)
        wcscpy(info->achName, filter->name);
    else
        info->achName[0] = 0;
    return S_OK;
}

static HRESULT WINAPI testfilter_JoinFilterGraph(IBaseFilter *iface, IFilterGraph *graph, const WCHAR *name)
{
    struct testfilter *filter = impl_from_IBaseFilter(iface);
    if (winetest_debug > 1) trace("%p->JoinFilterGraph(%p, %s)\n", filter, graph, wine_dbgstr_w(name));

    filter->graph = graph;
    heap_free(filter->name);
    if (name)
    {
        filter->name = heap_alloc((wcslen(name) + 1) * sizeof(WCHAR));
        wcscpy(filter->name, name);
    }
    else
        filter->name = NULL;
    return S_OK;
}

static HRESULT WINAPI testfilter_QueryVendorInfo(IBaseFilter * iface, WCHAR **info)
{
    return E_NOTIMPL;
}

static const IBaseFilterVtbl testfilter_vtbl =
{
    testfilter_QueryInterface,
    testfilter_AddRef,
    testfilter_Release,
    testfilter_GetClassID,
    testfilter_Stop,
    testfilter_Pause,
    testfilter_Run,
    testfilter_GetState,
    testfilter_SetSyncSource,
    testfilter_GetSyncSource,
    testfilter_EnumPins,
    testfilter_FindPin,
    testfilter_QueryFilterInfo,
    testfilter_JoinFilterGraph,
    testfilter_QueryVendorInfo
};

static struct testfilter *impl_from_IAMFilterMiscFlags(IAMFilterMiscFlags *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IAMFilterMiscFlags_iface);
}

static HRESULT WINAPI testmiscflags_QueryInterface(IAMFilterMiscFlags *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IAMFilterMiscFlags(iface);
    return IBaseFilter_QueryInterface(&filter->IBaseFilter_iface, iid, out);
}

static ULONG WINAPI testmiscflags_AddRef(IAMFilterMiscFlags *iface)
{
    struct testfilter *filter = impl_from_IAMFilterMiscFlags(iface);
    return InterlockedIncrement(&filter->ref);
}

static ULONG WINAPI testmiscflags_Release(IAMFilterMiscFlags *iface)
{
    struct testfilter *filter = impl_from_IAMFilterMiscFlags(iface);
    return InterlockedDecrement(&filter->ref);
}

static ULONG WINAPI testmiscflags_GetMiscFlags(IAMFilterMiscFlags *iface)
{
    struct testfilter *filter = impl_from_IAMFilterMiscFlags(iface);
    if (winetest_debug > 1) trace("%p->GetMiscFlags()\n", filter);
    return filter->misc_flags;
}

static const IAMFilterMiscFlagsVtbl testmiscflags_vtbl =
{
    testmiscflags_QueryInterface,
    testmiscflags_AddRef,
    testmiscflags_Release,
    testmiscflags_GetMiscFlags,
};

static struct testfilter *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IMediaSeeking_iface);
}

static HRESULT WINAPI testseek_QueryInterface(IMediaSeeking *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    return IBaseFilter_QueryInterface(&filter->IBaseFilter_iface, iid, out);
}

static ULONG WINAPI testseek_AddRef(IMediaSeeking *iface)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    InterlockedIncrement(&filter->seeking_ref);
    return InterlockedIncrement(&filter->ref);
}

static ULONG WINAPI testseek_Release(IMediaSeeking *iface)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    InterlockedDecrement(&filter->seeking_ref);
    return InterlockedDecrement(&filter->ref);
}

static HRESULT WINAPI testseek_GetCapabilities(IMediaSeeking *iface, DWORD *caps)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->GetCapabilities()\n", iface);
    *caps = filter->seek_caps;
    return S_OK;
}

static HRESULT WINAPI testseek_CheckCapabilities(IMediaSeeking *iface, DWORD *caps)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_IsFormatSupported(IMediaSeeking *iface, const GUID *format)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->IsFormatSupported(%s)\n", iface, wine_dbgstr_guid(format));
    if (IsEqualGUID(format, &testguid) && !filter->support_testguid)
        return S_FALSE;
    if (IsEqualGUID(format, &TIME_FORMAT_MEDIA_TIME) && !filter->support_media_time)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI testseek_QueryPreferredFormat(IMediaSeeking *iface, GUID *format)
{
    if (winetest_debug > 1) trace("%p->QueryPreferredFormat()\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetTimeFormat(IMediaSeeking *iface, GUID *format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_IsUsingTimeFormat(IMediaSeeking *iface, const GUID *format)
{
    if (winetest_debug > 1) trace("%p->IsUsingTimeFormat(%s)\n", iface, wine_dbgstr_guid(format));
    return S_OK;
}

static HRESULT WINAPI testseek_SetTimeFormat(IMediaSeeking *iface, const GUID *format)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->SetTimeFormat(%s)\n", iface, wine_dbgstr_guid(format));
    ok(!IsEqualGUID(format, &GUID_NULL), "Got unexpected GUID_NULL.\n");
    if (IsEqualGUID(format, &testguid) && !filter->support_testguid)
        return E_INVALIDARG;
    filter->time_format = *format;
    return S_OK;
}

static HRESULT WINAPI testseek_GetDuration(IMediaSeeking *iface, LONGLONG *duration)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->GetDuration()\n", iface);
    *duration = filter->seek_duration;
    return S_OK;
}

static HRESULT WINAPI testseek_GetStopPosition(IMediaSeeking *iface, LONGLONG *stop)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->GetStopPosition()\n", iface);
    *stop = filter->seek_stop;
    return filter->seek_hr;
}

static HRESULT WINAPI testseek_GetCurrentPosition(IMediaSeeking *iface, LONGLONG *current)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->GetCurrentPosition()\n", iface);
    ok(!filter->clock, "GetCurrentPosition() should only be called if there is no sync source.\n");
    *current = 0xdeadbeef;
    return S_OK;
}

static HRESULT WINAPI testseek_ConvertTimeFormat(IMediaSeeking *iface, LONGLONG *target,
    const GUID *target_format, LONGLONG source, const GUID *source_format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_SetPositions(IMediaSeeking *iface, LONGLONG *current,
    DWORD current_flags, LONGLONG *stop, DWORD stop_flags )
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->SetPositions(%s, %#x, %s, %#x)\n",
            iface, wine_dbgstr_longlong(*current), current_flags, wine_dbgstr_longlong(*stop), stop_flags);
    ok(filter->state != State_Running, "Filter should be paused or stopped while seeking.\n");
    filter->seek_current = *current;
    filter->seek_stop = *stop;
    *current = 12340000;
    *stop = 43210000;
    return filter->seek_hr;
}

static HRESULT WINAPI testseek_GetPositions(IMediaSeeking *iface, LONGLONG *current, LONGLONG *stop)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetAvailable(IMediaSeeking *iface, LONGLONG *earliest, LONGLONG *latest)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_SetRate(IMediaSeeking *iface, double rate)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    if (winetest_debug > 1) trace("%p->SetRate(%.16e)\n", iface, rate);
    filter->seek_rate = rate;
    return S_OK;
}

static HRESULT WINAPI testseek_GetRate(IMediaSeeking *iface, double *rate)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testseek_GetPreroll(IMediaSeeking *iface, LONGLONG *preroll)
{
    if (winetest_debug > 1) trace("%p->GetPreroll()\n", iface);
    return E_NOTIMPL;
}

static const IMediaSeekingVtbl testseek_vtbl =
{
    testseek_QueryInterface,
    testseek_AddRef,
    testseek_Release,
    testseek_GetCapabilities,
    testseek_CheckCapabilities,
    testseek_IsFormatSupported,
    testseek_QueryPreferredFormat,
    testseek_GetTimeFormat,
    testseek_IsUsingTimeFormat,
    testseek_SetTimeFormat,
    testseek_GetDuration,
    testseek_GetStopPosition,
    testseek_GetCurrentPosition,
    testseek_ConvertTimeFormat,
    testseek_SetPositions,
    testseek_GetPositions,
    testseek_GetAvailable,
    testseek_SetRate,
    testseek_GetRate,
    testseek_GetPreroll,
};

static struct testfilter *impl_from_IReferenceClock(IReferenceClock *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IReferenceClock_iface);
}

static HRESULT WINAPI testclock_QueryInterface(IReferenceClock *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IReferenceClock(iface);
    return IBaseFilter_QueryInterface(&filter->IBaseFilter_iface, iid, out);
}

static ULONG WINAPI testclock_AddRef(IReferenceClock *iface)
{
    struct testfilter *filter = impl_from_IReferenceClock(iface);
    return InterlockedIncrement(&filter->ref);
}

static ULONG WINAPI testclock_Release(IReferenceClock *iface)
{
    struct testfilter *filter = impl_from_IReferenceClock(iface);
    return InterlockedDecrement(&filter->ref);
}

static HRESULT WINAPI testclock_GetTime(IReferenceClock *iface, REFERENCE_TIME *time)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testclock_AdviseTime(IReferenceClock *iface,
        REFERENCE_TIME base, REFERENCE_TIME offset, HEVENT event, DWORD_PTR *cookie)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testclock_AdvisePeriodic(IReferenceClock *iface,
        REFERENCE_TIME start, REFERENCE_TIME period, HSEMAPHORE semaphore, DWORD_PTR *cookie)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testclock_Unadvise(IReferenceClock *iface, DWORD_PTR cookie)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IReferenceClockVtbl testclock_vtbl =
{
    testclock_QueryInterface,
    testclock_AddRef,
    testclock_Release,
    testclock_GetTime,
    testclock_AdviseTime,
    testclock_AdvisePeriodic,
    testclock_Unadvise,
};

static struct testfilter *impl_from_IFileSourceFilter(IFileSourceFilter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IFileSourceFilter_iface);
}

static HRESULT WINAPI testfilesource_QueryInterface(IFileSourceFilter *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IFileSourceFilter(iface);
    return IBaseFilter_QueryInterface(&filter->IBaseFilter_iface, iid, out);
}

static ULONG WINAPI testfilesource_AddRef(IFileSourceFilter *iface)
{
    struct testfilter *filter = impl_from_IFileSourceFilter(iface);
    return InterlockedIncrement(&filter->ref);
}

static ULONG WINAPI testfilesource_Release(IFileSourceFilter *iface)
{
    struct testfilter *filter = impl_from_IFileSourceFilter(iface);
    return InterlockedDecrement(&filter->ref);
}

static HRESULT WINAPI testfilesource_Load(IFileSourceFilter *iface,
        const WCHAR *filename, const AM_MEDIA_TYPE *mt)
{
    struct testfilter *filter = impl_from_IFileSourceFilter(iface);
    if (winetest_debug > 1) trace("%p->Load()\n", iface);

    wcscpy(filter->filename, filename);

    return S_OK;
}

static HRESULT WINAPI testfilesource_GetCurFile(IFileSourceFilter *iface,
        WCHAR **filename, AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IFileSourceFilterVtbl testfilesource_vtbl =
{
    testfilesource_QueryInterface,
    testfilesource_AddRef,
    testfilesource_Release,
    testfilesource_Load,
    testfilesource_GetCurFile,
};

struct testfilter_cf
{
    IClassFactory IClassFactory_iface;
    struct testfilter *filter;
};

static void testfilter_init(struct testfilter *filter, struct testpin *pins, int pin_count)
{
    unsigned int i;

    memset(filter, 0, sizeof(*filter));
    filter->IBaseFilter_iface.lpVtbl = &testfilter_vtbl;
    filter->IEnumPins_iface.lpVtbl = &testenumpins_vtbl;
    filter->ref = 1;
    filter->pins = pins;
    filter->pin_count = pin_count;
    for (i = 0; i < pin_count; i++)
        pins[i].filter = filter;

    filter->state = State_Stopped;
    filter->expect_stop_prev = filter->expect_run_prev = State_Paused;
    filter->support_media_time = TRUE;
    filter->time_format = TIME_FORMAT_MEDIA_TIME;
}

static HRESULT WINAPI testfilter_cf_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testfilter_cf_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI testfilter_cf_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI testfilter_cf_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **out)
{
    struct testfilter_cf *factory = CONTAINING_RECORD(iface, struct testfilter_cf, IClassFactory_iface);

    return IBaseFilter_QueryInterface(&factory->filter->IBaseFilter_iface, iid, out);
}

static HRESULT WINAPI testfilter_cf_LockServer(IClassFactory *iface, BOOL lock)
{
    return E_NOTIMPL;
}

static IClassFactoryVtbl testfilter_cf_vtbl =
{
    testfilter_cf_QueryInterface,
    testfilter_cf_AddRef,
    testfilter_cf_Release,
    testfilter_cf_CreateInstance,
    testfilter_cf_LockServer,
};

static void test_graph_builder_render(void)
{
    static const GUID sink1_clsid = {0x12345678};
    static const GUID sink2_clsid = {0x87654321};
    AM_MEDIA_TYPE source_type = {{0}};
    struct testpin source_pin, sink1_pin, sink2_pin, parser_pins[2];
    struct testfilter source, sink1, sink2, parser;
    struct testfilter_cf sink1_cf = { {&testfilter_cf_vtbl}, &sink1 };
    struct testfilter_cf sink2_cf = { {&testfilter_cf_vtbl}, &sink2 };

    IFilterGraph2 *graph = create_graph();
    REGFILTERPINS2 regpins = {0};
    REGPINTYPES regtypes = {0};
    REGFILTER2 regfilter = {0};
    IFilterMapper2 *mapper;
    DWORD cookie1, cookie2;
    HRESULT hr;
    ULONG ref;

    memset(&source_type.majortype, 0xcc, sizeof(GUID));
    testsource_init(&source_pin, &source_type, 1);
    testfilter_init(&source, &source_pin, 1);
    testsink_init(&sink1_pin);
    testfilter_init(&sink1, &sink1_pin, 1);
    testsink_init(&sink2_pin);
    testfilter_init(&sink2, &sink2_pin, 1);
    testsink_init(&parser_pins[0]);
    testsource_init(&parser_pins[1], &source_type, 1);
    testfilter_init(&parser, parser_pins, 2);

    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink2.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink2_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);

    IFilterGraph2_RemoveFilter(graph, &sink1.IBaseFilter_iface);
    IFilterGraph2_AddFilter(graph, &sink1.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink1_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);

    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, &sink1_pin.IPin_iface);

    /* No preference is given to smaller chains. */

    IFilterGraph2_AddFilter(graph, &parser.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(parser_pins[1].peer == &sink1_pin.IPin_iface, "Got peer %p.\n", parser_pins[1].peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, parser_pins[0].peer);
    IFilterGraph2_Disconnect(graph, &parser_pins[0].IPin_iface);

    IFilterGraph2_RemoveFilter(graph, &sink1.IBaseFilter_iface);
    IFilterGraph2_AddFilter(graph, &sink1.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink1_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);

    /* A pin whose name (not ID) begins with a tilde is not rendered. */

    IFilterGraph2_RemoveFilter(graph, &sink2.IBaseFilter_iface);
    IFilterGraph2_RemoveFilter(graph, &parser.IBaseFilter_iface);
    IFilterGraph2_AddFilter(graph, &parser.IBaseFilter_iface, NULL);

    parser_pins[1].name[0] = '~';
    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!parser_pins[1].peer, "Got peer %p.\n", parser_pins[1].peer);
    ok(!sink1_pin.peer, "Got peer %p.\n", sink1_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);

    parser_pins[1].name[0] = 0;
    parser_pins[1].id[0] = '~';
    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(parser_pins[1].peer == &sink1_pin.IPin_iface, "Got peer %p.\n", parser_pins[1].peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    /* Test enumeration of filters from the registry. */

    CoRegisterClassObject(&sink1_clsid, (IUnknown *)&sink1_cf.IClassFactory_iface,
            CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie1);
    CoRegisterClassObject(&sink2_clsid, (IUnknown *)&sink2_cf.IClassFactory_iface,
            CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie2);

    CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper);

    regfilter.dwVersion = 2;
    regfilter.dwMerit = MERIT_UNLIKELY;
    regfilter.cPins2 = 1;
    regfilter.rgPins2 = &regpins;
    regpins.dwFlags = 0;
    regpins.cInstances = 1;
    regpins.nMediaTypes = 1;
    regpins.lpMediaType = &regtypes;
    regtypes.clsMajorType = &source_type.majortype;
    regtypes.clsMinorType = &MEDIASUBTYPE_NULL;
    hr = IFilterMapper2_RegisterFilter(mapper, &sink1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    if (hr == E_ACCESSDENIED)
    {
        skip("Not enough permission to register filters.\n");
        goto out;
    }
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);

    regpins.dwFlags = REG_PINFLAG_B_RENDERER;
    IFilterMapper2_RegisterFilter(mapper, &sink2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink2_pin.IPin_iface || source_pin.peer == &sink1_pin.IPin_iface,
            "Got peer %p.\n", source_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    /* Preference is given to filters already in the graph. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink2.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink2_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    /* No preference is given to renderer filters. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink2_clsid);

    IFilterMapper2_RegisterFilter(mapper, &sink1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    regpins.dwFlags = 0;
    IFilterMapper2_RegisterFilter(mapper, &sink2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink2_pin.IPin_iface || source_pin.peer == &sink1_pin.IPin_iface,
            "Got peer %p.\n", source_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    /* Preference is given to filters with higher merit. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink2_clsid);

    regfilter.dwMerit = MERIT_UNLIKELY;
    IFilterMapper2_RegisterFilter(mapper, &sink1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    regfilter.dwMerit = MERIT_PREFERRED;
    IFilterMapper2_RegisterFilter(mapper, &sink2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink2_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink2_clsid);

    regfilter.dwMerit = MERIT_PREFERRED;
    IFilterMapper2_RegisterFilter(mapper, &sink1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    regfilter.dwMerit = MERIT_UNLIKELY;
    IFilterMapper2_RegisterFilter(mapper, &sink2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink1_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    /* Test AM_RENDEREX_RENDERTOEXISTINGRENDERERS. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_RenderEx(graph, &source_pin.IPin_iface, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL);
    ok(hr == VFW_E_CANNOT_RENDER, "Got hr %#x.\n", hr);

    IFilterGraph2_AddFilter(graph, &sink1.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_RenderEx(graph, &source_pin.IPin_iface, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink1_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &sink2_clsid);

out:
    CoRevokeClassObject(cookie1);
    CoRevokeClassObject(cookie2);
    IFilterMapper2_Release(mapper);
    ok(source.ref == 1, "Got outstanding refcount %d.\n", source.ref);
    ok(source_pin.ref == 1, "Got outstanding refcount %d.\n", source_pin.ref);
    ok(sink1.ref == 1, "Got outstanding refcount %d.\n", sink1.ref);
    ok(sink1_pin.ref == 1, "Got outstanding refcount %d.\n", sink1_pin.ref);
    ok(sink2.ref == 1, "Got outstanding refcount %d.\n", sink2.ref);
    ok(sink2_pin.ref == 1, "Got outstanding refcount %d.\n", sink2_pin.ref);
    ok(parser.ref == 1, "Got outstanding refcount %d.\n", parser.ref);
    ok(parser_pins[0].ref == 1, "Got outstanding refcount %d.\n", parser_pins[0].ref);
    ok(parser_pins[1].ref == 1, "Got outstanding refcount %d.\n", parser_pins[1].ref);
}

static void test_graph_builder_connect(void)
{
    static const GUID parser1_clsid = {0x12345678};
    static const GUID parser2_clsid = {0x87654321};
    AM_MEDIA_TYPE source_types[2] = {{{0}}}, sink_type = {{0}}, parser3_type = {{0}};
    struct testpin source_pin, sink_pin, sink2_pin, parser1_pins[3], parser2_pins[2], parser3_pins[2];
    struct testfilter source, sink, sink2, parser1, parser2, parser3;
    struct testfilter_cf parser1_cf = { {&testfilter_cf_vtbl}, &parser1 };
    struct testfilter_cf parser2_cf = { {&testfilter_cf_vtbl}, &parser2 };

    IFilterGraph2 *graph = create_graph();
    REGFILTERPINS2 regpins[2] = {{0}};
    REGPINTYPES regtypes = {0};
    REGFILTER2 regfilter = {0};
    IFilterMapper2 *mapper;
    DWORD cookie1, cookie2;
    HRESULT hr;
    ULONG ref;

    memset(&source_types[0].majortype, 0xcc, sizeof(GUID));
    memset(&source_types[1].majortype, 0xdd, sizeof(GUID));
    memset(&sink_type.majortype, 0x66, sizeof(GUID));
    testsource_init(&source_pin, source_types, 2);
    source_pin.request_mt = &source_types[1];
    testfilter_init(&source, &source_pin, 1);
    testsink_init(&sink_pin);
    testfilter_init(&sink, &sink_pin, 1);
    testsink_init(&sink2_pin);
    testfilter_init(&sink2, &sink2_pin, 1);

    testsink_init(&parser1_pins[0]);
    testsource_init(&parser1_pins[1], &sink_type, 1);
    parser1_pins[1].request_mt = &sink_type;
    testsource_init(&parser1_pins[2], &sink_type, 1);
    parser1_pins[2].request_mt = &sink_type;
    testfilter_init(&parser1, parser1_pins, 3);
    parser1.pin_count = 2;

    testsource_init(&parser2_pins[0], &sink_type, 1);
    testsink_init(&parser2_pins[1]);
    parser2_pins[0].request_mt = &sink_type;
    testfilter_init(&parser2, parser2_pins, 2);

    testsink_init(&parser3_pins[0]);
    testsource_init(&parser3_pins[1], &sink_type, 1);
    parser3_pins[1].request_mt = &parser3_type;
    testfilter_init(&parser3, parser3_pins, 2);

    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);

    for (source_pin.Connect_hr = 0x00040200; source_pin.Connect_hr <= 0x000402ff;
            ++source_pin.Connect_hr)
    {
        hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
        ok(hr == source_pin.Connect_hr, "Got hr %#x for Connect() hr %#x.\n",
                hr, source_pin.Connect_hr);
        ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
        IFilterGraph2_Disconnect(graph, source_pin.peer);
        IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    }
    source_pin.Connect_hr = S_OK;

    sink_pin.accept_mt = &sink_type;
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#x.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);

    for (source_pin.Connect_hr = 0x80040200; source_pin.Connect_hr <= 0x800402ff;
            ++source_pin.Connect_hr)
    {
        hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
        if (source_pin.Connect_hr == VFW_E_NOT_CONNECTED
                || source_pin.Connect_hr == VFW_E_NO_AUDIO_HARDWARE)
            ok(hr == source_pin.Connect_hr, "Got hr %#x for Connect() hr %#x.\n",
                    hr, source_pin.Connect_hr);
        else
            ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#x for Connect() hr %#x.\n",
                    hr, source_pin.Connect_hr);
        ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
        ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    }
    source_pin.Connect_hr = S_OK;

    for (source_pin.EnumMediaTypes_hr = 0x80040200; source_pin.EnumMediaTypes_hr <= 0x800402ff;
            ++source_pin.EnumMediaTypes_hr)
    {
        hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
        ok(hr == source_pin.EnumMediaTypes_hr, "Got hr %#x for EnumMediaTypes() hr %#x.\n",
                hr, source_pin.EnumMediaTypes_hr);
        ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
        ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    }
    source_pin.EnumMediaTypes_hr = S_OK;

    /* Test usage of intermediate filters. Similarly to Render(), filters are
     * simply tried in enumeration order. */

    IFilterGraph2_AddFilter(graph, &parser1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &parser2.IBaseFilter_iface, NULL);

    sink_pin.accept_mt = NULL;
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);

    sink_pin.accept_mt = &sink_type;
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser2_pins[1].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser2_pins[0].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);

    for (source_pin.Connect_hr = 0x00040200; source_pin.Connect_hr <= 0x000402ff;
            ++source_pin.Connect_hr)
    {
        hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
        ok(hr == S_OK, "Got hr %#x for Connect() hr %#x.\n", hr, source_pin.Connect_hr);
        ok(source_pin.peer == &parser2_pins[1].IPin_iface, "Got peer %p.\n", source_pin.peer);
        ok(sink_pin.peer == &parser2_pins[0].IPin_iface, "Got peer %p.\n", sink_pin.peer);
        IFilterGraph2_Disconnect(graph, source_pin.peer);
        IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
        IFilterGraph2_Disconnect(graph, sink_pin.peer);
        IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    }
    source_pin.Connect_hr = S_OK;

    IFilterGraph2_RemoveFilter(graph, &parser1.IBaseFilter_iface);
    IFilterGraph2_AddFilter(graph, &parser1.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);

    /* No preference is given to smaller chains. */

    IFilterGraph2_AddFilter(graph, &parser3.IBaseFilter_iface, NULL);
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser3_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(parser3_pins[1].peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", parser3_pins[1].peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, parser3_pins[0].peer);
    IFilterGraph2_Disconnect(graph, &parser3_pins[0].IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);

    IFilterGraph2_RemoveFilter(graph, &parser3.IBaseFilter_iface);
    IFilterGraph2_RemoveFilter(graph, &parser2.IBaseFilter_iface);

    /* Extra source pins on an intermediate filter are not rendered. */

    IFilterGraph2_RemoveFilter(graph, &parser1.IBaseFilter_iface);
    parser1.pin_count = 3;
    IFilterGraph2_AddFilter(graph, &parser1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink2.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
todo_wine
    ok(hr == VFW_S_PARTIAL_RENDER, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    ok(!parser1_pins[2].peer, "Got peer %p.\n", parser1_pins[2].peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    parser1.pin_count = 2;

    /* QueryInternalConnections is not used to find output pins. */

    parser1_pins[1].QueryInternalConnections_hr = S_OK;
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    parser1_pins[1].QueryInternalConnections_hr = E_NOTIMPL;

    /* A pin whose name (not ID) begins with a tilde is not connected. */

    parser1_pins[1].name[0] = '~';
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#x.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);

    parser1.pin_count = 3;
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[2].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    parser1.pin_count = 2;

    parser1_pins[1].name[0] = 0;
    parser1_pins[1].id[0] = '~';
    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);
    IFilterGraph2_Disconnect(graph, source_pin.peer);
    IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    IFilterGraph2_Disconnect(graph, sink_pin.peer);
    IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);

    hr = IFilterGraph2_Connect(graph, &parser1_pins[1].IPin_iface, &parser1_pins[0].IPin_iface);
    ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#x.\n", hr);

    parser1_pins[0].QueryInternalConnections_hr = S_OK;
    hr = IFilterGraph2_Connect(graph, &parser1_pins[1].IPin_iface, &parser1_pins[0].IPin_iface);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    parser1_pins[0].QueryInternalConnections_hr = E_NOTIMPL;

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    /* The graph connects from source to sink, not from sink to source. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, L"source");
    IFilterGraph2_AddFilter(graph, &parser1.IBaseFilter_iface, L"parser");
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, L"sink");

    parser1_pins[0].require_connected_pin = &parser1_pins[1];

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#x.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    parser1_pins[0].require_connected_pin = NULL;
    parser1_pins[1].require_connected_pin = &parser1_pins[0];

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);

    parser1_pins[1].require_connected_pin = NULL;

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    /* Test enumeration of filters from the registry. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);

    CoRegisterClassObject(&parser1_clsid, (IUnknown *)&parser1_cf.IClassFactory_iface,
            CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie1);
    CoRegisterClassObject(&parser2_clsid, (IUnknown *)&parser2_cf.IClassFactory_iface,
            CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie2);

    CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper);

    regfilter.dwVersion = 2;
    regfilter.dwMerit = MERIT_UNLIKELY;
    regfilter.cPins2 = 2;
    regfilter.rgPins2 = regpins;
    regpins[0].dwFlags = 0;
    regpins[0].cInstances = 1;
    regpins[0].nMediaTypes = 1;
    regpins[0].lpMediaType = &regtypes;
    regpins[1].dwFlags = REG_PINFLAG_B_OUTPUT;
    regpins[1].cInstances = 1;
    regpins[1].nMediaTypes = 1;
    regpins[1].lpMediaType = &regtypes;
    regtypes.clsMajorType = &source_types[1].majortype;
    regtypes.clsMinorType = &MEDIASUBTYPE_NULL;
    hr = IFilterMapper2_RegisterFilter(mapper, &parser1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    if (hr == E_ACCESSDENIED)
    {
        skip("Not enough permission to register filters.\n");
        goto out;
    }
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    IFilterMapper2_RegisterFilter(mapper, &parser2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface
            || source_pin.peer == &parser2_pins[1].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface
            || sink_pin.peer == &parser2_pins[0].IPin_iface, "Got peer %p.\n", sink_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    /* Preference is given to filters already in the graph. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &parser1.IBaseFilter_iface, NULL);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    /* Preference is given to filters with higher merit. */

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &parser1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &parser2_clsid);

    regfilter.dwMerit = MERIT_UNLIKELY;
    IFilterMapper2_RegisterFilter(mapper, &parser1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    regfilter.dwMerit = MERIT_PREFERRED;
    IFilterMapper2_RegisterFilter(mapper, &parser2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser2_pins[1].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser2_pins[0].IPin_iface, "Got peer %p.\n", sink_pin.peer);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    graph = create_graph();
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &parser1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &parser2_clsid);

    regfilter.dwMerit = MERIT_PREFERRED;
    IFilterMapper2_RegisterFilter(mapper, &parser1_clsid, L"test", NULL, NULL, NULL, &regfilter);
    regfilter.dwMerit = MERIT_UNLIKELY;
    IFilterMapper2_RegisterFilter(mapper, &parser2_clsid, L"test", NULL, NULL, NULL, &regfilter);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(sink_pin.peer == &parser1_pins[1].IPin_iface, "Got peer %p.\n", sink_pin.peer);

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &parser1_clsid);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &parser2_clsid);

out:
    CoRevokeClassObject(cookie1);
    CoRevokeClassObject(cookie2);
    IFilterMapper2_Release(mapper);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ok(source.ref == 1, "Got outstanding refcount %d.\n", source.ref);
    ok(source_pin.ref == 1, "Got outstanding refcount %d.\n", source_pin.ref);
    ok(sink.ref == 1, "Got outstanding refcount %d.\n", sink.ref);
    ok(sink_pin.ref == 1, "Got outstanding refcount %d.\n", sink_pin.ref);
    ok(parser1.ref == 1, "Got outstanding refcount %d.\n", parser1.ref);
    ok(parser1_pins[0].ref == 1, "Got outstanding refcount %d.\n", parser1_pins[0].ref);
    ok(parser1_pins[1].ref == 1, "Got outstanding refcount %d.\n", parser1_pins[1].ref);
    ok(parser1_pins[2].ref == 1, "Got outstanding refcount %d.\n", parser1_pins[2].ref);
    ok(parser2.ref == 1, "Got outstanding refcount %d.\n", parser2.ref);
    ok(parser2_pins[0].ref == 1, "Got outstanding refcount %d.\n", parser2_pins[0].ref);
    ok(parser2_pins[1].ref == 1, "Got outstanding refcount %d.\n", parser2_pins[1].ref);
    ok(parser3.ref == 1, "Got outstanding refcount %d.\n", parser3.ref);
    ok(parser3_pins[0].ref == 1, "Got outstanding refcount %d.\n", parser3_pins[0].ref);
    ok(parser3_pins[1].ref == 1, "Got outstanding refcount %d.\n", parser3_pins[1].ref);
}

static const GUID test_iid = {0x33333333};
static LONG outer_ref = 1;

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IFilterGraph2)
            || IsEqualGUID(iid, &test_iid))
    {
        *out = (IUnknown *)0xdeadbeef;
        return S_OK;
    }
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef(IUnknown *iface)
{
    return InterlockedIncrement(&outer_ref);
}

static ULONG WINAPI outer_Release(IUnknown *iface)
{
    return InterlockedDecrement(&outer_ref);
}

static const IUnknownVtbl outer_vtbl =
{
    outer_QueryInterface,
    outer_AddRef,
    outer_Release,
};

static IUnknown test_outer = {&outer_vtbl};

static void test_aggregation(void)
{
    IFilterGraph2 *graph, *graph2;
    IFilterMapper2 *mapper;
    IUnknown *unk, *unk2;
    HRESULT hr;
    ULONG ref;

    graph = (IFilterGraph2 *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_FilterGraph, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!graph, "Got interface %p.\n", graph);

    hr = CoCreateInstance(&CLSID_FilterGraph, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);
    ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
    ref = get_refcount(unk);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    ref = IUnknown_AddRef(unk);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);

    ref = IUnknown_Release(unk);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == unk, "Got unexpected IUnknown %p.\n", unk2);
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IFilterGraph2, (void **)&graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IFilterGraph2, (void **)&graph2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph2 == (IFilterGraph2 *)0xdeadbeef, "Got unexpected IFilterGraph2 %p.\n", graph2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IFilterGraph2_QueryInterface(graph, &test_iid, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    IFilterGraph2_Release(graph);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %d.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);

    /* Test the aggregated filter mapper. */

    graph = create_graph();

    ref = get_refcount(graph);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    hr = IFilterGraph2_QueryInterface(graph, &IID_IFilterMapper2, (void **)&mapper);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = get_refcount(graph);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(mapper);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);

    hr = IFilterMapper2_QueryInterface(mapper, &IID_IFilterGraph2, (void **)&graph2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph2 == graph, "Got unexpected IFilterGraph2 %p.\n", graph2);
    IFilterGraph2_Release(graph2);

    IFilterMapper2_Release(mapper);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got unexpected refcount %d.\n", ref);
}

/* Test how methods from "control" interfaces (IBasicAudio, IBasicVideo,
 * IVideoWindow) are delegated to filters exposing those interfaces. */
static void test_control_delegation(void)
{
    IFilterGraph2 *graph = create_graph();
    IBasicAudio *audio, *filter_audio;
    IBaseFilter *renderer;
    IVideoWindow *window;
    IBasicVideo2 *video;
    ITypeInfo *typeinfo;
    TYPEATTR *typeattr;
    ULONG count;
    HRESULT hr;
    LONG val;

    /* IBasicAudio */

    hr = IFilterGraph2_QueryInterface(graph, &IID_IBasicAudio, (void **)&audio);
    ok(hr == S_OK, "got %#x\n", hr);

    hr = IBasicAudio_GetTypeInfoCount(audio, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);

    hr = IBasicAudio_put_Volume(audio, -10);
    ok(hr == E_NOTIMPL, "got %#x\n", hr);
    hr = IBasicAudio_get_Volume(audio, &val);
    ok(hr == E_NOTIMPL, "got %#x\n", hr);
    hr = IBasicAudio_put_Balance(audio, 10);
    ok(hr == E_NOTIMPL, "got %#x\n", hr);
    hr = IBasicAudio_get_Balance(audio, &val);
    ok(hr == E_NOTIMPL, "got %#x\n", hr);

    hr = CoCreateInstance(&CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void **)&renderer);
    if (hr != VFW_E_NO_AUDIO_HARDWARE)
    {
        ok(hr == S_OK, "got %#x\n", hr);

        hr = IFilterGraph2_AddFilter(graph, renderer, NULL);
        ok(hr == S_OK, "got %#x\n", hr);

        hr = IBasicAudio_put_Volume(audio, -10);
        ok(hr == S_OK, "got %#x\n", hr);
        hr = IBasicAudio_get_Volume(audio, &val);
        ok(hr == S_OK, "got %#x\n", hr);
        ok(val == -10, "got %d\n", val);
        hr = IBasicAudio_put_Balance(audio, 10);
        ok(hr == S_OK || hr == VFW_E_MONO_AUDIO_HW, "got %#x\n", hr);
        hr = IBasicAudio_get_Balance(audio, &val);
        ok(hr == S_OK || hr == VFW_E_MONO_AUDIO_HW, "got %#x\n", hr);
        if (hr == S_OK)
            ok(val == 10, "got balance %d\n", val);

        hr = IBaseFilter_QueryInterface(renderer, &IID_IBasicAudio, (void **)&filter_audio);
        ok(hr == S_OK, "got %#x\n", hr);

        hr = IBasicAudio_get_Volume(filter_audio, &val);
        ok(hr == S_OK, "got %#x\n", hr);
        ok(val == -10, "got volume %d\n", val);

        hr = IFilterGraph2_RemoveFilter(graph, renderer);
        ok(hr == S_OK, "got %#x\n", hr);

        IBaseFilter_Release(renderer);
        IBasicAudio_Release(filter_audio);
    }

    hr = IBasicAudio_put_Volume(audio, -10);
    ok(hr == E_NOTIMPL, "got %#x\n", hr);
    hr = IBasicAudio_get_Volume(audio, &val);
    ok(hr == E_NOTIMPL, "got %#x\n", hr);
    hr = IBasicAudio_put_Balance(audio, 10);
    ok(hr == E_NOTIMPL, "got %#x\n", hr);
    hr = IBasicAudio_get_Balance(audio, &val);
    ok(hr == E_NOTIMPL, "got %#x\n", hr);

    IBasicAudio_Release(audio);

    /* IBasicVideo and IVideoWindow */

    hr = IFilterGraph2_QueryInterface(graph, &IID_IBasicVideo2, (void **)&video);
    ok(hr == S_OK, "got %#x\n", hr);
    hr = IFilterGraph2_QueryInterface(graph, &IID_IVideoWindow, (void **)&window);
    ok(hr == S_OK, "got %#x\n", hr);

    /* Unlike IBasicAudio, these return E_NOINTERFACE. */
    hr = IBasicVideo2_get_BitRate(video, &val);
    ok(hr == E_NOINTERFACE, "got %#x\n", hr);

    hr = IBasicVideo2_GetTypeInfoCount(video, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);

    hr = IBasicVideo2_GetTypeInfo(video, 0, 0, &typeinfo);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = ITypeInfo_GetTypeAttr(typeinfo, &typeattr);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(typeattr->typekind == TKIND_DISPATCH, "Got kind %u.\n", typeattr->typekind);
    ok(IsEqualGUID(&typeattr->guid, &IID_IBasicVideo), "Got IID %s.\n", wine_dbgstr_guid(&typeattr->guid));
    ITypeInfo_ReleaseTypeAttr(typeinfo, typeattr);
    ITypeInfo_Release(typeinfo);

    hr = IVideoWindow_SetWindowForeground(window, OAFALSE);
    ok(hr == E_NOINTERFACE, "got %#x\n", hr);

    hr = IVideoWindow_GetTypeInfoCount(window, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);

    hr = CoCreateInstance(&CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void **)&renderer);
    ok(hr == S_OK, "got %#x\n", hr);

    hr = IFilterGraph2_AddFilter(graph, renderer, NULL);
    ok(hr == S_OK, "got %#x\n", hr);

    hr = IBasicVideo2_get_BitRate(video, &val);
    ok(hr == VFW_E_NOT_CONNECTED, "got %#x\n", hr);
    hr = IVideoWindow_SetWindowForeground(window, OAFALSE);
    ok(hr == VFW_E_NOT_CONNECTED, "got %#x\n", hr);

    hr = IFilterGraph2_RemoveFilter(graph, renderer);
    ok(hr == S_OK, "got %#x\n", hr);

    hr = IBasicVideo2_get_BitRate(video, &val);
    ok(hr == E_NOINTERFACE, "got %#x\n", hr);
    hr = IVideoWindow_SetWindowForeground(window, OAFALSE);
    ok(hr == E_NOINTERFACE, "got %#x\n", hr);

    IBaseFilter_Release(renderer);
    IBasicVideo2_Release(video);
    IVideoWindow_Release(window);
    IFilterGraph2_Release(graph);
}

static void test_add_remove_filter(void)
{
    struct testfilter filter;

    IFilterGraph2 *graph = create_graph();
    IBaseFilter *ret_filter;
    HRESULT hr;

    testfilter_init(&filter, NULL, 0);

    hr = IFilterGraph2_FindFilterByName(graph, L"testid", &ret_filter);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#x.\n", hr);
    ok(!ret_filter, "Got filter %p.\n", ret_filter);

    hr = IFilterGraph2_AddFilter(graph, &filter.IBaseFilter_iface, L"testid");
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(filter.graph == (IFilterGraph *)graph, "Got graph %p.\n", filter.graph);
    ok(!wcscmp(filter.name, L"testid"), "Got name %s.\n", wine_dbgstr_w(filter.name));

    hr = IFilterGraph2_FindFilterByName(graph, L"testid", &ret_filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(ret_filter == &filter.IBaseFilter_iface, "Got filter %p.\n", ret_filter);
    IBaseFilter_Release(ret_filter);

    hr = IFilterGraph2_RemoveFilter(graph, &filter.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!filter.graph, "Got graph %p.\n", filter.graph);
    ok(!filter.name, "Got name %s.\n", wine_dbgstr_w(filter.name));
    ok(!filter.clock, "Got clock %p,\n", filter.clock);
    ok(filter.ref == 1, "Got outstanding refcount %d.\n", filter.ref);

    hr = IFilterGraph2_FindFilterByName(graph, L"testid", &ret_filter);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#x.\n", hr);
    ok(!ret_filter, "Got filter %p.\n", ret_filter);

    hr = IFilterGraph2_AddFilter(graph, &filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(filter.graph == (IFilterGraph *)graph, "Got graph %p.\n", filter.graph);
    ok(!wcscmp(filter.name, L"0001"), "Got name %s.\n", wine_dbgstr_w(filter.name));

    hr = IFilterGraph2_FindFilterByName(graph, L"0001", &ret_filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(ret_filter == &filter.IBaseFilter_iface, "Got filter %p.\n", ret_filter);
    IBaseFilter_Release(ret_filter);

    /* test releasing the filter graph while filters are still connected */
    hr = IFilterGraph2_Release(graph);
    ok(!hr, "Got outstanding refcount %d.\n", hr);
    ok(!filter.graph, "Got graph %p.\n", filter.graph);
    ok(!filter.name, "Got name %s.\n", wine_dbgstr_w(filter.name));
    ok(!filter.clock, "Got clock %p.\n", filter.clock);
    ok(filter.ref == 1, "Got outstanding refcount %d.\n", filter.ref);
}

static HRESULT WINAPI test_connect_direct_Connect(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct testpin *pin = impl_from_IPin(iface);
    if (winetest_debug > 1) trace("%p->Connect()\n", pin);

    pin->peer = peer;
    IPin_AddRef(peer);
    pin->mt = (AM_MEDIA_TYPE *)mt;
    return S_OK;
}

static const IPinVtbl test_connect_direct_vtbl =
{
    testpin_QueryInterface,
    testpin_AddRef,
    testpin_Release,
    test_connect_direct_Connect,
    no_ReceiveConnection,
    testpin_Disconnect,
    testpin_ConnectedTo,
    testpin_ConnectionMediaType,
    testpin_QueryPinInfo,
    testpin_QueryDirection,
    testpin_QueryId,
    testpin_QueryAccept,
    no_EnumMediaTypes,
    testpin_QueryInternalConnections,
    testpin_EndOfStream,
    testpin_BeginFlush,
    testpin_EndFlush,
    testpin_NewSegment
};

static void test_connect_direct_init(struct testpin *pin, PIN_DIRECTION dir)
{
    testpin_init(pin, &test_connect_direct_vtbl, dir);
}

static void test_connect_direct(void)
{
    struct testpin source_pin, sink_pin, parser1_pins[2], parser2_pins[2];
    struct testfilter source, sink, parser1, parser2;

    IFilterGraph2 *graph = create_graph();
    AM_MEDIA_TYPE mt;
    HRESULT hr;

    test_connect_direct_init(&source_pin, PINDIR_OUTPUT);
    testfilter_init(&source, &source_pin, 1);
    test_connect_direct_init(&sink_pin, PINDIR_INPUT);
    testfilter_init(&sink, &sink_pin, 1);
    test_connect_direct_init(&parser1_pins[0], PINDIR_INPUT);
    test_connect_direct_init(&parser1_pins[1], PINDIR_OUTPUT);
    testfilter_init(&parser1, parser1_pins, 2);
    test_connect_direct_init(&parser2_pins[0], PINDIR_INPUT);
    test_connect_direct_init(&parser2_pins[1], PINDIR_OUTPUT);
    testfilter_init(&parser2, parser2_pins, 2);

    hr = IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!source_pin.mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Connect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!source_pin.mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    /* Swap the pins when connecting. */
    hr = IFilterGraph2_ConnectDirect(graph, &sink_pin.IPin_iface, &source_pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
todo_wine
    ok(sink_pin.peer == &source_pin.IPin_iface, "Got peer %p.\n", sink_pin.peer);
    ok(!sink_pin.mt, "Got mt %p.\n", sink_pin.mt);
todo_wine
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
todo_wine
    ok(hr == S_OK, "Got hr %#x.\n", hr);
todo_wine
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Connect(graph, &sink_pin.IPin_iface, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
todo_wine
    ok(sink_pin.peer == &source_pin.IPin_iface, "Got peer %p.\n", sink_pin.peer);
    ok(!sink_pin.mt, "Got mt %p.\n", sink_pin.mt);
todo_wine
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
todo_wine
    ok(hr == S_OK, "Got hr %#x.\n", hr);
todo_wine
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    /* Disconnect() does not disconnect the peer. */
    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!source_pin.mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    sink_pin.peer = &source_pin.IPin_iface;
    IPin_AddRef(sink_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    /* Test specifying the media type. */
    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(source_pin.mt == &mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Test Reconnect[Ex](). */

    hr = IFilterGraph2_Reconnect(graph, &source_pin.IPin_iface);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);
    hr = IFilterGraph2_Reconnect(graph, &sink_pin.IPin_iface);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFilterGraph2_Reconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!source_pin.mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    sink_pin.peer = &source_pin.IPin_iface;
    IPin_AddRef(sink_pin.peer);
    hr = IFilterGraph2_Reconnect(graph, &sink_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!source_pin.mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_ReconnectEx(graph, &source_pin.IPin_iface, NULL);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);
    hr = IFilterGraph2_ReconnectEx(graph, &sink_pin.IPin_iface, NULL);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFilterGraph2_ReconnectEx(graph, &source_pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!source_pin.mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFilterGraph2_ReconnectEx(graph, &source_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(source_pin.mt == &mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    hr = IFilterGraph2_Disconnect(graph, &source_pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* ConnectDirect() protects against cyclical connections. */
    hr = IFilterGraph2_AddFilter(graph, &parser1.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFilterGraph2_AddFilter(graph, &parser2.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &parser1_pins[1].IPin_iface, &parser1_pins[0].IPin_iface, NULL);
    ok(hr == VFW_E_CIRCULAR_GRAPH, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &parser1_pins[1].IPin_iface, &parser2_pins[0].IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFilterGraph2_ConnectDirect(graph, &parser2_pins[1].IPin_iface, &parser1_pins[0].IPin_iface, NULL);
    todo_wine ok(hr == VFW_E_CIRCULAR_GRAPH, "Got hr %#x.\n", hr);
    IFilterGraph2_Disconnect(graph, &parser1_pins[1].IPin_iface);
    IFilterGraph2_Disconnect(graph, &parser2_pins[0].IPin_iface);

    parser1_pins[0].QueryInternalConnections_hr = S_OK;
    hr = IFilterGraph2_ConnectDirect(graph, &parser1_pins[1].IPin_iface, &parser1_pins[0].IPin_iface, NULL);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!parser1_pins[0].peer, "Got peer %p.\n", parser1_pins[0].peer);
    todo_wine ok(parser1_pins[1].peer == &parser1_pins[0].IPin_iface, "Got peer %p.\n", parser1_pins[1].peer);
    IFilterGraph2_Disconnect(graph, &parser1_pins[0].IPin_iface);
    IFilterGraph2_Disconnect(graph, &parser1_pins[1].IPin_iface);

    hr = IFilterGraph2_ConnectDirect(graph, &parser1_pins[1].IPin_iface, &parser2_pins[0].IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFilterGraph2_ConnectDirect(graph, &parser2_pins[1].IPin_iface, &parser1_pins[0].IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IFilterGraph2_Disconnect(graph, &parser1_pins[1].IPin_iface);
    IFilterGraph2_Disconnect(graph, &parser2_pins[0].IPin_iface);

    hr = IFilterGraph2_RemoveFilter(graph, &parser1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFilterGraph2_RemoveFilter(graph, &parser2.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Both pins are disconnected when a filter is removed. */
    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    sink_pin.peer = &source_pin.IPin_iface;
    IPin_AddRef(sink_pin.peer);
    hr = IFilterGraph2_RemoveFilter(graph, &source.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);

    /* Or when the graph is destroyed. */
    hr = IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink_pin.IPin_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source_pin.peer == &sink_pin.IPin_iface, "Got peer %p.\n", source_pin.peer);
    ok(!source_pin.mt, "Got mt %p.\n", source_pin.mt);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
    IPin_AddRef(sink_pin.peer = &source_pin.IPin_iface);

    hr = IFilterGraph2_Release(graph);
    ok(!hr, "Got outstanding refcount %d.\n", hr);
    ok(source.ref == 1, "Got outstanding refcount %d.\n", source.ref);
    ok(sink.ref == 1, "Got outstanding refcount %d.\n", sink.ref);
    ok(source_pin.ref == 1, "Got outstanding refcount %d.\n", source_pin.ref);
todo_wine
    ok(sink_pin.ref == 1, "Got outstanding refcount %d.\n", sink_pin.ref);
    ok(!source_pin.peer, "Got peer %p.\n", source_pin.peer);
    ok(!sink_pin.peer, "Got peer %p.\n", sink_pin.peer);
}

static void test_sync_source(void)
{
    struct testfilter filter1, filter2;

    IFilterGraph2 *graph = create_graph();
    IReferenceClock *systemclock, *clock;
    IMediaFilter *filter;
    HRESULT hr;
    ULONG ref;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);

    testfilter_init(&filter1, NULL, 0);
    testfilter_init(&filter2, NULL, 0);

    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &filter2.IBaseFilter_iface, NULL);

    ok(!filter1.clock, "Got clock %p.\n", filter1.clock);
    ok(!filter2.clock, "Got clock %p.\n", filter2.clock);

    CoCreateInstance(&CLSID_SystemClock, NULL, CLSCTX_INPROC_SERVER,
            &IID_IReferenceClock, (void **)&systemclock);

    hr = IMediaFilter_SetSyncSource(filter, systemclock);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(filter1.clock == systemclock, "Got clock %p.\n", filter1.clock);
    ok(filter2.clock == systemclock, "Got clock %p.\n", filter2.clock);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(clock == systemclock, "Got clock %p.\n", clock);
    IReferenceClock_Release(clock);

    hr = IMediaFilter_SetSyncSource(filter, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!filter1.clock, "Got clock %p.\n", filter1.clock);
    ok(!filter2.clock, "Got clock %p.\n", filter2.clock);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
todo_wine
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!clock, "Got clock %p.\n", clock);

    IReferenceClock_Release(systemclock);
    IMediaFilter_Release(filter);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d\n", ref);
    ok(filter1.ref == 1, "Got outstanding refcount %d.\n", filter1.ref);
    ok(filter2.ref == 1, "Got outstanding refcount %d.\n", filter2.ref);
}

#define check_filter_state(a, b) check_filter_state_(__LINE__, a, b)
static void check_filter_state_(unsigned int line, IFilterGraph2 *graph, FILTER_STATE expect)
{
    IMediaFilter *mediafilter;
    IEnumFilters *filterenum;
    IMediaControl *control;
    OAFilterState oastate;
    IBaseFilter *filter;
    FILTER_STATE state;
    HRESULT hr;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&mediafilter);
    hr = IMediaFilter_GetState(mediafilter, 1000, &state);
    ok_(__FILE__, line)(hr == S_OK, "IMediaFilter_GetState() returned %#x.\n", hr);
    ok_(__FILE__, line)(state == expect, "Expected state %u, got %u.\n", expect, state);
    IMediaFilter_Release(mediafilter);

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    hr = IMediaControl_GetState(control, 1000, &oastate);
    ok_(__FILE__, line)(hr == S_OK, "IMediaControl_GetState() returned %#x.\n", hr);
    ok_(__FILE__, line)(state == expect, "Expected state %u, got %u.\n", expect, state);
    IMediaControl_Release(control);

    IFilterGraph2_EnumFilters(graph, &filterenum);
    while (IEnumFilters_Next(filterenum, 1, &filter, NULL) == S_OK)
    {
        hr = IBaseFilter_GetState(filter, 1000, &state);
        ok_(__FILE__, line)(hr == S_OK, "IBaseFilter_GetState() returned %#x.\n", hr);
        ok_(__FILE__, line)(state == expect, "Expected state %u, got %u.\n", expect, state);
        IBaseFilter_Release(filter);
    }
    IEnumFilters_Release(filterenum);
}


static void test_filter_state(void)
{
    struct testfilter source, sink, dummy;
    struct testpin source_pin, sink_pin;

    IFilterGraph2 *graph = create_graph();
    REFERENCE_TIME start_time, time;
    IReferenceClock *clock;
    IMediaControl *control;
    IMediaSeeking *seeking;
    FILTER_STATE mf_state;
    IMediaFilter *filter;
    OAFilterState state;
    HRESULT hr;
    ULONG ref;

    testsource_init(&source_pin, NULL, 0);
    testsink_init(&sink_pin);
    testfilter_init(&source, &source_pin, 1);
    testfilter_init(&sink, &sink_pin, 1);
    testfilter_init(&dummy, NULL, 0);
    sink.IMediaSeeking_iface.lpVtbl = &testseek_vtbl;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);

    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &dummy.IBaseFilter_iface, NULL);
    /* Using IPin::Connect instead of IFilterGraph2::ConnectDirect to show that */
    /* FilterGraph does not rely on ::ConnectDirect to track filter connections. */
    IPin_Connect(&source_pin.IPin_iface, &sink_pin.IPin_iface, NULL);

    check_filter_state(graph, State_Stopped);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Paused);

    /* Pausing sets the default sync source, if it's not already set. */

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!clock, "Reference clock not set.\n");
    ok(source.clock == clock, "Expected %p, got %p.\n", clock, source.clock);
    ok(sink.clock == clock, "Expected %p, got %p.\n", clock, sink.clock);

    hr = IReferenceClock_GetTime(clock, &start_time);
    ok(SUCCEEDED(hr), "Got hr %#x.\n", hr);
    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Running);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(source.start_time >= start_time && source.start_time < start_time + 500 * 10000,
                "Expected time near %s, got %s.\n",
                wine_dbgstr_longlong(start_time), wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == source.start_time, "Expected time %s, got %s.\n",
        wine_dbgstr_longlong(source.start_time), wine_dbgstr_longlong(sink.start_time));

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Paused);

    sink.state = State_Stopped;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    sink.state = State_Running;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    sink.state = State_Paused;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Stopped);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Running);

    sink.state = State_Stopped;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);

    sink.state = State_Paused;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);

    sink.state = State_Running;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Stopped);

    sink.state = State_Running;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    sink.state = State_Paused;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    sink.state = State_Stopped;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Paused);

    hr = IMediaControl_StopWhenReady(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Stopped);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Running);

    hr = IMediaControl_StopWhenReady(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Stopped);

    hr = IMediaControl_StopWhenReady(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Stopped);

    IReferenceClock_Release(clock);
    IMediaFilter_Release(filter);
    IMediaControl_Release(control);
    IFilterGraph2_Release(graph);

    /* Test same methods using IMediaFilter. */

    graph = create_graph();
    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaSeeking, (void **)&seeking);

    /* Add the filters in reverse order this time. */
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    /* Using IPin::Connect instead of IFilterGraph2::ConnectDirect to show that */
    /* FilterGraph does not rely on ::ConnectDirect to track filter connections. */
    IPin_Connect(&source_pin.IPin_iface, &sink_pin.IPin_iface, NULL);

    hr = IMediaFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Paused);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!clock, "Reference clock not set.\n");
    ok(source.clock == clock, "Expected %p, got %p.\n", clock, source.clock);
    ok(sink.clock == clock, "Expected %p, got %p.\n", clock, sink.clock);

    hr = IMediaFilter_Run(filter, 0xdeadbeef);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Running);
    ok(source.start_time == 0xdeadbeef, "Got time %s.\n", wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == 0xdeadbeef, "Got time %s.\n", wine_dbgstr_longlong(sink.start_time));

    hr = IMediaFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Paused);

    hr = IMediaFilter_Run(filter, 0xdeadf00d);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Running);
    ok(source.start_time == 0xdeadf00d, "Got time %s.\n", wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == 0xdeadf00d, "Got time %s.\n", wine_dbgstr_longlong(sink.start_time));

    hr = IMediaFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Paused);

    hr = IMediaFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Stopped);

    source.expect_run_prev = sink.expect_run_prev = State_Stopped;
    hr = IReferenceClock_GetTime(clock, &start_time);
    ok(SUCCEEDED(hr), "Got hr %#x.\n", hr);
    hr = IMediaFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Running);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(source.start_time >= start_time && source.start_time < start_time + 500 * 10000,
                "Expected time near %s, got %s.\n",
                wine_dbgstr_longlong(start_time), wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == source.start_time, "Expected time %s, got %s.\n",
        wine_dbgstr_longlong(source.start_time), wine_dbgstr_longlong(sink.start_time));

    Sleep(600);
    hr = IMediaFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Paused);

    source.expect_run_prev = sink.expect_run_prev = State_Paused;
    hr = IMediaFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Running);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(source.start_time >= start_time && source.start_time < start_time + 500 * 10000,
                "Expected time near %s, got %s.\n",
                wine_dbgstr_longlong(start_time), wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == source.start_time, "Expected time %s, got %s.\n",
        wine_dbgstr_longlong(source.start_time), wine_dbgstr_longlong(sink.start_time));

    hr = IMediaFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Paused);
    Sleep(600);

    start_time += 550 * 10000;
    hr = IMediaFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Running);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(source.start_time >= start_time && source.start_time < start_time + 500 * 10000,
                "Expected time near %s, got %s.\n",
                wine_dbgstr_longlong(start_time), wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == source.start_time, "Expected time %s, got %s.\n",
        wine_dbgstr_longlong(source.start_time), wine_dbgstr_longlong(sink.start_time));

    hr = IMediaFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Stopped);

    /* Test removing the sync source. */

    IReferenceClock_Release(clock);
    IMediaFilter_SetSyncSource(filter, NULL);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Running);
todo_wine
    ok(source.start_time > 0 && source.start_time < 500 * 10000,
        "Got time %s.\n", wine_dbgstr_longlong(source.start_time));
    ok(sink.start_time == source.start_time, "Expected time %s, got %s.\n",
        wine_dbgstr_longlong(source.start_time), wine_dbgstr_longlong(sink.start_time));

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Stopped);

    /* Test asynchronous state change. */

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    sink.state_hr = sink.GetState_hr = S_OK;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Stop(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    sink.state_hr = sink.GetState_hr = S_OK;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    /* Renderers are expected to block completing a state change into paused
     * until they receive a sample. Because the graph can transition from
     * stopped -> paused -> running in one call, which itself needs to be
     * asynchronous, it actually waits on a separate thread for all filters
     * to be ready, then calls IMediaFilter::Run() once they are.
     *
     * However, IMediaControl::GetState() will return VFW_S_STATE_INTERMEDIATE
     * if filters haven't caught up to the graph yet. To make matters worse, it
     * doesn't take the above into account, meaning that it'll gladly return
     * VFW_S_STATE_INTERMEDIATE even if passed an infinite timeout. */

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);
    ok(sink.state == State_Paused, "Got state %u.\n", sink.state);
    ok(source.state == State_Paused, "Got state %u.\n", source.state);

    /* SetPositions() does not pause the graph in this case, since it is
     * already in a paused state. */
    time = 0;
    hr = IMediaSeeking_SetPositions(seeking, &time, AM_SEEKING_AbsolutePositioning,
            NULL, AM_SEEKING_NoPositioning);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_Run(control);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);
    ok(sink.state == State_Paused, "Got state %u.\n", sink.state);
    ok(source.state == State_Paused, "Got state %u.\n", source.state);

    sink.state_hr = sink.GetState_hr = S_OK;

    while ((hr = IMediaControl_GetState(control, INFINITE, &state)) == VFW_S_STATE_INTERMEDIATE)
    {
        ok(state == State_Running, "Got state %u.\n", state);
        ok(sink.state == State_Paused, "Got state %u.\n", sink.state);
        ok(source.state == State_Paused, "Got state %u.\n", source.state);
        Sleep(10);
    }
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);
    ok(sink.state == State_Running, "Got state %u.\n", sink.state);
    ok(source.state == State_Running, "Got state %u.\n", source.state);

    /* The above logic does not apply to the running -> paused -> stopped
     * transition. The filter graph will stop a filter regardless of whether
     * it's completely paused. Inasmuch as stopping the filter is like flushing
     * it—i.e. it has to succeed—this makes sense. */

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Stop(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(sink.state == State_Stopped, "Got state %u.\n", sink.state);
    ok(source.state == State_Stopped, "Got state %u.\n", source.state);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    sink.state_hr = sink.GetState_hr = S_OK;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    /* Try an asynchronous stopped->paused->running transition, but pause or
     * stop the graph before our filter is completely paused. */

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaControl_Pause(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    sink.state_hr = sink.GetState_hr = S_OK;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);
    ok(sink.state == State_Paused, "Got state %u.\n", sink.state);
    ok(source.state == State_Paused, "Got state %u.\n", source.state);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaControl_Stop(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(sink.state == State_Stopped, "Got state %u.\n", sink.state);
    ok(source.state == State_Stopped, "Got state %u.\n", source.state);

    sink.state_hr = sink.GetState_hr = S_OK;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);
    ok(sink.state == State_Stopped, "Got state %u.\n", sink.state);
    ok(source.state == State_Stopped, "Got state %u.\n", source.state);

    /* Same, but tear down the graph instead. */

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IMediaFilter_Release(filter);
    IMediaControl_Release(control);
    IMediaSeeking_Release(seeking);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    graph = create_graph();
    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaSeeking, (void **)&seeking);
    IFilterGraph2_AddFilter(graph, &sink.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IPin_Connect(&source_pin.IPin_iface, &sink_pin.IPin_iface, NULL);

    /* This logic doesn't apply when using IMediaFilter methods directly. */

    source.expect_run_prev = sink.expect_run_prev = State_Stopped;
    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaFilter_Run(filter, 0);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaFilter_GetState(filter, 0, &mf_state);
    ok(hr == VFW_S_STATE_INTERMEDIATE, "Got hr %#x.\n", hr);
    ok(mf_state == State_Running, "Got state %u.\n", mf_state);
    ok(sink.state == State_Running, "Got state %u.\n", sink.state);
    ok(source.state == State_Running, "Got state %u.\n", source.state);

    sink.state_hr = sink.GetState_hr = S_OK;
    hr = IMediaFilter_GetState(filter, 0, &mf_state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mf_state == State_Running, "Got state %u.\n", mf_state);
    ok(sink.state == State_Running, "Got state %u.\n", sink.state);
    ok(source.state == State_Running, "Got state %u.\n", source.state);

    hr = IMediaFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(sink.state == State_Stopped, "Got state %u.\n", sink.state);
    ok(source.state == State_Stopped, "Got state %u.\n", source.state);

    source.expect_run_prev = sink.expect_run_prev = State_Paused;

    /* Test VFW_S_CANT_CUE. */

    sink.GetState_hr = VFW_S_CANT_CUE;
    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_CANT_CUE, "Got hr %#x.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    source.GetState_hr = VFW_S_CANT_CUE;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_CANT_CUE, "Got hr %#x.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    sink.GetState_hr = VFW_S_CANT_CUE;
    source.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_CANT_CUE, "Got hr %#x.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    sink.GetState_hr = source.GetState_hr = S_OK;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    sink.state_hr = S_FALSE;
    sink.GetState_hr = VFW_S_STATE_INTERMEDIATE;
    source.GetState_hr = VFW_S_CANT_CUE;
    hr = IMediaControl_Run(control);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(sink.state == State_Running, "Got state %u.\n", sink.state);
    ok(source.state == State_Running, "Got state %u.\n", source.state);

    hr = IMediaControl_GetState(control, 0, &state);
    ok(hr == VFW_S_CANT_CUE, "Got hr %#x.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);
    ok(sink.state == State_Running, "Got state %u.\n", sink.state);
    ok(source.state == State_Running, "Got state %u.\n", source.state);

    sink.state_hr = sink.GetState_hr = source.GetState_hr = S_OK;

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Add and remove a filter while the graph is running. */

    hr = IFilterGraph2_AddFilter(graph, &dummy.IBaseFilter_iface, L"dummy");
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dummy.state == State_Stopped, "Got state %#x.\n", dummy.state);

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Paused);
    ok(dummy.state == State_Paused, "Got state %#x.\n", dummy.state);

    hr = IFilterGraph2_RemoveFilter(graph, &dummy.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dummy.state == State_Paused, "Got state %#x.\n", dummy.state);

    hr = IFilterGraph2_AddFilter(graph, &dummy.IBaseFilter_iface, L"dummy");
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dummy.state == State_Paused, "Got state %#x.\n", dummy.state);

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Stopped);

    hr = IFilterGraph2_RemoveFilter(graph, &dummy.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dummy.state == State_Stopped, "Got state %#x.\n", dummy.state);

    /* Destroying the graph while it's running stops all filters. */

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    check_filter_state(graph, State_Running);

    source.expect_stop_prev = sink.expect_stop_prev = State_Running;
    IMediaFilter_Release(filter);
    IMediaControl_Release(control);
    IMediaSeeking_Release(seeking);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ok(source.ref == 1, "Got outstanding refcount %d.\n", source.ref);
    ok(sink.ref == 1, "Got outstanding refcount %d.\n", sink.ref);
    ok(source_pin.ref == 1, "Got outstanding refcount %d.\n", source_pin.ref);
    ok(sink_pin.ref == 1, "Got outstanding refcount %d.\n", sink_pin.ref);
    ok(source.state == State_Stopped, "Got state %u.\n", source.state);
    ok(sink.state == State_Stopped, "Got state %u.\n", sink.state);
}

/* Helper function to check whether a filter is considered a renderer, i.e.
 * whether its EC_COMPLETE notification will be passed on to the application. */
static HRESULT check_ec_complete(IFilterGraph2 *graph, IBaseFilter *filter)
{
    IMediaEventSink *eventsink;
    LONG_PTR param1, param2;
    IMediaControl *control;
    IMediaEvent *eventsrc;
    HRESULT hr, ret_hr;
    LONG code;

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaEvent, (void **)&eventsrc);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaEventSink, (void **)&eventsink);

    IMediaControl_Run(control);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0);
    ok(hr == E_ABORT, "Got hr %#x.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ret_hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0);
    if (ret_hr == S_OK)
    {
        ok(code == EC_COMPLETE, "Got code %#x.\n", code);
        ok(param1 == S_OK, "Got param1 %#lx.\n", param1);
        ok(!param2, "Got param2 %#lx.\n", param2);
        hr = IMediaEvent_FreeEventParams(eventsrc, code, param1, param2);
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0);
        ok(hr == E_ABORT, "Got hr %#x.\n", hr);
    }

    IMediaControl_Stop(control);

    IMediaControl_Release(control);
    IMediaEvent_Release(eventsrc);
    IMediaEventSink_Release(eventsink);
    return ret_hr;
}

static void test_ec_complete(void)
{
    struct testpin filter1_pin, filter2_pin, filter3_pin;
    struct testfilter filter1, filter2, filter3;

    IFilterGraph2 *graph = create_graph();
    IMediaEventSink *eventsink;
    LONG_PTR param1, param2;
    IMediaControl *control;
    IMediaEvent *eventsrc;
    HRESULT hr;
    LONG code;

    testsink_init(&filter1_pin);
    testsink_init(&filter2_pin);
    testsink_init(&filter3_pin);
    testfilter_init(&filter1, &filter1_pin, 1);
    testfilter_init(&filter2, &filter2_pin, 1);
    testfilter_init(&filter3, &filter3_pin, 1);

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaEvent, (void **)&eventsrc);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaEventSink, (void **)&eventsink);

    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &filter2.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &filter3.IBaseFilter_iface, NULL);

    /* EC_COMPLETE is only delivered to the user after all renderers deliver it. */

    filter1.IAMFilterMiscFlags_iface.lpVtbl = &testmiscflags_vtbl;
    filter2.IAMFilterMiscFlags_iface.lpVtbl = &testmiscflags_vtbl;
    filter3.IAMFilterMiscFlags_iface.lpVtbl = &testmiscflags_vtbl;
    filter1.misc_flags = filter2.misc_flags = AM_FILTER_MISC_FLAGS_IS_RENDERER;

    IMediaControl_Run(control);

    filter3.misc_flags = AM_FILTER_MISC_FLAGS_IS_RENDERER;

    while ((hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0)) == S_OK)
    {
        ok(code != EC_COMPLETE, "Got unexpected EC_COMPLETE.\n");
        IMediaEvent_FreeEventParams(eventsrc, code, param1, param2);
    }
    ok(hr == E_ABORT, "Got hr %#x.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#x.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter2.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(code == EC_COMPLETE, "Got code %#x.\n", code);
    ok(param1 == S_OK, "Got param1 %#lx.\n", param1);
    ok(!param2, "Got param2 %#lx.\n", param2);
    hr = IMediaEvent_FreeEventParams(eventsrc, code, param1, param2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#x.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter3.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#x.\n", hr);

    IMediaControl_Stop(control);

    /* Test CancelDefaultHandling(). */

    IMediaControl_Run(control);

    hr = IMediaEvent_CancelDefaultHandling(eventsrc, EC_COMPLETE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#x.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(code == EC_COMPLETE, "Got code %#x.\n", code);
    ok(param1 == S_OK, "Got param1 %#lx.\n", param1);
    ok(param2 == (LONG_PTR)&filter1.IBaseFilter_iface, "Got param2 %#lx.\n", param2);
    hr = IMediaEvent_FreeEventParams(eventsrc, code, param1, param2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#x.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter3.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(code == EC_COMPLETE, "Got code %#x.\n", code);
    ok(param1 == S_OK, "Got param1 %#lx.\n", param1);
    ok(param2 == (LONG_PTR)&filter3.IBaseFilter_iface, "Got param2 %#lx.\n", param2);
    hr = IMediaEvent_FreeEventParams(eventsrc, code, param1, param2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaEvent_GetEvent(eventsrc, &code, &param1, &param2, 50);
    ok(hr == E_ABORT, "Got hr %#x.\n", hr);

    IMediaControl_Stop(control);
    hr = IMediaEvent_RestoreDefaultHandling(eventsrc, EC_COMPLETE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* A filter counts as a renderer if it (1) exposes IAMFilterMiscFlags and
     * reports itself as a renderer, or (2) exposes IMediaSeeking and has no
     * output pins. Despite MSDN, QueryInternalConnections() does not seem to
     * be used. */

    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);
    IFilterGraph2_RemoveFilter(graph, &filter2.IBaseFilter_iface);
    IFilterGraph2_RemoveFilter(graph, &filter3.IBaseFilter_iface);
    filter1.misc_flags = 0;
    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);

    hr = check_ec_complete(graph, &filter1.IBaseFilter_iface);
    ok(hr == E_ABORT, "Got hr %#x.\n", hr);

    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);
    filter1_pin.dir = PINDIR_INPUT;
    filter1.IAMFilterMiscFlags_iface.lpVtbl = NULL;
    filter1.IMediaSeeking_iface.lpVtbl = &testseek_vtbl;
    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);

    hr = check_ec_complete(graph, &filter1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ok(filter1.seeking_ref > 0, "Unexpected seeking refcount %d.\n", filter1.seeking_ref);
    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);
    ok(filter1.seeking_ref == 0, "Unexpected seeking refcount %d.\n", filter1.seeking_ref);

    filter1_pin.dir = PINDIR_OUTPUT;
    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);

    hr = check_ec_complete(graph, &filter1.IBaseFilter_iface);
    ok(hr == E_ABORT, "Got hr %#x.\n", hr);

    ok(filter1.seeking_ref > 0, "Unexpected seeking refcount %d.\n", filter1.seeking_ref);
    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);
    ok(filter1.seeking_ref == 0, "Unexpected seeking refcount %d.\n", filter1.seeking_ref);

    filter1_pin.dir = PINDIR_INPUT;
    filter1.support_media_time = FALSE;
    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);

    hr = check_ec_complete(graph, &filter1.IBaseFilter_iface);
    ok(hr == E_ABORT, "Got hr %#x.\n", hr);

    ok(filter1.seeking_ref == 0, "Unexpected seeking refcount %d.\n", filter1.seeking_ref);
    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);

    filter1.IMediaSeeking_iface.lpVtbl = NULL;
    filter1_pin.dir = PINDIR_INPUT;
    filter1.pin_count = 1;
    filter1_pin.QueryInternalConnections_hr = S_OK;
    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);

    hr = check_ec_complete(graph, &filter1.IBaseFilter_iface);
    ok(hr == E_ABORT, "Got hr %#x.\n", hr);

    IMediaControl_Release(control);
    IMediaEvent_Release(eventsrc);
    IMediaEventSink_Release(eventsink);
    hr = IFilterGraph2_Release(graph);
    ok(!hr, "Got outstanding refcount %d.\n", hr);
    ok(filter1.ref == 1, "Got outstanding refcount %d.\n", filter1.ref);
    ok(filter2.ref == 1, "Got outstanding refcount %d.\n", filter2.ref);
    ok(filter3.ref == 1, "Got outstanding refcount %d.\n", filter3.ref);
}

/* Remove and re-add the filter, to flush the graph's internal
 * IMediaSeeking cache. Don't expose IMediaSeeking when adding, to show
 * that it's only queried when needed. */
static void flush_cached_seeking(IFilterGraph2 *graph, struct testfilter *filter)
{
    IFilterGraph2_RemoveFilter(graph, &filter->IBaseFilter_iface);
    filter->IMediaSeeking_iface.lpVtbl = NULL;
    IFilterGraph2_AddFilter(graph, &filter->IBaseFilter_iface, NULL);
    filter->IMediaSeeking_iface.lpVtbl = &testseek_vtbl;
}

static void test_graph_seeking(void)
{
    struct testfilter filter1, filter2;

    LONGLONG time, current, stop, earliest, latest;
    IFilterGraph2 *graph = create_graph();
    IMediaEventSink *eventsink;
    IMediaControl *control;
    IMediaSeeking *seeking;
    IMediaFilter *filter;
    unsigned int i;
    double rate;
    GUID format;
    HRESULT hr;
    DWORD caps;
    ULONG ref;

    static const GUID *const all_formats[] =
    {
        NULL,
        &TIME_FORMAT_NONE,
        &TIME_FORMAT_FRAME,
        &TIME_FORMAT_SAMPLE,
        &TIME_FORMAT_FIELD,
        &TIME_FORMAT_BYTE,
        &TIME_FORMAT_MEDIA_TIME,
        &testguid,
    };

    static const GUID *const unsupported_formats[] =
    {
        &TIME_FORMAT_FRAME,
        &TIME_FORMAT_SAMPLE,
        &TIME_FORMAT_FIELD,
        &TIME_FORMAT_BYTE,
        &testguid,
    };

    testfilter_init(&filter1, NULL, 0);
    testfilter_init(&filter2, NULL, 0);

    IFilterGraph2_QueryInterface(graph, &IID_IMediaControl, (void **)&control);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaSeeking, (void **)&seeking);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);
    IFilterGraph2_QueryInterface(graph, &IID_IMediaEventSink, (void **)&eventsink);

    hr = IMediaSeeking_GetCapabilities(seeking, &caps);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(!caps, "Got caps %#x.\n", caps);

    caps = 0;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    todo_wine ok(hr == E_FAIL, "Got hr %#x.\n", hr);
    ok(!caps, "Got caps %#x.\n", caps);

    caps = AM_SEEKING_CanSeekAbsolute;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    todo_wine ok(hr == E_FAIL, "Got hr %#x.\n", hr);
    todo_wine ok(!caps, "Got caps %#x.\n", caps);

    hr = IMediaSeeking_IsFormatSupported(seeking, NULL);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(all_formats); ++i)
    {
        hr = IMediaSeeking_IsFormatSupported(seeking, all_formats[i]);
        todo_wine ok(hr == E_NOTIMPL, "Got hr %#x for format %s.\n", hr, wine_dbgstr_guid(all_formats[i]));
    }

    hr = IMediaSeeking_QueryPreferredFormat(seeking, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_QueryPreferredFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_GetTimeFormat(seeking, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_IsUsingTimeFormat(seeking, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_NONE);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_NONE);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_QueryPreferredFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_NONE);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(unsupported_formats); ++i)
    {
        hr = IMediaSeeking_SetTimeFormat(seeking, unsupported_formats[i]);
        todo_wine ok(hr == E_NOTIMPL, "Got hr %#x for format %s.\n", hr, wine_dbgstr_guid(unsupported_formats[i]));
    }

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, NULL, 0x123456789a, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_MEDIA_TIME, 0x123456789a, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, NULL, 0x123456789a, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_MEDIA_TIME, 0x123456789a, &TIME_FORMAT_MEDIA_TIME);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_NONE, 0x123456789a, &TIME_FORMAT_MEDIA_TIME);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_NONE, 0x123456789a, NULL);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_MEDIA_TIME, 0x123456789a, &TIME_FORMAT_NONE);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, NULL, 0x123456789a, &TIME_FORMAT_NONE);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &TIME_FORMAT_NONE, 0x123456789a, &TIME_FORMAT_NONE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &testguid, 0x123456789a, &testguid);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_GetDuration(seeking, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_GetDuration(seeking, &time);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetStopPosition(seeking, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_SetPositions(seeking, &current, 0, &stop, 0);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetPositions(seeking, NULL, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_GetPositions(seeking, NULL, &stop);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    current = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!current, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_GetCurrentPosition(seeking, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    current = 0xdeadbeef;
    hr = IMediaSeeking_GetCurrentPosition(seeking, &current);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!current, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_GetAvailable(seeking, &earliest, &latest);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_SetRate(seeking, 1.0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_SetRate(seeking, 2.0);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetRate(seeking, &rate);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(rate == 1.0, "Got rate %.16e.\n", rate);

    hr = IMediaSeeking_GetPreroll(seeking, &time);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    /* Try with filters added. Note that a filter need only expose
     * IMediaSeeking—no other heuristics are used to determine if it is a
     * renderer. */

    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);
    filter1.IMediaSeeking_iface.lpVtbl = &testseek_vtbl;
    filter1.support_media_time = FALSE;
    filter1.support_testguid = TRUE;

    hr = IMediaSeeking_GetDuration(seeking, &time);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_SetTimeFormat(seeking, &testguid);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    IFilterGraph2_RemoveFilter(graph, &filter1.IBaseFilter_iface);
    filter1.support_media_time = TRUE;
    filter1.support_testguid = FALSE;

    IFilterGraph2_AddFilter(graph, &filter1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &filter2.IBaseFilter_iface, NULL);
    filter1.IMediaSeeking_iface.lpVtbl = &testseek_vtbl;
    filter2.IMediaSeeking_iface.lpVtbl = &testseek_vtbl;

    filter1.seek_caps = AM_SEEKING_CanDoSegments | AM_SEEKING_CanGetCurrentPos;
    filter2.seek_caps = AM_SEEKING_CanDoSegments | AM_SEEKING_CanGetDuration;
    hr = IMediaSeeking_GetCapabilities(seeking, &caps);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(caps == AM_SEEKING_CanDoSegments, "Got caps %#x.\n", caps);
    ok(filter1.seeking_ref > 0, "Unexpected seeking refcount %d.\n", filter1.seeking_ref);
    ok(filter2.seeking_ref > 0, "Unexpected seeking refcount %d.\n", filter2.seeking_ref);

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    caps = AM_SEEKING_CanDoSegments | AM_SEEKING_CanGetCurrentPos;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(caps == AM_SEEKING_CanDoSegments, "Got caps %#x.\n", caps);

    caps = AM_SEEKING_CanDoSegments;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(caps == AM_SEEKING_CanDoSegments, "Got caps %#x.\n", caps);

    caps = AM_SEEKING_CanGetCurrentPos;
    hr = IMediaSeeking_CheckCapabilities(seeking, &caps);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);
    ok(!caps, "Got caps %#x.\n", caps);

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    hr = IMediaSeeking_IsFormatSupported(seeking, &testguid);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    filter1.support_testguid = TRUE;
    hr = IMediaSeeking_IsFormatSupported(seeking, &testguid);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    filter1.support_testguid = FALSE;
    filter2.support_testguid = TRUE;
    hr = IMediaSeeking_IsFormatSupported(seeking, &testguid);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Filters are not consulted about preferred formats. */
    hr = IMediaSeeking_QueryPreferredFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    filter2.support_testguid = FALSE;
    hr = IMediaSeeking_SetTimeFormat(seeking, &testguid);
    todo_wine ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    filter1.support_testguid = TRUE;
    hr = IMediaSeeking_SetTimeFormat(seeking, &testguid);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(IsEqualGUID(&filter1.time_format, &testguid), "Got format %s.\n",
            debugstr_guid(&filter1.time_format));
    ok(IsEqualGUID(&filter2.time_format, &TIME_FORMAT_MEDIA_TIME),
            "Got format %s.\n", debugstr_guid(&filter2.time_format));

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(IsEqualGUID(&format, &testguid), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_QueryPreferredFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &TIME_FORMAT_MEDIA_TIME);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_IsUsingTimeFormat(seeking, &testguid);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetCapabilities(seeking, &caps);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(caps == AM_SEEKING_CanDoSegments, "Got caps %#x.\n", caps);
    ok(filter1.seeking_ref > 0, "Unexpected seeking refcount %d.\n", filter1.seeking_ref);
    ok(filter2.seeking_ref > 0, "Unexpected seeking refcount %d.\n", filter2.seeking_ref);

    filter2.support_testguid = TRUE;
    hr = IMediaSeeking_SetTimeFormat(seeking, &testguid);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&filter1.time_format, &TIME_FORMAT_MEDIA_TIME),
            "Got format %s.\n", debugstr_guid(&filter1.time_format));
    todo_wine ok(IsEqualGUID(&filter2.time_format, &testguid),
            "Got format %s.\n", debugstr_guid(&filter2.time_format));

    filter1.support_media_time = FALSE;
    flush_cached_seeking(graph, &filter1);

    hr = IMediaSeeking_GetCapabilities(seeking, &caps);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(caps == (AM_SEEKING_CanDoSegments | AM_SEEKING_CanGetDuration), "Got caps %#x.\n", caps);
    ok(!filter1.seeking_ref, "Unexpected seeking refcount %d.\n", filter1.seeking_ref);
    ok(filter2.seeking_ref > 0, "Unexpected seeking refcount %d.\n", filter2.seeking_ref);

    filter1.support_media_time = TRUE;
    filter1.support_testguid = FALSE;
    flush_cached_seeking(graph, &filter1);

    hr = IMediaSeeking_GetCapabilities(seeking, &caps);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(caps == AM_SEEKING_CanDoSegments, "Got caps %#x.\n", caps);
    ok(filter1.seeking_ref > 0, "Unexpected seeking refcount %d.\n", filter1.seeking_ref);
    ok(filter2.seeking_ref > 0, "Unexpected seeking refcount %d.\n", filter2.seeking_ref);

    ok(IsEqualGUID(&filter1.time_format, &TIME_FORMAT_MEDIA_TIME),
            "Got format %s.\n", debugstr_guid(&filter1.time_format));
    todo_wine ok(IsEqualGUID(&filter2.time_format, &testguid),
            "Got format %s.\n", debugstr_guid(&filter2.time_format));
    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(IsEqualGUID(&format, &testguid), "Got format %s.\n", wine_dbgstr_guid(&format));

    hr = IMediaSeeking_SetTimeFormat(seeking, &TIME_FORMAT_NONE);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetTimeFormat(seeking, &format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&format, &TIME_FORMAT_MEDIA_TIME), "Got format %s.\n", wine_dbgstr_guid(&format));

    time = 0xdeadbeef;
    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &testguid, 0x123456789a, &testguid);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0x123456789a, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaSeeking_ConvertTimeFormat(seeking, &time, &testguid, 0x123456789a, &TIME_FORMAT_NONE);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    filter1.seek_duration = 0x12345;
    filter2.seek_duration = 0x23456;
    hr = IMediaSeeking_GetDuration(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0x23456, "Got time %s.\n", wine_dbgstr_longlong(time));

    filter2.seek_duration = 0x12345;
    filter1.seek_duration = 0x23456;
    hr = IMediaSeeking_GetDuration(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0x23456, "Got time %s.\n", wine_dbgstr_longlong(time));

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    filter1.seek_stop = 0x54321;
    filter2.seek_stop = 0x65432;
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0x65432, "Got time %s.\n", wine_dbgstr_longlong(time));

    filter2.seek_stop = 0x54321;
    filter1.seek_stop = 0x65432;
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0x65432, "Got time %s.\n", wine_dbgstr_longlong(time));

    filter1.seek_hr = filter2.seek_hr = 0xbeef;
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0x65432, "Got time %s.\n", wine_dbgstr_longlong(time));

    filter1.seek_hr = E_NOTIMPL;
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0x54321, "Got time %s.\n", wine_dbgstr_longlong(time));

    filter1.seek_hr = 0xdeadbeef;
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == 0xdeadbeef, "Got hr %#x.\n", hr);

    filter1.seek_hr = filter2.seek_hr = E_NOTIMPL;
    hr = IMediaSeeking_GetStopPosition(seeking, &time);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    filter1.seek_hr = filter2.seek_hr = S_OK;

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!time, "Got time %s.\n", wine_dbgstr_longlong(time));

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    current = stop = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!current, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 0x65432, "Got time %s.\n", wine_dbgstr_longlong(stop));

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    current = 0x123;
    stop = 0x321;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(stop));
    ok(filter1.seek_current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(filter1.seek_current));
    ok(filter1.seek_stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(filter1.seek_stop));
    ok(filter2.seek_current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(filter2.seek_current));
    ok(filter2.seek_stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(filter2.seek_stop));

    filter1.seek_hr = filter2.seek_hr = 0xbeef;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    filter1.seek_hr = E_NOTIMPL;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    filter1.seek_hr = 0xdeadbeef;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == 0xdeadbeef, "Got hr %#x.\n", hr);

    filter1.seek_hr = filter2.seek_hr = E_NOTIMPL;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    filter1.seek_hr = filter2.seek_hr = S_OK;

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 12340000, "Got time %s.\n", wine_dbgstr_longlong(time));

    current = stop = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(current == 12340000, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(stop));

    current = 0x123;
    stop = 0x321;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning | AM_SEEKING_ReturnTime,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(current == 12340000, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(stop));
    ok(filter1.seek_current == 12340000, "Got time %s.\n", wine_dbgstr_longlong(filter1.seek_current));
    ok(filter1.seek_stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(filter1.seek_stop));
    ok(filter2.seek_current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(filter2.seek_current));
    ok(filter2.seek_stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(filter2.seek_stop));

    current = 0x123;
    stop = 0x321;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning | AM_SEEKING_ReturnTime);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 43210000, "Got time %s.\n", wine_dbgstr_longlong(stop));
    ok(filter1.seek_current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(filter1.seek_current));
    ok(filter1.seek_stop == 43210000, "Got time %s.\n", wine_dbgstr_longlong(filter1.seek_stop));
    ok(filter2.seek_current == 0x123, "Got time %s.\n", wine_dbgstr_longlong(filter2.seek_current));
    ok(filter2.seek_stop == 0x321, "Got time %s.\n", wine_dbgstr_longlong(filter2.seek_stop));

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    hr = IMediaSeeking_SetRate(seeking, 2.0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(filter1.seek_rate == 2.0, "Got rate %.16e.\n", filter1.seek_rate);
    todo_wine ok(filter2.seek_rate == 2.0, "Got rate %.16e.\n", filter2.seek_rate);

    hr = IMediaSeeking_SetRate(seeking, 1.0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(filter1.seek_rate == 1.0, "Got rate %.16e.\n", filter1.seek_rate);
    todo_wine ok(filter2.seek_rate == 1.0, "Got rate %.16e.\n", filter2.seek_rate);

    hr = IMediaSeeking_SetRate(seeking, -1.0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(filter1.seek_rate == -1.0, "Got rate %.16e.\n", filter1.seek_rate);
    todo_wine ok(filter2.seek_rate == -1.0, "Got rate %.16e.\n", filter2.seek_rate);

    flush_cached_seeking(graph, &filter1);
    flush_cached_seeking(graph, &filter2);

    hr = IMediaSeeking_GetRate(seeking, &rate);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(rate == -1.0, "Got rate %.16e.\n", rate);

    hr = IMediaSeeking_SetRate(seeking, 1.0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Test how retrieving the current position behaves while the graph is
     * running. Apparently the graph caches the last position returned by
     * SetPositions() and then adds the clock offset to the stream start. */

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Note that if the graph is running, it is paused while seeking. */
    current = 0;
    stop = 9000 * 10000;
    hr = IMediaSeeking_SetPositions(seeking, &current, AM_SEEKING_AbsolutePositioning,
            &stop, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(compare_time(time, 1234 * 10000, 40 * 10000),
                "Expected about 1234ms, got %s.\n", wine_dbgstr_longlong(time));
    current = stop = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(compare_time(current, 1234 * 10000, 40 * 10000),
                "Expected about 1234ms, got %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 9000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(stop));

    /* This remains true even if NoFlush is specified. */
    current = 1000 * 10000;
    stop = 8000 * 10000;
    hr = IMediaSeeking_SetPositions(seeking, &current,
            AM_SEEKING_AbsolutePositioning | AM_SEEKING_NoFlush,
            &stop, AM_SEEKING_AbsolutePositioning | AM_SEEKING_NoFlush);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    Sleep(100);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(compare_time(time, 1334 * 10000, 80 * 10000),
                "Expected about 1334ms, got %s.\n", wine_dbgstr_longlong(time));
    current = stop = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(compare_time(current, 1334 * 10000, 80 * 10000),
                "Expected about 1334ms, got %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 8000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(stop));

    hr = IMediaControl_Pause(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    Sleep(100);
    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(compare_time(time, 1334 * 10000, 80 * 10000),
                "Expected about 1334ms, got %s.\n", wine_dbgstr_longlong(time));
    current = stop = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (winetest_interactive) /* Timing problems make this test too liable to fail. */
        ok(compare_time(current, 1334 * 10000, 80 * 10000),
                "Expected about 1334ms, got %s.\n", wine_dbgstr_longlong(current));
    ok(stop == 8000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(stop));

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 12340000, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaFilter_SetSyncSource(filter, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    Sleep(100);
    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(!time, "Got time %s.\n", wine_dbgstr_longlong(time));
    current = stop = 0xdeadbeef;
    hr = IMediaSeeking_GetPositions(seeking, &current, &stop);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(!current, "Got time %s.\n", wine_dbgstr_longlong(current));
    ok(!stop, "Got time %s.\n", wine_dbgstr_longlong(stop));

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* GetCurrentPositions() will return the stop position once all renderers
     * report EC_COMPLETE. Atelier Sophie depends on this behaviour. */

    hr = IFilterGraph2_SetDefaultSyncSource(graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    filter1.seek_stop = 5000 * 10000;
    filter2.seek_stop = 6000 * 10000;

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time < 5000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time < 5000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter2.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 6000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    filter1.seek_hr = filter2.seek_hr = E_NOTIMPL;
    filter1.seek_stop = filter2.seek_stop = 0xdeadbeef;

    hr = IMediaControl_Run(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time < 5000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter1.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaEventSink_Notify(eventsink, EC_COMPLETE, S_OK, (LONG_PTR)&filter2.IBaseFilter_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSeeking_GetCurrentPosition(seeking, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 6000 * 10000, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaControl_Stop(control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    IMediaFilter_Release(filter);
    IMediaControl_Release(control);
    IMediaSeeking_Release(seeking);
    IMediaEventSink_Release(eventsink);

    ok(filter1.seeking_ref > 0, "Unexpected seeking refcount %d.\n", filter1.seeking_ref);
    ok(filter2.seeking_ref > 0, "Unexpected seeking refcount %d.\n", filter2.seeking_ref);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", hr);
    ok(filter1.ref == 1, "Got outstanding refcount %d.\n", filter1.ref);
    ok(filter2.ref == 1, "Got outstanding refcount %d.\n", filter2.ref);
    ok(filter1.seeking_ref == 0, "Unexpected seeking refcount %d.\n", filter1.seeking_ref);
    ok(filter2.seeking_ref == 0, "Unexpected seeking refcount %d.\n", filter2.seeking_ref);
}

static void test_default_sync_source(void)
{
    struct testpin source_pin, sink1_pin, sink2_pin;
    struct testfilter source, sink1, sink2;

    IFilterGraph2 *graph = create_graph();
    IReferenceClock *clock;
    IMediaFilter *filter;
    HRESULT hr;
    ULONG ref;

    testsink_init(&sink1_pin);
    testsink_init(&sink2_pin);
    testsource_init(&source_pin, NULL, 0);
    testfilter_init(&source, &source_pin, 1);
    testfilter_init(&sink1, &sink1_pin, 1);
    testfilter_init(&sink2, &sink2_pin, 1);

    IFilterGraph2_AddFilter(graph, &sink1.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &sink2.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, NULL);
    IFilterGraph2_ConnectDirect(graph, &source_pin.IPin_iface, &sink1_pin.IPin_iface, NULL);

    IFilterGraph2_QueryInterface(graph, &IID_IMediaFilter, (void **)&filter);

    hr = IFilterGraph2_SetDefaultSyncSource(graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!clock, "Reference clock not set.\n");
    IReferenceClock_Release(clock);

    source.IReferenceClock_iface.lpVtbl = &testclock_vtbl;

    hr = IFilterGraph2_SetDefaultSyncSource(graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(clock == &source.IReferenceClock_iface, "Got unexpected clock.\n");
    IReferenceClock_Release(clock);

    /* The documentation says that connected filters are preferred, but this
     * does not in fact seem to be the case. */

    sink2.IReferenceClock_iface.lpVtbl = &testclock_vtbl;

    hr = IFilterGraph2_SetDefaultSyncSource(graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(clock == &sink2.IReferenceClock_iface, "Got unexpected clock.\n");
    IReferenceClock_Release(clock);

    sink1.IReferenceClock_iface.lpVtbl = &testclock_vtbl;

    hr = IFilterGraph2_SetDefaultSyncSource(graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaFilter_GetSyncSource(filter, &clock);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(clock == &sink1.IReferenceClock_iface, "Got unexpected clock.\n");
    IReferenceClock_Release(clock);

    IMediaFilter_Release(filter);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ok(sink1.ref == 1, "Got outstanding refcount %d.\n", sink1.ref);
    ok(sink2.ref == 1, "Got outstanding refcount %d.\n", sink2.ref);
    ok(source.ref == 1, "Got outstanding refcount %d.\n", source.ref);
}

static void test_add_source_filter(void)
{
    static const char bogus_data[20] = {0xde, 0xad, 0xbe, 0xef};
    static const char midi_data[20] = {'M','T','h','d'};

    IFilterGraph2 *graph = create_graph();
    IFileSourceFilter *filesource;
    IBaseFilter *filter, *filter2;
    FILTER_INFO filter_info;
    const WCHAR *filename;
    WCHAR *ret_filename;
    AM_MEDIA_TYPE mt;
    CLSID clsid;
    HRESULT hr;
    ULONG ref;
    BOOL ret;
    HKEY key;

    /* Test a file which should be registered by extension. */

    filename = create_file(L"test.mp3", midi_data, sizeof(midi_data));
    hr = IFilterGraph2_AddSourceFilter(graph, filename, L"test", &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetClassID(filter, &clsid);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&clsid, &CLSID_AsyncReader), "Got filter %s.\n", wine_dbgstr_guid(&clsid));
    hr = IBaseFilter_QueryFilterInfo(filter, &filter_info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!wcscmp(filter_info.achName, L"test"), "Got unexpected name %s.\n", wine_dbgstr_w(filter_info.achName));
    IFilterGraph_Release(filter_info.pGraph);

    hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFileSourceFilter_GetCurFile(filesource, &ret_filename, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!wcscmp(ret_filename, filename), "Expected filename %s, got %s.\n",
            wine_dbgstr_w(filename), wine_dbgstr_w(ret_filename));
    ok(IsEqualGUID(&mt.majortype, &MEDIATYPE_Stream), "Got major type %s.\n", wine_dbgstr_guid(&mt.majortype));
    ok(IsEqualGUID(&mt.subtype, &MEDIASUBTYPE_MPEG1Audio), "Got subtype %s.\n", wine_dbgstr_guid(&mt.subtype));
    IFileSourceFilter_Release(filesource);

    hr = IFilterGraph2_AddSourceFilter(graph, filename, L"test", &filter2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(filter2 != filter, "Filters shouldn't match.\n");
    hr = IFilterGraph2_RemoveFilter(graph, filter2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = IBaseFilter_Release(filter2);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = IFilterGraph2_RemoveFilter(graph, filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %u.\n", wine_dbgstr_w(filename), GetLastError());

    /* Test a file which should be registered by signature. */

    filename = create_file(L"test.avi", midi_data, sizeof(midi_data));
    hr = IFilterGraph2_AddSourceFilter(graph, filename, NULL, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetClassID(filter, &clsid);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&clsid, &CLSID_AsyncReader), "Got filter %s.\n", wine_dbgstr_guid(&clsid));
    hr = IBaseFilter_QueryFilterInfo(filter, &filter_info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(!wcscmp(filter_info.achName, filename), "Got unexpected name %s.\n", wine_dbgstr_w(filter_info.achName));
    IFilterGraph_Release(filter_info.pGraph);

    hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFileSourceFilter_GetCurFile(filesource, &ret_filename, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!wcscmp(ret_filename, filename), "Expected filename %s, got %s.\n",
            wine_dbgstr_w(filename), wine_dbgstr_w(ret_filename));
    ok(IsEqualGUID(&mt.majortype, &MEDIATYPE_Stream), "Got major type %s.\n", wine_dbgstr_guid(&mt.majortype));
    ok(IsEqualGUID(&mt.subtype, &MEDIATYPE_Midi), "Got subtype %s.\n", wine_dbgstr_guid(&mt.subtype));
    IFileSourceFilter_Release(filesource);

    hr = IFilterGraph2_RemoveFilter(graph, filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %u.\n", wine_dbgstr_w(filename), GetLastError());

    if (!RegCreateKeyA(HKEY_CLASSES_ROOT, "Media Type\\{abbccdde-0000-0000-0000-000000000000}"
            "\\{bccddeef-0000-0000-0000-000000000000}", &key))
    {
        static const GUID testfilter_clsid = {0x12345678};
        struct testpin testfilter_pin;
        struct testfilter testfilter;
        struct testfilter_cf cf = {{&testfilter_cf_vtbl}, &testfilter};
        DWORD cookie;

        testsource_init(&testfilter_pin, NULL, 0);
        testfilter_init(&testfilter, &testfilter_pin, 1);
        testfilter.IFileSourceFilter_iface.lpVtbl = &testfilesource_vtbl;

        CoRegisterClassObject(&testfilter_clsid, (IUnknown *)&cf.IClassFactory_iface,
                CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie);
        RegSetValueExA(key, "0", 0, REG_SZ, (const BYTE *)"0,4,,deadbeef", 14);
        RegSetValueExA(key, "Source Filter", 0, REG_SZ, (const BYTE *)"{12345678-0000-0000-0000-000000000000}", 39);

        filename = create_file(L"test.avi", bogus_data, sizeof(bogus_data));
        hr = IFilterGraph2_AddSourceFilter(graph, filename, NULL, &filter);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(filter == &testfilter.IBaseFilter_iface, "Got unexpected filter %p.\n", filter);

        hr = IFilterGraph2_RemoveFilter(graph, filter);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        IBaseFilter_Release(filter);
        ref = IBaseFilter_Release(&testfilter.IBaseFilter_iface);
        ok(!ref, "Got outstanding refcount %d.\n", ref);
        ret = DeleteFileW(filename);
        ok(ret, "Failed to delete %s, error %u.\n", wine_dbgstr_w(filename), GetLastError());
        RegDeleteKeyA(HKEY_CLASSES_ROOT, "Media Type\\{abbccdde-0000-0000-0000-000000000000}"
            "\\{bccddeef-0000-0000-0000-000000000000}");
        RegDeleteKeyA(HKEY_CLASSES_ROOT, "Media Type\\{abbccdde-0000-0000-0000-000000000000}");
        CoRevokeClassObject(cookie);
    }
    else
        skip("Not enough permission to register media types.\n");

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static HWND get_renderer_hwnd(IFilterGraph2 *graph)
{
    IEnumFilters *enum_filters;
    IEnumPins *enum_pins;
    IBaseFilter *filter;
    IOverlay *overlay;
    HWND hwnd = NULL;
    HRESULT hr;
    IPin *pin;

    hr = IFilterGraph2_EnumFilters(graph, &enum_filters);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    while (IEnumFilters_Next(enum_filters, 1, &filter, NULL) == S_OK)
    {
        hr = IBaseFilter_EnumPins(filter, &enum_pins);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        hr = IEnumPins_Next(enum_pins, 1, &pin, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        IEnumPins_Release(enum_pins);

        if (SUCCEEDED(IPin_QueryInterface(pin, &IID_IOverlay, (void **)&overlay)))
        {
            hr = IOverlay_GetWindowHandle(overlay, &hwnd);
            ok(hr == S_OK, "Got hr %#x.\n", hr);
            IOverlay_Release(overlay);
        }

        IPin_Release(pin);
        IBaseFilter_Release(filter);
    }

    IEnumFilters_Release(enum_filters);

    return hwnd;
}

static BOOL expect_parent_message = TRUE;

static LRESULT CALLBACK parent_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    ok(expect_parent_message, "Got unexpected message %#x.\n", msg);
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void test_window_threading(void)
{
    static const WNDCLASSA class =
    {
        .lpfnWndProc = parent_proc,
        .lpszClassName = "quartz_test_parent",
    };
    WCHAR *filename = load_resource(L"test.avi");
    IFilterGraph2 *graph = create_graph();
    IVideoWindow *window;
    HWND hwnd, parent;
    HRESULT hr;
    DWORD tid;
    ULONG ref;
    BOOL ret;

    RegisterClassA(&class);

    parent = CreateWindowA("quartz_test_parent", NULL, WS_OVERLAPPEDWINDOW,
            50, 50, 150, 150, NULL, NULL, NULL, NULL);
    ok(!!parent, "Failed to create parent window.\n");

    hr = IFilterGraph2_RenderFile(graph, filename, NULL);
    if (FAILED(hr))
    {
        skip("Cannot render test file, hr %#x.\n", hr);
        IFilterGraph2_Release(graph);
        DeleteFileW(filename);
        return;
    }
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    if ((hwnd = get_renderer_hwnd(graph)))
    {
        tid = GetWindowThreadProcessId(hwnd, NULL);
        ok(tid != GetCurrentThreadId(), "Window should have been created on a separate thread.\n");

        /* The thread should be processing messages, or this will hang. */
        SendMessageA(hwnd, WM_NULL, 0, 0);

        /* Media Player Classic deadlocks if the parent is sent any messages
         * while the video window is released. In particular, we must not send
         * WM_PARENTNOTIFY. This is not achieved through window styles. */
        hr = IFilterGraph2_QueryInterface(graph, &IID_IVideoWindow, (void **)&window);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        hr = IVideoWindow_put_Owner(window, (OAHWND)parent);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        IVideoWindow_Release(window);
        ok(!(GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_NOPARENTNOTIFY), "Window has WS_EX_NOPARENTNOTIFY.\n");
    }
    else
        skip("Could not find renderer window.\n");

    SetActiveWindow(parent);
    expect_parent_message = FALSE;
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    expect_parent_message = TRUE;

    hwnd = GetActiveWindow();
    ok(hwnd == parent, "Parent window lost focus, active window %p.\n", hwnd);

    hr = CoCreateInstance(&CLSID_FilterGraphNoThread, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterGraph2, (void **)&graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_RenderFile(graph, filename, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    if ((hwnd = get_renderer_hwnd(graph)))
    {
        tid = GetWindowThreadProcessId(hwnd, NULL);
        ok(tid == GetCurrentThreadId(), "Window should be created on main thread.\n");
    }
    else
        skip("Could not find renderer window.\n");

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    DestroyWindow(parent);
    UnregisterClassA("quartz_test_parent", GetModuleHandleA(NULL));
    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete file, error %u.\n", GetLastError());
}

/* Hyperdevotion Noire needs to be able to Render() from UYVY. */
static void test_autoplug_uyvy(void)
{
    static const VIDEOINFOHEADER source_format =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = 600,
        .bmiHeader.biHeight = 400,
        .bmiHeader.biCompression = mmioFOURCC('U','Y','V','Y'),
        .bmiHeader.biBitCount = 16,
    };
    AM_MEDIA_TYPE source_type =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_UYVY,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(source_format),
        .pbFormat = (BYTE *)&source_format,
    };

    IFilterGraph2 *graph = create_graph();
    struct testpin source_pin;
    struct testfilter source;
    HRESULT hr;
    ULONG ref;

    testsource_init(&source_pin, NULL, 0);
    testfilter_init(&source, &source_pin, 1);
    source_pin.request_mt = &source_type;

    IFilterGraph2_AddFilter(graph, &source.IBaseFilter_iface, L"source");

    /* Windows 2008 doesn't seem to have an UYVY decoder, and the testbot chalks
     * failure to decode up to missing audio hardware, even though we're not
     * trying to render audio. */
    hr = IFilterGraph2_Render(graph, &source_pin.IPin_iface);
    todo_wine ok(hr == S_OK || hr == VFW_E_NO_AUDIO_HARDWARE, "Got hr %#x.\n", hr);

    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ok(source.ref == 1, "Got outstanding refcount %d.\n", source.ref);
    ok(source_pin.ref == 1, "Got outstanding refcount %d.\n", source_pin.ref);
}

START_TEST(filtergraph)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    test_interfaces();
    test_render_run(L"test.avi", FALSE, TRUE);
    test_render_run(L"test.mpg", TRUE, TRUE);
    test_render_run(L"test.mp3", TRUE, FALSE);
    test_render_run(L"test.wav", TRUE, FALSE);
    test_enum_filters();
    test_graph_builder_render();
    test_graph_builder_connect();
    test_aggregation();
    test_control_delegation();
    test_add_remove_filter();
    test_connect_direct();
    test_sync_source();
    test_filter_state();
    test_ec_complete();
    test_graph_seeking();
    test_default_sync_source();
    test_add_source_filter();
    test_window_threading();
    test_autoplug_uyvy();

    CoUninitialize();
    test_render_with_multithread();
}
