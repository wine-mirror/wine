/*
 * Marshaling Tests
 *
 * Copyright 2004 Robert Shearman
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define _WIN32_DCOM
#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wine/test.h"

/* functions that are not present on all versions of Windows */
HRESULT (WINAPI * pCoInitializeEx)(LPVOID lpReserved, DWORD dwCoInit);

/* helper macros to make tests a bit leaner */
#define ok_more_than_one_lock() ok(cLocks > 0, "Number of locks should be > 0, but actually is %ld\n", cLocks)
#define ok_no_locks() ok(cLocks == 0, "Number of locks should be 0, but actually is %ld\n", cLocks)
#define ok_ole_success(hr, func) ok(hr == S_OK, #func " failed with error 0x%08lx\n", hr)

static const IID IID_IWineTest =
{
    0x5201163f,
    0x8164,
    0x4fd0,
    {0xa1, 0xa2, 0x5d, 0x5a, 0x36, 0x54, 0xd3, 0xbd}
}; /* 5201163f-8164-4fd0-a1a2-5d5a3654d3bd */

static void test_CoGetPSClsid(void)
{
	HRESULT hr;
	CLSID clsid;

	hr = CoGetPSClsid(&IID_IClassFactory, &clsid);
	ok_ole_success(hr, CoGetPSClsid);

	hr = CoGetPSClsid(&IID_IWineTest, &clsid);
	ok(hr == REGDB_E_IIDNOTREG,
	   "CoGetPSClsid for random IID returned 0x%08lx instead of REGDB_E_IIDNOTREG\n",
	   hr);
}

static const LARGE_INTEGER ullZero;
static LONG cLocks;

static void LockModule(void)
{
    InterlockedIncrement(&cLocks);
}

static void UnlockModule(void)
{
    InterlockedDecrement(&cLocks);
}


static HRESULT WINAPI Test_IUnknown_QueryInterface(
    LPUNKNOWN iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObj = (LPVOID)iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Test_IUnknown_AddRef(LPUNKNOWN iface)
{
    LockModule();
    return 2; /* non-heap-based object */
}

static ULONG WINAPI Test_IUnknown_Release(LPUNKNOWN iface)
{
    UnlockModule();
    return 1; /* non-heap-based object */
}

static const IUnknownVtbl TestUnknown_Vtbl =
{
    Test_IUnknown_QueryInterface,
    Test_IUnknown_AddRef,
    Test_IUnknown_Release,
};

static IUnknown Test_Unknown = { &TestUnknown_Vtbl };


static HRESULT WINAPI Test_IClassFactory_QueryInterface(
    LPCLASSFACTORY iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = (LPVOID)iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Test_IClassFactory_AddRef(LPCLASSFACTORY iface)
{
    LockModule();
    return 2; /* non-heap-based object */
}

static ULONG WINAPI Test_IClassFactory_Release(LPCLASSFACTORY iface)
{
    UnlockModule();
    return 1; /* non-heap-based object */
}

static HRESULT WINAPI Test_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (pUnkOuter) return CLASS_E_NOAGGREGATION;
    return IUnknown_QueryInterface((IUnknown*)&Test_Unknown, riid, ppvObj);
}

static HRESULT WINAPI Test_IClassFactory_LockServer(
    LPCLASSFACTORY iface,
    BOOL fLock)
{
    return S_OK;
}

static const IClassFactoryVtbl TestClassFactory_Vtbl =
{
    Test_IClassFactory_QueryInterface,
    Test_IClassFactory_AddRef,
    Test_IClassFactory_Release,
    Test_IClassFactory_CreateInstance,
    Test_IClassFactory_LockServer
};

static IClassFactory Test_ClassFactory = { &TestClassFactory_Vtbl };

#define RELEASEMARSHALDATA WM_USER

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
    struct host_object_data *data = (struct host_object_data *)p;
    HRESULT hr;
    MSG msg;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

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
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    SetEvent(data->marshal_event);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.hwnd == NULL && msg.message == RELEASEMARSHALDATA)
        {
            trace("releasing marshal data\n");
            CoReleaseMarshalData(data->stream);
            SetEvent((HANDLE)msg.lParam);
        }
        else
            DispatchMessage(&msg);
    }

    HeapFree(GetProcessHeap(), 0, data);

    CoUninitialize();

    return hr;
}

static DWORD start_host_object2(IStream *stream, REFIID riid, IUnknown *object, MSHLFLAGS marshal_flags, IMessageFilter *filter, HANDLE *thread)
{
    DWORD tid = 0;
    HANDLE marshal_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    struct host_object_data *data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data));

    data->stream = stream;
    data->iid = *riid;
    data->object = object;
    data->marshal_flags = marshal_flags;
    data->marshal_event = marshal_event;
    data->filter = filter;

    *thread = CreateThread(NULL, 0, host_object_proc, data, 0, &tid);

    /* wait for marshaling to complete before returning */
    WaitForSingleObject(marshal_event, INFINITE);
    CloseHandle(marshal_event);

    return tid;
}

static DWORD start_host_object(IStream *stream, REFIID riid, IUnknown *object, MSHLFLAGS marshal_flags, HANDLE *thread)
{
    return start_host_object2(stream, riid, object, marshal_flags, NULL, thread);
}

/* asks thread to release the marshal data because it has to be done by the
 * same thread that marshaled the interface in the first place. */
