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
#include "ole2.h"
#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "mfreadwrite.h"
#include "propvarutil.h"
#include "strsafe.h"
#include "evr.h"

#include "wine/test.h"
#include "wine/heap.h"

#define D3D11_INIT_GUID
#include "initguid.h"
#include "d3d11_4.h"
#include "d3d9.h"
#include "d3d9types.h"
#include "ks.h"
#include "ksmedia.h"

DEFINE_GUID(DUMMY_CLSID, 0x12345678,0x1234,0x1234,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19);
DEFINE_GUID(DUMMY_GUID1, 0x12345678,0x1234,0x1234,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21);
DEFINE_GUID(DUMMY_GUID2, 0x12345678,0x1234,0x1234,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22);
DEFINE_GUID(DUMMY_GUID3, 0x12345678,0x1234,0x1234,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23);

extern const CLSID CLSID_FileSchemePlugin;

DEFINE_MEDIATYPE_GUID(MFVideoFormat_IMC1, MAKEFOURCC('I','M','C','1'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_IMC2, MAKEFOURCC('I','M','C','2'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_IMC3, MAKEFOURCC('I','M','C','3'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_IMC4, MAKEFOURCC('I','M','C','4'));

static BOOL is_win8_plus;

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown *obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "Unexpected refcount %d, expected %d.\n", rc, ref);
}

static HRESULT (WINAPI *pD3D11CreateDevice)(IDXGIAdapter *adapter, D3D_DRIVER_TYPE driver_type, HMODULE swrast, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT levels, UINT sdk_version, ID3D11Device **device_out,
        D3D_FEATURE_LEVEL *obtained_feature_level, ID3D11DeviceContext **immediate_context);

static HRESULT (WINAPI *pCoGetApartmentType)(APTTYPE *type, APTTYPEQUALIFIER *qualifier);

static HRESULT (WINAPI *pMFCopyImage)(BYTE *dest, LONG deststride, const BYTE *src, LONG srcstride,
        DWORD width, DWORD lines);
static HRESULT (WINAPI *pMFCreateDXGIDeviceManager)(UINT *token, IMFDXGIDeviceManager **manager);
static HRESULT (WINAPI *pMFCreateSourceResolver)(IMFSourceResolver **resolver);
static HRESULT (WINAPI *pMFCreateMFByteStreamOnStream)(IStream *stream, IMFByteStream **bytestream);
static HRESULT (WINAPI *pMFPutWaitingWorkItem)(HANDLE event, LONG priority, IMFAsyncResult *result, MFWORKITEM_KEY *key);
static HRESULT (WINAPI *pMFAllocateSerialWorkQueue)(DWORD queue, DWORD *serial_queue);
static HRESULT (WINAPI *pMFAddPeriodicCallback)(MFPERIODICCALLBACK callback, IUnknown *context, DWORD *key);
static HRESULT (WINAPI *pMFRemovePeriodicCallback)(DWORD key);
static HRESULT (WINAPI *pMFRegisterLocalByteStreamHandler)(const WCHAR *extension, const WCHAR *mime,
        IMFActivate *activate);
static HRESULT (WINAPI *pMFRegisterLocalSchemeHandler)(const WCHAR *scheme, IMFActivate *activate);
static HRESULT (WINAPI *pMFCreateTransformActivate)(IMFActivate **activate);
static HRESULT (WINAPI *pMFTRegisterLocal)(IClassFactory *factory, REFGUID category, LPCWSTR name,
        UINT32 flags, UINT32 cinput, const MFT_REGISTER_TYPE_INFO *input_types, UINT32 coutput,
        const MFT_REGISTER_TYPE_INFO* output_types);
static HRESULT (WINAPI *pMFTRegisterLocalByCLSID)(REFCLSID clsid, REFGUID category, LPCWSTR name, UINT32 flags,
        UINT32 input_count, const MFT_REGISTER_TYPE_INFO *input_types, UINT32 output_count,
        const MFT_REGISTER_TYPE_INFO *output_types);
static HRESULT (WINAPI *pMFTUnregisterLocal)(IClassFactory *factory);
static HRESULT (WINAPI *pMFTUnregisterLocalByCLSID)(CLSID clsid);
static HRESULT (WINAPI *pMFAllocateWorkQueueEx)(MFASYNC_WORKQUEUE_TYPE queue_type, DWORD *queue);
static HRESULT (WINAPI *pMFTEnumEx)(GUID category, UINT32 flags, const MFT_REGISTER_TYPE_INFO *input_type,
        const MFT_REGISTER_TYPE_INFO *output_type, IMFActivate ***activate, UINT32 *count);
static HRESULT (WINAPI *pMFGetPlaneSize)(DWORD format, DWORD width, DWORD height, DWORD *size);
static HRESULT (WINAPI *pMFGetStrideForBitmapInfoHeader)(DWORD format, DWORD width, LONG *stride);
static HRESULT (WINAPI *pMFCreate2DMediaBuffer)(DWORD width, DWORD height, DWORD fourcc, BOOL bottom_up,
        IMFMediaBuffer **buffer);
static HRESULT (WINAPI *pMFCreateMediaBufferFromMediaType)(IMFMediaType *media_type, LONGLONG duration, DWORD min_length,
        DWORD min_alignment, IMFMediaBuffer **buffer);
static HRESULT (WINAPI *pMFCreateDXSurfaceBuffer)(REFIID riid, IUnknown *surface, BOOL bottom_up, IMFMediaBuffer **buffer);
static HRESULT (WINAPI *pMFCreateTrackedSample)(IMFTrackedSample **sample);

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

static IDirect3DDevice9 *create_device(IDirect3D9 *d3d9, HWND focus_window)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9 *device = NULL;

    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = focus_window;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
    present_parameters.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

    IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device);

    return device;
}

static const WCHAR fileschemeW[] = L"file://";

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

struct test_callback
{
    IMFAsyncCallback IMFAsyncCallback_iface;
    HANDLE event;
    DWORD param;
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
    WCHAR name[] = L"Wine test";
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
    ok(count == 0, "Expected count == 0\n");

    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, NULL, NULL, &clsids, NULL);
    ok(ret == E_POINTER, "Failed to enumerate filters: %x\n", ret);
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

static HRESULT WINAPI test_create_from_url_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    IMFSourceResolver *resolver;
    IUnknown *object, *object2;
    MF_OBJECT_TYPE obj_type;
    HRESULT hr;

    ok(!!result, "Unexpected result object.\n");

    resolver = (IMFSourceResolver *)IMFAsyncResult_GetStateNoAddRef(result);

    object = NULL;
    hr = IMFSourceResolver_EndCreateObjectFromURL(resolver, result, &obj_type, &object);
todo_wine
    ok(hr == S_OK, "Failed to create an object, hr %#x.\n", hr);

    hr = IMFAsyncResult_GetObject(result, &object2);
    ok(hr == S_OK, "Failed to get result object, hr %#x.\n", hr);
todo_wine
    ok(object2 == object, "Unexpected object.\n");

    if (object)
        IUnknown_Release(object);
    IUnknown_Release(object2);

    SetEvent(callback->event);

    return S_OK;
}

static const IMFAsyncCallbackVtbl test_create_from_url_callback_vtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    test_create_from_url_callback_Invoke,
};

static HRESULT WINAPI test_create_from_file_handler_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    IMFSchemeHandler *handler;
    IUnknown *object, *object2;
    MF_OBJECT_TYPE obj_type;
    HRESULT hr;

    ok(!!result, "Unexpected result object.\n");

    handler = (IMFSchemeHandler *)IMFAsyncResult_GetStateNoAddRef(result);

    hr = IMFSchemeHandler_EndCreateObject(handler, result, &obj_type, &object);
    ok(hr == S_OK, "Failed to create an object, hr %#x.\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IMFAsyncResult_GetObject(result, &object2);
        ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

        IUnknown_Release(object);
    }

    SetEvent(callback->event);

    return S_OK;
}

static const IMFAsyncCallbackVtbl test_create_from_file_handler_callback_vtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    test_create_from_file_handler_callback_Invoke,
};

static BOOL get_event(IMFMediaEventGenerator *generator, MediaEventType expected_event_type, PROPVARIANT *value)
{
    MediaEventType event_type;
    HRESULT hr, event_status;
    IMFMediaEvent *event;

    hr = IMFMediaEventGenerator_GetEvent(generator, 0, &event);
    ok(hr == S_OK, "Failed to get event, hr %#x.\n", hr);

    hr = IMFMediaEvent_GetStatus(event, &event_status);
    ok(hr == S_OK, "Failed to get status code, hr %#x.\n", hr);
    ok(event_status == S_OK, "Unexpected event status code %#x.\n", event_status);

    hr = IMFMediaEvent_GetType(event, &event_type);
    ok(hr == S_OK, "Failed to event type, hr %#x.\n", hr);
    ok(event_type == expected_event_type, "Unexpected event type %u, expected %u.\n", event_type, expected_event_type);

    if (event_type != expected_event_type)
    {
        IMFMediaEvent_Release(event);
        return FALSE;
    }

    if (value)
    {
        hr = IMFMediaEvent_GetValue(event, value);
        ok(hr == S_OK, "Failed to get value of event, hr %#x.\n", hr);
    }

    IMFMediaEvent_Release(event);

    return TRUE;
}

static void test_source_resolver(void)
{
    struct test_callback callback = { { &test_create_from_url_callback_vtbl } };
    struct test_callback callback2 = { { &test_create_from_file_handler_callback_vtbl } };
    IMFSourceResolver *resolver, *resolver2;
    IMFPresentationDescriptor *descriptor;
    IMFSchemeHandler *scheme_handler;
    IMFMediaStream *video_stream;
    IMFAttributes *attributes;
    IMFMediaSource *mediasource;
    IMFMediaTypeHandler *handler;
    IMFMediaType *media_type;
    BOOL selected, do_uninit;
    MF_OBJECT_TYPE obj_type;
    IMFStreamDescriptor *sd;
    IUnknown *cancel_cookie;
    IMFByteStream *stream;
    WCHAR pathW[MAX_PATH];
    int i, sample_count;
    WCHAR *filename;
    PROPVARIANT var;
    HRESULT hr;
    GUID guid;

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

    filename = load_resource(L"test.mp4");

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, NULL, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
        &obj_type, (IUnknown **)&mediasource);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
            NULL, (IUnknown **)&mediasource);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
            &obj_type, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
            &obj_type, (IUnknown **)&mediasource);
    ok(hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE, "got 0x%08x\n", hr);
    if (hr == S_OK) IMFMediaSource_Release(mediasource);

    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_BYTESTREAM, NULL,
            &obj_type, (IUnknown **)&mediasource);
    ok(hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE, "got 0x%08x\n", hr);

    IMFByteStream_Release(stream);

    /* Create from URL. */
    callback.event = CreateEventA(NULL, FALSE, FALSE, NULL);

    hr = IMFSourceResolver_CreateObjectFromURL(resolver, L"nonexisting.mp4", MF_RESOLUTION_BYTESTREAM, NULL, &obj_type,
            (IUnknown **)&stream);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Unexpected hr %#x.\n", hr);

    hr = IMFSourceResolver_CreateObjectFromURL(resolver, filename, MF_RESOLUTION_BYTESTREAM, NULL, &obj_type,
            (IUnknown **)&stream);
    ok(hr == S_OK, "Failed to resolve url, hr %#x.\n", hr);
    IMFByteStream_Release(stream);

    hr = IMFSourceResolver_BeginCreateObjectFromURL(resolver, filename, MF_RESOLUTION_BYTESTREAM, NULL,
            &cancel_cookie, &callback.IMFAsyncCallback_iface, (IUnknown *)resolver);
    ok(hr == S_OK, "Create request failed, hr %#x.\n", hr);
    ok(cancel_cookie != NULL, "Unexpected cancel object.\n");
    IUnknown_Release(cancel_cookie);

    if (SUCCEEDED(hr))
        WaitForSingleObject(callback.event, INFINITE);

    /* With explicit scheme. */
    lstrcpyW(pathW, fileschemeW);
    lstrcatW(pathW, filename);

    hr = IMFSourceResolver_CreateObjectFromURL(resolver, pathW, MF_RESOLUTION_BYTESTREAM, NULL, &obj_type,
            (IUnknown **)&stream);
    ok(hr == S_OK, "Failed to resolve url, hr %#x.\n", hr);
    IMFByteStream_Release(stream);

    /* We have to create a new bytestream here, because all following
     * calls to CreateObjectFromByteStream will fail. */
    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFByteStream_QueryInterface(stream, &IID_IMFAttributes, (void **)&attributes);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IMFAttributes_SetString(attributes, &MF_BYTESTREAM_CONTENT_TYPE, L"video/mp4");
    ok(hr == S_OK, "Failed to set string value, hr %#x.\n", hr);
    IMFAttributes_Release(attributes);

    /* Start of gstreamer dependent tests */

    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
            &obj_type, (IUnknown **)&mediasource);
    if (strcmp(winetest_platform, "wine"))
        ok(hr == S_OK, "got 0x%08x\n", hr);
    if (FAILED(hr))
    {
        IMFByteStream_Release(stream);
        IMFSourceResolver_Release(resolver);

        hr = MFShutdown();
        ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

        DeleteFileW(filename);
        return;
    }
    ok(mediasource != NULL, "got %p\n", mediasource);
    ok(obj_type == MF_OBJECT_MEDIASOURCE, "got %d\n", obj_type);

    hr = IMFMediaSource_CreatePresentationDescriptor(mediasource, &descriptor);
