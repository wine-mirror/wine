/*
 * OLE DB Marshaling Tests
 *
 * Copyright 2009 Robert Shearman
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

#define _WIN32_DCOM
#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "oledb.h"

#include "wine/test.h"

#define RELEASEMARSHALDATA WM_USER

#define ok_ole_success(hr, func) ok(hr == S_OK, #func " failed with error %#08lx\n", hr)

struct host_object_data
{
    IStream *stream;
    IID iid;
    IUnknown *object;
    MSHLFLAGS marshal_flags;
    HANDLE marshal_event;
    IMessageFilter *filter;
};

static DWORD CALLBACK host_object_proc(LPVOID p)
{
    struct host_object_data *data = p;
    HRESULT hr;
    MSG msg;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (data->filter)
    {
        IMessageFilter * prev_filter = NULL;
        hr = CoRegisterMessageFilter(data->filter, &prev_filter);
        if (prev_filter) IMessageFilter_Release(prev_filter);
        ok_ole_success(hr, CoRegisterMessageFilter);
    }

    hr = CoMarshalInterface(data->stream, &data->iid, data->object, MSHCTX_INPROC, NULL, data->marshal_flags);
    ok_ole_success(hr, CoMarshalInterface);

    /* force the message queue to be created before signaling parent thread */
    PeekMessageW(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    SetEvent(data->marshal_event);

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (msg.hwnd == NULL && msg.message == RELEASEMARSHALDATA)
        {
            CoReleaseMarshalData(data->stream);
            SetEvent((HANDLE)msg.lParam);
        }
        else
            DispatchMessageW(&msg);
    }

    free(data);

    CoUninitialize();

    return hr;
}

static DWORD start_host_object2(IStream *stream, REFIID riid, IUnknown *object, MSHLFLAGS marshal_flags, IMessageFilter *filter, HANDLE *thread)
{
    DWORD tid = 0;
    HANDLE marshal_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    struct host_object_data *data = malloc(sizeof(*data));

    data->stream = stream;
    data->iid = *riid;
    data->object = object;
    data->marshal_flags = marshal_flags;
    data->marshal_event = marshal_event;
    data->filter = filter;

    *thread = CreateThread(NULL, 0, host_object_proc, data, 0, &tid);

    /* wait for marshaling to complete before returning */
    ok( !WaitForSingleObject(marshal_event, 10000), "wait timed out\n" );
    CloseHandle(marshal_event);

    return tid;
}

static DWORD start_host_object(IStream *stream, REFIID riid, IUnknown *object, MSHLFLAGS marshal_flags, HANDLE *thread)
{
    return start_host_object2(stream, riid, object, marshal_flags, NULL, thread);
}

static void end_host_object(DWORD tid, HANDLE thread)
{
    BOOL ret = PostThreadMessageW(tid, WM_QUIT, 0, 0);
    ok(ret, "PostThreadMessageW failed with error %ld\n", GetLastError());
    /* be careful of races - don't return until hosting thread has terminated */
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    CloseHandle(thread);
}

#define TEST_PROPID 0xdead
static const WCHAR wszDBPropertyTestString[] = {'D','B','P','r','o','p','e','r','t','y','T','e','s','t','S','t','r','i','n','g',0};
static const WCHAR wszDBPropertyColumnName[] = {'C','o','l','u','m','n',0};