static void release_host_object(DWORD tid)
{
    HANDLE event = CreateEvent(NULL, FALSE, FALSE, NULL);
    PostThreadMessage(tid, RELEASEMARSHALDATA, 0, (LPARAM)event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
}

static void end_host_object(DWORD tid, HANDLE thread)
{
    BOOL ret = PostThreadMessage(tid, WM_QUIT, 0, 0);
    ok(ret, "PostThreadMessage failed with error %ld\n", GetLastError());
    /* be careful of races - don't return until hosting thread has terminated */
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

/* tests failure case of interface not having a marshaler specified in the
 * registry */
static void test_no_marshaler(void)
{
    IStream *pStream;
    HRESULT hr;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IWineTest, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == E_NOINTERFACE, "CoMarshalInterface should have returned E_NOINTERFACE instead of 0x%08lx\n", hr);

    IStream_Release(pStream);
}

/* tests normal marshal and then release without unmarshaling */
static void test_normal_marshal_and_release(void)
{
    HRESULT hr;
    IStream *pStream = NULL;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoReleaseMarshalData(pStream);
    ok_ole_success(hr, CoReleaseMarshalData);
    IStream_Release(pStream);

    ok_no_locks();
}

/* tests success case of a same-thread marshal and unmarshal */
static void test_normal_marshal_and_unmarshal(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    IUnknown_Release(pProxy);

    ok_no_locks();
}

/* tests failure case of unmarshaling a freed object */
static void test_marshal_and_unmarshal_invalid(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IClassFactory *pProxy = NULL;
    DWORD tid;
    void * dummy;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
	
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoReleaseMarshalData(pStream);
    ok_ole_success(hr, CoReleaseMarshalData);

    ok_no_locks();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    todo_wine { ok_ole_success(hr, CoUnmarshalInterface); }

    ok_no_locks();

    if (pProxy)
    {
        hr = IClassFactory_CreateInstance(pProxy, NULL, &IID_IUnknown, &dummy);
        ok(hr == RPC_E_DISCONNECTED, "Remote call should have returned RPC_E_DISCONNECTED, instead of 0x%08lx\n", hr);

        IClassFactory_Release(pProxy);
    }

    IStream_Release(pStream);

    end_host_object(tid, thread);
}

/* tests success case of an interthread marshal */
static void test_interthread_marshal_and_unmarshal(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    IUnknown_Release(pProxy);

    ok_no_locks();

    end_host_object(tid, thread);
}

/* tests success case of an interthread marshal and then marshaling the proxy */
static void test_proxy_marshal_and_unmarshal(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    IUnknown *pProxy2 = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    /* marshal the proxy */
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, pProxy, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    /* unmarshal the second proxy to the object */
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    /* now the proxies should be as follows:
     *  pProxy -> &Test_ClassFactory
     *  pProxy2 -> &Test_ClassFactory
     * they should NOT be as follows:
     *  pProxy -> &Test_ClassFactory
     *  pProxy2 -> pProxy
     * the above can only really be tested by looking in +ole traces
     */

    ok_more_than_one_lock();

    IUnknown_Release(pProxy);

    ok_more_than_one_lock();

    IUnknown_Release(pProxy2);

    ok_no_locks();

    end_host_object(tid, thread);
}

/* tests success case of an interthread marshal and then marshaling the proxy
 * using an iid that hasn't previously been unmarshaled */
static void test_proxy_marshal_and_unmarshal2(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    IUnknown *pProxy2 = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IUnknown, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    /* marshal the proxy */
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, pProxy, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    /* unmarshal the second proxy to the object */
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    /* now the proxies should be as follows:
     *  pProxy -> &Test_ClassFactory
     *  pProxy2 -> &Test_ClassFactory
     * they should NOT be as follows:
     *  pProxy -> &Test_ClassFactory
     *  pProxy2 -> pProxy
     * the above can only really be tested by looking in +ole traces
     */

    ok_more_than_one_lock();

    IUnknown_Release(pProxy);

    ok_more_than_one_lock();

    IUnknown_Release(pProxy2);

    ok_no_locks();

    end_host_object(tid, thread);
}

/* tests that stubs are released when the containing apartment is destroyed */
static void test_marshal_stub_apartment_shutdown(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    end_host_object(tid, thread);

    ok_no_locks();

    IUnknown_Release(pProxy);

    ok_no_locks();
}

/* tests that proxies are released when the containing apartment is destroyed */
static void test_marshal_proxy_apartment_shutdown(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    CoUninitialize();

    ok_no_locks();

    IUnknown_Release(pProxy);

    ok_no_locks();

    end_host_object(tid, thread);

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
}

/* tests that proxies are released when the containing mta apartment is destroyed */
static void test_marshal_proxy_mta_apartment_shutdown(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid;
    HANDLE thread;

    CoUninitialize();
    pCoInitializeEx(NULL, COINIT_MULTITHREADED);

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    CoUninitialize();

    ok_no_locks();

    IUnknown_Release(pProxy);

    ok_no_locks();

    end_host_object(tid, thread);

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
}

struct ncu_params
{
    LPSTREAM stream;
    HANDLE marshal_event;
    HANDLE unmarshal_event;
};

/* helper for test_no_couninitialize_server */
static DWORD CALLBACK no_couninitialize_server_proc(LPVOID p)
{
    struct ncu_params *ncu_params = (struct ncu_params *)p;
    HRESULT hr;

    pCoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoMarshalInterface(ncu_params->stream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    SetEvent(ncu_params->marshal_event);

    WaitForSingleObject(ncu_params->unmarshal_event, INFINITE);

    /* die without calling CoUninitialize */

    return 0;
}

/* tests apartment that an apartment with a stub is released without deadlock
 * if the owning thread exits */
static void test_no_couninitialize_server(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid;
    HANDLE thread;
    struct ncu_params ncu_params;

    cLocks = 0;

    ncu_params.marshal_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    ncu_params.unmarshal_event = CreateEvent(NULL, TRUE, FALSE, NULL);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    ncu_params.stream = pStream;

    thread = CreateThread(NULL, 0, no_couninitialize_server_proc, &ncu_params, 0, &tid);

    WaitForSingleObject(ncu_params.marshal_event, INFINITE);
    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    SetEvent(ncu_params.unmarshal_event);
    WaitForSingleObject(thread, INFINITE);

    ok_no_locks();

    CloseHandle(thread);
    CloseHandle(ncu_params.marshal_event);
    CloseHandle(ncu_params.unmarshal_event);

    IUnknown_Release(pProxy);

    ok_no_locks();
}

/* STA -> STA call during DLL_THREAD_DETACH */
static DWORD CALLBACK no_couninitialize_client_proc(LPVOID p)
{
    struct ncu_params *ncu_params = (struct ncu_params *)p;
    HRESULT hr;
    IUnknown *pProxy = NULL;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoUnmarshalInterface(ncu_params->stream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    /* die without calling CoUninitialize */

    return 0;
}

/* tests STA -> STA call during DLL_THREAD_DETACH doesn't deadlock */
static void test_no_couninitialize_client(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    DWORD tid;
    DWORD host_tid;
    HANDLE thread;
    HANDLE host_thread;
    struct ncu_params ncu_params;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    ncu_params.stream = pStream;

    /* NOTE: assumes start_host_object uses an STA to host the object, as MTAs
     * always deadlock when called from within DllMain */
    host_tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown *)&Test_ClassFactory, MSHLFLAGS_NORMAL, &host_thread);
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);

    ok_more_than_one_lock();

    thread = CreateThread(NULL, 0, no_couninitialize_client_proc, &ncu_params, 0, &tid);

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    ok_no_locks();

    end_host_object(host_tid, host_thread);
}

