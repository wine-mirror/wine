/*
 * Tests for marshaling IDispatchEx
 *
 * Copyright 2005-2006 Robert Shearman
 * Copyright 2010 Huw Davies
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
 *
 */

#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>

#include "initguid.h"
#include "objidl.h"
#include "dispex.h"

#include "wine/test.h"

#define RELEASEMARSHALDATA WM_USER

struct host_object_data
{
    IStream *stream;
    IID iid;
    IUnknown *object;
    MSHLFLAGS marshal_flags;
    HANDLE marshal_event;
    HANDLE error_event;
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
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }

    hr = CoMarshalInterface(data->stream, &data->iid, data->object, MSHCTX_INPROC, NULL, data->marshal_flags);

    /* force the message queue to be created before signaling parent thread */
    PeekMessageA(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    if(hr == S_OK)
        SetEvent(data->marshal_event);
    else
    {
        win_skip("IDispatchEx marshaller not available.\n");
        SetEvent(data->error_event);
        return hr;
    }

    while (GetMessageA(&msg, NULL, 0, 0))
    {
        if (msg.hwnd == NULL && msg.message == RELEASEMARSHALDATA)
        {
            trace("releasing marshal data\n");
            CoReleaseMarshalData(data->stream);
            SetEvent((HANDLE)msg.lParam);
        }
        else
            DispatchMessageA(&msg);
    }

    free(data);

    CoUninitialize();

    return hr;
}

static DWORD start_host_object2(IStream *stream, REFIID riid, IUnknown *object, MSHLFLAGS marshal_flags, IMessageFilter *filter, HANDLE *thread)
{
    DWORD tid = 0, ret;
    HANDLE events[2];
    struct host_object_data *data = malloc(sizeof(*data));

    data->stream = stream;
    data->iid = *riid;
    data->object = object;
    data->marshal_flags = marshal_flags;
    data->marshal_event = events[0] = CreateEventW(NULL, FALSE, FALSE, NULL);
    data->error_event   = events[1] = CreateEventW(NULL, FALSE, FALSE, NULL);
    data->filter = filter;

    *thread = CreateThread(NULL, 0, host_object_proc, data, 0, &tid);

    /* wait for marshaling to complete before returning */
    ret = WaitForMultipleObjects(2, events, FALSE, INFINITE);
    CloseHandle(events[0]);
    CloseHandle(events[1]);

    if(ret == WAIT_OBJECT_0) return tid;

    WaitForSingleObject(*thread, INFINITE);
    CloseHandle(*thread);
    *thread = INVALID_HANDLE_VALUE;
    return 0;
}

static DWORD start_host_object(IStream *stream, REFIID riid, IUnknown *object, MSHLFLAGS marshal_flags, HANDLE *thread)
{
    return start_host_object2(stream, riid, object, marshal_flags, NULL, thread);
}

