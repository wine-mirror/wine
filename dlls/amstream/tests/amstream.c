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

static const WCHAR filenameW[] = {'t','e','s','t','.','a','v','i',0};

static IAMMultiMediaStream* pams;
static IDirectDraw7* pdd7;
static IDirectDrawSurface7* pdds7;

static int create_ammultimediastream(void)
{
    return S_OK == CoCreateInstance(
        &CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMultiMediaStream, (LPVOID*)&pams);
}

static void release_ammultimediastream(void)
{
    IAMMultiMediaStream_Release(pams);
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

static void test_openfile(void)
{
    HRESULT hr;
    IGraphBuilder* pgraph;

    if (!create_ammultimediastream())
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

    release_ammultimediastream();
}

static void test_renderfile(void)
{
    HRESULT hr;
    IMediaStream *pvidstream = NULL;
    IDirectDrawMediaStream *pddstream = NULL;
    IDirectDrawStreamSample *pddsample = NULL;

    if (!create_ammultimediastream())
        return;
    if (!create_directdraw())
    {
        release_ammultimediastream();
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
    todo_wine ok(hr==S_OK, "IDirectDrawMediaStream_CreateSample returned: %x\n", hr);

error:
    if (pddsample)
        IDirectDrawMediaSample_Release(pddsample);
    if (pddstream)
        IDirectDrawMediaStream_Release(pddstream);
    if (pvidstream)
        IMediaStream_Release(pvidstream);

    release_directdraw();
    release_ammultimediastream();
}

static void test_media_streams(void)
{
    HRESULT hr;
    IMediaStream *video_stream = NULL;
    IMediaStream *audio_stream = NULL;
    IMediaStream *dummy_stream;
    IMediaStreamFilter* media_stream_filter = NULL;

    if (!create_ammultimediastream())
        return;
    if (!create_directdraw())
    {
        release_ammultimediastream();
        return;
    }

    hr = IAMMultiMediaStream_Initialize(pams, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "IAMMultiMediaStream_Initialize returned: %x\n", hr);

    /* Retreive media stream filter */
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
        IAudioMediaStream* audio_media_stream;
        IDirectDrawMediaStream *ddraw_stream = NULL;
        IDirectDrawStreamSample *ddraw_sample = NULL;

        hr = IMediaStream_QueryInterface(video_stream, &IID_IAMMediaStream, (LPVOID*)&am_media_stream);
        todo_wine ok(hr == S_OK, "IMediaStream_QueryInterface returned: %x\n", hr);
        todo_wine ok((void*)am_media_stream == (void*)video_stream, "Not same interface, got %p expected %p\n", am_media_stream, video_stream);
        if (hr == S_OK)
            IAMMediaStream_Release(am_media_stream);

        hr = IMediaStream_QueryInterface(video_stream, &IID_IAudioMediaStream, (LPVOID*)&audio_media_stream);
        ok(hr == E_NOINTERFACE, "IMediaStream_QueryInterface returned: %x\n", hr);

        hr = IMediaStream_QueryInterface(video_stream, &IID_IDirectDrawMediaStream, (LPVOID*)&ddraw_stream);
        ok(hr == S_OK, "IMediaStream_QueryInterface returned: %x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IDirectDrawMediaStream_CreateSample(ddraw_stream, NULL, NULL, 0, &ddraw_sample);
            todo_wine ok(hr==S_OK, "IDirectDrawMediaStream_CreateSample returned: %x\n", hr);
        }

        if (ddraw_sample)
            IDirectDrawMediaSample_Release(ddraw_sample);
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
        IDirectDrawMediaStream* ddraw_stream = NULL;
        IAudioMediaStream* audio_media_stream = NULL;
        IAudioStreamSample *audio_sample = NULL;

        hr = IMediaStream_QueryInterface(audio_stream, &IID_IAMMediaStream, (LPVOID*)&am_media_stream);
        todo_wine ok(hr == S_OK, "IMediaStream_QueryInterface returned: %x\n", hr);
        todo_wine ok((void*)am_media_stream == (void*)audio_stream, "Not same interface, got %p expected %p\n", am_media_stream, video_stream);
        if (hr == S_OK)
            IAMMediaStream_Release(am_media_stream);

        hr = IMediaStream_QueryInterface(audio_stream, &IID_IDirectDrawMediaStream, (LPVOID*)&ddraw_stream);
        ok(hr == E_NOINTERFACE, "IMediaStream_QueryInterface returned: %x\n", hr);

        hr = IMediaStream_QueryInterface(audio_stream, &IID_IAudioMediaStream, (LPVOID*)&audio_media_stream);
        ok(hr == S_OK, "IMediaStream_QueryInterface returned: %x\n", hr);

        if (SUCCEEDED(hr))
        {
            IAudioData* audio_data = NULL;
            hr = CoCreateInstance(&CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER, &IID_IAudioData, (void **)&audio_data);
            ok(hr == S_OK, "CoCreateInstance returned: %x\n", hr);

            hr = IAudioMediaStream_CreateSample(audio_media_stream, NULL, 0, &audio_sample);
            todo_wine ok(hr == E_POINTER, "IAudioMediaStream_CreateSample returned: %x\n", hr);
            hr = IAudioMediaStream_CreateSample(audio_media_stream, audio_data, 0, &audio_sample);
            todo_wine ok(hr == S_OK, "IAudioMediaStream_CreateSample returned: %x\n", hr);

            if (audio_data)
                IAudioData_Release(audio_data);
            if (audio_sample)
                IAudioStreamSample_Release(audio_sample);
            if (audio_media_stream)
                IAudioMediaStream_Release(audio_media_stream);
        }
    }

    if (video_stream)
        IMediaStream_Release(video_stream);
    if (audio_stream)
        IMediaStream_Release(audio_stream);
    if (media_stream_filter)
        IMediaStreamFilter_Release(media_stream_filter);

    release_directdraw();
    release_ammultimediastream();
}

START_TEST(amstream)
{
    HANDLE file;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    test_media_streams();

    file = CreateFileW(filenameW, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file);

        test_openfile();
        test_renderfile();
    }

    CoUninitialize();
}