todo_wine
    ok(hr == S_OK, "Failed to get presentation descriptor, hr %#x.\n", hr);
    if (FAILED(hr))
        goto skip_source_tests;
    ok(descriptor != NULL, "got %p\n", descriptor);

    hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(descriptor, 0, &selected, &sd);
    ok(hr == S_OK, "Failed to get stream descriptor, hr %#x.\n", hr);

    hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, &handler);
    ok(hr == S_OK, "Failed to get type handler, hr %#x.\n", hr);
    IMFStreamDescriptor_Release(sd);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == S_OK, "Failed to get stream major type, hr %#x.\n", hr);

    /* Check major/minor type for the test media. */
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected major type %s.\n", debugstr_guid(&guid));

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type);
    ok(hr == S_OK, "Failed to get current media type, hr %#x.\n", hr);
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Failed to get media sub type, hr %#x.\n", hr);
    ok(IsEqualGUID(&guid, &MFVideoFormat_M4S2), "Unexpected sub type %s.\n", debugstr_guid(&guid));
    IMFMediaType_Release(media_type);

    hr = IMFPresentationDescriptor_SelectStream(descriptor, 0);
    ok(hr == S_OK, "Failed to select video stream, hr %#x.\n", hr);

    var.vt = VT_EMPTY;
    hr = IMFMediaSource_Start(mediasource, descriptor, &GUID_NULL, &var);
    ok(hr == S_OK, "Failed to start media source, hr %#x.\n", hr);

    get_event((IMFMediaEventGenerator *)mediasource, MENewStream, &var);
    ok(var.vt == VT_UNKNOWN, "Unexpected value type %u from MENewStream event.\n", var.vt);
    video_stream = (IMFMediaStream *)var.punkVal;

    get_event((IMFMediaEventGenerator *)mediasource, MESourceStarted, NULL);

    /* Request samples, our file is 10 frames at 25fps */
    get_event((IMFMediaEventGenerator *)video_stream, MEStreamStarted, NULL);
    sample_count = 10;

    /* Request one beyond EOS, otherwise EndOfStream isn't queued. */
    for (i = 0; i <= sample_count; ++i)
    {
        hr = IMFMediaStream_RequestSample(video_stream, NULL);
        if (i == sample_count)
            break;
        ok(hr == S_OK, "Failed to request sample %u, hr %#x.\n", i + 1, hr);
        if (hr != S_OK)
            break;
    }

    for (i = 0; i < sample_count; ++i)
    {
        static const LONGLONG MILLI_TO_100_NANO = 10000;
        LONGLONG duration, time;
        DWORD buffer_count;
        IMFSample *sample;
        BOOL ret;

        ret = get_event((IMFMediaEventGenerator *)video_stream, MEMediaSample, &var);
        ok(ret, "Sample %u not received.\n", i + 1);
        if (!ret)
            break;

        ok(var.vt == VT_UNKNOWN, "Unexpected value type %u from MEMediaSample event.\n", var.vt);
        sample = (IMFSample *)var.punkVal;

        hr = IMFSample_GetBufferCount(sample, &buffer_count);
        ok(hr == S_OK, "Failed to get buffer count, hr %#x.\n", hr);
        ok(buffer_count == 1, "Unexpected buffer count %u.\n", buffer_count);

        hr = IMFSample_GetSampleDuration(sample, &duration);
        ok(hr == S_OK, "Failed to get sample duration, hr %#x.\n", hr);
        ok(duration == 40 * MILLI_TO_100_NANO, "Unexpected duration %s.\n", wine_dbgstr_longlong(duration));

        hr = IMFSample_GetSampleTime(sample, &time);
        ok(hr == S_OK, "Failed to get sample time, hr %#x.\n", hr);
        ok(time == i * 40 * MILLI_TO_100_NANO, "Unexpected time %s.\n", wine_dbgstr_longlong(time));

        IMFSample_Release(sample);
    }

    if (i == sample_count)
        get_event((IMFMediaEventGenerator *)video_stream, MEEndOfStream, NULL);

    hr = IMFMediaStream_RequestSample(video_stream, NULL);
    ok(hr == MF_E_END_OF_STREAM, "Unexpected hr %#x.\n", hr);
    IMFMediaStream_Release(video_stream);

    get_event((IMFMediaEventGenerator *)mediasource, MEEndOfPresentation, NULL);

    IMFMediaTypeHandler_Release(handler);
    IMFPresentationDescriptor_Release(descriptor);

skip_source_tests:

    hr = IMFMediaSource_Shutdown(mediasource);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaSource_CreatePresentationDescriptor(mediasource, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#x.\n", hr);

    IMFMediaSource_Release(mediasource);
    IMFByteStream_Release(stream);

    /* Create directly through scheme handler. */
    hr = CoInitialize(NULL);
    ok(SUCCEEDED(hr), "Failed to initialize, hr %#x.\n", hr);
    do_uninit = hr == S_OK;

    hr = CoCreateInstance(&CLSID_FileSchemePlugin, NULL, CLSCTX_INPROC_SERVER, &IID_IMFSchemeHandler,
            (void **)&scheme_handler);
    ok(hr == S_OK, "Failed to create handler object, hr %#x.\n", hr);

    callback2.event = callback.event;
    cancel_cookie = NULL;
    hr = IMFSchemeHandler_BeginCreateObject(scheme_handler, pathW, MF_RESOLUTION_MEDIASOURCE, NULL, &cancel_cookie,
            &callback2.IMFAsyncCallback_iface, (IUnknown *)scheme_handler);
    ok(hr == S_OK, "Create request failed, hr %#x.\n", hr);
    ok(!!cancel_cookie, "Unexpected cancel object.\n");
    IUnknown_Release(cancel_cookie);

    WaitForSingleObject(callback2.event, INFINITE);

    IMFSchemeHandler_Release(scheme_handler);

    if (do_uninit)
        CoUninitialize();

    CloseHandle(callback.event);

    IMFSourceResolver_Release(resolver);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    DeleteFileW(filename);
}

static void init_functions(void)
{
    HMODULE mod = GetModuleHandleA("mfplat.dll");

#define X(f) p##f = (void*)GetProcAddress(mod, #f)
    X(MFAddPeriodicCallback);
    X(MFAllocateSerialWorkQueue);
    X(MFAllocateWorkQueueEx);
    X(MFCopyImage);
    X(MFCreate2DMediaBuffer);
    X(MFCreateDXGIDeviceManager);
    X(MFCreateDXSurfaceBuffer);
    X(MFCreateSourceResolver);
    X(MFCreateMediaBufferFromMediaType);
    X(MFCreateMFByteStreamOnStream);
    X(MFCreateTrackedSample);
    X(MFCreateTransformActivate);
    X(MFGetPlaneSize);
    X(MFGetStrideForBitmapInfoHeader);
    X(MFPutWaitingWorkItem);
    X(MFRegisterLocalByteStreamHandler);
    X(MFRegisterLocalSchemeHandler);
    X(MFRemovePeriodicCallback);
    X(MFTEnumEx);
    X(MFTRegisterLocal);
    X(MFTRegisterLocalByCLSID);
    X(MFTUnregisterLocal);
    X(MFTUnregisterLocalByCLSID);

    if ((mod = LoadLibraryA("d3d11.dll")))
    {
        X(D3D11CreateDevice);
    }

    mod = GetModuleHandleA("ole32.dll");

    X(CoGetApartmentType);
#undef X

    is_win8_plus = pMFPutWaitingWorkItem != NULL;
}

static void test_media_type(void)
{
    IMFMediaType *mediatype, *mediatype2;
    IMFVideoMediaType *video_type;
    IUnknown *unk, *unk2;
    DWORD count, flags;
    BOOL compressed;
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

    /* Different user data. */
    hr = IMFMediaType_SetBlob(mediatype, &MF_MT_USER_DATA, (const UINT8 *)&flags, sizeof(flags));
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    flags = 0;
    hr = IMFMediaType_IsEqual(mediatype, mediatype2, &flags);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);
    ok(flags == (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_DATA),
            "Unexpected flags %#x.\n", flags);

    hr = IMFMediaType_DeleteItem(mediatype, &MF_MT_USER_DATA);
    ok(hr == S_OK, "Failed to delete item, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Failed to set subtype, hr %#x.\n", hr);

    flags = 0;
    hr = IMFMediaType_IsEqual(mediatype, mediatype2, &flags);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);
    ok(flags == (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_DATA | MF_MEDIATYPE_EQUAL_FORMAT_USER_DATA),
            "Unexpected flags %#x.\n", flags);

    IMFMediaType_Release(mediatype2);
    IMFMediaType_Release(mediatype);

    /* IMFVideoMediaType */
    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_QueryInterface(mediatype, &IID_IMFVideoMediaType, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);
    hr = IMFMediaType_QueryInterface(mediatype, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unk == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set GUID value, hr %#x.\n", hr);

    hr = IMFMediaType_QueryInterface(mediatype, &IID_IMFVideoMediaType, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IMFAttributes, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IMFMediaType, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk2);

    IUnknown_Release(unk);

    hr = MFCreateVideoMediaTypeFromSubtype(&MFVideoFormat_RGB555, &video_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFVideoMediaType_QueryInterface(video_type, &IID_IMFMediaType, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);

    hr = IMFVideoMediaType_QueryInterface(video_type, &IID_IMFVideoMediaType, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IUnknown_Release(unk);

    /* Major and subtype are set on creation. */
    hr = IMFVideoMediaType_GetCount(video_type, &count);
    ok(count == 2, "Unexpected attribute count %#x.\n", hr);

    hr = IMFVideoMediaType_DeleteAllItems(video_type);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFVideoMediaType_GetCount(video_type, &count);
    ok(!count, "Unexpected attribute count %#x.\n", hr);

    hr = IMFVideoMediaType_QueryInterface(video_type, &IID_IMFVideoMediaType, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    IMFVideoMediaType_Release(video_type);

    IMFMediaType_Release(mediatype);

    /* IMFAudioMediaType */
    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_QueryInterface(mediatype, &IID_IMFAudioMediaType, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);
    hr = IMFMediaType_QueryInterface(mediatype, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unk == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set GUID value, hr %#x.\n", hr);

    hr = IMFMediaType_QueryInterface(mediatype, &IID_IMFAudioMediaType, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IMFAttributes, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IMFMediaType, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk2);

    IUnknown_Release(unk);

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
    static const WCHAR stringW[] = L"Wine";
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

    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID1, NULL);
    ok(hr == S_OK, "Item check failed, hr %#x.\n", hr);

    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID2, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

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
    DWORD caps, written, count;
    IUnknown *unknown;
    ULONG ref, size;
    HRESULT hr;

    if(!pMFCreateMFByteStreamOnStream)
    {
        win_skip("MFCreateMFByteStreamOnStream() not found\n");
        return;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    caps = 0xffff0000;
    hr = IStream_Write(stream, &caps, sizeof(caps), &written);
    ok(hr == S_OK, "Failed to write, hr %#x.\n", hr);

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
    hr = IMFAttributes_GetCount(attributes, &count);
    ok(hr == S_OK, "Failed to get attributes count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected attributes count %u.\n", count);

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

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFByteStreamBuffering, (void **)&unknown);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFByteStreamCacheControl, (void **)&unknown);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFMediaEventGenerator, (void **)&unknown);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFGetService, (void **)&unknown);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IMFByteStream_GetCapabilities(bytestream, &caps);
    ok(hr == S_OK, "Failed to get stream capabilities, hr %#x.\n", hr);
    ok(caps == (MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE), "Unexpected caps %#x.\n", caps);

    hr = IMFByteStream_Close(bytestream);
    ok(hr == S_OK, "Failed to close, hr %#x.\n", hr);

    hr = IMFByteStream_Close(bytestream);
    ok(hr == S_OK, "Failed to close, hr %#x.\n", hr);

    hr = IMFByteStream_GetCapabilities(bytestream, &caps);
    ok(hr == S_OK, "Failed to get stream capabilities, hr %#x.\n", hr);
    ok(caps == (MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE), "Unexpected caps %#x.\n", caps);

    caps = 0;
    hr = IMFByteStream_Read(bytestream, (BYTE *)&caps, sizeof(caps), &size);
    ok(hr == S_OK, "Failed to read from stream, hr %#x.\n", hr);
    ok(caps == 0xffff0000, "Unexpected content.\n");

    IMFAttributes_Release(attributes);
    IMFByteStream_Release(bytestream);
    IStream_Release(stream);
}

