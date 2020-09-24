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
#include "dvdmedia.h"
#include "wmcodecdsp.h"
#include "wine/strmbase.h"

static const WAVEFORMATEX audio_format =
{
    .wFormatTag = WAVE_FORMAT_PCM,
    .nChannels = 1,
    .nSamplesPerSec = 11025,
    .wBitsPerSample = 16,
    .nBlockAlign = 2,
    .nAvgBytesPerSec = 2 * 11025,
};

static const AM_MEDIA_TYPE audio_mt =
{
    /* MEDIATYPE_Audio, MEDIASUBTYPE_PCM, FORMAT_WaveFormatEx */
    .majortype = {0x73647561, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .subtype = {0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .formattype = {0x05589f81, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}},
    .cbFormat = sizeof(WAVEFORMATEX),
    .pbFormat = (BYTE *)&audio_format,
};

static const VIDEOINFO rgb8_video_info =
{
    .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
    .bmiHeader.biWidth = 333,
    .bmiHeader.biHeight = -444,
    .bmiHeader.biPlanes = 1,
    .bmiHeader.biBitCount = 8,
    .bmiHeader.biCompression = BI_RGB,
};

static const VIDEOINFO rgb555_video_info =
{
    .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
    .bmiHeader.biWidth = 333,
    .bmiHeader.biHeight = -444,
    .bmiHeader.biPlanes = 1,
    .bmiHeader.biBitCount = 16,
    .bmiHeader.biCompression = BI_RGB,
};

static const VIDEOINFO rgb565_video_info =
{
    .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
    .bmiHeader.biWidth = 333,
    .bmiHeader.biHeight = -444,
    .bmiHeader.biPlanes = 1,
    .bmiHeader.biBitCount = 16,
    .bmiHeader.biCompression = BI_BITFIELDS,
    .dwBitMasks = {0xf800, 0x07e0, 0x001f},
};

static const VIDEOINFO rgb24_video_info =
{
    .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
    .bmiHeader.biWidth = 333,
    .bmiHeader.biHeight = -444,
    .bmiHeader.biPlanes = 1,
    .bmiHeader.biBitCount = 24,
    .bmiHeader.biCompression = BI_RGB,
};

static const VIDEOINFO rgb32_video_info =
{
    .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
    .bmiHeader.biWidth = 333,
    .bmiHeader.biHeight = -444,
    .bmiHeader.biPlanes = 1,
    .bmiHeader.biBitCount = 32,
    .bmiHeader.biCompression = BI_RGB,
};

static const AM_MEDIA_TYPE rgb8_mt =
{
    /* MEDIATYPE_Video, MEDIASUBTYPE_RGB8, FORMAT_VideoInfo */
    .majortype = {0x73646976, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .subtype = {0xe436eb7a, 0x524f, 0x11ce, {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}},
    .formattype = {0x05589f80, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}},
    .cbFormat = sizeof(VIDEOINFO),
    .pbFormat = (BYTE *)&rgb8_video_info,
};

static const AM_MEDIA_TYPE rgb555_mt =
{
    /* MEDIATYPE_Video, MEDIASUBTYPE_RGB555, FORMAT_VideoInfo */
    .majortype = {0x73646976, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .subtype = {0xe436eb7c, 0x524f, 0x11ce, {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}},
    .formattype = {0x05589f80, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}},
    .cbFormat = sizeof(VIDEOINFO),
    .pbFormat = (BYTE *)&rgb555_video_info,
};

static const AM_MEDIA_TYPE rgb565_mt =
{
    /* MEDIATYPE_Video, MEDIASUBTYPE_RGB565, FORMAT_VideoInfo */
    .majortype = {0x73646976, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .subtype = {0xe436eb7b, 0x524f, 0x11ce, {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}},
    .formattype = {0x05589f80, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}},
    .cbFormat = sizeof(VIDEOINFO),
    .pbFormat = (BYTE *)&rgb565_video_info,
};

static const AM_MEDIA_TYPE rgb24_mt =
{
    /* MEDIATYPE_Video, MEDIASUBTYPE_RGB24, FORMAT_VideoInfo */
    .majortype = {0x73646976, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .subtype = {0xe436eb7d, 0x524f, 0x11ce, {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}},
    .formattype = {0x05589f80, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}},
    .cbFormat = sizeof(VIDEOINFO),
    .pbFormat = (BYTE *)&rgb24_video_info,
};

static const AM_MEDIA_TYPE rgb32_mt =
{
    /* MEDIATYPE_Video, MEDIASUBTYPE_RGB32, FORMAT_VideoInfo */
    .majortype = {0x73646976, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    .subtype = {0xe436eb7e, 0x524f, 0x11ce, {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}},
    .formattype = {0x05589f80, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}},
    .cbFormat = sizeof(VIDEOINFO),
    .pbFormat = (BYTE *)&rgb32_video_info,
};

static const DDSURFACEDESC rgb8_format =
{
    .dwSize = sizeof(DDSURFACEDESC),
    .dwFlags = DDSD_PIXELFORMAT,
    .ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT),
    .ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8,
    .ddpfPixelFormat.dwRGBBitCount = 8,
};

static const DDSURFACEDESC rgb555_format =
{
    .dwSize = sizeof(DDSURFACEDESC),
    .dwFlags = DDSD_PIXELFORMAT,
    .ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT),
    .ddpfPixelFormat.dwFlags = DDPF_RGB,
    .ddpfPixelFormat.dwRGBBitCount = 16,
    .ddpfPixelFormat.dwRBitMask = 0x7c00,
    .ddpfPixelFormat.dwGBitMask = 0x03e0,
    .ddpfPixelFormat.dwBBitMask = 0x001f,
};

static const DDSURFACEDESC rgb565_format =
{
    .dwSize = sizeof(DDSURFACEDESC),
    .dwFlags = DDSD_PIXELFORMAT,
    .ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT),
    .ddpfPixelFormat.dwFlags = DDPF_RGB,
    .ddpfPixelFormat.dwRGBBitCount = 16,
    .ddpfPixelFormat.dwRBitMask = 0xf800,
    .ddpfPixelFormat.dwGBitMask = 0x07e0,
    .ddpfPixelFormat.dwBBitMask = 0x001f,
};

static const DDSURFACEDESC rgb24_format =
{
    .dwSize = sizeof(DDSURFACEDESC),
    .dwFlags = DDSD_PIXELFORMAT,
    .ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT),
    .ddpfPixelFormat.dwFlags = DDPF_RGB,
    .ddpfPixelFormat.dwRGBBitCount = 24,
    .ddpfPixelFormat.dwRBitMask = 0xff0000,
    .ddpfPixelFormat.dwGBitMask = 0x00ff00,
    .ddpfPixelFormat.dwBBitMask = 0x0000ff,
};

static const DDSURFACEDESC rgb32_format =
{
    .dwSize = sizeof(DDSURFACEDESC),
    .dwFlags = DDSD_PIXELFORMAT,
    .ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT),
    .ddpfPixelFormat.dwFlags = DDPF_RGB,
    .ddpfPixelFormat.dwRGBBitCount = 32,
    .ddpfPixelFormat.dwRBitMask = 0xff0000,
    .ddpfPixelFormat.dwGBitMask = 0x00ff00,
    .ddpfPixelFormat.dwBBitMask = 0x0000ff,
};

static const DDSURFACEDESC argb32_format =
{
    .dwSize = sizeof(DDSURFACEDESC),
    .dwFlags = DDSD_PIXELFORMAT,
    .ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT),
    .ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS,
    .ddpfPixelFormat.dwRGBBitCount = 32,
    .ddpfPixelFormat.dwRBitMask = 0xff0000,
    .ddpfPixelFormat.dwGBitMask = 0x00ff00,
    .ddpfPixelFormat.dwBBitMask = 0x0000ff,
    .ddpfPixelFormat.dwRGBAlphaBitMask = 0xff000000,
};

static const DDSURFACEDESC yuy2_format =
{
    .dwSize = sizeof(DDSURFACEDESC),
    .dwFlags = DDSD_PIXELFORMAT,
    .ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT),
    .ddpfPixelFormat.dwFlags = DDPF_FOURCC,
    .ddpfPixelFormat.dwFourCC = MAKEFOURCC('Y', 'U', 'Y', '2'),
    .ddpfPixelFormat.dwYUVBitCount = 16,
};

static const DDSURFACEDESC yv12_format =
{
    .dwSize = sizeof(DDSURFACEDESC),
    .dwFlags = DDSD_PIXELFORMAT,
    .ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT),
    .ddpfPixelFormat.dwFlags = DDPF_FOURCC,
    .ddpfPixelFormat.dwFourCC = MAKEFOURCC('Y', 'V', '1', '2'),
    .ddpfPixelFormat.dwYUVBitCount = 12,
};

static const WCHAR primary_video_sink_id[] = L"I{A35FF56A-9FDA-11D0-8FDF-00C04FD9189D}";
static const WCHAR primary_audio_sink_id[] = L"I{A35FF56B-9FDA-11D0-8FDF-00C04FD9189D}";

static const WCHAR *load_resource(const WCHAR *name)
{
    HMODULE module = GetModuleHandleA(NULL);
    HRSRC resource;
    DWORD written;
    HANDLE file;
    WCHAR *path;
    DWORD size;
    void *ptr;

    path = calloc(MAX_PATH + 1, sizeof(WCHAR));
    ok(!!path, "Failed to allocate temp path string.\n");
    GetTempPathW(MAX_PATH + 1, path);
    wcscat(path, name);

    file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create file %s, error %u.\n", wine_dbgstr_w(path), GetLastError());

    resource = FindResourceW(module, name, (const WCHAR *)RT_RCDATA);
    ok(!!resource, "Failed to find resource %s, error %u.\n", wine_dbgstr_w(name), GetLastError());

    size = SizeofResource(module, resource);
    ptr = LockResource(LoadResource(module, resource));

    WriteFile(file, ptr, size, &written, NULL);
    ok(written == size, "Failed to write file %s.\n", wine_dbgstr_w(path));

    CloseHandle(file);

    return path;
}

static void unload_resource(const WCHAR *path)
{
    BOOL ret = DeleteFileW(path);
    ok(ret, "Failed to delete file %s.\n", wine_dbgstr_w(path));
    free((void *)path);
}

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %d, got %d\n", ref, rc);
}

static IAMMultiMediaStream *create_ammultimediastream(void)
{
    IAMMultiMediaStream *stream = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER,
            &IID_IAMMultiMediaStream, (void **)&stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    return stream;
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

static void test_openfile(const WCHAR *test_avi_path)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IMediaControl *media_control;
    IMediaStreamFilter *filter;
    IGraphBuilder *graph;
    OAFilterState state;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!graph, "Expected NULL graph.\n");

    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(!!filter, "Expected non-NULL filter.\n");
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    check_interface(filter, &IID_IMediaSeeking, FALSE);

    hr = IAMMultiMediaStream_OpenFile(mmstream, test_avi_path, AMMSF_NORENDER);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    check_interface(filter, &IID_IMediaSeeking, FALSE);

    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!graph, "Expected non-NULL graph.\n");
    IGraphBuilder_Release(graph);
    IMediaStreamFilter_Release(filter);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!graph, "Expected NULL graph.\n");

    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(!!filter, "Expected non-NULL filter.\n");
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    check_interface(filter, &IID_IMediaSeeking, FALSE);

    hr = IAMMultiMediaStream_OpenFile(mmstream, test_avi_path, 0);
    ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#x.\n", hr);

    check_interface(filter, &IID_IMediaSeeking, FALSE);

    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!graph, "Expected non-NULL graph.\n");
    IGraphBuilder_Release(graph);
    IMediaStreamFilter_Release(filter);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    check_interface(filter, &IID_IMediaSeeking, FALSE);

    hr = IAMMultiMediaStream_OpenFile(mmstream, test_avi_path, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    check_interface(filter, &IID_IMediaSeeking, TRUE);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStreamFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!graph, "Expected non-NULL graph.\n");
    hr = IGraphBuilder_QueryInterface(graph, &IID_IMediaControl, (void **)&media_control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_OpenFile(mmstream, test_avi_path, AMMSF_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    state = 0xdeadbeef;
    hr = IMediaControl_GetState(media_control, INFINITE, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Running, "Got state %#x.\n", state);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IMediaControl_Release(media_control);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_mmstream_get_duration(const WCHAR *test_avi_path)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    HRESULT hr, audio_hr;
    LONGLONG duration;
    ULONG ref;

    duration = 0xdeadbeefdeadbeefULL;
    hr = IAMMultiMediaStream_GetDuration(mmstream, &duration);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(duration == 0xdeadbeefdeadbeefULL, "Got duration %s.\n", wine_dbgstr_longlong(duration));

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, AMMSF_ADDDEFAULTRENDERER, NULL);
    ok(hr == S_OK || hr == VFW_E_NO_AUDIO_HARDWARE, "Got hr %#x.\n", hr);
    audio_hr = hr;

    hr = IAMMultiMediaStream_OpenFile(mmstream, test_avi_path, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    duration = 0xdeadbeefdeadbeefULL;
    hr = IAMMultiMediaStream_GetDuration(mmstream, &duration);
    if (audio_hr == S_OK)
    {
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(duration == 1000000LL, "Got duration %s.\n", wine_dbgstr_longlong(duration));
    }
    else
    {
        ok(hr == S_FALSE, "Got hr %#x.\n", hr);
        ok(!duration, "Got duration %s.\n", wine_dbgstr_longlong(duration));
    }

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    duration = 0xdeadbeefdeadbeefULL;
    hr = IAMMultiMediaStream_GetDuration(mmstream, &duration);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(duration == 0, "Got duration %s.\n", wine_dbgstr_longlong(duration));

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();

    hr = IAMMultiMediaStream_OpenFile(mmstream, test_avi_path, 0);
    ok(hr == VFW_E_CANNOT_CONNECT, "Got hr %#x.\n", hr);

    duration = 0xdeadbeefdeadbeefULL;
    hr = IAMMultiMediaStream_GetDuration(mmstream, &duration);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(duration == 0, "Got duration %s.\n", wine_dbgstr_longlong(duration));

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_OpenFile(mmstream, test_avi_path, AMMSF_NORENDER);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    duration = 0xdeadbeefdeadbeefULL;
    hr = IAMMultiMediaStream_GetDuration(mmstream, &duration);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(duration == 0, "Got duration %s.\n", wine_dbgstr_longlong(duration));

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static const GUID test_mspid = {0x88888888};

struct teststream
{
    IAMMediaStream IAMMediaStream_iface;
    IPin IPin_iface;
    LONG refcount;
    GUID mspid;
    IAMMultiMediaStream *mmstream;
    IMediaStreamFilter *filter;
    IFilterGraph *graph;
    FILTER_STATE state;
    HRESULT set_state_result;
};

static struct teststream *impl_from_IAMMediaStream(IAMMediaStream *iface)
{
    return CONTAINING_RECORD(iface, struct teststream, IAMMediaStream_iface);
}

static struct teststream *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, struct teststream, IPin_iface);
}

static HRESULT WINAPI pin_QueryInterface(IPin *iface, REFIID iid, void **out)
{
    struct teststream *stream = impl_from_IPin(iface);
    return IAMMediaStream_QueryInterface(&stream->IAMMediaStream_iface, iid, out);
}

static ULONG WINAPI pin_AddRef(IPin *iface)
{
    struct teststream *stream = impl_from_IPin(iface);
    return IAMMediaStream_AddRef(&stream->IAMMediaStream_iface);
}

static ULONG WINAPI pin_Release(IPin *iface)
{
    struct teststream *stream = impl_from_IPin(iface);
    return IAMMediaStream_Release(&stream->IAMMediaStream_iface);
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

static HRESULT WINAPI stream_QueryInterface(IAMMediaStream *iface, REFIID iid, void **out)
{
    struct teststream *stream = impl_from_IAMMediaStream(iface);

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
        *out = &stream->IPin_iface;
        return S_OK;
    }

    ok(0, "Unexpected interface %s.\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI stream_AddRef(IAMMediaStream *iface)
{
    struct teststream *stream = impl_from_IAMMediaStream(iface);
    return InterlockedIncrement(&stream->refcount);
}

static ULONG WINAPI stream_Release(IAMMediaStream *iface)
{
    struct teststream *stream = impl_from_IAMMediaStream(iface);
    return InterlockedDecrement(&stream->refcount);
}

static HRESULT WINAPI stream_GetMultiMediaStream(IAMMediaStream *iface, IMultiMediaStream **mmstream)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_GetInformation(IAMMediaStream *iface, MSPID *id, STREAM_TYPE *type)
{
    struct teststream *stream = impl_from_IAMMediaStream(iface);
    if (winetest_debug > 1) trace("GetInformation(%p, %p)\n", id, type);
    if (id)
        *id = stream->mspid;
    if (type)
        *type = STREAMTYPE_READ;
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
    struct teststream *stream = impl_from_IAMMediaStream(iface);
    if (winetest_debug > 1) trace("SetState(%#x)\n", state);
    if (SUCCEEDED(stream->set_state_result))
        stream->state = state;
    return stream->set_state_result;
}

static HRESULT WINAPI stream_JoinAMMultiMediaStream(IAMMediaStream *iface, IAMMultiMediaStream *mmstream)
{
    struct teststream *stream = impl_from_IAMMediaStream(iface);
    if (winetest_debug > 1) trace("JoinAMMultiMediaStream(%p)\n", mmstream);
    stream->mmstream = mmstream;
    return S_OK;
}

static HRESULT WINAPI stream_JoinFilter(IAMMediaStream *iface, IMediaStreamFilter *filter)
{
    struct teststream *stream = impl_from_IAMMediaStream(iface);
    if (winetest_debug > 1) trace("JoinFilter(%p)\n", filter);
    stream->filter = filter;
    return S_OK;
}

static HRESULT WINAPI stream_JoinFilterGraph(IAMMediaStream *iface, IFilterGraph *graph)
{
    struct teststream *stream = impl_from_IAMMediaStream(iface);
    if (winetest_debug > 1) trace("JoinFilterGraph(%p)\n", graph);
    stream->graph = graph;
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

static void teststream_init(struct teststream *stream)
{
    memset(stream, 0, sizeof(*stream));
    stream->IAMMediaStream_iface.lpVtbl = &stream_vtbl;
    stream->IPin_iface.lpVtbl = &pin_vtbl;
    stream->refcount = 1;
    stream->mspid = test_mspid;
    stream->set_state_result = S_OK;
}

#define check_enum_stream(a,b,c,d) check_enum_stream_(__LINE__,a,b,c,d)
static void check_enum_stream_(int line, IAMMultiMediaStream *mmstream,
        IMediaStreamFilter *filter, LONG index, IMediaStream *expect)
{
    IMediaStream *stream = NULL, *stream2 = NULL;
    HRESULT hr;

    hr = IAMMultiMediaStream_EnumMediaStreams(mmstream, index, &stream);
    ok_(__FILE__, line)(hr == (expect ? S_OK : S_FALSE),
            "IAMMultiMediaStream::EnumMediaStreams() returned %#x.\n", hr);
    hr = IMediaStreamFilter_EnumMediaStreams(filter, index, &stream2);
    ok_(__FILE__, line)(hr == (expect ? S_OK : S_FALSE),
            "IMediaStreamFilter::EnumMediaStreams() returned %#x.\n", hr);
    if (hr == S_OK)
    {
        ok_(__FILE__, line)(stream == expect, "Expected stream %p, got %p.\n", expect, stream);
        ok_(__FILE__, line)(stream2 == expect, "Expected stream %p, got %p.\n", expect, stream2);
        IMediaStream_Release(stream);
        IMediaStream_Release(stream2);
    }
}

struct testfilter
{
    struct strmbase_filter filter;
    struct strmbase_source source;
    IMediaSeeking IMediaSeeking_iface;
    LONGLONG current_position;
    LONGLONG stop_position;
    const AM_MEDIA_TYPE *preferred_mt;
    HRESULT get_duration_hr;
    HRESULT get_stop_position_hr;
    HRESULT set_positions_hr;
    HRESULT init_stream_hr;
    HRESULT cleanup_stream_hr;
};

static inline struct testfilter *impl_from_BaseFilter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, filter);
}

static struct strmbase_pin *testfilter_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct testfilter *filter = impl_from_BaseFilter(iface);
    if (!index)
        return &filter->source.pin;
    return NULL;
}

