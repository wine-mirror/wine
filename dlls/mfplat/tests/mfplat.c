/*
 * Unit test suite for mfplat.
 *
 * Copyright 2015 Michael Müller
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

#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"

#include "initguid.h"
#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "mfreadwrite.h"

#include "wine/test.h"

static BOOL is_win8_plus;

static HRESULT (WINAPI *pMFCopyImage)(BYTE *dest, LONG deststride, const BYTE *src, LONG srcstride,
        DWORD width, DWORD lines);
static HRESULT (WINAPI *pMFCreateSourceResolver)(IMFSourceResolver **resolver);
static HRESULT (WINAPI *pMFCreateMFByteStreamOnStream)(IStream *stream, IMFByteStream **bytestream);
static HRESULT (WINAPI *pMFCreateMemoryBuffer)(DWORD max_length, IMFMediaBuffer **buffer);
static void*   (WINAPI *pMFHeapAlloc)(SIZE_T size, ULONG flags, char *file, int line, EAllocationType type);
static void    (WINAPI *pMFHeapFree)(void *p);
static HRESULT (WINAPI *pMFPutWaitingWorkItem)(HANDLE event, LONG priority, IMFAsyncResult *result, MFWORKITEM_KEY *key);
static HRESULT (WINAPI *pMFAllocateSerialWorkQueue)(DWORD queue, DWORD *serial_queue);
static HRESULT (WINAPI *pMFAddPeriodicCallback)(MFPERIODICCALLBACK callback, IUnknown *context, DWORD *key);
static HRESULT (WINAPI *pMFRemovePeriodicCallback)(DWORD key);

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

DEFINE_GUID(MF_BYTESTREAM_CONTENT_TYPE, 0xfc358289,0x3cb6,0x460c,0xa4,0x24,0xb6,0x68,0x12,0x60,0x37,0x5a);

DEFINE_GUID(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 0xa634a91c, 0x822b, 0x41b9, 0xa4, 0x94, 0x4d, 0xe4, 0x64, 0x36, 0x12, 0xb0);

DEFINE_GUID(MFT_CATEGORY_OTHER, 0x90175d57,0xb7ea,0x4901,0xae,0xb3,0x93,0x3a,0x87,0x47,0x75,0x6f);

DEFINE_GUID(DUMMY_CLSID, 0x12345678,0x1234,0x1234,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19);
DEFINE_GUID(DUMMY_GUID1, 0x12345678,0x1234,0x1234,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21);
DEFINE_GUID(DUMMY_GUID2, 0x12345678,0x1234,0x1234,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22);

static const WCHAR mp4file[] = {'t','e','s','t','.','m','p','4',0};

static WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    lstrcatW(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0,
                       NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %d\n",
       wine_dbgstr_w(pathW), GetLastError());

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok(res != 0, "couldn't find resource\n");
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource(GetModuleHandleA(NULL), res),
               &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res),
       "couldn't write resource\n" );
    CloseHandle(file);

    return pathW;
}

static BOOL check_clsid(CLSID *clsids, UINT32 count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        if (IsEqualGUID(&clsids[i], &DUMMY_CLSID))
            return TRUE;
    }
    return FALSE;
}

static void test_register(void)
{
    static WCHAR name[] = {'W','i','n','e',' ','t','e','s','t',0};
    MFT_REGISTER_TYPE_INFO input[] =
    {
        { DUMMY_CLSID, DUMMY_GUID1 }
    };
    MFT_REGISTER_TYPE_INFO output[] =
    {
        { DUMMY_CLSID, DUMMY_GUID2 }
    };
    CLSID *clsids;
    UINT32 count;
    HRESULT ret;

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 1, input, 1, output, NULL);
    if (ret == E_ACCESSDENIED)
    {
        win_skip("Not enough permissions to register a filter\n");
        return;
    }
    ok(ret == S_OK, "Failed to register dummy filter: %x\n", ret);

if(0)
{
    /* NULL name crashes on windows */
    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, NULL, 0, 1, input, 1, output, NULL);
    ok(ret == E_INVALIDARG, "got %x\n", ret);
}

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 0, NULL, 0, NULL, NULL);
    ok(ret == S_OK, "Failed to register dummy filter: %x\n", ret);

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 1, NULL, 0, NULL, NULL);
    ok(ret == S_OK, "Failed to register dummy filter: %x\n", ret);

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 0, NULL, 1, NULL, NULL);
    ok(ret == S_OK, "Failed to register dummy filter: %x\n", ret);

if(0)
{
    /* NULL clsids/count crashes on windows (vista) */
    count = 0;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, NULL, NULL, NULL, &count);
    ok(ret == E_POINTER, "Failed to enumerate filters: %x\n", ret);
    ok(count == 0, "Expected count > 0\n");

    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, NULL, NULL, &clsids, NULL);
    ok(ret == E_POINTER, "Failed to enumerate filters: %x\n", ret);
    ok(count == 0, "Expected count > 0\n");
}

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, NULL, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, input, NULL, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, output, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, input, output, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    /* exchange input and output */
    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, output, input, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(!count, "got %d\n", count);
    ok(clsids == NULL, "Expected clsids == NULL\n");

    ret = MFTUnregister(DUMMY_CLSID);
    ok(ret == S_OK ||
       /* w7pro64 */
       broken(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)), "got %x\n", ret);

    ret = MFTUnregister(DUMMY_CLSID);
    ok(ret == S_OK || broken(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)), "got %x\n", ret);
}