static void test_file_stream(void)
{
    static const WCHAR newfilename[] = L"new.mp4";
    IMFByteStream *bytestream, *bytestream2;
    QWORD bytestream_length, position;
    IMFAttributes *attributes = NULL;
    MF_ATTRIBUTE_TYPE item_type;
    WCHAR pathW[MAX_PATH];
    DWORD caps, count;
    WCHAR *filename;
    IUnknown *unk;
    HRESULT hr;
    WCHAR *str;
    BOOL eos;

    filename = load_resource(L"test.mp4");

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFByteStreamBuffering, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFByteStreamCacheControl, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFMediaEventGenerator, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFGetService, (void **)&unk);
    ok(hr == S_OK, "Failed to get interface pointer, hr %#x.\n", hr);
    IUnknown_Release(unk);

    hr = IMFByteStream_GetCapabilities(bytestream, &caps);
    ok(hr == S_OK, "Failed to get stream capabilities, hr %#x.\n", hr);
    if (is_win8_plus)
    {
        ok(caps == (MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE | MFBYTESTREAM_DOES_NOT_USE_NETWORK),
            "Unexpected caps %#x.\n", caps);
    }
    else
        ok(caps == (MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE), "Unexpected caps %#x.\n", caps);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFAttributes,
                                 (void **)&attributes);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(attributes != NULL, "got NULL\n");

    hr = IMFAttributes_GetCount(attributes, &count);
    ok(hr == S_OK, "Failed to get attributes count, hr %#x.\n", hr);
    ok(count == 2, "Unexpected attributes count %u.\n", count);

    /* Original file name. */
    hr = IMFAttributes_GetAllocatedString(attributes, &MF_BYTESTREAM_ORIGIN_NAME, &str, &count);
    ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);
    ok(!lstrcmpW(str, filename), "Unexpected name %s.\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);

    /* Modification time. */
    hr = IMFAttributes_GetItemType(attributes, &MF_BYTESTREAM_LAST_MODIFIED_TIME, &item_type);
    ok(hr == S_OK, "Failed to get item type, hr %#x.\n", hr);
    ok(item_type == MF_ATTRIBUTE_BLOB, "Unexpected item type.\n");

    IMFAttributes_Release(attributes);

    /* Length. */
    hr = IMFByteStream_GetLength(bytestream, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    bytestream_length = 0;
    hr = IMFByteStream_GetLength(bytestream, &bytestream_length);
    ok(hr == S_OK, "Failed to get bytestream length, hr %#x.\n", hr);
    ok(bytestream_length > 0, "Unexpected bytestream length %s.\n", wine_dbgstr_longlong(bytestream_length));

    hr = IMFByteStream_SetCurrentPosition(bytestream, bytestream_length);
    ok(hr == S_OK, "Failed to set bytestream position, hr %#x.\n", hr);

    hr = IMFByteStream_IsEndOfStream(bytestream, &eos);
    ok(hr == S_OK, "Failed query end of stream, hr %#x.\n", hr);
    ok(eos == TRUE, "Unexpected IsEndOfStream result, %u.\n", eos);

    hr = IMFByteStream_SetCurrentPosition(bytestream, 2 * bytestream_length);
    ok(hr == S_OK, "Failed to set bytestream position, hr %#x.\n", hr);

    hr = IMFByteStream_GetCurrentPosition(bytestream, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFByteStream_GetCurrentPosition(bytestream, &position);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(position == 2 * bytestream_length, "Unexpected position.\n");

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IMFByteStream_Release(bytestream2);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &bytestream2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "Unexpected hr %#x.\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &bytestream2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "Unexpected hr %#x.\n", hr);

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

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, newfilename, &bytestream2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "Unexpected hr %#x.\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, newfilename, &bytestream2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "Unexpected hr %#x.\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_ALLOW_WRITE_SHARING,
            newfilename, &bytestream2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "Unexpected hr %#x.\n", hr);

    IMFByteStream_Release(bytestream);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_ALLOW_WRITE_SHARING, newfilename, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* Opening the file again fails even though MF_FILEFLAGS_ALLOW_WRITE_SHARING is set. */
    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_ALLOW_WRITE_SHARING,
            newfilename, &bytestream2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "Unexpected hr %#x.\n", hr);

    IMFByteStream_Release(bytestream);

    /* Explicit file: scheme */
    lstrcpyW(pathW, fileschemeW);
    lstrcatW(pathW, filename);
    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, pathW, &bytestream);
    ok(FAILED(hr), "Unexpected hr %#x.\n", hr);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

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

    /* Attempt to lock the buffer twice */
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
    hr = MFCreateAlignedMemoryBuffer(16, MF_8_BYTE_ALIGNMENT, NULL);
    ok(FAILED(hr), "Unexpected hr %#x.\n", hr);

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
    static const DWORD test_pattern = 0x22222222;
    IMFMediaBuffer *buffer, *buffer2, *buffer3;
    DWORD count, flags, length;
    IMFAttributes *attributes;
    IMFSample *sample;
    LONGLONG time;
    HRESULT hr;
    BYTE *data;

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

    /* CopyToBuffer() */
    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#x.\n", hr);

    hr = MFCreateMemoryBuffer(16, &buffer2);
    ok(hr == S_OK, "Failed to create a buffer, hr %#x.\n", hr);

    /* Sample with no buffers. */
    hr = IMFMediaBuffer_SetCurrentLength(buffer2, 1);
    ok(hr == S_OK, "Failed to set current length, hr %#x.\n", hr);
    hr = IMFSample_CopyToBuffer(sample, buffer2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFMediaBuffer_GetCurrentLength(buffer2, &length);
    ok(hr == S_OK, "Failed to get current length, hr %#x.\n", hr);
    ok(!length, "Unexpected length %u.\n", length);

    /* Single buffer, larger destination. */
    hr = MFCreateMemoryBuffer(8, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#x.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "Failed to lock buffer, hr %#x.\n", hr);
    *(DWORD *)data = 0x11111111;
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock, hr %#x.\n", hr);
    hr = IMFMediaBuffer_SetCurrentLength(buffer, 4);
    ok(hr == S_OK, "Failed to set current length, hr %#x.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#x.\n", hr);

    /* Existing content is overwritten. */
    hr = IMFMediaBuffer_SetCurrentLength(buffer2, 8);
    ok(hr == S_OK, "Failed to set length, hr %#x.\n", hr);

    hr = IMFSample_CopyToBuffer(sample, buffer2);
    ok(hr == S_OK, "Failed to copy to buffer, hr %#x.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer2, &length);
    ok(hr == S_OK, "Failed to get length, hr %#x.\n", hr);
    ok(length == 4, "Unexpected buffer length %u.\n", length);

    /* Multiple buffers, matching total size. */
    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#x.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#x.\n", hr);
    ok(count == 2, "Unexpected buffer count %u.\n", count);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 8);
    ok(hr == S_OK, "Failed to set current length, hr %#x.\n", hr);

    hr = IMFSample_CopyToBuffer(sample, buffer2);
    ok(hr == S_OK, "Failed to copy to buffer, hr %#x.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer2, &length);
    ok(hr == S_OK, "Failed to get length, hr %#x.\n", hr);
    ok(length == 16, "Unexpected buffer length %u.\n", length);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#x.\n", hr);

    hr = IMFMediaBuffer_SetCurrentLength(buffer2, 1);
    ok(hr == S_OK, "Failed to set buffer length, hr %#x.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer2, &data, NULL, NULL);
    ok(hr == S_OK, "Failed to lock buffer, hr %#x.\n", hr);
    *(DWORD *)data = test_pattern;
    hr = IMFMediaBuffer_Unlock(buffer2);
    ok(hr == S_OK, "Failed to unlock buffer, hr %#x.\n", hr);

    hr = IMFSample_CopyToBuffer(sample, buffer2);
    ok(hr == MF_E_BUFFERTOOSMALL, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer2, &data, NULL, NULL);
    ok(hr == S_OK, "Failed to lock buffer, hr %#x.\n", hr);
    ok(!memcmp(data, &test_pattern, sizeof(test_pattern)), "Unexpected contents, %#x\n", *(DWORD *)data);
    hr = IMFMediaBuffer_Unlock(buffer2);
    ok(hr == S_OK, "Failed to unlock buffer, hr %#x.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer2, &length);
    ok(hr == S_OK, "Failed to get length, hr %#x.\n", hr);
    ok(!length, "Unexpected buffer length %u.\n", length);

    IMFMediaBuffer_Release(buffer2);
    IMFSample_Release(sample);

    /* ConvertToContiguousBuffer() */
    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#x.\n", hr);

    hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMemoryBuffer(16, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#x.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#x.\n", hr);

    hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer2);
    ok(hr == S_OK, "Failed to convert, hr %#x.\n", hr);
    ok(buffer2 == buffer, "Unexpected buffer instance.\n");
    IMFMediaBuffer_Release(buffer2);

    hr = IMFSample_ConvertToContiguousBuffer(sample, NULL);
    ok(hr == S_OK, "Failed to convert, hr %#x.\n", hr);

    hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer2);
    ok(hr == S_OK, "Failed to convert, hr %#x.\n", hr);
    ok(buffer2 == buffer, "Unexpected buffer instance.\n");
    IMFMediaBuffer_Release(buffer2);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 3);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMemoryBuffer(16, &buffer2);
    ok(hr == S_OK, "Failed to create a buffer, hr %#x.\n", hr);

    hr = IMFMediaBuffer_SetCurrentLength(buffer2, 4);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer2);
    ok(hr == S_OK, "Failed to add buffer, hr %#x.\n", hr);
    IMFMediaBuffer_Release(buffer2);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#x.\n", hr);
    ok(count == 2, "Unexpected buffer count %u.\n", count);

    hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer3);
    ok(hr == S_OK, "Failed to convert, hr %#x.\n", hr);

    hr = IMFMediaBuffer_GetMaxLength(buffer3, &length);
    ok(hr == S_OK, "Failed to get maximum length, hr %#x.\n", hr);
    ok(length == 7, "Unexpected length %u.\n", length);

    hr = IMFMediaBuffer_GetCurrentLength(buffer3, &length);
    ok(hr == S_OK, "Failed to get maximum length, hr %#x.\n", hr);
    ok(length == 7, "Unexpected length %u.\n", length);

    IMFMediaBuffer_Release(buffer3);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected buffer count %u.\n", count);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#x.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#x.\n", hr);
    ok(count == 2, "Unexpected buffer count %u.\n", count);

    hr = IMFSample_ConvertToContiguousBuffer(sample, NULL);
    ok(hr == S_OK, "Failed to convert, hr %#x.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected buffer count %u.\n", count);

    IMFMediaBuffer_Release(buffer);

    IMFSample_Release(sample);
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
        IMFMediaEvent *event = NULL, *event2;

        if (is_win8_plus)
        {
            hr = IMFMediaEventQueue_GetEvent(queue, MF_EVENT_FLAG_NO_WAIT, &event);
            ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Failed to get event, hr %#x.\n", hr);

            hr = IMFMediaEventQueue_GetEvent(queue, 0, &event);
            ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Failed to get event, hr %#x.\n", hr);

            hr = IMFMediaEventQueue_EndGetEvent(queue, result, &event);
            ok(hr == S_OK, "Failed to finalize GetEvent, hr %#x.\n", hr);

            hr = IMFMediaEventQueue_EndGetEvent(queue, result, &event2);
            ok(hr == E_FAIL, "Unexpected result, hr %#x.\n", hr);

            if (event)
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
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
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

    res = MFHeapAlloc(16, 0, NULL, 0, eAllocationTypeIgnore);
    ok(res != NULL, "MFHeapAlloc failed.\n");

    MFHeapFree(res);
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
        win_skip("Serial queues are not supported.\n");
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
        hr = MFShutdown();
        ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
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

    ret = WaitForSingleObject(callback.event, 500);
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
    UINT64 value;
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

    hr = IMFPresentationDescriptor_SetUINT64(pd, &MF_PD_TOTAL_FILE_SIZE, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFPresentationDescriptor_Clone(pd, &pd2);
    ok(hr == S_OK, "Failed to clone, hr %#x.\n", hr);

    hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd2, 0, &selected, &stream_desc2);
    ok(hr == S_OK, "Failed to get stream descriptor, hr %#x.\n", hr);
    ok(!!selected, "Unexpected selected state.\n");
    ok(stream_desc2 == stream_desc[0], "Unexpected stream descriptor.\n");
    IMFStreamDescriptor_Release(stream_desc2);

    value = 0;
    hr = IMFPresentationDescriptor_GetUINT64(pd2, &MF_PD_TOTAL_FILE_SIZE, &value);
    ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);
    ok(value == 1, "Unexpected attribute value.\n");

    IMFPresentationDescriptor_Release(pd2);
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
    IMFPresentationTimeSource *time_source, *time_source2;
    IMFClockStateSink *statesink;
    IMFClock *clock, *clock2;
    MFCLOCK_PROPERTIES props;
    MFCLOCK_STATE state;
    unsigned int i;
    MFTIME systime;
    LONGLONG time;
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

    /* Properties. */
    hr = IMFPresentationTimeSource_GetProperties(time_source, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetProperties(time_source, &props);
    ok(hr == S_OK, "Failed to get clock properties, hr %#x.\n", hr);

    ok(props.qwCorrelationRate == 0, "Unexpected correlation rate %s.\n",
            wine_dbgstr_longlong(props.qwCorrelationRate));
    ok(IsEqualGUID(&props.guidClockId, &GUID_NULL), "Unexpected clock id %s.\n", wine_dbgstr_guid(&props.guidClockId));
    ok(props.dwClockFlags == 0, "Unexpected flags %#x.\n", props.dwClockFlags);
    ok(props.qwClockFrequency == MFCLOCK_FREQUENCY_HNS, "Unexpected frequency %s.\n",
            wine_dbgstr_longlong(props.qwClockFrequency));
    ok(props.dwClockTolerance == MFCLOCK_TOLERANCE_UNKNOWN, "Unexpected tolerance %u.\n", props.dwClockTolerance);
    ok(props.dwClockJitter == 1, "Unexpected jitter %u.\n", props.dwClockJitter);

    /* Underlying clock. */
    hr = MFCreateSystemTimeSource(&time_source2);
    ok(hr == S_OK, "Failed to create time source, hr %#x.\n", hr);
    EXPECT_REF(time_source2, 1);
    hr = IMFPresentationTimeSource_GetUnderlyingClock(time_source2, &clock2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    EXPECT_REF(time_source2, 1);
    EXPECT_REF(clock2, 2);

    EXPECT_REF(time_source, 1);
    hr = IMFPresentationTimeSource_GetUnderlyingClock(time_source, &clock);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    EXPECT_REF(time_source, 1);
    EXPECT_REF(clock, 2);

    ok(clock != clock2, "Unexpected clock instance.\n");

    IMFPresentationTimeSource_Release(time_source2);
    IMFClock_Release(clock2);

    hr = IMFClock_GetClockCharacteristics(clock, &value);
    ok(hr == S_OK, "Failed to get clock flags, hr %#x.\n", hr);
    ok(value == (MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ | MFCLOCK_CHARACTERISTICS_FLAG_ALWAYS_RUNNING |
            MFCLOCK_CHARACTERISTICS_FLAG_IS_SYSTEM_CLOCK), "Unexpected flags %#x.\n", value);

    hr = IMFClock_GetContinuityKey(clock, &value);
    ok(hr == S_OK, "Failed to get clock key, hr %#x.\n", hr);
    ok(value == 0, "Unexpected key value %u.\n", value);

    hr = IMFClock_GetState(clock, 0, &state);
    ok(hr == S_OK, "Failed to get clock state, hr %#x.\n", hr);
    ok(state == MFCLOCK_STATE_RUNNING, "Unexpected state %d.\n", state);

    hr = IMFClock_GetProperties(clock, &props);
    ok(hr == S_OK, "Failed to get clock properties, hr %#x.\n", hr);

    ok(props.qwCorrelationRate == 0, "Unexpected correlation rate %s.\n",
            wine_dbgstr_longlong(props.qwCorrelationRate));
    ok(IsEqualGUID(&props.guidClockId, &GUID_NULL), "Unexpected clock id %s.\n", wine_dbgstr_guid(&props.guidClockId));
    ok(props.dwClockFlags == 0, "Unexpected flags %#x.\n", props.dwClockFlags);
    ok(props.qwClockFrequency == MFCLOCK_FREQUENCY_HNS, "Unexpected frequency %s.\n",
            wine_dbgstr_longlong(props.qwClockFrequency));
    ok(props.dwClockTolerance == MFCLOCK_TOLERANCE_UNKNOWN, "Unexpected tolerance %u.\n", props.dwClockTolerance);
    ok(props.dwClockJitter == 1, "Unexpected jitter %u.\n", props.dwClockJitter);

    hr = IMFClock_GetCorrelatedTime(clock, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get clock time, hr %#x.\n", hr);
    ok(time == systime, "Unexpected time %s, %s.\n", wine_dbgstr_longlong(time), wine_dbgstr_longlong(systime));

    IMFClock_Release(clock);

    /* Test returned time regarding specified rate and offset.  */
    hr = IMFPresentationTimeSource_QueryInterface(time_source, &IID_IMFClockStateSink, (void **)&statesink);
    ok(hr == S_OK, "Failed to get sink interface, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetState(time_source, 0, &state);
    ok(hr == S_OK, "Failed to get state %#x.\n", hr);
    ok(state == MFCLOCK_STATE_STOPPED, "Unexpected state %d.\n", state);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == 0, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time), wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 0, 0);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time), wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 0, 1);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == systime + 1, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 2);
    ok(hr == S_OK, "Failed to pause source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == 3, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time), wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockRestart(statesink, 5);
    ok(hr == S_OK, "Failed to restart source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == systime - 2, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 0);
    ok(hr == S_OK, "Failed to pause source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == -2, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStop(statesink, 123);
    ok(hr == S_OK, "Failed to stop source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == 0, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time), wine_dbgstr_longlong(systime));

    /* Increased rate. */
    hr = IMFClockStateSink_OnClockSetRate(statesink, 0, 2.0f);
    ok(hr == S_OK, "Failed to set rate, hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockStart(statesink, 0, 0);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == 2 * systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(2 * systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 0, 10);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == 2 * systime + 10, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(2 * systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 2);
    ok(hr == S_OK, "Failed to pause source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == 10 + 2 * 2, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockRestart(statesink, 5);
    ok(hr == S_OK, "Failed to restart source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == 2 * systime + 14 - 5 * 2, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 0);
    ok(hr == S_OK, "Failed to pause source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == 4, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStop(statesink, 123);
    ok(hr == S_OK, "Failed to stop source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == 0, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time), wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 10, 0);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == 2 * systime - 2 * 10, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(2 * systime));

    hr = IMFClockStateSink_OnClockStop(statesink, 123);
    ok(hr == S_OK, "Failed to stop source, hr %#x.\n", hr);

    hr = IMFClockStateSink_OnClockStart(statesink, 10, 20);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == 2 * systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(2 * systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 2);
    ok(hr == S_OK, "Failed to pause source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == 2 * 2, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockRestart(statesink, 5);
    ok(hr == S_OK, "Failed to restart source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == 2 * systime + 4 - 5 * 2, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 0);
    ok(hr == S_OK, "Failed to pause source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == -6, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    IMFClockStateSink_Release(statesink);
    IMFPresentationTimeSource_Release(time_source);

    /* PRESENTATION_CURRENT_POSITION */
    hr = MFCreateSystemTimeSource(&time_source);
    ok(hr == S_OK, "Failed to create time source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_QueryInterface(time_source, &IID_IMFClockStateSink, (void **)&statesink);
    ok(hr == S_OK, "Failed to get sink interface, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(!time && systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    /* INVALID -> RUNNING */
    hr = IMFClockStateSink_OnClockStart(statesink, 10, PRESENTATION_CURRENT_POSITION);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == systime - 10, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    /* RUNNING -> RUNNING */
    hr = IMFClockStateSink_OnClockStart(statesink, 20, PRESENTATION_CURRENT_POSITION);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == systime - 10, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 0, PRESENTATION_CURRENT_POSITION);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == systime - 10, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 0, 0);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 30, PRESENTATION_CURRENT_POSITION);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    /* STOPPED -> RUNNING */
    hr = IMFClockStateSink_OnClockStop(statesink, 567);
    ok(hr == S_OK, "Failed to stop source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(!time && systime != 0, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 30, PRESENTATION_CURRENT_POSITION);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == systime - 30, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    /* PAUSED -> RUNNING */
    hr = IMFClockStateSink_OnClockPause(statesink, 8);
    ok(hr == S_OK, "Failed to pause source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == (-30 + 8) && systime != 0, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 40, PRESENTATION_CURRENT_POSITION);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == systime + (-30 + 8 - 40), "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 7);
    ok(hr == S_OK, "Failed to pause source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == (-30 + 8 - 40 + 7) && systime != 0, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 50, 7);
    ok(hr == S_OK, "Failed to start source, hr %#x.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#x.\n", hr);
    ok(time == systime + (-50 + 7), "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

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

static void test_stream_descriptor(void)
{
    IMFMediaType *media_types[2], *media_type, *media_type2, *media_type3;
    IMFMediaTypeHandler *type_handler;
    IMFStreamDescriptor *stream_desc;
    GUID major_type;
    DWORD id, count;
    unsigned int i;
    HRESULT hr;

    hr = MFCreateStreamDescriptor(123, 0, NULL, &stream_desc);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(media_types); ++i)
    {
        hr = MFCreateMediaType(&media_types[i]);
        ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);
    }

    hr = MFCreateStreamDescriptor(123, 0, media_types, &stream_desc);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = MFCreateStreamDescriptor(123, ARRAY_SIZE(media_types), media_types, &stream_desc);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFStreamDescriptor_GetStreamIdentifier(stream_desc, &id);
    ok(hr == S_OK, "Failed to get descriptor id, hr %#x.\n", hr);
    ok(id == 123, "Unexpected id %#x.\n", id);

    hr = IMFStreamDescriptor_GetMediaTypeHandler(stream_desc, &type_handler);
    ok(hr == S_OK, "Failed to get type handler, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeCount(type_handler, &count);
    ok(hr == S_OK, "Failed to get type count, hr %#x.\n", hr);
    ok(count == ARRAY_SIZE(media_types), "Unexpected type count.\n");

    hr = IMFMediaTypeHandler_GetCurrentMediaType(type_handler, &media_type);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &major_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(media_types); ++i)
    {
        hr = IMFMediaTypeHandler_GetMediaTypeByIndex(type_handler, i, &media_type);
        ok(hr == S_OK, "Failed to get media type, hr %#x.\n", hr);
        ok(media_type == media_types[i], "Unexpected object.\n");

        if (SUCCEEDED(hr))
            IMFMediaType_Release(media_type);
    }

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(type_handler, 2, &media_type);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#x.\n", hr);

    /* IsMediaTypeSupported() */

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, NULL, &media_type2);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = MFCreateMediaType(&media_type3);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, media_type);
    ok(hr == S_OK, "Failed to set current type, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &major_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set major type, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &major_type);
    ok(hr == S_OK, "Failed to get major type, hr %#x.\n", hr);
    ok(IsEqualGUID(&major_type, &MFMediaType_Audio), "Unexpected major type.\n");

    /* Mismatching major types. */
    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set major type, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Subtype missing. */
    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Mismatching subtype. */
    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFAudioFormat_MP3);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaTypeHandler_GetMediaTypeCount(type_handler, &count);
    ok(hr == S_OK, "Failed to get type count, hr %#x.\n", hr);
    ok(count == ARRAY_SIZE(media_types), "Unexpected type count.\n");

    IMFMediaTypeHandler_Release(type_handler);
    IMFStreamDescriptor_Release(stream_desc);

    /* IsMediaTypeSupported() for unset current type. */
    hr = MFCreateStreamDescriptor(123, ARRAY_SIZE(media_types), media_types, &stream_desc);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFStreamDescriptor_GetMediaTypeHandler(stream_desc, &type_handler);
    ok(hr == S_OK, "Failed to get type handler, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, NULL);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    /* Initialize one from initial type set. */
    hr = IMFMediaType_CopyAllItems(media_type3, (IMFAttributes *)media_types[0]);
    ok(hr == S_OK, "Failed to copy attributes, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to copy attributes, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFAudioFormat_MP3);
    ok(hr == S_OK, "Failed to copy attributes, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Now set current type that's not compatible. */
    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to copy attributes, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFVideoFormat_RGB8);
    ok(hr == S_OK, "Failed to copy attributes, hr %#x.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, media_type3);
    ok(hr == S_OK, "Failed to set current type, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaType_CopyAllItems(media_types[0], (IMFAttributes *)media_type);
    ok(hr == S_OK, "Failed to copy attributes, hr %#x.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    IMFMediaType_Release(media_type);
    IMFMediaType_Release(media_type3);

    IMFMediaTypeHandler_Release(type_handler);

    IMFStreamDescriptor_Release(stream_desc);
}

static void test_MFCalculateImageSize(void)
{
    static const struct image_size_test
    {
        const GUID *subtype;
        UINT32 width;
        UINT32 height;
        UINT32 size;
        UINT32 plane_size; /* Matches image size when 0. */
    }
    image_size_tests[] =
    {
        { &MFVideoFormat_RGB8, 3, 5, 20 },
        { &MFVideoFormat_RGB8, 1, 1, 4 },
        { &MFVideoFormat_RGB555, 3, 5, 40 },
        { &MFVideoFormat_RGB555, 1, 1, 4 },
        { &MFVideoFormat_RGB565, 3, 5, 40 },
        { &MFVideoFormat_RGB565, 1, 1, 4 },
        { &MFVideoFormat_RGB24, 3, 5, 60 },
        { &MFVideoFormat_RGB24, 1, 1, 4 },
        { &MFVideoFormat_RGB32, 3, 5, 60 },
        { &MFVideoFormat_RGB32, 1, 1, 4 },
        { &MFVideoFormat_ARGB32, 3, 5, 60 },
        { &MFVideoFormat_ARGB32, 1, 1, 4 },
        { &MFVideoFormat_A2R10G10B10, 3, 5, 60 },
        { &MFVideoFormat_A2R10G10B10, 1, 1, 4 },
        { &MFVideoFormat_A16B16G16R16F, 3, 5, 120 },
        { &MFVideoFormat_A16B16G16R16F, 1, 1, 8 },

        /* YUV */
        { &MFVideoFormat_NV12, 1, 3, 9, 4 },
        { &MFVideoFormat_NV12, 1, 2, 6, 3 },
        { &MFVideoFormat_NV12, 2, 2, 6, 6 },
        { &MFVideoFormat_NV12, 3, 2, 12, 9 },
        { &MFVideoFormat_NV12, 4, 2, 12 },
        { &MFVideoFormat_NV12, 320, 240, 115200 },
        { &MFVideoFormat_AYUV, 1, 1, 4 },
        { &MFVideoFormat_AYUV, 2, 1, 8 },
        { &MFVideoFormat_AYUV, 1, 2, 8 },
        { &MFVideoFormat_AYUV, 4, 3, 48 },
        { &MFVideoFormat_AYUV, 320, 240, 307200 },
        { &MFVideoFormat_IMC1, 1, 1, 4 },
        { &MFVideoFormat_IMC1, 2, 1, 4 },
        { &MFVideoFormat_IMC1, 1, 2, 8 },
        { &MFVideoFormat_IMC1, 4, 3, 24 },
        { &MFVideoFormat_IMC1, 320, 240, 153600 },
        { &MFVideoFormat_IMC3, 1, 1, 4 },
        { &MFVideoFormat_IMC3, 2, 1, 4 },
        { &MFVideoFormat_IMC3, 1, 2, 8 },
        { &MFVideoFormat_IMC3, 4, 3, 24 },
        { &MFVideoFormat_IMC3, 320, 240, 153600 },
        { &MFVideoFormat_IMC2, 1, 3, 9, 4 },
        { &MFVideoFormat_IMC2, 1, 2, 6, 3 },
        { &MFVideoFormat_IMC2, 2, 2, 6, 6 },
        { &MFVideoFormat_IMC2, 3, 2, 12, 9 },
        { &MFVideoFormat_IMC2, 4, 2, 12 },
        { &MFVideoFormat_IMC2, 320, 240, 115200 },
        { &MFVideoFormat_IMC4, 1, 3, 9, 4 },
        { &MFVideoFormat_IMC4, 1, 2, 6, 3 },
        { &MFVideoFormat_IMC4, 2, 2, 6, 6 },
        { &MFVideoFormat_IMC4, 3, 2, 12, 9 },
        { &MFVideoFormat_IMC4, 4, 2, 12 },
        { &MFVideoFormat_IMC4, 320, 240, 115200 },
        { &MFVideoFormat_YV12, 1, 1, 3, 1 },
        { &MFVideoFormat_YV12, 2, 1, 3 },
        { &MFVideoFormat_YV12, 1, 2, 6, 3 },
        { &MFVideoFormat_YV12, 4, 3, 18 },
        { &MFVideoFormat_YV12, 320, 240, 115200 },

        { &MFVideoFormat_I420, 1, 1, 3, 1 },
        { &MFVideoFormat_I420, 2, 1, 3 },
        { &MFVideoFormat_I420, 1, 2, 6, 3 },
        { &MFVideoFormat_I420, 4, 3, 18 },
        { &MFVideoFormat_I420, 320, 240, 115200 },

        { &MFVideoFormat_YUY2, 2, 1, 4 },
        { &MFVideoFormat_YUY2, 4, 3, 24 },
        { &MFVideoFormat_YUY2, 128, 128, 32768 },
        { &MFVideoFormat_YUY2, 320, 240, 153600 },

        { &MFVideoFormat_UYVY, 2, 1, 4 },
        { &MFVideoFormat_UYVY, 4, 3, 24 },
        { &MFVideoFormat_UYVY, 128, 128, 32768 },
        { &MFVideoFormat_UYVY, 320, 240, 153600 },
    };
    unsigned int i;
    UINT32 size;
    HRESULT hr;

    if (!pMFGetPlaneSize)
        win_skip("MFGetPlaneSize() is not available.\n");

    size = 1;
    hr = MFCalculateImageSize(&IID_IUnknown, 1, 1, &size);
    ok(hr == E_INVALIDARG || broken(hr == S_OK) /* Vista */, "Unexpected hr %#x.\n", hr);
    ok(size == 0, "Unexpected size %u.\n", size);

    for (i = 0; i < ARRAY_SIZE(image_size_tests); ++i)
    {
        const struct image_size_test *ptr = &image_size_tests[i];

        /* Those are supported since Win10. */
        BOOL is_broken = IsEqualGUID(ptr->subtype, &MFVideoFormat_A16B16G16R16F) ||
                IsEqualGUID(ptr->subtype, &MFVideoFormat_A2R10G10B10);

        hr = MFCalculateImageSize(ptr->subtype, ptr->width, ptr->height, &size);
        ok(hr == S_OK || (is_broken && hr == E_INVALIDARG), "%u: failed to calculate image size, hr %#x.\n", i, hr);
        ok(size == ptr->size, "%u: unexpected image size %u, expected %u. Size %u x %u, format %s.\n", i, size, ptr->size,
                ptr->width, ptr->height, wine_dbgstr_an((char *)&ptr->subtype->Data1, 4));

        if (pMFGetPlaneSize)
        {
            unsigned int plane_size = ptr->plane_size ? ptr->plane_size : ptr->size;

            hr = pMFGetPlaneSize(ptr->subtype->Data1, ptr->width, ptr->height, &size);
            ok(hr == S_OK, "%u: failed to get plane size, hr %#x.\n", i, hr);
            ok(size == plane_size, "%u: unexpected plane size %u, expected %u.\n", i, size, plane_size);
        }
    }
}

static void test_MFCompareFullToPartialMediaType(void)
{
    IMFMediaType *full_type, *partial_type;
    HRESULT hr;
    BOOL ret;

    hr = MFCreateMediaType(&full_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = MFCreateMediaType(&partial_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    ret = MFCompareFullToPartialMediaType(full_type, partial_type);
    ok(!ret, "Unexpected result %d.\n", ret);

    hr = IMFMediaType_SetGUID(full_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set major type, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(partial_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set major type, hr %#x.\n", hr);

    ret = MFCompareFullToPartialMediaType(full_type, partial_type);
    ok(ret, "Unexpected result %d.\n", ret);

    hr = IMFMediaType_SetGUID(full_type, &MF_MT_SUBTYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set major type, hr %#x.\n", hr);

    ret = MFCompareFullToPartialMediaType(full_type, partial_type);
    ok(ret, "Unexpected result %d.\n", ret);

    hr = IMFMediaType_SetGUID(partial_type, &MF_MT_SUBTYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set major type, hr %#x.\n", hr);

    ret = MFCompareFullToPartialMediaType(full_type, partial_type);
    ok(!ret, "Unexpected result %d.\n", ret);

    IMFMediaType_Release(full_type);
    IMFMediaType_Release(partial_type);
}

static void test_attributes_serialization(void)
{
    static const UINT8 blob[] = {1,2,3};
    IMFAttributes *attributes, *dest;
    UINT32 size, count, value32;
    double value_dbl;
    UINT64 value64;
    UINT8 *buffer;
    IUnknown *obj;
    HRESULT hr;
    WCHAR *str;
    GUID guid;

    hr = MFCreateAttributes(&attributes, 0);
    ok(hr == S_OK, "Failed to create object, hr %#x.\n", hr);

    hr = MFCreateAttributes(&dest, 0);
    ok(hr == S_OK, "Failed to create object, hr %#x.\n", hr);

    hr = MFGetAttributesAsBlobSize(attributes, &size);
    ok(hr == S_OK, "Failed to get blob size, hr %#x.\n", hr);
    ok(size == 8, "Got size %u.\n", size);

    buffer = heap_alloc(size);

    hr = MFGetAttributesAsBlob(attributes, buffer, size);
    ok(hr == S_OK, "Failed to serialize, hr %#x.\n", hr);

    hr = MFGetAttributesAsBlob(attributes, buffer, size - 1);
    ok(hr == MF_E_BUFFERTOOSMALL, "Unexpected hr %#x.\n", hr);

    hr = MFInitAttributesFromBlob(dest, buffer, size - 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMFAttributes_SetUINT32(dest, &MF_MT_MAJOR_TYPE, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = MFInitAttributesFromBlob(dest, buffer, size);
    ok(hr == S_OK, "Failed to deserialize, hr %#x.\n", hr);

    /* Previous items are cleared. */
    hr = IMFAttributes_GetCount(dest, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#x.\n", hr);
    ok(count == 0, "Unexpected count %u.\n", count);

    heap_free(buffer);

    /* Set some attributes of various types. */
    IMFAttributes_SetUINT32(attributes, &MF_MT_MAJOR_TYPE, 456);
    IMFAttributes_SetUINT64(attributes, &MF_MT_SUBTYPE, 123);
    IMFAttributes_SetDouble(attributes, &IID_IUnknown, 0.5);
    IMFAttributes_SetUnknown(attributes, &IID_IMFAttributes, (IUnknown *)attributes);
    IMFAttributes_SetGUID(attributes, &GUID_NULL, &IID_IUnknown);
    IMFAttributes_SetString(attributes, &DUMMY_CLSID, L"Text");
    IMFAttributes_SetBlob(attributes, &DUMMY_GUID1, blob, sizeof(blob));

    hr = MFGetAttributesAsBlobSize(attributes, &size);
    ok(hr == S_OK, "Failed to get blob size, hr %#x.\n", hr);
    ok(size > 8, "Got unexpected size %u.\n", size);

    buffer = heap_alloc(size);
    hr = MFGetAttributesAsBlob(attributes, buffer, size);
    ok(hr == S_OK, "Failed to serialize, hr %#x.\n", hr);
    hr = MFInitAttributesFromBlob(dest, buffer, size);
    ok(hr == S_OK, "Failed to deserialize, hr %#x.\n", hr);
    heap_free(buffer);

    hr = IMFAttributes_GetUINT32(dest, &MF_MT_MAJOR_TYPE, &value32);
    ok(hr == S_OK, "Failed to get get uint32 value, hr %#x.\n", hr);
    ok(value32 == 456, "Unexpected value %u.\n", value32);
    hr = IMFAttributes_GetUINT64(dest, &MF_MT_SUBTYPE, &value64);
    ok(hr == S_OK, "Failed to get get uint64 value, hr %#x.\n", hr);
    ok(value64 == 123, "Unexpected value.\n");
    hr = IMFAttributes_GetDouble(dest, &IID_IUnknown, &value_dbl);
    ok(hr == S_OK, "Failed to get get double value, hr %#x.\n", hr);
    ok(value_dbl == 0.5, "Unexpected value.\n");
    hr = IMFAttributes_GetUnknown(dest, &IID_IMFAttributes, &IID_IUnknown, (void **)&obj);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);
    hr = IMFAttributes_GetGUID(dest, &GUID_NULL, &guid);
    ok(hr == S_OK, "Failed to get guid value, hr %#x.\n", hr);
    ok(IsEqualGUID(&guid, &IID_IUnknown), "Unexpected guid.\n");
    hr = IMFAttributes_GetAllocatedString(dest, &DUMMY_CLSID, &str, &size);
    ok(hr == S_OK, "Failed to get string value, hr %#x.\n", hr);
    ok(!lstrcmpW(str, L"Text"), "Unexpected string.\n");
    CoTaskMemFree(str);
    hr = IMFAttributes_GetAllocatedBlob(dest, &DUMMY_GUID1, &buffer, &size);
    ok(hr == S_OK, "Failed to get blob value, hr %#x.\n", hr);
    ok(!memcmp(buffer, blob, sizeof(blob)), "Unexpected blob.\n");
    CoTaskMemFree(buffer);

    IMFAttributes_Release(attributes);
    IMFAttributes_Release(dest);
}

static void test_wrapped_media_type(void)
{
    IMFMediaType *mediatype, *mediatype2;
    UINT32 count, type;
    HRESULT hr;
    GUID guid;

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = MFUnwrapMediaType(mediatype, &mediatype2);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetUINT32(mediatype, &GUID_NULL, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);
    hr = IMFMediaType_SetUINT32(mediatype, &DUMMY_GUID1, 2);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set GUID value, hr %#x.\n", hr);

    hr = MFWrapMediaType(mediatype, &MFMediaType_Audio, &IID_IUnknown, &mediatype2);
    ok(hr == S_OK, "Failed to create wrapped media type, hr %#x.\n", hr);

    hr = IMFMediaType_GetGUID(mediatype2, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#x.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Audio), "Unexpected major type.\n");

    hr = IMFMediaType_GetGUID(mediatype2, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Failed to get subtype, hr %#x.\n", hr);
    ok(IsEqualGUID(&guid, &IID_IUnknown), "Unexpected major type.\n");

    hr = IMFMediaType_GetCount(mediatype2, &count);
    ok(hr == S_OK, "Failed to get item count, hr %#x.\n", hr);
    ok(count == 3, "Unexpected count %u.\n", count);

    hr = IMFMediaType_GetItemType(mediatype2, &MF_MT_WRAPPED_TYPE, &type);
    ok(hr == S_OK, "Failed to get item type, hr %#x.\n", hr);
    ok(type == MF_ATTRIBUTE_BLOB, "Unexpected item type.\n");

    IMFMediaType_Release(mediatype);

    hr = MFUnwrapMediaType(mediatype2, &mediatype);
    ok(hr == S_OK, "Failed to unwrap, hr %#x.\n", hr);

    hr = IMFMediaType_GetGUID(mediatype, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#x.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected major type.\n");

    hr = IMFMediaType_GetGUID(mediatype, &MF_MT_SUBTYPE, &guid);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    IMFMediaType_Release(mediatype);
    IMFMediaType_Release(mediatype2);
}

static void test_MFCreateWaveFormatExFromMFMediaType(void)
{
    WAVEFORMATEXTENSIBLE *format_ext;
    IMFMediaType *mediatype;
    WAVEFORMATEX *format;
    UINT32 size;
    HRESULT hr;

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, &format, &size, MFWaveFormatExConvertFlag_Normal);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, &format, &size, MFWaveFormatExConvertFlag_Normal);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    /* Audio/PCM */
    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);
    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, &format, &size, MFWaveFormatExConvertFlag_Normal);
    ok(hr == S_OK, "Failed to create format, hr %#x.\n", hr);
    ok(format != NULL, "Expected format structure.\n");
    ok(size == sizeof(*format), "Unexpected size %u.\n", size);
    ok(format->wFormatTag == WAVE_FORMAT_PCM, "Unexpected tag.\n");
    ok(format->nChannels == 0, "Unexpected number of channels, %u.\n", format->nChannels);
    ok(format->nSamplesPerSec == 0, "Unexpected sample rate, %u.\n", format->nSamplesPerSec);
    ok(format->nAvgBytesPerSec == 0, "Unexpected average data rate rate, %u.\n", format->nAvgBytesPerSec);
    ok(format->nBlockAlign == 0, "Unexpected alignment, %u.\n", format->nBlockAlign);
    ok(format->wBitsPerSample == 0, "Unexpected sample size, %u.\n", format->wBitsPerSample);
    ok(format->cbSize == 0, "Unexpected size field, %u.\n", format->cbSize);
    CoTaskMemFree(format);

    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, (WAVEFORMATEX **)&format_ext, &size,
            MFWaveFormatExConvertFlag_ForceExtensible);
    ok(hr == S_OK, "Failed to create format, hr %#x.\n", hr);
    ok(format_ext != NULL, "Expected format structure.\n");
    ok(size == sizeof(*format_ext), "Unexpected size %u.\n", size);
    ok(format_ext->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE, "Unexpected tag.\n");
    ok(format_ext->Format.nChannels == 0, "Unexpected number of channels, %u.\n", format_ext->Format.nChannels);
    ok(format_ext->Format.nSamplesPerSec == 0, "Unexpected sample rate, %u.\n", format_ext->Format.nSamplesPerSec);
    ok(format_ext->Format.nAvgBytesPerSec == 0, "Unexpected average data rate rate, %u.\n",
            format_ext->Format.nAvgBytesPerSec);
    ok(format_ext->Format.nBlockAlign == 0, "Unexpected alignment, %u.\n", format_ext->Format.nBlockAlign);
    ok(format_ext->Format.wBitsPerSample == 0, "Unexpected sample size, %u.\n", format_ext->Format.wBitsPerSample);
    ok(format_ext->Format.cbSize == sizeof(*format_ext) - sizeof(format_ext->Format), "Unexpected size field, %u.\n",
            format_ext->Format.cbSize);
    CoTaskMemFree(format_ext);

    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, &format, &size, MFWaveFormatExConvertFlag_ForceExtensible + 1);
    ok(hr == S_OK, "Failed to create format, hr %#x.\n", hr);
    ok(size == sizeof(*format), "Unexpected size %u.\n", size);
    CoTaskMemFree(format);

    IMFMediaType_Release(mediatype);
}

static HRESULT WINAPI test_create_file_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    IMFByteStream *stream;
    IUnknown *object;
    HRESULT hr;

    ok(!!result, "Unexpected result object.\n");

    ok((IUnknown *)iface == IMFAsyncResult_GetStateNoAddRef(result), "Unexpected result state.\n");

    hr = IMFAsyncResult_GetObject(result, &object);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = MFEndCreateFile(result, &stream);
    ok(hr == S_OK, "Failed to get file stream, hr %#x.\n", hr);
    IMFByteStream_Release(stream);

    SetEvent(callback->event);

    return S_OK;
}

static const IMFAsyncCallbackVtbl test_create_file_callback_vtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    test_create_file_callback_Invoke,
};

static void test_async_create_file(void)
{
    struct test_callback callback = { { &test_create_file_callback_vtbl } };
    WCHAR pathW[MAX_PATH], fileW[MAX_PATH];
    IUnknown *cancel_cookie;
    HRESULT hr;
    BOOL ret;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Fail to start up, hr %#x.\n", hr);

    callback.event = CreateEventA(NULL, FALSE, FALSE, NULL);

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    GetTempFileNameW(pathW, NULL, 0, fileW);

    hr = MFBeginCreateFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_DELETE_IF_EXIST, MF_FILEFLAGS_NONE, fileW,
            &callback.IMFAsyncCallback_iface, (IUnknown *)&callback.IMFAsyncCallback_iface, &cancel_cookie);
    ok(hr == S_OK, "Async create request failed, hr %#x.\n", hr);
    ok(cancel_cookie != NULL, "Unexpected cancellation object.\n");

    WaitForSingleObject(callback.event, INFINITE);

    IUnknown_Release(cancel_cookie);

    CloseHandle(callback.event);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);

    ret = DeleteFileW(fileW);
    ok(ret, "Failed to delete test file.\n");
}

struct activate_object
{
    IMFActivate IMFActivate_iface;
    LONG refcount;
};

static HRESULT WINAPI activate_object_QueryInterface(IMFActivate *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFActivate) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFActivate_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activate_object_AddRef(IMFActivate *iface)
{
    return 2;
}

static ULONG WINAPI activate_object_Release(IMFActivate *iface)
{
    return 1;
}

static HRESULT WINAPI activate_object_GetItem(IMFActivate *iface, REFGUID key, PROPVARIANT *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetItemType(IMFActivate *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_CompareItem(IMFActivate *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_Compare(IMFActivate *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
        BOOL *result)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetUINT32(IMFActivate *iface, REFGUID key, UINT32 *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetUINT64(IMFActivate *iface, REFGUID key, UINT64 *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetDouble(IMFActivate *iface, REFGUID key, double *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetGUID(IMFActivate *iface, REFGUID key, GUID *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetStringLength(IMFActivate *iface, REFGUID key, UINT32 *length)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetString(IMFActivate *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetAllocatedString(IMFActivate *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetBlobSize(IMFActivate *iface, REFGUID key, UINT32 *size)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetBlob(IMFActivate *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetAllocatedBlob(IMFActivate *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetUnknown(IMFActivate *iface, REFGUID key, REFIID riid, void **ppv)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetItem(IMFActivate *iface, REFGUID key, REFPROPVARIANT value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_DeleteItem(IMFActivate *iface, REFGUID key)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_DeleteAllItems(IMFActivate *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetUINT32(IMFActivate *iface, REFGUID key, UINT32 value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetUINT64(IMFActivate *iface, REFGUID key, UINT64 value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetDouble(IMFActivate *iface, REFGUID key, double value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetGUID(IMFActivate *iface, REFGUID key, REFGUID value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetString(IMFActivate *iface, REFGUID key, const WCHAR *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetBlob(IMFActivate *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetUnknown(IMFActivate *iface, REFGUID key, IUnknown *unknown)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_LockStore(IMFActivate *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_UnlockStore(IMFActivate *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetCount(IMFActivate *iface, UINT32 *count)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetItemByIndex(IMFActivate *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_CopyAllItems(IMFActivate *iface, IMFAttributes *dest)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_ActivateObject(IMFActivate *iface, REFIID riid, void **obj)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_ShutdownObject(IMFActivate *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_DetachObject(IMFActivate *iface)
{
    return E_NOTIMPL;
}

static const IMFActivateVtbl activate_object_vtbl =
{
    activate_object_QueryInterface,
    activate_object_AddRef,
    activate_object_Release,
    activate_object_GetItem,
    activate_object_GetItemType,
    activate_object_CompareItem,
    activate_object_Compare,
    activate_object_GetUINT32,
    activate_object_GetUINT64,
    activate_object_GetDouble,
    activate_object_GetGUID,
    activate_object_GetStringLength,
    activate_object_GetString,
    activate_object_GetAllocatedString,
    activate_object_GetBlobSize,
    activate_object_GetBlob,
    activate_object_GetAllocatedBlob,
    activate_object_GetUnknown,
    activate_object_SetItem,
    activate_object_DeleteItem,
    activate_object_DeleteAllItems,
    activate_object_SetUINT32,
    activate_object_SetUINT64,
    activate_object_SetDouble,
    activate_object_SetGUID,
    activate_object_SetString,
    activate_object_SetBlob,
    activate_object_SetUnknown,
    activate_object_LockStore,
    activate_object_UnlockStore,
    activate_object_GetCount,
    activate_object_GetItemByIndex,
    activate_object_CopyAllItems,
    activate_object_ActivateObject,
    activate_object_ShutdownObject,
    activate_object_DetachObject,
};

static void test_local_handlers(void)
{
    IMFActivate local_activate = { &activate_object_vtbl };
    static const WCHAR localW[] = L"local";
    HRESULT hr;

    if (!pMFRegisterLocalSchemeHandler)
    {
        win_skip("Local handlers are not supported.\n");
        return;
    }

    hr = pMFRegisterLocalSchemeHandler(NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = pMFRegisterLocalSchemeHandler(localW, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = pMFRegisterLocalSchemeHandler(NULL, &local_activate);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = pMFRegisterLocalSchemeHandler(localW, &local_activate);
    ok(hr == S_OK, "Failed to register scheme handler, hr %#x.\n", hr);

    hr = pMFRegisterLocalSchemeHandler(localW, &local_activate);
    ok(hr == S_OK, "Failed to register scheme handler, hr %#x.\n", hr);

    hr = pMFRegisterLocalByteStreamHandler(NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = pMFRegisterLocalByteStreamHandler(NULL, NULL, &local_activate);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = pMFRegisterLocalByteStreamHandler(NULL, localW, &local_activate);
    ok(hr == S_OK, "Failed to register stream handler, hr %#x.\n", hr);

    hr = pMFRegisterLocalByteStreamHandler(localW, NULL, &local_activate);
    ok(hr == S_OK, "Failed to register stream handler, hr %#x.\n", hr);

    hr = pMFRegisterLocalByteStreamHandler(localW, localW, &local_activate);
    ok(hr == S_OK, "Failed to register stream handler, hr %#x.\n", hr);
}

static void test_create_property_store(void)
{
    static const PROPERTYKEY test_pkey = {{0x12345678}, 9};
    IPropertyStore *store, *store2;
    PROPVARIANT value = {0};
    PROPERTYKEY key;
    ULONG refcount;
    IUnknown *unk;
    DWORD count;
    HRESULT hr;

    hr = CreatePropertyStore(NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = CreatePropertyStore(&store);
    ok(hr == S_OK, "Failed to create property store, hr %#x.\n", hr);

    hr = CreatePropertyStore(&store2);
    ok(hr == S_OK, "Failed to create property store, hr %#x.\n", hr);
    ok(store2 != store, "Expected different store objects.\n");
    IPropertyStore_Release(store2);

    hr = IPropertyStore_QueryInterface(store, &IID_IPropertyStoreCache, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);
    hr = IPropertyStore_QueryInterface(store, &IID_IPersistSerializedPropStorage, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    hr = IPropertyStore_GetCount(store, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    count = 0xdeadbeef;
    hr = IPropertyStore_GetCount(store, &count);
    ok(hr == S_OK, "Failed to get count, hr %#x.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);

    hr = IPropertyStore_Commit(store);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IPropertyStore_GetAt(store, 0, &key);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IPropertyStore_GetValue(store, NULL, &value);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IPropertyStore_GetValue(store, &test_pkey, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IPropertyStore_GetValue(store, &test_pkey, &value);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    memset(&value, 0, sizeof(PROPVARIANT));
    value.vt = VT_I4;
    value.lVal = 0xdeadbeef;
    hr = IPropertyStore_SetValue(store, &test_pkey, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

if (0)
{
    /* crashes on Windows */
    hr = IPropertyStore_SetValue(store, NULL, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
}

    hr = IPropertyStore_GetCount(store, &count);
    ok(hr == S_OK, "Failed to get count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    hr = IPropertyStore_Commit(store);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IPropertyStore_GetAt(store, 0, &key);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!memcmp(&key, &test_pkey, sizeof(PROPERTYKEY)), "Keys didn't match.\n");

    hr = IPropertyStore_GetAt(store, 1, &key);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    memset(&value, 0xcc, sizeof(PROPVARIANT));
    hr = IPropertyStore_GetValue(store, &test_pkey, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(value.vt == VT_I4, "Unexpected type %u.\n", value.vt);
    ok(value.lVal == 0xdeadbeef, "Unexpected value %#x.\n", value.lVal);

    memset(&value, 0, sizeof(PROPVARIANT));
    hr = IPropertyStore_SetValue(store, &test_pkey, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IPropertyStore_GetCount(store, &count);
    ok(hr == S_OK, "Failed to get count, hr %#x.\n", hr);
    ok(count == 1, "Unexpected count %u.\n", count);

    memset(&value, 0xcc, sizeof(PROPVARIANT));
    hr = IPropertyStore_GetValue(store, &test_pkey, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(value.vt == VT_EMPTY, "Unexpected type %u.\n", value.vt);
    ok(!value.lVal, "Unexpected value %#x.\n", value.lVal);

    refcount = IPropertyStore_Release(store);
    ok(!refcount, "Unexpected refcount %u.\n", refcount);
}

static void test_dxgi_device_manager(void)
{
    IMFDXGIDeviceManager *manager, *manager2;
    ID3D11Device *d3d11_dev, *d3d11_dev2;
    UINT token, token2;
    HRESULT hr;

    if (!pMFCreateDXGIDeviceManager)
    {
        win_skip("MFCreateDXGIDeviceManager not found.\n");
        return;
    }

    hr = pMFCreateDXGIDeviceManager(NULL, &manager);
    ok(hr == E_POINTER, "MFCreateDXGIDeviceManager should failed: %#x.\n", hr);

    token = 0;
    hr = pMFCreateDXGIDeviceManager(&token, NULL);
    ok(hr == E_POINTER, "MFCreateDXGIDeviceManager should failed: %#x.\n", hr);
    ok(!token, "got wrong token: %u.\n", token);

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "MFCreateDXGIDeviceManager failed: %#x.\n", hr);
    EXPECT_REF(manager, 1);
    ok(!!token, "got wrong token: %u.\n", token);

    Sleep(50);
    token2 = 0;
    hr = pMFCreateDXGIDeviceManager(&token2, &manager2);
    ok(hr == S_OK, "MFCreateDXGIDeviceManager failed: %#x.\n", hr);
    EXPECT_REF(manager2, 1);
    ok(token2 && token2 != token, "got wrong token: %u, %u.\n", token2, token);
    ok(manager != manager2, "got wrong pointer: %p.\n", manager2);
    EXPECT_REF(manager, 1);

    hr = pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
                           NULL, 0, D3D11_SDK_VERSION, &d3d11_dev, NULL, NULL);
    ok(hr == S_OK, "D3D11CreateDevice failed: %#x.\n", hr);
    EXPECT_REF(d3d11_dev, 1);

    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)d3d11_dev, token - 1);
    ok(hr == E_INVALIDARG, "IMFDXGIDeviceManager_ResetDevice should failed: %#x.\n", hr);
    EXPECT_REF(d3d11_dev, 1);

    hr = IMFDXGIDeviceManager_ResetDevice(manager, NULL, token);
    ok(hr == E_INVALIDARG, "IMFDXGIDeviceManager_ResetDevice should failed: %#x.\n", hr);

    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)d3d11_dev, token);
    ok(hr == S_OK, "IMFDXGIDeviceManager_ResetDevice failed: %#x.\n", hr);
    EXPECT_REF(manager, 1);
    EXPECT_REF(d3d11_dev, 2);

    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)manager2, token);
    ok(hr == E_INVALIDARG, "IMFDXGIDeviceManager_ResetDevice should failed: %#x.\n", hr);
    EXPECT_REF(manager2, 1);
    EXPECT_REF(d3d11_dev, 2);

    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)d3d11_dev, token);
    ok(hr == S_OK, "IMFDXGIDeviceManager_ResetDevice failed: %#x.\n", hr);
    EXPECT_REF(manager, 1);
    EXPECT_REF(d3d11_dev, 2);

    hr = pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
                           NULL, 0, D3D11_SDK_VERSION, &d3d11_dev2, NULL, NULL);
    ok(hr == S_OK, "D3D11CreateDevice failed: %#x.\n", hr);
    EXPECT_REF(d3d11_dev2, 1);
    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)d3d11_dev2, token);
    ok(hr == S_OK, "IMFDXGIDeviceManager_ResetDevice failed: %#x.\n", hr);
    EXPECT_REF(manager, 1);
    EXPECT_REF(d3d11_dev2, 2);
    EXPECT_REF(d3d11_dev, 1);

    IMFDXGIDeviceManager_Release(manager);
    EXPECT_REF(d3d11_dev2, 1);
    ID3D11Device_Release(d3d11_dev);
    ID3D11Device_Release(d3d11_dev2);
    IMFDXGIDeviceManager_Release(manager2);
}

static void test_MFCreateTransformActivate(void)
{
    IMFActivate *activate;
    UINT32 count;
    HRESULT hr;

    if (!pMFCreateTransformActivate)
    {
        win_skip("MFCreateTransformActivate() is not available.\n");
        return;
    }

    hr = pMFCreateTransformActivate(&activate);
    ok(hr == S_OK, "Failed to create activator, hr %#x.\n", hr);

    hr = IMFActivate_GetCount(activate, &count);
    ok(hr == S_OK, "Failed to get count, hr %#x.\n", hr);
    ok(!count, "Unexpected attribute count %u.\n", count);

    IMFActivate_Release(activate);
}

static HRESULT WINAPI test_mft_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IClassFactory) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_mft_factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI test_mft_factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI test_mft_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_mft_factory_LockServer(IClassFactory *iface, BOOL fLock)
{
    return S_OK;
}

static const IClassFactoryVtbl test_mft_factory_vtbl =
{
    test_mft_factory_QueryInterface,
    test_mft_factory_AddRef,
    test_mft_factory_Release,
    test_mft_factory_CreateInstance,
    test_mft_factory_LockServer,
};

static void test_MFTRegisterLocal(void)
{
    IClassFactory test_factory = { &test_mft_factory_vtbl };
    MFT_REGISTER_TYPE_INFO input_types[1];
    IMFActivate **activate;
    UINT32 count, count2;
    HRESULT hr;

    if (!pMFTRegisterLocal)
    {
        win_skip("MFTRegisterLocal() is not available.\n");
        return;
    }

    input_types[0].guidMajorType = MFMediaType_Audio;
    input_types[0].guidSubtype = MFAudioFormat_PCM;
    hr = pMFTRegisterLocal(&test_factory, &MFT_CATEGORY_OTHER, L"Local MFT name", 0, 1, input_types, 0, NULL);
    ok(hr == S_OK, "Failed to register MFT, hr %#x.\n", hr);

    hr = pMFTRegisterLocal(&test_factory, &MFT_CATEGORY_OTHER, L"Local MFT name", 0, 1, input_types, 0, NULL);
    ok(hr == S_OK, "Failed to register MFT, hr %#x.\n", hr);

    hr = pMFTEnumEx(MFT_CATEGORY_OTHER, MFT_ENUM_FLAG_LOCALMFT, NULL, NULL, &activate, &count);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count > 0, "Unexpected count %u.\n", count);
    CoTaskMemFree(activate);

    hr = pMFTUnregisterLocal(&test_factory);
    ok(hr == S_OK, "Failed to unregister MFT, hr %#x.\n", hr);

    hr = pMFTEnumEx(MFT_CATEGORY_OTHER, MFT_ENUM_FLAG_LOCALMFT, NULL, NULL, &activate, &count2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(count2 < count, "Unexpected count %u.\n", count2);
    CoTaskMemFree(activate);

    hr = pMFTUnregisterLocal(&test_factory);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Unexpected hr %#x.\n", hr);

    hr = pMFTUnregisterLocal(NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = pMFTRegisterLocalByCLSID(&MFT_CATEGORY_OTHER, &MFT_CATEGORY_OTHER, L"Local MFT name 2", 0, 1, input_types,
            0, NULL);
    ok(hr == S_OK, "Failed to register MFT, hr %#x.\n", hr);

    hr = pMFTUnregisterLocal(NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = pMFTUnregisterLocalByCLSID(MFT_CATEGORY_OTHER);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Unexpected hr %#x.\n", hr);

    hr = pMFTRegisterLocalByCLSID(&MFT_CATEGORY_OTHER, &MFT_CATEGORY_OTHER, L"Local MFT name 2", 0, 1, input_types,
            0, NULL);
    ok(hr == S_OK, "Failed to register MFT, hr %#x.\n", hr);

    hr = pMFTUnregisterLocalByCLSID(MFT_CATEGORY_OTHER);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
}

static void test_queue_com(void)
{
    static int system_queues[] =
    {
        MFASYNC_CALLBACK_QUEUE_STANDARD,
        MFASYNC_CALLBACK_QUEUE_RT,
        MFASYNC_CALLBACK_QUEUE_IO,
        MFASYNC_CALLBACK_QUEUE_TIMER,
        MFASYNC_CALLBACK_QUEUE_MULTITHREADED,
        MFASYNC_CALLBACK_QUEUE_LONG_FUNCTION,
    };

    static int user_queues[] =
    {
        MF_STANDARD_WORKQUEUE,
        MF_WINDOW_WORKQUEUE,
        MF_MULTITHREADED_WORKQUEUE,
    };

    char path_name[MAX_PATH];
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char **argv;
    int i;

    if (!pCoGetApartmentType)
    {
        win_skip("CoGetApartmentType() is not available.\n");
        return;
    }

    winetest_get_mainargs(&argv);

    for (i = 0; i < ARRAY_SIZE(system_queues); ++i)
    {
        memset(&startup, 0, sizeof(startup));
        startup.cb = sizeof(startup);
        sprintf(path_name, "%s mfplat s%d", argv[0], system_queues[i]);
        ok(CreateProcessA( NULL, path_name, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info),
                "CreateProcess failed.\n" );
        wait_child_process(info.hProcess);
        CloseHandle(info.hProcess);
        CloseHandle(info.hThread);
    }

    for (i = 0; i < ARRAY_SIZE(user_queues); ++i)
    {
        memset(&startup, 0, sizeof(startup));
        startup.cb = sizeof(startup);
        sprintf(path_name, "%s mfplat u%d", argv[0], user_queues[i]);
        ok(CreateProcessA( NULL, path_name, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info),
                "CreateProcess failed.\n" );
        wait_child_process(info.hProcess);
        CloseHandle(info.hProcess);
        CloseHandle(info.hThread);
    }
}

static HRESULT WINAPI test_queue_com_state_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    APTTYPEQUALIFIER qualifier;
    APTTYPE com_type;
    HRESULT hr;

    hr = pCoGetApartmentType(&com_type, &qualifier);
    ok(SUCCEEDED(hr), "Failed to get apartment type, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
    todo_wine {
        if (callback->param == MFASYNC_CALLBACK_QUEUE_LONG_FUNCTION)
            ok(com_type == APTTYPE_MAINSTA && qualifier == APTTYPEQUALIFIER_NONE,
                "%#x: unexpected type %u, qualifier %u.\n", callback->param, com_type, qualifier);
        else
            ok(com_type == APTTYPE_MTA && qualifier == APTTYPEQUALIFIER_NONE,
                "%#x: unexpected type %u, qualifier %u.\n", callback->param, com_type, qualifier);
    }
    }

    SetEvent(callback->event);
    return S_OK;
}

static const IMFAsyncCallbackVtbl test_queue_com_state_callback_vtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    test_queue_com_state_callback_Invoke,
};

static void test_queue_com_state(const char *name)
{
    struct test_callback callback = { { &test_queue_com_state_callback_vtbl } };
    DWORD queue, queue_type;
    HRESULT hr;

    callback.event = CreateEventA(NULL, FALSE, FALSE, NULL);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#x.\n", hr);

    if (name[0] == 's')
    {
        callback.param = name[1] - '0';
        hr = MFPutWorkItem(callback.param, &callback.IMFAsyncCallback_iface, NULL);
        ok(SUCCEEDED(hr), "Failed to queue work item, hr %#x.\n", hr);
        WaitForSingleObject(callback.event, INFINITE);
    }
    else if (name[0] == 'u')
    {
        queue_type = name[1] - '0';

        hr = pMFAllocateWorkQueueEx(queue_type, &queue);
        ok(hr == S_OK, "Failed to allocate a queue, hr %#x.\n", hr);

        callback.param = queue;
        hr = MFPutWorkItem(queue, &callback.IMFAsyncCallback_iface, NULL);
        ok(SUCCEEDED(hr), "Failed to queue work item, hr %#x.\n", hr);
        WaitForSingleObject(callback.event, INFINITE);

        hr = MFUnlockWorkQueue(queue);
        ok(hr == S_OK, "Failed to unlock the queue, hr %#x.\n", hr);
    }

    CloseHandle(callback.event);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#x.\n", hr);
}

static void test_MFGetStrideForBitmapInfoHeader(void)
{
    static const struct stride_test
    {
        const GUID *subtype;
        unsigned int width;
        LONG stride;
    }
    stride_tests[] =
    {
        { &MFVideoFormat_RGB8, 3, -4 },
        { &MFVideoFormat_RGB8, 1, -4 },
        { &MFVideoFormat_RGB555, 3, -8 },
        { &MFVideoFormat_RGB555, 1, -4 },
        { &MFVideoFormat_RGB565, 3, -8 },
        { &MFVideoFormat_RGB565, 1, -4 },
        { &MFVideoFormat_RGB24, 3, -12 },
        { &MFVideoFormat_RGB24, 1, -4 },
        { &MFVideoFormat_RGB32, 3, -12 },
        { &MFVideoFormat_RGB32, 1, -4 },
        { &MFVideoFormat_ARGB32, 3, -12 },
        { &MFVideoFormat_ARGB32, 1, -4 },
        { &MFVideoFormat_A2R10G10B10, 3, -12 },
        { &MFVideoFormat_A2R10G10B10, 1, -4 },
        { &MFVideoFormat_A16B16G16R16F, 3, -24 },
        { &MFVideoFormat_A16B16G16R16F, 1, -8 },

        /* YUV */
        { &MFVideoFormat_NV12, 1, 1 },
        { &MFVideoFormat_NV12, 2, 2 },
        { &MFVideoFormat_NV12, 3, 3 },
        { &MFVideoFormat_AYUV, 1, 4 },
        { &MFVideoFormat_AYUV, 4, 16 },
        { &MFVideoFormat_AYUV, 5, 20 },
        { &MFVideoFormat_IMC1, 1, 4 },
        { &MFVideoFormat_IMC1, 2, 4 },
        { &MFVideoFormat_IMC1, 3, 8 },
        { &MFVideoFormat_IMC3, 1, 4 },
        { &MFVideoFormat_IMC3, 2, 4 },
        { &MFVideoFormat_IMC3, 3, 8 },
        { &MFVideoFormat_IMC2, 1, 1 },
        { &MFVideoFormat_IMC2, 2, 2 },
        { &MFVideoFormat_IMC2, 3, 3 },
        { &MFVideoFormat_IMC4, 1, 1 },
        { &MFVideoFormat_IMC4, 2, 2 },
        { &MFVideoFormat_IMC4, 3, 3 },
        { &MFVideoFormat_YV12, 1, 1 },
        { &MFVideoFormat_YV12, 2, 2 },
        { &MFVideoFormat_YV12, 3, 3 },
        { &MFVideoFormat_YV12, 320, 320 },
        { &MFVideoFormat_I420, 1, 1 },
        { &MFVideoFormat_I420, 2, 2 },
        { &MFVideoFormat_I420, 3, 3 },
        { &MFVideoFormat_I420, 320, 320 },
    };
    unsigned int i;
    LONG stride;
    HRESULT hr;

    if (!pMFGetStrideForBitmapInfoHeader)
    {
        win_skip("MFGetStrideForBitmapInfoHeader() is not available.\n");
        return;
    }

    hr = pMFGetStrideForBitmapInfoHeader(MAKEFOURCC('H','2','6','4'), 1, &stride);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(stride_tests); ++i)
    {
        hr = pMFGetStrideForBitmapInfoHeader(stride_tests[i].subtype->Data1, stride_tests[i].width, &stride);
        ok(hr == S_OK, "%u: failed to get stride, hr %#x.\n", i, hr);
        ok(stride == stride_tests[i].stride, "%u: format %s, unexpected stride %d, expected %d.\n", i,
                wine_dbgstr_an((char *)&stride_tests[i].subtype->Data1, 4), stride, stride_tests[i].stride);
    }
}

static void test_MFCreate2DMediaBuffer(void)
{
    static const struct _2d_buffer_test
    {
        unsigned int width;
        unsigned int height;
        unsigned int fourcc;
        unsigned int contiguous_length;
        int pitch;
        unsigned int plane_multiplier;
    } _2d_buffer_tests[] =
    {
        { 2, 2, MAKEFOURCC('N','V','1','2'), 6, 64 },
        { 4, 2, MAKEFOURCC('N','V','1','2'), 12, 64 },
        { 2, 4, MAKEFOURCC('N','V','1','2'), 12, 64 },
        { 1, 3, MAKEFOURCC('N','V','1','2'), 4, 64 },

        { 2, 2, MAKEFOURCC('I','M','C','2'), 6, 128 },
        { 4, 2, MAKEFOURCC('I','M','C','2'), 12, 128 },
        { 2, 4, MAKEFOURCC('I','M','C','2'), 12, 128 },
        { 2, 2, MAKEFOURCC('I','M','C','4'), 6, 128 },
        { 4, 2, MAKEFOURCC('I','M','C','4'), 12, 128 },
        { 2, 4, MAKEFOURCC('I','M','C','4'), 12, 128 },

        { 4,  2, MAKEFOURCC('I','M','C','1'),  32, 128, 2 },
        { 4,  4, MAKEFOURCC('I','M','C','1'),  64, 128, 2 },
        { 4, 16, MAKEFOURCC('I','M','C','1'), 256, 128, 2 },
        { 4, 20, MAKEFOURCC('I','M','C','1'), 320, 128, 2 },

        { 4,  2, MAKEFOURCC('I','M','C','3'),  32, 128, 2 },
        { 4,  4, MAKEFOURCC('I','M','C','3'),  64, 128, 2 },
        { 4, 16, MAKEFOURCC('I','M','C','3'), 256, 128, 2 },
        { 4, 20, MAKEFOURCC('I','M','C','3'), 320, 128, 2 },

        { 4,  2, MAKEFOURCC('Y','V','1','2'),  12, 128 },
        { 4,  4, MAKEFOURCC('Y','V','1','2'),  24, 128 },
        { 4, 16, MAKEFOURCC('Y','V','1','2'),  96, 128 },

        { 4,  2, MAKEFOURCC('A','Y','U','V'),  32, 64 },
        { 4,  4, MAKEFOURCC('A','Y','U','V'),  64, 64 },
        { 4, 16, MAKEFOURCC('A','Y','U','V'), 256, 64 },

        { 4,  2, MAKEFOURCC('Y','U','Y','2'),  16, 64 },
        { 4,  4, MAKEFOURCC('Y','U','Y','2'),  32, 64 },
        { 4, 16, MAKEFOURCC('Y','U','Y','2'), 128, 64 },

        { 4,  2, MAKEFOURCC('U','Y','V','Y'),  16, 64 },
        { 4,  4, MAKEFOURCC('U','Y','V','Y'),  32, 64 },
        { 4, 16, MAKEFOURCC('U','Y','V','Y'), 128, 64 },

        { 2, 4, D3DFMT_A8R8G8B8, 32, 64 },
        { 1, 4, D3DFMT_A8R8G8B8, 16, 64 },
        { 4, 1, D3DFMT_A8R8G8B8, 16, 64 },
    };
    unsigned int max_length, length, length2;
    BYTE *buffer_start, *data, *data2;
    IMF2DBuffer2 *_2dbuffer2;
    IMF2DBuffer *_2dbuffer;
    IMFMediaBuffer *buffer;
    int i, pitch, pitch2;
    HRESULT hr;
    BOOL ret;

    if (!pMFCreate2DMediaBuffer)
    {
        win_skip("MFCreate2DMediaBuffer() is not available.\n");
        return;
    }

    hr = pMFCreate2DMediaBuffer(2, 3, MAKEFOURCC('H','2','6','4'), FALSE, &buffer);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    hr = pMFCreate2DMediaBuffer(2, 3, MAKEFOURCC('N','V','1','2'), FALSE, NULL);
    ok(FAILED(hr), "Unexpected hr %#x.\n", hr);

    /* YUV formats can't be bottom-up. */
    hr = pMFCreate2DMediaBuffer(2, 3, MAKEFOURCC('N','V','1','2'), TRUE, &buffer);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#x.\n", hr);

    hr = pMFCreate2DMediaBuffer(2, 3, MAKEFOURCC('N','V','1','2'), FALSE, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#x.\n", hr);

    /* Full backing buffer size, with 64 bytes per row alignment.  */
    hr = IMFMediaBuffer_GetMaxLength(buffer, &max_length);
    ok(hr == S_OK, "Failed to get length, hr %#x.\n", hr);
    ok(max_length > 0, "Unexpected length %u.\n", max_length);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get current length, hr %#x.\n", hr);
    ok(!length, "Unexpected length.\n");

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 10);
    ok(hr == S_OK, "Failed to set current length, hr %#x.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get current length, hr %#x.\n", hr);
    ok(length == 10, "Unexpected length.\n");

    /* Linear lock/unlock. */

    hr = IMFMediaBuffer_Lock(buffer, NULL, &max_length, &length);
    ok(FAILED(hr), "Unexpected hr %#x.\n", hr);

    /* Linear locking call returns plane size.*/
    hr = IMFMediaBuffer_Lock(buffer, &data, &max_length, &length);
    ok(hr == S_OK, "Failed to lock buffer, hr %#x.\n", hr);
    ok(max_length == length, "Unexpected length.\n");

    length = 0;
    pMFGetPlaneSize(MAKEFOURCC('N','V','1','2'), 2, 3, &length);
    ok(max_length == length && length == 9, "Unexpected length %u.\n", length);

    /* Already locked */
    hr = IMFMediaBuffer_Lock(buffer, &data2, NULL, NULL);
    ok(hr == S_OK, "Failed to lock buffer, hr %#x.\n", hr);
    ok(data2 == data, "Unexpected pointer.\n");

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock buffer, hr %#x.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&_2dbuffer);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);

    hr = IMF2DBuffer_GetContiguousLength(_2dbuffer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_GetContiguousLength(_2dbuffer, &length);
    ok(hr == S_OK, "Failed to get length, hr %#x.\n", hr);
    ok(length == 9, "Unexpected length %u.\n", length);

    hr = IMF2DBuffer_IsContiguousFormat(_2dbuffer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    /* 2D lock. */
    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
    ok(hr == MF_E_UNEXPECTED, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, &data, &pitch);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock buffer, hr %#x.\n", hr);

    hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, &data, &pitch);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, NULL, &pitch);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
    ok(hr == S_OK, "Failed to lock buffer, hr %#x.\n", hr);
    ok(!!data, "Expected data pointer.\n");
    ok(pitch == 64, "Unexpected pitch %d.\n", pitch);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data2, &pitch);
    ok(hr == S_OK, "Failed to lock buffer, hr %#x.\n", hr);
    ok(data == data2, "Expected data pointer.\n");

    hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, NULL, &pitch);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, &data, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    /* Active 2D lock */
    hr = IMFMediaBuffer_Lock(buffer, &data2, NULL, NULL);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2dbuffer);
    ok(hr == S_OK, "Failed to unlock buffer, hr %#x.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data2, NULL, NULL);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2dbuffer);
    ok(hr == S_OK, "Failed to unlock buffer, hr %#x.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2dbuffer);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&_2dbuffer2);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE), "Failed to get interface, hr %#x.\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
        ok(hr == S_OK, "Failed to lock buffer, hr %#x.\n", hr);

        hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data2, &pitch, &buffer_start, &length);
        ok(hr == S_OK, "Failed to lock buffer, hr %#x.\n", hr);

        hr = IMF2DBuffer_Unlock2D(_2dbuffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#x.\n", hr);

        hr = IMF2DBuffer_Unlock2D(_2dbuffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#x.\n", hr);

        /* Flags are ignored. */
        hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data2, &pitch, &buffer_start, &length);
        ok(hr == S_OK, "Failed to lock buffer, hr %#x.\n", hr);

        hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data2, &pitch, &buffer_start, &length);
        ok(hr == S_OK, "Failed to lock buffer, hr %#x.\n", hr);

        hr = IMF2DBuffer_Unlock2D(_2dbuffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#x.\n", hr);

        hr = IMF2DBuffer_Unlock2D(_2dbuffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#x.\n", hr);

        hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data2, &pitch, NULL, &length);
        ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

        hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data2, &pitch, &buffer_start, NULL);
        ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

        IMF2DBuffer2_Release(_2dbuffer2);
    }
    else
        win_skip("IMF2DBuffer2 is not supported.\n");

    IMF2DBuffer_Release(_2dbuffer);

    IMFMediaBuffer_Release(buffer);

    for (i = 0; i < ARRAY_SIZE(_2d_buffer_tests); ++i)
    {
        const struct _2d_buffer_test *ptr = &_2d_buffer_tests[i];

        hr = pMFCreate2DMediaBuffer(ptr->width, ptr->height, ptr->fourcc, FALSE, &buffer);
        ok(hr == S_OK, "Failed to create a buffer, hr %#x.\n", hr);

        hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&_2dbuffer);
        ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);

        hr = IMF2DBuffer_GetContiguousLength(_2dbuffer, &length);
        ok(hr == S_OK, "Failed to get length, hr %#x.\n", hr);
        ok(length == ptr->contiguous_length, "%d: unexpected contiguous length %u for %u x %u, format %s.\n",
                i, length, ptr->width, ptr->height, wine_dbgstr_an((char *)&ptr->fourcc, 4));

        hr = IMFMediaBuffer_Lock(buffer, &data, &length2, NULL);
        ok(hr == S_OK, "Failed to lock buffer, hr %#x.\n", hr);
        ok(length == ptr->contiguous_length, "%d: unexpected linear buffer length %u for %u x %u, format %s.\n",
                i, length2, ptr->width, ptr->height, wine_dbgstr_an((char *)&ptr->fourcc, 4));

        hr = IMFMediaBuffer_Unlock(buffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#x.\n", hr);

        hr = pMFGetPlaneSize(ptr->fourcc, ptr->width, ptr->height, &length2);
        ok(hr == S_OK, "Failed to get plane size, hr %#x.\n", hr);
        if (ptr->plane_multiplier)
            length2 *= ptr->plane_multiplier;
        ok(length2 == length, "%d: contiguous length %u does not match plane size %u, %u x %u, format %s.\n", i, length,
                length2, ptr->width, ptr->height, wine_dbgstr_an((char *)&ptr->fourcc, 4));

        hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
        ok(hr == S_OK, "Failed to lock buffer, hr %#x.\n", hr);

        hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, &data2, &pitch2);
        ok(hr == S_OK, "Failed to get scanline, hr %#x.\n", hr);
        ok(data2 == data, "Unexpected data pointer.\n");
        ok(pitch == pitch2, "Unexpected pitch.\n");

        hr = IMF2DBuffer_Unlock2D(_2dbuffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#x.\n", hr);

        ok(pitch == ptr->pitch, "%d: unexpected pitch %d, expected %d, %u x %u, format %s.\n", i, pitch, ptr->pitch,
                ptr->width, ptr->height, wine_dbgstr_an((char *)&ptr->fourcc, 4));

        ret = TRUE;
        hr = IMF2DBuffer_IsContiguousFormat(_2dbuffer, &ret);
        ok(hr == S_OK, "Failed to get format flag, hr %#x.\n", hr);
        ok(!ret, "%d: unexpected format flag %d.\n", i, ret);

        IMF2DBuffer_Release(_2dbuffer);

        IMFMediaBuffer_Release(buffer);
    }
}

static void test_MFCreateMediaBufferFromMediaType(void)
{
    static struct audio_buffer_test
    {
        unsigned int duration;
        unsigned int min_length;
        unsigned int min_alignment;
        unsigned int block_alignment;
        unsigned int bytes_per_second;
        unsigned int buffer_length;
    } audio_tests[] =
    {
        { 0,  0,  0,  4,  0, 20 },
        { 0, 16,  0,  4,  0, 20 },
        { 0,  0, 32,  4,  0, 36 },
        { 0, 64, 32,  4,  0, 64 },
        { 1,  0,  0,  4, 16, 36 },
        { 2,  0,  0,  4, 16, 52 },
    };
    IMFMediaBuffer *buffer;
    UINT32 length;
    HRESULT hr;
    IMFMediaType *media_type;
    unsigned int i;

    if (!pMFCreateMediaBufferFromMediaType)
    {
        win_skip("MFCreateMediaBufferFromMediaType() is not available.\n");
        return;
    }

    hr = pMFCreateMediaBufferFromMediaType(NULL, 0, 0, 0, &buffer);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(audio_tests); ++i)
    {
        const struct audio_buffer_test *ptr = &audio_tests[i];

        hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, ptr->block_alignment);
        ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

        hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, ptr->bytes_per_second);
        ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

        hr = pMFCreateMediaBufferFromMediaType(media_type, ptr->duration * 10000000, ptr->min_length,
                ptr->min_alignment, &buffer);
        ok(hr == S_OK || broken(FAILED(hr)) /* Win8 */, "Unexpected hr %#x.\n", hr);
        if (FAILED(hr))
            break;

        hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
        ok(hr == S_OK, "Failed to get length, hr %#x.\n", hr);
        ok(ptr->buffer_length == length, "%d: unexpected buffer length %u, expected %u.\n", i, length, ptr->buffer_length);

        IMFMediaBuffer_Release(buffer);
    }

    IMFMediaType_Release(media_type);
}

static void validate_media_type(IMFMediaType *mediatype, const WAVEFORMATEX *format)
{
    GUID guid, subtype;
    UINT32 value;
    HRESULT hr;

    hr = IMFMediaType_GetMajorType(mediatype, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#x.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Audio), "Unexpected major type %s.\n", wine_dbgstr_guid(&guid));

    hr = IMFMediaType_GetGUID(mediatype, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Failed to get subtype, hr %#x.\n", hr);

    if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        const WAVEFORMATEXTENSIBLE *fex = (const WAVEFORMATEXTENSIBLE *)format;
        ok(IsEqualGUID(&guid, &fex->SubFormat), "Unexpected subtype %s.\n", wine_dbgstr_guid(&guid));

        if (fex->dwChannelMask)
        {
            hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_CHANNEL_MASK, &value);
            ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);
            ok(value == fex->dwChannelMask, "Unexpected CHANNEL_MASK %#x.\n", value);
        }

        if (format->wBitsPerSample && fex->Samples.wValidBitsPerSample)
        {
            hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, &value);
            ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);
            ok(value == fex->Samples.wValidBitsPerSample, "Unexpected VALID_BITS_PER_SAMPLE %#x.\n", value);
        }
    }
    else
    {
        memcpy(&subtype, &MFAudioFormat_Base, sizeof(subtype));
        subtype.Data1 = format->wFormatTag;
        ok(IsEqualGUID(&guid, &subtype), "Unexpected subtype %s.\n", wine_dbgstr_guid(&guid));

        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_PREFER_WAVEFORMATEX, &value);
        ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);
        ok(value, "Unexpected value.\n");
    }

    if (format->nChannels)
    {
        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_NUM_CHANNELS, &value);
        ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);
        ok(value == format->nChannels, "Unexpected NUM_CHANNELS %u.\n", value);
    }

    if (format->nSamplesPerSec)
    {
        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &value);
        ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);
        ok(value == format->nSamplesPerSec, "Unexpected SAMPLES_PER_SECOND %u.\n", value);
    }

    if (format->nAvgBytesPerSec)
    {
        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &value);
        ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);
        ok(value == format->nAvgBytesPerSec, "Unexpected AVG_BYTES_PER_SECOND %u.\n", value);
    }

    if (format->nBlockAlign)
    {
        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &value);
        ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);
        ok(value == format->nBlockAlign, "Unexpected BLOCK_ALIGNMENT %u.\n", value);
    }

    if (format->wBitsPerSample)
    {
        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_BITS_PER_SAMPLE, &value);
        ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);
        ok(value == format->wBitsPerSample, "Unexpected BITS_PER_SAMPLE %u.\n", value);
    }

    /* Only set for uncompressed formats. */
    hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value);
    if (IsEqualGUID(&guid, &MFAudioFormat_Float) ||
            IsEqualGUID(&guid, &MFAudioFormat_PCM))
    {
        ok(hr == S_OK, "Failed to get attribute, hr %#x.\n", hr);
        ok(value, "Unexpected ALL_SAMPLES_INDEPENDENT value.\n");
    }
    else
        ok(FAILED(hr), "Unexpected ALL_SAMPLES_INDEPENDENT.\n");
}

static void test_MFInitMediaTypeFromWaveFormatEx(void)
{
    static const WAVEFORMATEX waveformatex_tests[] =
    {
        { WAVE_FORMAT_PCM, 2, 44100, 0, 2, 8 },
        { WAVE_FORMAT_PCM, 2, 44100, 1, 2, 8 },
        { WAVE_FORMAT_PCM, 0, 44100, 0, 0, 0 },
        { WAVE_FORMAT_PCM, 0,     0, 0, 0, 0 },
        { WAVE_FORMAT_IEEE_FLOAT, 2, 44100, 1, 2, 8 },
        { 1234, 0,     0, 0, 0, 0 },
        { WAVE_FORMAT_ALAW },
        { WAVE_FORMAT_CREATIVE_ADPCM },
        { WAVE_FORMAT_MPEGLAYER3 },
        { WAVE_FORMAT_MPEG_ADTS_AAC },
        { WAVE_FORMAT_ALAC },
        { WAVE_FORMAT_AMR_NB },
        { WAVE_FORMAT_AMR_WB },
        { WAVE_FORMAT_AMR_WP },
        { WAVE_FORMAT_DOLBY_AC3_SPDIF },
        { WAVE_FORMAT_DRM },
        { WAVE_FORMAT_DTS },
        { WAVE_FORMAT_FLAC },
        { WAVE_FORMAT_MPEG },
        { WAVE_FORMAT_WMAVOICE9 },
        { WAVE_FORMAT_OPUS },
        { WAVE_FORMAT_WMAUDIO2 },
        { WAVE_FORMAT_WMAUDIO3 },
        { WAVE_FORMAT_WMAUDIO_LOSSLESS },
        { WAVE_FORMAT_WMASPDIF },
    };
    WAVEFORMATEXTENSIBLE waveformatext;
    IMFMediaType *mediatype;
    unsigned int i;
    HRESULT hr;

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create mediatype, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(waveformatex_tests); ++i)
    {
        hr = MFInitMediaTypeFromWaveFormatEx(mediatype, &waveformatex_tests[i], sizeof(waveformatex_tests[i]));
        ok(hr == S_OK, "%d: format %#x, failed to initialize media type, hr %#x.\n", i, waveformatex_tests[i].wFormatTag, hr);

        validate_media_type(mediatype, &waveformatex_tests[i]);

        waveformatext.Format = waveformatex_tests[i];
        waveformatext.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        waveformatext.Format.cbSize = sizeof(waveformatext) - sizeof(waveformatext.Format);
        waveformatext.Samples.wSamplesPerBlock = 123;
        waveformatext.dwChannelMask = 0x8;
        memcpy(&waveformatext.SubFormat, &MFAudioFormat_Base, sizeof(waveformatext.SubFormat));
        waveformatext.SubFormat.Data1 = waveformatex_tests[i].wFormatTag;
        hr = MFInitMediaTypeFromWaveFormatEx(mediatype, &waveformatext.Format, sizeof(waveformatext));
        ok(hr == S_OK, "Failed to initialize media type, hr %#x.\n", hr);

        validate_media_type(mediatype, &waveformatext.Format);
    }

    IMFMediaType_Release(mediatype);
}

static void test_MFCreateMFVideoFormatFromMFMediaType(void)
{
    MFVIDEOFORMAT *video_format;
    IMFMediaType *media_type;
    UINT32 size;
    HRESULT hr;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#x.\n", hr);

    hr = MFCreateMFVideoFormatFromMFMediaType(media_type, &video_format, &size);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!!video_format, "Unexpected format.\n");
    ok(video_format->dwSize == size && size == sizeof(*video_format), "Unexpected size %u.\n", size);
    CoTaskMemFree(video_format);

    IMFMediaType_Release(media_type);
}

static void test_MFCreateDXSurfaceBuffer(void)
{
    IDirect3DSurface9 *backbuffer = NULL, *surface;
    IDirect3DSwapChain9 *swapchain;
    IDirect3DDevice9 *device;
    IMF2DBuffer2 *_2dbuffer2;
    IMFMediaBuffer *buffer;
    IMF2DBuffer *_2dbuffer;
    BYTE *data, *data2;
    IMFGetService *gs;
    IDirect3D9 *d3d;
    DWORD length;
    HWND window;
    HRESULT hr;
    LONG pitch;
    BOOL value;

    if (!pMFCreateDXSurfaceBuffer)
    {
        skip("MFCreateDXSurfaceBuffer is not available.\n");
        return;
    }

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get the implicit swapchain (%08x)\n", hr);

    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%08x)\n", hr);
    ok(backbuffer != NULL, "The back buffer is NULL\n");

    IDirect3DSwapChain9_Release(swapchain);

    hr = pMFCreateDXSurfaceBuffer(&IID_IDirect3DSurface9, (IUnknown *)backbuffer, FALSE, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#x.\n", hr);

    /* Surface is accessible. */
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFGetService, (void **)&gs);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    hr = IMFGetService_GetService(gs, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, (void **)&surface);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(surface == backbuffer, "Unexpected surface pointer.\n");
    IDirect3DSurface9_Release(surface);
    IMFGetService_Release(gs);

    length = 0;
    hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!!length, "Unexpected length %u.\n", length);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get length, hr %#x.\n", hr);
    ok(!length, "Unexpected length %u.\n", length);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Unlock twice. */
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#x.\n", hr);

    /* Lock twice. */
    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data2, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(data == data2, "Unexpected pointer.\n");

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&_2dbuffer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* Unlocked. */
    hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, &data, &pitch);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data2, NULL, NULL);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2dbuffer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2dbuffer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2dbuffer);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer_IsContiguousFormat(_2dbuffer, &value);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!value, "Unexpected return value %d.\n", value);

    IMF2DBuffer_Release(_2dbuffer);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(data == data2, "Unexpected scanline pointer.\n");
    memset(data, 0xab, 4);
    IMF2DBuffer2_Unlock2D(_2dbuffer2);

    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(data[0] == 0xab, "Unexpected leading byte.\n");
    IMF2DBuffer2_Unlock2D(_2dbuffer2);

    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_ReadWrite, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IMF2DBuffer2_Unlock2D(_2dbuffer2);

    IMF2DBuffer2_Release(_2dbuffer2);

    IMFMediaBuffer_Release(buffer);

done:
    if (backbuffer)
        IDirect3DSurface9_Release(backbuffer);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_MFCreateTrackedSample(void)
{
    IMFTrackedSample *tracked_sample;
    IMFDesiredSample *desired_sample;
    IMFSample *sample;
    IUnknown *unk;
    HRESULT hr;

    if (!pMFCreateTrackedSample)
    {
        win_skip("MFCreateTrackedSample() is not available.\n");
        return;
    }

    hr = pMFCreateTrackedSample(&tracked_sample);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    /* It's actually a sample. */
    hr = IMFTrackedSample_QueryInterface(tracked_sample, &IID_IMFSample, (void **)&sample);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMFTrackedSample_QueryInterface(tracked_sample, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unk == (IUnknown *)sample, "Unexpected pointer.\n");
    IUnknown_Release(unk);

    IMFSample_Release(sample);

    hr = IMFTrackedSample_QueryInterface(tracked_sample, &IID_IMFDesiredSample, (void **)&desired_sample);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#x.\n", hr);

    IMFTrackedSample_Release(tracked_sample);
}

START_TEST(mfplat)
{
    char **argv;
    int argc;

    init_functions();

    argc = winetest_get_mainargs(&argv);
    if (argc >= 3)
    {
        test_queue_com_state(argv[2]);
        return;
    }

    CoInitialize(NULL);

    test_startup();
    test_register();
    test_media_type();
    test_MFCreateMediaEvent();
    test_attributes();
    test_sample();
    test_file_stream();
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
    test_stream_descriptor();
    test_MFCalculateImageSize();
    test_MFCompareFullToPartialMediaType();
    test_attributes_serialization();
    test_wrapped_media_type();
    test_MFCreateWaveFormatExFromMFMediaType();
    test_async_create_file();
    test_local_handlers();
    test_create_property_store();
    test_dxgi_device_manager();
    test_MFCreateTransformActivate();
    test_MFTRegisterLocal();
    test_queue_com();
    test_MFGetStrideForBitmapInfoHeader();
    test_MFCreate2DMediaBuffer();
    test_MFCreateMediaBufferFromMediaType();
    test_MFInitMediaTypeFromWaveFormatEx();
    test_MFCreateMFVideoFormatFromMFMediaType();
    test_MFCreateDXSurfaceBuffer();
    test_MFCreateTrackedSample();

    CoUninitialize();
}