/* tests success case of a same-thread table-weak marshal, unmarshal, unmarshal */
static void test_tableweak_marshal_and_unmarshal_twice(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy1 = NULL;
    IUnknown *pProxy2 = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_TABLEWEAK, &thread);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy1);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    IUnknown_Release(pProxy1);
    IUnknown_Release(pProxy2);

    /* this line is shows the difference between weak and strong table marshaling:
     *  weak has cLocks == 0
     *  strong has cLocks > 0 */
    ok_no_locks();

    end_host_object(tid, thread);
}

/* tests releasing after unmarshaling one object */
static void test_tableweak_marshal_releasedata1(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy1 = NULL;
    IUnknown *pProxy2 = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_TABLEWEAK, &thread);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy1);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    /* release the remaining reference on the object by calling
     * CoReleaseMarshalData in the hosting thread */
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    release_host_object(tid);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    IUnknown_Release(pProxy1);
    if (pProxy2)
        IUnknown_Release(pProxy2);

    /* this line is shows the difference between weak and strong table marshaling:
     *  weak has cLocks == 0
     *  strong has cLocks > 0 */
    ok_no_locks();

    end_host_object(tid, thread);
}

/* tests releasing after unmarshaling one object */
static void test_tableweak_marshal_releasedata2(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_TABLEWEAK, &thread);

    ok_more_than_one_lock();

    /* release the remaining reference on the object by calling
     * CoReleaseMarshalData in the hosting thread */
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    release_host_object(tid);

    ok_no_locks();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    todo_wine
    {
    ok(hr == CO_E_OBJNOTREG,
       "CoUnmarshalInterface should have failed with CO_E_OBJNOTREG, but returned 0x%08lx instead\n",
       hr);
    }
    IStream_Release(pStream);

    ok_no_locks();

    end_host_object(tid, thread);
}

/* tests success case of a same-thread table-strong marshal, unmarshal, unmarshal */
static void test_tablestrong_marshal_and_unmarshal_twice(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy1 = NULL;
    IUnknown *pProxy2 = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_TABLESTRONG, &thread);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy1);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    if (pProxy1) IUnknown_Release(pProxy1);
    if (pProxy2) IUnknown_Release(pProxy2);

    /* this line is shows the difference between weak and strong table marshaling:
     *  weak has cLocks == 0
     *  strong has cLocks > 0 */
    ok_more_than_one_lock();

    /* release the remaining reference on the object by calling
     * CoReleaseMarshalData in the hosting thread */
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    release_host_object(tid);
    IStream_Release(pStream);

    ok_no_locks();

    end_host_object(tid, thread);
}

/* tests CoLockObjectExternal */
static void test_lock_object_external(void)
{
    HRESULT hr;
    IStream *pStream = NULL;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    CoLockObjectExternal((IUnknown*)&Test_ClassFactory, TRUE, TRUE);

    ok_more_than_one_lock();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoReleaseMarshalData(pStream);
    ok_ole_success(hr, CoReleaseMarshalData);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    CoLockObjectExternal((IUnknown*)&Test_ClassFactory, FALSE, TRUE);

    ok_no_locks();
}

/* tests disconnecting stubs */
static void test_disconnect_stub(void)
{
    HRESULT hr;
    IStream *pStream = NULL;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    CoLockObjectExternal((IUnknown*)&Test_ClassFactory, TRUE, TRUE);

    ok_more_than_one_lock();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoReleaseMarshalData(pStream);
    ok_ole_success(hr, CoReleaseMarshalData);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    CoDisconnectObject((IUnknown*)&Test_ClassFactory, 0);

    ok_no_locks();
}

/* tests failure case of a same-thread marshal and unmarshal twice */
static void test_normal_marshal_and_unmarshal_twice(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy1 = NULL;
    IUnknown *pProxy2 = NULL;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy1);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    ok(hr == CO_E_OBJNOTCONNECTED,
        "CoUnmarshalInterface should have failed with error CO_E_OBJNOTCONNECTED for double unmarshal, instead of 0x%08lx\n", hr);

    IStream_Release(pStream);

    ok_more_than_one_lock();

    IUnknown_Release(pProxy1);

    ok_no_locks();
}

/* tests success case of marshaling and unmarshaling an HRESULT */
static void test_hresult_marshaling(void)
{
    HRESULT hr;
    HRESULT hr_marshaled = 0;
    IStream *pStream = NULL;
    static const HRESULT E_DEADBEEF = 0xdeadbeef;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);

    hr = CoMarshalHresult(pStream, E_DEADBEEF);
    ok_ole_success(hr, CoMarshalHresult);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = IStream_Read(pStream, &hr_marshaled, sizeof(HRESULT), NULL);
    ok_ole_success(hr, IStream_Read);

    ok(hr_marshaled == E_DEADBEEF, "Didn't marshal HRESULT as expected: got value 0x%08lx instead\n", hr_marshaled);

    hr_marshaled = 0;
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalHresult(pStream, &hr_marshaled);
    ok_ole_success(hr, CoUnmarshalHresult);

    ok(hr_marshaled == E_DEADBEEF, "Didn't marshal HRESULT as expected: got value 0x%08lx instead\n", hr_marshaled);

    IStream_Release(pStream);
}