static void test_source_resolver(void)
{
    IMFSourceResolver *resolver, *resolver2;
    IMFByteStream *bytestream;
    IMFAttributes *attributes;
    IMFMediaSource *mediasource;
    IMFPresentationDescriptor *descriptor;
    MF_OBJECT_TYPE obj_type;
    HRESULT hr;
    WCHAR *filename;

    static const WCHAR file_type[] = {'v','i','d','e','o','/','m','p','4',0};

    if (!pMFCreateSourceResolver)
    {
        win_skip("MFCreateSourceResolver() not found\n");
        return;
    }

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = pMFCreateSourceResolver(NULL);
    ok(hr == E_POINTER, "got %#x\n", hr);

    hr = pMFCreateSourceResolver(&resolver);
    ok(hr == S_OK, "got %#x\n", hr);

    hr = pMFCreateSourceResolver(&resolver2);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(resolver != resolver2, "Expected new instance\n");

    IMFSourceResolver_Release(resolver2);

    filename = load_resource(mp4file);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, NULL, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
        &obj_type, (IUnknown **)&mediasource);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, bytestream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
        NULL, (IUnknown **)&mediasource);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, bytestream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
        &obj_type, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, bytestream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
        &obj_type, (IUnknown **)&mediasource);
    todo_wine ok(hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE, "got 0x%08x\n", hr);
    if (hr == S_OK) IMFMediaSource_Release(mediasource);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, bytestream, NULL, MF_RESOLUTION_BYTESTREAM, NULL,
        &obj_type, (IUnknown **)&mediasource);
    todo_wine ok(hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE, "got 0x%08x\n", hr);

    IMFByteStream_Release(bytestream);

    /* We have to create a new bytestream here, because all following
     * calls to CreateObjectFromByteStream will fail. */
    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IUnknown_QueryInterface(bytestream, &IID_IMFAttributes,
                                 (void **)&attributes);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IMFAttributes_SetString(attributes, &MF_BYTESTREAM_CONTENT_TYPE, file_type);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
    IMFAttributes_Release(attributes);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, bytestream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
        &obj_type, (IUnknown **)&mediasource);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(mediasource != NULL, "got %p\n", mediasource);
    ok(obj_type == MF_OBJECT_MEDIASOURCE, "got %d\n", obj_type);

    hr = IMFMediaSource_CreatePresentationDescriptor(
        mediasource, &descriptor);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(descriptor != NULL, "got %p\n", descriptor);

    IMFPresentationDescriptor_Release(descriptor);
    IMFMediaSource_Release(mediasource);
    IMFByteStream_Release(bytestream);

    IMFSourceResolver_Release(resolver);

    MFShutdown();

    DeleteFileW(filename);
}

static void init_functions(void)
{
    HMODULE mod = GetModuleHandleA("mfplat.dll");

#define X(f) p##f = (void*)GetProcAddress(mod, #f)
    X(MFAddPeriodicCallback);
    X(MFAllocateSerialWorkQueue);
    X(MFCopyImage);
    X(MFCreateSourceResolver);
    X(MFCreateMFByteStreamOnStream);
    X(MFCreateMemoryBuffer);
    X(MFHeapAlloc);
    X(MFHeapFree);
    X(MFPutWaitingWorkItem);
    X(MFRemovePeriodicCallback);
#undef X

    is_win8_plus = pMFPutWaitingWorkItem != NULL;
}

static void test_MFCreateMediaType(void)
{
    HRESULT hr;
    IMFMediaType *mediatype;

if(0)
{
    /* Crash on Windows Vista/7 */
    hr = MFCreateMediaType(NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
}

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);

    IMFMediaType_Release(mediatype);
}

static void test_MFCreateMediaEvent(void)
{
    HRESULT hr;
    IMFMediaEvent *mediaevent;

    MediaEventType type;
    GUID extended_type;
    HRESULT status;
    PROPVARIANT value;

    PropVariantInit(&value);
    value.vt = VT_UNKNOWN;

    hr = MFCreateMediaEvent(MEError, &GUID_NULL, E_FAIL, &value, &mediaevent);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    PropVariantClear(&value);

    hr = IMFMediaEvent_GetType(mediaevent, &type);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(type == MEError, "got %#x\n", type);

    hr = IMFMediaEvent_GetExtendedType(mediaevent, &extended_type);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualGUID(&extended_type, &GUID_NULL), "got %s\n",
       wine_dbgstr_guid(&extended_type));

    hr = IMFMediaEvent_GetStatus(mediaevent, &status);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(status == E_FAIL, "got 0x%08x\n", status);

    PropVariantInit(&value);
    hr = IMFMediaEvent_GetValue(mediaevent, &value);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(value.vt == VT_UNKNOWN, "got %#x\n", value.vt);
    PropVariantClear(&value);

    IMFMediaEvent_Release(mediaevent);

    hr = MFCreateMediaEvent(MEUnknown, &DUMMY_GUID1, S_OK, NULL, &mediaevent);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaEvent_GetType(mediaevent, &type);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(type == MEUnknown, "got %#x\n", type);

    hr = IMFMediaEvent_GetExtendedType(mediaevent, &extended_type);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualGUID(&extended_type, &DUMMY_GUID1), "got %s\n",
       wine_dbgstr_guid(&extended_type));

    hr = IMFMediaEvent_GetStatus(mediaevent, &status);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(status == S_OK, "got 0x%08x\n", status);

    PropVariantInit(&value);
    hr = IMFMediaEvent_GetValue(mediaevent, &value);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(value.vt == VT_EMPTY, "got %#x\n", value.vt);
    PropVariantClear(&value);

    IMFMediaEvent_Release(mediaevent);
}

