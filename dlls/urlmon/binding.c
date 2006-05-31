/*
 * Copyright 2005 Jacek Caban
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "urlmon.h"
#include "urlmon_main.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct Binding Binding;

enum task_enum {
    TASK_ON_PROGRESS,
    TASK_SWITCH
};

typedef struct {
    Binding *binding;
    ULONG progress;
    ULONG progress_max;
    ULONG status_code;
    LPWSTR status_text;
} on_progress_data;

typedef struct _task_t {
    enum task_enum task;
    Binding *binding;
    struct _task_t *next;
    union {
        on_progress_data on_progress;
        PROTOCOLDATA *protocol_data;
    } data;
} task_t;

typedef struct {
    const IStreamVtbl *lpStreamVtbl;

    LONG ref;

    IInternetProtocol *protocol;

    BYTE buf[1024*8];
    DWORD buf_size;
    BOOL init_buf;
} ProtocolStream;

struct Binding {
    const IBindingVtbl               *lpBindingVtbl;
    const IInternetProtocolSinkVtbl  *lpInternetProtocolSinkVtbl;
    const IInternetBindInfoVtbl      *lpInternetBindInfoVtbl;
    const IServiceProviderVtbl       *lpServiceProviderVtbl;

    LONG ref;

    IBindStatusCallback *callback;
    IInternetProtocol *protocol;
    IServiceProvider *service_provider;
    ProtocolStream *stream;

    BINDINFO bindinfo;
    DWORD bindf;
    LPWSTR mime;
    LPWSTR url;
    BOOL verified_mime;
    DWORD continue_call;

    DWORD apartment_thread;
    HWND notif_hwnd;

    STGMEDIUM stgmed;

    task_t *task_queue_head, *task_queue_tail;
    CRITICAL_SECTION section;
};

#define BINDING(x)   ((IBinding*)               &(x)->lpBindingVtbl)
#define PROTSINK(x)  ((IInternetProtocolSink*)  &(x)->lpInternetProtocolSinkVtbl)
#define BINDINF(x)   ((IInternetBindInfo*)      &(x)->lpInternetBindInfoVtbl)
#define SERVPROV(x)  ((IServiceProvider*)       &(x)->lpServiceProviderVtbl)

#define STREAM(x) ((IStream*) &(x)->lpStreamVtbl)

#define WM_MK_CONTINUE   (WM_USER+101)

static void push_task(task_t *task)
{
    task->next = NULL;

     EnterCriticalSection(&task->binding->section);

    if(task->binding->task_queue_tail)
        task->binding->task_queue_tail->next = task;
    else
        task->binding->task_queue_tail = task->binding->task_queue_head = task;

    LeaveCriticalSection(&task->binding->section);
}

static task_t *pop_task(Binding *binding)
{
    task_t *ret;

     EnterCriticalSection(&binding->section);

     ret = binding->task_queue_head;
     if(ret) {
         binding->task_queue_head = ret->next;
         if(!binding->task_queue_head)
             binding->task_queue_tail = NULL;
     }

     LeaveCriticalSection(&binding->section);

     return ret;
}

static void do_task(task_t *task)
{
    switch(task->task) {
    case TASK_ON_PROGRESS: {
        on_progress_data *data = &task->data.on_progress;

        IBindStatusCallback_OnProgress(task->binding->callback, data->progress,
                data->progress_max, data->status_code, data->status_text);

        HeapFree(GetProcessHeap(), 0, data->status_text);
        HeapFree(GetProcessHeap(), 0, data);

        break;
    }
    case TASK_SWITCH:
        task->binding->continue_call++;
        IInternetProtocol_Continue(task->binding->protocol, task->data.protocol_data);
        task->binding->continue_call--;
    }
}

static void do_tasks(Binding *This)
{
    task_t *task;

    while((task = pop_task(This))) {
        do_task(task);
        IBinding_Release(BINDING(task->binding));
    }
}

static LRESULT WINAPI notif_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_MK_CONTINUE) {
        Binding *binding = (Binding*)lParam;

        do_tasks(binding);

        IBinding_Release(BINDING(binding));
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static HWND get_notif_hwnd(void)
{
    static ATOM wnd_class = 0;
    HWND hwnd;

    static const WCHAR wszURLMonikerNotificationWindow[] =
        {'U','R','L',' ','M','o','n','i','k','e','r',' ',
         'N','o','t','i','f','i','c','a','t','i','o','n',' ','W','i','n','d','o','w',0};

    if(!wnd_class) {
        static WNDCLASSEXW wndclass = {
            sizeof(wndclass), 0,
            notif_wnd_proc, 0, 0,
            NULL, NULL, NULL, NULL, NULL,
            wszURLMonikerNotificationWindow,
            NULL        
        };

        wndclass.hInstance = URLMON_hInstance;

        wnd_class = RegisterClassExW(&wndclass);
    }

    if(!urlmon_tls)
        urlmon_tls = TlsAlloc();

    hwnd = TlsGetValue(urlmon_tls);
    if(hwnd)
        return hwnd;

    hwnd = CreateWindowExW(0, MAKEINTATOMW(wnd_class),
                           wszURLMonikerNotificationWindow, 0, 0, 0, 0, 0, HWND_MESSAGE,
                           NULL, URLMON_hInstance, NULL);
    TlsSetValue(urlmon_tls, hwnd);

    TRACE("hwnd = %p\n", hwnd);

    return hwnd;
}

static void on_progress(Binding *This, ULONG progress, ULONG progress_max,
                        ULONG status_code, LPCWSTR status_text)
{
    task_t *task;

    if(GetCurrentThreadId() == This->apartment_thread && !This->continue_call) {
        IBindStatusCallback_OnProgress(This->callback, progress, progress_max,
                                       status_code, status_text);
        return;
    }

    task = HeapAlloc(GetProcessHeap(), 0, sizeof(task_t));

    task->task = TASK_ON_PROGRESS;

    IBinding_AddRef(BINDING(This));
    task->binding = This;

    task->data.on_progress.progress = progress;
    task->data.on_progress.progress_max = progress_max;
    task->data.on_progress.status_code = status_code;

    if(status_text) {
        DWORD size = (strlenW(status_text)+1)*sizeof(WCHAR);

        task->data.on_progress.status_text = HeapAlloc(GetProcessHeap(), 0, size);
        memcpy(task->data.on_progress.status_text, status_text, size);
    }else {
        task->data.on_progress.status_text = NULL;
    }

    push_task(task);

    if(GetCurrentThreadId() != This->apartment_thread) {
        IBinding_AddRef(BINDING(This));
        PostMessageW(This->notif_hwnd, WM_MK_CONTINUE, 0, (LPARAM)This);
    }
}

static void dump_BINDINFO(BINDINFO *bi)
{
    static const char *BINDINFOF_str[] = {
        "#0",
        "BINDINFOF_URLENCODESTGMEDDATA",
        "BINDINFOF_URLENCODEDEXTRAINFO"
    };

    static const char *BINDVERB_str[] = {
        "BINDVERB_GET",
        "BINDVERB_POST",
        "BINDVERB_PUT",
        "BINDVERB_CUSTOM"
    };

    TRACE("\n"
            "BINDINFO = {\n"
            "    %ld, %s,\n"
            "    {%ld, %p, %p},\n"
            "    %s,\n"
            "    %s,\n"
            "    %s,\n"
            "    %ld, %08lx, %ld, %ld\n"
            "    {%ld %p %x},\n"
            "    %s\n"
            "    %p, %ld\n"
            "}\n",

            bi->cbSize, debugstr_w(bi->szExtraInfo),
            bi->stgmedData.tymed, bi->stgmedData.u.hGlobal, bi->stgmedData.pUnkForRelease,
            bi->grfBindInfoF > BINDINFOF_URLENCODEDEXTRAINFO
                ? "unknown" : BINDINFOF_str[bi->grfBindInfoF],
            bi->dwBindVerb > BINDVERB_CUSTOM
                ? "unknown" : BINDVERB_str[bi->dwBindVerb],
            debugstr_w(bi->szCustomVerb),
            bi->cbStgmedData, bi->dwOptions, bi->dwOptionsFlags, bi->dwCodePage,
            bi->securityAttributes.nLength,
            bi->securityAttributes.lpSecurityDescriptor,
            bi->securityAttributes.bInheritHandle,
            debugstr_guid(&bi->iid),
            bi->pUnk, bi->dwReserved
            );
}

static HRESULT WINAPI HttpNegotiate_QueryInterface(IHttpNegotiate2 *iface,
                                                   REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(IID_IUnknown %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IHttpNegotiate, riid)) {
        TRACE("(IID_IHttpNegotiate %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IHttpNegotiate2, riid)) {
        TRACE("(IID_IHttpNegotiate2 %p)\n", ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IHttpNegotiate2_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI HttpNegotiate_AddRef(IHttpNegotiate2 *iface)
{
    URLMON_LockModule();
    return 2;
}

static ULONG WINAPI HttpNegotiate_Release(IHttpNegotiate2 *iface)
{
    URLMON_UnlockModule();
    return 1;
}

static HRESULT WINAPI HttpNegotiate_BeginningTransaction(IHttpNegotiate2 *iface,
        LPCWSTR szURL, LPCWSTR szHeaders, DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    TRACE("(%s %s %ld %p)\n", debugstr_w(szURL), debugstr_w(szHeaders), dwReserved,
          pszAdditionalHeaders);

    *pszAdditionalHeaders = NULL;
    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate2 *iface, DWORD dwResponseCode,
        LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders,
        LPWSTR *pszAdditionalRequestHeaders)
{
    FIXME("(%ld %s %s %p)\n", dwResponseCode, debugstr_w(szResponseHeaders),
          debugstr_w(szRequestHeaders), pszAdditionalRequestHeaders);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpNegotiate_GetRootSecurityId(IHttpNegotiate2 *iface,
        BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    TRACE("(%p %p %ld)\n", pbSecurityId, pcbSecurityId, dwReserved);

    /* That's all we have to do here */
    return E_FAIL;
}