/* helper for test_proxy_used_in_wrong_thread */
static DWORD CALLBACK bad_thread_proc(LPVOID p)
{
    IClassFactory * cf = (IClassFactory *)p;
    HRESULT hr;
    IUnknown * proxy = NULL;

    pCoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (LPVOID*)&proxy);
    if (proxy) IUnknown_Release(proxy);
    ok(hr == RPC_E_WRONG_THREAD,
        "COM should have failed with RPC_E_WRONG_THREAD on using proxy from wrong apartment, but instead returned 0x%08lx\n",
        hr);

    CoUninitialize();

    return 0;
}

/* tests failure case of a using a proxy in the wrong apartment */
static void test_proxy_used_in_wrong_thread(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid, tid2;
    HANDLE thread;
    HANDLE host_thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &host_thread);

    ok_more_than_one_lock();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    /* create a thread that we can misbehave in */
    thread = CreateThread(NULL, 0, bad_thread_proc, (LPVOID)pProxy, 0, &tid2);

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    IUnknown_Release(pProxy);

    ok_no_locks();

    end_host_object(tid, host_thread);
}

static HRESULT WINAPI MessageFilter_QueryInterface(IMessageFilter *iface, REFIID riid, void ** ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = (LPVOID)iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI MessageFilter_AddRef(IMessageFilter *iface)
{
    return 2; /* non-heap object */
}

static ULONG WINAPI MessageFilter_Release(IMessageFilter *iface)
{
    return 1; /* non-heap object */
}

static DWORD WINAPI MessageFilter_HandleInComingCall(
  IMessageFilter *iface,
  DWORD dwCallType,
  HTASK threadIDCaller,
  DWORD dwTickCount,
  LPINTERFACEINFO lpInterfaceInfo)
{
    static int callcount = 0;
    DWORD ret;
    trace("HandleInComingCall\n");
    switch (callcount)
    {
    case 0:
        ret = SERVERCALL_REJECTED;
        break;
    case 1:
        ret = SERVERCALL_RETRYLATER;
        break;
    default:
        ret = SERVERCALL_ISHANDLED;
        break;
    }
    callcount++;
    return ret;
}

static DWORD WINAPI MessageFilter_RetryRejectedCall(
  IMessageFilter *iface,
  HTASK threadIDCallee,
  DWORD dwTickCount,
  DWORD dwRejectType)
{
    trace("RetryRejectedCall\n");
    return 0;
}

static DWORD WINAPI MessageFilter_MessagePending(
  IMessageFilter *iface,
  HTASK threadIDCallee,
  DWORD dwTickCount,
  DWORD dwPendingType)
{
    trace("MessagePending\n");
    return PENDINGMSG_WAITNOPROCESS;
}

static const IMessageFilterVtbl MessageFilter_Vtbl =
{
    MessageFilter_QueryInterface,
    MessageFilter_AddRef,
    MessageFilter_Release,
    MessageFilter_HandleInComingCall,
    MessageFilter_RetryRejectedCall,
    MessageFilter_MessagePending
};

static IMessageFilter MessageFilter = { &MessageFilter_Vtbl };

static void test_message_filter(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IClassFactory *cf = NULL;
    DWORD tid;
    IUnknown *proxy = NULL;
    IMessageFilter *prev_filter = NULL;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object2(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &MessageFilter, &thread);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&cf);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    hr = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (LPVOID*)&proxy);
    todo_wine { ok(hr == RPC_E_CALL_REJECTED, "Call should have returned RPC_E_CALL_REJECTED, but return 0x%08lx instead\n", hr); }
    if (proxy) IUnknown_Release(proxy);
    proxy = NULL;

    hr = CoRegisterMessageFilter(&MessageFilter, &prev_filter);
    ok_ole_success(hr, CoRegisterMessageFilter);
    if (prev_filter) IMessageFilter_Release(prev_filter);

    hr = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (LPVOID*)&proxy);
    ok_ole_success(hr, IClassFactory_CreateInstance);

    IUnknown_Release(proxy);

    IClassFactory_Release(cf);

    ok_no_locks();

    end_host_object(tid, thread);
}

/* test failure case of trying to unmarshal from bad stream */
static void test_bad_marshal_stream(void)
{
    HRESULT hr;
    IStream *pStream = NULL;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();

    /* try to read beyond end of stream */
    hr = CoReleaseMarshalData(pStream);
    ok(hr == STG_E_READFAULT, "Should have failed with STG_E_READFAULT, but returned 0x%08lx instead\n", hr);

    /* now release for real */
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoReleaseMarshalData(pStream);
    ok_ole_success(hr, CoReleaseMarshalData);

    IStream_Release(pStream);
}

/* tests that proxies implement certain interfaces */
static void test_proxy_interfaces(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    IUnknown *pOtherUnknown = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
	
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    hr = IUnknown_QueryInterface(pProxy, &IID_IUnknown, (LPVOID*)&pOtherUnknown);
    ok_ole_success(hr, IUnknown_QueryInterface IID_IUnknown);
    if (hr == S_OK) IUnknown_Release(pOtherUnknown);

    hr = IUnknown_QueryInterface(pProxy, &IID_IClientSecurity, (LPVOID*)&pOtherUnknown);
    todo_wine { ok_ole_success(hr, IUnknown_QueryInterface IID_IClientSecurity); }
    if (hr == S_OK) IUnknown_Release(pOtherUnknown);

    hr = IUnknown_QueryInterface(pProxy, &IID_IMultiQI, (LPVOID*)&pOtherUnknown);
    ok_ole_success(hr, IUnknown_QueryInterface IID_IMultiQI);
    if (hr == S_OK) IUnknown_Release(pOtherUnknown);

    hr = IUnknown_QueryInterface(pProxy, &IID_IMarshal, (LPVOID*)&pOtherUnknown);
    ok_ole_success(hr, IUnknown_QueryInterface IID_IMarshal);
    if (hr == S_OK) IUnknown_Release(pOtherUnknown);

    /* IMarshal2 is also supported on NT-based systems, but is pretty much
     * useless as it has no more methods over IMarshal that it inherits from. */

    IUnknown_Release(pProxy);

    ok_no_locks();

    end_host_object(tid, thread);
}