static void test_MFCreateAttributes(void)
{
    IMFAttributes *attributes;
    HRESULT hr;
    UINT32 count;

    hr = MFCreateAttributes( &attributes, 3 );
    ok(hr == S_OK, "got 0x%08x\n", hr);

    count = 88;
    hr = IMFAttributes_GetCount(attributes, &count);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 0, "got %d\n", count);

    hr = IMFAttributes_SetUINT32(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 0);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFAttributes_GetCount(attributes, &count);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
    todo_wine ok(count == 1, "got %d\n", count);

    IMFAttributes_Release(attributes);
}

static void test_MFCreateMFByteStreamOnStream(void)
{
    IMFByteStream *bytestream;
    IMFByteStream *bytestream2;
    IStream *stream;
    IMFAttributes *attributes = NULL;
    IUnknown *unknown;
    HRESULT hr;
    ULONG ref;

    if(!pMFCreateMFByteStreamOnStream)
    {
        win_skip("MFCreateMFByteStreamOnStream() not found\n");
        return;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = pMFCreateMFByteStreamOnStream(stream, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IUnknown,
                                 (void **)&unknown);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok((void *)unknown == (void *)bytestream, "got %p\n", unknown);
    ref = IUnknown_Release(unknown);
    ok(ref == 1, "got %u\n", ref);

    hr = IUnknown_QueryInterface(unknown, &IID_IMFByteStream,
                                 (void **)&bytestream2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(bytestream2 == bytestream, "got %p\n", bytestream2);
    ref = IMFByteStream_Release(bytestream2);
    ok(ref == 1, "got %u\n", ref);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFAttributes,
                                 (void **)&attributes);
    ok(hr == S_OK ||
       /* w7pro64 */
       broken(hr == E_NOINTERFACE), "got 0x%08x\n", hr);

    if (hr != S_OK)
    {
        win_skip("Can not retrieve IMFAttributes interface from IMFByteStream\n");
        IStream_Release(stream);
        IMFByteStream_Release(bytestream);
        return;
    }

    ok(attributes != NULL, "got NULL\n");

    hr = IMFAttributes_QueryInterface(attributes, &IID_IUnknown,
                                 (void **)&unknown);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok((void *)unknown == (void *)bytestream, "got %p\n", unknown);
    ref = IUnknown_Release(unknown);
    ok(ref == 2, "got %u\n", ref);

    hr = IMFAttributes_QueryInterface(attributes, &IID_IMFByteStream,
                                 (void **)&bytestream2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(bytestream2 == bytestream, "got %p\n", bytestream2);
    ref = IMFByteStream_Release(bytestream2);
    ok(ref == 2, "got %u\n", ref);

    IMFAttributes_Release(attributes);
    IMFByteStream_Release(bytestream);
    IStream_Release(stream);
}

static void test_MFCreateFile(void)
{
    IMFByteStream *bytestream;
    IMFByteStream *bytestream2;
    IMFAttributes *attributes = NULL;
    HRESULT hr;
    WCHAR *filename;

    static const WCHAR newfilename[] = {'n','e','w','.','m','p','4',0};

    filename = load_resource(mp4file);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFAttributes,
                                 (void **)&attributes);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(attributes != NULL, "got NULL\n");
    IMFAttributes_Release(attributes);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IMFByteStream_Release(bytestream2);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream2);
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "got 0x%08x\n", hr);
    if (hr == S_OK) IMFByteStream_Release(bytestream2);

    hr = MFCreateFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream2);
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "got 0x%08x\n", hr);
    if (hr == S_OK) IMFByteStream_Release(bytestream2);

    IMFByteStream_Release(bytestream);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, newfilename, &bytestream);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "got 0x%08x\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "got 0x%08x\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_EXIST,
                      MF_FILEFLAGS_NONE, newfilename, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, newfilename, &bytestream2);
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "got 0x%08x\n", hr);
    if (hr == S_OK) IMFByteStream_Release(bytestream2);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, newfilename, &bytestream2);
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "got 0x%08x\n", hr);
    if (hr == S_OK) IMFByteStream_Release(bytestream2);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_ALLOW_WRITE_SHARING, newfilename, &bytestream2);
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "got 0x%08x\n", hr);
    if (hr == S_OK) IMFByteStream_Release(bytestream2);

    IMFByteStream_Release(bytestream);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_ALLOW_WRITE_SHARING, newfilename, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* Opening the file again fails even though MF_FILEFLAGS_ALLOW_WRITE_SHARING is set. */
    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_ALLOW_WRITE_SHARING, newfilename, &bytestream2);
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "got 0x%08x\n", hr);
    if (hr == S_OK) IMFByteStream_Release(bytestream2);

    IMFByteStream_Release(bytestream);

    MFShutdown();

    DeleteFileW(filename);
    DeleteFileW(newfilename);
}

