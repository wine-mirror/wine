/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "urlmon.h"
#include "itsstor.h"
#include "chm_lib.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(itss);

typedef struct {
    const IInternetProtocolVtbl  *lpInternetProtocolVtbl;

    LONG ref;

    ULONG offset;
    struct chmFile *chm_file;
    struct chmUnitInfo chm_object;
} ITSProtocol;

#define PROTOCOL(x)  ((IInternetProtocol*)  &(x)->lpInternetProtocolVtbl)

static void release_chm(ITSProtocol *This)
{
    if(This->chm_file) {
        chm_close(This->chm_file);
        This->chm_file = NULL;
    }
    This->offset = 0;
}

#define PROTOCOL_THIS(iface) DEFINE_THIS(ITSProtocol, InternetProtocol, iface)

static HRESULT WINAPI ITSProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = PROTOCOL(This);
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", This, ppv);
        *ppv = PROTOCOL(This);
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", This, ppv);
        *ppv = PROTOCOL(This);
    }

    if(*ppv) {
        IInternetProtocol_AddRef(iface);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ITSProtocol_AddRef(IInternetProtocol *iface)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI ITSProtocol_Release(IInternetProtocol *iface)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_chm(This);
        HeapFree(GetProcessHeap(), 0, This);

        ITSS_UnlockModule();
    }

    return ref;
}

static LPCWSTR skip_schema(LPCWSTR url)
{
    static const WCHAR its_schema[] = {'i','t','s',':'};
    static const WCHAR msits_schema[] = {'m','s','-','i','t','s',':'};
    static const WCHAR mk_schema[] = {'m','k',':','@','M','S','I','T','S','t','o','r','e',':'};

    if(!strncmpiW(its_schema, url, sizeof(its_schema)/sizeof(WCHAR)))
        return url+sizeof(its_schema)/sizeof(WCHAR);
    if(!strncmpiW(msits_schema, url, sizeof(msits_schema)/sizeof(WCHAR)))
        return url+sizeof(msits_schema)/sizeof(WCHAR);
    if(!strncmpiW(mk_schema, url, sizeof(mk_schema)/sizeof(WCHAR)))
        return url+sizeof(mk_schema)/sizeof(WCHAR);

    return NULL;
}

static HRESULT report_result(IInternetProtocolSink *sink, HRESULT hres)
{
    IInternetProtocolSink_ReportResult(sink, hres, 0, NULL);
    return hres;
}

static HRESULT WINAPI ITSProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, DWORD dwReserved)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    BINDINFO bindinfo;
    DWORD bindf = 0, len;
    LPWSTR file_name, mime;
    LPCWSTR object_name, path;
    struct chmFile *chm_file;
    struct chmUnitInfo chm_object;
    int res;
    HRESULT hres;

    static const WCHAR separator[] = {':',':',0};

    TRACE("(%p)->(%s %p %p %08x %d)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    path = skip_schema(szUrl);
    if(!path)
        return INET_E_USE_DEFAULT_PROTOCOLHANDLER;

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(BINDINFO);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &bindf, &bindinfo);
    if(FAILED(hres)) {
        WARN("GetBindInfo failed: %08x\n", hres);
        return hres;
    }

    ReleaseBindInfo(&bindinfo);

    object_name = strstrW(path, separator);
    if(!object_name) {
        WARN("invalid url\n");
        return report_result(pOIProtSink, STG_E_FILENOTFOUND);
    }

    len = object_name-path;
    file_name = HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR));
    memcpy(file_name, path, len*sizeof(WCHAR));
    file_name[len] = 0;
    chm_file = chm_openW(file_name);
    if(!chm_file) {
        WARN("Could not open chm file\n");
        return report_result(pOIProtSink, STG_E_FILENOTFOUND);
    }

    object_name += 2;
    memset(&chm_object, 0, sizeof(chm_object));
    res = chm_resolve_object(chm_file, object_name, &chm_object);
    if(res != CHM_RESOLVE_SUCCESS) {
        WARN("Could not resolve chm object\n");
        chm_close(chm_file);
        return report_result(pOIProtSink, STG_E_FILENOTFOUND);
    }

    IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_SENDINGREQUEST, object_name+1);

    /* FIXME: Native doesn't use FindMimeFromData */
    hres = FindMimeFromData(NULL, szUrl, NULL, 0, NULL, 0, &mime, 0);
    if(SUCCEEDED(hres)) {
        IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_MIMETYPEAVAILABLE, mime);
        CoTaskMemFree(mime);
    }

    release_chm(This); /* Native leaks handle here */
    This->chm_file = chm_file;
    memcpy(&This->chm_object, &chm_object, sizeof(chm_object));

    hres = IInternetProtocolSink_ReportData(pOIProtSink,
            BSCF_FIRSTDATANOTIFICATION|BSCF_DATAFULLYAVAILABLE,
            chm_object.length, chm_object.length);
    if(FAILED(hres)) {
        WARN("ReportData failed: %08x\n", hres);
        chm_close(chm_file);
        return report_result(pOIProtSink, hres);
    }

    hres = IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_BEGINDOWNLOADDATA, NULL);

    return report_result(pOIProtSink, hres);
}

static HRESULT WINAPI ITSProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    return S_OK;
}

static HRESULT WINAPI ITSProtocol_Suspend(IInternetProtocol *iface)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_Resume(IInternetProtocol *iface)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);

    if(!This->chm_file)
        return INET_E_DATA_NOT_AVAILABLE;

    *pcbRead = chm_retrieve_object(This->chm_file, &This->chm_object, pv, This->offset, cb);
    This->offset += *pcbRead;

    return *pcbRead ? S_OK : S_FALSE;
}

static HRESULT WINAPI ITSProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITSProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    return S_OK;
}

static HRESULT WINAPI ITSProtocol_UnlockRequest(IInternetProtocol *iface)
{
    ITSProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)\n", This);

    return S_OK;
}

#undef PROTOCOL_THIS

static const IInternetProtocolVtbl ITSProtocolVtbl = {
    ITSProtocol_QueryInterface,
    ITSProtocol_AddRef,
    ITSProtocol_Release,
    ITSProtocol_Start,
    ITSProtocol_Continue,
    ITSProtocol_Abort,
    ITSProtocol_Terminate,
    ITSProtocol_Suspend,
    ITSProtocol_Resume,
    ITSProtocol_Read,
    ITSProtocol_Seek,
    ITSProtocol_LockRequest,
    ITSProtocol_UnlockRequest
};

HRESULT ITSProtocol_create(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    ITSProtocol *ret;

    TRACE("(%p %p)\n", pUnkOuter, ppobj);

    ITSS_LockModule();

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ITSProtocol));

    ret->lpInternetProtocolVtbl = &ITSProtocolVtbl;
    ret->ref = 1;

    *ppobj = PROTOCOL(ret);

    return S_OK;
}