static void testfilter_destroy(struct strmbase_filter *iface)
{
    struct testfilter *filter = impl_from_BaseFilter(iface);
    strmbase_source_cleanup(&filter->source);
    strmbase_filter_cleanup(&filter->filter);
}

static HRESULT testfilter_init_stream(struct strmbase_filter *iface)
{
    struct testfilter *filter = impl_from_BaseFilter(iface);

    if (SUCCEEDED(filter->init_stream_hr))
        BaseOutputPinImpl_Active(&filter->source);

    return filter->init_stream_hr;
}

static HRESULT testfilter_cleanup_stream(struct strmbase_filter *iface)
{
    struct testfilter *filter = impl_from_BaseFilter(iface);

    if (SUCCEEDED(filter->cleanup_stream_hr))
        BaseOutputPinImpl_Inactive(&filter->source);

    return filter->cleanup_stream_hr;
}

static const struct strmbase_filter_ops testfilter_ops =
{
    .filter_get_pin = testfilter_get_pin,
    .filter_destroy = testfilter_destroy,
    .filter_init_stream = testfilter_init_stream,
    .filter_cleanup_stream = testfilter_cleanup_stream,
};

static inline struct testfilter *impl_from_base_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, source.pin);
}

static HRESULT testsource_get_media_type(struct strmbase_pin *iface, unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct testfilter *filter = impl_from_base_pin(iface);

    if (index < 1 && filter->preferred_mt)
        return CopyMediaType(mt, filter->preferred_mt);

    return VFW_S_NO_MORE_ITEMS;
}

static HRESULT testsource_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_base_pin(iface);

    if (IsEqualGUID(iid, &IID_IMediaSeeking) && filter->IMediaSeeking_iface.lpVtbl)
        *out = &filter->IMediaSeeking_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);

    return S_OK;
}

static HRESULT WINAPI testsource_DecideAllocator(struct strmbase_source *iface, IMemInputPin *pin, IMemAllocator **alloc)
{
    ALLOCATOR_PROPERTIES props = {0};
    HRESULT hr;

    /* AMDirectDrawStream tries to use it's custom allocator and
     * when it is able to do so it's behavior changes slightly
     * (e.g. it uses dynamic format change instead of reconnecting in SetFormat).
     * We don't yet implement the custom allocator so force the standard one for now. */
    hr = BaseOutputPinImpl_InitAllocator(iface, alloc);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    IMemInputPin_GetAllocatorRequirements(pin, &props);
    hr = iface->pFuncsTable->pfnDecideBufferSize(iface, *alloc, &props);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    return IMemInputPin_NotifyAllocator(pin, *alloc, FALSE);
}

static HRESULT WINAPI testsource_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *alloc, ALLOCATOR_PROPERTIES *requested)
{
    ALLOCATOR_PROPERTIES actual;

    if (!requested->cbAlign)
        requested->cbAlign = 1;

    if (requested->cbBuffer < 4096)
        requested->cbBuffer = 4096;

    if (!requested->cBuffers)
        requested->cBuffers = 2;

    return IMemAllocator_SetProperties(alloc, requested, &actual);
}

static const struct strmbase_source_ops testsource_ops =
{
    .base.pin_get_media_type = testsource_get_media_type,
    .base.pin_query_interface = testsource_query_interface,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideBufferSize = testsource_DecideBufferSize,
    .pfnDecideAllocator = testsource_DecideAllocator,
};

static void testfilter_init(struct testfilter *filter)
{
    static const GUID clsid = {0xabacab};
    memset(filter, 0, sizeof(*filter));
    strmbase_filter_init(&filter->filter, NULL, &clsid, &testfilter_ops);
    strmbase_source_init(&filter->source, &filter->filter, L"", &testsource_ops);
    filter->stop_position = 0x8000000000000000ULL;
}

static inline struct testfilter *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, IMediaSeeking_iface);
}

static HRESULT WINAPI testsource_seeking_QueryInterface(IMediaSeeking *iface, REFIID iid, void **out)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    return IBaseFilter_QueryInterface(&filter->filter.IBaseFilter_iface, iid, out);
}

static ULONG WINAPI testsource_seeking_AddRef(IMediaSeeking *iface)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    return IBaseFilter_AddRef(&filter->filter.IBaseFilter_iface);
}

static ULONG WINAPI testsource_seeking_Release(IMediaSeeking *iface)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);
    return IBaseFilter_Release(&filter->filter.IBaseFilter_iface);
}