static void test_MFCreateMemoryBuffer(void)
{
    IMFMediaBuffer *buffer;
    HRESULT hr;
    DWORD length, max;
    BYTE *data, *data2;

    if(!pMFCreateMemoryBuffer)
    {
        win_skip("MFCreateMemoryBuffer() not found\n");
        return;
    }

    hr = pMFCreateMemoryBuffer(1024, NULL);
    ok(hr == E_INVALIDARG || hr == E_POINTER, "got 0x%08x\n", hr);

    hr = pMFCreateMemoryBuffer(0, &buffer);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    if(buffer)
    {
        hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(length == 0, "got %u\n", length);

        IMFMediaBuffer_Release(buffer);
    }

    hr = pMFCreateMemoryBuffer(1024, &buffer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_GetMaxLength(buffer, NULL);
    ok(hr == E_INVALIDARG || hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(length == 1024, "got %u\n", length);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 1025);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 10);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, NULL);
    ok(hr == E_INVALIDARG || hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(length == 10, "got %u\n", length);

    length = 0;
    max = 0;
    hr = IMFMediaBuffer_Lock(buffer, NULL, &length, &max);
    ok(hr == E_INVALIDARG || hr == E_POINTER, "got 0x%08x\n", hr);
    ok(length == 0, "got %u\n", length);
    ok(max == 0, "got %u\n", length);

    hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(length == 10, "got %u\n", length);
    ok(max == 1024, "got %u\n", max);

    /* Attempt to lock the bufer twice */
    hr = IMFMediaBuffer_Lock(buffer, &data2, &max, &length);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(data == data2, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* Extra Unlock */
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IMFMediaBuffer_Release(buffer);
}

static void test_MFSample(void)
{
    IMFSample *sample;
    HRESULT hr;
    UINT32 count;

    hr = MFCreateSample( &sample );
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 0, "got %d\n", count);

    IMFSample_Release(sample);
}

struct test_callback
{
    IMFAsyncCallback IMFAsyncCallback_iface;
    HANDLE event;
};

static struct test_callback *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct test_callback, IMFAsyncCallback_iface);
}

static HRESULT WINAPI testcallback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testcallback_AddRef(IMFAsyncCallback *iface)
{
    return 2;
}

static ULONG WINAPI testcallback_Release(IMFAsyncCallback *iface)
{
    return 1;
}

static HRESULT WINAPI testcallback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    ok(flags != NULL && queue != NULL, "Unexpected arguments.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testcallback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    IMFMediaEventQueue *queue;
    IUnknown *state, *obj;
    HRESULT hr;

    ok(result != NULL, "Unexpected result object.\n");

    state = IMFAsyncResult_GetStateNoAddRef(result);
    if (state && SUCCEEDED(IUnknown_QueryInterface(state, &IID_IMFMediaEventQueue, (void **)&queue)))
    {
        IMFMediaEvent *event;

        if (is_win8_plus)
        {
            hr = IMFMediaEventQueue_GetEvent(queue, MF_EVENT_FLAG_NO_WAIT, &event);
            ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Failed to get event, hr %#x.\n", hr);

            hr = IMFMediaEventQueue_GetEvent(queue, 0, &event);
            ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Failed to get event, hr %#x.\n", hr);
        }

        hr = IMFMediaEventQueue_EndGetEvent(queue, result, &event);
        ok(hr == S_OK, "Failed to finalize GetEvent, hr %#x.\n", hr);

        hr = IMFAsyncResult_GetObject(result, &obj);
        ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

        IMFMediaEvent_Release(event);

        hr = IMFMediaEventQueue_EndGetEvent(queue, result, &event);
        ok(hr == E_FAIL, "Unexpected result, hr %#x.\n", hr);

        IMFMediaEventQueue_Release(queue);

        SetEvent(callback->event);
    }

    return E_NOTIMPL;
}

static const IMFAsyncCallbackVtbl testcallbackvtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    testcallback_Invoke,
};

static void init_test_callback(struct test_callback *callback)
{
    callback->IMFAsyncCallback_iface.lpVtbl = &testcallbackvtbl;
    callback->event = NULL;
}

