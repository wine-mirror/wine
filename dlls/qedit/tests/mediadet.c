/*
 * Unit tests for Media Detector
 *
 * Copyright (C) 2008 Google (Lei Zhang, Dan Hipschman)
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

#include "initguid.h"
#include "ole2.h"
#include "vfwmsgs.h"
#include "uuids.h"
#include "wine/test.h"
#include "qedit.h"
#include "rc.h"

/* Outer IUnknown for COM aggregation tests */
struct unk_impl {
    IUnknown IUnknown_iface;
    LONG ref;
    IUnknown *inner_unk;
};

static inline struct unk_impl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct unk_impl, IUnknown_iface);
}

static HRESULT WINAPI unk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    struct unk_impl *This = impl_from_IUnknown(iface);

    return IUnknown_QueryInterface(This->inner_unk, riid, ppv);
}

static ULONG WINAPI unk_AddRef(IUnknown *iface)
{
    struct unk_impl *This = impl_from_IUnknown(iface);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI unk_Release(IUnknown *iface)
{
    struct unk_impl *This = impl_from_IUnknown(iface);

    return InterlockedDecrement(&This->ref);
}

static const IUnknownVtbl unk_vtbl =
{
    unk_QueryInterface,
    unk_AddRef,
    unk_Release
};


static WCHAR test_avi_filename[MAX_PATH];
static WCHAR test_sound_avi_filename[MAX_PATH];

static BOOL unpack_avi_file(int id, WCHAR name[MAX_PATH])
{
    static WCHAR temp_path[MAX_PATH];
    static WCHAR prefix[] = {'D','E','S',0};
    static WCHAR avi[] = {'a','v','i',0};
    HRSRC res;
    HGLOBAL data;
    char *mem;
    DWORD size, written;
    HANDLE fh;

    res = FindResource(NULL, MAKEINTRESOURCE(id), MAKEINTRESOURCE(AVI_RES_TYPE));
    if (!res)
        return FALSE;

    data = LoadResource(NULL, res);
    if (!data)
        return FALSE;

    mem = LockResource(data);
    if (!mem)
        return FALSE;

    size = SizeofResource(NULL, res);
    if (size == 0)
        return FALSE;

    if (!GetTempPathW(MAX_PATH, temp_path))
        return FALSE;

    /* We might end up relying on the extension here, so .TMP is no good.  */
    if (!GetTempFileNameW(temp_path, prefix, 0, name))
        return FALSE;

    DeleteFileW(name);
    lstrcpyW(name + lstrlenW(name) - 3, avi);

    fh = CreateFileW(name, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                     FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE)
        return FALSE;

    if (!WriteFile(fh, mem, size, &written, NULL) || written != size)
        return FALSE;

    CloseHandle(fh);

    return TRUE;
}

static BOOL init_tests(void)
{
    return unpack_avi_file(TEST_AVI_RES, test_avi_filename)
        && unpack_avi_file(TEST_SOUND_AVI_RES, test_sound_avi_filename);
}

static void test_mediadet(void)
{
    HRESULT hr;
    struct unk_impl unk_obj = {{&unk_vtbl}, 19, NULL};
    IMediaDet *pM = NULL;
    ULONG refcount;
    BSTR filename = NULL;
    LONG nstrms = 0;
    LONG strm;
    AM_MEDIA_TYPE mt;
    double fps;
    int flags;
    int i;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_MediaDet, &unk_obj.IUnknown_iface, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&unk_obj.inner_unk);
    ok(hr == S_OK, "CoCreateInstance failed: %08x\n", hr);

    hr = IUnknown_QueryInterface(unk_obj.inner_unk, &IID_IMediaDet, (void**)&pM);
    ok(hr == S_OK, "QueryInterface for IID_IMediaDet failed: %08x\n", hr);
    refcount = IMediaDet_AddRef(pM);
    ok(refcount == unk_obj.ref, "MediaDet just pretends to support COM aggregation\n");
    refcount = IMediaDet_Release(pM);
    ok(refcount == unk_obj.ref, "MediaDet just pretends to support COM aggregation\n");
    refcount = IMediaDet_Release(pM);
    ok(refcount == 19, "Refcount should be back at 19 but is %u\n", refcount);

    IUnknown_Release(unk_obj.inner_unk);

    /* test.avi has one video stream.  */
    hr = CoCreateInstance(&CLSID_MediaDet, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaDet, (LPVOID*)&pM);
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    ok(pM != NULL, "pM is NULL\n");

    filename = NULL;
    hr = IMediaDet_get_Filename(pM, &filename);
    /* Despite what MSDN claims, this returns S_OK.  */
    ok(hr == S_OK, "IMediaDet_get_Filename\n");
    ok(filename == NULL, "IMediaDet_get_Filename\n");

    filename = (BSTR) -1;
    hr = IMediaDet_get_Filename(pM, &filename);
    /* Despite what MSDN claims, this returns S_OK.  */
    ok(hr == S_OK, "IMediaDet_get_Filename\n");
    ok(filename == NULL, "IMediaDet_get_Filename\n");

    nstrms = -1;
    hr = IMediaDet_get_OutputStreams(pM, &nstrms);
    ok(hr == E_INVALIDARG, "IMediaDet_get_OutputStreams\n");
    ok(nstrms == -1, "IMediaDet_get_OutputStreams\n");

    strm = -1;
    /* The stream defaults to 0, even without a file!  */
    hr = IMediaDet_get_CurrentStream(pM, &strm);
    ok(hr == S_OK, "IMediaDet_get_CurrentStream\n");
    ok(strm == 0, "IMediaDet_get_CurrentStream\n");

    hr = IMediaDet_get_CurrentStream(pM, NULL);
    ok(hr == E_POINTER, "IMediaDet_get_CurrentStream\n");

    /* But put_CurrentStream doesn't.  */
    hr = IMediaDet_put_CurrentStream(pM, 0);
    ok(hr == E_INVALIDARG, "IMediaDet_put_CurrentStream\n");

    hr = IMediaDet_put_CurrentStream(pM, -1);
    ok(hr == E_INVALIDARG, "IMediaDet_put_CurrentStream\n");

    hr = IMediaDet_get_StreamMediaType(pM, &mt);
    ok(hr == E_INVALIDARG, "IMediaDet_get_StreamMediaType\n");

    hr = IMediaDet_get_StreamMediaType(pM, NULL);
    ok(hr == E_POINTER, "IMediaDet_get_StreamMediaType\n");

    filename = SysAllocString(test_avi_filename);
    hr = IMediaDet_put_Filename(pM, filename);
    ok(hr == S_OK, "IMediaDet_put_Filename -> %x\n", hr);
    SysFreeString(filename);

    strm = -1;
    /* The stream defaults to 0.  */
    hr = IMediaDet_get_CurrentStream(pM, &strm);
    ok(hr == S_OK, "IMediaDet_get_CurrentStream\n");
    ok(strm == 0, "IMediaDet_get_CurrentStream\n");

    ZeroMemory(&mt, sizeof mt);
    hr = IMediaDet_get_StreamMediaType(pM, &mt);
    ok(hr == S_OK, "IMediaDet_get_StreamMediaType\n");
    CoTaskMemFree(mt.pbFormat);

    /* Even before get_OutputStreams.  */
    hr = IMediaDet_put_CurrentStream(pM, 1);
    ok(hr == E_INVALIDARG, "IMediaDet_put_CurrentStream\n");

    hr = IMediaDet_get_OutputStreams(pM, &nstrms);
    ok(hr == S_OK, "IMediaDet_get_OutputStreams\n");
    ok(nstrms == 1, "IMediaDet_get_OutputStreams\n");

    filename = NULL;
    hr = IMediaDet_get_Filename(pM, &filename);
    ok(hr == S_OK, "IMediaDet_get_Filename\n");
    ok(lstrcmpW(filename, test_avi_filename) == 0,
       "IMediaDet_get_Filename\n");
    SysFreeString(filename);

    hr = IMediaDet_get_Filename(pM, NULL);
    ok(hr == E_POINTER, "IMediaDet_get_Filename\n");

    strm = -1;
    hr = IMediaDet_get_CurrentStream(pM, &strm);
    ok(hr == S_OK, "IMediaDet_get_CurrentStream\n");
    ok(strm == 0, "IMediaDet_get_CurrentStream\n");

    hr = IMediaDet_get_CurrentStream(pM, NULL);
    ok(hr == E_POINTER, "IMediaDet_get_CurrentStream\n");

    hr = IMediaDet_put_CurrentStream(pM, -1);
    ok(hr == E_INVALIDARG, "IMediaDet_put_CurrentStream\n");

    hr = IMediaDet_put_CurrentStream(pM, 1);
    ok(hr == E_INVALIDARG, "IMediaDet_put_CurrentStream\n");

    /* Try again.  */
    strm = -1;
    hr = IMediaDet_get_CurrentStream(pM, &strm);
    ok(hr == S_OK, "IMediaDet_get_CurrentStream\n");
    ok(strm == 0, "IMediaDet_get_CurrentStream\n");

    hr = IMediaDet_put_CurrentStream(pM, 0);
    ok(hr == S_OK, "IMediaDet_put_CurrentStream\n");

    strm = -1;
    hr = IMediaDet_get_CurrentStream(pM, &strm);
    ok(hr == S_OK, "IMediaDet_get_CurrentStream\n");
    ok(strm == 0, "IMediaDet_get_CurrentStream\n");

    ZeroMemory(&mt, sizeof mt);
    hr = IMediaDet_get_StreamMediaType(pM, &mt);
    ok(hr == S_OK, "IMediaDet_get_StreamMediaType\n");
    ok(IsEqualGUID(&mt.majortype, &MEDIATYPE_Video),
                 "IMediaDet_get_StreamMediaType\n");
    CoTaskMemFree(mt.pbFormat);

    hr = IMediaDet_get_FrameRate(pM, NULL);
    ok(hr == E_POINTER, "IMediaDet_get_FrameRate\n");

    hr = IMediaDet_get_FrameRate(pM, &fps);
    ok(hr == S_OK, "IMediaDet_get_FrameRate\n");
    ok(fps == 10.0, "IMediaDet_get_FrameRate\n");

    hr = IMediaDet_Release(pM);
    ok(hr == 0, "IMediaDet_Release returned: %x\n", hr);

    DeleteFileW(test_avi_filename);

    /* test_sound.avi has one video stream and one audio stream.  */
    hr = CoCreateInstance(&CLSID_MediaDet, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaDet, (LPVOID*)&pM);
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    ok(pM != NULL, "pM is NULL\n");

    filename = SysAllocString(test_sound_avi_filename);
    hr = IMediaDet_put_Filename(pM, filename);
    ok(hr == S_OK, "IMediaDet_put_Filename -> %x\n", hr);
    SysFreeString(filename);

    hr = IMediaDet_get_OutputStreams(pM, &nstrms);
    ok(hr == S_OK, "IMediaDet_get_OutputStreams\n");
    ok(nstrms == 2, "IMediaDet_get_OutputStreams\n");

    filename = NULL;
    hr = IMediaDet_get_Filename(pM, &filename);
    ok(hr == S_OK, "IMediaDet_get_Filename\n");
    ok(lstrcmpW(filename, test_sound_avi_filename) == 0,
       "IMediaDet_get_Filename\n");
    SysFreeString(filename);

    /* I don't know if the stream order is deterministic.  Just check
       for both an audio and video stream.  */
    flags = 0;

    for (i = 0; i < 2; ++i)
    {
        hr = IMediaDet_put_CurrentStream(pM, i);
        ok(hr == S_OK, "IMediaDet_put_CurrentStream\n");

        strm = -1;
        hr = IMediaDet_get_CurrentStream(pM, &strm);
        ok(hr == S_OK, "IMediaDet_get_CurrentStream\n");
        ok(strm == i, "IMediaDet_get_CurrentStream\n");

        ZeroMemory(&mt, sizeof mt);
        hr = IMediaDet_get_StreamMediaType(pM, &mt);
        ok(hr == S_OK, "IMediaDet_get_StreamMediaType\n");
        flags += (IsEqualGUID(&mt.majortype, &MEDIATYPE_Video)
                  ? 1
                  : (IsEqualGUID(&mt.majortype, &MEDIATYPE_Audio)
                     ? 2
                     : 0));

        if (IsEqualGUID(&mt.majortype, &MEDIATYPE_Audio))
        {
            hr = IMediaDet_get_FrameRate(pM, &fps);
            ok(hr == VFW_E_INVALIDMEDIATYPE, "IMediaDet_get_FrameRate\n");
        }

        CoTaskMemFree(mt.pbFormat);
    }
    ok(flags == 3, "IMediaDet_get_StreamMediaType\n");

    hr = IMediaDet_put_CurrentStream(pM, 2);
    ok(hr == E_INVALIDARG, "IMediaDet_put_CurrentStream\n");

    strm = -1;
    hr = IMediaDet_get_CurrentStream(pM, &strm);
    ok(hr == S_OK, "IMediaDet_get_CurrentStream\n");
    ok(strm == 1, "IMediaDet_get_CurrentStream\n");

    hr = IMediaDet_Release(pM);
    ok(hr == 0, "IMediaDet_Release returned: %x\n", hr);

    DeleteFileW(test_sound_avi_filename);
}

