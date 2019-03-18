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
#include "ole2.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(DUMMY_CLSID, 0x12345678,0x1234,0x1234,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19);
DEFINE_GUID(DUMMY_GUID1, 0x12345678,0x1234,0x1234,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21);
DEFINE_GUID(DUMMY_GUID2, 0x12345678,0x1234,0x1234,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22);
DEFINE_GUID(DUMMY_GUID3, 0x12345678,0x1234,0x1234,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23);

#undef INITGUID
#include <guiddef.h>
#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "mfreadwrite.h"
#include "propvarutil.h"
#include "strsafe.h"

#include "wine/test.h"

static BOOL is_win8_plus;

static HRESULT (WINAPI *pMFCopyImage)(BYTE *dest, LONG deststride, const BYTE *src, LONG srcstride,
        DWORD width, DWORD lines);
static HRESULT (WINAPI *pMFCreateSourceResolver)(IMFSourceResolver **resolver);
static HRESULT (WINAPI *pMFCreateMFByteStreamOnStream)(IStream *stream, IMFByteStream **bytestream);
static void*   (WINAPI *pMFHeapAlloc)(SIZE_T size, ULONG flags, char *file, int line, EAllocationType type);
static void    (WINAPI *pMFHeapFree)(void *p);
static HRESULT (WINAPI *pMFPutWaitingWorkItem)(HANDLE event, LONG priority, IMFAsyncResult *result, MFWORKITEM_KEY *key);
static HRESULT (WINAPI *pMFAllocateSerialWorkQueue)(DWORD queue, DWORD *serial_queue);
static HRESULT (WINAPI *pMFAddPeriodicCallback)(MFPERIODICCALLBACK callback, IUnknown *context, DWORD *key);
static HRESULT (WINAPI *pMFRemovePeriodicCallback)(DWORD key);

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
    ok(hr == S_OK, "Failed to set string value, hr %#x.\n", hr);
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
    X(MFHeapAlloc);
    X(MFHeapFree);
    X(MFPutWaitingWorkItem);
    X(MFRemovePeriodicCallback);
#undef X

    is_win8_plus = pMFPutWaitingWorkItem != NULL;
}