static void end_host_object(DWORD tid, HANDLE thread)
{
    BOOL ret = PostThreadMessageA(tid, WM_QUIT, 0, 0);
    ok(ret, "PostThreadMessage failed with error %ld\n", GetLastError());
    /* be careful of races - don't return until hosting thread has terminated */
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

typedef struct
{
    IDispatchEx IDispatchEx_iface;
    LONG refs;
} dispex;

static inline dispex *impl_from_IDispatchEx(IDispatchEx *iface)
{
    return CONTAINING_RECORD(iface, dispex, IDispatchEx_iface);
}

static HRESULT WINAPI dispex_QueryInterface(IDispatchEx* iface,
                                            REFIID iid,  void **obj)
{
    trace("QI %s\n", debugstr_guid(iid));
    if(IsEqualIID(iid, &IID_IUnknown) ||
       IsEqualIID(iid, &IID_IDispatchEx))
    {
        IDispatchEx_AddRef(iface);
        *obj = iface;
        return S_OK;
    }
    else
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI dispex_AddRef(IDispatchEx* iface)
{
    dispex *This = impl_from_IDispatchEx(iface);
    trace("AddRef\n");

    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI dispex_Release(IDispatchEx* iface)
{
    dispex *This = impl_from_IDispatchEx(iface);
    ULONG refs = InterlockedDecrement(&This->refs);
    trace("Release\n");
    if(!refs)
        free(This);

    return refs;
}

 static HRESULT WINAPI dispex_GetTypeInfoCount(IDispatchEx* iface,
                                               UINT *pctinfo)
{
    trace("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetTypeInfo(IDispatchEx* iface,
                                         UINT iTInfo,
                                         LCID lcid,
                                         ITypeInfo **ppTInfo)
{
    trace("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetIDsOfNames(IDispatchEx* iface,
                                           REFIID riid,
                                           LPOLESTR *rgszNames,
                                           UINT cNames,
                                           LCID lcid,
                                           DISPID *rgDispId)
{
    trace("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_Invoke(IDispatchEx* iface,
                                    DISPID dispIdMember,
                                    REFIID riid,
                                    LCID lcid,
                                    WORD wFlags,
                                    DISPPARAMS *pDispParams,
                                    VARIANT *pVarResult,
                                    EXCEPINFO *pExcepInfo,
                                    UINT *puArgErr)
{
    trace("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetDispID(IDispatchEx* iface,
                                       BSTR bstrName,
                                       DWORD grfdex,
                                       DISPID *pid)
{
    trace("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI defer_fn(EXCEPINFO *except)
{
    except->scode = E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI dispex_InvokeEx(IDispatchEx* iface,
                                      DISPID id,
                                      LCID lcid,
                                      WORD wFlags,
                                      DISPPARAMS *pdp,
                                      VARIANT *pvarRes,
                                      EXCEPINFO *pei,
                                      IServiceProvider *pspCaller)
{
    if(id == 1)
    {
        ok(pdp->cArgs == 0, "got %d\n", pdp->cArgs);
        ok(pei == NULL, "got non-NULL excepinfo\n");
        ok(pvarRes == NULL, "got non-NULL result\n");
    }
    else if(id == 2)
    {
        ok(pdp->cArgs == 2, "got %d\n", pdp->cArgs);
        ok(V_VT(&pdp->rgvarg[0]) == VT_INT, "got %04x\n", V_VT(&pdp->rgvarg[0]));
        ok(V_VT(&pdp->rgvarg[1]) == (VT_INT | VT_BYREF), "got %04x\n", V_VT(&pdp->rgvarg[1]));
        ok(*V_INTREF(&pdp->rgvarg[1]) == 0xbeef, "got %08x\n", *V_INTREF(&pdp->rgvarg[1]));
        *V_INTREF(&pdp->rgvarg[1]) = 0xdead;
    }
    else if(id == 3)
    {
        ok(pdp->cArgs == 2, "got %d\n", pdp->cArgs);
        ok(V_VT(&pdp->rgvarg[0]) == VT_INT, "got %04x\n", V_VT(&pdp->rgvarg[0]));
        ok(V_VT(&pdp->rgvarg[1]) == (VT_INT | VT_BYREF), "got %04x\n", V_VT(&pdp->rgvarg[1]));
        V_VT(&pdp->rgvarg[0]) = VT_I4;
    }
    else if(id == 4)
    {
        ok(wFlags == 0xf, "got %04x\n", wFlags);
    }
    else if(id == 5)
    {
        if(pei) pei->pfnDeferredFillIn = defer_fn;
        return DISP_E_EXCEPTION;
    }
    return S_OK;
}

static HRESULT WINAPI dispex_DeleteMemberByName(IDispatchEx* iface,
                                                BSTR bstrName,
                                                DWORD grfdex)
{
    trace("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_DeleteMemberByDispID(IDispatchEx* iface, DISPID id)
{
    trace("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetMemberProperties(IDispatchEx* iface, DISPID id,
                                                 DWORD grfdexFetch, DWORD *pgrfdex)
{
    trace("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetMemberName(IDispatchEx* iface,
                                           DISPID id, BSTR *pbstrName)
{
    trace("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetNextDispID(IDispatchEx* iface,
                                           DWORD grfdex,
                                           DISPID id,
                                           DISPID *pid)
{
    trace("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI dispex_GetNameSpaceParent(IDispatchEx* iface,
                                                IUnknown **ppunk)
{
    trace("\n");
    return E_NOTIMPL;
}

static const IDispatchExVtbl dispex_vtable =
{
    dispex_QueryInterface,
    dispex_AddRef,
    dispex_Release,
    dispex_GetTypeInfoCount,
    dispex_GetTypeInfo,
    dispex_GetIDsOfNames,
    dispex_Invoke,
    dispex_GetDispID,
    dispex_InvokeEx,
    dispex_DeleteMemberByName,
    dispex_DeleteMemberByDispID,
    dispex_GetMemberProperties,
    dispex_GetMemberName,
    dispex_GetNextDispID,
    dispex_GetNameSpaceParent
};

static IDispatchEx *dispex_create(void)
{
    dispex *This;

    This = malloc(sizeof(*This));
    if (!This) return NULL;
    This->IDispatchEx_iface.lpVtbl = &dispex_vtable;
    This->refs = 1;
    return &This->IDispatchEx_iface;
}

static void test_dispex(void)
{
    HRESULT hr;
    IStream *stream;
    DWORD tid;
    HANDLE thread;
    static const LARGE_INTEGER zero;
    IDispatchEx *dispex = dispex_create();
    DISPPARAMS params;
    VARIANTARG args[10];
    INT i;
    EXCEPINFO excepinfo;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    tid = start_host_object(stream, &IID_IDispatchEx, (IUnknown *)dispex, MSHLFLAGS_NORMAL, &thread);
    IDispatchEx_Release(dispex);
    if(tid == 0)
    {
        IStream_Release(stream);
        return;
    }

    IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(stream, &IID_IDispatchEx, (void **)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IStream_Release(stream);

    params.rgvarg = NULL;
    params.rgdispidNamedArgs = NULL;
    params.cArgs = 0;
    params.cNamedArgs = 0;
    hr = IDispatchEx_InvokeEx(dispex, 1, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, NULL, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    params.rgvarg = args;
    params.rgdispidNamedArgs = NULL;
    params.cArgs = 2;
    params.cNamedArgs = 0;
    V_VT(&args[0]) = VT_INT;
    V_INT(&args[0]) = 0xcafe;
    V_VT(&args[1]) = VT_INT | VT_BYREF;
    V_INTREF(&args[1]) = &i;
    i = 0xbeef;
    hr = IDispatchEx_InvokeEx(dispex, 2, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, NULL, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(i == 0xdead, "got %08x\n", i);

    /* change one of the argument vts */
    i = 0xbeef;
    hr = IDispatchEx_InvokeEx(dispex, 3, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, NULL, NULL, NULL);
    ok(hr == DISP_E_BADCALLEE, "Unexpected hr %#lx.\n", hr);

    hr = IDispatchEx_InvokeEx(dispex, 4, LOCALE_SYSTEM_DEFAULT, 0xffff, &params, NULL, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    params.cArgs = 0;
    hr = IDispatchEx_InvokeEx(dispex, 5, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, NULL, NULL, NULL);
    ok(hr == DISP_E_EXCEPTION, "Unexpected hr %#lx.\n", hr);
    hr = IDispatchEx_InvokeEx(dispex, 5, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, NULL, &excepinfo, NULL);
    ok(hr == DISP_E_EXCEPTION, "Unexpected hr %#lx.\n", hr);
    ok(excepinfo.scode == E_OUTOFMEMORY, "got scode %#lx.\n", excepinfo.scode);
    ok(excepinfo.pfnDeferredFillIn == NULL, "got non-NULL pfnDeferredFillIn\n");

    IDispatchEx_Release(dispex);
    end_host_object(tid, thread);
}

START_TEST(marshal)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_dispex();

    CoUninitialize();
}