static const IHttpNegotiate2Vtbl HttpNegotiate2Vtbl = {
    HttpNegotiate_QueryInterface,
    HttpNegotiate_AddRef,
    HttpNegotiate_Release,
    HttpNegotiate_BeginningTransaction,
    HttpNegotiate_OnResponse,
    HttpNegotiate_GetRootSecurityId
};

static IHttpNegotiate2 HttpNegotiate = { &HttpNegotiate2Vtbl };

#define STREAM_THIS(iface) DEFINE_THIS(ProtocolStream, Stream, iface)

static HRESULT WINAPI ProtocolStream_QueryInterface(IStream *iface,
                                                          REFIID riid, void **ppv)
{
    ProtocolStream *This = STREAM_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = STREAM(This);
    }else if(IsEqualGUID(&IID_ISequentialStream, riid)) {
        TRACE("(%p)->(IID_ISequentialStream %p)\n", This, ppv);
        *ppv = STREAM(This);
    }else if(IsEqualGUID(&IID_IStream, riid)) {
        TRACE("(%p)->(IID_IStream %p)\n", This, ppv);
        *ppv = STREAM(This);
    }

    if(*ppv) {
        IStream_AddRef(STREAM(This));
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ProtocolStream_AddRef(IStream *iface)
{
    ProtocolStream *This = STREAM_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI ProtocolStream_Release(IStream *iface)
{
    ProtocolStream *This = STREAM_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        IInternetProtocol_Release(This->protocol);
        HeapFree(GetProcessHeap(), 0, This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI ProtocolStream_Read(IStream *iface, void *pv,
                                         ULONG cb, ULONG *pcbRead)
{
    ProtocolStream *This = STREAM_THIS(iface);
    DWORD read = 0, pread = 0;
    HRESULT hres;

    TRACE("(%p)->(%p %ld %p)\n", This, pv, cb, pcbRead);

    if(This->buf_size) {
        read = cb;

        if(read > This->buf_size)
            read = This->buf_size;

        memcpy(pv, This->buf, read);

        if(read < This->buf_size)
            memmove(This->buf, This->buf+read, This->buf_size-read);
        This->buf_size -= read;
    }

    if(read == cb) {
        *pcbRead = read;
        return S_OK;
    }

    hres = IInternetProtocol_Read(This->protocol, (PBYTE)pv+read, cb-read, &pread);
    *pcbRead = read + pread;

    if(hres == E_PENDING)
        return E_PENDING;
    else if(FAILED(hres))
        FIXME("Read failed: %08lx\n", hres);

    return read || pread ? S_OK : S_FALSE;
}

static HRESULT WINAPI ProtocolStream_Write(IStream *iface, const void *pv,
                                          ULONG cb, ULONG *pcbWritten)
{
    ProtocolStream *This = STREAM_THIS(iface);

    TRACE("(%p)->(%p %ld %p)\n", This, pv, cb, pcbWritten);

    return STG_E_ACCESSDENIED;
}

static HRESULT WINAPI ProtocolStream_Seek(IStream *iface, LARGE_INTEGER dlibMove,
                                         DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%ld %08lx %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_SetSize(IStream *iface, ULARGE_INTEGER libNewSize)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, libNewSize.u.LowPart);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_CopyTo(IStream *iface, IStream *pstm,
        ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%p %ld %p %p)\n", This, pstm, cb.u.LowPart, pcbRead, pcbWritten);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_Commit(IStream *iface, DWORD grfCommitFlags)
{
    ProtocolStream *This = STREAM_THIS(iface);

    TRACE("(%p)->(%08lx)\n", This, grfCommitFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_Revert(IStream *iface)
{
    ProtocolStream *This = STREAM_THIS(iface);

    TRACE("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_LockRegion(IStream *iface, ULARGE_INTEGER libOffset,
                                               ULARGE_INTEGER cb, DWORD dwLockType)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%ld %ld %ld)\n", This, libOffset.u.LowPart, cb.u.LowPart, dwLockType);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_UnlockRegion(IStream *iface,
        ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%ld %ld %ld)\n", This, libOffset.u.LowPart, cb.u.LowPart, dwLockType);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_Stat(IStream *iface, STATSTG *pstatstg,
                                         DWORD dwStatFlag)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%p %08lx)\n", This, pstatstg, dwStatFlag);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_Clone(IStream *iface, IStream **ppstm)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppstm);
    return E_NOTIMPL;
}

#undef STREAM_THIS

static const IStreamVtbl ProtocolStreamVtbl = {
    ProtocolStream_QueryInterface,
    ProtocolStream_AddRef,
    ProtocolStream_Release,
    ProtocolStream_Read,
    ProtocolStream_Write,
    ProtocolStream_Seek,
    ProtocolStream_SetSize,
    ProtocolStream_CopyTo,
    ProtocolStream_Commit,
    ProtocolStream_Revert,
    ProtocolStream_LockRegion,
    ProtocolStream_UnlockRegion,
    ProtocolStream_Stat,
    ProtocolStream_Clone
};

#define BINDING_THIS(iface) DEFINE_THIS(Binding, Binding, iface)

static ProtocolStream *create_stream(IInternetProtocol *protocol)
{
    ProtocolStream *ret = HeapAlloc(GetProcessHeap(), 0, sizeof(ProtocolStream));

    ret->lpStreamVtbl = &ProtocolStreamVtbl;
    ret->ref = 1;
    ret->buf_size = 0;
    ret->init_buf = FALSE;

    IInternetProtocol_AddRef(protocol);
    ret->protocol = protocol;

    URLMON_LockModule();

    return ret;
}

static void fill_stream_buffer(ProtocolStream *This)
{
    DWORD read = 0;
    HRESULT hres;

    hres = IInternetProtocol_Read(This->protocol, This->buf+This->buf_size,
                                  sizeof(This->buf)-This->buf_size, &read);
    if(SUCCEEDED(hres)) {
        This->buf_size += read;
        This->init_buf = TRUE;
    }
}

static HRESULT WINAPI Binding_QueryInterface(IBinding *iface, REFIID riid, void **ppv)
{
    Binding *This = BINDING_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = BINDING(This);
    }else if(IsEqualGUID(&IID_IBinding, riid)) {
        TRACE("(%p)->(IID_IBinding %p)\n", This, ppv);
        *ppv = BINDING(This);
    }else if(IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolSink %p)\n", This, ppv);
        *ppv = PROTSINK(This);
    }else if(IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        TRACE("(%p)->(IID_IInternetBindInfo %p)\n", This, ppv);
        *ppv = BINDINF(This);
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = SERVPROV(This);
    }

    if(*ppv) {
        IBinding_AddRef(BINDING(This));
        return S_OK;
    }

    WARN("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Binding_AddRef(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI Binding_Release(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->callback)
            IBindStatusCallback_Release(This->callback);
        if(This->protocol)
            IInternetProtocol_Release(This->protocol);
        if(This->service_provider)
            IServiceProvider_Release(This->service_provider);
        if(This->stream)
            IStream_Release(STREAM(This->stream));

        ReleaseBindInfo(&This->bindinfo);
        DeleteCriticalSection(&This->section);
        HeapFree(GetProcessHeap(), 0, This->mime);
        HeapFree(GetProcessHeap(), 0, This->url);

        HeapFree(GetProcessHeap(), 0, This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI Binding_Abort(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_Suspend(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_Resume(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_SetPriority(IBinding *iface, LONG nPriority)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, nPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_GetPriority(IBinding *iface, LONG *pnPriority)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_GetBindResult(IBinding *iface, CLSID *pclsidProtocol,
        DWORD *pdwResult, LPOLESTR *pszResult, DWORD *pdwReserved)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)->(%p %p %p %p)\n", This, pclsidProtocol, pdwResult, pszResult, pdwReserved);
    return E_NOTIMPL;
}

#undef BINDING_THIS

static const IBindingVtbl BindingVtbl = {
    Binding_QueryInterface,
    Binding_AddRef,
    Binding_Release,
    Binding_Abort,
    Binding_Suspend,
    Binding_Resume,
    Binding_SetPriority,
    Binding_GetPriority,
    Binding_GetBindResult
};

#define PROTSINK_THIS(iface) DEFINE_THIS(Binding, InternetProtocolSink, iface)

static HRESULT WINAPI InternetProtocolSink_QueryInterface(IInternetProtocolSink *iface,
        REFIID riid, void **ppv)
{
    Binding *This = PROTSINK_THIS(iface);
    return IBinding_QueryInterface(BINDING(This), riid, ppv);
}

static ULONG WINAPI InternetProtocolSink_AddRef(IInternetProtocolSink *iface)
{
    Binding *This = PROTSINK_THIS(iface);
    return IBinding_AddRef(BINDING(This));
}

static ULONG WINAPI InternetProtocolSink_Release(IInternetProtocolSink *iface)
{
    Binding *This = PROTSINK_THIS(iface);
    return IBinding_Release(BINDING(This));
}

static HRESULT WINAPI InternetProtocolSink_Switch(IInternetProtocolSink *iface,
        PROTOCOLDATA *pProtocolData)
{
    Binding *This = PROTSINK_THIS(iface);
    task_t *task;

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    task = HeapAlloc(GetProcessHeap(), 0, sizeof(task_t));
    task->task = TASK_SWITCH;

    IBinding_AddRef(BINDING(This));
    task->binding = This;

    task->data.protocol_data = pProtocolData;

    push_task(task);

    IBinding_AddRef(BINDING(This));
    PostMessageW(This->notif_hwnd, WM_MK_CONTINUE, 0, (LPARAM)This);

    return S_OK;
}

static HRESULT WINAPI InternetProtocolSink_ReportProgress(IInternetProtocolSink *iface,
        ULONG ulStatusCode, LPCWSTR szStatusText)
{
    Binding *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%lu %s)\n", This, ulStatusCode, debugstr_w(szStatusText));

    switch(ulStatusCode) {
    case BINDSTATUS_FINDINGRESOURCE:
        on_progress(This, 0, 0, BINDSTATUS_FINDINGRESOURCE, szStatusText);
        break;
    case BINDSTATUS_CONNECTING:
        on_progress(This, 0, 0, BINDSTATUS_CONNECTING, szStatusText);
        break;
    case BINDSTATUS_MIMETYPEAVAILABLE: {
        int len = strlenW(szStatusText)+1;
        This->mime = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        memcpy(This->mime, szStatusText, len*sizeof(WCHAR));
        break;
    }
    case BINDSTATUS_SENDINGREQUEST:
        on_progress(This, 0, 0, BINDSTATUS_SENDINGREQUEST, szStatusText);
        break;
    case BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE:
        This->verified_mime = TRUE;
        on_progress(This, 0, 0, BINDSTATUS_MIMETYPEAVAILABLE, szStatusText);
        break;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        break;
    default:
        FIXME("Unhandled status code %ld\n", ulStatusCode);
        return E_NOTIMPL;
    };

    return S_OK;
}

static HRESULT WINAPI InternetProtocolSink_ReportData(IInternetProtocolSink *iface,
        DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    Binding *This = PROTSINK_THIS(iface);
    FORMATETC formatetc;

    TRACE("(%p)->(%ld %lu %lu)\n", This, grfBSCF, ulProgress, ulProgressMax);

    fill_stream_buffer(This->stream);

    if(!This->verified_mime) {
        LPWSTR mime;

        This->verified_mime = TRUE;

        /* FIXME: Always call FindMediaFromData (its implementation is not yet ready for this). */
        if(This->mime)
            mime = This->mime;
        else
            FindMimeFromData(NULL, This->url, This->stream->buf,
                              min(This->stream->buf_size, 255), This->mime, 0, &mime, 0);

        on_progress(This, ulProgress, ulProgressMax, BINDSTATUS_MIMETYPEAVAILABLE, mime);
    }

    if(grfBSCF & BSCF_FIRSTDATANOTIFICATION) {
        on_progress(This, ulProgress, ulProgressMax, BINDSTATUS_BEGINDOWNLOADDATA, This->url);
    }

    if(grfBSCF & BSCF_LASTDATANOTIFICATION)
        on_progress(This, ulProgress, ulProgressMax, BINDSTATUS_ENDDOWNLOADDATA, This->url);

    if(grfBSCF & BSCF_FIRSTDATANOTIFICATION)
        IInternetProtocol_LockRequest(This->protocol, 0);

    formatetc.cfFormat = 0; /* FIXME */
    formatetc.ptd = NULL;
    formatetc.dwAspect = 1;
    formatetc.lindex = -1;
    formatetc.tymed = TYMED_ISTREAM;

    IBindStatusCallback_OnDataAvailable(This->callback, grfBSCF, This->stream->buf_size,
            &formatetc, &This->stgmed);

    if(grfBSCF & BSCF_LASTDATANOTIFICATION)
        IBindStatusCallback_OnStopBinding(This->callback, S_OK, NULL);

    return S_OK;
}

static HRESULT WINAPI InternetProtocolSink_ReportResult(IInternetProtocolSink *iface,
        HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    Binding *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%08lx %ld %s)\n", This, hrResult, dwError, debugstr_w(szResult));

    IInternetProtocol_Terminate(This->protocol, 0);
    return S_OK;
}

#undef PROTSINK_THIS

static const IInternetProtocolSinkVtbl InternetProtocolSinkVtbl = {
    InternetProtocolSink_QueryInterface,
    InternetProtocolSink_AddRef,
    InternetProtocolSink_Release,
    InternetProtocolSink_Switch,
    InternetProtocolSink_ReportProgress,
    InternetProtocolSink_ReportData,
    InternetProtocolSink_ReportResult
};

#define BINDINF_THIS(iface) DEFINE_THIS(Binding, InternetBindInfo, iface)

static HRESULT WINAPI InternetBindInfo_QueryInterface(IInternetBindInfo *iface,
        REFIID riid, void **ppv)
{
    Binding *This = BINDINF_THIS(iface);
    return IBinding_QueryInterface(BINDING(This), riid, ppv);
}

static ULONG WINAPI InternetBindInfo_AddRef(IInternetBindInfo *iface)
{
    Binding *This = BINDINF_THIS(iface);
    return IBinding_AddRef(BINDING(This));
}

static ULONG WINAPI InternetBindInfo_Release(IInternetBindInfo *iface)
{
    Binding *This = BINDINF_THIS(iface);
    return IBinding_Release(BINDING(This));
}

static HRESULT WINAPI InternetBindInfo_GetBindInfo(IInternetBindInfo *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    Binding *This = BINDINF_THIS(iface);

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    *grfBINDF = This->bindf;

    memcpy(pbindinfo, &This->bindinfo, sizeof(BINDINFO));

    if(pbindinfo->szExtraInfo || pbindinfo->szCustomVerb)
        FIXME("copy strings\n");

    if(pbindinfo->pUnk)
        IUnknown_AddRef(pbindinfo->pUnk);

    return S_OK;
}

static HRESULT WINAPI InternetBindInfo_GetBindString(IInternetBindInfo *iface,
        ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    Binding *This = BINDINF_THIS(iface);

    TRACE("(%p)->(%ld %p %ld %p)\n", This, ulStringType, ppwzStr, cEl, pcElFetched);

    switch(ulStringType) {
    case BINDSTRING_ACCEPT_MIMES: {
        static const WCHAR wszMimes[] = {'*','/','*',0};

        if(!ppwzStr || !pcElFetched)
            return E_INVALIDARG;

        ppwzStr[0] = CoTaskMemAlloc(sizeof(wszMimes));
        memcpy(ppwzStr[0], wszMimes, sizeof(wszMimes));
        *pcElFetched = 1;
        return S_OK;
    }
    case BINDSTRING_USER_AGENT: {
        IInternetBindInfo *bindinfo = NULL;
        HRESULT hres;

        hres = IBindStatusCallback_QueryInterface(This->callback, &IID_IInternetBindInfo,
                                                  (void**)&bindinfo);
        if(FAILED(hres))
            return hres;

        hres = IInternetBindInfo_GetBindString(bindinfo, ulStringType, ppwzStr,
                                               cEl, pcElFetched);
        IInternetBindInfo_Release(bindinfo);

        return hres;
    }
    }

    FIXME("not supported string type %ld\n", ulStringType);
    return E_NOTIMPL;
}

#undef BINDF_THIS

static const IInternetBindInfoVtbl InternetBindInfoVtbl = {
    InternetBindInfo_QueryInterface,
    InternetBindInfo_AddRef,
    InternetBindInfo_Release,
    InternetBindInfo_GetBindInfo,
    InternetBindInfo_GetBindString
};

#define SERVPROV_THIS(iface) DEFINE_THIS(Binding, ServiceProvider, iface)

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface,
        REFIID riid, void **ppv)
{
    Binding *This = SERVPROV_THIS(iface);
    return IBinding_QueryInterface(BINDING(This), riid, ppv);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    Binding *This = SERVPROV_THIS(iface);
    return IBinding_AddRef(BINDING(This));
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    Binding *This = SERVPROV_THIS(iface);
    return IBinding_Release(BINDING(This));
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    Binding *This = SERVPROV_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    if(This->service_provider) {
        hres = IServiceProvider_QueryService(This->service_provider, guidService,
                                             riid, ppv);
        if(SUCCEEDED(hres))
            return hres;
    }

    if(IsEqualGUID(&IID_IHttpNegotiate, guidService)
       || IsEqualGUID(&IID_IHttpNegotiate2, guidService))
        return IHttpNegotiate2_QueryInterface(&HttpNegotiate, riid, ppv);

    WARN("unknown service %s\n", debugstr_guid(guidService));
    return E_NOTIMPL;
}

#undef SERVPROV_THIS

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static HRESULT get_callback(IBindCtx *pbc, IBindStatusCallback **callback)
{
    HRESULT hres;

    static WCHAR wszBSCBHolder[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };

    hres = IBindCtx_GetObjectParam(pbc, wszBSCBHolder, (IUnknown**)callback);
    if(FAILED(hres))
        return MK_E_SYNTAX;

    return S_OK;
}

static HRESULT get_protocol(Binding *This, LPCWSTR url)
{
    IClassFactory *cf = NULL;
    HRESULT hres;

    hres = IBindStatusCallback_QueryInterface(This->callback, &IID_IInternetProtocol,
            (void**)&This->protocol);
    if(SUCCEEDED(hres))
        return S_OK;

    if(This->service_provider) {
        hres = IServiceProvider_QueryService(This->service_provider, &IID_IInternetProtocol,
                &IID_IInternetProtocol, (void**)&This->protocol);
        if(SUCCEEDED(hres))
            return S_OK;
    }

    hres = get_protocol_handler(url, &cf);
    if(FAILED(hres))
        return hres;

    hres = IClassFactory_CreateInstance(cf, NULL, &IID_IInternetProtocol, (void**)&This->protocol);
    IClassFactory_Release(cf);

    return hres;
}

static HRESULT Binding_Create(LPCWSTR url, IBindCtx *pbc, REFIID riid, Binding **binding)
{
    Binding *ret;
    int len;
    HRESULT hres;

    static const WCHAR wszFile[] = {'f','i','l','e',':'};

    if(!IsEqualGUID(&IID_IStream, riid)) {
        FIXME("Unsupported riid %s\n", debugstr_guid(riid));
        return E_NOTIMPL;
    }

    URLMON_LockModule();

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(Binding));

    ret->lpBindingVtbl              = &BindingVtbl;
    ret->lpInternetProtocolSinkVtbl = &InternetProtocolSinkVtbl;
    ret->lpInternetBindInfoVtbl     = &InternetBindInfoVtbl;
    ret->lpServiceProviderVtbl      = &ServiceProviderVtbl;

    ret->ref = 1;

    ret->callback = NULL;
    ret->protocol = NULL;
    ret->service_provider = NULL;
    ret->stream = NULL;
    ret->mime = NULL;
    ret->url = NULL;
    ret->apartment_thread = GetCurrentThreadId();
    ret->notif_hwnd = get_notif_hwnd();
    ret->verified_mime = FALSE;
    ret->continue_call = 0;
    ret->task_queue_head = ret->task_queue_tail = NULL;

    memset(&ret->bindinfo, 0, sizeof(BINDINFO));
    ret->bindinfo.cbSize = sizeof(BINDINFO);
    ret->bindf = 0;

    InitializeCriticalSection(&ret->section);

    hres = get_callback(pbc, &ret->callback);
    if(FAILED(hres)) {
        WARN("Could not get IBindStatusCallback\n");
        IBinding_Release(BINDING(ret));
        return hres;
    }

    IBindStatusCallback_QueryInterface(ret->callback, &IID_IServiceProvider,
                                       (void**)&ret->service_provider);

    hres = get_protocol(ret, url);
    if(FAILED(hres)) {
        WARN("Could not get protocol handler\n");
        IBinding_Release(BINDING(ret));
        return hres;
    }

    hres = IBindStatusCallback_GetBindInfo(ret->callback, &ret->bindf, &ret->bindinfo);
    if(FAILED(hres)) {
        WARN("GetBindInfo failed: %08lx\n", hres);
        IBinding_Release(BINDING(ret));
        return hres;
    }

    dump_BINDINFO(&ret->bindinfo);

    ret->bindf |= BINDF_FROMURLMON;

    len = strlenW(url)+1;

    if(len < sizeof(wszFile)/sizeof(WCHAR) || memcmp(wszFile, url, sizeof(wszFile)))
        ret->bindf |= BINDF_NEEDFILE;

    ret->url = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
    memcpy(ret->url, url, len*sizeof(WCHAR));

    ret->stream = create_stream(ret->protocol);
    ret->stgmed.tymed = TYMED_ISTREAM;
    ret->stgmed.u.pstm = STREAM(ret->stream);
    ret->stgmed.pUnkForRelease = (IUnknown*)BINDING(ret); /* NOTE: Windows uses other IUnknown */

    *binding = ret;
    return S_OK;
}

