/*
 * Unit tests for MultiMedia Stream functions
 *
 * Copyright (C) 2009, 2012 Christian Costa
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
#include "amstream.h"
#include "mmreg.h"
#include "ks.h"
#include "initguid.h"
#include "ksmedia.h"

static const WCHAR primary_video_sink_id[] = L"I{A35FF56A-9FDA-11D0-8FDF-00C04FD9189D}";
static const WCHAR primary_audio_sink_id[] = L"I{A35FF56B-9FDA-11D0-8FDF-00C04FD9189D}";

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %d, got %d\n", ref, rc);
}

static IDirectDraw7* pdd7;
static IDirectDrawSurface7* pdds7;

static IAMMultiMediaStream *create_ammultimediastream(void)
{
    IAMMultiMediaStream *stream = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER,
            &IID_IAMMultiMediaStream, (void **)&stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    return stream;
}

static int create_directdraw(void)
{
    HRESULT hr;
    IDirectDraw* pdd = NULL;
    DDSURFACEDESC2 ddsd;

    hr = DirectDrawCreate(NULL, &pdd, NULL);
    ok(hr==DD_OK, "DirectDrawCreate returned: %x\n", hr);
    if (hr != DD_OK)
       goto error;

    hr = IDirectDraw_QueryInterface(pdd, &IID_IDirectDraw7, (LPVOID*)&pdd7);
    ok(hr==DD_OK, "QueryInterface returned: %x\n", hr);
    if (hr != DD_OK) goto error;

    hr = IDirectDraw7_SetCooperativeLevel(pdd7, GetDesktopWindow(), DDSCL_NORMAL);
    ok(hr==DD_OK, "SetCooperativeLevel returned: %x\n", hr);

    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw7_CreateSurface(pdd7, &ddsd, &pdds7, NULL);
    ok(hr==DD_OK, "CreateSurface returned: %x\n", hr);

    return TRUE;

error:
    if (pdds7)
        IDirectDrawSurface7_Release(pdds7);
    if (pdd7)
        IDirectDraw7_Release(pdd7);
    if (pdd)
        IDirectDraw_Release(pdd);

    return FALSE;
}

static void release_directdraw(void)
{
    IDirectDrawSurface7_Release(pdds7);
    IDirectDraw7_Release(pdd7);
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
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IMediaStreamFilter *filter;
    IMediaStream *stream;
    HRESULT hr;
    ULONG ref;

    check_interface(mmstream, &IID_IAMMultiMediaStream, TRUE);
    check_interface(mmstream, &IID_IMultiMediaStream, TRUE);
    check_interface(mmstream, &IID_IUnknown, TRUE);

    check_interface(mmstream, &IID_IAMMediaStream, FALSE);
    check_interface(mmstream, &IID_IAMMediaTypeStream, FALSE);
    check_interface(mmstream, &IID_IAudioMediaStream, FALSE);
    check_interface(mmstream, &IID_IBaseFilter, FALSE);
    check_interface(mmstream, &IID_IDirectDrawMediaStream, FALSE);
    check_interface(mmstream, &IID_IMediaFilter, FALSE);
    check_interface(mmstream, &IID_IMediaStream, FALSE);
    check_interface(mmstream, &IID_IMediaStreamFilter, FALSE);
    check_interface(mmstream, &IID_IPin, FALSE);

    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IMediaStreamFilter, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);

    check_interface(filter, &IID_IAMMediaStream, FALSE);
    check_interface(filter, &IID_IAMMediaTypeStream, FALSE);
    check_interface(filter, &IID_IAMMultiMediaStream, FALSE);
    check_interface(filter, &IID_IAudioMediaStream, FALSE);
    check_interface(filter, &IID_IDirectDrawMediaStream, FALSE);
    check_interface(filter, &IID_IMediaStream, FALSE);
    check_interface(filter, &IID_IMultiMediaStream, FALSE);
    check_interface(filter, &IID_IPin, FALSE);

    IMediaStreamFilter_Release(filter);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    check_interface(stream, &IID_IAMMediaStream, TRUE);
    check_interface(stream, &IID_IAudioMediaStream, TRUE);
    check_interface(stream, &IID_IMediaStream, TRUE);
    check_interface(stream, &IID_IMemInputPin, TRUE);
    check_interface(stream, &IID_IPin, TRUE);
    check_interface(stream, &IID_IUnknown, TRUE);

    check_interface(stream, &IID_IAMMediaTypeStream, FALSE);
    check_interface(stream, &IID_IAMMultiMediaStream, FALSE);
    check_interface(stream, &IID_IBaseFilter, FALSE);
    check_interface(stream, &IID_IDirectDrawMediaStream, FALSE);
    check_interface(stream, &IID_IMediaFilter, FALSE);
    check_interface(stream, &IID_IMediaStreamFilter, FALSE);
    check_interface(stream, &IID_IMultiMediaStream, FALSE);
    check_interface(stream, &IID_IPersist, FALSE);

    IMediaStream_Release(stream);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    check_interface(stream, &IID_IAMMediaStream, TRUE);
    check_interface(stream, &IID_IDirectDrawMediaStream, TRUE);
    check_interface(stream, &IID_IMediaStream, TRUE);
    check_interface(stream, &IID_IMemInputPin, TRUE);
    check_interface(stream, &IID_IPin, TRUE);
    check_interface(stream, &IID_IUnknown, TRUE);

    check_interface(stream, &IID_IAMMediaTypeStream, FALSE);
    check_interface(stream, &IID_IAMMultiMediaStream, FALSE);
    check_interface(stream, &IID_IAudioMediaStream, FALSE);
    check_interface(stream, &IID_IBaseFilter, FALSE);
    check_interface(stream, &IID_IDirectDraw, FALSE);
    check_interface(stream, &IID_IMediaFilter, FALSE);
    check_interface(stream, &IID_IMediaStreamFilter, FALSE);
    check_interface(stream, &IID_IMultiMediaStream, FALSE);
    check_interface(stream, &IID_IPersist, FALSE);

    IMediaStream_Release(stream);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %u.\n", ref);
}

static void test_openfile(void)
{
    IAMMultiMediaStream *pams;
    HRESULT hr;
    IGraphBuilder* pgraph;

    if (!(pams = create_ammultimediastream()))
        return;

    hr = IAMMultiMediaStream_GetFilterGraph(pams, &pgraph);
    ok(hr==S_OK, "IAMMultiMediaStream_GetFilterGraph returned: %x\n", hr);
    ok(pgraph==NULL, "Filtergraph should not be created yet\n");

    if (pgraph)
        IGraphBuilder_Release(pgraph);

    hr = IAMMultiMediaStream_OpenFile(pams, L"test.avi", 0);
    ok(hr==S_OK, "IAMMultiMediaStream_OpenFile returned: %x\n", hr);

    hr = IAMMultiMediaStream_GetFilterGraph(pams, &pgraph);
    ok(hr==S_OK, "IAMMultiMediaStream_GetFilterGraph returned: %x\n", hr);
    ok(pgraph!=NULL, "Filtergraph should be created\n");

    if (pgraph)
        IGraphBuilder_Release(pgraph);

    IAMMultiMediaStream_Release(pams);
}

static void test_renderfile(void)
{
    IAMMultiMediaStream *pams;
    HRESULT hr;
    IMediaStream *pvidstream = NULL;
    IDirectDrawMediaStream *pddstream = NULL;
    IDirectDrawStreamSample *pddsample = NULL;
    IDirectDrawSurface *surface;
    RECT rect;

    if (!(pams = create_ammultimediastream()))
        return;
    if (!create_directdraw())
    {
        IAMMultiMediaStream_Release(pams);
        return;
    }

    hr = IAMMultiMediaStream_Initialize(pams, STREAMTYPE_READ, 0, NULL);
    ok(hr==S_OK, "IAMMultiMediaStream_Initialize returned: %x\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(pams, (IUnknown*)pdd7, &MSPID_PrimaryVideo, 0, NULL);
    ok(hr==S_OK, "IAMMultiMediaStream_AddMediaStream returned: %x\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(pams, NULL, &MSPID_PrimaryAudio, AMMSF_ADDDEFAULTRENDERER, NULL);
    ok(hr==S_OK, "IAMMultiMediaStream_AddMediaStream returned: %x\n", hr);

    hr = IAMMultiMediaStream_OpenFile(pams, L"test.avi", 0);
    ok(hr==S_OK, "IAMMultiMediaStream_OpenFile returned: %x\n", hr);

    hr = IAMMultiMediaStream_GetMediaStream(pams, &MSPID_PrimaryVideo, &pvidstream);
    ok(hr==S_OK, "IAMMultiMediaStream_GetMediaStream returned: %x\n", hr);
    if (FAILED(hr)) goto error;

    hr = IMediaStream_QueryInterface(pvidstream, &IID_IDirectDrawMediaStream, (LPVOID*)&pddstream);
    ok(hr==S_OK, "IMediaStream_QueryInterface returned: %x\n", hr);
    if (FAILED(hr)) goto error;

    hr = IDirectDrawMediaStream_CreateSample(pddstream, NULL, NULL, 0, &pddsample);
    ok(hr == S_OK, "IDirectDrawMediaStream_CreateSample returned: %x\n", hr);

    surface = NULL;
    hr = IDirectDrawStreamSample_GetSurface(pddsample, &surface, &rect);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(surface == NULL, "got %p\n", surface);
    IDirectDrawStreamSample_Release(pddsample);

    hr = IDirectDrawSurface7_QueryInterface(pdds7, &IID_IDirectDrawSurface, (void**)&surface);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    EXPECT_REF(surface, 1);
    hr = IDirectDrawMediaStream_CreateSample(pddstream, surface, NULL, 0, &pddsample);
    ok(hr == S_OK, "IDirectDrawMediaStream_CreateSample returned: %x\n", hr);
    EXPECT_REF(surface, 2);
    IDirectDrawStreamSample_Release(pddsample);
    IDirectDrawSurface_Release(surface);

error:
    if (pddstream)
        IDirectDrawMediaStream_Release(pddstream);
    if (pvidstream)
        IMediaStream_Release(pvidstream);

    release_directdraw();
    IAMMultiMediaStream_Release(pams);
}

static const GUID test_mspid = {0x88888888};
static IAMMediaStream teststream;
static LONG teststream_refcount = 1;
static IAMMultiMediaStream *teststream_mmstream;
static IMediaStreamFilter *teststream_filter;
static IFilterGraph *teststream_graph;

static HRESULT WINAPI pin_QueryInterface(IPin *iface, REFIID iid, void **out)
{
    return IAMMediaStream_QueryInterface(&teststream, iid, out);
}

static ULONG WINAPI pin_AddRef(IPin *iface)
{
    return IAMMediaStream_AddRef(&teststream);
}

static ULONG WINAPI pin_Release(IPin *iface)
{
    return IAMMediaStream_Release(&teststream);
}

static HRESULT WINAPI pin_Connect(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pin_ReceiveConnection(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pin_Disconnect(IPin *iface)
{
    if (winetest_debug > 1) trace("Disconnect\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pin_ConnectedTo(IPin *iface, IPin **peer)
{
    if (winetest_debug > 1) trace("ConnectedTo\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pin_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pin_QueryPinInfo(IPin *iface, PIN_INFO *info)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pin_QueryDirection(IPin *iface, PIN_DIRECTION *dir)
{
    if (winetest_debug > 1) trace("QueryDirection\n");
    *dir = PINDIR_INPUT;
    return S_OK;
}

static HRESULT WINAPI pin_QueryId(IPin *iface, WCHAR **id)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pin_QueryInternalConnections(IPin *iface, IPin **pins, ULONG *count)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pin_EndOfStream(IPin *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pin_BeginFlush(IPin *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pin_EndFlush(IPin *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pin_NewSegment(IPin *iface, REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IPinVtbl pin_vtbl =
{
    pin_QueryInterface,
    pin_AddRef,
    pin_Release,
    pin_Connect,
    pin_ReceiveConnection,
    pin_Disconnect,
    pin_ConnectedTo,
    pin_ConnectionMediaType,
    pin_QueryPinInfo,
    pin_QueryDirection,
    pin_QueryId,
    pin_QueryAccept,
    pin_EnumMediaTypes,
    pin_QueryInternalConnections,
    pin_EndOfStream,
    pin_BeginFlush,
    pin_EndFlush,
    pin_NewSegment
};

static IPin testpin = {&pin_vtbl};

static HRESULT WINAPI stream_QueryInterface(IAMMediaStream *iface, REFIID iid, void **out)
{
    if (winetest_debug > 1) trace("QueryInterface(%s)\n", wine_dbgstr_guid(iid));

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IMediaStream) || IsEqualGUID(iid, &IID_IAMMediaStream))
    {
        IAMMediaStream_AddRef(iface);
        *out = iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IPin))
    {
        IAMMediaStream_AddRef(iface);
        *out = &testpin;
        return S_OK;
    }

    ok(0, "Unexpected interface %s.\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI stream_AddRef(IAMMediaStream *iface)
{
    return InterlockedIncrement(&teststream_refcount);
}

static ULONG WINAPI stream_Release(IAMMediaStream *iface)
{
    return InterlockedDecrement(&teststream_refcount);
}

static HRESULT WINAPI stream_GetMultiMediaStream(IAMMediaStream *iface, IMultiMediaStream **mmstream)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_GetInformation(IAMMediaStream *iface, MSPID *id, STREAM_TYPE *type)
{
    if (winetest_debug > 1) trace("GetInformation(%p, %p)\n", id, type);
    *id = test_mspid;
    ok(!type, "Got unexpected type %p.\n", type);
    return S_OK;
}

static HRESULT WINAPI stream_SetSameFormat(IAMMediaStream *iface, IMediaStream *ref, DWORD flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_AllocateSample(IAMMediaStream *iface, DWORD flags, IStreamSample **sample)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_CreateSharedSample(IAMMediaStream *iface,
        IStreamSample *existing, DWORD flags, IStreamSample **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_SendEndOfStream(IAMMediaStream *iface, DWORD flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Initialize(IAMMediaStream *iface, IUnknown *source,
        DWORD flags, REFMSPID id, const STREAM_TYPE type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_SetState(IAMMediaStream *iface, FILTER_STATE state)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_JoinAMMultiMediaStream(IAMMediaStream *iface, IAMMultiMediaStream *mmstream)
{
    if (winetest_debug > 1) trace("JoinAMMultiMediaStream(%p)\n", mmstream);
    teststream_mmstream = mmstream;
    return S_OK;
}

static HRESULT WINAPI stream_JoinFilter(IAMMediaStream *iface, IMediaStreamFilter *filter)
{
    if (winetest_debug > 1) trace("JoinFilter(%p)\n", filter);
    teststream_filter = filter;
    return S_OK;
}

static HRESULT WINAPI stream_JoinFilterGraph(IAMMediaStream *iface, IFilterGraph *graph)
{
    if (winetest_debug > 1) trace("JoinFilterGraph(%p)\n", graph);
    teststream_graph = graph;
    return S_OK;
}

static const IAMMediaStreamVtbl stream_vtbl =
{
    stream_QueryInterface,
    stream_AddRef,
    stream_Release,
    stream_GetMultiMediaStream,
    stream_GetInformation,
    stream_SetSameFormat,
    stream_AllocateSample,
    stream_CreateSharedSample,
    stream_SendEndOfStream,
    stream_Initialize,
    stream_SetState,
    stream_JoinAMMultiMediaStream,
    stream_JoinFilter,
    stream_JoinFilterGraph,
};

static IAMMediaStream teststream = {&stream_vtbl};

#define check_enum_stream(a,b,c,d) check_enum_stream_(__LINE__,a,b,c,d)
static void check_enum_stream_(int line, IAMMultiMediaStream *mmstream,
        IMediaStreamFilter *filter, LONG index, IMediaStream *expect)
{
    IMediaStream *stream = NULL, *stream2 = NULL;
    HRESULT hr;

    hr = IAMMultiMediaStream_EnumMediaStreams(mmstream, index, &stream);
    todo_wine ok_(__FILE__, line)(hr == (expect ? S_OK : S_FALSE),
            "IAMMultiMediaStream::EnumMediaStreams() returned %#x.\n", hr);
    hr = IMediaStreamFilter_EnumMediaStreams(filter, index, &stream2);
    todo_wine ok_(__FILE__, line)(hr == (expect ? S_OK : S_FALSE),
            "IMediaStreamFilter::EnumMediaStreams() returned %#x.\n", hr);
    if (hr == S_OK)
    {
        ok_(__FILE__, line)(stream == expect, "Expected stream %p, got %p.\n", expect, stream);
        ok_(__FILE__, line)(stream2 == expect, "Expected stream %p, got %p.\n", expect, stream2);
    }

    if (stream) IMediaStream_Release(stream);
    if (stream2) IMediaStream_Release(stream2);
}

#define check_get_stream(a,b,c,d) check_get_stream_(__LINE__,a,b,c,d)
static void check_get_stream_(int line, IAMMultiMediaStream *mmstream,
        IMediaStreamFilter *filter, const GUID *mspid, IMediaStream *expect)
{
    IMediaStream *stream = NULL, *stream2 = NULL;
    HRESULT hr;

    hr = IAMMultiMediaStream_GetMediaStream(mmstream, mspid, &stream);
    ok_(__FILE__, line)(hr == (expect ? S_OK : MS_E_NOSTREAM),
            "IAMMultiMediaStream::GetMediaStream() returned %#x.\n", hr);
    hr = IMediaStreamFilter_GetMediaStream(filter, mspid, &stream2);
    ok_(__FILE__, line)(hr == (expect ? S_OK : MS_E_NOSTREAM),
            "IMediaStreamFilter::GetMediaStream() returned %#x.\n", hr);
    if (hr == S_OK)
    {
        ok_(__FILE__, line)(stream == expect, "Expected stream %p, got %p.\n", expect, stream);
        ok_(__FILE__, line)(stream2 == expect, "Expected stream %p, got %p.\n", expect, stream2);
    }

    if (stream) IMediaStream_Release(stream);
    if (stream2) IMediaStream_Release(stream2);
}

static void test_add_stream(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IMediaStream *video_stream, *audio_stream, *stream;
    IDirectDrawMediaStream *ddraw_stream;
    IMediaStreamFilter *stream_filter;
    IDirectDraw *ddraw, *ddraw2;
    IEnumFilters *enum_filters;
    IBaseFilter *filters[3];
    IGraphBuilder *graph;
    ULONG ref, count;
    CLSID clsid;
    HRESULT hr;

    hr = IAMMultiMediaStream_GetFilter(mmstream, &stream_filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_EnumMediaStreams(mmstream, 0, NULL);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    hr = IMediaStreamFilter_EnumMediaStreams(stream_filter, 0, NULL);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_GetMediaStream(mmstream, &MSPID_PrimaryAudio, NULL);
    todo_wine ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = IMediaStreamFilter_GetMediaStream(stream_filter, &MSPID_PrimaryAudio, NULL);
    todo_wine ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    check_enum_stream(mmstream, stream_filter, 0, NULL);

    check_get_stream(mmstream, stream_filter, NULL, NULL);
    check_get_stream(mmstream, stream_filter, &MSPID_PrimaryAudio, NULL);
    check_get_stream(mmstream, stream_filter, &MSPID_PrimaryVideo, NULL);
    check_get_stream(mmstream, stream_filter, &test_mspid, NULL);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &test_mspid, 0, &stream);
    ok(hr == MS_E_PURPOSEID, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &video_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    todo_wine ok(hr == MS_E_PURPOSEID, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_EnumMediaStreams(mmstream, 0, NULL);
    todo_wine ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = IMediaStreamFilter_EnumMediaStreams(stream_filter, 0, NULL);
    todo_wine ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_EnumMediaStreams(mmstream, 1, NULL);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    hr = IMediaStreamFilter_EnumMediaStreams(stream_filter, 1, NULL);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    check_enum_stream(mmstream, stream_filter, 0, video_stream);
    check_enum_stream(mmstream, stream_filter, 1, NULL);

    check_get_stream(mmstream, stream_filter, &MSPID_PrimaryVideo, video_stream);
    check_get_stream(mmstream, stream_filter, &MSPID_PrimaryAudio, NULL);
    check_get_stream(mmstream, stream_filter, &test_mspid, NULL);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &audio_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    check_enum_stream(mmstream, stream_filter, 0, video_stream);
    check_enum_stream(mmstream, stream_filter, 1, audio_stream);
    check_enum_stream(mmstream, stream_filter, 2, NULL);

    check_get_stream(mmstream, stream_filter, &MSPID_PrimaryVideo, video_stream);
    check_get_stream(mmstream, stream_filter, &MSPID_PrimaryAudio, audio_stream);
    check_get_stream(mmstream, stream_filter, &test_mspid, NULL);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)&teststream, &IID_IUnknown, 0, &stream);
    ok(hr == MS_E_PURPOSEID, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)&teststream, &test_mspid, 0, &stream);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(stream == (IMediaStream *)&teststream, "Streams didn't match.\n");
    if (hr == S_OK) IMediaStream_Release(stream);
    todo_wine ok(teststream_mmstream == mmstream, "IAMMultiMediaStream objects didn't match.\n");
    todo_wine ok(teststream_filter == stream_filter, "IMediaStreamFilter objects didn't match.\n");
    todo_wine ok(!!teststream_graph, "Expected a non-NULL graph.\n");

    check_enum_stream(mmstream, stream_filter, 0, video_stream);
    check_enum_stream(mmstream, stream_filter, 1, audio_stream);
    check_enum_stream(mmstream, stream_filter, 2, (IMediaStream *)&teststream);
    check_enum_stream(mmstream, stream_filter, 3, NULL);

    check_get_stream(mmstream, stream_filter, &MSPID_PrimaryVideo, video_stream);
    check_get_stream(mmstream, stream_filter, &MSPID_PrimaryAudio, audio_stream);
    todo_wine check_get_stream(mmstream, stream_filter, &test_mspid, (IMediaStream *)&teststream);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    todo_wine ok(hr == MS_E_PURPOSEID, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(!!graph, "Expected a non-NULL graph.\n");
    if (graph)
    {
        hr = IGraphBuilder_EnumFilters(graph, &enum_filters);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        hr = IEnumFilters_Next(enum_filters, 3, filters, &count);
        ok(hr == S_FALSE, "Got hr %#x.\n", hr);
        ok(count == 1, "Got count %u.\n", count);
        ok(filters[0] == (IBaseFilter *)stream_filter,
                "Expected filter %p, got %p.\n", stream_filter, filters[0]);
        IBaseFilter_Release(filters[0]);
        IEnumFilters_Release(enum_filters);
        IGraphBuilder_Release(graph);
    }

    IMediaStreamFilter_Release(stream_filter);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStream_Release(video_stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStream_Release(audio_stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ok(teststream_refcount == 1, "Got outstanding refcount %d.\n", ref);

    /* The return parameter is optional. */

    mmstream = create_ammultimediastream();

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_GetMediaStream(mmstream, &MSPID_PrimaryVideo, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IMediaStream_Release(stream);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    /* Test supplying a DirectDraw object with the primary video stream. */

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(hr == DD_OK, "Got hr %#x.\n", hr);
    mmstream = create_ammultimediastream();

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)ddraw, &MSPID_PrimaryVideo, 0, &video_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStream_QueryInterface(video_stream, &IID_IDirectDrawMediaStream, (void **)&ddraw_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IDirectDrawMediaStream_GetDirectDraw(ddraw_stream, &ddraw2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(ddraw2 == ddraw, "Expected IDirectDraw %p, got %p.\n", ddraw, ddraw2);
    IDirectDraw_Release(ddraw2);
    IDirectDrawMediaStream_Release(ddraw_stream);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStream_Release(video_stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IDirectDraw_Release(ddraw);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &video_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStream_QueryInterface(video_stream, &IID_IDirectDrawMediaStream, (void **)&ddraw_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IDirectDrawMediaStream_GetDirectDraw(ddraw_stream, &ddraw);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!ddraw, "Expected a non-NULL IDirectDraw.\n");
    IDirectDraw_Release(ddraw);
    IDirectDrawMediaStream_Release(ddraw_stream);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStream_Release(video_stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    /* Test the AMMSF_ADDDEFAULTRENDERER flag. No stream is added; however, a
     * new filter will be added to the graph. */

    mmstream = create_ammultimediastream();
    hr = IAMMultiMediaStream_GetFilter(mmstream, &stream_filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* FIXME: This call should not be necessary. */
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo,
            AMMSF_ADDDEFAULTRENDERER, &video_stream);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo,
            AMMSF_ADDDEFAULTRENDERER, NULL);
    ok(hr == MS_E_PURPOSEID, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio,
            AMMSF_ADDDEFAULTRENDERER, &audio_stream);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio,
            AMMSF_ADDDEFAULTRENDERER, NULL);
    ok(hr == S_OK || hr == VFW_E_NO_AUDIO_HARDWARE, "Got hr %#x.\n", hr);

    check_enum_stream(mmstream, stream_filter, 0, NULL);

    check_get_stream(mmstream, stream_filter, &MSPID_PrimaryAudio, NULL);
    check_get_stream(mmstream, stream_filter, &MSPID_PrimaryVideo, NULL);

    if (hr == S_OK)
    {
        hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(!!graph, "Got graph %p.\n", graph);
        hr = IAMMultiMediaStream_GetFilter(mmstream, &stream_filter);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        hr = IGraphBuilder_EnumFilters(graph, &enum_filters);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        hr = IEnumFilters_Next(enum_filters, 3, filters, &count);
        todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
        todo_wine ok(count == 2, "Got count %u.\n", count);
        todo_wine ok(filters[1] == (IBaseFilter *)stream_filter,
                "Expected filter %p, got %p.\n", stream_filter, filters[1]);
        hr = IBaseFilter_GetClassID(filters[0], &clsid);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(IsEqualGUID(&clsid, &CLSID_DSoundRender), "Got unexpected filter %s.\n", wine_dbgstr_guid(&clsid));
        IBaseFilter_Release(filters[0]);
        IMediaStreamFilter_Release(stream_filter);
        IEnumFilters_Release(enum_filters);
        IGraphBuilder_Release(graph);
    }

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &test_mspid,
            AMMSF_ADDDEFAULTRENDERER, &audio_stream);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    IMediaStreamFilter_Release(stream_filter);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_media_streams(void)
{
    IAMMultiMediaStream *pams;
    HRESULT hr;
    IMediaStream *video_stream = NULL;
    IMediaStream *audio_stream = NULL;
    IMediaStreamFilter* media_stream_filter = NULL;

    if (!(pams = create_ammultimediastream()))
        return;
    if (!create_directdraw())
    {
        IAMMultiMediaStream_Release(pams);
        return;
    }

    hr = IAMMultiMediaStream_Initialize(pams, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "IAMMultiMediaStream_Initialize returned: %x\n", hr);

    /* Retrieve media stream filter */
    hr = IAMMultiMediaStream_GetFilter(pams, NULL);
    ok(hr == E_POINTER, "IAMMultiMediaStream_GetFilter returned: %x\n", hr);
    hr = IAMMultiMediaStream_GetFilter(pams, &media_stream_filter);
    ok(hr == S_OK, "IAMMultiMediaStream_GetFilter returned: %x\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(pams, NULL, &MSPID_PrimaryVideo, 0, NULL);
    ok(hr == S_OK, "IAMMultiMediaStream_AddMediaStream returned: %x\n", hr);
    hr = IAMMultiMediaStream_GetMediaStream(pams, &MSPID_PrimaryVideo, &video_stream);
    ok(hr == S_OK, "IAMMultiMediaStream_GetMediaStream returned: %x\n", hr);

    /* Check interfaces and samples for video */
    if (video_stream)
    {
        IAMMediaStream* am_media_stream;
        IMultiMediaStream *multi_media_stream;
        IPin *pin = NULL;
        IAudioMediaStream* audio_media_stream;
        IDirectDrawMediaStream *ddraw_stream = NULL;
        IDirectDrawStreamSample *ddraw_sample = NULL;

        hr = IMediaStream_QueryInterface(video_stream, &IID_IAMMediaStream, (LPVOID*)&am_media_stream);
        ok(hr == S_OK, "IMediaStream_QueryInterface returned: %x\n", hr);
        ok((void*)am_media_stream == (void*)video_stream, "Not same interface, got %p expected %p\n", am_media_stream, video_stream);

        hr = IAMMediaStream_GetMultiMediaStream(am_media_stream, NULL);
        ok(hr == E_POINTER, "Expected E_POINTER, got %x\n", hr);

        multi_media_stream = (void *)0xdeadbeef;
        hr = IAMMediaStream_GetMultiMediaStream(am_media_stream, &multi_media_stream);
        ok(hr == S_OK, "IAMMediaStream_GetMultiMediaStream returned: %x\n", hr);
        ok((void *)multi_media_stream == (void *)pams, "Expected %p, got %p\n", pams, multi_media_stream);
        IMultiMediaStream_Release(multi_media_stream);

        IAMMediaStream_Release(am_media_stream);

        hr = IMediaStream_QueryInterface(video_stream, &IID_IPin, (void **)&pin);
        ok(hr == S_OK, "IMediaStream_QueryInterface returned: %x\n", hr);

        IPin_Release(pin);

        hr = IMediaStream_QueryInterface(video_stream, &IID_IAudioMediaStream, (LPVOID*)&audio_media_stream);
        ok(hr == E_NOINTERFACE, "IMediaStream_QueryInterface returned: %x\n", hr);

        hr = IMediaStream_QueryInterface(video_stream, &IID_IDirectDrawMediaStream, (LPVOID*)&ddraw_stream);
        ok(hr == S_OK, "IMediaStream_QueryInterface returned: %x\n", hr);

        if (SUCCEEDED(hr))
        {
            DDSURFACEDESC current_format, desired_format;
            IDirectDrawPalette *palette;
            DWORD flags;

            hr = IDirectDrawMediaStream_GetFormat(ddraw_stream, &current_format, &palette, &desired_format, &flags);
            ok(hr == MS_E_NOSTREAM, "IDirectDrawoMediaStream_GetFormat returned: %x\n", hr);

            hr = IDirectDrawMediaStream_CreateSample(ddraw_stream, NULL, NULL, 0, &ddraw_sample);
            ok(hr == S_OK, "IDirectDrawMediaStream_CreateSample returned: %x\n", hr);

            hr = IDirectDrawMediaStream_GetMultiMediaStream(ddraw_stream, NULL);
            ok(hr == E_POINTER, "Expected E_POINTER, got %x\n", hr);

            multi_media_stream = (void *)0xdeadbeef;
            hr = IDirectDrawMediaStream_GetMultiMediaStream(ddraw_stream, &multi_media_stream);
            ok(hr == S_OK, "IDirectDrawMediaStream_GetMultiMediaStream returned: %x\n", hr);
            ok((void *)multi_media_stream == (void *)pams, "Expected %p, got %p\n", pams, multi_media_stream);
            IMultiMediaStream_Release(multi_media_stream);
        }

        if (ddraw_sample)
            IDirectDrawStreamSample_Release(ddraw_sample);
        if (ddraw_stream)
            IDirectDrawMediaStream_Release(ddraw_stream);
    }

    hr = IAMMultiMediaStream_AddMediaStream(pams, NULL, &MSPID_PrimaryAudio, 0, NULL);
    ok(hr == S_OK, "IAMMultiMediaStream_AddMediaStream returned: %x\n", hr);
    hr = IAMMultiMediaStream_GetMediaStream(pams, &MSPID_PrimaryAudio, &audio_stream);
    ok(hr == S_OK, "IAMMultiMediaStream_GetMediaStream returned: %x\n", hr);

   /* Check interfaces and samples for audio */
    if (audio_stream)
    {
        IAMMediaStream* am_media_stream;
        IMultiMediaStream *multi_media_stream;
        IPin *pin = NULL;
        IDirectDrawMediaStream* ddraw_stream = NULL;
        IAudioMediaStream* audio_media_stream = NULL;
        IAudioStreamSample *audio_sample = NULL;

        hr = IMediaStream_QueryInterface(audio_stream, &IID_IAMMediaStream, (LPVOID*)&am_media_stream);
        ok(hr == S_OK, "IMediaStream_QueryInterface returned: %x\n", hr);
        ok((void*)am_media_stream == (void*)audio_stream, "Not same interface, got %p expected %p\n", am_media_stream, audio_stream);

        hr = IAMMediaStream_GetMultiMediaStream(am_media_stream, NULL);
        ok(hr == E_POINTER, "Expected E_POINTER, got %x\n", hr);

        multi_media_stream = (void *)0xdeadbeef;
        hr = IAMMediaStream_GetMultiMediaStream(am_media_stream, &multi_media_stream);
        ok(hr == S_OK, "IAMMediaStream_GetMultiMediaStream returned: %x\n", hr);
        ok((void *)multi_media_stream == (void *)pams, "Expected %p, got %p\n", pams, multi_media_stream);
        IMultiMediaStream_Release(multi_media_stream);

        IAMMediaStream_Release(am_media_stream);

        hr = IMediaStream_QueryInterface(audio_stream, &IID_IPin, (void **)&pin);
        ok(hr == S_OK, "IMediaStream_QueryInterface returned: %x\n", hr);

        IPin_Release(pin);

        hr = IMediaStream_QueryInterface(audio_stream, &IID_IDirectDrawMediaStream, (LPVOID*)&ddraw_stream);
        ok(hr == E_NOINTERFACE, "IMediaStream_QueryInterface returned: %x\n", hr);

        hr = IMediaStream_QueryInterface(audio_stream, &IID_IAudioMediaStream, (LPVOID*)&audio_media_stream);
        ok(hr == S_OK, "IMediaStream_QueryInterface returned: %x\n", hr);

        if (SUCCEEDED(hr))
        {
            IAudioData* audio_data = NULL;
            WAVEFORMATEX format;

            hr = CoCreateInstance(&CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER, &IID_IAudioData, (void **)&audio_data);
            ok(hr == S_OK, "CoCreateInstance returned: %x\n", hr);

            hr = IAudioMediaStream_GetFormat(audio_media_stream, NULL);
            ok(hr == E_POINTER, "IAudioMediaStream_GetFormat returned: %x\n", hr);
            hr = IAudioMediaStream_GetFormat(audio_media_stream, &format);
            ok(hr == MS_E_NOSTREAM, "IAudioMediaStream_GetFormat returned: %x\n", hr);

            hr = IAudioMediaStream_CreateSample(audio_media_stream, NULL, 0, &audio_sample);
            ok(hr == E_POINTER, "IAudioMediaStream_CreateSample returned: %x\n", hr);
            hr = IAudioMediaStream_CreateSample(audio_media_stream, audio_data, 0, &audio_sample);
            ok(hr == S_OK, "IAudioMediaStream_CreateSample returned: %x\n", hr);

            hr = IAudioMediaStream_GetMultiMediaStream(audio_media_stream, NULL);
            ok(hr == E_POINTER, "Expected E_POINTER, got %x\n", hr);

            multi_media_stream = (void *)0xdeadbeef;
            hr = IAudioMediaStream_GetMultiMediaStream(audio_media_stream, &multi_media_stream);
            ok(hr == S_OK, "IAudioMediaStream_GetMultiMediaStream returned: %x\n", hr);
            ok((void *)multi_media_stream == (void *)pams, "Expected %p, got %p\n", pams, multi_media_stream);
            IMultiMediaStream_Release(multi_media_stream);

            if (audio_data)
                IAudioData_Release(audio_data);
            if (audio_sample)
                IAudioStreamSample_Release(audio_sample);
            if (audio_media_stream)
                IAudioMediaStream_Release(audio_media_stream);
        }
    }

    if (media_stream_filter)
    {
        IEnumPins *enum_pins;

        hr = IMediaStreamFilter_EnumPins(media_stream_filter, &enum_pins);
        ok(hr == S_OK, "IBaseFilter_EnumPins returned: %x\n", hr);
        if (hr == S_OK)
        {
            IPin* pins[3] = { NULL, NULL, NULL };
            ULONG nb_pins;
            ULONG expected_nb_pins = audio_stream ? 2 : 1;
            int i;

            hr = IEnumPins_Next(enum_pins, 3, pins, &nb_pins);
            ok(SUCCEEDED(hr), "IEnumPins_Next returned: %x\n", hr);
            ok(nb_pins == expected_nb_pins, "Number of pins is %u instead of %u\n", nb_pins, expected_nb_pins);
            for (i = 0; i < min(nb_pins, expected_nb_pins); i++)
            {
                IPin* pin;

                hr = IPin_ConnectedTo(pins[i], &pin);
                ok(hr == VFW_E_NOT_CONNECTED, "IPin_ConnectedTo returned: %x\n", hr);
                IPin_Release(pins[i]);
            }
            IEnumPins_Release(enum_pins);
        }
    }

    /* Test open file with no filename */
    hr = IAMMultiMediaStream_OpenFile(pams, NULL, 0);
    ok(hr == E_POINTER, "IAMMultiMediaStream_OpenFile returned %x instead of %x\n", hr, E_POINTER);

    if (video_stream)
        IMediaStream_Release(video_stream);
    if (audio_stream)
        IMediaStream_Release(audio_stream);
    if (media_stream_filter)
        IMediaStreamFilter_Release(media_stream_filter);

    release_directdraw();
    IAMMultiMediaStream_Release(pams);
}

static void test_enum_pins(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IMediaStreamFilter *filter;
    IEnumPins *enum1, *enum2;
    IMediaStream *stream;
    IPin *pins[3], *pin;
    ULONG ref, count;
    HRESULT hr;

    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = get_refcount(filter);
    todo_wine ok(ref == 3, "Got unexpected refcount %d.\n", ref);

    hr = IMediaStreamFilter_EnumPins(filter, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IMediaStreamFilter_EnumPins(filter, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(filter);
    todo_wine ok(ref == 3, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    hr = IEnumPins_Next(enum1, 1, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 0);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Reset() isn't enough; we have to call EnumPins() again to see the updated
     * pin count. */
    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumPins_Release(enum1);

    hr = IMediaStreamFilter_EnumPins(filter, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = get_refcount(filter);
    todo_wine ok(ref == 4, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 4, "Got unexpected refcount %d.\n", ref);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(pins[0] == pin, "Expected pin %p, got %p.\n", pin, pins[0]);
    ref = get_refcount(filter);
    todo_wine ok(ref == 4, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 5, "Got unexpected refcount %d.\n", ref);
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    ok(pins[0] == pin, "Expected pin %p, got %p.\n", pin, pins[0]);
    IPin_Release(pins[0]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    ok(pins[0] == pin, "Expected pin %p, got %p.\n", pin, pins[0]);
    IPin_Release(pins[0]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum2, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(pins[0] == pin, "Expected pin %p, got %p.\n", pin, pins[0]);
    IPin_Release(pins[0]);

    IEnumPins_Release(enum2);
    IEnumPins_Release(enum1);

    IMediaStreamFilter_Release(filter);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IMediaStream_Release(stream);
    ref = IPin_Release(pin);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_find_pin(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IMediaStreamFilter *filter;
    IMediaStream *stream;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStreamFilter_FindPin(filter, primary_video_sink_id, &pin2);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStreamFilter_FindPin(filter, primary_video_sink_id, &pin2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(pin2 == pin, "Expected pin %p, got %p.\n", pin, pin2);

    IPin_Release(pin2);
    IPin_Release(pin);
    IMediaStreamFilter_Release(filter);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_pin_info(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IMediaStreamFilter *filter;
    IMediaStream *stream;
    PIN_DIRECTION dir;
    ULONG ref, count;
    PIN_INFO info;
    HRESULT hr;
    WCHAR *id;
    IPin *pin;

    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(info.pFilter == (IBaseFilter *)filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
    ok(!wcscmp(info.achName, primary_video_sink_id), "Got name %s.\n", wine_dbgstr_w(info.achName));
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!wcscmp(id, primary_video_sink_id), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, &count);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    IPin_Release(pin);
    IMediaStream_Release(stream);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_QueryPinInfo(pin, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(info.pFilter == (IBaseFilter *)filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
    ok(!wcscmp(info.achName, primary_audio_sink_id), "Got name %s.\n", wine_dbgstr_w(info.achName));
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!wcscmp(id, primary_audio_sink_id), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, &count);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    IPin_Release(pin);
    IMediaStream_Release(stream);

    IMediaStreamFilter_Release(filter);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static IUnknown *graph_inner_unk;
static IFilterGraph2 *graph_inner;
static LONG graph_refcount = 1;
static unsigned int got_add_filter;
static IBaseFilter *graph_filter;
static WCHAR graph_filter_name[128];

static HRESULT WINAPI graph_QueryInterface(IFilterGraph2 *iface, REFIID iid, void **out)
{
    if (winetest_debug > 1) trace("QueryInterface(%s)\n", wine_dbgstr_guid(iid));
    if (IsEqualGUID(iid, &IID_IFilterGraph2)
            || IsEqualGUID(iid, &IID_IGraphBuilder)
            || IsEqualGUID(iid, &IID_IFilterGraph)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        *out = iface;
        IFilterGraph2_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IMediaSeeking)
            || IsEqualGUID(iid, &IID_IMediaControl)
            || IsEqualGUID(iid, &IID_IMediaEventEx))
    {
        return IUnknown_QueryInterface(graph_inner_unk, iid, out);
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI graph_AddRef(IFilterGraph2 *iface)
{
    return InterlockedIncrement(&graph_refcount);
}

static ULONG WINAPI graph_Release(IFilterGraph2 *iface)
{
    return InterlockedDecrement(&graph_refcount);
}

static HRESULT WINAPI graph_AddFilter(IFilterGraph2 *iface, IBaseFilter *filter, const WCHAR *name)
{
    if (winetest_debug > 1) trace("AddFilter(%p, %s)\n", filter, wine_dbgstr_w(name));
    ++got_add_filter;
    graph_filter = filter;
    if (name)
        wcscpy(graph_filter_name, name);
    else
        graph_filter_name[0] = 0;
    return IFilterGraph2_AddFilter(graph_inner, filter, name);
}

static HRESULT WINAPI graph_RemoveFilter(IFilterGraph2 *iface, IBaseFilter *filter)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_EnumFilters(IFilterGraph2 *iface, IEnumFilters **enumfilters)
{
    if (winetest_debug > 1) trace("EnumFilters()\n");
    return IFilterGraph2_EnumFilters(graph_inner, enumfilters);
}

static HRESULT WINAPI graph_FindFilterByName(IFilterGraph2 *iface, const WCHAR *name, IBaseFilter **filter)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_ConnectDirect(IFilterGraph2 *iface, IPin *source, IPin *sink, const AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_Reconnect(IFilterGraph2 *iface, IPin *pin)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_Disconnect(IFilterGraph2 *iface, IPin *pin)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_SetDefaultSyncSource(IFilterGraph2 *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_Connect(IFilterGraph2 *iface, IPin *source, IPin *sink)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_Render(IFilterGraph2 *iface, IPin *source)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_RenderFile(IFilterGraph2 *iface, const WCHAR *filename, const WCHAR *playlist)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_AddSourceFilter(IFilterGraph2 *iface,
        const WCHAR *filename, const WCHAR *filter_name, IBaseFilter **filter)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_SetLogFile(IFilterGraph2 *iface, DWORD_PTR file)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_Abort(IFilterGraph2 *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_ShouldOperationContinue(IFilterGraph2 *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_AddSourceFilterForMoniker(IFilterGraph2 *iface,
        IMoniker *moniker, IBindCtx *bind_ctx, const WCHAR *filter_name, IBaseFilter **filter)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_ReconnectEx(IFilterGraph2 *iface, IPin *pin, const AM_MEDIA_TYPE *mt)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI graph_RenderEx(IFilterGraph2 *iface, IPin *pin, DWORD flags, DWORD *context)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IFilterGraph2Vtbl graph_vtbl =
{
    graph_QueryInterface,
    graph_AddRef,
    graph_Release,
    graph_AddFilter,
    graph_RemoveFilter,
    graph_EnumFilters,
    graph_FindFilterByName,
    graph_ConnectDirect,
    graph_Reconnect,
    graph_Disconnect,
    graph_SetDefaultSyncSource,
    graph_Connect,
    graph_Render,
    graph_RenderFile,
    graph_AddSourceFilter,
    graph_SetLogFile,
    graph_Abort,
    graph_ShouldOperationContinue,
    graph_AddSourceFilterForMoniker,
    graph_ReconnectEx,
    graph_RenderEx,
};

static void test_initialize(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IFilterGraph2 graph = {&graph_vtbl};
    IMediaStreamFilter *filter;
    IGraphBuilder *ret_graph;
    IMediaStream *stream;
    STREAM_TYPE type;
    HRESULT hr;
    ULONG ref;

    ret_graph = (IGraphBuilder *)0xdeadbeef;
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &ret_graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!ret_graph, "Got unexpected graph %p.\n", ret_graph);

    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!filter, "Expected a non-NULL filter.");
    IMediaStreamFilter_Release(filter);

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_WRITE, 0, NULL);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_TRANSFORM, 0, NULL);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    ret_graph = (IGraphBuilder *)0xdeadbeef;
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &ret_graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(!ret_graph, "Got unexpected graph %p.\n", ret_graph);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    type = 0xdeadbeef;
    hr = IMediaStream_GetInformation(stream, NULL, &type);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(type == STREAMTYPE_READ, "Got type %u.\n", type);
    IMediaStream_Release(stream);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_WRITE, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_WRITE, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_TRANSFORM, 0, NULL);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    ret_graph = (IGraphBuilder *)0xdeadbeef;
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &ret_graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(!ret_graph, "Got unexpected graph %p.\n", ret_graph);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    type = 0xdeadbeef;
    hr = IMediaStream_GetInformation(stream, NULL, &type);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(type == STREAMTYPE_WRITE, "Got type %u.\n", type);
    IMediaStream_Release(stream);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_TRANSFORM, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_TRANSFORM, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_WRITE, 0, NULL);
    todo_wine ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    ret_graph = (IGraphBuilder *)0xdeadbeef;
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &ret_graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(!ret_graph, "Got unexpected graph %p.\n", ret_graph);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    type = 0xdeadbeef;
    hr = IMediaStream_GetInformation(stream, NULL, &type);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(type == STREAMTYPE_TRANSFORM, "Got type %u.\n", type);
    IMediaStream_Release(stream);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    type = 0xdeadbeef;
    hr = IMediaStream_GetInformation(stream, NULL, &type);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(type == STREAMTYPE_READ, "Got type %u.\n", type);

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_TRANSFORM, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    type = 0xdeadbeef;
    hr = IMediaStream_GetInformation(stream, NULL, &type);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(type == STREAMTYPE_READ, "Got type %u.\n", type);

    IMediaStream_Release(stream);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    /* Test with a custom filter graph. */

    mmstream = create_ammultimediastream();

    CoCreateInstance(&CLSID_FilterGraph, (IUnknown *)&graph, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&graph_inner_unk);
    IUnknown_QueryInterface(graph_inner_unk, &IID_IFilterGraph2, (void **)&graph_inner);

    ret_graph = (IGraphBuilder *)0xdeadbeef;
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &ret_graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!ret_graph, "Got unexpected graph %p.\n", ret_graph);

    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!filter, "Expected a non-NULL filter.");

    got_add_filter = 0;
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, (IGraphBuilder *)&graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(got_add_filter == 1, "Got %d calls to IGraphBuilder::AddFilter().\n", got_add_filter);
    ok(graph_filter == (IBaseFilter *)filter, "Got filter %p.\n", filter);
    ok(!wcscmp(graph_filter_name, L"MediaStreamFilter"), "Got unexpected name %s.\n", wine_dbgstr_w(graph_filter_name));

    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &ret_graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(ret_graph == (IGraphBuilder *)&graph, "Got unexpected graph %p.\n", ret_graph);
    IGraphBuilder_Release(ret_graph);

    got_add_filter = 0;
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!got_add_filter, "Got %d calls to IGraphBuilder::AddFilter().\n", got_add_filter);

    got_add_filter = 0;
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!got_add_filter, "Got %d calls to IGraphBuilder::AddFilter().\n", got_add_filter);

    IMediaStreamFilter_Release(filter);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IFilterGraph2_Release(graph_inner);
    ok(graph_refcount == 1, "Got outstanding refcount %d.\n", graph_refcount);
    IUnknown_Release(graph_inner_unk);
}

static void test_enum_media_types(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IEnumMediaTypes *enum1, *enum2;
    AM_MEDIA_TYPE *mts[2];
    IMediaStream *stream;
    ULONG ref, count;
    HRESULT hr;
    IPin *pin;

    /* FIXME: This call should not be necessary. */
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 0, mts, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumMediaTypes_Next(enum2, 1, mts, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    CoTaskMemFree(mts[0]);

    IEnumMediaTypes_Release(enum2);
    IEnumMediaTypes_Release(enum1);
    IPin_Release(pin);
    IMediaStream_Release(stream);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 0, mts, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 2, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumMediaTypes_Next(enum2, 1, mts, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    CoTaskMemFree(mts[0]->pbFormat);
    CoTaskMemFree(mts[0]);

    IEnumMediaTypes_Release(enum2);
    IEnumMediaTypes_Release(enum1);
    IPin_Release(pin);
    IMediaStream_Release(stream);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_media_types(void)
{
    static const VIDEOINFOHEADER req_vih = {};
    static const WAVEFORMATEX expect_wfx =
    {
        .wFormatTag = WAVE_FORMAT_PCM,
        .nChannels = 1,
        .nSamplesPerSec = 11025,
        .nAvgBytesPerSec = 11025 * 2,
        .nBlockAlign = 2,
        .wBitsPerSample = 16,
        .cbSize = 0,
    };
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IEnumMediaTypes *enummt;
    IMediaStream *stream;
    AM_MEDIA_TYPE *pmt;
    ULONG ref, count;
    unsigned int i;
    HRESULT hr;
    IPin *pin;

    static const GUID *rejected_subtypes[] =
    {
        &MEDIASUBTYPE_RGB1,
        &MEDIASUBTYPE_RGB4,
        &MEDIASUBTYPE_RGB565,
        &MEDIASUBTYPE_RGB555,
        &MEDIASUBTYPE_RGB24,
        &MEDIASUBTYPE_RGB32,
        &MEDIASUBTYPE_ARGB32,
        &MEDIASUBTYPE_ARGB1555,
        &MEDIASUBTYPE_ARGB4444,
        &GUID_NULL,
    };

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    ok(IsEqualGUID(&pmt->majortype, &MEDIATYPE_Video), "Got major type %s\n",
            wine_dbgstr_guid(&pmt->majortype));
    ok(IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_RGB8), "Got subtype %s\n",
            wine_dbgstr_guid(&pmt->subtype));
    ok(pmt->bFixedSizeSamples == TRUE, "Got fixed size %d.\n", pmt->bFixedSizeSamples);
    ok(!pmt->bTemporalCompression, "Got temporal compression %d.\n", pmt->bTemporalCompression);
    ok(pmt->lSampleSize == 10000, "Got sample size %u.\n", pmt->lSampleSize);
    ok(IsEqualGUID(&pmt->formattype, &GUID_NULL), "Got format type %s.\n",
            wine_dbgstr_guid(&pmt->formattype));
    ok(!pmt->pUnk, "Got pUnk %p.\n", pmt->pUnk);

    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);

    pmt->formattype = FORMAT_VideoInfo;
    pmt->cbFormat = sizeof(VIDEOINFOHEADER);
    pmt->pbFormat = (BYTE *)&req_vih;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    pmt->bFixedSizeSamples = FALSE;
    pmt->bTemporalCompression = TRUE;
    pmt->lSampleSize = 123;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    pmt->majortype = MEDIATYPE_NULL;
    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);
    pmt->majortype = MEDIATYPE_Audio;
    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);
    pmt->majortype = MEDIATYPE_Stream;
    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);
    pmt->majortype = MEDIATYPE_Video;

    for (i = 0; i < ARRAY_SIZE(rejected_subtypes); ++i)
    {
        pmt->subtype = *rejected_subtypes[i];
        hr = IPin_QueryAccept(pin, pmt);
        todo_wine ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x for subtype %s.\n",
                hr, wine_dbgstr_guid(rejected_subtypes[i]));
    }

    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumMediaTypes_Release(enummt);
    IPin_Release(pin);
    IMediaStream_Release(stream);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    ok(IsEqualGUID(&pmt->majortype, &MEDIATYPE_Audio), "Got major type %s\n",
            wine_dbgstr_guid(&pmt->majortype));
    todo_wine ok(IsEqualGUID(&pmt->subtype, &GUID_NULL), "Got subtype %s\n",
            wine_dbgstr_guid(&pmt->subtype));
    todo_wine ok(pmt->bFixedSizeSamples == TRUE, "Got fixed size %d.\n", pmt->bFixedSizeSamples);
    ok(!pmt->bTemporalCompression, "Got temporal compression %d.\n", pmt->bTemporalCompression);
    todo_wine ok(pmt->lSampleSize == 2, "Got sample size %u.\n", pmt->lSampleSize);
    todo_wine ok(IsEqualGUID(&pmt->formattype, &FORMAT_WaveFormatEx), "Got format type %s.\n",
            wine_dbgstr_guid(&pmt->formattype));
    ok(!pmt->pUnk, "Got pUnk %p.\n", pmt->pUnk);
    todo_wine ok(pmt->cbFormat == sizeof(WAVEFORMATEX), "Got format size %u.\n", pmt->cbFormat);
    ok(!memcmp(pmt->pbFormat, &expect_wfx, pmt->cbFormat), "Format blocks didn't match.\n");

    hr = IPin_QueryAccept(pin, pmt);
    todo_wine ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumMediaTypes_Release(enummt);
    IPin_Release(pin);
    IMediaStream_Release(stream);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_IDirectDrawStreamSample(void)
{
    DDSURFACEDESC desc = { sizeof(desc) };
    IAMMultiMediaStream *pams;
    HRESULT hr;
    IMediaStream *pvidstream = NULL;
    IDirectDrawMediaStream *pddstream = NULL;
    IDirectDrawStreamSample *pddsample = NULL;
    IDirectDrawSurface7 *surface7;
    IDirectDrawSurface *surface, *surface2;
    IDirectDraw *ddraw, *ddraw2;
    IDirectDraw7 *ddraw7;
    RECT rect;

    if (!(pams = create_ammultimediastream()))
        return;
    if (!create_directdraw())
    {
        IAMMultiMediaStream_Release(pams);
        return;
    }

    hr = IAMMultiMediaStream_Initialize(pams, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(pams, (IUnknown*)pdd7, &MSPID_PrimaryVideo, 0, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IAMMultiMediaStream_GetMediaStream(pams, &MSPID_PrimaryVideo, &pvidstream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    if (FAILED(hr)) goto error;

    hr = IMediaStream_QueryInterface(pvidstream, &IID_IDirectDrawMediaStream, (LPVOID*)&pddstream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    if (FAILED(hr)) goto error;

    hr = IDirectDrawMediaStream_GetDirectDraw(pddstream, &ddraw);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDirectDrawMediaStream_GetDirectDraw(pddstream, &ddraw2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(ddraw == ddraw2, "got %p, %p\n", ddraw, ddraw2);

    hr = IDirectDraw_QueryInterface(ddraw, &IID_IDirectDraw7, (void **)&ddraw7);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(ddraw7 == pdd7, "Got IDirectDraw instance %p, expected %p.\n", ddraw7, pdd7);
    IDirectDraw7_Release(ddraw7);

    IDirectDraw_Release(ddraw2);
    IDirectDraw_Release(ddraw);

    hr = IDirectDrawMediaStream_CreateSample(pddstream, NULL, NULL, 0, &pddsample);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    surface = NULL;
    hr = IDirectDrawStreamSample_GetSurface(pddsample, &surface, &rect);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(surface != NULL, "got %p\n", surface);

    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirectDrawSurface7, (void **)&surface7);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDirectDrawSurface7_Release(surface7);

    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &desc);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(desc.dwWidth == 100, "width %d\n", desc.dwWidth);
    ok(desc.dwHeight == 100, "height %d\n", desc.dwHeight);
    ok(desc.ddpfPixelFormat.dwFlags == DDPF_RGB, "format flags %08x\n", desc.ddpfPixelFormat.dwFlags);
    ok(desc.ddpfPixelFormat.dwRGBBitCount, "dwRGBBitCount %d\n", desc.ddpfPixelFormat.dwRGBBitCount);
    IDirectDrawSurface_Release(surface);
    IDirectDrawStreamSample_Release(pddsample);

    hr = IDirectDrawSurface7_QueryInterface(pdds7, &IID_IDirectDrawSurface, (void **)&surface);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    EXPECT_REF(surface, 1);
    hr = IDirectDrawMediaStream_CreateSample(pddstream, surface, NULL, 0, &pddsample);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(surface, 2);

    surface2 = NULL;
    SetRectEmpty(&rect);
    hr = IDirectDrawStreamSample_GetSurface(pddsample, &surface2, &rect);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(surface == surface2, "got %p\n", surface2);
    ok(rect.right > 0 && rect.bottom > 0, "got %d, %d\n", rect.right, rect.bottom);
    EXPECT_REF(surface, 3);
    IDirectDrawSurface_Release(surface2);

    hr = IDirectDrawStreamSample_GetSurface(pddsample, NULL, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IDirectDrawStreamSample_Release(pddsample);
    IDirectDrawSurface_Release(surface);

error:
    if (pddstream)
        IDirectDrawMediaStream_Release(pddstream);
    if (pvidstream)
        IMediaStream_Release(pvidstream);

    release_directdraw();
    IAMMultiMediaStream_Release(pams);
}

static IUnknown *create_audio_data(void)
{
    IUnknown *audio_data = NULL;
    HRESULT result = CoCreateInstance(&CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&audio_data);
    ok(S_OK == result, "got 0x%08x\n", result);
    return audio_data;
}

static void test_audiodata_query_interface(void)
{
    IUnknown *unknown = create_audio_data();
    IMemoryData *memory_data = NULL;
    IAudioData *audio_data = NULL;

    HRESULT result;

    result = IUnknown_QueryInterface(unknown, &IID_IMemoryData, (void **)&memory_data);
    ok(E_NOINTERFACE == result, "got 0x%08x\n", result);

    result = IUnknown_QueryInterface(unknown, &IID_IAudioData, (void **)&audio_data);
    ok(S_OK == result, "got 0x%08x\n", result);
    if (S_OK == result)
    {
        result = IAudioData_QueryInterface(audio_data, &IID_IMemoryData, (void **)&memory_data);
        ok(E_NOINTERFACE == result, "got 0x%08x\n", result);

        IAudioData_Release(audio_data);
    }

    IUnknown_Release(unknown);
}

static void test_audiodata_get_info(void)
{
    IUnknown *unknown = create_audio_data();
    IAudioData *audio_data = NULL;

    HRESULT result;

    result = IUnknown_QueryInterface(unknown, &IID_IAudioData, (void **)&audio_data);
    if (FAILED(result))
    {
        /* test_audiodata_query_interface handles this case */
        skip("No IAudioData\n");
        goto out_unknown;
    }

    result = IAudioData_GetInfo(audio_data, NULL, NULL, NULL);
    ok(MS_E_NOTINIT == result, "got 0x%08x\n", result);

    IAudioData_Release(audio_data);

out_unknown:
    IUnknown_Release(unknown);
}

static void test_audiodata_set_buffer(void)
{
    IUnknown *unknown = create_audio_data();
    IAudioData *audio_data = NULL;
    BYTE buffer[100] = {0};
    DWORD length = 0;
    BYTE *data = NULL;

    HRESULT result;

    result = IUnknown_QueryInterface(unknown, &IID_IAudioData, (void **)&audio_data);
    if (FAILED(result))
    {
        /* test_audiodata_query_interface handles this case */
        skip("No IAudioData\n");
        goto out_unknown;
    }

    result = IAudioData_SetBuffer(audio_data, 100, NULL, 0);
    ok(S_OK == result, "got 0x%08x\n", result);

    data = (BYTE *)0xdeadbeef;
    length = 0xdeadbeef;
    result = IAudioData_GetInfo(audio_data, &length, &data, NULL);
    ok(S_OK == result, "got 0x%08x\n", result);
    ok(100 == length, "got %u\n", length);
    ok(NULL != data, "got %p\n", data);

    result = IAudioData_SetBuffer(audio_data, 0, buffer, 0);
    ok(E_INVALIDARG == result, "got 0x%08x\n", result);

    result = IAudioData_SetBuffer(audio_data, sizeof(buffer), buffer, 0);
    ok(S_OK == result, "got 0x%08x\n", result);

    data = (BYTE *)0xdeadbeef;
    length = 0xdeadbeef;
    result = IAudioData_GetInfo(audio_data, &length, &data, NULL);
    ok(S_OK == result, "got 0x%08x\n", result);
    ok(sizeof(buffer) == length, "got %u\n", length);
    ok(buffer == data, "got %p\n", data);

    IAudioData_Release(audio_data);

out_unknown:
    IUnknown_Release(unknown);
}

static void test_audiodata_set_actual(void)
{
    IUnknown *unknown = create_audio_data();
    IAudioData *audio_data = NULL;
    BYTE buffer[100] = {0};
    DWORD actual_data = 0;

    HRESULT result;

    result = IUnknown_QueryInterface(unknown, &IID_IAudioData, (void **)&audio_data);
    if (FAILED(result))
    {
        /* test_audiodata_query_interface handles this case */
        skip("No IAudioData\n");
        goto out_unknown;
    }

    result = IAudioData_SetActual(audio_data, 0);
    ok(S_OK == result, "got 0x%08x\n", result);

    result = IAudioData_SetBuffer(audio_data, sizeof(buffer), buffer, 0);
    ok(S_OK == result, "got 0x%08x\n", result);

    result = IAudioData_SetActual(audio_data, sizeof(buffer) + 1);
    ok(E_INVALIDARG == result, "got 0x%08x\n", result);

    result = IAudioData_SetActual(audio_data, sizeof(buffer));
    ok(S_OK == result, "got 0x%08x\n", result);

    actual_data = 0xdeadbeef;
    result = IAudioData_GetInfo(audio_data, NULL, NULL, &actual_data);
    ok(S_OK == result, "got 0x%08x\n", result);
    ok(sizeof(buffer) == actual_data, "got %u\n", actual_data);

    result = IAudioData_SetActual(audio_data, 0);
    ok(S_OK == result, "got 0x%08x\n", result);

    actual_data = 0xdeadbeef;
    result = IAudioData_GetInfo(audio_data, NULL, NULL, &actual_data);
    ok(S_OK == result, "got 0x%08x\n", result);
    ok(0 == actual_data, "got %u\n", actual_data);

    IAudioData_Release(audio_data);

out_unknown:
    IUnknown_Release(unknown);
}

static void test_audiodata_get_format(void)
{
    IUnknown *unknown = create_audio_data();
    IAudioData *audio_data = NULL;
    WAVEFORMATEX wave_format = {0};

    HRESULT result;

    result = IUnknown_QueryInterface(unknown, &IID_IAudioData, (void **)&audio_data);
    if (FAILED(result))
    {
        /* test_audiodata_query_interface handles this case */
        skip("No IAudioData\n");
        goto out_unknown;
    }

    result = IAudioData_GetFormat(audio_data, NULL);
    ok(E_POINTER == result, "got 0x%08x\n", result);

    wave_format.wFormatTag = 0xdead;
    wave_format.nChannels = 0xdead;
    wave_format.nSamplesPerSec = 0xdeadbeef;
    wave_format.nAvgBytesPerSec = 0xdeadbeef;
    wave_format.nBlockAlign = 0xdead;
    wave_format.wBitsPerSample = 0xdead;
    wave_format.cbSize = 0xdead;
    result = IAudioData_GetFormat(audio_data, &wave_format);
    ok(S_OK == result, "got 0x%08x\n", result);
    ok(WAVE_FORMAT_PCM == wave_format.wFormatTag, "got %u\n", wave_format.wFormatTag);
    ok(1 == wave_format.nChannels, "got %u\n", wave_format.nChannels);
    ok(11025 == wave_format.nSamplesPerSec, "got %u\n", wave_format.nSamplesPerSec);
    ok(22050 == wave_format.nAvgBytesPerSec, "got %u\n", wave_format.nAvgBytesPerSec);
    ok(2 == wave_format.nBlockAlign, "got %u\n", wave_format.nBlockAlign);
    ok(16 == wave_format.wBitsPerSample, "got %u\n", wave_format.wBitsPerSample);
    ok(0 == wave_format.cbSize, "got %u\n", wave_format.cbSize);

    IAudioData_Release(audio_data);

out_unknown:
    IUnknown_Release(unknown);
}

static void test_audiodata_set_format(void)
{
    IUnknown *unknown = create_audio_data();
    IAudioData *audio_data = NULL;
    WAVEFORMATPCMEX wave_format = {{0}};

    HRESULT result;

    result = IUnknown_QueryInterface(unknown, &IID_IAudioData, (void **)&audio_data);
    if (FAILED(result))
    {
        /* test_audiodata_query_interface handles this case */
        skip("No IAudioData\n");
        goto out_unknown;
    }

    result = IAudioData_SetFormat(audio_data, NULL);
    ok(E_POINTER == result, "got 0x%08x\n", result);

    wave_format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wave_format.Format.nChannels = 2;
    wave_format.Format.nSamplesPerSec = 44100;
    wave_format.Format.nAvgBytesPerSec = 176400;
    wave_format.Format.nBlockAlign = 4;
    wave_format.Format.wBitsPerSample = 16;
    wave_format.Format.cbSize = 22;
    wave_format.Samples.wValidBitsPerSample = 16;
    wave_format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
    wave_format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    result = IAudioData_SetFormat(audio_data, &wave_format.Format);
    ok(E_INVALIDARG == result, "got 0x%08x\n", result);

    wave_format.Format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.Format.nChannels = 2;
    wave_format.Format.nSamplesPerSec = 44100;
    wave_format.Format.nAvgBytesPerSec = 176400;
    wave_format.Format.nBlockAlign = 4;
    wave_format.Format.wBitsPerSample = 16;
    wave_format.Format.cbSize = 0;
    result = IAudioData_SetFormat(audio_data, &wave_format.Format);
    ok(S_OK == result, "got 0x%08x\n", result);

    wave_format.Format.wFormatTag = 0xdead;
    wave_format.Format.nChannels = 0xdead;
    wave_format.Format.nSamplesPerSec = 0xdeadbeef;
    wave_format.Format.nAvgBytesPerSec = 0xdeadbeef;
    wave_format.Format.nBlockAlign = 0xdead;
    wave_format.Format.wBitsPerSample = 0xdead;
    wave_format.Format.cbSize = 0xdead;
    result = IAudioData_GetFormat(audio_data, &wave_format.Format);
    ok(S_OK == result, "got 0x%08x\n", result);
    ok(WAVE_FORMAT_PCM == wave_format.Format.wFormatTag, "got %u\n", wave_format.Format.wFormatTag);
    ok(2 == wave_format.Format.nChannels, "got %u\n", wave_format.Format.nChannels);
    ok(44100 == wave_format.Format.nSamplesPerSec, "got %u\n", wave_format.Format.nSamplesPerSec);
    ok(176400 == wave_format.Format.nAvgBytesPerSec, "got %u\n", wave_format.Format.nAvgBytesPerSec);
    ok(4 == wave_format.Format.nBlockAlign, "got %u\n", wave_format.Format.nBlockAlign);
    ok(16 == wave_format.Format.wBitsPerSample, "got %u\n", wave_format.Format.wBitsPerSample);
    ok(0 == wave_format.Format.cbSize, "got %u\n", wave_format.Format.cbSize);

    IAudioData_Release(audio_data);

out_unknown:
    IUnknown_Release(unknown);
}

START_TEST(amstream)
{
    HANDLE file;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    test_interfaces();
    test_add_stream();
    test_media_streams();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_initialize();
    test_enum_media_types();
    test_media_types();
    test_IDirectDrawStreamSample();

    file = CreateFileW(L"test.avi", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file);

        test_openfile();
        test_renderfile();
    }

    test_audiodata_query_interface();
    test_audiodata_get_info();
    test_audiodata_set_buffer();
    test_audiodata_set_actual();
    test_audiodata_get_format();
    test_audiodata_set_format();

    CoUninitialize();
}