static HRESULT WINAPI testsource_seeking_GetCapabilities(IMediaSeeking *iface, DWORD *capabilities)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsource_seeking_CheckCapabilities(IMediaSeeking *iface, DWORD *capabilities)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsource_seeking_IsFormatSupported(IMediaSeeking *iface, const GUID *format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsource_seeking_QueryPreferredFormat(IMediaSeeking *iface, GUID *format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsource_seeking_GetTimeFormat(IMediaSeeking *iface, GUID *format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsource_seeking_IsUsingTimeFormat(IMediaSeeking *iface, const GUID *format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsource_seeking_SetTimeFormat(IMediaSeeking *iface, const GUID *format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsource_seeking_GetDuration(IMediaSeeking *iface, LONGLONG *duration)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);

    if (SUCCEEDED(filter->get_duration_hr))
        *duration = 0x8000000000000000ULL;

    return filter->get_duration_hr;
}

static HRESULT WINAPI testsource_seeking_GetStopPosition(IMediaSeeking *iface, LONGLONG *stop)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);

    if (SUCCEEDED(filter->get_stop_position_hr))
        *stop = 0x8000000000000000ULL;

    return filter->get_stop_position_hr;
}

static HRESULT WINAPI testsource_seeking_GetCurrentPosition(IMediaSeeking *iface, LONGLONG *current)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsource_seeking_ConvertTimeFormat(IMediaSeeking *iface, LONGLONG *target,
        const GUID *target_format, LONGLONG source, const GUID *source_format)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsource_seeking_SetPositions(IMediaSeeking *iface, LONGLONG *current_ptr, DWORD current_flags,
        LONGLONG *stop_ptr, DWORD stop_flags)
{
    struct testfilter *filter = impl_from_IMediaSeeking(iface);

    if (SUCCEEDED(filter->set_positions_hr))
    {
        if (current_ptr)
            filter->current_position = *current_ptr;

        if (stop_ptr)
            filter->stop_position = *stop_ptr;
    }

    return filter->set_positions_hr;
}

static HRESULT WINAPI testsource_seeking_GetPositions(IMediaSeeking *iface, LONGLONG *current, LONGLONG *stop)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsource_seeking_GetAvailable(IMediaSeeking *iface, LONGLONG *earliest, LONGLONG *latest)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsource_seeking_SetRate(IMediaSeeking *iface, double rate)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsource_seeking_GetRate(IMediaSeeking *iface, double *rate)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testsource_seeking_GetPreroll(IMediaSeeking *iface, LONGLONG *preroll)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IMediaSeekingVtbl testsource_seeking_vtbl =
{
    testsource_seeking_QueryInterface,
    testsource_seeking_AddRef,
    testsource_seeking_Release,
    testsource_seeking_GetCapabilities,
    testsource_seeking_CheckCapabilities,
    testsource_seeking_IsFormatSupported,
    testsource_seeking_QueryPreferredFormat,
    testsource_seeking_GetTimeFormat,
    testsource_seeking_IsUsingTimeFormat,
    testsource_seeking_SetTimeFormat,
    testsource_seeking_GetDuration,
    testsource_seeking_GetStopPosition,
    testsource_seeking_GetCurrentPosition,
    testsource_seeking_ConvertTimeFormat,
    testsource_seeking_SetPositions,
    testsource_seeking_GetPositions,
    testsource_seeking_GetAvailable,
    testsource_seeking_SetRate,
    testsource_seeking_GetRate,
    testsource_seeking_GetPreroll,
};

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
    struct teststream teststream;
    IDirectDraw *ddraw, *ddraw2;
    IEnumFilters *enum_filters;
    IBaseFilter *filters[3];
    IGraphBuilder *graph;
    FILTER_INFO info;
    ULONG ref, count;
    CLSID clsid;
    HRESULT hr;

    teststream_init(&teststream);

    hr = IAMMultiMediaStream_GetFilter(mmstream, &stream_filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_EnumMediaStreams(mmstream, 0, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    hr = IMediaStreamFilter_EnumMediaStreams(stream_filter, 0, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_GetMediaStream(mmstream, &MSPID_PrimaryAudio, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = IMediaStreamFilter_GetMediaStream(stream_filter, &MSPID_PrimaryAudio, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetMediaStream(mmstream, &MSPID_PrimaryVideo, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = IMediaStreamFilter_GetMediaStream(stream_filter, &MSPID_PrimaryVideo, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

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
    ok(hr == MS_E_PURPOSEID, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_EnumMediaStreams(mmstream, 0, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = IMediaStreamFilter_EnumMediaStreams(stream_filter, 0, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_EnumMediaStreams(mmstream, 1, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    hr = IMediaStreamFilter_EnumMediaStreams(stream_filter, 1, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

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

    hr = IMediaStreamFilter_QueryFilterInfo(stream_filter, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)&teststream, &test_mspid, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(stream == (IMediaStream *)&teststream, "Streams didn't match.\n");
    IMediaStream_Release(stream);
    ok(teststream.mmstream == mmstream, "IAMMultiMediaStream objects didn't match.\n");
    ok(teststream.filter == stream_filter, "IMediaStreamFilter objects didn't match.\n");
    ok(teststream.graph == info.pGraph, "IFilterGraph objects didn't match.\n");

    IFilterGraph_Release(info.pGraph);

    check_enum_stream(mmstream, stream_filter, 0, video_stream);
    check_enum_stream(mmstream, stream_filter, 1, audio_stream);
    check_enum_stream(mmstream, stream_filter, 2, (IMediaStream *)&teststream);
    check_enum_stream(mmstream, stream_filter, 3, NULL);

    check_get_stream(mmstream, stream_filter, &MSPID_PrimaryVideo, video_stream);
    check_get_stream(mmstream, stream_filter, &MSPID_PrimaryAudio, audio_stream);
    check_get_stream(mmstream, stream_filter, &test_mspid, (IMediaStream *)&teststream);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == MS_E_PURPOSEID, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!graph, "Expected a non-NULL graph.\n");

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

    IMediaStreamFilter_Release(stream_filter);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStream_Release(video_stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStream_Release(audio_stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ok(teststream.refcount == 1, "Got outstanding refcount %d.\n", teststream.refcount);

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

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo,
            AMMSF_ADDDEFAULTRENDERER, &video_stream);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo,
            AMMSF_ADDDEFAULTRENDERER, NULL);
    ok(hr == MS_E_PURPOSEID, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio,
            AMMSF_ADDDEFAULTRENDERER, &audio_stream);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

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
        hr = IGraphBuilder_EnumFilters(graph, &enum_filters);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        hr = IEnumFilters_Next(enum_filters, 3, filters, &count);
        ok(hr == S_FALSE, "Got hr %#x.\n", hr);
        ok(count == 2, "Got count %u.\n", count);
        ok(filters[1] == (IBaseFilter *)stream_filter,
                "Expected filter %p, got %p.\n", stream_filter, filters[1]);
        hr = IBaseFilter_GetClassID(filters[0], &clsid);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(IsEqualGUID(&clsid, &CLSID_DSoundRender), "Got unexpected filter %s.\n", wine_dbgstr_guid(&clsid));
        IBaseFilter_Release(filters[0]);
        IBaseFilter_Release(filters[1]);
        IEnumFilters_Release(enum_filters);
        IGraphBuilder_Release(graph);
    }

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &test_mspid,
            AMMSF_ADDDEFAULTRENDERER, NULL);
    ok(hr == MS_E_PURPOSEID, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &test_mspid,
            AMMSF_ADDDEFAULTRENDERER, &audio_stream);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStreamFilter_Release(stream_filter);
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

            hr = CoCreateInstance(&CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER, &IID_IAudioData, (void **)&audio_data);
            ok(hr == S_OK, "CoCreateInstance returned: %x\n", hr);

            hr = IAudioMediaStream_CreateSample(audio_media_stream, NULL, 0, &audio_sample);
            ok(hr == E_POINTER, "IAudioMediaStream_CreateSample returned: %x\n", hr);

            EXPECT_REF(audio_stream, 3);
            EXPECT_REF(audio_data, 1);
            hr = IAudioMediaStream_CreateSample(audio_media_stream, audio_data, 0, &audio_sample);
            ok(hr == S_OK, "IAudioMediaStream_CreateSample returned: %x\n", hr);
            EXPECT_REF(audio_stream, 4);
            EXPECT_REF(audio_data, 2);

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
    ok(ref == 4, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(pin);
    ok(ref == 4, "Got unexpected refcount %d.\n", ref);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(pins[0] == pin, "Expected pin %p, got %p.\n", pin, pins[0]);
    ref = get_refcount(filter);
    ok(ref == 4, "Got unexpected refcount %d.\n", ref);
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
    IMediaStream_Release(stream);
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
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_TRANSFORM, 0, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    ret_graph = (IGraphBuilder *)0xdeadbeef;
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &ret_graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!ret_graph, "Got unexpected graph %p.\n", ret_graph);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    type = 0xdeadbeef;
    hr = IMediaStream_GetInformation(stream, NULL, &type);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(type == STREAMTYPE_READ, "Got type %u.\n", type);
    IMediaStream_Release(stream);

    ret_graph = NULL;
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &ret_graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!ret_graph, "Got unexpected graph %p.\n", ret_graph);
    IGraphBuilder_Release(ret_graph);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_WRITE, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_WRITE, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_TRANSFORM, 0, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    ret_graph = (IGraphBuilder *)0xdeadbeef;
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &ret_graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!ret_graph, "Got unexpected graph %p.\n", ret_graph);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    type = 0xdeadbeef;
    hr = IMediaStream_GetInformation(stream, NULL, &type);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(type == STREAMTYPE_WRITE, "Got type %u.\n", type);
    IMediaStream_Release(stream);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_TRANSFORM, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_TRANSFORM, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_WRITE, 0, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    ret_graph = (IGraphBuilder *)0xdeadbeef;
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &ret_graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!ret_graph, "Got unexpected graph %p.\n", ret_graph);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    type = 0xdeadbeef;
    hr = IMediaStream_GetInformation(stream, NULL, &type);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(type == STREAMTYPE_TRANSFORM, "Got type %u.\n", type);
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

    ret_graph = NULL;
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &ret_graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!ret_graph, "Got unexpected graph %p.\n", ret_graph);
    IGraphBuilder_Release(ret_graph);

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

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, (IGraphBuilder *)&graph);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_WRITE, 0, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_TRANSFORM, 0, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    IMediaStreamFilter_Release(filter);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IFilterGraph2_Release(graph_inner);
    ok(graph_refcount == 1, "Got outstanding refcount %d.\n", graph_refcount);
    IUnknown_Release(graph_inner_unk);
}

static void test_set_state(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    struct testfilter source;
    IGraphBuilder *graph;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph != NULL, "Expected non-NULL graph.\n");
    testfilter_init(&source);

    hr = IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    source.init_stream_hr = E_FAIL;
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);
    source.init_stream_hr = S_OK;
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    source.init_stream_hr = S_FALSE;
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    source.init_stream_hr = S_OK;
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    source.cleanup_stream_hr = E_FAIL;
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);
    source.cleanup_stream_hr = S_OK;
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    source.cleanup_stream_hr = S_FALSE;
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    source.cleanup_stream_hr = S_OK;

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
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
    static const VIDEOINFOHEADER2 req_vih2;
    static const VIDEOINFOHEADER req_vih;
    static const MPEG2VIDEOINFO req_m2vi;
    static const MPEG1VIDEOINFO req_m1vi;
    static const WAVEFORMATEX req_wfx;
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

    static const struct
    {
        const GUID *type;
        BYTE *format;
        ULONG size;
    }
    tests[] =
    {
        {&GUID_NULL, NULL, 0 },
        {&FORMAT_None, NULL, 0 },
        {&FORMAT_WaveFormatEx, (BYTE *)&req_wfx, sizeof(WAVEFORMATEX)},
        {&FORMAT_MPEG2Video, (BYTE *)&req_m2vi, sizeof(MPEG2VIDEOINFO)},
        {&FORMAT_MPEGVideo, (BYTE *)&req_m1vi, sizeof(MPEG2VIDEOINFO)},
        {&FORMAT_VideoInfo2, (BYTE *)&req_vih2, sizeof(VIDEOINFOHEADER2)},
        {&FORMAT_VideoInfo, (BYTE *)&req_vih, sizeof(VIDEOINFOHEADER)},
    };

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
        &MEDIASUBTYPE_Avi,
        &MEDIASUBTYPE_I420,
        &MEDIASUBTYPE_AYUV,
        &MEDIASUBTYPE_YV12,
        &MEDIASUBTYPE_YUY2,
        &MEDIASUBTYPE_UYVY,
        &MEDIASUBTYPE_YVYU,
        &MEDIASUBTYPE_NV12,
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

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        pmt->formattype = *tests[i].type;
        pmt->cbFormat = tests[i].size;
        pmt->pbFormat = tests[i].format;
        hr = IPin_QueryAccept(pin, pmt);
        ok(hr == (i == 6 ? S_OK : VFW_E_TYPE_NOT_ACCEPTED), "Got hr %#x.\n", hr);
    }

    pmt->bFixedSizeSamples = FALSE;
    pmt->bTemporalCompression = TRUE;
    pmt->lSampleSize = 123;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    pmt->majortype = MEDIATYPE_NULL;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);
    pmt->majortype = MEDIATYPE_Audio;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);
    pmt->majortype = MEDIATYPE_Stream;
    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);
    pmt->majortype = MEDIATYPE_Video;

    for (i = 0; i < ARRAY_SIZE(rejected_subtypes); ++i)
    {
        pmt->subtype = *rejected_subtypes[i];
        hr = IPin_QueryAccept(pin, pmt);
        ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x for subtype %s.\n",
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
    ok(IsEqualGUID(&pmt->subtype, &GUID_NULL), "Got subtype %s\n",
            wine_dbgstr_guid(&pmt->subtype));
    ok(pmt->bFixedSizeSamples == TRUE, "Got fixed size %d.\n", pmt->bFixedSizeSamples);
    ok(!pmt->bTemporalCompression, "Got temporal compression %d.\n", pmt->bTemporalCompression);
    ok(pmt->lSampleSize == 2, "Got sample size %u.\n", pmt->lSampleSize);
    ok(IsEqualGUID(&pmt->formattype, &FORMAT_WaveFormatEx), "Got format type %s.\n",
            wine_dbgstr_guid(&pmt->formattype));
    ok(!pmt->pUnk, "Got pUnk %p.\n", pmt->pUnk);
    ok(pmt->cbFormat == sizeof(WAVEFORMATEX), "Got format size %u.\n", pmt->cbFormat);
    ok(!memcmp(pmt->pbFormat, &expect_wfx, pmt->cbFormat), "Format blocks didn't match.\n");

    hr = IPin_QueryAccept(pin, pmt);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    CoTaskMemFree(pmt->pbFormat);
    CoTaskMemFree(pmt);

    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumMediaTypes_Release(enummt);
    IPin_Release(pin);
    IMediaStream_Release(stream);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
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

struct testclock
{
    IReferenceClock IReferenceClock_iface;
    LONG refcount;
    LONGLONG time;
    HRESULT get_time_hr;
};

static inline struct testclock *impl_from_IReferenceClock(IReferenceClock *iface)
{
    return CONTAINING_RECORD(iface, struct testclock, IReferenceClock_iface);
}

static HRESULT WINAPI testclock_QueryInterface(IReferenceClock *iface, REFIID iid, void **out)
{
    if (winetest_debug > 1) trace("QueryInterface(%s)\n", wine_dbgstr_guid(iid));
    if (IsEqualGUID(iid, &IID_IReferenceClock)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        *out = iface;
        IReferenceClock_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI testclock_AddRef(IReferenceClock *iface)
{
    struct testclock *clock = impl_from_IReferenceClock(iface);
    return InterlockedIncrement(&clock->refcount);
}

static ULONG WINAPI testclock_Release(IReferenceClock *iface)
{
    struct testclock *clock = impl_from_IReferenceClock(iface);
    return InterlockedDecrement(&clock->refcount);
}

static HRESULT WINAPI testclock_GetTime(IReferenceClock *iface, REFERENCE_TIME *time)
{
    struct testclock *clock = impl_from_IReferenceClock(iface);
    if (SUCCEEDED(clock->get_time_hr))
        *time = clock->time;
    return clock->get_time_hr;
}

static HRESULT WINAPI testclock_AdviseTime(IReferenceClock *iface, REFERENCE_TIME base, REFERENCE_TIME offset, HEVENT event, DWORD_PTR *cookie)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testclock_AdvisePeriodic(IReferenceClock *iface, REFERENCE_TIME start, REFERENCE_TIME period, HSEMAPHORE semaphore, DWORD_PTR *cookie)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testclock_Unadvise(IReferenceClock *iface, DWORD_PTR cookie)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static IReferenceClockVtbl testclock_vtbl =
{
    testclock_QueryInterface,
    testclock_AddRef,
    testclock_Release,
    testclock_GetTime,
    testclock_AdviseTime,
    testclock_AdvisePeriodic,
    testclock_Unadvise,
};

static void testclock_init(struct testclock *clock)
{
    memset(clock, 0, sizeof(*clock));
    clock->IReferenceClock_iface.lpVtbl = &testclock_vtbl;
}

static void test_audiostream_get_format(void)
{
    static const WAVEFORMATEX pin_format =
    {
        .wFormatTag = WAVE_FORMAT_PCM,
        .nChannels = 2,
        .nSamplesPerSec = 44100,
        .wBitsPerSample = 16,
        .nBlockAlign = 4,
        .nAvgBytesPerSec = 4 * 44100,
    };
    AM_MEDIA_TYPE mt =
    {
        .majortype = MEDIATYPE_Audio,
        .subtype = MEDIASUBTYPE_PCM,
        .formattype = FORMAT_WaveFormatEx,
        .cbFormat = sizeof(WAVEFORMATEX),
        .pbFormat = (BYTE *)&pin_format,
    };
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IAudioMediaStream *audio_stream;
    struct testfilter source;
    IGraphBuilder *graph;
    IMediaStream *stream;
    WAVEFORMATEX format;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IAudioMediaStream, (void **)&audio_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!graph, "Expected non-NULL graph.\n");

    testfilter_init(&source);

    hr = IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, L"source");
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioMediaStream_GetFormat(audio_stream, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IAudioMediaStream_GetFormat(audio_stream, &format);
    ok(hr == MS_E_NOSTREAM, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    memset(&format, 0xcc, sizeof(format));
    hr = IAudioMediaStream_GetFormat(audio_stream, &format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(format.wFormatTag == WAVE_FORMAT_PCM, "Got tag %#x.\n", format.wFormatTag);
    ok(format.nChannels == 2, "Got %u channels.\n", format.nChannels);
    ok(format.nSamplesPerSec == 44100, "Got sample rate %u.\n", format.nSamplesPerSec);
    ok(format.nAvgBytesPerSec == 176400, "Got %u bytes/sec.\n", format.nAvgBytesPerSec);
    ok(format.nBlockAlign == 4, "Got alignment %u.\n", format.nBlockAlign);
    ok(format.wBitsPerSample == 16, "Got %u bits/sample.\n", format.wBitsPerSample);
    ok(!format.cbSize, "Got extra size %u.\n", format.cbSize);

    hr = IGraphBuilder_Disconnect(graph, pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioMediaStream_GetFormat(audio_stream, &format);
    ok(hr == MS_E_NOSTREAM, "Got hr %#x.\n", hr);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin);
    IAudioMediaStream_Release(audio_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static HRESULT set_audiostream_format(const WAVEFORMATEX *format)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IAudioMediaStream *audio_stream;
    IMediaStream *stream;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IAudioMediaStream, (void **)&audio_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioMediaStream_SetFormat(audio_stream, format);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IAudioMediaStream_Release(audio_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    return hr;
}

static void test_audiostream_set_format(void)
{
    static const WAVEFORMATEX valid_format =
    {
        .wFormatTag = WAVE_FORMAT_PCM,
        .nChannels = 2,
        .nSamplesPerSec = 44100,
        .wBitsPerSample = 16,
        .nBlockAlign = 4,
        .nAvgBytesPerSec = 4 * 44100,
    };

    const AM_MEDIA_TYPE mt =
    {
        .majortype = MEDIATYPE_Audio,
        .subtype = MEDIASUBTYPE_PCM,
        .formattype = FORMAT_WaveFormatEx,
        .cbFormat = sizeof(WAVEFORMATEX),
        .pbFormat = (BYTE *)&valid_format,
    };

    WAVEFORMATEXTENSIBLE extensible_format;
    IAudioMediaStream *audio_stream;
    IAMMultiMediaStream *mmstream;
    struct testfilter source;
    IGraphBuilder *graph;
    IMediaStream *stream;
    WAVEFORMATEX format;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    hr = set_audiostream_format(&valid_format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = set_audiostream_format(NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    extensible_format.Format = valid_format;
    extensible_format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    extensible_format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    extensible_format.Samples.wValidBitsPerSample = valid_format.wBitsPerSample;
    extensible_format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
    extensible_format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    hr = set_audiostream_format(&extensible_format.Format);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    format = valid_format;
    format.nBlockAlign = 1;
    hr = set_audiostream_format(&format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    format = valid_format;
    format.nAvgBytesPerSec = 1234;
    hr = set_audiostream_format(&format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    mmstream = create_ammultimediastream();

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IAudioMediaStream, (void **)&audio_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioMediaStream_SetFormat(audio_stream, &valid_format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioMediaStream_GetFormat(audio_stream, &format);
    ok(hr == MS_E_NOSTREAM, "Got hr %#x.\n", hr);

    format = valid_format;
    format.nChannels = 1;
    hr = IAudioMediaStream_SetFormat(audio_stream, &format);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    format = valid_format;
    format.nSamplesPerSec = 11025;
    hr = IAudioMediaStream_SetFormat(audio_stream, &format);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    format = valid_format;
    format.nAvgBytesPerSec = 1234;
    hr = IAudioMediaStream_SetFormat(audio_stream, &format);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    format = valid_format;
    format.nBlockAlign = 1;
    hr = IAudioMediaStream_SetFormat(audio_stream, &format);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    format = valid_format;
    format.wBitsPerSample = 8;
    hr = IAudioMediaStream_SetFormat(audio_stream, &format);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    format = valid_format;
    format.cbSize = 1;
    hr = IAudioMediaStream_SetFormat(audio_stream, &format);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IAudioMediaStream_SetFormat(audio_stream, &valid_format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IAudioMediaStream_Release(audio_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IAudioMediaStream, (void **)&audio_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!graph, "Expected non-NULL graph.\n");

    testfilter_init(&source);

    hr = IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    format = valid_format;
    format.nChannels = 1;
    hr = IAudioMediaStream_SetFormat(audio_stream, &format);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_Disconnect(graph, pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    format = valid_format;
    format.nChannels = 1;
    hr = IAudioMediaStream_SetFormat(audio_stream, &format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin);
    IAudioMediaStream_Release(audio_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_audiostream_receive_connection(void)
{
    WAVEFORMATEXTENSIBLE extensible_format;
    IAudioMediaStream *audio_stream;
    IAMMultiMediaStream *mmstream;
    struct testfilter source;
    IGraphBuilder *graph;
    IMediaStream *stream;
    WAVEFORMATEX format;
    AM_MEDIA_TYPE mt;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    mmstream = create_ammultimediastream();
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IAudioMediaStream, (void **)&audio_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph != NULL, "Expected non-null graph\n");
    testfilter_init(&source);
    hr = IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IGraphBuilder_Disconnect(graph, pin);
    IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);

    mt = audio_mt;
    mt.majortype = GUID_NULL;
    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);

    mt = audio_mt;
    mt.subtype = MEDIASUBTYPE_RGB24;
    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IGraphBuilder_Disconnect(graph, pin);
    IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);

    mt = audio_mt;
    mt.formattype = GUID_NULL;
    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);

    mt = audio_mt;
    mt.cbFormat = sizeof(WAVEFORMATEX) - 1;
    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);

    extensible_format.Format = audio_format;
    extensible_format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    extensible_format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    extensible_format.Samples.wValidBitsPerSample = audio_format.wBitsPerSample;
    extensible_format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
    extensible_format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    mt = audio_mt;
    mt.cbFormat = sizeof(extensible_format);
    mt.pbFormat = (BYTE *)&extensible_format;
    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &mt);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IAudioMediaStream_SetFormat(audio_stream, &audio_format);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    format = audio_format;
    format.nChannels = 2;
    mt = audio_mt;
    mt.pbFormat = (BYTE *)&format;
    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &mt);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IGraphBuilder_Disconnect(graph, pin);
    IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin);
    IAudioMediaStream_Release(audio_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_ddrawstream_receive_connection(void)
{
    static const VIDEOINFOHEADER req_vih;
    IDirectDrawMediaStream *ddraw_stream;
    IAMMultiMediaStream *mmstream;
    struct testfilter source;
    DDSURFACEDESC format;
    IMediaStream *stream;
    VIDEOINFO video_info;
    AM_MEDIA_TYPE mt;
    HRESULT hr;
    ULONG ref;
    IPin *pin;
    int i;

    static const VIDEOINFO yuy2_video_info =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = 333,
        .bmiHeader.biHeight = -444,
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biBitCount = 16,
        .bmiHeader.biCompression = MAKEFOURCC('Y', 'U', 'Y', '2'),
    };

    const AM_MEDIA_TYPE yuy2_mt =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_YUY2,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(VIDEOINFOHEADER),
        .pbFormat = (BYTE *)&yuy2_video_info,
    };

    const AM_MEDIA_TYPE video_mt =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_RGB8,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(VIDEOINFOHEADER),
        .pbFormat = (BYTE *)&req_vih,
    };

    static const GUID *subtypes[] =
    {
        &MEDIASUBTYPE_RGB24,
        &MEDIASUBTYPE_RGB32,
        &MEDIASUBTYPE_RGB555,
        &MEDIASUBTYPE_RGB565,
        &MEDIASUBTYPE_RGB1,
        &MEDIASUBTYPE_RGB4,
        &MEDIASUBTYPE_ARGB32,
        &MEDIASUBTYPE_ARGB1555,
        &MEDIASUBTYPE_ARGB4444,
        &MEDIASUBTYPE_Avi,
        &MEDIASUBTYPE_I420,
        &MEDIASUBTYPE_AYUV,
        &MEDIASUBTYPE_YV12,
        &MEDIASUBTYPE_YUY2,
        &MEDIASUBTYPE_UYVY,
        &MEDIASUBTYPE_YVYU,
        &MEDIASUBTYPE_NV12,
        &GUID_NULL,
    };

    mmstream = create_ammultimediastream();
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IDirectDrawMediaStream, (void **)&ddraw_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    testfilter_init(&source);

    mt = video_mt;
    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_Disconnect(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(subtypes); ++i)
    {
        mt = video_mt;
        mt.subtype = *subtypes[i];
        hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &mt);
        ok(hr == (i < 4 ? S_OK : VFW_E_TYPE_NOT_ACCEPTED), "Got hr %#x.\n", hr);
        if (hr == S_OK)
        {
            hr = IPin_Disconnect(pin);
            ok(hr == S_OK, "Got hr %#x.\n", hr);
        }
    }

    format = rgb8_format;
    format.dwFlags = DDSD_WIDTH;
    format.dwWidth = 222;
    format.dwHeight = 555;
    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb32_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_Disconnect(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    format = rgb8_format;
    format.dwFlags = DDSD_HEIGHT;
    format.dwWidth = 333;
    format.dwHeight = 444;
    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    video_info = rgb555_video_info;
    video_info.bmiHeader.biWidth = 333;
    video_info.bmiHeader.biHeight = 444;
    mt = rgb555_mt;
    mt.pbFormat = (BYTE *)&video_info;
    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_Disconnect(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb32_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_Disconnect(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    video_info = rgb32_video_info;
    video_info.bmiHeader.biWidth = 332;
    video_info.bmiHeader.biHeight = 444;
    mt = rgb32_mt;
    mt.pbFormat = (BYTE *)&video_info;
    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);

    video_info = rgb32_video_info;
    video_info.bmiHeader.biWidth = 333;
    video_info.bmiHeader.biHeight = 443;
    mt = rgb32_mt;
    mt.pbFormat = (BYTE *)&video_info;
    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &rgb8_format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb555_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb8_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_Disconnect(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &rgb555_format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb565_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb555_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_Disconnect(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &rgb565_format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb24_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb565_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_Disconnect(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &rgb24_format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb32_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb24_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_Disconnect(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &rgb32_format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb8_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb32_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_Disconnect(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &yuy2_format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &yuy2_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);

    format = yuy2_format;
    format.ddpfPixelFormat.dwRBitMask = 0xf800;
    format.ddpfPixelFormat.dwGBitMask = 0x07e0;
    format.ddpfPixelFormat.dwBBitMask = 0x001f;
    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb565_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);

    format = rgb8_format;
    format.dwFlags = 0;
    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_ReceiveConnection(pin, &source.source.pin.IPin_iface, &rgb565_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_Disconnect(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin);
    IDirectDrawMediaStream_Release(ddraw_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_audiostream_receive(void)
{
    ALLOCATOR_PROPERTIES properties =
    {
        .cBuffers = 3,
        .cbBuffer = 16,
        .cbAlign = 1,
    };

    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    ALLOCATOR_PROPERTIES actual;
    struct testfilter source;
    IMemAllocator *allocator;
    IGraphBuilder *graph;
    IMediaStream *stream;
    IMediaSample *sample1;
    IMediaSample *sample2;
    IMediaSample *sample3;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph != NULL, "Expected non-NULL graph.\n");
    testfilter_init(&source);
    hr = IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER, &IID_IMemAllocator, (void **)&allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemAllocator_SetProperties(allocator, &properties, &actual);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMemAllocator_GetBuffer(allocator, &sample1, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemInputPin_Receive(source.source.pMemInputPin, sample1);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#x.\n", hr);
    ref = IMediaSample_Release(sample1);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMemAllocator_GetBuffer(allocator, &sample1, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemInputPin_Receive(source.source.pMemInputPin, sample1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(sample1);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);

    hr = IMemAllocator_GetBuffer(allocator, &sample2, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemInputPin_Receive(source.source.pMemInputPin, sample2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(sample2);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMemAllocator_GetBuffer(allocator, &sample3, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemInputPin_Receive(source.source.pMemInputPin, sample3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(sample3);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = IMediaSample_Release(sample1);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaSample_Release(sample2);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaSample_Release(sample3);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = IMemAllocator_GetBuffer(allocator, &sample1, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemInputPin_Receive(source.source.pMemInputPin, sample1);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#x.\n", hr);
    ref = IMediaSample_Release(sample1);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    IGraphBuilder_Disconnect(graph, pin);
    IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);

    hr = IMemAllocator_Decommit(allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMemAllocator_Release(allocator);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_audiostream_initialize(void)
{
    IAMMediaStream *stream;
    STREAM_TYPE type;
    MSPID mspid;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&CLSID_AMAudioStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Crashes on native. */
    if (0)
    {
        hr = IAMMediaStream_Initialize(stream, NULL, 0, NULL, STREAMTYPE_WRITE);
        ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    }

    hr = IAMMediaStream_Initialize(stream, NULL, 0, &test_mspid, STREAMTYPE_WRITE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMediaStream_GetInformation(stream, &mspid, &type);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&mspid, &test_mspid), "Got mspid %s.\n", wine_dbgstr_guid(&mspid));
    ok(type == STREAMTYPE_WRITE, "Got type %u.\n", type);

    hr = IAMMediaStream_Initialize(stream, NULL, 0, &MSPID_PrimaryAudio, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMediaStream_GetInformation(stream, &mspid, &type);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&mspid, &MSPID_PrimaryAudio), "Got mspid %s.\n", wine_dbgstr_guid(&mspid));
    ok(type == STREAMTYPE_READ, "Got type %u.\n", type);

    ref = IAMMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_audiostream_begin_flush_end_flush(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IAudioStreamSample *stream_sample;
    IAudioMediaStream *audio_stream;
    IMediaSample *media_sample;
    struct testfilter source;
    IAudioData *audio_data;
    IGraphBuilder *graph;
    IMediaStream *stream;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IAudioMediaStream, (void **)&audio_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph != NULL, "Expected non-NULL graph.\n");
    testfilter_init(&source);
    hr = IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER, &IID_IAudioData, (void **)&audio_data);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAudioData_SetBuffer(audio_data, 16, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAudioMediaStream_CreateSample(audio_stream, audio_data, 0, &stream_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = BaseOutputPinImpl_GetDeliveryBuffer(&source.source, &media_sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemInputPin_Receive(source.source.pMemInputPin, media_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(media_sample);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_BeginFlush(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = IMediaSample_Release(media_sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = BaseOutputPinImpl_GetDeliveryBuffer(&source.source, &media_sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemInputPin_Receive(source.source.pMemInputPin, media_sample);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    ref = IMediaSample_Release(media_sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = IAudioStreamSample_Update(stream_sample, SSUPDATE_ASYNC, NULL, NULL, 0);
    ok(hr == MS_S_PENDING, "Got hr %#x.\n", hr);

    hr = IPin_EndOfStream(pin);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    hr = IPin_EndFlush(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = BaseOutputPinImpl_GetDeliveryBuffer(&source.source, &media_sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemInputPin_Receive(source.source.pMemInputPin, media_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = IMediaSample_Release(media_sample);
    ok(ref == 1, "Got outstanding refcount %d.\n", ref);

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    IGraphBuilder_Disconnect(graph, pin);
    IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);

    ref = IAudioStreamSample_Release(stream_sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAudioData_Release(audio_data);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin);
    IAudioMediaStream_Release(audio_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static IMediaSample *audiostream_allocate_sample(struct testfilter *source, const BYTE *input_data, DWORD input_length)
{
    IMediaSample *sample;
    BYTE *sample_data;
    HRESULT hr;

    hr = BaseOutputPinImpl_GetDeliveryBuffer(&source->source, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSample_GetPointer(sample, &sample_data);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSample_SetActualDataLength(sample, input_length);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    memcpy(sample_data, input_data, input_length);

    return sample;
}

static void test_audiostream_new_segment(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    static const BYTE test_data[8] = { 0 };
    IAudioStreamSample *stream_sample;
    IAudioMediaStream *audio_stream;
    IMemInputPin *mem_input_pin;
    IMediaSample *media_sample;
    struct testfilter source;
    IAudioData *audio_data;
    STREAM_TIME start_time;
    STREAM_TIME end_time;
    IGraphBuilder *graph;
    IMediaStream *stream;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IAudioMediaStream, (void **)&audio_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IMemInputPin, (void **)&mem_input_pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph != NULL, "Expected non-NULL graph.\n");
    testfilter_init(&source);
    hr = IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER, &IID_IAudioData, (void **)&audio_data);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAudioMediaStream_CreateSample(audio_stream, audio_data, 0, &stream_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAudioData_SetBuffer(audio_data, 5, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_NewSegment(pin, 11111111, 22222222, 1.0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    media_sample = audiostream_allocate_sample(&source, test_data, 5);
    start_time = 12345678;
    end_time = 23456789;
    hr = IMediaSample_SetTime(media_sample, &start_time, &end_time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemInputPin_Receive(mem_input_pin, media_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IMediaSample_Release(media_sample);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    start_time = 0xdeadbeefdeadbeef;
    end_time = 0xdeadbeefdeadbeef;
    hr = IAudioStreamSample_GetSampleTimes(stream_sample, &start_time, &end_time, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(start_time == 23456789, "Got start time %s.\n", wine_dbgstr_longlong(start_time));
    ok(end_time == 23459057, "Got end time %s.\n", wine_dbgstr_longlong(end_time));

    hr = IPin_NewSegment(pin, 11111111, 22222222, 2.0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    media_sample = audiostream_allocate_sample(&source, test_data, 5);
    start_time = 12345678;
    end_time = 23456789;
    hr = IMediaSample_SetTime(media_sample, &start_time, &end_time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemInputPin_Receive(mem_input_pin, media_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IMediaSample_Release(media_sample);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    start_time = 0xdeadbeefdeadbeef;
    end_time = 0xdeadbeefdeadbeef;
    hr = IAudioStreamSample_GetSampleTimes(stream_sample, &start_time, &end_time, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(start_time == 23456789, "Got start time %s.\n", wine_dbgstr_longlong(start_time));
    ok(end_time == 23459057, "Got end time %s.\n", wine_dbgstr_longlong(end_time));

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IGraphBuilder_Disconnect(graph, pin);
    IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);

    ref = IAudioStreamSample_Release(stream_sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAudioData_Release(audio_data);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin);
    IMemInputPin_Release(mem_input_pin);
    IAudioMediaStream_Release(audio_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void CALLBACK apc_func(ULONG_PTR param)
{
}

static IPin *audiostream_pin;
static IMemInputPin *audiostream_mem_input_pin;
static IMediaSample *audiostream_media_sample;

static DWORD CALLBACK audiostream_end_of_stream(void *param)
{
    HRESULT hr;

    Sleep(100);
    hr = IPin_EndOfStream(audiostream_pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    return 0;
}

static DWORD CALLBACK audiostream_receive(void *param)
{
    HRESULT hr;

    Sleep(100);
    hr = IMemInputPin_Receive(audiostream_mem_input_pin, audiostream_media_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    return 0;
}

static void test_audiostreamsample_update(void)
{
    static const BYTE test_data[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IAudioStreamSample *stream_sample;
    IAudioMediaStream *audio_stream;
    IMediaControl *media_control;
    IMemInputPin *mem_input_pin;
    IMediaSample *media_sample1;
    IMediaSample *media_sample2;
    struct testfilter source;
    IAudioData *audio_data;
    IGraphBuilder *graph;
    IMediaStream *stream;
    DWORD actual_length;
    BYTE buffer[6];
    HANDLE thread;
    HANDLE event;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IAudioMediaStream, (void **)&audio_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IMemInputPin, (void **)&mem_input_pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph != NULL, "Expected non-NULL graph.\n");
    hr = IGraphBuilder_QueryInterface(graph, &IID_IMediaControl, (void **)&media_control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    testfilter_init(&source);
    hr = IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER, &IID_IAudioData, (void **)&audio_data);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAudioMediaStream_CreateSample(audio_stream, audio_data, 0, &stream_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(event != NULL, "Expected non-NULL event.");

    hr = IAudioStreamSample_Update(stream_sample, 0, event, apc_func, 0);
    ok(hr == MS_E_NOTINIT, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == MS_E_NOTINIT, "Got hr %#x.\n", hr);

    hr = IAudioData_SetBuffer(audio_data, sizeof(buffer), buffer, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_Update(stream_sample, 0, event, apc_func, 0);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == MS_E_NOTRUNNING, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == MS_S_ENDOFSTREAM, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    media_sample1 = audiostream_allocate_sample(&source, test_data, 8);
    hr = IMemInputPin_Receive(mem_input_pin, media_sample1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(media_sample1);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioData_GetInfo(audio_data, NULL, NULL, &actual_length);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(actual_length == 6, "Got actual length %u.\n", actual_length);

    ok(memcmp(buffer, test_data, 6) == 0, "Sample data didn't match.\n");

    ref = get_refcount(media_sample1);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);

    media_sample2 = audiostream_allocate_sample(&source, test_data, 8);
    hr = IMemInputPin_Receive(mem_input_pin, media_sample2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(media_sample2);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioData_GetInfo(audio_data, NULL, NULL, &actual_length);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(actual_length == 6, "Got actual length %u.\n", actual_length);

    ok(memcmp(buffer, &test_data[6], 2) == 0, "Sample data didn't match.\n");
    ok(memcmp(&buffer[2], test_data, 4) == 0, "Sample data didn't match.\n");

    ref = IMediaSample_Release(media_sample1);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioData_GetInfo(audio_data, NULL, NULL, &actual_length);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(actual_length == 4, "Got actual length %u.\n", actual_length);

    ok(memcmp(buffer, &test_data[4], 4) == 0, "Sample data didn't match.\n");

    ref = IMediaSample_Release(media_sample2);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == MS_S_ENDOFSTREAM, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaControl_Pause(media_control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    media_sample1 = audiostream_allocate_sample(&source, test_data, 6);
    hr = IMemInputPin_Receive(mem_input_pin, media_sample1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = IMediaSample_Release(media_sample1);
    ok(ref == 1, "Got outstanding refcount %d.\n", ref);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == MS_E_NOTRUNNING, "Got hr %#x.\n", hr);

    hr = IMediaControl_Stop(media_control);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    media_sample1 = audiostream_allocate_sample(&source, test_data, 6);

    audiostream_mem_input_pin = mem_input_pin;
    audiostream_media_sample = media_sample1;
    thread = CreateThread(NULL, 0, audiostream_receive, NULL, 0, NULL);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioData_GetInfo(audio_data, NULL, NULL, &actual_length);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(actual_length == 6, "Got actual length %u.\n", actual_length);

    ok(memcmp(buffer, test_data, 6) == 0, "Sample data didn't match.\n");

    ok(!WaitForSingleObject(thread, 2000), "Wait timed out.\n");
    CloseHandle(thread);

    ref = IMediaSample_Release(media_sample1);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    audiostream_pin = pin;
    thread = CreateThread(NULL, 0, audiostream_end_of_stream, NULL, 0, NULL);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == MS_S_ENDOFSTREAM, "Got hr %#x.\n", hr);

    ok(!WaitForSingleObject(thread, 2000), "Wait timed out.\n");
    CloseHandle(thread);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_Update(stream_sample, SSUPDATE_ASYNC, NULL, NULL, 0);
    ok(hr == MS_S_PENDING, "Got hr %#x.\n", hr);

    IAudioStreamSample_AddRef(stream_sample);
    ref = IAudioStreamSample_Release(stream_sample);
    ok(ref == 1, "Got outstanding refcount %d.\n", ref);

    hr = IAudioStreamSample_Update(stream_sample, SSUPDATE_ASYNC, NULL, NULL, 0);
    ok(hr == MS_E_BUSY, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IGraphBuilder_Disconnect(graph, pin);
    IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == MS_S_ENDOFSTREAM, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);

    CloseHandle(event);
    ref = IAudioStreamSample_Release(stream_sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAudioData_Release(audio_data);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IMediaControl_Release(media_control);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin);
    IMemInputPin_Release(mem_input_pin);
    IAudioMediaStream_Release(audio_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

void test_audiostreamsample_completion_status(void)
{
    static const BYTE test_data[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IAudioStreamSample *stream_sample1;
    IAudioStreamSample *stream_sample2;
    IAudioMediaStream *audio_stream;
    IMediaSample *media_sample;
    struct testfilter source;
    IAudioData *audio_data1;
    IAudioData *audio_data2;
    IGraphBuilder *graph;
    IMediaStream *stream;
    HANDLE event;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(event != NULL, "Expected non-NULL event.");

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IAudioMediaStream, (void **)&audio_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph != NULL, "Expected non-NULL graph.\n");
    testfilter_init(&source);
    hr = IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER, &IID_IAudioData, (void **)&audio_data1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER, &IID_IAudioData, (void **)&audio_data2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAudioData_SetBuffer(audio_data1, 6, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAudioData_SetBuffer(audio_data2, 6, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAudioMediaStream_CreateSample(audio_stream, audio_data1, 0, &stream_sample1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAudioMediaStream_CreateSample(audio_stream, audio_data2, 0, &stream_sample2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_CompletionStatus(stream_sample1, 0, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_Update(stream_sample1, SSUPDATE_ASYNC, NULL, NULL, 0);
    ok(hr == MS_S_PENDING, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_CompletionStatus(stream_sample1, 0, 0);
    ok(hr == MS_S_PENDING, "Got hr %#x.\n", hr);

    media_sample = audiostream_allocate_sample(&source, test_data, 6);
    hr = IMemInputPin_Receive(source.source.pMemInputPin, media_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = IMediaSample_Release(media_sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = IAudioStreamSample_CompletionStatus(stream_sample1, 0, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_Update(stream_sample1, SSUPDATE_ASYNC, NULL, NULL, 0);
    ok(hr == MS_S_PENDING, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_Update(stream_sample2, SSUPDATE_ASYNC, NULL, NULL, 0);
    ok(hr == MS_S_PENDING, "Got hr %#x.\n", hr);

    media_sample = audiostream_allocate_sample(&source, test_data, 12);
    hr = IMemInputPin_Receive(source.source.pMemInputPin, media_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = IMediaSample_Release(media_sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = IAudioStreamSample_CompletionStatus(stream_sample1, 0, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_CompletionStatus(stream_sample2, 0, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_Update(stream_sample1, SSUPDATE_ASYNC, NULL, NULL, 0);
    ok(hr == MS_S_PENDING, "Got hr %#x.\n", hr);

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_CompletionStatus(stream_sample1, 0, 0);
    ok(hr == MS_S_ENDOFSTREAM, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_Update(stream_sample1, SSUPDATE_ASYNC, NULL, NULL, 0);
    ok(hr == MS_S_PENDING, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_CompletionStatus(stream_sample1, 0, 0);
    ok(hr == MS_S_PENDING, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    media_sample = audiostream_allocate_sample(&source, test_data, 6);
    hr = IMemInputPin_Receive(source.source.pMemInputPin, media_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = IMediaSample_Release(media_sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = IAudioStreamSample_CompletionStatus(stream_sample1, 0, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    IGraphBuilder_Disconnect(graph, pin);
    IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);

    ref = IAudioStreamSample_Release(stream_sample1);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAudioStreamSample_Release(stream_sample2);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAudioData_Release(audio_data1);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAudioData_Release(audio_data2);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin);
    IAudioMediaStream_Release(audio_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    CloseHandle(event);
}

static void test_audiostreamsample_get_sample_times(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    static const BYTE test_data[8] = { 0 };
    IAudioStreamSample *stream_sample;
    IMediaFilter *graph_media_filter;
    IAudioMediaStream *audio_stream;
    STREAM_TIME filter_start_time;
    IMemInputPin *mem_input_pin;
    IMediaStreamFilter *filter;
    IMediaSample *media_sample;
    struct testfilter source;
    STREAM_TIME current_time;
    struct testclock clock;
    IAudioData *audio_data;
    STREAM_TIME start_time;
    STREAM_TIME end_time;
    IGraphBuilder *graph;
    IMediaStream *stream;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!filter, "Expected non-null filter.\n");
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IAudioMediaStream, (void **)&audio_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IMemInputPin, (void **)&mem_input_pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph != NULL, "Expected non-NULL graph.\n");
    hr = IGraphBuilder_QueryInterface(graph, &IID_IMediaFilter, (void **)&graph_media_filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    testfilter_init(&source);
    hr = IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER, &IID_IAudioData, (void **)&audio_data);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAudioMediaStream_CreateSample(audio_stream, audio_data, 0, &stream_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAudioData_SetBuffer(audio_data, 5, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    testclock_init(&clock);

    clock.time = 12345678;

    current_time = 0xdeadbeefdeadbeef;
    hr = IAudioStreamSample_GetSampleTimes(stream_sample, NULL, NULL, &current_time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(current_time == 0, "Got current time %s.\n", wine_dbgstr_longlong(current_time));

    IMediaFilter_SetSyncSource(graph_media_filter, &clock.IReferenceClock_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    current_time = 0xdeadbeefdeadbeef;
    hr = IAudioStreamSample_GetSampleTimes(stream_sample, NULL, NULL, &current_time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(current_time == 0, "Got current time %s.\n", wine_dbgstr_longlong(current_time));

    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStreamFilter_GetCurrentStreamTime(filter, &filter_start_time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    clock.get_time_hr = E_FAIL;

    current_time = 0xdeadbeefdeadbeef;
    hr = IAudioStreamSample_GetSampleTimes(stream_sample, NULL, NULL, &current_time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(current_time == 0xdeadbeefddf15da1 + filter_start_time, "Expected current time %s, got %s.\n",
            wine_dbgstr_longlong(0xdeadbeefddf15da1 + filter_start_time), wine_dbgstr_longlong(current_time));

    clock.get_time_hr = S_OK;

    current_time = 0xdeadbeefdeadbeef;
    hr = IAudioStreamSample_GetSampleTimes(stream_sample, NULL, NULL, &current_time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(current_time == filter_start_time, "Expected current time %s, got %s.\n",
            wine_dbgstr_longlong(filter_start_time), wine_dbgstr_longlong(current_time));

    clock.time = 23456789;

    current_time = 0xdeadbeefdeadbeef;
    hr = IAudioStreamSample_GetSampleTimes(stream_sample, NULL, NULL, &current_time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(current_time == filter_start_time + 11111111, "Expected current time %s, got %s.\n",
            wine_dbgstr_longlong(filter_start_time + 11111111), wine_dbgstr_longlong(current_time));

    start_time = 0xdeadbeefdeadbeef;
    end_time = 0xdeadbeefdeadbeef;
    hr = IAudioStreamSample_GetSampleTimes(stream_sample, &start_time, &end_time, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(start_time == 0, "Got start time %s.\n", wine_dbgstr_longlong(start_time));
    ok(end_time == 0, "Got end time %s.\n", wine_dbgstr_longlong(end_time));

    media_sample = audiostream_allocate_sample(&source, test_data, 8);
    start_time = 12345678;
    end_time = 23456789;
    hr = IMediaSample_SetTime(media_sample, &start_time, &end_time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemInputPin_Receive(mem_input_pin, media_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IMediaSample_Release(media_sample);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    start_time = 0xdeadbeefdeadbeef;
    end_time = 0xdeadbeefdeadbeef;
    hr = IAudioStreamSample_GetSampleTimes(stream_sample, &start_time, &end_time, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(start_time == 12345678, "Got start time %s.\n", wine_dbgstr_longlong(start_time));
    ok(end_time == 12347946, "Got end time %s.\n", wine_dbgstr_longlong(end_time));

    media_sample = audiostream_allocate_sample(&source, test_data, 6);
    start_time = 12345678;
    end_time = 23456789;
    hr = IMediaSample_SetTime(media_sample, &start_time, &end_time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemInputPin_Receive(mem_input_pin, media_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IMediaSample_Release(media_sample);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    start_time = 0xdeadbeefdeadbeef;
    end_time = 0xdeadbeefdeadbeef;
    hr = IAudioStreamSample_GetSampleTimes(stream_sample, &start_time, &end_time, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(start_time == 12347946, "Got start time %s.\n", wine_dbgstr_longlong(start_time));
    ok(end_time == 12346585, "Got end time %s.\n", wine_dbgstr_longlong(end_time));

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_Update(stream_sample, 0, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    start_time = 0xdeadbeefdeadbeef;
    end_time = 0xdeadbeefdeadbeef;
    hr = IAudioStreamSample_GetSampleTimes(stream_sample, &start_time, &end_time, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(start_time == 12346585, "Got start time %s.\n", wine_dbgstr_longlong(start_time));
    ok(end_time == 12348399, "Got end time %s.\n", wine_dbgstr_longlong(end_time));

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IGraphBuilder_Disconnect(graph, pin);
    IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);

    ref = IAudioStreamSample_Release(stream_sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAudioData_Release(audio_data);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IMediaFilter_Release(graph_media_filter);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStreamFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin);
    IMemInputPin_Release(mem_input_pin);
    IAudioMediaStream_Release(audio_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_ddrawstream_initialize(void)
{
    IDirectDrawMediaStream *ddraw_stream;
    IAMMediaStream *stream;
    IDirectDraw *ddraw2;
    IDirectDraw *ddraw;
    STREAM_TYPE type;
    MSPID mspid;
    HRESULT hr;
    ULONG ref;

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = CoCreateInstance(&CLSID_AMDirectDrawStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMediaStream_QueryInterface(stream, &IID_IDirectDrawMediaStream, (void **)&ddraw_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Crashes on native. */
    if (0)
    {
        hr = IAMMediaStream_Initialize(stream, NULL, 0, NULL, STREAMTYPE_WRITE);
        ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    }

    hr = IAMMediaStream_Initialize(stream, NULL, 0, &test_mspid, STREAMTYPE_WRITE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMediaStream_GetInformation(stream, &mspid, &type);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&mspid, &test_mspid), "Got mspid %s.\n", wine_dbgstr_guid(&mspid));
    ok(type == STREAMTYPE_WRITE, "Got type %u.\n", type);

    hr = IAMMediaStream_Initialize(stream, (IUnknown *)ddraw, 0, &MSPID_PrimaryAudio, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMediaStream_GetInformation(stream, &mspid, &type);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&mspid, &MSPID_PrimaryAudio), "Got mspid %s.\n", wine_dbgstr_guid(&mspid));
    ok(type == STREAMTYPE_READ, "Got type %u.\n", type);

    hr = IDirectDrawMediaStream_GetDirectDraw(ddraw_stream, &ddraw2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(ddraw2 == ddraw, "Expected ddraw %p, got %p.\n", ddraw, ddraw2);

    IDirectDrawMediaStream_Release(ddraw_stream);
    ref = IAMMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IDirectDraw_Release(ddraw2);
    ref = IDirectDraw_Release(ddraw);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

#define check_ddrawstream_get_format(a,b,c) check_ddrawstream_get_format_(__LINE__,a,b,c)
static void check_ddrawstream_get_format_(int line, IDirectDrawMediaStream *stream,
        const AM_MEDIA_TYPE *mt, const DDSURFACEDESC *expected_format)
{
    DDSURFACEDESC current_format;
    DDSURFACEDESC desired_format;
    struct testfilter source;
    FILTER_INFO filter_info;
    DDSURFACEDESC format;
    PIN_INFO pin_info;
    DWORD flags;
    HRESULT hr;
    IPin *pin;

    hr = IDirectDrawMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_QueryPinInfo(pin, &pin_info);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IBaseFilter_QueryFilterInfo(pin_info.pFilter, &filter_info);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

    testfilter_init(&source);

    hr = IFilterGraph_AddFilter(filter_info.pGraph, &source.filter.IBaseFilter_iface, L"source");
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IFilterGraph_ConnectDirect(filter_info.pGraph, &source.source.pin.IPin_iface, pin, mt);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_GetFormat(stream, NULL, NULL, NULL, NULL);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

    memset(&current_format, 0xcc, sizeof(current_format));
    current_format.dwSize = sizeof(current_format);
    memset(&desired_format, 0xcc, sizeof(desired_format));
    desired_format.dwSize = sizeof(desired_format);
    flags = 0xdeadbeef;
    hr = IDirectDrawMediaStream_GetFormat(stream, &current_format, NULL, &desired_format, &flags);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    memset(&format, 0xcc, sizeof(format));
    format.dwSize = sizeof(format);
    format.ddpfPixelFormat = expected_format->ddpfPixelFormat;
    format.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    format.dwWidth = 333;
    format.dwHeight = 444;
    format.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    ok_(__FILE__, line)(memcmp(&current_format, &format, sizeof(DDSURFACEDESC)) == 0, "Current format didn't match.\n");
    format.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    ok_(__FILE__, line)(memcmp(&desired_format, &format, sizeof(DDSURFACEDESC)) == 0, "Desired format didn't match.\n");

    hr = IFilterGraph_Disconnect(filter_info.pGraph, &source.source.pin.IPin_iface);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFilterGraph_Disconnect(filter_info.pGraph, pin);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IFilterGraph_RemoveFilter(filter_info.pGraph, &source.filter.IBaseFilter_iface);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

    IFilterGraph_Release(filter_info.pGraph);
    IBaseFilter_Release(pin_info.pFilter);
    IPin_Release(pin);
}

static void test_ddrawstream_get_format(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IDirectDrawMediaStream *ddraw_stream;
    DDSURFACEDESC current_format;
    DDSURFACEDESC desired_format;
    IDirectDrawPalette *palette;
    IMediaStream *stream;
    VIDEOINFO video_info;
    AM_MEDIA_TYPE mt;
    DWORD flags;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IDirectDrawMediaStream, (void **)&ddraw_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    current_format.dwSize = sizeof(current_format);
    desired_format.dwSize = sizeof(desired_format);
    hr = IDirectDrawMediaStream_GetFormat(ddraw_stream, &current_format, &palette, &desired_format, &flags);
    ok(hr == MS_E_NOSTREAM, "Got hr %#x.\n", hr);

    video_info = rgb32_video_info;
    video_info.rcSource.right = 222;
    video_info.rcSource.bottom = 333;
    video_info.rcTarget.right = 444;
    video_info.rcTarget.bottom = 666;
    mt = rgb32_mt;
    mt.pbFormat = (BYTE *)&video_info;
    check_ddrawstream_get_format(ddraw_stream, &mt, &rgb32_format);

    video_info = rgb32_video_info;
    video_info.bmiHeader.biHeight = 444;
    mt = rgb32_mt;
    mt.pbFormat = (BYTE *)&video_info;
    check_ddrawstream_get_format(ddraw_stream, &mt, &rgb32_format);

    check_ddrawstream_get_format(ddraw_stream, &rgb8_mt, &rgb8_format);
    check_ddrawstream_get_format(ddraw_stream, &rgb555_mt, &rgb555_format);
    check_ddrawstream_get_format(ddraw_stream, &rgb565_mt, &rgb565_format);
    check_ddrawstream_get_format(ddraw_stream, &rgb24_mt, &rgb24_format);
    check_ddrawstream_get_format(ddraw_stream, &rgb32_mt, &rgb32_format);

    current_format.dwSize = sizeof(current_format);
    desired_format.dwSize = sizeof(desired_format);
    hr = IDirectDrawMediaStream_GetFormat(ddraw_stream, &current_format, &palette, &desired_format, &flags);
    ok(hr == MS_E_NOSTREAM, "Got hr %#x.\n", hr);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IDirectDrawMediaStream_Release(ddraw_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

#define check_ddrawstream_set_format(a,b,c,d) check_ddrawstream_set_format_(__LINE__,a,b,c,d)
static void check_ddrawstream_set_format_(int line, IDirectDrawMediaStream *stream,
        const DDSURFACEDESC *format, const AM_MEDIA_TYPE *mt, HRESULT expected_hr)
{
    struct testfilter source;
    FILTER_INFO filter_info;
    PIN_INFO pin_info;
    HRESULT hr;
    IPin *pin;

    hr = IDirectDrawMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IPin_QueryPinInfo(pin, &pin_info);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IBaseFilter_QueryFilterInfo(pin_info.pFilter, &filter_info);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

    testfilter_init(&source);

    hr = IFilterGraph_AddFilter(filter_info.pGraph, &source.filter.IBaseFilter_iface, L"source");
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_SetFormat(stream, format, NULL);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#x.\n", hr);

    if (mt)
    {
        DDSURFACEDESC current_format;
        DDSURFACEDESC desired_format;
        DWORD flags;

        hr = IFilterGraph_ConnectDirect(filter_info.pGraph, &source.source.pin.IPin_iface, pin, mt);
        ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

        memset(&current_format, 0xcc, sizeof(current_format));
        memset(&desired_format, 0xcc, sizeof(desired_format));
        flags = 0xdeadbeef;
        current_format.dwSize = sizeof(current_format);
        desired_format.dwSize = sizeof(desired_format);
        hr = IDirectDrawMediaStream_GetFormat(stream, &current_format, NULL, &desired_format, &flags);
        ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
        if (format->dwFlags & DDSD_PIXELFORMAT)
        {
            ok_(__FILE__, line)(current_format.dwFlags == (DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT),
                    "Got current format flags %#x.\n", current_format.dwFlags);
            ok_(__FILE__, line)(memcmp(&current_format.ddpfPixelFormat, &format->ddpfPixelFormat, sizeof(DDPIXELFORMAT)) == 0,
                    "Current pixel format didn't match.\n");
            ok_(__FILE__, line)(memcmp(&desired_format.ddpfPixelFormat, &format->ddpfPixelFormat, sizeof(DDPIXELFORMAT)) == 0,
                    "Desired pixel format didn't match.\n");
        }
        else
        {
            ok_(__FILE__, line)(current_format.dwFlags == (DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS),
                    "Got flags %#x.\n", current_format.dwFlags);
        }
        ok_(__FILE__, line)(desired_format.dwFlags == (DDSD_WIDTH | DDSD_HEIGHT),
                "Got desired format flags %#x.\n", desired_format.dwFlags);
        ok_(__FILE__, line)(current_format.ddsCaps.dwCaps == (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY),
                "Got current format caps %#x.\n", current_format.ddsCaps.dwCaps);
        ok_(__FILE__, line)(desired_format.ddsCaps.dwCaps == (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY),
                "Got desired format caps %#x.\n", desired_format.ddsCaps.dwCaps);
        ok_(__FILE__, line)(flags == 0, "Got flags %#x.\n", flags);

        hr = IFilterGraph_Disconnect(filter_info.pGraph, &source.source.pin.IPin_iface);
        ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
        hr = IFilterGraph_Disconnect(filter_info.pGraph, pin);
        ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    }

    hr = IFilterGraph_RemoveFilter(filter_info.pGraph, &source.filter.IBaseFilter_iface);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

    IFilterGraph_Release(filter_info.pGraph);
    IBaseFilter_Release(pin_info.pFilter);
    IPin_Release(pin);
}

static void test_ddrawstream_set_format(void)
{
    static const DDSURFACEDESC rgb1_format =
    {
        .dwSize = sizeof(DDSURFACEDESC),
        .dwFlags = DDSD_PIXELFORMAT,
        .ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT),
        .ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED1,
        .ddpfPixelFormat.dwRGBBitCount = 1,
    };
    static const DDSURFACEDESC rgb2_format =
    {
        .dwSize = sizeof(DDSURFACEDESC),
        .dwFlags = DDSD_PIXELFORMAT,
        .ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT),
        .ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED2,
        .ddpfPixelFormat.dwRGBBitCount = 2,
    };
    static const DDSURFACEDESC rgb4_format =
    {
        .dwSize = sizeof(DDSURFACEDESC),
        .dwFlags = DDSD_PIXELFORMAT,
        .ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT),
        .ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED4,
        .ddpfPixelFormat.dwRGBBitCount = 4,
    };
    static const DDSURFACEDESC rgb4to8_format =
    {
        .dwSize = sizeof(DDSURFACEDESC),
        .dwFlags = DDSD_PIXELFORMAT,
        .ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT),
        .ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXEDTO8,
        .ddpfPixelFormat.dwRGBBitCount = 4,
    };
    static const DDSURFACEDESC rgb332_format =
    {
        .dwSize = sizeof(DDSURFACEDESC),
        .dwFlags = DDSD_PIXELFORMAT,
        .ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT),
        .ddpfPixelFormat.dwFlags = DDPF_RGB,
        .ddpfPixelFormat.dwRGBBitCount = 8,
        .ddpfPixelFormat.dwRBitMask = 0xe0,
        .ddpfPixelFormat.dwGBitMask = 0x1c,
        .ddpfPixelFormat.dwBBitMask = 0x03,
    };

    IDirectDrawMediaStream *ddraw_stream;
    IAMMultiMediaStream *mmstream;
    DDSURFACEDESC current_format;
    DDSURFACEDESC desired_format;
    struct testfilter source;
    IGraphBuilder *graph;
    DDSURFACEDESC format;
    IMediaStream *stream;
    VIDEOINFO video_info;
    AM_MEDIA_TYPE mt;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    mmstream = create_ammultimediastream();

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IDirectDrawMediaStream, (void **)&ddraw_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    check_ddrawstream_set_format(ddraw_stream, &rgb8_format, &rgb8_mt, S_OK);
    check_ddrawstream_set_format(ddraw_stream, &rgb555_format, &rgb555_mt, S_OK);
    check_ddrawstream_set_format(ddraw_stream, &rgb565_format, &rgb565_mt, S_OK);
    check_ddrawstream_set_format(ddraw_stream, &rgb24_format, &rgb24_mt, S_OK);
    check_ddrawstream_set_format(ddraw_stream, &rgb32_format, &rgb32_mt, S_OK);
    check_ddrawstream_set_format(ddraw_stream, &argb32_format, &rgb32_mt, S_OK);
    check_ddrawstream_set_format(ddraw_stream, &yuy2_format, NULL, S_OK);
    check_ddrawstream_set_format(ddraw_stream, &yv12_format, NULL, S_OK);

    format = rgb32_format;
    format.ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS | DDPF_ALPHA
            | DDPF_COMPRESSED | DDPF_RGBTOYUV | DDPF_ZBUFFER | DDPF_ZPIXELS | DDPF_STENCILBUFFER
            | DDPF_ALPHAPREMULT | DDPF_LUMINANCE | DDPF_BUMPLUMINANCE | DDPF_BUMPDUDV;
    check_ddrawstream_set_format(ddraw_stream, &format, &rgb32_mt, S_OK);

    format = yuy2_format;
    format.ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS | DDPF_ALPHA
            | DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXEDTO8 | DDPF_PALETTEINDEXED8
            | DDPF_RGB | DDPF_COMPRESSED | DDPF_RGBTOYUV | DDPF_YUV | DDPF_ZBUFFER
            | DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 | DDPF_ZPIXELS
            | DDPF_STENCILBUFFER | DDPF_ALPHAPREMULT | DDPF_LUMINANCE
            | DDPF_BUMPLUMINANCE | DDPF_BUMPDUDV;
    check_ddrawstream_set_format(ddraw_stream, &format, NULL, S_OK);

    format = rgb32_format;
    format.dwFlags |= DDSD_CAPS;
    format.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
    check_ddrawstream_set_format(ddraw_stream, &format, &rgb32_mt, S_OK);

    format = rgb8_format;
    format.dwFlags = 0;
    check_ddrawstream_set_format(ddraw_stream, &format, &rgb32_mt, S_OK);

    check_ddrawstream_set_format(ddraw_stream, &rgb1_format, NULL, DDERR_INVALIDSURFACETYPE);
    check_ddrawstream_set_format(ddraw_stream, &rgb2_format, NULL, DDERR_INVALIDSURFACETYPE);
    check_ddrawstream_set_format(ddraw_stream, &rgb4_format, NULL, DDERR_INVALIDSURFACETYPE);
    check_ddrawstream_set_format(ddraw_stream, &rgb4to8_format, NULL, DDERR_INVALIDSURFACETYPE);
    check_ddrawstream_set_format(ddraw_stream, &rgb332_format, NULL, DDERR_INVALIDSURFACETYPE);

    format = rgb8_format;
    format.ddpfPixelFormat.dwFlags &= ~DDPF_RGB;
    check_ddrawstream_set_format(ddraw_stream, &format, NULL, DDERR_INVALIDSURFACETYPE);

    format = rgb8_format;
    format.ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED1;
    check_ddrawstream_set_format(ddraw_stream, &format, NULL, DDERR_INVALIDSURFACETYPE);

    format = rgb32_format;
    format.ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED8;
    check_ddrawstream_set_format(ddraw_stream, &format, NULL, DDERR_INVALIDSURFACETYPE);

    format = rgb32_format;
    format.ddpfPixelFormat.dwFlags |= DDPF_YUV;
    check_ddrawstream_set_format(ddraw_stream, &format, NULL, DDERR_INVALIDSURFACETYPE);

    format = rgb565_format;
    format.ddpfPixelFormat.dwRBitMask = 0x001f;
    format.ddpfPixelFormat.dwGBitMask = 0x07e0;
    format.ddpfPixelFormat.dwBBitMask = 0xf800;
    check_ddrawstream_set_format(ddraw_stream, &format, NULL, DDERR_INVALIDSURFACETYPE);

    format = rgb32_format;
    format.ddpfPixelFormat.dwRBitMask = 0x00ff00;
    format.ddpfPixelFormat.dwGBitMask = 0x0000ff;
    format.ddpfPixelFormat.dwBBitMask = 0xff0000;
    check_ddrawstream_set_format(ddraw_stream, &format, NULL, DDERR_INVALIDSURFACETYPE);

    format = yuy2_format;
    format.ddpfPixelFormat.dwYUVBitCount = 0;
    check_ddrawstream_set_format(ddraw_stream, &format, NULL, E_INVALIDARG);

    format = rgb32_format;
    format.dwSize = sizeof(DDSURFACEDESC) + 1;
    check_ddrawstream_set_format(ddraw_stream, &format, NULL, E_INVALIDARG);

    format = rgb32_format;
    format.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT) + 1;
    check_ddrawstream_set_format(ddraw_stream, &format, NULL, DDERR_INVALIDSURFACETYPE);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IDirectDrawMediaStream_Release(ddraw_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    mmstream = create_ammultimediastream();

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IDirectDrawMediaStream, (void **)&ddraw_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!graph, "Expected non-NULL graph.\n");

    testfilter_init(&source);

    hr = IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, L"source");
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &rgb8_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    source.preferred_mt = NULL;

    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &rgb555_format, NULL);
    ok(hr == DDERR_INVALIDSURFACETYPE, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&source.source.pin.mt.subtype, &MEDIASUBTYPE_RGB8),
            "Got subtype %s.\n", wine_dbgstr_guid(&source.source.pin.mt.subtype));
    hr = IDirectDrawMediaStream_GetFormat(ddraw_stream, &current_format, NULL, &desired_format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(current_format.ddpfPixelFormat.dwRGBBitCount == 8,
            "Got rgb bit count %u.\n", current_format.ddpfPixelFormat.dwRGBBitCount);
    ok(desired_format.ddpfPixelFormat.dwRGBBitCount == 8,
            "Got rgb bit count %u.\n", desired_format.ddpfPixelFormat.dwRGBBitCount);

    format = rgb555_format;
    format.dwFlags = 0;
    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&source.source.pin.mt.subtype, &MEDIASUBTYPE_RGB8),
            "Got subtype %s.\n", wine_dbgstr_guid(&source.source.pin.mt.subtype));

    source.preferred_mt = &rgb555_mt;

    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &rgb8_format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &rgb555_format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&source.source.pin.mt.subtype, &MEDIASUBTYPE_RGB555),
            "Got subtype %s.\n", wine_dbgstr_guid(&source.source.pin.mt.subtype));
    hr = IDirectDrawMediaStream_GetFormat(ddraw_stream, &current_format, NULL, &desired_format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(current_format.ddpfPixelFormat.dwRGBBitCount == 16,
            "Got rgb bit count %u.\n", current_format.ddpfPixelFormat.dwRGBBitCount);
    ok(desired_format.ddpfPixelFormat.dwRGBBitCount == 16,
            "Got rgb bit count %u.\n", desired_format.ddpfPixelFormat.dwRGBBitCount);

    video_info = rgb555_video_info;
    video_info.bmiHeader.biWidth = 222;
    video_info.bmiHeader.biHeight = -555;
    mt = rgb555_mt;
    mt.pbFormat = (BYTE *)&video_info;
    source.preferred_mt = &mt;

    format = rgb555_format;
    format.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
    format.dwWidth = 222;
    format.dwHeight = 555;
    hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, &format, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(IsEqualGUID(&source.source.pin.mt.subtype, &MEDIASUBTYPE_RGB555),
            "Got subtype %s.\n", wine_dbgstr_guid(&source.source.pin.mt.subtype));
    ok(((VIDEOINFO *)source.source.pin.mt.pbFormat)->bmiHeader.biWidth == 222,
            "Got width %d.\n", ((VIDEOINFO *)source.source.pin.mt.pbFormat)->bmiHeader.biWidth);
    ok(((VIDEOINFO *)source.source.pin.mt.pbFormat)->bmiHeader.biHeight == -555,
            "Got height %d.\n", ((VIDEOINFO *)source.source.pin.mt.pbFormat)->bmiHeader.biHeight);

    hr = IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_Disconnect(graph, pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin);
    IDirectDrawMediaStream_Release(ddraw_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void check_ammediastream_join_am_multi_media_stream(const CLSID *clsid)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IMultiMediaStream *mmstream2;
    IAMMediaStream *stream;
    HRESULT hr;
    ULONG mmstream_ref;
    ULONG ref;

    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    mmstream_ref = get_refcount(mmstream);

    hr = IAMMediaStream_JoinAMMultiMediaStream(stream, mmstream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = get_refcount(mmstream);
    ok(ref == mmstream_ref, "Expected outstanding refcount %d, got %d.\n", mmstream_ref, ref);

    hr = IAMMediaStream_GetMultiMediaStream(stream, &mmstream2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mmstream2 == (IMultiMediaStream *)mmstream, "Expected mmstream %p, got %p.\n", mmstream, mmstream2);

    IMultiMediaStream_Release(mmstream2);

    hr = IAMMediaStream_JoinAMMultiMediaStream(stream, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMediaStream_GetMultiMediaStream(stream, &mmstream2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mmstream2 == NULL, "Got mmstream %p.\n", mmstream2);

    ref = IAMMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_ammediastream_join_am_multi_media_stream(void)
{
    check_ammediastream_join_am_multi_media_stream(&CLSID_AMAudioStream);
    check_ammediastream_join_am_multi_media_stream(&CLSID_AMDirectDrawStream);
}

static void check_ammediastream_join_filter(const CLSID *clsid)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IMediaStreamFilter *filter, *filter2, *filter3;
    IAMMediaStream *stream;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!filter, "Expected non-null filter.\n");
    EXPECT_REF(filter, 3);

    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(filter, 3);

    hr = CoCreateInstance(&CLSID_MediaStreamFilter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaStreamFilter, (void **)&filter2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(filter, 3);
    EXPECT_REF(filter2, 1);

    hr = IAMMediaStream_JoinFilter(stream, filter2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(filter, 3);
    EXPECT_REF(filter2, 1);

    /* Crashes on native. */
    if (0)
    {
        hr = IAMMediaStream_JoinFilter(stream, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
    }

    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(filter3 == filter, "Expected filter %p, got %p.\n", filter, filter3);
    EXPECT_REF(filter, 4);

    IMediaStreamFilter_Release(filter3);
    EXPECT_REF(filter, 3);

    ref = IMediaStreamFilter_Release(filter2);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    EXPECT_REF(filter, 3);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    EXPECT_REF(filter, 1);
    ref = IMediaStreamFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_ammediastream_join_filter(void)
{
    check_ammediastream_join_filter(&CLSID_AMAudioStream);
    check_ammediastream_join_filter(&CLSID_AMDirectDrawStream);
}

static void check_ammediastream_join_filter_graph(const MSPID *id)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IGraphBuilder *builder, *builder2;
    IMediaStreamFilter *filter;
    IAMMediaStream *stream;
    IFilterGraph *graph;
    FILTER_INFO info;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!filter, "Expected non-null filter.\n");

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, id, 0, (IMediaStream **)&stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &builder);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!builder, "Expected non-null graph.\n");
    EXPECT_REF(builder, 4);

    hr = IMediaStreamFilter_QueryFilterInfo(filter, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(info.pGraph == (IFilterGraph *)builder, "Expected graph %p, got %p.\n", (IFilterGraph *)builder, info.pGraph);
    EXPECT_REF(builder, 5);
    IFilterGraph_Release(info.pGraph);
    EXPECT_REF(builder, 4);

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterGraph, (void **)&graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(builder, 4);
    EXPECT_REF(graph, 1);

    /* Crashes on native. */
    if (0)
    {
        hr = IAMMediaStream_JoinFilterGraph(stream, NULL);
        ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    }

    hr = IAMMediaStream_JoinFilterGraph(stream, graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(builder, 4);
    EXPECT_REF(graph, 1);

    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &builder2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(builder2 == builder, "Expected graph %p, got %p.\n", builder, builder2);
    EXPECT_REF(builder, 5);
    EXPECT_REF(graph, 1);
    IGraphBuilder_Release(builder2);
    EXPECT_REF(builder, 4);
    EXPECT_REF(graph, 1);

    hr = IMediaStreamFilter_QueryFilterInfo(filter, &info);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(info.pGraph == (IFilterGraph *)builder, "Expected graph %p, got %p.\n", (IFilterGraph *)builder, info.pGraph);
    EXPECT_REF(builder, 5);
    EXPECT_REF(graph, 1);
    IFilterGraph_Release(info.pGraph);
    EXPECT_REF(builder, 4);
    EXPECT_REF(graph, 1);

    ref = IFilterGraph_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(builder);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStreamFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_ammediastream_join_filter_graph(void)
{
    check_ammediastream_join_filter_graph(&MSPID_PrimaryAudio);
    check_ammediastream_join_filter_graph(&MSPID_PrimaryVideo);
}

static void check_ammediastream_set_state(const MSPID *id)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IAMMediaStream *am_stream;
    IMediaStream *stream;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, id, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IAMMediaStream, (void **)&am_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMediaStream_SetState(am_stream, 4);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMediaStream_SetState(am_stream, State_Running);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMediaStream_SetState(am_stream, State_Paused);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMediaStream_SetState(am_stream, State_Stopped);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IAMMediaStream_Release(am_stream);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_ammediastream_set_state(void)
{
    check_ammediastream_set_state(&MSPID_PrimaryAudio);
    check_ammediastream_set_state(&MSPID_PrimaryVideo);
}

static void check_ammediastream_end_of_stream(const MSPID *id, const AM_MEDIA_TYPE *mt)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    struct testfilter source;
    IGraphBuilder *graph;
    IMediaStream *stream;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, id, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!graph, "Expected non-NULL graph.\n");
    testfilter_init(&source);
    hr = IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_EndOfStream(pin);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_RUN);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_EndOfStream(pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_EndOfStream(pin);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_SetState(mmstream, STREAMSTATE_STOP);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_EndOfStream(pin);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);

    IGraphBuilder_Disconnect(graph, pin);
    IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_ammediastream_end_of_stream(void)
{
    check_ammediastream_end_of_stream(&MSPID_PrimaryAudio, &audio_mt);
    check_ammediastream_end_of_stream(&MSPID_PrimaryVideo, &rgb32_mt);
}

void test_mediastreamfilter_get_state(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IMediaStreamFilter *filter;
    FILTER_STATE state;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!!filter, "Expected non-null filter.\n");

    /* Crashes on native. */
    if (0)
    {
        hr = IMediaStreamFilter_GetState(filter, 0, NULL);
        ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    }

    state = 0xcc;
    hr = IMediaStreamFilter_GetState(filter, 0, &state);
    ok(state == State_Stopped, "Got state %#x.\n", state);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStreamFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

void check_mediastreamfilter_state(FILTER_STATE expected_state, HRESULT (*set_state)(IMediaStreamFilter *),
        HRESULT (*reset_state)(IMediaStreamFilter *))
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    struct teststream teststream, teststream2;
    IMediaStreamFilter *filter;
    FILTER_STATE state;
    HRESULT hr;
    ULONG ref;

    teststream_init(&teststream);
    teststream_init(&teststream2);

    teststream2.mspid.Data2 = 1;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)&teststream, &teststream.mspid, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)&teststream2, &teststream2.mspid, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(filter != NULL, "Expected non-null filter\n");

    hr = reset_state(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    teststream.state = 0xcc;
    teststream2.state = 0xcc;
    hr = set_state(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(teststream.state == expected_state, "Got state %#x.\n", teststream.state);
    ok(teststream2.state == expected_state, "Got state %#x.\n", teststream2.state);
    hr = IMediaStreamFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == expected_state, "Got state %#x.\n", state);

    teststream.state = 0xcc;
    teststream2.state = 0xcc;
    hr = set_state(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(teststream.state == 0xcc, "Got state %#x.\n", teststream.state);
    ok(teststream2.state == 0xcc, "Got state %#x.\n", teststream2.state);

    hr = reset_state(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    teststream.set_state_result = E_FAIL;
    teststream.state = 0xcc;
    teststream2.state = 0xcc;
    hr = set_state(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(teststream.state == 0xcc, "Got state %#x.\n", teststream.state);
    ok(teststream2.state == expected_state, "Got state %#x.\n", teststream2.state);
    hr = IMediaStreamFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == expected_state, "Got state %#x.\n", state);

    hr = reset_state(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    teststream.set_state_result = E_FAIL;
    teststream2.set_state_result = E_FAIL;
    teststream.state = 0xcc;
    teststream2.state = 0xcc;
    hr = set_state(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(teststream.state == 0xcc, "Got state %#x.\n", teststream.state);
    ok(teststream2.state == 0xcc, "Got state %#x.\n", teststream2.state);
    hr = IMediaStreamFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == expected_state, "Got state %#x.\n", state);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStreamFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ok(teststream.refcount == 1, "Got outstanding refcount %d.\n", teststream.refcount);
    ok(teststream2.refcount == 1, "Got outstanding refcount %d.\n", teststream2.refcount);
}

static HRESULT mediastreamfilter_stop(IMediaStreamFilter *filter)
{
    return IMediaStreamFilter_Stop(filter);
}

static HRESULT mediastreamfilter_pause(IMediaStreamFilter *filter)
{
    return IMediaStreamFilter_Pause(filter);
}

static HRESULT mediastreamfilter_run(IMediaStreamFilter *filter)
{
    return IMediaStreamFilter_Run(filter, 0);
}

void test_mediastreamfilter_stop_pause_run(void)
{
    check_mediastreamfilter_state(State_Stopped, mediastreamfilter_stop, mediastreamfilter_run);
    check_mediastreamfilter_state(State_Paused, mediastreamfilter_pause, mediastreamfilter_stop);
    check_mediastreamfilter_state(State_Running, mediastreamfilter_run, mediastreamfilter_stop);
}

static void test_mediastreamfilter_support_seeking(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    static const MSPID mspid1 = {0x88888888, 1};
    static const MSPID mspid2 = {0x88888888, 2};
    static const MSPID mspid3 = {0x88888888, 3};
    struct testfilter source1, source2, source3;
    IAMMediaStream *stream1, *stream2, *stream3;
    IMediaStreamFilter *filter;
    IPin *pin1, *pin2, *pin3;
    ULONG ref, seeking_ref;
    IGraphBuilder *graph;
    HRESULT hr;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_Initialize(stream1, NULL, 0, &mspid1, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_Initialize(stream2, NULL, 0, &mspid2, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_Initialize(stream3, NULL, 0, &mspid3, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)stream1, &mspid1, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)stream2, &mspid2, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)stream3, &mspid3, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_QueryInterface(stream1, &IID_IPin, (void **)&pin1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_QueryInterface(stream2, &IID_IPin, (void **)&pin2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_QueryInterface(stream3, &IID_IPin, (void **)&pin3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph != NULL, "Expected non-NULL graph.\n");
    testfilter_init(&source1);
    testfilter_init(&source2);
    testfilter_init(&source3);
    source2.IMediaSeeking_iface.lpVtbl = &testsource_seeking_vtbl;
    source3.IMediaSeeking_iface.lpVtbl = &testsource_seeking_vtbl;
    hr = IGraphBuilder_AddFilter(graph, &source1.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_AddFilter(graph, &source2.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_AddFilter(graph, &source3.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStreamFilter_SupportSeeking(filter, TRUE);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source1.source.pin.IPin_iface, pin1, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    source2.get_duration_hr = E_FAIL;

    hr = IMediaStreamFilter_SupportSeeking(filter, TRUE);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source2.source.pin.IPin_iface, pin2, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStreamFilter_SupportSeeking(filter, TRUE);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source3.source.pin.IPin_iface, pin3, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    check_interface(filter, &IID_IMediaSeeking, FALSE);

    seeking_ref = get_refcount(&source3.IMediaSeeking_iface);

    hr = IMediaStreamFilter_SupportSeeking(filter, TRUE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    check_interface(filter, &IID_IMediaSeeking, TRUE);

    ref = get_refcount(&source3.IMediaSeeking_iface);
    ok(ref == seeking_ref, "Expected outstanding refcount %d, got %d.\n", seeking_ref, ref);

    hr = IMediaStreamFilter_SupportSeeking(filter, TRUE);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED), "Got hr %#x.\n", hr);

    IGraphBuilder_Disconnect(graph, pin1);
    IGraphBuilder_Disconnect(graph, &source1.source.pin.IPin_iface);

    IGraphBuilder_Disconnect(graph, pin2);
    IGraphBuilder_Disconnect(graph, &source2.source.pin.IPin_iface);

    IGraphBuilder_Disconnect(graph, pin3);
    IGraphBuilder_Disconnect(graph, &source3.source.pin.IPin_iface);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStreamFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin1);
    ref = IAMMediaStream_Release(stream1);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin2);
    ref = IAMMediaStream_Release(stream2);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin3);
    ref = IAMMediaStream_Release(stream3);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_mediastreamfilter_set_positions(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    static const MSPID mspid1 = {0x88888888, 1};
    static const MSPID mspid2 = {0x88888888, 2};
    static const MSPID mspid3 = {0x88888888, 3};
    IMediaStreamFilter *filter;
    struct testfilter source1;
    struct testfilter source2;
    struct testfilter source3;
    LONGLONG current_position;
    IAMMediaStream *stream1;
    IAMMediaStream *stream2;
    IAMMediaStream *stream3;
    LONGLONG stop_position;
    IMediaSeeking *seeking;
    IGraphBuilder *graph;
    IPin *pin1;
    IPin *pin2;
    IPin *pin3;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_Initialize(stream1, NULL, 0, &mspid1, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_Initialize(stream2, NULL, 0, &mspid2, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_Initialize(stream3, NULL, 0, &mspid3, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)stream1, &mspid1, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)stream2, &mspid2, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)stream3, &mspid3, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_QueryInterface(stream1, &IID_IPin, (void **)&pin1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_QueryInterface(stream2, &IID_IPin, (void **)&pin2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_QueryInterface(stream3, &IID_IPin, (void **)&pin3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph != NULL, "Expected non-NULL graph.\n");
    testfilter_init(&source1);
    testfilter_init(&source2);
    testfilter_init(&source3);
    source1.IMediaSeeking_iface.lpVtbl = &testsource_seeking_vtbl;
    source2.IMediaSeeking_iface.lpVtbl = &testsource_seeking_vtbl;
    source3.IMediaSeeking_iface.lpVtbl = &testsource_seeking_vtbl;
    hr = IGraphBuilder_AddFilter(graph, &source1.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_AddFilter(graph, &source2.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_AddFilter(graph, &source3.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source2.source.pin.IPin_iface, pin2, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_ConnectDirect(graph, &source3.source.pin.IPin_iface, pin3, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStreamFilter_SupportSeeking(filter, TRUE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source1.source.pin.IPin_iface, pin1, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStreamFilter_QueryInterface(filter, &IID_IMediaSeeking, (void **)&seeking);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    current_position = 12345678;
    stop_position = 87654321;
    source1.current_position = 0xdeadbeefdeadbeefULL;
    source1.stop_position = 0xdeadbeefdeadbeefULL;
    source2.current_position = 0xdeadbeefdeadbeefULL;
    source2.stop_position = 0xdeadbeefdeadbeefULL;
    source3.current_position = 0xdeadbeefdeadbeefULL;
    source3.stop_position = 0xdeadbeefdeadbeefULL;
    hr = IMediaSeeking_SetPositions(seeking, &current_position, AM_SEEKING_AbsolutePositioning,
            &stop_position, AM_SEEKING_AbsolutePositioning);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(source1.current_position == 0xdeadbeefdeadbeefULL, "Got current position %s.\n",
            wine_dbgstr_longlong(source1.current_position));
    ok(source1.stop_position == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n",
            wine_dbgstr_longlong(source1.stop_position));
    ok(source2.current_position == 12345678, "Got current position %s.\n",
            wine_dbgstr_longlong(source2.current_position));
    ok(source2.stop_position == 87654321, "Got stop position %s.\n",
            wine_dbgstr_longlong(source2.stop_position));
    ok(source3.current_position == 0xdeadbeefdeadbeefULL, "Got current position %s.\n",
            wine_dbgstr_longlong(source3.current_position));
    ok(source3.stop_position == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n",
            wine_dbgstr_longlong(source3.stop_position));

    source2.set_positions_hr = E_FAIL;
    source1.current_position = 0xdeadbeefdeadbeefULL;
    source1.stop_position = 0xdeadbeefdeadbeefULL;
    source3.current_position = 0xdeadbeefdeadbeefULL;
    source3.stop_position = 0xdeadbeefdeadbeefULL;
    current_position = 12345678;
    stop_position = 87654321;
    hr = IMediaSeeking_SetPositions(seeking, &current_position, AM_SEEKING_AbsolutePositioning,
            &stop_position, AM_SEEKING_AbsolutePositioning);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);
    ok(source1.current_position == 0xdeadbeefdeadbeefULL, "Got current position %s.\n",
            wine_dbgstr_longlong(source1.current_position));
    ok(source1.stop_position == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n",
            wine_dbgstr_longlong(source1.stop_position));
    ok(source3.current_position == 0xdeadbeefdeadbeefULL, "Got current position %s.\n",
            wine_dbgstr_longlong(source3.current_position));
    ok(source3.stop_position == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n",
            wine_dbgstr_longlong(source3.stop_position));

    source2.set_positions_hr = E_NOTIMPL;
    source1.current_position = 0xdeadbeefdeadbeefULL;
    source1.stop_position = 0xdeadbeefdeadbeefULL;
    source3.current_position = 0xdeadbeefdeadbeefULL;
    source3.stop_position = 0xdeadbeefdeadbeefULL;
    current_position = 12345678;
    stop_position = 87654321;
    hr = IMediaSeeking_SetPositions(seeking, &current_position, AM_SEEKING_AbsolutePositioning,
            &stop_position, AM_SEEKING_AbsolutePositioning);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    ok(source1.current_position == 0xdeadbeefdeadbeefULL, "Got current position %s.\n",
            wine_dbgstr_longlong(source1.current_position));
    ok(source1.stop_position == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n",
            wine_dbgstr_longlong(source1.stop_position));
    ok(source3.current_position == 0xdeadbeefdeadbeefULL, "Got current position %s.\n",
            wine_dbgstr_longlong(source3.current_position));
    ok(source3.stop_position == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n",
            wine_dbgstr_longlong(source3.stop_position));

    source2.IMediaSeeking_iface.lpVtbl = NULL;
    source1.current_position = 0xdeadbeefdeadbeefULL;
    source1.stop_position = 0xdeadbeefdeadbeefULL;
    source3.current_position = 0xdeadbeefdeadbeefULL;
    source3.stop_position = 0xdeadbeefdeadbeefULL;
    current_position = 12345678;
    stop_position = 87654321;
    hr = IMediaSeeking_SetPositions(seeking, &current_position, AM_SEEKING_AbsolutePositioning,
            &stop_position, AM_SEEKING_AbsolutePositioning);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    ok(source1.current_position == 0xdeadbeefdeadbeefULL, "Got current position %s.\n",
            wine_dbgstr_longlong(source1.current_position));
    ok(source1.stop_position == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n",
            wine_dbgstr_longlong(source1.stop_position));
    ok(source3.current_position == 0xdeadbeefdeadbeefULL, "Got current position %s.\n",
            wine_dbgstr_longlong(source3.current_position));
    ok(source3.stop_position == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n",
            wine_dbgstr_longlong(source3.stop_position));

    IGraphBuilder_Disconnect(graph, pin2);
    IGraphBuilder_Disconnect(graph, &source2.source.pin.IPin_iface);

    source2.IMediaSeeking_iface.lpVtbl = NULL;
    source1.current_position = 0xdeadbeefdeadbeefULL;
    source1.stop_position = 0xdeadbeefdeadbeefULL;
    source3.current_position = 0xdeadbeefdeadbeefULL;
    source3.stop_position = 0xdeadbeefdeadbeefULL;
    current_position = 12345678;
    stop_position = 87654321;
    hr = IMediaSeeking_SetPositions(seeking, &current_position, AM_SEEKING_AbsolutePositioning,
            &stop_position, AM_SEEKING_AbsolutePositioning);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    ok(source1.current_position == 0xdeadbeefdeadbeefULL, "Got current position %s.\n",
            wine_dbgstr_longlong(source1.current_position));
    ok(source1.stop_position == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n",
            wine_dbgstr_longlong(source1.stop_position));
    ok(source3.current_position == 0xdeadbeefdeadbeefULL, "Got current position %s.\n",
            wine_dbgstr_longlong(source3.current_position));
    ok(source3.stop_position == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n",
            wine_dbgstr_longlong(source3.stop_position));

    IGraphBuilder_Disconnect(graph, pin2);
    IGraphBuilder_Disconnect(graph, &source2.source.pin.IPin_iface);
    IGraphBuilder_Disconnect(graph, pin3);
    IGraphBuilder_Disconnect(graph, &source3.source.pin.IPin_iface);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IMediaSeeking_Release(seeking);
    ref = IMediaStreamFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin1);
    ref = IAMMediaStream_Release(stream1);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin2);
    ref = IAMMediaStream_Release(stream2);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin3);
    ref = IAMMediaStream_Release(stream3);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_mediastreamfilter_get_duration(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    static const MSPID mspid1 = {0x88888888, 1};
    static const MSPID mspid2 = {0x88888888, 2};
    static const MSPID mspid3 = {0x88888888, 3};
    struct testfilter source1, source2, source3;
    IAMMediaStream *stream1, *stream2, *stream3;
    IMediaStreamFilter *filter;
    IPin *pin1, *pin2, *pin3;
    IMediaSeeking *seeking;
    IGraphBuilder *graph;
    LONGLONG duration;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_Initialize(stream1, NULL, 0, &mspid1, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_Initialize(stream2, NULL, 0, &mspid2, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_Initialize(stream3, NULL, 0, &mspid3, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)stream1, &mspid1, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)stream2, &mspid2, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)stream3, &mspid3, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_QueryInterface(stream1, &IID_IPin, (void **)&pin1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_QueryInterface(stream2, &IID_IPin, (void **)&pin2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_QueryInterface(stream3, &IID_IPin, (void **)&pin3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph != NULL, "Expected non-NULL graph.\n");
    testfilter_init(&source1);
    testfilter_init(&source2);
    testfilter_init(&source3);
    source1.IMediaSeeking_iface.lpVtbl = &testsource_seeking_vtbl;
    source2.IMediaSeeking_iface.lpVtbl = &testsource_seeking_vtbl;
    source3.IMediaSeeking_iface.lpVtbl = &testsource_seeking_vtbl;
    hr = IGraphBuilder_AddFilter(graph, &source1.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_AddFilter(graph, &source2.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_AddFilter(graph, &source3.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source2.source.pin.IPin_iface, pin2, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_ConnectDirect(graph, &source3.source.pin.IPin_iface, pin3, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStreamFilter_SupportSeeking(filter, TRUE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source1.source.pin.IPin_iface, pin1, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStreamFilter_QueryInterface(filter, &IID_IMediaSeeking, (void **)&seeking);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    duration = 0xdeadbeefdeadbeefULL;
    hr = IMediaSeeking_GetDuration(seeking, &duration);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(duration == 0x8000000000000000ULL, "Got duration %s.\n", wine_dbgstr_longlong(duration));

    source2.get_duration_hr = E_FAIL;
    duration = 0xdeadbeefdeadbeefULL;
    hr = IMediaSeeking_GetDuration(seeking, &duration);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);
    ok(duration == 0xdeadbeefdeadbeefULL, "Got duration %s.\n", wine_dbgstr_longlong(duration));

    source2.get_duration_hr = E_NOTIMPL;
    duration = 0xdeadbeefdeadbeefULL;
    hr = IMediaSeeking_GetDuration(seeking, &duration);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    ok(duration == 0xdeadbeefdeadbeefULL, "Got duration %s.\n", wine_dbgstr_longlong(duration));

    source2.IMediaSeeking_iface.lpVtbl = NULL;
    duration = 0xdeadbeefdeadbeefULL;
    hr = IMediaSeeking_GetDuration(seeking, &duration);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    ok(duration == 0xdeadbeefdeadbeefULL, "Got duration %s.\n", wine_dbgstr_longlong(duration));

    IGraphBuilder_Disconnect(graph, pin2);
    IGraphBuilder_Disconnect(graph, &source2.source.pin.IPin_iface);

    source2.IMediaSeeking_iface.lpVtbl = NULL;
    duration = 0xdeadbeefdeadbeefULL;
    hr = IMediaSeeking_GetDuration(seeking, &duration);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    ok(duration == 0xdeadbeefdeadbeefULL, "Got duration %s.\n", wine_dbgstr_longlong(duration));

    IGraphBuilder_Disconnect(graph, pin2);
    IGraphBuilder_Disconnect(graph, &source2.source.pin.IPin_iface);
    IGraphBuilder_Disconnect(graph, pin3);
    IGraphBuilder_Disconnect(graph, &source3.source.pin.IPin_iface);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IMediaSeeking_Release(seeking);
    ref = IMediaStreamFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin1);
    ref = IAMMediaStream_Release(stream1);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin2);
    ref = IAMMediaStream_Release(stream2);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin3);
    ref = IAMMediaStream_Release(stream3);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_mediastreamfilter_get_stop_position(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    static const MSPID mspid1 = {0x88888888, 1};
    static const MSPID mspid2 = {0x88888888, 2};
    static const MSPID mspid3 = {0x88888888, 3};
    struct testfilter source1, source2, source3;
    IAMMediaStream *stream1, *stream2, *stream3;
    IMediaStreamFilter *filter;
    IPin *pin1, *pin2, *pin3;
    IMediaSeeking *seeking;
    IGraphBuilder *graph;
    LONGLONG stop;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = CoCreateInstance(&CLSID_AMAudioStream, NULL, CLSCTX_INPROC_SERVER, &IID_IAMMediaStream, (void **)&stream3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_Initialize(stream1, NULL, 0, &mspid1, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_Initialize(stream2, NULL, 0, &mspid2, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_Initialize(stream3, NULL, 0, &mspid3, STREAMTYPE_READ);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)stream1, &mspid1, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)stream2, &mspid2, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)stream3, &mspid3, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_QueryInterface(stream1, &IID_IPin, (void **)&pin1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_QueryInterface(stream2, &IID_IPin, (void **)&pin2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMediaStream_QueryInterface(stream3, &IID_IPin, (void **)&pin3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilter(mmstream, &filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(graph != NULL, "Expected non-NULL graph.\n");
    testfilter_init(&source1);
    testfilter_init(&source2);
    testfilter_init(&source3);
    source1.IMediaSeeking_iface.lpVtbl = &testsource_seeking_vtbl;
    source2.IMediaSeeking_iface.lpVtbl = &testsource_seeking_vtbl;
    source3.IMediaSeeking_iface.lpVtbl = &testsource_seeking_vtbl;
    hr = IGraphBuilder_AddFilter(graph, &source1.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_AddFilter(graph, &source2.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_AddFilter(graph, &source3.filter.IBaseFilter_iface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source2.source.pin.IPin_iface, pin2, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IGraphBuilder_ConnectDirect(graph, &source3.source.pin.IPin_iface, pin3, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStreamFilter_SupportSeeking(filter, TRUE);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IGraphBuilder_ConnectDirect(graph, &source1.source.pin.IPin_iface, pin1, &audio_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStreamFilter_QueryInterface(filter, &IID_IMediaSeeking, (void **)&seeking);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    stop = 0xdeadbeefdeadbeefULL;
    hr = IMediaSeeking_GetStopPosition(seeking, &stop);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(stop == 0x8000000000000000ULL, "Got stop position %s.\n", wine_dbgstr_longlong(stop));

    source2.get_stop_position_hr = E_FAIL;
    stop = 0xdeadbeefdeadbeefULL;
    hr = IMediaSeeking_GetStopPosition(seeking, &stop);
    ok(hr == E_FAIL, "Got hr %#x.\n", hr);
    ok(stop == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n", wine_dbgstr_longlong(stop));

    source2.get_stop_position_hr = E_NOTIMPL;
    stop = 0xdeadbeefdeadbeefULL;
    hr = IMediaSeeking_GetStopPosition(seeking, &stop);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    ok(stop == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n", wine_dbgstr_longlong(stop));

    source2.IMediaSeeking_iface.lpVtbl = NULL;
    stop = 0xdeadbeefdeadbeefULL;
    hr = IMediaSeeking_GetStopPosition(seeking, &stop);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    ok(stop == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n", wine_dbgstr_longlong(stop));

    IGraphBuilder_Disconnect(graph, pin2);
    IGraphBuilder_Disconnect(graph, &source2.source.pin.IPin_iface);

    source2.IMediaSeeking_iface.lpVtbl = NULL;
    stop = 0xdeadbeefdeadbeefULL;
    hr = IMediaSeeking_GetStopPosition(seeking, &stop);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);
    ok(stop == 0xdeadbeefdeadbeefULL, "Got stop position %s.\n", wine_dbgstr_longlong(stop));

    IGraphBuilder_Disconnect(graph, pin2);
    IGraphBuilder_Disconnect(graph, &source2.source.pin.IPin_iface);
    IGraphBuilder_Disconnect(graph, pin3);
    IGraphBuilder_Disconnect(graph, &source3.source.pin.IPin_iface);

    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IMediaSeeking_Release(seeking);
    ref = IMediaStreamFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin1);
    ref = IAMMediaStream_Release(stream1);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin2);
    ref = IAMMediaStream_Release(stream2);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin3);
    ref = IAMMediaStream_Release(stream3);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_mediastreamfilter_get_current_stream_time(void)
{
    IMediaStreamFilter *filter;
    struct testclock clock;
    REFERENCE_TIME time;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&CLSID_MediaStreamFilter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaStreamFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    testclock_init(&clock);

    /* Crashes on native. */
    if (0)
    {
        hr = IMediaStreamFilter_GetCurrentStreamTime(filter, NULL);
        ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    }

    time = 0xdeadbeefdeadbeef;
    hr = IMediaStreamFilter_GetCurrentStreamTime(filter, &time);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(time == 0, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaStreamFilter_SetSyncSource(filter, &clock.IReferenceClock_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    clock.get_time_hr = E_FAIL;

    time = 0xdeadbeefdeadbeef;
    hr = IMediaStreamFilter_GetCurrentStreamTime(filter, &time);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(time == 0, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaStreamFilter_Run(filter, 23456789);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    time = 0xdeadbeefdeadbeef;
    hr = IMediaStreamFilter_GetCurrentStreamTime(filter, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0xdeadbeefdd47d2da, "Got time %s.\n", wine_dbgstr_longlong(time));

    clock.time = 34567890;
    clock.get_time_hr = S_OK;

    time = 0xdeadbeefdeadbeef;
    hr = IMediaStreamFilter_GetCurrentStreamTime(filter, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 11111101, "Got time %s.\n", wine_dbgstr_longlong(time));

    ref = IMediaStreamFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_mediastreamfilter_reference_time_to_stream_time(void)
{
    IMediaStreamFilter *filter;
    struct testclock clock;
    REFERENCE_TIME time;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&CLSID_MediaStreamFilter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaStreamFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    testclock_init(&clock);

    hr = IMediaStreamFilter_ReferenceTimeToStreamTime(filter, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    time = 0xdeadbeefdeadbeef;
    hr = IMediaStreamFilter_ReferenceTimeToStreamTime(filter, &time);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(time == 0xdeadbeefdeadbeef, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaStreamFilter_SetSyncSource(filter, &clock.IReferenceClock_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    clock.get_time_hr = E_FAIL;

    /* Crashes on native. */
    if (0)
    {
        hr = IMediaStreamFilter_ReferenceTimeToStreamTime(filter, NULL);
        ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    }

    time = 0xdeadbeefdeadbeef;
    hr = IMediaStreamFilter_ReferenceTimeToStreamTime(filter, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0xdeadbeefdeadbeef, "Got time %s.\n", wine_dbgstr_longlong(time));

    hr = IMediaStreamFilter_Run(filter, 23456789);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    time = 0xdeadbeefdeadbeef;
    hr = IMediaStreamFilter_ReferenceTimeToStreamTime(filter, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0xdeadbeefdd47d2da, "Got time %s.\n", wine_dbgstr_longlong(time));

    clock.time = 34567890;
    clock.get_time_hr = S_OK;

    time = 0xdeadbeefdeadbeef;
    hr = IMediaStreamFilter_ReferenceTimeToStreamTime(filter, &time);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time == 0xdeadbeefdd47d2da, "Got time %s.\n", wine_dbgstr_longlong(time));

    ref = IMediaStreamFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_ddrawstream_getsetdirectdraw(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IDirectDraw *ddraw, *ddraw2, *ddraw3, *ddraw4;
    IDirectDrawMediaStream *ddraw_stream;
    IDirectDrawStreamSample *sample;
    IDirectDraw7 *ddraw7;
    IMediaStream *stream;
    HRESULT hr;
    ULONG ref;

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IDirectDraw_QueryInterface(ddraw, &IID_IDirectDraw7, (void **)&ddraw7);
    ok(hr == DD_OK, "Got hr %#x.\n", hr);
    hr = IDirectDraw7_SetCooperativeLevel(ddraw7, GetDesktopWindow(), DDSCL_NORMAL);
    ok(hr == DD_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(ddraw, 1);
    EXPECT_REF(ddraw7, 1);

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)ddraw7, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(ddraw, 2);

    hr = IMediaStream_QueryInterface(stream, &IID_IDirectDrawMediaStream, (void **)&ddraw_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_GetDirectDraw(ddraw_stream, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_GetDirectDraw(ddraw_stream, &ddraw2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(ddraw2 == ddraw, "Expected ddraw %p, got %p.\n", ddraw, ddraw2);
    EXPECT_REF(ddraw, 3);

    hr = IDirectDrawMediaStream_GetDirectDraw(ddraw_stream, &ddraw3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(ddraw3 == ddraw2, "Expected ddraw %p, got %p.\n", ddraw2, ddraw3);
    EXPECT_REF(ddraw, 4);
    IDirectDraw_Release(ddraw3);
    EXPECT_REF(ddraw, 3);

    /* The current ddraw is released when SetDirectDraw() is called. */
    hr = IDirectDrawMediaStream_SetDirectDraw(ddraw_stream, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(ddraw, 2);

    hr = IDirectDrawMediaStream_GetDirectDraw(ddraw_stream, &ddraw3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(ddraw3 == NULL, "Expected NULL, got %p.\n", ddraw3);

    hr = IDirectDrawMediaStream_SetDirectDraw(ddraw_stream, ddraw2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(ddraw, 3);

    hr = IDirectDrawMediaStream_GetDirectDraw(ddraw_stream, &ddraw3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(ddraw3 == ddraw2, "Expected ddraw %p, got %p.\n", ddraw2, ddraw3);
    EXPECT_REF(ddraw, 4);
    IDirectDraw_Release(ddraw3);
    EXPECT_REF(ddraw, 3);

    hr = IDirectDrawMediaStream_CreateSample(ddraw_stream, NULL, NULL, 0, &sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

     /* SetDirectDraw() doesn't take an extra reference to the ddraw object
      * if there are samples extant. */
    hr = IDirectDrawMediaStream_SetDirectDraw(ddraw_stream, ddraw2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(ddraw, 3);

    hr = DirectDrawCreate(NULL, &ddraw3, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IDirectDraw_SetCooperativeLevel(ddraw3, GetDesktopWindow(), DDSCL_NORMAL);
    ok(hr == DD_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(ddraw3, 1);

    hr = IDirectDrawMediaStream_SetDirectDraw(ddraw_stream, ddraw3);
    ok(hr == MS_E_SAMPLEALLOC, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_GetDirectDraw(ddraw_stream, &ddraw4);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(ddraw4 == ddraw2, "Expected ddraw %p, got %p.\n", ddraw2, ddraw4);
    EXPECT_REF(ddraw, 4);
    IDirectDraw_Release(ddraw4);
    EXPECT_REF(ddraw, 3);

    ref = IDirectDrawStreamSample_Release(sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = IDirectDrawMediaStream_SetDirectDraw(ddraw_stream, ddraw3);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(ddraw, 2);
    EXPECT_REF(ddraw3, 2);

    hr = IDirectDrawMediaStream_GetDirectDraw(ddraw_stream, &ddraw4);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(ddraw4 == ddraw3, "Expected ddraw %p, got %p.\n", ddraw3, ddraw4);
    EXPECT_REF(ddraw3, 3);
    IDirectDraw_Release(ddraw4);
    EXPECT_REF(ddraw3, 2);

    hr = IDirectDrawMediaStream_SetDirectDraw(ddraw_stream, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(ddraw3, 1);

    ref = IDirectDraw_Release(ddraw3);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    EXPECT_REF(stream, 3);
    IDirectDrawMediaStream_Release(ddraw_stream);
    EXPECT_REF(stream, 2);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    EXPECT_REF(stream, 1);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IDirectDraw7_Release(ddraw7);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    IDirectDraw_Release(ddraw2);
    EXPECT_REF(ddraw, 1);
    ref = IDirectDraw_Release(ddraw);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_audiostreamsample_get_media_stream(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IAudioStreamSample *audio_sample;
    IAudioMediaStream *audio_stream;
    IMediaStream *stream, *stream2;
    IAudioData *audio_data;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStream_QueryInterface(stream, &IID_IAudioMediaStream, (void **)&audio_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = CoCreateInstance(&CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER, &IID_IAudioData, (void **)&audio_data);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioMediaStream_CreateSample(audio_stream, audio_data, 0, &audio_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Crashes on native. */
    if (0)
    {
        hr = IAudioStreamSample_GetMediaStream(audio_sample, NULL);
        ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    }

    EXPECT_REF(stream, 4);
    hr = IAudioStreamSample_GetMediaStream(audio_sample, &stream2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(stream2 == stream, "Expected stream %p, got %p.\n", stream, stream2);
    EXPECT_REF(stream, 5);

    IMediaStream_Release(stream2);

    IAudioMediaStream_Release(audio_stream);
    ref = IAudioStreamSample_Release(audio_sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAudioData_Release(audio_data);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_audiostreamsample_get_audio_data(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IAudioData *audio_data, *audio_data2;
    IAudioStreamSample *audio_sample;
    IAudioMediaStream *audio_stream;
    IMediaStream *stream;
    HRESULT hr;
    ULONG ref;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryAudio, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStream_QueryInterface(stream, &IID_IAudioMediaStream, (void **)&audio_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = CoCreateInstance(&CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER, &IID_IAudioData, (void **)&audio_data);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioMediaStream_CreateSample(audio_stream, audio_data, 0, &audio_sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAudioStreamSample_GetAudioData(audio_sample, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    EXPECT_REF(audio_data, 2);
    hr = IAudioStreamSample_GetAudioData(audio_sample, &audio_data2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(audio_data2 == audio_data, "Expected audio data %p, got %p.\n", audio_data, audio_data2);
    EXPECT_REF(audio_data, 3);

    IAudioData_Release(audio_data2);

    IAudioMediaStream_Release(audio_stream);
    ref = IAudioStreamSample_Release(audio_sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAudioData_Release(audio_data);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

#define get_ddrawstream_create_sample_desc(a,b,c,d) get_ddrawstream_create_sample_desc_(__LINE__,a,b,c,d)
static void get_ddrawstream_create_sample_desc_(int line, const DDSURFACEDESC *format1,
        const DDSURFACEDESC *format2, const AM_MEDIA_TYPE *mt, DDSURFACEDESC *desc)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IDirectDrawMediaStream *ddraw_stream;
    IDirectDrawStreamSample *sample;
    IDirectDrawSurface *surface;
    struct testfilter source;
    IGraphBuilder *graph;
    IMediaStream *stream;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_AddMediaStream(mmstream, NULL, &MSPID_PrimaryVideo, 0, &stream);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IDirectDrawMediaStream, (void **)&ddraw_stream);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IAMMultiMediaStream_GetFilterGraph(mmstream, &graph);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

    testfilter_init(&source);

    hr = IGraphBuilder_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

    if (format1)
    {
        hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, format1, NULL);
        ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    }
    if (format2)
    {
        hr = IDirectDrawMediaStream_SetFormat(ddraw_stream, format2, NULL);
        ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    }
    if (mt)
    {
        hr = IGraphBuilder_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, mt);
        ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IGraphBuilder_Disconnect(graph, &source.source.pin.IPin_iface);
        ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
        hr = IGraphBuilder_Disconnect(graph, pin);
        ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    }

    hr = IDirectDrawMediaStream_CreateSample(ddraw_stream, NULL, NULL, 0, &sample);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawStreamSample_GetSurface(sample, &surface, NULL);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);
    ok_(__FILE__, line)(!!surface, "Expected non-NULL sufrace.\n");

    desc->dwSize = sizeof(*desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, desc);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#x.\n", hr);

    ref = IDirectDrawStreamSample_Release(sample);
    ok_(__FILE__, line)(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IDirectDrawSurface_Release(surface);
    ok_(__FILE__, line)(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok_(__FILE__, line)(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IGraphBuilder_Release(graph);
    ok_(__FILE__, line)(!ref, "Got outstanding refcount %d.\n", ref);
    IPin_Release(pin);
    IDirectDrawMediaStream_Release(ddraw_stream);
    ref = IMediaStream_Release(stream);
    ok_(__FILE__, line)(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_ddrawstream_create_sample(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    DDSURFACEDESC desc2 = { sizeof(desc2) };
    IDirectDrawSurface *surface, *surface2;
    DDSURFACEDESC desc = { sizeof(desc) };
    IDirectDrawMediaStream *ddraw_stream;
    IDirectDrawStreamSample *sample;
    DDSURFACEDESC format1;
    DDSURFACEDESC format2;
    IMediaStream *stream;
    IDirectDraw *ddraw;
    HRESULT hr;
    RECT rect;
    ULONG ref;

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)ddraw, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStream_QueryInterface(stream, &IID_IDirectDrawMediaStream, (void **)&ddraw_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Crashes on native. */
    if (0)
    {
        hr = IDirectDrawMediaStream_CreateSample(ddraw_stream, NULL, NULL, 0, NULL);
        ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    }

    SetRectEmpty(&rect);
    hr = IDirectDrawMediaStream_CreateSample(ddraw_stream, NULL, &rect, 0, &sample);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    EXPECT_REF(stream, 3);
    hr = IDirectDrawMediaStream_CreateSample(ddraw_stream, NULL, NULL, 0, &sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(stream, 4);

    hr = IDirectDrawStreamSample_GetSurface(sample, NULL, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawStreamSample_GetSurface(sample, NULL, &rect);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawStreamSample_GetSurface(sample, &surface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(surface != NULL, "Expected non-NULL surface.\n");
    IDirectDrawSurface_Release(surface);

    surface = NULL;
    hr = IDirectDrawStreamSample_GetSurface(sample, &surface, &rect);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(surface != NULL, "Expected non-NULL surface.\n");

    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &desc);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(desc.dwWidth == 100, "Expected width 100, got %d.\n", desc.dwWidth);
    ok(desc.dwHeight == 100, "Expected height 100, got %d.\n", desc.dwHeight);
    ok(desc.ddpfPixelFormat.dwFlags == DDPF_RGB, "Expected format flags DDPF_RGB, got %#x.\n", desc.ddpfPixelFormat.dwFlags);
    ok(desc.ddpfPixelFormat.dwRGBBitCount, "Expected non-zero RGB bit count.\n");
    IDirectDrawSurface_Release(surface);
    IDirectDrawStreamSample_Release(sample);
    EXPECT_REF(stream, 3);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    EXPECT_REF(surface, 1);
    hr = IDirectDrawMediaStream_CreateSample(ddraw_stream, surface, NULL, 0, &sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    EXPECT_REF(surface, 2);

    surface2 = NULL;
    SetRectEmpty(&rect);
    hr = IDirectDrawStreamSample_GetSurface(sample, &surface2, &rect);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(surface2 == surface, "Expected surface %p, got %p.\n", surface, surface2);
    ok(rect.right > 0 && rect.bottom > 0, "Got rect %d, %d.\n", rect.right, rect.bottom);
    EXPECT_REF(surface, 3);
    IDirectDrawSurface_Release(surface2);
    EXPECT_REF(surface, 2);
    IDirectDrawStreamSample_Release(sample);
    EXPECT_REF(surface, 1);

    hr = IDirectDrawMediaStream_CreateSample(ddraw_stream, surface, &rect, 0, &sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    ref = IDirectDrawStreamSample_Release(sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IDirectDrawSurface_Release(surface);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    desc.dwWidth = 444;
    desc.dwHeight = 400;
    desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    desc.ddpfPixelFormat.dwRGBBitCount = 32;
    desc.ddpfPixelFormat.dwRBitMask = 0xff0000;
    desc.ddpfPixelFormat.dwGBitMask = 0x00ff00;
    desc.ddpfPixelFormat.dwBBitMask = 0x0000ff;
    desc.ddpfPixelFormat.dwRGBAlphaBitMask = 0xff000000;
    desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    SetRect(&rect, 111, 100, 333, 300);

    hr = IDirectDrawMediaStream_CreateSample(ddraw_stream, surface, &rect, 0, &sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = IDirectDrawStreamSample_Release(sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = IDirectDrawMediaStream_CreateSample(ddraw_stream, NULL, NULL, 0, &sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    surface2 = NULL;
    hr = IDirectDrawStreamSample_GetSurface(sample, &surface2, &rect);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &desc);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface2, &desc2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(desc2.dwWidth == 222, "Got width %u.\n", desc2.dwWidth);
    ok(desc2.dwHeight == 200, "Got height %u.\n", desc2.dwHeight);
    ok(memcmp(&desc2.ddpfPixelFormat, &desc.ddpfPixelFormat, sizeof(DDPIXELFORMAT)) == 0,
            "Pixel format didn't match.\n");

    ref = IDirectDrawStreamSample_Release(sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IDirectDrawSurface_Release(surface);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IDirectDrawSurface_Release(surface2);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    desc.dwWidth = 444;
    desc.dwHeight = 400;
    desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED4;
    desc.ddpfPixelFormat.dwRGBBitCount = 4;
    desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_CreateSample(ddraw_stream, surface, NULL, 0, &sample);
    ok(hr == DDERR_INVALIDSURFACETYPE, "Got hr %#x.\n", hr);

    IDirectDrawMediaStream_Release(ddraw_stream);
    ref = IDirectDrawSurface_Release(surface);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IDirectDraw_Release(ddraw);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    format1 = rgb8_format;
    format1.dwFlags = 0;
    format1.dwWidth = 333;
    format1.dwHeight = 444;
    get_ddrawstream_create_sample_desc(&format1, NULL, NULL, &desc);
    ok(desc.dwWidth == 100, "Got width %u.\n", desc.dwWidth);
    ok(desc.dwHeight == 100, "Got height %u.\n", desc.dwHeight);
    ok(desc.ddpfPixelFormat.dwFlags == DDPF_RGB, "Got flags %#x.\n", desc.ddpfPixelFormat.dwFlags);
    ok(desc.ddpfPixelFormat.dwRGBBitCount == 32, "Got rgb bit count %u.\n", desc.ddpfPixelFormat.dwRGBBitCount);
    ok(desc.ddpfPixelFormat.dwRBitMask == 0xff0000, "Got r bit mask %#x.\n", desc.ddpfPixelFormat.dwRBitMask);
    ok(desc.ddpfPixelFormat.dwGBitMask == 0x00ff00, "Got g bit mask %#x.\n", desc.ddpfPixelFormat.dwGBitMask);
    ok(desc.ddpfPixelFormat.dwBBitMask == 0x0000ff, "Got b bit mask %#x.\n", desc.ddpfPixelFormat.dwBBitMask);

    format1 = rgb8_format;
    format1.dwFlags |= DDSD_WIDTH;
    format1.dwWidth = 333;
    format1.dwHeight = 444;
    format2 = rgb8_format;
    format2.dwFlags = 0;
    get_ddrawstream_create_sample_desc(&format1, &format2, NULL, &desc);
    ok(desc.dwWidth == 333, "Got width %u.\n", desc.dwWidth);
    ok(desc.dwHeight == 444, "Got height %u.\n", desc.dwHeight);

    format1 = rgb8_format;
    format1.dwFlags |= DDSD_HEIGHT;
    format1.dwWidth = 333;
    format1.dwHeight = 444;
    format2 = rgb8_format;
    format2.dwFlags = 0;
    get_ddrawstream_create_sample_desc(&format1, &format2, NULL, &desc);
    ok(desc.dwWidth == 333, "Got width %u.\n", desc.dwWidth);
    ok(desc.dwHeight == 444, "Got height %u.\n", desc.dwHeight);

    get_ddrawstream_create_sample_desc(NULL, NULL, &rgb8_mt, &desc);
    ok(desc.dwWidth == 333, "Got width %u.\n", desc.dwWidth);
    ok(desc.dwHeight == 444, "Got height %u.\n", desc.dwHeight);
    ok(desc.ddpfPixelFormat.dwFlags == DDPF_RGB, "Got flags %#x.\n", desc.ddpfPixelFormat.dwFlags);
    ok(desc.ddpfPixelFormat.dwRGBBitCount == 32, "Got rgb bit count %u.\n", desc.ddpfPixelFormat.dwRGBBitCount);
    ok(desc.ddpfPixelFormat.dwRBitMask == 0xff0000, "Got r bit mask %#x.\n", desc.ddpfPixelFormat.dwRBitMask);
    ok(desc.ddpfPixelFormat.dwGBitMask == 0x00ff00, "Got g bit mask %#x.\n", desc.ddpfPixelFormat.dwGBitMask);
    ok(desc.ddpfPixelFormat.dwBBitMask == 0x0000ff, "Got b bit mask %#x.\n", desc.ddpfPixelFormat.dwBBitMask);

    get_ddrawstream_create_sample_desc(&rgb565_format, NULL, NULL, &desc);
    ok(desc.dwWidth == 100, "Got width %u.\n", desc.dwWidth);
    ok(desc.dwHeight == 100, "Got height %u.\n", desc.dwHeight);
    ok(desc.ddpfPixelFormat.dwFlags == DDPF_RGB, "Got flags %#x.\n", desc.ddpfPixelFormat.dwFlags);
    ok(desc.ddpfPixelFormat.dwRGBBitCount == 16, "Got rgb bit count %u.\n", desc.ddpfPixelFormat.dwRGBBitCount);
    ok(desc.ddpfPixelFormat.dwRBitMask == 0xf800, "Got r bit mask %#x.\n", desc.ddpfPixelFormat.dwRBitMask);
    ok(desc.ddpfPixelFormat.dwGBitMask == 0x07e0, "Got g bit mask %#x.\n", desc.ddpfPixelFormat.dwGBitMask);
    ok(desc.ddpfPixelFormat.dwBBitMask == 0x001f, "Got b bit mask %#x.\n", desc.ddpfPixelFormat.dwBBitMask);

    get_ddrawstream_create_sample_desc(&argb32_format, NULL, NULL, &desc);
    ok(desc.dwWidth == 100, "Got width %u.\n", desc.dwWidth);
    ok(desc.dwHeight == 100, "Got height %u.\n", desc.dwHeight);
    ok(desc.ddpfPixelFormat.dwFlags == (DDPF_RGB | DDPF_ALPHAPIXELS), "Got flags %#x.\n", desc.ddpfPixelFormat.dwFlags);
    ok(desc.ddpfPixelFormat.dwRGBBitCount == 32, "Got rgb bit count %u.\n", desc.ddpfPixelFormat.dwRGBBitCount);
    ok(desc.ddpfPixelFormat.dwRBitMask == 0xff0000, "Got r bit mask %#x.\n", desc.ddpfPixelFormat.dwRBitMask);
    ok(desc.ddpfPixelFormat.dwGBitMask == 0x00ff00, "Got g bit mask %#x.\n", desc.ddpfPixelFormat.dwGBitMask);
    ok(desc.ddpfPixelFormat.dwBBitMask == 0x0000ff, "Got b bit mask %#x.\n", desc.ddpfPixelFormat.dwBBitMask);
    ok(desc.ddpfPixelFormat.dwRGBAlphaBitMask == 0xff000000,
            "Got alpha bit mask %#x.\n", desc.ddpfPixelFormat.dwBBitMask);

    format1 = rgb32_format;
    format1.dwFlags |= DDSD_CAPS;
    format1.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
    get_ddrawstream_create_sample_desc(&format1, NULL, NULL, &desc);
    ok(desc.ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN, "Expected set DDSCAPS_OFFSCREENPLAIN.\n");
    ok(desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY, "Expected set DDSCAPS_SYSTEMMEMORY.\n");
    ok(!(desc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY), "Expected unset DDSCAPS_VIDEOMEMORY.\n");
    ok(desc.dwWidth == 100, "Got width %u.\n", desc.dwWidth);
    ok(desc.dwHeight == 100, "Got height %u.\n", desc.dwHeight);
    ok(desc.ddpfPixelFormat.dwFlags == DDPF_RGB, "Got flags %#x.\n", desc.ddpfPixelFormat.dwFlags);
    ok(desc.ddpfPixelFormat.dwRGBBitCount == 32, "Got rgb bit count %u.\n", desc.ddpfPixelFormat.dwRGBBitCount);
    ok(desc.ddpfPixelFormat.dwRBitMask == 0xff0000, "Got r bit mask %#x.\n", desc.ddpfPixelFormat.dwRBitMask);
    ok(desc.ddpfPixelFormat.dwGBitMask == 0x00ff00, "Got g bit mask %#x.\n", desc.ddpfPixelFormat.dwGBitMask);
    ok(desc.ddpfPixelFormat.dwBBitMask == 0x0000ff, "Got b bit mask %#x.\n", desc.ddpfPixelFormat.dwBBitMask);

    format1 = rgb32_format;
    format1.dwFlags |= DDSD_CKSRCBLT;
    format1.ddckCKSrcBlt.dwColorSpaceLowValue = 0xff00ff;
    format1.ddckCKSrcBlt.dwColorSpaceHighValue = 0xff00ff;
    get_ddrawstream_create_sample_desc(&format1, NULL, NULL, &desc);
    ok(!(desc.dwFlags & DDSD_CKSRCBLT), "Expected unset DDSD_CKSRCBLT.\n");
    ok(desc.dwWidth == 100, "Got width %u.\n", desc.dwWidth);
    ok(desc.dwHeight == 100, "Got height %u.\n", desc.dwHeight);
    ok(desc.ddpfPixelFormat.dwFlags == DDPF_RGB, "Got flags %#x.\n", desc.ddpfPixelFormat.dwFlags);
    ok(desc.ddpfPixelFormat.dwRGBBitCount == 32, "Got rgb bit count %u.\n", desc.ddpfPixelFormat.dwRGBBitCount);
    ok(desc.ddpfPixelFormat.dwRBitMask == 0xff0000, "Got r bit mask %#x.\n", desc.ddpfPixelFormat.dwRBitMask);
    ok(desc.ddpfPixelFormat.dwGBitMask == 0x00ff00, "Got g bit mask %#x.\n", desc.ddpfPixelFormat.dwGBitMask);
    ok(desc.ddpfPixelFormat.dwBBitMask == 0x0000ff, "Got b bit mask %#x.\n", desc.ddpfPixelFormat.dwBBitMask);
    ok(desc.ddckCKSrcBlt.dwColorSpaceLowValue == 0, "Got color key low value %#x.\n",
            desc.ddckCKSrcBlt.dwColorSpaceLowValue);
    ok(desc.ddckCKSrcBlt.dwColorSpaceHighValue == 0, "Got color key high value %#x.\n",
            desc.ddckCKSrcBlt.dwColorSpaceHighValue);
}

static void test_ddrawstreamsample_get_media_stream(void)
{
    IAMMultiMediaStream *mmstream = create_ammultimediastream();
    IDirectDrawMediaStream *ddraw_stream;
    IDirectDrawStreamSample *sample;
    IMediaStream *stream, *stream2;
    IDirectDraw *ddraw;
    HRESULT hr;
    ULONG ref;

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_Initialize(mmstream, STREAMTYPE_READ, 0, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IAMMultiMediaStream_AddMediaStream(mmstream, (IUnknown *)ddraw, &MSPID_PrimaryVideo, 0, &stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaStream_QueryInterface(stream, &IID_IDirectDrawMediaStream, (void **)&ddraw_stream);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IDirectDrawMediaStream_CreateSample(ddraw_stream, NULL, NULL, 0, &sample);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Crashes on native. */
    if (0)
    {
        hr = IDirectDrawStreamSample_GetMediaStream(sample, NULL);
        ok(hr == E_POINTER, "Got hr %#x.\n", hr);
    }

    EXPECT_REF(stream, 4);
    hr = IDirectDrawStreamSample_GetMediaStream(sample, &stream2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(stream2 == stream, "Expected stream %p, got %p.\n", stream, stream2);
    EXPECT_REF(stream, 5);
    IMediaStream_Release(stream2);
    EXPECT_REF(stream, 4);

    IDirectDrawMediaStream_Release(ddraw_stream);
    ref = IDirectDrawStreamSample_Release(sample);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IAMMultiMediaStream_Release(mmstream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IMediaStream_Release(stream);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IDirectDraw_Release(ddraw);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

START_TEST(amstream)
{
    const WCHAR *test_avi_path;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    test_interfaces();
    test_add_stream();
    test_media_streams();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_initialize();
    test_set_state();
    test_enum_media_types();
    test_media_types();

    test_avi_path = load_resource(L"test.avi");

    test_openfile(test_avi_path);
    test_mmstream_get_duration(test_avi_path);

    unload_resource(test_avi_path);

    test_audiodata_query_interface();
    test_audiodata_get_info();
    test_audiodata_set_buffer();
    test_audiodata_set_actual();
    test_audiodata_get_format();
    test_audiodata_set_format();

    test_audiostream_get_format();
    test_audiostream_set_format();
    test_audiostream_receive_connection();
    test_audiostream_receive();
    test_audiostream_initialize();
    test_audiostream_begin_flush_end_flush();
    test_audiostream_new_segment();

    test_audiostreamsample_update();
    test_audiostreamsample_completion_status();
    test_audiostreamsample_get_sample_times();
    test_audiostreamsample_get_media_stream();
    test_audiostreamsample_get_audio_data();

    test_ddrawstream_initialize();
    test_ddrawstream_getsetdirectdraw();
    test_ddrawstream_receive_connection();
    test_ddrawstream_create_sample();
    test_ddrawstream_get_format();
    test_ddrawstream_set_format();

    test_ddrawstreamsample_get_media_stream();

    test_ammediastream_join_am_multi_media_stream();
    test_ammediastream_join_filter();
    test_ammediastream_join_filter_graph();
    test_ammediastream_set_state();
    test_ammediastream_end_of_stream();

    test_mediastreamfilter_get_state();
    test_mediastreamfilter_stop_pause_run();
    test_mediastreamfilter_support_seeking();
    test_mediastreamfilter_set_positions();
    test_mediastreamfilter_get_duration();
    test_mediastreamfilter_get_stop_position();
    test_mediastreamfilter_get_current_stream_time();
    test_mediastreamfilter_reference_time_to_stream_time();

    CoUninitialize();
}