HRESULT start_binding(LPCWSTR url, IBindCtx *pbc, REFIID riid, void **ppv)
{
    Binding *binding = NULL;
    HRESULT hres;

    *ppv = NULL;

    hres = Binding_Create(url, pbc, riid, &binding);
    if(FAILED(hres))
        return hres;

    hres = IBindStatusCallback_OnStartBinding(binding->callback, 0, BINDING(binding));
    if(FAILED(hres)) {
        WARN("OnStartBinding failed: %08lx\n", hres);
        IBindStatusCallback_OnStopBinding(binding->callback, 0x800c0008, NULL);
        IBinding_Release(BINDING(binding));
        return hres;
    }

    hres = IInternetProtocol_Start(binding->protocol, url, PROTSINK(binding),
             BINDINF(binding), 0, 0);

    if(FAILED(hres)) {
        WARN("Start failed: %08lx\n", hres);

        IInternetProtocol_Terminate(binding->protocol, 0);
        IBindStatusCallback_OnStopBinding(binding->callback, S_OK, NULL);
        IBinding_Release(BINDING(binding));

        return hres;
    }

    if(binding->stream->init_buf) {
        IInternetProtocol_UnlockRequest(binding->protocol);

        IStream_AddRef(STREAM(binding->stream));
        *ppv = binding->stream;

        hres = S_OK;
    }else {
        hres = MK_S_ASYNCHRONOUS;
    }

    IBinding_Release(BINDING(binding));

    return hres;
}
