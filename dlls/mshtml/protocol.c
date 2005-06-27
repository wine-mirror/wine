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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "docobj.h"

#include "mshtml.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

/********************************************************************
 * common ProtocolFactory implementation
 */

#define PROTOCOLINFO(x) ((IInternetProtocolInfo*) &(x)->lpInternetProtocolInfoVtbl)
#define CLASSFACTORY(x) ((IClassFactory*)         &(x)->lpClassFactoryVtbl)

typedef struct {
    const IInternetProtocolInfoVtbl *lpInternetProtocolInfoVtbl;
    const IClassFactoryVtbl         *lpClassFactoryVtbl;
} ProtocolFactory;

#define PROTOCOLINFO_THIS \
        ProtocolFactory* const This= \
            (ProtocolFactory*)((BYTE*)(iface)-offsetof(ProtocolFactory,lpInternetProtocolInfoVtbl));

static HRESULT WINAPI InternetProtocolInfo_QueryInterface(IInternetProtocolInfo *iface, REFIID riid, void **ppv)
{
    PROTOCOLINFO_THIS

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = PROTOCOLINFO(This);
    }else if(IsEqualGUID(&IID_IInternetProtocolInfo, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolInfo %p)\n", This, ppv);
        *ppv = PROTOCOLINFO(This);
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", This, ppv);
        *ppv = CLASSFACTORY(This);
    }

    if(!*ppv) {
        WARN("unknown interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IInternetProtocolInfo_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI InternetProtocolInfo_AddRef(IInternetProtocolInfo *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI InternetProtocolInfo_Release(IInternetProtocolInfo *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

#define CLASSFACTORY_THIS \
    ProtocolFactory* const This= \
        (ProtocolFactory*)((BYTE*)(iface)-offsetof(ProtocolFactory,lpClassFactoryVtbl));

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    CLASSFACTORY_THIS
    return IInternetProtocolInfo_QueryInterface(PROTOCOLINFO(This), riid, ppv);
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    CLASSFACTORY_THIS
    return IInternetProtocolInfo_AddRef(PROTOCOLINFO(This));
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    CLASSFACTORY_THIS
    return IInternetProtocolInfo_Release(PROTOCOLINFO(This));
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("(%p)->(%x)\n", iface, dolock);
    return S_OK;
}

/********************************************************************
 * AboutProtocol implementation
 */

typedef struct {
    const IInternetProtocolVtbl *lpInternetProtocolVtbl;
    ULONG ref;
} AboutProtocol;

static HRESULT WINAPI AboutProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        FIXME("IServiceProvider is not implemented\n");
        return E_NOINTERFACE;
    }

    if(!*ppv) {
        TRACE("unknown interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IInternetProtocol_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI AboutProtocol_AddRef(IInternetProtocol *iface)
{
    AboutProtocol *This = (AboutProtocol*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", iface, ref);
    return ref;
}

static ULONG WINAPI AboutProtocol_Release(IInternetProtocol *iface)
{
    AboutProtocol *This = (AboutProtocol*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lx\n", iface, ref);

    if(!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI AboutProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink* pOIProtSink, IInternetBindInfo* pOIBindInfo,
        DWORD grfPI, DWORD dwReserved)
{
    AboutProtocol *This = (AboutProtocol*)iface;
    FIXME("(%p)->(%s %p %p %08lx %ld)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA* pProtocolData)
{
    AboutProtocol *This = (AboutProtocol*)iface;
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    AboutProtocol *This = (AboutProtocol*)iface;
    FIXME("(%p)->(%08lx %08lx)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    AboutProtocol *This = (AboutProtocol*)iface;
    FIXME("(%p)->(%08lx)\n", This, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_Suspend(IInternetProtocol *iface)
{
    AboutProtocol *This = (AboutProtocol*)iface;
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_Resume(IInternetProtocol *iface)
{
    AboutProtocol *This = (AboutProtocol*)iface;
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_Read(IInternetProtocol *iface, void* pv, ULONG cb, ULONG* pcbRead)
{
    AboutProtocol *This = (AboutProtocol*)iface;
    FIXME("(%p)->(%lu %p)\n", This, cb, pcbRead);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
    AboutProtocol *This = (AboutProtocol*)iface;
    FIXME("(%p)->(%ld %ld %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    AboutProtocol *This = (AboutProtocol*)iface;
    FIXME("(%p)->(%ld)\n", This, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocol_UnlockRequest(IInternetProtocol *iface)
{
    AboutProtocol *This = (AboutProtocol*)iface;
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IInternetProtocolVtbl AboutProtocolVtbl = {
    AboutProtocol_QueryInterface,
    AboutProtocol_AddRef,
    AboutProtocol_Release,
    AboutProtocol_Start,
    AboutProtocol_Continue,
    AboutProtocol_Abort,
    AboutProtocol_Terminate,
    AboutProtocol_Suspend,
    AboutProtocol_Resume,
    AboutProtocol_Read,
    AboutProtocol_Seek,
    AboutProtocol_LockRequest,
    AboutProtocol_UnlockRequest
};

static HRESULT WINAPI AboutProtocolFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
        REFIID riid, void **ppv)
{
    AboutProtocol *ret;
    HRESULT hres;

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(AboutProtocol));
    ret->lpInternetProtocolVtbl = &AboutProtocolVtbl;
    ret->ref = 0;

    hres = IUnknown_QueryInterface((IUnknown*)ret, riid, ppv);

    if(FAILED(hres))
        HeapFree(GetProcessHeap(), 0, ret);

    return hres;
}

static HRESULT WINAPI AboutProtocolInfo_ParseUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD* pcchResult, DWORD dwReserved)
{
    FIXME("%p)->(%s %08x %08lx %p %ld %p %ld)\n", iface, debugstr_w(pwzUrl), ParseAction,
            dwParseFlags, pwzResult, cchResult, pcchResult, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocolInfo_CombineUrl(IInternetProtocolInfo *iface, LPCWSTR pwzBaseUrl,
        LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD* pcchResult, DWORD dwReserved)
{
    FIXME("%p)->(%s %s %08lx %p %ld %p %ld)\n", iface, debugstr_w(pwzBaseUrl), debugstr_w(pwzRelativeUrl),
            dwCombineFlags, pwzResult, cchResult, pcchResult, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocolInfo_CompareUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl1,
        LPCWSTR pwzUrl2, DWORD dwCompareFlags)
{
    FIXME("%p)->(%s %s %08lx)\n", iface, debugstr_w(pwzUrl1), debugstr_w(pwzUrl2), dwCompareFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI AboutProtocolInfo_QueryInfo(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        QUERYOPTION QueryOption, DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD* pcbBuf,
        DWORD dwReserved)
{
    FIXME("%p)->(%s %08x %08lx %p %ld %p %ld)\n", iface, debugstr_w(pwzUrl), QueryOption, dwQueryFlags, pBuffer,
            cbBuffer, pcbBuf, dwReserved);
    return E_NOTIMPL;
}

static const IInternetProtocolInfoVtbl AboutProtocolInfoVtbl = {
    InternetProtocolInfo_QueryInterface,
    InternetProtocolInfo_AddRef,
    InternetProtocolInfo_Release,
    AboutProtocolInfo_ParseUrl,
    AboutProtocolInfo_CombineUrl,
    AboutProtocolInfo_CompareUrl,
    AboutProtocolInfo_QueryInfo
};

static const IClassFactoryVtbl AboutProtocolFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    AboutProtocolFactory_CreateInstance,
    ClassFactory_LockServer
};

static ProtocolFactory AboutProtocolFactory = {
    &AboutProtocolInfoVtbl,
    &AboutProtocolFactoryVtbl
};

/********************************************************************
 * ResProtocol implementation
 */

typedef struct {
    const IInternetProtocolVtbl *lpInternetProtocolVtbl;
    ULONG ref;
} ResProtocol;

static HRESULT WINAPI ResProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        FIXME("IServiceProvider is not implemented\n");
        return E_NOINTERFACE;
    }

    if(!*ppv) {
        TRACE("unknown interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IInternetProtocol_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI ResProtocol_AddRef(IInternetProtocol *iface)
{
    ResProtocol *This = (ResProtocol*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", iface, ref);
    return ref;
}

static ULONG WINAPI ResProtocol_Release(IInternetProtocol *iface)
{
    ResProtocol *This = (ResProtocol*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lx\n", iface, ref);

    if(!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI ResProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink* pOIProtSink, IInternetBindInfo* pOIBindInfo,
        DWORD grfPI, DWORD dwReserved)
{
    ResProtocol *This = (ResProtocol*)iface;
    FIXME("(%p)->(%s %p %p %08lx %ld)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA* pProtocolData)
{
    ResProtocol *This = (ResProtocol*)iface;
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    ResProtocol *This = (ResProtocol*)iface;
    FIXME("(%p)->(%08lx %08lx)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    ResProtocol *This = (ResProtocol*)iface;
    FIXME("(%p)->(%08lx)\n", This, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_Suspend(IInternetProtocol *iface)
{
    ResProtocol *This = (ResProtocol*)iface;
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_Resume(IInternetProtocol *iface)
{
    ResProtocol *This = (ResProtocol*)iface;
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_Read(IInternetProtocol *iface, void* pv, ULONG cb, ULONG* pcbRead)
{
    ResProtocol *This = (ResProtocol*)iface;
    FIXME("(%p)->(%lu %p)\n", This, cb, pcbRead);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
    ResProtocol *This = (ResProtocol*)iface;
    FIXME("(%p)->(%ld %ld %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    ResProtocol *This = (ResProtocol*)iface;
    FIXME("(%p)->(%ld)\n", This, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocol_UnlockRequest(IInternetProtocol *iface)
{
    ResProtocol *This = (ResProtocol*)iface;
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IInternetProtocolVtbl ResProtocolVtbl = {
    ResProtocol_QueryInterface,
    ResProtocol_AddRef,
    ResProtocol_Release,
    ResProtocol_Start,
    ResProtocol_Continue,
    ResProtocol_Abort,
    ResProtocol_Terminate,
    ResProtocol_Suspend,
    ResProtocol_Resume,
    ResProtocol_Read,
    ResProtocol_Seek,
    ResProtocol_LockRequest,
    ResProtocol_UnlockRequest
};

static HRESULT WINAPI ResProtocolFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
        REFIID riid, void **ppv)
{
    ResProtocol *ret;
    HRESULT hres;

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(ResProtocol));
    ret->lpInternetProtocolVtbl = &ResProtocolVtbl;
    ret->ref = 0;

    hres = IUnknown_QueryInterface((IUnknown*)ret, riid, ppv);

    if(FAILED(hres))
        HeapFree(GetProcessHeap(), 0, ret);

    return hres;
}

static HRESULT WINAPI ResProtocolInfo_ParseUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD* pcchResult, DWORD dwReserved)
{
    FIXME("%p)->(%s %08x %08lx %p %ld %p %ld)\n", iface, debugstr_w(pwzUrl), ParseAction,
            dwParseFlags, pwzResult, cchResult, pcchResult, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocolInfo_CombineUrl(IInternetProtocolInfo *iface, LPCWSTR pwzBaseUrl,
        LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD* pcchResult, DWORD dwReserved)
{
    FIXME("%p)->(%s %s %08lx %p %ld %p %ld)\n", iface, debugstr_w(pwzBaseUrl), debugstr_w(pwzRelativeUrl),
            dwCombineFlags, pwzResult, cchResult, pcchResult, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocolInfo_CompareUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl1,
        LPCWSTR pwzUrl2, DWORD dwCompareFlags)
{
    FIXME("%p)->(%s %s %08lx)\n", iface, debugstr_w(pwzUrl1), debugstr_w(pwzUrl2), dwCompareFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI ResProtocolInfo_QueryInfo(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        QUERYOPTION QueryOption, DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD* pcbBuf,
        DWORD dwReserved)
{
    FIXME("%p)->(%s %08x %08lx %p %ld %p %ld)\n", iface, debugstr_w(pwzUrl), QueryOption, dwQueryFlags, pBuffer,
            cbBuffer, pcbBuf, dwReserved);
    return E_NOTIMPL;
}

static const IInternetProtocolInfoVtbl ResProtocolInfoVtbl = {
    InternetProtocolInfo_QueryInterface,
    InternetProtocolInfo_AddRef,
    InternetProtocolInfo_Release,
    ResProtocolInfo_ParseUrl,
    ResProtocolInfo_CombineUrl,
    ResProtocolInfo_CompareUrl,
    ResProtocolInfo_QueryInfo
};

static const IClassFactoryVtbl ResProtocolFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ResProtocolFactory_CreateInstance,
    ClassFactory_LockServer
};

static ProtocolFactory ResProtocolFactory = {
    &ResProtocolInfoVtbl,
    &ResProtocolFactoryVtbl
};

HRESULT ProtocolFactory_Create(REFCLSID rclsid, REFIID riid, void **ppv)
{
    ProtocolFactory *cf = NULL;

    if(IsEqualGUID(&CLSID_AboutProtocol, rclsid))
        cf = &AboutProtocolFactory;
    else if(IsEqualGUID(&CLSID_ResProtocol, rclsid))
        cf = &ResProtocolFactory;

    if(!cf) {
        FIXME("not implemented protocol %s\n", debugstr_guid(rclsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    return IUnknown_QueryInterface((IUnknown*)cf, riid, ppv);
}