typedef struct
{
    const IUnknownVtbl *lpVtbl;
    ULONG refs;
} HeapUnknown;

static HRESULT WINAPI HeapUnknown_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *ppv = (LPVOID)iface;
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI HeapUnknown_AddRef(IUnknown *iface)
{
    HeapUnknown *This = (HeapUnknown *)iface;
    trace("HeapUnknown_AddRef(%p)\n", iface);
    return InterlockedIncrement((LONG*)&This->refs);
}

static ULONG WINAPI HeapUnknown_Release(IUnknown *iface)
{
    HeapUnknown *This = (HeapUnknown *)iface;
    ULONG refs = InterlockedDecrement((LONG*)&This->refs);
    trace("HeapUnknown_Release(%p)\n", iface);
    if (!refs) HeapFree(GetProcessHeap(), 0, This);
    return refs;
}

static const IUnknownVtbl HeapUnknown_Vtbl =
{
    HeapUnknown_QueryInterface,
    HeapUnknown_AddRef,
    HeapUnknown_Release
};

static void test_proxybuffer(REFIID riid)
{
    HRESULT hr;
    IPSFactoryBuffer *psfb;
    IRpcProxyBuffer *proxy;
    LPVOID lpvtbl;
    ULONG refs;
    CLSID clsid;
    HeapUnknown *pUnkOuter = (HeapUnknown *)HeapAlloc(GetProcessHeap(), 0, sizeof(*pUnkOuter));

    pUnkOuter->lpVtbl = &HeapUnknown_Vtbl;
    pUnkOuter->refs = 1;

    hr = CoGetPSClsid(riid, &clsid);
    ok_ole_success(hr, CoGetPSClsid);

    hr = CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IPSFactoryBuffer, (LPVOID*)&psfb);
    ok_ole_success(hr, CoGetClassObject);

    hr = IPSFactoryBuffer_CreateProxy(psfb, (IUnknown*)pUnkOuter, riid, &proxy, &lpvtbl);
    ok_ole_success(hr, IPSFactoryBuffer_CreateProxy);
    ok(lpvtbl != NULL, "IPSFactoryBuffer_CreateProxy succeeded, but returned a NULL vtable!\n");

    refs = IPSFactoryBuffer_Release(psfb);
#if 0 /* not reliable on native. maybe it leaks references! */
    ok(refs == 0, "Ref-count leak of %ld on IPSFactoryBuffer\n", refs);
#endif

    refs = IUnknown_Release((IUnknown *)lpvtbl);
    ok(refs == 1, "Ref-count leak of %ld on IRpcProxyBuffer\n", refs-1);

    refs = IRpcProxyBuffer_Release(proxy);
    ok(refs == 0, "Ref-count leak of %ld on IRpcProxyBuffer\n", refs);
}

static void test_stubbuffer(REFIID riid)
{
    HRESULT hr;
    IPSFactoryBuffer *psfb;
    IRpcStubBuffer *stub;
    ULONG refs;
    CLSID clsid;

    cLocks = 0;

    hr = CoGetPSClsid(riid, &clsid);
    ok_ole_success(hr, CoGetPSClsid);

    hr = CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IPSFactoryBuffer, (LPVOID*)&psfb);
    ok_ole_success(hr, CoGetClassObject);

    hr = IPSFactoryBuffer_CreateStub(psfb, riid, (IUnknown*)&Test_ClassFactory, &stub);
    ok_ole_success(hr, IPSFactoryBuffer_CreateStub);

    refs = IPSFactoryBuffer_Release(psfb);
#if 0 /* not reliable on native. maybe it leaks references */
    ok(refs == 0, "Ref-count leak of %ld on IPSFactoryBuffer\n", refs);
#endif

    ok_more_than_one_lock();

    IRpcStubBuffer_Disconnect(stub);

    ok_no_locks();

    refs = IRpcStubBuffer_Release(stub);
    ok(refs == 0, "Ref-count leak of %ld on IRpcProxyBuffer\n", refs);
}

static HWND hwnd_app;

static HRESULT WINAPI TestRE_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    DWORD_PTR res;
    if (IsEqualIID(riid, &IID_IWineTest))
    {
        BOOL ret = SendMessageTimeout(hwnd_app, WM_NULL, 0, 0, SMTO_BLOCK, 5000, &res);
        ok(ret, "Timed out sending a message to originating window during RPC call\n");
    }
    return S_FALSE;
}

static const IClassFactoryVtbl TestREClassFactory_Vtbl =
{
    Test_IClassFactory_QueryInterface,
    Test_IClassFactory_AddRef,
    Test_IClassFactory_Release,
    TestRE_IClassFactory_CreateInstance,
    Test_IClassFactory_LockServer
};