static void test_MFCreateAsyncResult(void)
{
    IMFAsyncResult *result, *result2;
    struct test_callback callback;
    IUnknown *state, *object;
    MFASYNCRESULT *data;
    ULONG refcount;
    HRESULT hr;

    init_test_callback(&callback);

    hr = MFCreateAsyncResult(NULL, NULL, NULL, NULL);
    ok(FAILED(hr), "Unexpected hr %#x.\n", hr);

    hr = MFCreateAsyncResult(NULL, NULL, NULL, &result);
    ok(hr == S_OK, "Failed to create object, hr %#x.\n", hr);

    data = (MFASYNCRESULT *)result;
    ok(data->pCallback == NULL, "Unexpected callback value.\n");
    ok(data->hrStatusResult == S_OK, "Unexpected status %#x.\n", data->hrStatusResult);
    ok(data->dwBytesTransferred == 0, "Unexpected byte length %u.\n", data->dwBytesTransferred);
    ok(data->hEvent == NULL, "Unexpected event.\n");

    hr = IMFAsyncResult_GetState(result, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    state = (void *)0xdeadbeef;
    hr = IMFAsyncResult_GetState(result, &state);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);
    ok(state == (void *)0xdeadbeef, "Unexpected state.\n");

    hr = IMFAsyncResult_GetStatus(result);
    ok(hr == S_OK, "Unexpected status %#x.\n", hr);

    data->hrStatusResult = 123;
    hr = IMFAsyncResult_GetStatus(result);
    ok(hr == 123, "Unexpected status %#x.\n", hr);

    hr = IMFAsyncResult_SetStatus(result, E_FAIL);
    ok(hr == S_OK, "Failed to set status, hr %#x.\n", hr);
    ok(data->hrStatusResult == E_FAIL, "Unexpected status %#x.\n", hr);

    hr = IMFAsyncResult_GetObject(result, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    object = (void *)0xdeadbeef;
    hr = IMFAsyncResult_GetObject(result, &object);
    ok(hr == E_POINTER, "Failed to get object, hr %#x.\n", hr);
    ok(object == (void *)0xdeadbeef, "Unexpected object.\n");

    state = IMFAsyncResult_GetStateNoAddRef(result);
    ok(state == NULL, "Unexpected state.\n");

    /* Object. */
    hr = MFCreateAsyncResult((IUnknown *)result, &callback.IMFAsyncCallback_iface, NULL, &result2);
    ok(hr == S_OK, "Failed to create object, hr %#x.\n", hr);

    data = (MFASYNCRESULT *)result2;
    ok(data->pCallback == &callback.IMFAsyncCallback_iface, "Unexpected callback value.\n");
    ok(data->hrStatusResult == S_OK, "Unexpected status %#x.\n", data->hrStatusResult);
    ok(data->dwBytesTransferred == 0, "Unexpected byte length %u.\n", data->dwBytesTransferred);
    ok(data->hEvent == NULL, "Unexpected event.\n");

    object = NULL;
    hr = IMFAsyncResult_GetObject(result2, &object);
    ok(hr == S_OK, "Failed to get object, hr %#x.\n", hr);
    ok(object == (IUnknown *)result, "Unexpected object.\n");
    IUnknown_Release(object);

    IMFAsyncResult_Release(result2);

    /* State object. */
    hr = MFCreateAsyncResult(NULL, &callback.IMFAsyncCallback_iface, (IUnknown *)result, &result2);
    ok(hr == S_OK, "Failed to create object, hr %#x.\n", hr);

    data = (MFASYNCRESULT *)result2;
    ok(data->pCallback == &callback.IMFAsyncCallback_iface, "Unexpected callback value.\n");
    ok(data->hrStatusResult == S_OK, "Unexpected status %#x.\n", data->hrStatusResult);
    ok(data->dwBytesTransferred == 0, "Unexpected byte length %u.\n", data->dwBytesTransferred);
    ok(data->hEvent == NULL, "Unexpected event.\n");

    state = NULL;
    hr = IMFAsyncResult_GetState(result2, &state);
    ok(hr == S_OK, "Failed to get state object, hr %#x.\n", hr);
    ok(state == (IUnknown *)result, "Unexpected state.\n");
    IUnknown_Release(state);

    state = IMFAsyncResult_GetStateNoAddRef(result2);
    ok(state == (IUnknown *)result, "Unexpected state.\n");

    refcount = IMFAsyncResult_Release(result2);
    ok(!refcount, "Unexpected refcount %u\n.", refcount);
    refcount = IMFAsyncResult_Release(result);
    ok(!refcount, "Unexpected refcount %u\n.", refcount);
}

static void test_startup(void)
{
    DWORD queue;
    HRESULT hr;

    hr = MFStartup(MAKELONG(MF_API_VERSION, 0xdead), MFSTARTUP_FULL);
    ok(hr == MF_E_BAD_STARTUP_VERSION, "Unexpected hr %#x.\n", hr);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#x.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == S_OK, "Failed to allocate a queue, hr %#x.\n", hr);
    hr = MFUnlockWorkQueue(queue);
    ok(hr == S_OK, "Failed to unlock the queue, hr %#x.\n", hr);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    /* Already shut down, has no effect. */
    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#x.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == S_OK, "Failed to allocate a queue, hr %#x.\n", hr);
    hr = MFUnlockWorkQueue(queue);
    ok(hr == S_OK, "Failed to unlock the queue, hr %#x.\n", hr);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    /* Platform lock. */
    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#x.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == S_OK, "Failed to allocate a queue, hr %#x.\n", hr);
    hr = MFUnlockWorkQueue(queue);
    ok(hr == S_OK, "Failed to unlock the queue, hr %#x.\n", hr);

    /* Unlocking implies shutdown. */
    hr = MFUnlockPlatform();
    ok(hr == S_OK, "Failed to unlock, %#x.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = MFLockPlatform();
    ok(hr == S_OK, "Failed to lock, %#x.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == S_OK, "Failed to allocate a queue, hr %#x.\n", hr);
    hr = MFUnlockWorkQueue(queue);
    ok(hr == S_OK, "Failed to unlock the queue, hr %#x.\n", hr);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
}

static void test_allocate_queue(void)
{
    DWORD queue, queue2;
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#x.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == S_OK, "Failed to allocate a queue, hr %#x.\n", hr);
    ok(queue & MFASYNC_CALLBACK_QUEUE_PRIVATE_MASK, "Unexpected queue id.\n");

    hr = MFUnlockWorkQueue(queue);
    ok(hr == S_OK, "Failed to unlock the queue, hr %#x.\n", hr);

    hr = MFUnlockWorkQueue(queue);
    ok(FAILED(hr), "Unexpected hr %#x.\n", hr);

    hr = MFAllocateWorkQueue(&queue2);
    ok(hr == S_OK, "Failed to allocate a queue, hr %#x.\n", hr);
    ok(queue2 & MFASYNC_CALLBACK_QUEUE_PRIVATE_MASK, "Unexpected queue id.\n");

    hr = MFUnlockWorkQueue(queue2);
    ok(hr == S_OK, "Failed to unlock the queue, hr %#x.\n", hr);

    /* Unlock in system queue range. */
    hr = MFUnlockWorkQueue(MFASYNC_CALLBACK_QUEUE_STANDARD);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = MFUnlockWorkQueue(MFASYNC_CALLBACK_QUEUE_UNDEFINED);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = MFUnlockWorkQueue(0x20);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shutdown, hr %#x.\n", hr);
}

static void test_MFCopyImage(void)
{
    BYTE dest[16], src[16];
    HRESULT hr;

    if (!pMFCopyImage)
    {
        win_skip("MFCopyImage() is not available.\n");
        return;
    }

    memset(dest, 0xaa, sizeof(dest));
    memset(src, 0x11, sizeof(src));

    hr = pMFCopyImage(dest, 8, src, 8, 4, 1);
    ok(hr == S_OK, "Failed to copy image %#x.\n", hr);
    ok(!memcmp(dest, src, 4) && dest[4] == 0xaa, "Unexpected buffer contents.\n");

    memset(dest, 0xaa, sizeof(dest));
    memset(src, 0x11, sizeof(src));

    hr = pMFCopyImage(dest, 8, src, 8, 16, 1);
    ok(hr == S_OK, "Failed to copy image %#x.\n", hr);
    ok(!memcmp(dest, src, 16), "Unexpected buffer contents.\n");

    memset(dest, 0xaa, sizeof(dest));
    memset(src, 0x11, sizeof(src));

    hr = pMFCopyImage(dest, 8, src, 8, 8, 2);
    ok(hr == S_OK, "Failed to copy image %#x.\n", hr);
    ok(!memcmp(dest, src, 16), "Unexpected buffer contents.\n");
}

static void test_MFCreateCollection(void)
{
    IMFCollection *collection;
    IUnknown *element;
    DWORD count;
    HRESULT hr;

    hr = MFCreateCollection(NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = MFCreateCollection(&collection);
    ok(hr == S_OK, "Failed to create collection, hr %#x.\n", hr);

    hr = IMFCollection_GetElementCount(collection, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    count = 1;
    hr = IMFCollection_GetElementCount(collection, &count);
    ok(hr == S_OK, "Failed to get element count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected count %u.\n", count);

    hr = IMFCollection_GetElement(collection, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    element = (void *)0xdeadbeef;
    hr = IMFCollection_GetElement(collection, 0, &element);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);
    ok(element == (void *)0xdeadbeef, "Unexpected pointer.\n");

    hr = IMFCollection_RemoveElement(collection, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    element = (void *)0xdeadbeef;
    hr = IMFCollection_RemoveElement(collection, 0, &element);
    ok(hr == E_INVALIDARG, "Failed to remove element, hr %#x.\n", hr);
    ok(element == (void *)0xdeadbeef, "Unexpected pointer.\n");

    hr = IMFCollection_RemoveAllElements(collection);
    ok(hr == S_OK, "Failed to clear, hr %#x.\n", hr);

    hr = IMFCollection_AddElement(collection, (IUnknown *)collection);
    ok(hr == S_OK, "Failed to add element, hr %#x.\n", hr);

    count = 0;
    hr = IMFCollection_GetElementCount(collection, &count);
    ok(hr == S_OK, "Failed to get element count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = IMFCollection_AddElement(collection, NULL);
    ok(hr == S_OK, "Failed to add element, hr %#x.\n", hr);

    count = 0;
    hr = IMFCollection_GetElementCount(collection, &count);
    ok(hr == S_OK, "Failed to get element count, hr %#x.\n", hr);
    ok(count == 2, "Unexpected count %u.\n", count);

    hr = IMFCollection_InsertElementAt(collection, 10, (IUnknown *)collection);
    ok(hr == S_OK, "Failed to insert element, hr %#x.\n", hr);

    count = 0;
    hr = IMFCollection_GetElementCount(collection, &count);
    ok(hr == S_OK, "Failed to get element count, hr %#x.\n", hr);
    ok(count == 11, "Unexpected count %u.\n", count);

    hr = IMFCollection_GetElement(collection, 0, &element);
    ok(hr == S_OK, "Failed to get element, hr %#x.\n", hr);
    ok(element == (IUnknown *)collection, "Unexpected element.\n");
    IUnknown_Release(element);

    hr = IMFCollection_GetElement(collection, 1, &element);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#x.\n", hr);
    ok(!element, "Unexpected element.\n");

    hr = IMFCollection_GetElement(collection, 2, &element);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#x.\n", hr);
    ok(!element, "Unexpected element.\n");

    hr = IMFCollection_GetElement(collection, 10, &element);
    ok(hr == S_OK, "Failed to get element, hr %#x.\n", hr);
    ok(element == (IUnknown *)collection, "Unexpected element.\n");
    IUnknown_Release(element);

    hr = IMFCollection_InsertElementAt(collection, 0, NULL);
    ok(hr == S_OK, "Failed to insert element, hr %#x.\n", hr);

    hr = IMFCollection_GetElement(collection, 0, &element);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#x.\n", hr);

    hr = IMFCollection_RemoveAllElements(collection);
    ok(hr == S_OK, "Failed to clear, hr %#x.\n", hr);

    count = 1;
    hr = IMFCollection_GetElementCount(collection, &count);
    ok(hr == S_OK, "Failed to get element count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected count %u.\n", count);

    hr = IMFCollection_InsertElementAt(collection, 0, NULL);
    ok(hr == S_OK, "Failed to insert element, hr %#x.\n", hr);

    IMFCollection_Release(collection);
}

static void test_MFHeapAlloc(void)
{
    void *res;

    if (!pMFHeapAlloc)
    {
        win_skip("MFHeapAlloc() is not available.\n");
        return;
    }

    res = pMFHeapAlloc(16, 0, NULL, 0, eAllocationTypeIgnore);
    ok(res != NULL, "MFHeapAlloc failed.\n");

    pMFHeapFree(res);
}

static void test_scheduled_items(void)
{
    struct test_callback callback;
    IMFAsyncResult *result;
    MFWORKITEM_KEY key, key2;
    HRESULT hr;

    init_test_callback(&callback);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#x.\n", hr);

    hr = MFScheduleWorkItem(&callback.IMFAsyncCallback_iface, NULL, -5000, &key);
    ok(hr == S_OK, "Failed to schedule item, hr %#x.\n", hr);

    hr = MFCancelWorkItem(key);
    ok(hr == S_OK, "Failed to cancel item, hr %#x.\n", hr);

    hr = MFCancelWorkItem(key);
    ok(hr == MF_E_NOT_FOUND || broken(hr == S_OK) /* < win10 */, "Unexpected hr %#x.\n", hr);

    if (!pMFPutWaitingWorkItem)
    {
        win_skip("Waiting items are not supported.\n");
        return;
    }

    hr = MFCreateAsyncResult(NULL, &callback.IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#x.\n", hr);

    hr = pMFPutWaitingWorkItem(NULL, 0, result, &key);
    ok(hr == S_OK, "Failed to add waiting item, hr %#x.\n", hr);

    hr = pMFPutWaitingWorkItem(NULL, 0, result, &key2);
    ok(hr == S_OK, "Failed to add waiting item, hr %#x.\n", hr);

    hr = MFCancelWorkItem(key);
    ok(hr == S_OK, "Failed to cancel item, hr %#x.\n", hr);

    hr = MFCancelWorkItem(key2);
    ok(hr == S_OK, "Failed to cancel item, hr %#x.\n", hr);

    IMFAsyncResult_Release(result);

    hr = MFScheduleWorkItem(&callback.IMFAsyncCallback_iface, NULL, -5000, &key);
    ok(hr == S_OK, "Failed to schedule item, hr %#x.\n", hr);

    hr = MFCancelWorkItem(key);
    ok(hr == S_OK, "Failed to cancel item, hr %#x.\n", hr);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
}

static void test_serial_queue(void)
{
    static const DWORD queue_ids[] =
    {
        MFASYNC_CALLBACK_QUEUE_STANDARD,
        MFASYNC_CALLBACK_QUEUE_RT,
        MFASYNC_CALLBACK_QUEUE_IO,
        MFASYNC_CALLBACK_QUEUE_TIMER,
        MFASYNC_CALLBACK_QUEUE_MULTITHREADED,
        MFASYNC_CALLBACK_QUEUE_LONG_FUNCTION,
    };
    DWORD queue, serial_queue;
    unsigned int i;
    HRESULT hr;

    if (!pMFAllocateSerialWorkQueue)
    {
        skip("Serial queues are not supported.\n");
        return;
    }

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(queue_ids); ++i)
    {
        BOOL broken_types = queue_ids[i] == MFASYNC_CALLBACK_QUEUE_TIMER ||
                queue_ids[i] == MFASYNC_CALLBACK_QUEUE_LONG_FUNCTION;

        hr = pMFAllocateSerialWorkQueue(queue_ids[i], &serial_queue);
        ok(hr == S_OK || broken(broken_types && hr == E_INVALIDARG) /* Win8 */,
                "%u: failed to allocate a queue, hr %#x.\n", i, hr);

        if (SUCCEEDED(hr))
        {
            hr = MFUnlockWorkQueue(serial_queue);
            ok(hr == S_OK, "%u: failed to unlock the queue, hr %#x.\n", i, hr);
        }
    }

    /* Chain them together. */
    hr = pMFAllocateSerialWorkQueue(MFASYNC_CALLBACK_QUEUE_STANDARD, &serial_queue);
    ok(hr == S_OK, "Failed to allocate a queue, hr %#x.\n", hr);

    hr = pMFAllocateSerialWorkQueue(serial_queue, &queue);
    ok(hr == S_OK, "Failed to allocate a queue, hr %#x.\n", hr);

    hr = MFUnlockWorkQueue(serial_queue);
    ok(hr == S_OK, "Failed to unlock the queue, hr %#x.\n", hr);

    hr = MFUnlockWorkQueue(queue);
    ok(hr == S_OK, "Failed to unlock the queue, hr %#x.\n", hr);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
}

static LONG periodic_counter;
static void CALLBACK periodic_callback(IUnknown *context)
{
    InterlockedIncrement(&periodic_counter);
}

static void test_periodic_callback(void)
{
    DWORD period, key;
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#x.\n", hr);

    period = 0;
    hr = MFGetTimerPeriodicity(&period);
    ok(hr == S_OK, "Failed to get timer perdiod, hr %#x.\n", hr);
    ok(period == 10, "Unexpected period %u.\n", period);

    if (!pMFAddPeriodicCallback)
    {
        win_skip("Periodic callbacks are not supported.\n");
        MFShutdown();
        return;
    }

    ok(periodic_counter == 0, "Unexpected counter value %u.\n", periodic_counter);

    hr = pMFAddPeriodicCallback(periodic_callback, NULL, &key);
    ok(hr == S_OK, "Failed to add periodic callback, hr %#x.\n", hr);
    ok(key != 0, "Unexpected key %#x.\n", key);

    Sleep(10 * period);

    hr = pMFRemovePeriodicCallback(key);
    ok(hr == S_OK, "Failed to remove callback, hr %#x.\n", hr);

    ok(periodic_counter > 0, "Unexpected counter value %u.\n", periodic_counter);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
}

static void test_event_queue(void)
{
    struct test_callback callback, callback2;
    IMFMediaEvent *event, *event2;
    IMFMediaEventQueue *queue;
    IMFAsyncResult *result;
    HRESULT hr;
    DWORD ret;

    init_test_callback(&callback);
    init_test_callback(&callback2);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#x.\n", hr);

    hr = MFCreateEventQueue(&queue);
    ok(hr == S_OK, "Failed to create event queue, hr %#x.\n", hr);

    hr = IMFMediaEventQueue_GetEvent(queue, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_NO_EVENTS_AVAILABLE, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMediaEvent(MEError, &GUID_NULL, E_FAIL, NULL, &event);
    ok(hr == S_OK, "Failed to create event object, hr %#x.\n", hr);

    if (is_win8_plus)
    {
        hr = IMFMediaEventQueue_QueueEvent(queue, event);
        ok(hr == S_OK, "Failed to queue event, hr %#x.\n", hr);

        hr = IMFMediaEventQueue_GetEvent(queue, MF_EVENT_FLAG_NO_WAIT, &event2);
        ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
        ok(event2 == event, "Unexpected event object.\n");
        IMFMediaEvent_Release(event2);

        hr = IMFMediaEventQueue_QueueEvent(queue, event);
        ok(hr == S_OK, "Failed to queue event, hr %#x.\n", hr);

        hr = IMFMediaEventQueue_GetEvent(queue, 0, &event2);
        ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
        IMFMediaEvent_Release(event2);
    }

    /* Async case. */
    hr = IMFMediaEventQueue_BeginGetEvent(queue, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEventQueue_BeginGetEvent(queue, &callback.IMFAsyncCallback_iface, (IUnknown *)queue);
    ok(hr == S_OK, "Failed to Begin*, hr %#x.\n", hr);

    /* Same callback, same state. */
    hr = IMFMediaEventQueue_BeginGetEvent(queue, &callback.IMFAsyncCallback_iface, (IUnknown *)queue);
    ok(hr == MF_S_MULTIPLE_BEGIN, "Unexpected hr %#x.\n", hr);

    /* Same callback, different state. */
    hr = IMFMediaEventQueue_BeginGetEvent(queue, &callback.IMFAsyncCallback_iface, (IUnknown *)&callback);
    ok(hr == MF_E_MULTIPLE_BEGIN, "Unexpected hr %#x.\n", hr);

    /* Different callback, same state. */
    hr = IMFMediaEventQueue_BeginGetEvent(queue, &callback2.IMFAsyncCallback_iface, (IUnknown *)queue);
    ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Unexpected hr %#x.\n", hr);

    /* Different callback, different state. */
    hr = IMFMediaEventQueue_BeginGetEvent(queue, &callback2.IMFAsyncCallback_iface, (IUnknown *)&callback.IMFAsyncCallback_iface);
    ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Unexpected hr %#x.\n", hr);

    callback.event = CreateEventA(NULL, FALSE, FALSE, NULL);

    hr = IMFMediaEventQueue_QueueEvent(queue, event);
    ok(hr == S_OK, "Failed to queue event, hr %#x.\n", hr);

    ret = WaitForSingleObject(callback.event, 100);
    ok(ret == WAIT_OBJECT_0, "Unexpected return value %#x.\n", ret);

    CloseHandle(callback.event);

    IMFMediaEvent_Release(event);

    hr = MFCreateAsyncResult(NULL, &callback.IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#x.\n", hr);

    hr = IMFMediaEventQueue_EndGetEvent(queue, result, &event);
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);

    /* Shutdown behavior. */
    hr = IMFMediaEventQueue_Shutdown(queue);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    hr = IMFMediaEventQueue_GetEvent(queue, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMediaEvent(MEError, &GUID_NULL, E_FAIL, NULL, &event);
    ok(hr == S_OK, "Failed to create event object, hr %#x.\n", hr);
    hr = IMFMediaEventQueue_QueueEvent(queue, event);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);
    IMFMediaEvent_Release(event);

    hr = IMFMediaEventQueue_QueueEventParamUnk(queue, MEError, &GUID_NULL, E_FAIL, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEventQueue_QueueEventParamVar(queue, MEError, &GUID_NULL, E_FAIL, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEventQueue_BeginGetEvent(queue, &callback.IMFAsyncCallback_iface, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEventQueue_BeginGetEvent(queue, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaEventQueue_EndGetEvent(queue, result, &event);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);
    IMFAsyncResult_Release(result);

    /* Already shut down. */
    hr = IMFMediaEventQueue_Shutdown(queue);
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    IMFMediaEventQueue_Release(queue);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
}

START_TEST(mfplat)
{
    CoInitialize(NULL);

    init_functions();

    test_startup();
    test_register();
    test_MFCreateMediaType();
    test_MFCreateMediaEvent();
    test_MFCreateAttributes();
    test_MFSample();
    test_MFCreateFile();
    test_MFCreateMFByteStreamOnStream();
    test_MFCreateMemoryBuffer();
    test_source_resolver();
    test_MFCreateAsyncResult();
    test_allocate_queue();
    test_MFCopyImage();
    test_MFCreateCollection();
    test_MFHeapAlloc();
    test_scheduled_items();
    test_serial_queue();
    test_periodic_callback();
    test_event_queue();

    CoUninitialize();
}