static void test_samplegrabber(void)
{
    struct unk_impl unk_obj = {{&unk_vtbl}, 19, NULL};
    ISampleGrabber *sg;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_SampleGrabber, &unk_obj.IUnknown_iface, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&unk_obj.inner_unk);
    ok(hr == S_OK, "CoCreateInstance failed: %08x\n", hr);

    hr = IUnknown_QueryInterface(unk_obj.inner_unk, &IID_ISampleGrabber, (void**)&sg);
    ok(hr == S_OK, "QueryInterface for IID_ISampleGrabber failed: %08x\n", hr);
    refcount = ISampleGrabber_AddRef(sg);
    ok(refcount == unk_obj.ref, "SampleGrabber just pretends to support COM aggregation\n");
    refcount = ISampleGrabber_Release(sg);
    ok(refcount == unk_obj.ref, "SampleGrabber just pretends to support COM aggregation\n");
    refcount = ISampleGrabber_Release(sg);
    ok(refcount == 19, "Refcount should be back at 19 but is %u\n", refcount);

    IUnknown_Release(unk_obj.inner_unk);
}

START_TEST(mediadet)
{
    if (!init_tests())
    {
        skip("Couldn't initialize tests!\n");
        return;
    }

    CoInitialize(NULL);
    test_mediadet();
    test_samplegrabber();
    CoUninitialize();
}
