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
#include "initguid.h"
#include "uuids.h"
#include "amstream.h"
#include "vfwmsgs.h"
#include "mmreg.h"
#include "ks.h"
#include "ksmedia.h"

static const WCHAR primary_video_sink_id[] = {'I','{','A','3','5','F','F','5','6','A',
        '-','9','F','D','A','-','1','1','D','0','-','8','F','D','F',
        '-','0','0','C','0','4','F','D','9','1','8','9','D','}',0};

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %d, got %d\n", ref, rc);
}

static const WCHAR filenameW[] = {'t','e','s','t','.','a','v','i',0};

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

    /* FIXME: This call should not be necessary. */
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

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

    hr = IAMMultiMediaStream_OpenFile(pams, filenameW, 0);
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

    hr = IAMMultiMediaStream_OpenFile(pams, filenameW, 0);
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

static void test_media_streams(void)
{
    IAMMultiMediaStream *pams;
    HRESULT hr;
    IMediaStream *video_stream = NULL;
    IMediaStream *audio_stream = NULL;
    IMediaStream *dummy_stream;
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

    /* Verify behaviour with invalid purpose id */
    hr = IAMMultiMediaStream_GetMediaStream(pams, &IID_IUnknown, &dummy_stream);
    ok(hr == MS_E_NOSTREAM, "IAMMultiMediaStream_GetMediaStream returned: %x\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(pams, NULL, &IID_IUnknown, 0, NULL);
    ok(hr == MS_E_PURPOSEID, "IAMMultiMediaStream_AddMediaStream returned: %x\n", hr);

    /* Verify there is no video media stream */
    hr = IAMMultiMediaStream_GetMediaStream(pams, &MSPID_PrimaryVideo, &video_stream);
    ok(hr == MS_E_NOSTREAM, "IAMMultiMediaStream_GetMediaStream returned: %x\n", hr);

    /* Verify there is no default renderer for video stream */
    hr = IAMMultiMediaStream_AddMediaStream(pams, (IUnknown*)pdd7, &MSPID_PrimaryVideo, AMMSF_ADDDEFAULTRENDERER, NULL);
    ok(hr == MS_E_PURPOSEID, "IAMMultiMediaStream_AddMediaStream returned: %x\n", hr);
    hr = IAMMultiMediaStream_GetMediaStream(pams, &MSPID_PrimaryVideo, &video_stream);
    ok(hr == MS_E_NOSTREAM, "IAMMultiMediaStream_GetMediaStream returned: %x\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(pams, NULL, &MSPID_PrimaryVideo, AMMSF_ADDDEFAULTRENDERER, NULL);
    ok(hr == MS_E_PURPOSEID, "IAMMultiMediaStream_AddMediaStream returned: %x\n", hr);
    hr = IAMMultiMediaStream_GetMediaStream(pams, &MSPID_PrimaryVideo, &video_stream);
    ok(hr == MS_E_NOSTREAM, "IAMMultiMediaStream_GetMediaStream returned: %x\n", hr);

    /* Verify normal case for video stream */
    hr = IAMMultiMediaStream_AddMediaStream(pams, NULL, &MSPID_PrimaryVideo, 0, NULL);
    ok(hr == S_OK, "IAMMultiMediaStream_AddMediaStream returned: %x\n", hr);
    hr = IAMMultiMediaStream_GetMediaStream(pams, &MSPID_PrimaryVideo, &video_stream);
    ok(hr == S_OK, "IAMMultiMediaStream_GetMediaStream returned: %x\n", hr);

    /* Verify the video stream has been added to the media stream filter */
    if (media_stream_filter)
    {
        hr = IMediaStreamFilter_GetMediaStream(media_stream_filter, &MSPID_PrimaryVideo, &dummy_stream);
        ok(hr == S_OK, "IMediaStreamFilter_GetMediaStream returned: %x\n", hr);
        ok(dummy_stream == video_stream, "Got wrong returned pointer %p, expected %p\n", dummy_stream, video_stream);
        if (SUCCEEDED(hr))
            IMediaStream_Release(dummy_stream);
    }

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

    /* Verify there is no audio media stream */
    hr = IAMMultiMediaStream_GetMediaStream(pams, &MSPID_PrimaryAudio, &audio_stream);
    ok(hr == MS_E_NOSTREAM, "IAMMultiMediaStream_GetMediaStream returned: %x\n", hr);

    /* Verify no stream is created when using the default renderer for audio stream */
    hr = IAMMultiMediaStream_AddMediaStream(pams, NULL, &MSPID_PrimaryAudio, AMMSF_ADDDEFAULTRENDERER, NULL);
    ok((hr == S_OK) || (hr == VFW_E_NO_AUDIO_HARDWARE), "IAMMultiMediaStream_AddMediaStream returned: %x\n", hr);
    if (hr == S_OK)
    {
        IGraphBuilder* filtergraph = NULL;
        IBaseFilter* filter = NULL;
        const WCHAR name[] = {'0','0','0','1',0};
        CLSID clsid;

        hr = IAMMultiMediaStream_GetFilterGraph(pams, &filtergraph);
        ok(hr == S_OK, "IAMMultiMediaStream_GetFilterGraph returned: %x\n", hr);
        if (hr == S_OK)
        {
            hr = IGraphBuilder_FindFilterByName(filtergraph, name, &filter);
            ok(hr == S_OK, "IGraphBuilder_FindFilterByName returned: %x\n", hr);
        }
        if (hr == S_OK)
        {
            hr = IBaseFilter_GetClassID(filter, &clsid);
            ok(hr == S_OK, "IGraphBuilder_FindFilterByName returned: %x\n", hr);
        }
        if (hr == S_OK)
            ok(IsEqualGUID(&clsid, &CLSID_DSoundRender), "Got wrong CLSID\n");
        if (filter)
            IBaseFilter_Release(filter);
        if (filtergraph)
            IGraphBuilder_Release(filtergraph);
    }
    hr = IAMMultiMediaStream_GetMediaStream(pams, &MSPID_PrimaryAudio, &audio_stream);
    ok(hr == MS_E_NOSTREAM, "IAMMultiMediaStream_GetMediaStream returned: %x\n", hr);

    /* Verify a stream is created when no default renderer is used */
    hr = IAMMultiMediaStream_AddMediaStream(pams, NULL, &MSPID_PrimaryAudio, 0, NULL);
    ok(hr == S_OK, "IAMMultiMediaStream_AddMediaStream returned: %x\n", hr);
    hr = IAMMultiMediaStream_GetMediaStream(pams, &MSPID_PrimaryAudio, &audio_stream);
    ok(hr == S_OK, "IAMMultiMediaStream_GetMediaStream returned: %x\n", hr);

    /* verify the audio stream has been added to the media stream filter */
    if (media_stream_filter)
    {
        hr = IMediaStreamFilter_GetMediaStream(media_stream_filter, &MSPID_PrimaryAudio, &dummy_stream);
        ok(hr == S_OK, "IAMMultiMediaStream_GetMediaStream returned: %x\n", hr);
        ok(dummy_stream == audio_stream, "Got wrong returned pointer %p, expected %p\n", dummy_stream, audio_stream);
        if (SUCCEEDED(hr))
            IMediaStream_Release(dummy_stream);
    }

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
                IEnumMediaTypes* enum_media_types;
                AM_MEDIA_TYPE* media_types[10];
                ULONG nb_media_types;
                IPin* pin;
                PIN_INFO info;
                WCHAR id[40];

                /* Pin name is "I{guid MSPID_PrimaryVideo or MSPID_PrimaryAudio}" */
                id[0] = 'I';
                StringFromGUID2(i ? &MSPID_PrimaryAudio : &MSPID_PrimaryVideo, id + 1, 39);

                hr = IPin_ConnectedTo(pins[i], &pin);
                ok(hr == VFW_E_NOT_CONNECTED, "IPin_ConnectedTo returned: %x\n", hr);
                hr = IPin_QueryPinInfo(pins[i], &info);
                ok(hr == S_OK, "IPin_QueryPinInfo returned: %x\n", hr);
                IBaseFilter_Release(info.pFilter);
                ok(info.dir == PINDIR_INPUT, "Pin direction is %u instead of %u\n", info.dir, PINDIR_INPUT);
                ok(!lstrcmpW(info.achName, id), "Pin name is %s instead of %s\n", wine_dbgstr_w(info.achName), wine_dbgstr_w(id));
                hr = IPin_EnumMediaTypes(pins[i], &enum_media_types);
                ok(hr == S_OK, "IPin_EnumMediaTypes returned: %x\n", hr);
                hr = IEnumMediaTypes_Next(enum_media_types, ARRAY_SIZE(media_types), media_types, &nb_media_types);
                ok(SUCCEEDED(hr), "IEnumMediaTypes_Next returned: %x\n", hr);
                ok(nb_media_types > 0, "nb_media_types should be >0\n");
                IEnumMediaTypes_Release(enum_media_types);
                hr = IMediaStream_QueryInterface(i ? audio_stream : video_stream, &IID_IPin, (void **)&pin);
                ok(hr == S_OK, "IMediaStream_QueryInterface returned: %x\n", hr);
                ok(pin == pins[i], "Pin is %p instead of %p\n", pins[i], pin);
                IPin_Release(pin);
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

    /* FIXME: This call should not be necessary. */
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);

    hr = IMediaStreamFilter_EnumPins(filter, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IMediaStreamFilter_EnumPins(filter, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);
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
    todo_wine ok(ref == 4, "Got unexpected refcount %d.\n", ref);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(pins[0] == pin, "Expected pin %p, got %p.\n", pin, pins[0]);
    ref = get_refcount(filter);
    todo_wine ok(ref == 4, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(pin);
    todo_wine ok(ref == 5, "Got unexpected refcount %d.\n", ref);
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

    /* FIXME: This call should not be necessary. */
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

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

    /* FIXME: This call should not be necessary. */
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

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
    ok(!lstrcmpW(info.achName, primary_video_sink_id), "Got name %s.\n", wine_dbgstr_w(info.achName));
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!lstrcmpW(id, primary_video_sink_id), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, &count);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    IPin_Release(pin);
    IMediaStreamFilter_Release(filter);
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
    test_media_streams();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_IDirectDrawStreamSample();

    file = CreateFileW(filenameW, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
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