IClassFactory TestRE_ClassFactory = { &TestREClassFactory_Vtbl };

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_USER:
    {
        HRESULT hr;
        IStream *pStream = NULL;
        IClassFactory *proxy = NULL;
        IUnknown *object;
        DWORD tid;
        HANDLE thread;

        cLocks = 0;

        hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
        ok_ole_success(hr, CreateStreamOnHGlobal);
        tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&TestRE_ClassFactory, MSHLFLAGS_NORMAL, &thread);

        ok_more_than_one_lock();

        IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
        hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&proxy);
        ok_ole_success(hr, CoReleaseMarshalData);
        IStream_Release(pStream);

        ok_more_than_one_lock();

        /* note the use of the magic IID_IWineTest value to tell remote thread
         * to try to send a message back to us */
        hr = IClassFactory_CreateInstance(proxy, NULL, &IID_IWineTest, (void **)&object);

        IClassFactory_Release(proxy);

        ok_no_locks();

        end_host_object(tid, thread);

        PostMessage(hwnd, WM_QUIT, 0, 0);

        return 0;
    }
    case WM_USER+1:
    {
        HRESULT hr;
        IStream *pStream = NULL;
        IClassFactory *proxy = NULL;
        IUnknown *object;
        DWORD tid;
        HANDLE thread;

        cLocks = 0;

        hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
        ok_ole_success(hr, CreateStreamOnHGlobal);
        tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&TestRE_ClassFactory, MSHLFLAGS_NORMAL, &thread);

        ok_more_than_one_lock();

        IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
        hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&proxy);
        ok_ole_success(hr, CoReleaseMarshalData);
        IStream_Release(pStream);

        ok_more_than_one_lock();

        /* post quit message before a doing a COM call to show that a pending
        * WM_QUIT message doesn't stop the call from succeeding */
        PostMessage(hwnd, WM_QUIT, 0, 0);
        hr = IClassFactory_CreateInstance(proxy, NULL, &IID_IUnknown, (void **)&object);

        IClassFactory_Release(proxy);

        ok_no_locks();

        end_host_object(tid, thread);

        return 0;
    }
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

static void test_message_reentrancy(void)
{
    WNDCLASS wndclass;
    MSG msg;

    memset(&wndclass, 0, sizeof(wndclass));
    wndclass.lpfnWndProc = window_proc;
    wndclass.lpszClassName = "WineCOMTest";
    RegisterClass(&wndclass);

    hwnd_app = CreateWindow("WineCOMTest", NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, 0);
    ok(hwnd_app != NULL, "Window creation failed\n");

    /* start message re-entrancy test */
    PostMessage(hwnd_app, WM_USER, 0, 0);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static void test_WM_QUIT_handling(void)
{
    MSG msg;

    hwnd_app = CreateWindow("WineCOMTest", NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, 0);
    ok(hwnd_app != NULL, "Window creation failed\n");

    /* start WM_QUIT handling test */
    PostMessage(hwnd_app, WM_USER+1, 0, 0);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

/* doesn't pass with Win9x COM DLLs (even though Essential COM says it should) */
#if 0

static HANDLE heventShutdown;

static void LockModuleOOP()
{
    InterlockedIncrement(&cLocks); /* for test purposes only */
    CoAddRefServerProcess();
}

static void UnlockModuleOOP()
{
    InterlockedDecrement(&cLocks); /* for test purposes only */
    if (!CoReleaseServerProcess())
        SetEvent(heventShutdown);
}

static HWND hwnd_app;

static HRESULT WINAPI TestOOP_IClassFactory_QueryInterface(
    LPCLASSFACTORY iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = (LPVOID)iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI TestOOP_IClassFactory_AddRef(LPCLASSFACTORY iface)
{
    return 2; /* non-heap-based object */
}

static ULONG WINAPI TestOOP_IClassFactory_Release(LPCLASSFACTORY iface)
{
    return 1; /* non-heap-based object */
}

static HRESULT WINAPI TestOOP_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    return CLASS_E_CLASSNOTAVAILABLE;
}

static HRESULT WINAPI TestOOP_IClassFactory_LockServer(
    LPCLASSFACTORY iface,
    BOOL fLock)
{
    if (fLock)
        LockModuleOOP();
    else
        UnlockModuleOOP();
    return S_OK;
}

static const IClassFactoryVtbl TestClassFactoryOOP_Vtbl =
{
    TestOOP_IClassFactory_QueryInterface,
    TestOOP_IClassFactory_AddRef,
    TestOOP_IClassFactory_Release,
    TestOOP_IClassFactory_CreateInstance,
    TestOOP_IClassFactory_LockServer
};

static IClassFactory TestOOP_ClassFactory = { &TestClassFactoryOOP_Vtbl };

/* tests functions commonly used by out of process COM servers */
static void test_out_of_process_com(void)
{
    static const CLSID CLSID_WineOOPTest = {
        0x5201163f,
        0x8164,
        0x4fd0,
        {0xa1, 0xa2, 0x5d, 0x5a, 0x36, 0x54, 0xd3, 0xbd}
    }; /* 5201163f-8164-4fd0-a1a2-5d5a3654d3bd */
    DWORD cookie;
    HRESULT hr;
    IClassFactory * cf;
    DWORD ret;

    heventShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);

    cLocks = 0;

    /* Start the object suspended */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&TestOOP_ClassFactory,
        CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED, &cookie);
    ok_ole_success(hr, CoRegisterClassObject);

    /* ... and CoGetClassObject does not find it and fails when it looks for the
     * class in the registry */
    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER,
        NULL, &IID_IClassFactory, (LPVOID*)&cf);
    todo_wine {
    ok(hr == REGDB_E_CLASSNOTREG,
        "CoGetClassObject should have returned REGDB_E_CLASSNOTREG instead of 0x%08lx\n", hr);
    }

    /* Resume the object suspended above ... */
    hr = CoResumeClassObjects();
    ok_ole_success(hr, CoResumeClassObjects);

    /* ... and now it should succeed */
    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER,
        NULL, &IID_IClassFactory, (LPVOID*)&cf);
    ok_ole_success(hr, CoGetClassObject);

    /* Now check the locking is working */
    /* NOTE: we are accessing the class directly, not through a proxy */

    ok_no_locks();

    hr = IClassFactory_LockServer(cf, TRUE);
    trace("IClassFactory_LockServer returned 0x%08lx\n", hr);

    ok_more_than_one_lock();
    
    IClassFactory_LockServer(cf, FALSE);

    ok_no_locks();

    IClassFactory_Release(cf);

    /* wait for shutdown signal */
    ret = WaitForSingleObject(heventShutdown, 5000);
    todo_wine { ok(ret != WAIT_TIMEOUT, "Server didn't shut down or machine is under very heavy load\n"); }

    /* try to connect again after SCM has suspended registered class objects */
    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, NULL,
        &IID_IClassFactory, (LPVOID*)&cf);
    todo_wine {
    ok(hr == CO_E_SERVER_STOPPING,
        "CoGetClassObject should have returned CO_E_SERVER_STOPPING instead of 0x%08lx\n", hr);
    }

    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, CoRevokeClassObject);

    CloseHandle(heventShutdown);
}
#endif