static HRESULT WINAPI Test_DBProperties_QueryInterface(
    IDBProperties* iface,
    REFIID riid,
    void **ppvObject)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDBProperties))
    {
        *ppvObject = iface;
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Test_DBProperties_AddRef(
        IDBProperties* iface)
{
    return 2;
}

static ULONG WINAPI Test_DBProperties_Release(
        IDBProperties* iface)
{
    return 1;
}

    /*** IDBProperties methods ***/
static HRESULT WINAPI Test_DBProperties_GetProperties(
        IDBProperties* iface,
        ULONG cPropertyIDSets,
        const DBPROPIDSET rgPropertyIDSets[],
        ULONG *pcPropertySets,
        DBPROPSET **prgPropertySets)
{
    ok(cPropertyIDSets == 0, "Expected cPropertyIDSets to be 0 instead of %ld\n", cPropertyIDSets);
    ok(*pcPropertySets == 0, "Expected *pcPropertySets to be 0 instead of %ld\n", *pcPropertySets);
    *pcPropertySets = 1;
    *prgPropertySets = CoTaskMemAlloc(sizeof(DBPROPSET));
    (*prgPropertySets)[0].rgProperties = CoTaskMemAlloc(sizeof(DBPROP));
    (*prgPropertySets)[0].rgProperties[0].dwPropertyID = TEST_PROPID;
    (*prgPropertySets)[0].rgProperties[0].dwOptions = DBPROPOPTIONS_REQUIRED;
    (*prgPropertySets)[0].rgProperties[0].dwStatus = S_OK;
    (*prgPropertySets)[0].rgProperties[0].colid.eKind = DBKIND_GUID_NAME;
    /* colid contents */
    (*prgPropertySets)[0].rgProperties[0].colid.uGuid.guid = IID_IDBProperties;
    (*prgPropertySets)[0].rgProperties[0].colid.uName.pwszName = CoTaskMemAlloc(sizeof(wszDBPropertyColumnName));
    memcpy((*prgPropertySets)[0].rgProperties[0].colid.uName.pwszName, wszDBPropertyColumnName, sizeof(wszDBPropertyColumnName));
    /* vValue contents */
    V_VT(&(*prgPropertySets)[0].rgProperties[0].vValue) = VT_BSTR;
    V_BSTR(&(*prgPropertySets)[0].rgProperties[0].vValue) = SysAllocString(wszDBPropertyTestString);
    (*prgPropertySets)[0].cProperties = 1;
    (*prgPropertySets)[0].guidPropertySet = IID_IDBProperties;

    return S_OK;
}

static HRESULT WINAPI Test_DBProperties_GetPropertyInfo(
        IDBProperties* iface,
        ULONG cPropertyIDSets,
        const DBPROPIDSET rgPropertyIDSets[],
        ULONG *pcPropertyInfoSets,
        DBPROPINFOSET **prgPropertyInfoSets,
        OLECHAR **ppDescBuffer)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI Test_DBProperties_SetProperties(
        IDBProperties* iface,
        ULONG cPropertySets,
        DBPROPSET rgPropertySets[])
{
    return E_NOTIMPL;
}

static const IDBPropertiesVtbl Test_DBProperties_Vtbl =
{
    Test_DBProperties_QueryInterface,
    Test_DBProperties_AddRef,
    Test_DBProperties_Release,
    Test_DBProperties_GetProperties,
    Test_DBProperties_GetPropertyInfo,
    Test_DBProperties_SetProperties,
};

static IDBProperties Test_DBProperties =
{
    &Test_DBProperties_Vtbl
};

static void free_dbpropset(ULONG count, DBPROPSET *propset)
{
    ULONG i;

    for (i = 0; i < count; i++)
    {
        ULONG p;

        for (p = 0; p < propset[i].cProperties; p++)
            VariantClear(&propset[i].rgProperties[p].vValue);

        CoTaskMemFree(propset[i].rgProperties);
    }
    CoTaskMemFree(propset);
}

static void test_IDBProperties(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IDBProperties *pProxy = NULL;
    DWORD tid;
    HANDLE thread;
    static const LARGE_INTEGER ullZero;
    ULONG propset_count;
    DBPROPSET *propsets = NULL;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");
    tid = start_host_object(pStream, &IID_IDBProperties, (IUnknown*)&Test_DBProperties, MSHLFLAGS_NORMAL, &thread);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IDBProperties, (void **)&pProxy);
    ok_ole_success(hr, "CoUnmarshalInterface");
    IStream_Release(pStream);

    propset_count = 1;
    hr = IDBProperties_GetProperties(pProxy, 0, NULL, &propset_count, &propsets);
    ok(hr == S_OK, "IDBProperties_GetProperties failed with error 0x%08lx\n", hr);

    ok(propset_count == 1, "Expected propset_count of 1 but got %ld\n", propset_count);
    ok(propsets->rgProperties[0].dwPropertyID == TEST_PROPID, "Expected property ID of 0x%x, but got 0x%lx\n", TEST_PROPID, propsets->rgProperties[0].dwPropertyID);
    ok(propsets->rgProperties[0].dwOptions == DBPROPOPTIONS_REQUIRED, "Expected property options of 0x%x, but got 0x%lx\n", DBPROPOPTIONS_REQUIRED, propsets->rgProperties[0].dwOptions);
    ok(propsets->rgProperties[0].dwStatus == S_OK, "Expected property options of 0x%lx, but got 0x%lx\n", S_OK, propsets->rgProperties[0].dwStatus);
    ok(propsets->rgProperties[0].colid.eKind == DBKIND_GUID_NAME, "Expected property colid kind of DBKIND_GUID_NAME, but got %ld\n", propsets->rgProperties[0].colid.eKind);
    /* colid contents */
    ok(IsEqualGUID(&propsets->rgProperties[0].colid.uGuid.guid, &IID_IDBProperties), "Unexpected property colid guid\n");
    ok(!lstrcmpW(propsets->rgProperties[0].colid.uName.pwszName, wszDBPropertyColumnName), "Unexpected property colid name\n");
    /* vValue contents */
    ok(V_VT(&propsets->rgProperties[0].vValue) == VT_BSTR, "Expected property value vt of VT_BSTR, but got %d\n", V_VT(&propsets->rgProperties[0].vValue));
    ok(!lstrcmpW(V_BSTR(&propsets->rgProperties[0].vValue), wszDBPropertyTestString), "Unexpected property value string\n");
    ok(propsets->cProperties == 1, "Expected property count of 1 but got %ld\n", propsets->cProperties);
    ok(IsEqualGUID(&propsets->guidPropertySet, &IID_IDBProperties), "Unexpected guid for property set\n");

    free_dbpropset(propset_count, propsets);

    IDBProperties_Release(pProxy);

    end_host_object(tid, thread);
}

START_TEST(marshal)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_IDBProperties();
    CoUninitialize();
}