static void test_media_type(void)
{
    IMFMediaType *mediatype, *mediatype2;
    BOOL compressed;
    DWORD flags;
    HRESULT hr;
    GUID guid;

if(0)
{
    /* Crash on Windows Vista/7 */
    hr = MFCreateMediaType(NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
}

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaType_GetMajorType(mediatype, &guid);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    compressed = FALSE;
    hr = IMFMediaType_IsCompressedFormat(mediatype, &compressed);
todo_wine
    ok(hr == S_OK, "Failed to get media type property, hr %#x.\n", hr);
    ok(compressed, "Unexpected value %d.\n", compressed);

    hr = IMFMediaType_SetUINT32(mediatype, &MF_MT_ALL_SAMPLES_INDEPENDENT, 0);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    compressed = FALSE;
    hr = IMFMediaType_IsCompressedFormat(mediatype, &compressed);
    ok(hr == S_OK, "Failed to get media type property, hr %#x.\n", hr);
    ok(compressed, "Unexpected value %d.\n", compressed);

    hr = IMFMediaType_SetUINT32(mediatype, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    compressed = TRUE;
    hr = IMFMediaType_IsCompressedFormat(mediatype, &compressed);
    ok(hr == S_OK, "Failed to get media type property, hr %#x.\n", hr);
    ok(!compressed, "Unexpected value %d.\n", compressed);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set GUID value, hr %#x.\n", hr);

    hr = IMFMediaType_GetMajorType(mediatype, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#x.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected major type.\n");

    /* IsEqual() */
    hr = MFCreateMediaType(&mediatype2);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    flags = 0xdeadbeef;
    hr = IMFMediaType_IsEqual(mediatype, mediatype2, &flags);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);
    ok(flags == 0, "Unexpected flags %#x.\n", flags);

    /* Different major types. */
    hr = IMFMediaType_SetGUID(mediatype2, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set major type, hr %#x.\n", hr);

    flags = 0;
    hr = IMFMediaType_IsEqual(mediatype, mediatype2, &flags);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);
    ok(flags == (MF_MEDIATYPE_EQUAL_FORMAT_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_USER_DATA),
            "Unexpected flags %#x.\n", flags);

    /* Same major types, different subtypes. */
    hr = IMFMediaType_SetGUID(mediatype2, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set major type, hr %#x.\n", hr);

    flags = 0;
    hr = IMFMediaType_IsEqual(mediatype, mediatype2, &flags);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(flags == (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_DATA
            | MF_MEDIATYPE_EQUAL_FORMAT_USER_DATA), "Unexpected flags %#x.\n", flags);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Failed to set subtype, hr %#x.\n", hr);

    flags = 0;
    hr = IMFMediaType_IsEqual(mediatype, mediatype2, &flags);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);
    ok(flags == (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_DATA | MF_MEDIATYPE_EQUAL_FORMAT_USER_DATA),
            "Unexpected flags %#x.\n", flags);

    IMFMediaType_Release(mediatype2);
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

#define CHECK_ATTR_COUNT(obj, expected) check_attr_count(obj, expected, __LINE__)
static void check_attr_count(IMFAttributes* obj, UINT32 expected, int line)
{
    UINT32 count = expected + 1;
    HRESULT hr = IMFAttributes_GetCount(obj, &count);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get attributes count, hr %#x.\n", hr);
    ok_(__FILE__, line)(count == expected, "Unexpected count %u, expected %u.\n", count, expected);
}

#define CHECK_ATTR_TYPE(obj, key, expected) check_attr_type(obj, key, expected, __LINE__)
static void check_attr_type(IMFAttributes *obj, const GUID *key, MF_ATTRIBUTE_TYPE expected, int line)
{
    MF_ATTRIBUTE_TYPE type;
    HRESULT hr;

    hr = IMFAttributes_GetItemType(obj, key, &type);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get item type, hr %#x.\n", hr);
    ok_(__FILE__, line)(type == expected, "Unexpected item type %d, expected %d.\n", type, expected);
}

static void test_attributes(void)
{
    static const WCHAR stringW[] = {'W','i','n','e',0};
    static const UINT8 blob[] = {0,1,2,3,4,5};
    IMFAttributes *attributes, *attributes1;
    UINT8 blob_value[256], *blob_buf = NULL;
    MF_ATTRIBUTES_MATCH_TYPE match_type;
    UINT32 value, string_length, size;
    PROPVARIANT propvar, ret_propvar;
    MF_ATTRIBUTE_TYPE type;
    double double_value;
    IUnknown *unk_value;
    WCHAR bufferW[256];
    UINT64 value64;
    WCHAR *string;
    BOOL result;
    HRESULT hr;
    GUID key;

    hr = MFCreateAttributes( &attributes, 3 );
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFAttributes_GetItemType(attributes, &GUID_NULL, &type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    CHECK_ATTR_COUNT(attributes, 0);
    hr = IMFAttributes_SetUINT32(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 123);
    ok(hr == S_OK, "Failed to set UINT32 value, hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes, 1);
    CHECK_ATTR_TYPE(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, MF_ATTRIBUTE_UINT32);

    value = 0xdeadbeef;
    hr = IMFAttributes_GetUINT32(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, &value);
    ok(hr == S_OK, "Failed to get UINT32 value, hr %#x.\n", hr);
    ok(value == 123, "Unexpected value %u, expected: 123.\n", value);

    value64 = 0xdeadbeef;
    hr = IMFAttributes_GetUINT64(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, &value64);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#x.\n", hr);
    ok(value64 == 0xdeadbeef, "Unexpected value.\n");

    hr = IMFAttributes_SetUINT64(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 65536);
    ok(hr == S_OK, "Failed to set UINT64 value, hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes, 1);
    CHECK_ATTR_TYPE(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, MF_ATTRIBUTE_UINT64);

    hr = IMFAttributes_GetUINT64(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, &value64);
    ok(hr == S_OK, "Failed to get UINT64 value, hr %#x.\n", hr);
    ok(value64 == 65536, "Unexpected value.\n");

    value = 0xdeadbeef;
    hr = IMFAttributes_GetUINT32(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, &value);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#x.\n", hr);
    ok(value == 0xdeadbeef, "Unexpected value.\n");

    IMFAttributes_Release(attributes);

    hr = MFCreateAttributes(&attributes, 0);
    ok(hr == S_OK, "Failed to create attributes object, hr %#x.\n", hr);

    PropVariantInit(&propvar);
    propvar.vt = MF_ATTRIBUTE_UINT32;
    U(propvar).ulVal = 123;
    hr = IMFAttributes_SetItem(attributes, &DUMMY_GUID1, &propvar);
    ok(hr == S_OK, "Failed to set item, hr %#x.\n", hr);
    PropVariantInit(&ret_propvar);
    ret_propvar.vt = MF_ATTRIBUTE_UINT32;
    U(ret_propvar).ulVal = 0xdeadbeef;
    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID1, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#x.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "Unexpected item value.\n");
    PropVariantClear(&ret_propvar);
    CHECK_ATTR_COUNT(attributes, 1);

    PropVariantInit(&ret_propvar);
    ret_propvar.vt = MF_ATTRIBUTE_STRING;
    U(ret_propvar).pwszVal = NULL;
    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID1, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#x.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "Unexpected item value.\n");
    PropVariantClear(&ret_propvar);

    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = MF_ATTRIBUTE_UINT64;
    U(propvar).uhVal.QuadPart = 65536;
    hr = IMFAttributes_SetItem(attributes, &DUMMY_GUID1, &propvar);
    ok(hr == S_OK, "Failed to set item, hr %#x.\n", hr);
    PropVariantInit(&ret_propvar);
    ret_propvar.vt = MF_ATTRIBUTE_UINT32;
    U(ret_propvar).ulVal = 0xdeadbeef;
    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID1, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#x.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "Unexpected item value.\n");
    PropVariantClear(&ret_propvar);
    PropVariantClear(&propvar);
    CHECK_ATTR_COUNT(attributes, 1);

    PropVariantInit(&propvar);
    propvar.vt = VT_I4;
    U(propvar).lVal = 123;
    hr = IMFAttributes_SetItem(attributes, &DUMMY_GUID2, &propvar);
    ok(hr == MF_E_INVALIDTYPE, "Failed to set item, hr %#x.\n", hr);
    PropVariantInit(&ret_propvar);
    ret_propvar.vt = MF_ATTRIBUTE_UINT32;
    U(ret_propvar).lVal = 0xdeadbeef;
    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID2, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#x.\n", hr);
    PropVariantClear(&propvar);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "Unexpected item value.\n");
    PropVariantClear(&ret_propvar);

    PropVariantInit(&propvar);
    propvar.vt = MF_ATTRIBUTE_UINT32;
    U(propvar).ulVal = 123;
    hr = IMFAttributes_SetItem(attributes, &DUMMY_GUID3, &propvar);
    ok(hr == S_OK, "Failed to set item, hr %#x.\n", hr);

    hr = IMFAttributes_DeleteItem(attributes, &DUMMY_GUID2);
    ok(hr == S_OK, "Failed to delete item, hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes, 2);

    hr = IMFAttributes_DeleteItem(attributes, &DUMMY_GUID2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes, 2);

    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID3, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#x.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "Unexpected item value.\n");
    PropVariantClear(&ret_propvar);
    PropVariantClear(&propvar);

    propvar.vt = MF_ATTRIBUTE_UINT64;
    U(propvar).uhVal.QuadPart = 65536;

    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID1, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#x.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "Unexpected item value.\n");
    PropVariantClear(&ret_propvar);
    PropVariantClear(&propvar);

    /* Item ordering is not consistent across Windows version. */
    hr = IMFAttributes_GetItemByIndex(attributes, 0, &key, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#x.\n", hr);
    PropVariantClear(&ret_propvar);

    hr = IMFAttributes_GetItemByIndex(attributes, 100, &key, &ret_propvar);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);
    PropVariantClear(&ret_propvar);

    hr = IMFAttributes_SetDouble(attributes, &GUID_NULL, 22.0);
    ok(hr == S_OK, "Failed to set double value, hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes, 3);
    CHECK_ATTR_TYPE(attributes, &GUID_NULL, MF_ATTRIBUTE_DOUBLE);

    double_value = 0xdeadbeef;
    hr = IMFAttributes_GetDouble(attributes, &GUID_NULL, &double_value);
    ok(hr == S_OK, "Failed to get double value, hr %#x.\n", hr);
    ok(double_value == 22.0, "Unexpected value: %f, expected: 22.0.\n", double_value);

    propvar.vt = MF_ATTRIBUTE_UINT64;
    U(propvar).uhVal.QuadPart = 22;
    hr = IMFAttributes_CompareItem(attributes, &GUID_NULL, &propvar, &result);
    ok(hr == S_OK, "Failed to compare items, hr %#x.\n", hr);
    ok(!result, "Unexpected result.\n");

    propvar.vt = MF_ATTRIBUTE_DOUBLE;
    U(propvar).dblVal = 22.0;
    hr = IMFAttributes_CompareItem(attributes, &GUID_NULL, &propvar, &result);
    ok(hr == S_OK, "Failed to compare items, hr %#x.\n", hr);
    ok(result, "Unexpected result.\n");

    hr = IMFAttributes_SetString(attributes, &DUMMY_GUID1, stringW);
    ok(hr == S_OK, "Failed to set string attribute, hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes, 3);
    CHECK_ATTR_TYPE(attributes, &DUMMY_GUID1, MF_ATTRIBUTE_STRING);

    hr = IMFAttributes_GetStringLength(attributes, &DUMMY_GUID1, &string_length);
    ok(hr == S_OK, "Failed to get string length, hr %#x.\n", hr);
    ok(string_length == lstrlenW(stringW), "Unexpected length %u.\n", string_length);

    string_length = 0xdeadbeef;
    hr = IMFAttributes_GetAllocatedString(attributes, &DUMMY_GUID1, &string, &string_length);
    ok(hr == S_OK, "Failed to get allocated string, hr %#x.\n", hr);
    ok(!lstrcmpW(string, stringW), "Unexpected string %s.\n", wine_dbgstr_w(string));
    ok(string_length == lstrlenW(stringW), "Unexpected length %u.\n", string_length);
    CoTaskMemFree(string);

    string_length = 0xdeadbeef;
    hr = IMFAttributes_GetString(attributes, &DUMMY_GUID1, bufferW, ARRAY_SIZE(bufferW), &string_length);
    ok(hr == S_OK, "Failed to get string value, hr %#x.\n", hr);
    ok(!lstrcmpW(bufferW, stringW), "Unexpected string %s.\n", wine_dbgstr_w(bufferW));
    ok(string_length == lstrlenW(stringW), "Unexpected length %u.\n", string_length);
    memset(bufferW, 0, sizeof(bufferW));

    hr = IMFAttributes_GetString(attributes, &DUMMY_GUID1, bufferW, ARRAY_SIZE(bufferW), NULL);
    ok(hr == S_OK, "Failed to get string value, hr %#x.\n", hr);
    ok(!lstrcmpW(bufferW, stringW), "Unexpected string %s.\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));

    string_length = 0;
    hr = IMFAttributes_GetString(attributes, &DUMMY_GUID1, bufferW, 1, &string_length);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "Unexpected hr %#x.\n", hr);
    ok(!bufferW[0], "Unexpected string %s.\n", wine_dbgstr_w(bufferW));
    ok(string_length, "Unexpected length.\n");

    string_length = 0xdeadbeef;
    hr = IMFAttributes_GetStringLength(attributes, &GUID_NULL, &string_length);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#x.\n", hr);
    ok(string_length == 0xdeadbeef, "Unexpected length %u.\n", string_length);

    /* VT_UNKNOWN */
    hr = IMFAttributes_SetUnknown(attributes, &DUMMY_GUID2, (IUnknown *)attributes);
    ok(hr == S_OK, "Failed to set value, hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes, 4);
    CHECK_ATTR_TYPE(attributes, &DUMMY_GUID2, MF_ATTRIBUTE_IUNKNOWN);

    hr = IMFAttributes_GetUnknown(attributes, &DUMMY_GUID2, &IID_IUnknown, (void **)&unk_value);
    ok(hr == S_OK, "Failed to get value, hr %#x.\n", hr);
    IUnknown_Release(unk_value);

    hr = IMFAttributes_GetUnknown(attributes, &DUMMY_GUID2, &IID_IMFAttributes, (void **)&unk_value);
    ok(hr == S_OK, "Failed to get value, hr %#x.\n", hr);
    IUnknown_Release(unk_value);

    hr = IMFAttributes_GetUnknown(attributes, &DUMMY_GUID2, &IID_IStream, (void **)&unk_value);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &DUMMY_CLSID, NULL);
    ok(hr == S_OK, "Failed to set value, hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes, 5);

    unk_value = NULL;
    hr = IMFAttributes_GetUnknown(attributes, &DUMMY_CLSID, &IID_IUnknown, (void **)&unk_value);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#x.\n", hr);

    /* CopyAllItems() */
    hr = MFCreateAttributes(&attributes1, 0);
    ok(hr == S_OK, "Failed to create attributes object, hr %#x.\n", hr);
    hr = IMFAttributes_CopyAllItems(attributes, attributes1);
    ok(hr == S_OK, "Failed to copy items, hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes, 5);
    CHECK_ATTR_COUNT(attributes1, 5);

    hr = IMFAttributes_DeleteAllItems(attributes1);
    ok(hr == S_OK, "Failed to delete items, hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes1, 0);

    propvar.vt = MF_ATTRIBUTE_UINT64;
    U(propvar).uhVal.QuadPart = 22;
    hr = IMFAttributes_CompareItem(attributes, &GUID_NULL, &propvar, &result);
    ok(hr == S_OK, "Failed to compare items, hr %#x.\n", hr);
    ok(!result, "Unexpected result.\n");

    hr = IMFAttributes_CopyAllItems(attributes1, attributes);
    ok(hr == S_OK, "Failed to copy items, hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes, 0);

    /* Blob */
    hr = IMFAttributes_SetBlob(attributes, &DUMMY_GUID1, blob, sizeof(blob));
    ok(hr == S_OK, "Failed to set blob attribute, hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes, 1);
    CHECK_ATTR_TYPE(attributes, &DUMMY_GUID1, MF_ATTRIBUTE_BLOB);
    hr = IMFAttributes_GetBlobSize(attributes, &DUMMY_GUID1, &size);
    ok(hr == S_OK, "Failed to get blob size, hr %#x.\n", hr);
    ok(size == sizeof(blob), "Unexpected blob size %u.\n", size);

    hr = IMFAttributes_GetBlobSize(attributes, &DUMMY_GUID2, &size);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    size = 0;
    hr = IMFAttributes_GetBlob(attributes, &DUMMY_GUID1, blob_value, sizeof(blob_value), &size);
    ok(hr == S_OK, "Failed to get blob, hr %#x.\n", hr);
    ok(size == sizeof(blob), "Unexpected blob size %u.\n", size);
    ok(!memcmp(blob_value, blob, size), "Unexpected blob.\n");

    hr = IMFAttributes_GetBlob(attributes, &DUMMY_GUID2, blob_value, sizeof(blob_value), &size);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    memset(blob_value, 0, sizeof(blob_value));
    size = 0;
    hr = IMFAttributes_GetAllocatedBlob(attributes, &DUMMY_GUID1, &blob_buf, &size);
    ok(hr == S_OK, "Failed to get allocated blob, hr %#x.\n", hr);
    ok(size == sizeof(blob), "Unexpected blob size %u.\n", size);
    ok(!memcmp(blob_buf, blob, size), "Unexpected blob.\n");
    CoTaskMemFree(blob_buf);

    hr = IMFAttributes_GetAllocatedBlob(attributes, &DUMMY_GUID2, &blob_buf, &size);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    hr = IMFAttributes_GetBlob(attributes, &DUMMY_GUID1, blob_value, sizeof(blob) - 1, NULL);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#x.\n", hr);

    IMFAttributes_Release(attributes);
    IMFAttributes_Release(attributes1);

    /* Compare() */
    hr = MFCreateAttributes(&attributes, 0);
    ok(hr == S_OK, "Failed to create attributes object, hr %#x.\n", hr);
    hr = MFCreateAttributes(&attributes1, 0);
    ok(hr == S_OK, "Failed to create attributes object, hr %#x.\n", hr);

    hr = IMFAttributes_Compare(attributes, attributes, MF_ATTRIBUTES_MATCH_SMALLER + 1, &result);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    for (match_type = MF_ATTRIBUTES_MATCH_OUR_ITEMS; match_type <= MF_ATTRIBUTES_MATCH_SMALLER; ++match_type)
    {
        result = FALSE;
        hr = IMFAttributes_Compare(attributes, attributes, match_type, &result);
        ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
        ok(result, "Unexpected result %d.\n", result);

        result = FALSE;
        hr = IMFAttributes_Compare(attributes, attributes1, match_type, &result);
        ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
        ok(result, "Unexpected result %d.\n", result);
    }

    hr = IMFAttributes_SetUINT32(attributes, &DUMMY_GUID1, 1);
    ok(hr == S_OK, "Failed to set value, hr %#x.\n", hr);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    hr = IMFAttributes_SetUINT32(attributes1, &DUMMY_GUID1, 2);
    ok(hr == S_OK, "Failed to set value, hr %#x.\n", hr);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_THEIR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_THEIR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    hr = IMFAttributes_SetUINT32(attributes1, &DUMMY_GUID1, 1);
    ok(hr == S_OK, "Failed to set value, hr %#x.\n", hr);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_THEIR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_THEIR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    hr = IMFAttributes_SetUINT32(attributes1, &DUMMY_GUID2, 2);
    ok(hr == S_OK, "Failed to set value, hr %#x.\n", hr);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_THEIR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_THEIR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#x.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    IMFAttributes_Release(attributes);
    IMFAttributes_Release(attributes1);
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
        win_skip("Cannot retrieve IMFAttributes interface from IMFByteStream\n");
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

static void test_system_memory_buffer(void)
{
    IMFMediaBuffer *buffer;
    HRESULT hr;
    DWORD length, max;
    BYTE *data, *data2;

    hr = MFCreateMemoryBuffer(1024, NULL);
    ok(hr == E_INVALIDARG || hr == E_POINTER, "got 0x%08x\n", hr);

    hr = MFCreateMemoryBuffer(0, &buffer);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    if(buffer)
    {
        hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(length == 0, "got %u\n", length);

        IMFMediaBuffer_Release(buffer);
    }

    hr = MFCreateMemoryBuffer(1024, &buffer);
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

    /* Aligned buffer. */
    hr = MFCreateAlignedMemoryBuffer(201, MF_8_BYTE_ALIGNMENT, &buffer);
    ok(hr == S_OK, "Failed to create memory buffer, hr %#x.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get current length, hr %#x.\n", hr);
    ok(length == 0, "Unexpected current length %u.\n", length);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 1);
    ok(hr == S_OK, "Failed to set current length, hr %#x.\n", hr);
    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get current length, hr %#x.\n", hr);
    ok(length == 1, "Unexpected current length %u.\n", length);

    hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
    ok(hr == S_OK, "Failed to get max length, hr %#x.\n", hr);
    ok(length == 201, "Unexpected max length %u.\n", length);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 202);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);
    hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
    ok(hr == S_OK, "Failed to get max length, hr %#x.\n", hr);
    ok(length == 201, "Unexpected max length %u.\n", length);
    hr = IMFMediaBuffer_SetCurrentLength(buffer, 10);
    ok(hr == S_OK, "Failed to set current length, hr %#x.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
    ok(hr == S_OK, "Failed to lock, hr %#x.\n", hr);
    ok(max == 201 && length == 10, "Unexpected length.\n");
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock, hr %#x.\n", hr);

    IMFMediaBuffer_Release(buffer);
}

static void test_sample(void)
{
    IMFMediaBuffer *buffer, *buffer2;
    DWORD count, flags, length;
    IMFAttributes *attributes;
    IMFSample *sample;
    LONGLONG time;
    HRESULT hr;

    hr = MFCreateSample( &sample );
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFSample_QueryInterface(sample, &IID_IMFAttributes, (void **)&attributes);
    ok(hr == S_OK, "Failed to get attributes interface, hr %#x.\n", hr);

    CHECK_ATTR_COUNT(attributes, 0);

    hr = IMFSample_GetBufferCount(sample, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 0, "got %d\n", count);

    hr = IMFSample_GetSampleFlags(sample, &flags);
    ok(hr == S_OK, "Failed to get sample flags, hr %#x.\n", hr);
    ok(!flags, "Unexpected flags %#x.\n", flags);

    hr = IMFSample_SetSampleFlags(sample, 0x123);
    ok(hr == S_OK, "Failed to set sample flags, hr %#x.\n", hr);
    hr = IMFSample_GetSampleFlags(sample, &flags);
    ok(hr == S_OK, "Failed to get sample flags, hr %#x.\n", hr);
    ok(flags == 0x123, "Unexpected flags %#x.\n", flags);

    hr = IMFSample_GetSampleTime(sample, &time);
    ok(hr == MF_E_NO_SAMPLE_TIMESTAMP, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetSampleDuration(sample, &time);
    ok(hr == MF_E_NO_SAMPLE_DURATION, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_RemoveBufferByIndex(sample, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_RemoveAllBuffers(sample);
    ok(hr == S_OK, "Failed to remove all, hr %#x.\n", hr);

    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "Failed to get total length, hr %#x.\n", hr);
    ok(!length, "Unexpected total length %u.\n", length);

    hr = MFCreateMemoryBuffer(16, &buffer);
    ok(hr == S_OK, "Failed to create buffer, hr %#x.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#x.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#x.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#x.\n", hr);
    ok(count == 2, "Unexpected buffer count %u.\n", count);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer2);
    ok(hr == S_OK, "Failed to get buffer, hr %#x.\n", hr);
    ok(buffer2 == buffer, "Unexpected object.\n");
    IMFMediaBuffer_Release(buffer2);

    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "Failed to get total length, hr %#x.\n", hr);
    ok(!length, "Unexpected total length %u.\n", length);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 2);
    ok(hr == S_OK, "Failed to set current length, hr %#x.\n", hr);

    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "Failed to get total length, hr %#x.\n", hr);
    ok(length == 4, "Unexpected total length %u.\n", length);

    hr = IMFSample_RemoveBufferByIndex(sample, 1);
    ok(hr == S_OK, "Failed to remove buffer, hr %#x.\n", hr);

    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "Failed to get total length, hr %#x.\n", hr);
    ok(length == 2, "Unexpected total length %u.\n", length);

    IMFMediaBuffer_Release(buffer);

    /* Duration */
    hr = IMFSample_SetSampleDuration(sample, 10);
    ok(hr == S_OK, "Failed to set duration, hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes, 0);
    hr = IMFSample_GetSampleDuration(sample, &time);
    ok(hr == S_OK, "Failed to get sample duration, hr %#x.\n", hr);
    ok(time == 10, "Unexpected duration.\n");

    /* Timestamp */
    hr = IMFSample_SetSampleTime(sample, 1);
    ok(hr == S_OK, "Failed to set timestamp, hr %#x.\n", hr);
    CHECK_ATTR_COUNT(attributes, 0);
    hr = IMFSample_GetSampleTime(sample, &time);
    ok(hr == S_OK, "Failed to get sample time, hr %#x.\n", hr);
    ok(time == 1, "Unexpected timestamp.\n");

    IMFAttributes_Release(attributes);
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

            hr = IMFMediaEventQueue_EndGetEvent(queue, result, &event);
            ok(hr == S_OK, "Failed to finalize GetEvent, hr %#x.\n", hr);

            hr = IMFMediaEventQueue_EndGetEvent(queue, result, &event);
            ok(hr == E_FAIL, "Unexpected result, hr %#x.\n", hr);

            IMFMediaEvent_Release(event);
        }

        hr = IMFAsyncResult_GetObject(result, &obj);
        ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

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
    HANDLE event;
    DWORD flags;
    HRESULT hr;
    BOOL ret;

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
    ok(!refcount, "Unexpected refcount %u.\n", refcount);
    refcount = IMFAsyncResult_Release(result);
    ok(!refcount, "Unexpected refcount %u.\n", refcount);

    /* Event handle is closed on release. */
    hr = MFCreateAsyncResult(NULL, NULL, NULL, &result);
    ok(hr == S_OK, "Failed to create object, hr %#x.\n", hr);

    data = (MFASYNCRESULT *)result;
    data->hEvent = event = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(data->hEvent != NULL, "Failed to create event.\n");
    ret = GetHandleInformation(event, &flags);
    ok(ret, "Failed to get handle info.\n");

    refcount = IMFAsyncResult_Release(result);
    ok(!refcount, "Unexpected refcount %u.\n", refcount);
    ret = GetHandleInformation(event, &flags);
    ok(!ret, "Expected handle to be closed.\n");

    hr = MFCreateAsyncResult(NULL, &callback.IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create object, hr %#x.\n", hr);

    data = (MFASYNCRESULT *)result;
    data->hEvent = event = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(data->hEvent != NULL, "Failed to create event.\n");
    ret = GetHandleInformation(event, &flags);
    ok(ret, "Failed to get handle info.\n");

    refcount = IMFAsyncResult_Release(result);
    ok(!refcount, "Unexpected refcount %u.\n", refcount);
    ret = GetHandleInformation(event, &flags);
    ok(!ret, "Expected handle to be closed.\n");
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

static void test_presentation_descriptor(void)
{
    IMFStreamDescriptor *stream_desc[2], *stream_desc2;
    IMFPresentationDescriptor *pd, *pd2;
    IMFMediaType *media_type;
    unsigned int i;
    BOOL selected;
    DWORD count;
    HRESULT hr;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(stream_desc); ++i)
    {
        hr = MFCreateStreamDescriptor(0, 1, &media_type, &stream_desc[i]);
        ok(hr == S_OK, "Failed to create descriptor, hr %#x.\n", hr);
    }

    hr = MFCreatePresentationDescriptor(ARRAY_SIZE(stream_desc), stream_desc, &pd);
    ok(hr == S_OK, "Failed to create presentation descriptor, hr %#x.\n", hr);

    hr = IMFPresentationDescriptor_GetStreamDescriptorCount(pd, &count);
    ok(count == ARRAY_SIZE(stream_desc), "Unexpected count %u.\n", count);

    for (i = 0; i < count; ++i)
    {
        hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, i, &selected, &stream_desc2);
        ok(hr == S_OK, "Failed to get stream descriptor, hr %#x.\n", hr);
        ok(!selected, "Unexpected selected state.\n");
        ok(stream_desc[i] == stream_desc2, "Unexpected object.\n");
        IMFStreamDescriptor_Release(stream_desc2);
    }

    hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, 10, &selected, &stream_desc2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationDescriptor_SelectStream(pd, 10);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationDescriptor_SelectStream(pd, 0);
    ok(hr == S_OK, "Failed to select a stream, hr %#x.\n", hr);

    hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, 0, &selected, &stream_desc2);
    ok(hr == S_OK, "Failed to get stream descriptor, hr %#x.\n", hr);
    ok(!!selected, "Unexpected selected state.\n");
    IMFStreamDescriptor_Release(stream_desc2);

    hr = IMFPresentationDescriptor_Clone(pd, &pd2);
    ok(hr == S_OK, "Failed to clone, hr %#x.\n", hr);

    hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd2, 0, &selected, &stream_desc2);
    ok(hr == S_OK, "Failed to get stream descriptor, hr %#x.\n", hr);
    ok(!!selected, "Unexpected selected state.\n");
    IMFStreamDescriptor_Release(stream_desc2);

    IMFPresentationDescriptor_Release(pd);

    for (i = 0; i < ARRAY_SIZE(stream_desc); ++i)
    {
        IMFStreamDescriptor_Release(stream_desc[i]);
    }

    /* Partially initialized array. */
    hr = MFCreateStreamDescriptor(0, 1, &media_type, &stream_desc[1]);
    ok(hr == S_OK, "Failed to create descriptor, hr %#x.\n", hr);
    stream_desc[0] = NULL;

    hr = MFCreatePresentationDescriptor(ARRAY_SIZE(stream_desc), stream_desc, &pd);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    IMFStreamDescriptor_Release(stream_desc[1]);
    IMFMediaType_Release(media_type);
}

enum clock_action
{
    CLOCK_START,
    CLOCK_STOP,
    CLOCK_PAUSE,
    CLOCK_RESTART,
};

static void test_system_time_source(void)
{
    static const struct clock_state_test
    {
        enum clock_action action;
        MFCLOCK_STATE state;
        BOOL is_invalid;
    }
    clock_state_change[] =
    {
        { CLOCK_STOP, MFCLOCK_STATE_INVALID },
        { CLOCK_RESTART, MFCLOCK_STATE_INVALID, TRUE },
        { CLOCK_PAUSE, MFCLOCK_STATE_PAUSED },
        { CLOCK_PAUSE, MFCLOCK_STATE_PAUSED, TRUE },
        { CLOCK_STOP, MFCLOCK_STATE_STOPPED },
        { CLOCK_STOP, MFCLOCK_STATE_STOPPED },
        { CLOCK_RESTART, MFCLOCK_STATE_STOPPED, TRUE },
        { CLOCK_START, MFCLOCK_STATE_RUNNING },
        { CLOCK_START, MFCLOCK_STATE_RUNNING },
        { CLOCK_RESTART, MFCLOCK_STATE_RUNNING, TRUE },
        { CLOCK_PAUSE, MFCLOCK_STATE_PAUSED },
        { CLOCK_START, MFCLOCK_STATE_RUNNING },
        { CLOCK_PAUSE, MFCLOCK_STATE_PAUSED },
        { CLOCK_RESTART, MFCLOCK_STATE_RUNNING },
        { CLOCK_RESTART, MFCLOCK_STATE_RUNNING, TRUE },
        { CLOCK_STOP, MFCLOCK_STATE_STOPPED },
        { CLOCK_PAUSE, MFCLOCK_STATE_STOPPED, TRUE },
    };
    IMFPresentationTimeSource *time_source;
    IMFClockStateSink *statesink;
    MFCLOCK_STATE state;
    unsigned int i;
    DWORD value;
    HRESULT hr;

    hr = MFCreateSystemTimeSource(&time_source);
    ok(hr == S_OK, "Failed to create time source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetClockCharacteristics(time_source, &value);
    ok(hr == S_OK, "Failed to get flags, hr %#x.\n", hr);
    ok(value == (MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ | MFCLOCK_CHARACTERISTICS_FLAG_IS_SYSTEM_CLOCK),
            "Unexpected flags %#x.\n", value);

    value = 1;
    hr = IMFPresentationTimeSource_GetContinuityKey(time_source, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(value == 0, "Unexpected value %u.\n", value);

    hr = IMFPresentationTimeSource_GetState(time_source, 0, &state);
    ok(hr == S_OK, "Failed to get state, hr %#x.\n", hr);
    ok(state == MFCLOCK_STATE_INVALID, "Unexpected state %d.\n", state);

    hr = IMFPresentationTimeSource_QueryInterface(time_source, &IID_IMFClockStateSink, (void **)&statesink);
    ok(hr == S_OK, "Failed to get state sink, hr %#x.\n", hr);

    /* State changes. */
    for (i = 0; i < ARRAY_SIZE(clock_state_change); ++i)
    {
        switch (clock_state_change[i].action)
        {
            case CLOCK_STOP:
                hr = IMFClockStateSink_OnClockStop(statesink, 0);
                break;
            case CLOCK_RESTART:
                hr = IMFClockStateSink_OnClockRestart(statesink, 0);
                break;
            case CLOCK_PAUSE:
                hr = IMFClockStateSink_OnClockPause(statesink, 0);
                break;
            case CLOCK_START:
                hr = IMFClockStateSink_OnClockStart(statesink, 0, 0);
                break;
            default:
                ;
        }
        ok(hr == (clock_state_change[i].is_invalid ? MF_E_INVALIDREQUEST : S_OK), "%u: unexpected hr %#x.\n", i, hr);
        hr = IMFPresentationTimeSource_GetState(time_source, 0, &state);
        ok(hr == S_OK, "%u: failed to get state, hr %#x.\n", i, hr);
        ok(state == clock_state_change[i].state, "%u: unexpected state %d.\n", i, state);
    }

    IMFClockStateSink_Release(statesink);

    IMFPresentationTimeSource_Release(time_source);
}

static void test_MFInvokeCallback(void)
{
    struct test_callback callback;
    IMFAsyncResult *result;
    MFASYNCRESULT *data;
    ULONG refcount;
    HRESULT hr;
    DWORD ret;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#x.\n", hr);

    init_test_callback(&callback);

    hr = MFCreateAsyncResult(NULL, &callback.IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create object, hr %#x.\n", hr);

    data = (MFASYNCRESULT *)result;
    data->hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(data->hEvent != NULL, "Failed to create event.\n");

    hr = MFInvokeCallback(result);
    ok(hr == S_OK, "Failed to invoke, hr %#x.\n", hr);

    ret = WaitForSingleObject(data->hEvent, 100);
    ok(ret == WAIT_TIMEOUT, "Expected timeout, ret %#x.\n", ret);

    refcount = IMFAsyncResult_Release(result);
    ok(!refcount, "Unexpected refcount %u.\n", refcount);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
}

START_TEST(mfplat)
{
    CoInitialize(NULL);

    init_functions();

    test_startup();
    test_register();
    test_media_type();
    test_MFCreateMediaEvent();
    test_attributes();
    test_sample();
    test_MFCreateFile();
    test_MFCreateMFByteStreamOnStream();
    test_system_memory_buffer();
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
    test_presentation_descriptor();
    test_system_time_source();
    test_MFInvokeCallback();

    CoUninitialize();
}