static void test_ROT(void)
{
    static const WCHAR wszFileName[] = {'B','E','2','0','E','2','F','5','-',
        '1','9','0','3','-','4','A','A','E','-','B','1','A','F','-',
        '2','0','4','6','E','5','8','6','C','9','2','5',0};
    HRESULT hr;
    IMoniker *pMoniker = NULL;
    IRunningObjectTable *pROT = NULL;
    DWORD dwCookie;

    cLocks = 0;

    hr = CreateFileMoniker(wszFileName, &pMoniker);
    ok_ole_success(hr, CreateClassMoniker);
    hr = GetRunningObjectTable(0, &pROT);
    ok_ole_success(hr, GetRunningObjectTable);
    hr = IRunningObjectTable_Register(pROT, 0, (IUnknown*)&Test_ClassFactory, pMoniker, &dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Register);

    ok_more_than_one_lock();

    hr = IRunningObjectTable_Revoke(pROT, dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Revoke);

    ok_no_locks();
}

static const char cf_marshaled[] =
{
    0x9, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0,
    0x9, 0x0, 0x0, 0x0,
    'M', 0x0, 'y', 0x0,
    'F', 0x0, 'o', 0x0,
    'r', 0x0, 'm', 0x0,
    'a', 0x0, 't', 0x0,
    0x0, 0x0
};

static void test_marshal_CLIPFORMAT(void)
{
    unsigned char *buffer;
    unsigned long size;
    unsigned long flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
    wireCLIPFORMAT wirecf;
    CLIPFORMAT cf = RegisterClipboardFormatA("MyFormat");
    CLIPFORMAT cf2;

    size = CLIPFORMAT_UserSize(&flags, 0, &cf);
    ok(size == sizeof(*wirecf) + sizeof(cf_marshaled), "Size should be %d, instead of %ld\n", sizeof(*wirecf) + sizeof(cf_marshaled), size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    CLIPFORMAT_UserMarshal(&flags, buffer, &cf);
    wirecf = (wireCLIPFORMAT)buffer;
    ok(wirecf->fContext == WDT_REMOTE_CALL, "Context should be WDT_REMOTE_CALL instead of 0x%08lx\n", wirecf->fContext);
    ok(wirecf->u.dwValue == cf, "Marshaled value should be 0x%04x instead of 0x%04lx\n", cf, wirecf->u.dwValue);
    ok(!memcmp(wirecf+1, cf_marshaled, sizeof(cf_marshaled)), "Marshaled data differs\n");

    CLIPFORMAT_UserUnmarshal(&flags, buffer, &cf2);
    ok(cf == cf2, "Didn't unmarshal properly\n");
    HeapFree(GetProcessHeap(), 0, buffer);

    CLIPFORMAT_UserFree(&flags, &cf2);
}

static void test_marshal_HWND(void)
{
    unsigned char *buffer;
    unsigned long size;
    unsigned long flags = MAKELONG(MSHCTX_LOCAL, NDR_LOCAL_DATA_REPRESENTATION);
    HWND hwnd = GetDesktopWindow();
    HWND hwnd2;
    wireHWND wirehwnd;

    size = HWND_UserSize(&flags, 0, &hwnd);
    ok(size == sizeof(*wirehwnd), "Size should be %d, instead of %ld\n", sizeof(*wirehwnd), size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    HWND_UserMarshal(&flags, buffer, &hwnd);
    wirehwnd = (wireHWND)buffer;
    ok(wirehwnd->fContext == WDT_INPROC_CALL, "Context should be WDT_INPROC_CALL instead of 0x%08lx\n", wirehwnd->fContext);
    ok(wirehwnd->u.hInproc == (LONG_PTR)hwnd, "Marshaled value should be %p instead of %p\n", hwnd, (HANDLE)wirehwnd->u.hRemote);

    HWND_UserUnmarshal(&flags, buffer, &hwnd2);
    ok(hwnd == hwnd2, "Didn't unmarshal properly\n");
    HeapFree(GetProcessHeap(), 0, buffer);

    HWND_UserFree(&flags, &hwnd2);
}

static void test_marshal_HGLOBAL(void)
{
    unsigned char *buffer;
    unsigned long size;
    unsigned long flags = MAKELONG(MSHCTX_LOCAL, NDR_LOCAL_DATA_REPRESENTATION);
    HGLOBAL hglobal;
    HGLOBAL hglobal2;
    unsigned char *wirehglobal;
    int i;

    hglobal = NULL;
    flags = MAKELONG(MSHCTX_LOCAL, NDR_LOCAL_DATA_REPRESENTATION);
    size = HGLOBAL_UserSize(&flags, 0, &hglobal);
    /* native is poorly programmed and allocates 4 bytes more than it needs to
     * here - Wine doesn't have to emulate that */
    ok((size == 8) || (size == 12), "Size should be 12, instead of %ld\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    HGLOBAL_UserMarshal(&flags, buffer, &hglobal);
    wirehglobal = buffer;
    ok(*(ULONG *)wirehglobal == WDT_REMOTE_CALL, "Context should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(ULONG *)wirehglobal);
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == (ULONG)hglobal, "buffer+4 should be HGLOBAL\n");
    HGLOBAL_UserUnmarshal(&flags, buffer, &hglobal2);
    ok(hglobal2 == hglobal, "Didn't unmarshal properly\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    HGLOBAL_UserFree(&flags, &hglobal2);

    hglobal = GlobalAlloc(0, 4);
    buffer = GlobalLock(hglobal);
    for (i = 0; i < 4; i++)
        buffer[i] = i;
    GlobalUnlock(hglobal);
    flags = MAKELONG(MSHCTX_LOCAL, NDR_LOCAL_DATA_REPRESENTATION);
    size = HGLOBAL_UserSize(&flags, 0, &hglobal);
    /* native is poorly programmed and allocates 4 bytes more than it needs to
     * here - Wine doesn't have to emulate that */
    ok((size == 24) || (size == 28), "Size should be 24 or 28, instead of %ld\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    HGLOBAL_UserMarshal(&flags, buffer, &hglobal);
    wirehglobal = buffer;
    ok(*(ULONG *)wirehglobal == WDT_REMOTE_CALL, "Context should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(ULONG *)wirehglobal);
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == (ULONG)hglobal, "buffer+0x4 should be HGLOBAL\n");
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == 4, "buffer+0x8 should be size of HGLOBAL\n");
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == (ULONG)hglobal, "buffer+0xc should be HGLOBAL\n");
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == 4, "buffer+0x10 should be size of HGLOBAL\n");
    wirehglobal += sizeof(ULONG);
    for (i = 0; i < 4; i++)
        ok(wirehglobal[i] == i, "buffer+0x%x should be %d\n", 0x10 + i, i);
    HGLOBAL_UserUnmarshal(&flags, buffer, &hglobal2);
    ok(hglobal2 != NULL, "Didn't unmarshal properly\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    HGLOBAL_UserFree(&flags, &hglobal2);
    GlobalFree(hglobal);
}

static HENHMETAFILE create_emf(void)
{
    RECT rect = {0, 0, 100, 100};
    HDC hdc = CreateEnhMetaFile(NULL, NULL, &rect, "HENHMETAFILE Marshaling Test\0Test\0\0");
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, NULL, "Test String", strlen("Test String"), NULL);
    return CloseEnhMetaFile(hdc);
}

static void test_marshal_HENHMETAFILE(void)
{
    unsigned char *buffer;
    unsigned long size;
    unsigned long flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
    HENHMETAFILE hemf;
    HENHMETAFILE hemf2 = NULL;
    unsigned char *wirehemf;

    hemf = create_emf();

    size = HENHMETAFILE_UserSize(&flags, 0, &hemf);
    ok(size > 20, "size should be at least 20 bytes, not %ld\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    HENHMETAFILE_UserMarshal(&flags, buffer, &hemf);
    wirehemf = buffer;
    ok(*(DWORD *)wirehemf == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (DWORD)(DWORD_PTR)hemf, "wirestgm + 0x4 should be hemf instead of 0x%08lx\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (size - 0x10), "wirestgm + 0x8 should be size - 0x10 instead of 0x%08lx\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (size - 0x10), "wirestgm + 0xc should be size - 0x10 instead of 0x%08lx\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == EMR_HEADER, "wirestgm + 0x10 should be EMR_HEADER instead of %ld\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    /* ... rest of data not tested - refer to tests for GetEnhMetaFileBits
     * at this point */

    HENHMETAFILE_UserUnmarshal(&flags, buffer, &hemf2);
    ok(hemf2 != NULL, "HENHMETAFILE didn't unmarshal\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    HENHMETAFILE_UserFree(&flags, &hemf2);
    DeleteEnhMetaFile(hemf);

    /* test NULL emf */
    hemf = NULL;

    size = HENHMETAFILE_UserSize(&flags, 0, &hemf);
    ok(size == 8, "size should be 8 bytes, not %ld\n", size);
    buffer = (unsigned char *)HeapAlloc(GetProcessHeap(), 0, size);
    HENHMETAFILE_UserMarshal(&flags, buffer, &hemf);
    wirehemf = buffer;
    ok(*(DWORD *)wirehemf == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (DWORD)(DWORD_PTR)hemf, "wirestgm + 0x4 should be hemf instead of 0x%08lx\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);

    HENHMETAFILE_UserUnmarshal(&flags, buffer, &hemf2);
    ok(hemf2 == NULL, "NULL HENHMETAFILE didn't unmarshal\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    HENHMETAFILE_UserFree(&flags, &hemf2);
}

START_TEST(marshal)
{
    WNDCLASS wndclass;
    HMODULE hOle32 = GetModuleHandle("ole32");
    if (!(pCoInitializeEx = (void*)GetProcAddress(hOle32, "CoInitializeEx"))) goto no_test;

    /* register a window class used in several tests */
    memset(&wndclass, 0, sizeof(wndclass));
    wndclass.lpfnWndProc = window_proc;
    wndclass.lpszClassName = "WineCOMTest";
    RegisterClass(&wndclass);

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    /* FIXME: test CoCreateInstanceEx */

    /* helper function tests */
    test_CoGetPSClsid();

    /* lifecycle management and marshaling tests */
    test_no_marshaler();
    test_normal_marshal_and_release();
    test_normal_marshal_and_unmarshal();
    test_marshal_and_unmarshal_invalid();
    test_interthread_marshal_and_unmarshal();
    test_proxy_marshal_and_unmarshal();
    test_proxy_marshal_and_unmarshal2();
    test_marshal_stub_apartment_shutdown();
    test_marshal_proxy_apartment_shutdown();
    test_marshal_proxy_mta_apartment_shutdown();
    test_no_couninitialize_server();
    test_no_couninitialize_client();
    test_tableweak_marshal_and_unmarshal_twice();
    test_tableweak_marshal_releasedata1();
    test_tableweak_marshal_releasedata2();
    test_tablestrong_marshal_and_unmarshal_twice();
    test_lock_object_external();
    test_disconnect_stub();
    test_normal_marshal_and_unmarshal_twice();
    test_hresult_marshaling();
    test_proxy_used_in_wrong_thread();
    test_message_filter();
    test_bad_marshal_stream();
    test_proxy_interfaces();
    test_stubbuffer(&IID_IClassFactory);
    test_proxybuffer(&IID_IClassFactory);
    test_message_reentrancy();
    test_WM_QUIT_handling();

/*    test_out_of_process_com(); */

    test_ROT();
    /* FIXME: test GIT */

    test_marshal_CLIPFORMAT();
    test_marshal_HWND();
    test_marshal_HGLOBAL();
    test_marshal_HENHMETAFILE();

    CoUninitialize();
    return;

no_test:
    trace("You need DCOM95 installed to run this test\n");
    return;
}
