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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define CONTENT_LENGTH "Content-Length"
#define UTF16_STR "utf-16"

#define NSINSTREAM(x) ((nsIInputStream*) &(x)->lpInputStreamVtbl)

#define NSINSTREAM_THIS(iface) DEFINE_THIS(nsProtocolStream, InputStream, iface)

static nsresult NSAPI nsInputStream_QueryInterface(nsIInputStream *iface, nsIIDRef riid,
                                                   nsQIResult result)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);

    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result  = NSINSTREAM(This);
    }else if(IsEqualGUID(&IID_nsIInputStream, riid)) {
        TRACE("(%p)->(IID_nsIInputStream %p)\n", This, result);
        *result  = NSINSTREAM(This);
    }

    if(*result) {
        nsIInputStream_AddRef(NSINSTREAM(This));
        return NS_OK;
    }

    WARN("unsupported interface %s\n", debugstr_guid(riid));
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsInputStream_AddRef(nsIInputStream *iface)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}


static nsrefcnt NSAPI nsInputStream_Release(nsIInputStream *iface)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        mshtml_free(This);

    return ref;
}

static nsresult NSAPI nsInputStream_Close(nsIInputStream *iface)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);
    FIXME("(%p)\n", This);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsInputStream_Available(nsIInputStream *iface, PRUint32 *_retval)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsInputStream_Read(nsIInputStream *iface, char *aBuf, PRUint32 aCount,
                                         PRUint32 *_retval)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);

    TRACE("(%p)->(%p %d %p)\n", This, aBuf, aCount, _retval);

    /* Gecko always calls Read with big enough buffer */
    if(aCount < This->buf_size)
        FIXME("aCount < This->buf_size\n");

    *_retval = This->buf_size;
    if(This->buf_size)
        memcpy(aBuf, This->buf, This->buf_size);
    This->buf_size = 0;

    return NS_OK;
}

static nsresult NSAPI nsInputStream_ReadSegments(nsIInputStream *iface,
        nsresult (WINAPI *aWriter)(nsIInputStream*,void*,const char*,PRUint32,PRUint32,PRUint32*),
        void *aClousure, PRUint32 aCount, PRUint32 *_retval)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);
    PRUint32 written = 0;
    nsresult nsres;

    TRACE("(%p)->(%p %p %d %p)\n", This, aWriter, aClousure, aCount, _retval);

    if(!This->buf_size)
        return S_OK;

    if(This->buf_size > aCount)
        FIXME("buf_size > aCount\n");

    nsres = aWriter(NSINSTREAM(This), aClousure, This->buf, 0, This->buf_size, &written);
    if(NS_FAILED(nsres))
        TRACE("aWritter failed: %08x\n", nsres);
    else if(written != This->buf_size)
        FIXME("written %d != buf_size %d\n", written, This->buf_size);

    This->buf_size -= written; 

    *_retval = written;
    return nsres;
}

static nsresult NSAPI nsInputStream_IsNonBlocking(nsIInputStream *iface, PRBool *_retval)
{
    nsProtocolStream *This = NSINSTREAM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

#undef NSINSTREAM_THIS

static const nsIInputStreamVtbl nsInputStreamVtbl = {
    nsInputStream_QueryInterface,
    nsInputStream_AddRef,
    nsInputStream_Release,
    nsInputStream_Close,
    nsInputStream_Available,
    nsInputStream_Read,
    nsInputStream_ReadSegments,
    nsInputStream_IsNonBlocking
};

static nsProtocolStream *create_nsprotocol_stream(IStream *stream)
{
    nsProtocolStream *ret = mshtml_alloc(sizeof(nsProtocolStream));

    ret->lpInputStreamVtbl = &nsInputStreamVtbl;
    ret->ref = 1;
    ret->buf_size = 0;

    return ret;
}

#define STATUSCLB_THIS(iface) DEFINE_THIS(BSCallback, BindStatusCallback, iface)

static HRESULT WINAPI BindStatusCallback_QueryInterface(IBindStatusCallback *iface,
        REFIID riid, void **ppv)
{
    BSCallback *This = STATUSCLB_THIS(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown, %p)\n", This, ppv);
        *ppv = STATUSCLB(This);
    }else if(IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        TRACE("(%p)->(IID_IBindStatusCallback, %p)\n", This, ppv);
        *ppv = STATUSCLB(This);
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = SERVPROV(This);
    }else if(IsEqualGUID(&IID_IHttpNegotiate, riid)) {
        TRACE("(%p)->(IID_IHttpNegotiate %p)\n", This, ppv);
        *ppv = HTTPNEG(This);
    }else if(IsEqualGUID(&IID_IHttpNegotiate2, riid)) {
        TRACE("(%p)->(IID_IHttpNegotiate2 %p)\n", This, ppv);
        *ppv = HTTPNEG(This);
    }else if(IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        TRACE("(%p)->(IID_IInternetBindInfo %p)\n", This, ppv);
        *ppv = BINDINFO(This);
    }

    if(*ppv) {
        IBindStatusCallback_AddRef(STATUSCLB(This));
        return S_OK;
    }

    TRACE("Unsupported riid = %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI BindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI BindStatusCallback_Release(IBindStatusCallback *iface)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %d\n", This, ref);

    if(!ref) {
        if(This->post_data)
            GlobalFree(This->post_data);
        if(This->nschannel)
            nsIChannel_Release(NSCHANNEL(This->nschannel));
        if(This->nslistener)
            nsIStreamListener_Release(This->nslistener);
        if(This->nscontext)
            nsISupports_Release(This->nscontext);
        if(This->nsstream)
            nsIInputStream_Release(NSINSTREAM(This->nsstream));
        if(This->mon)
            IMoniker_Release(This->mon);
        if(This->binding)
            IBinding_Release(This->binding);
        mshtml_free(This->headers);
        mshtml_free(This);
    }

    return ref;
}

static HRESULT WINAPI BindStatusCallback_OnStartBinding(IBindStatusCallback *iface,
        DWORD dwReserved, IBinding *pbind)
{
    BSCallback *This = STATUSCLB_THIS(iface);

    TRACE("(%p)->(%d %p)\n", This, dwReserved, pbind);

    IBinding_AddRef(pbind);
    This->binding = pbind;

    if(This->nschannel && This->nschannel->load_group) {
        nsresult nsres = nsILoadGroup_AddRequest(This->nschannel->load_group,
                (nsIRequest*)NSCHANNEL(This->nschannel), This->nscontext);

        if(NS_FAILED(nsres))
            ERR("AddRequest failed:%08x\n", nsres);
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%d)\n", This, reserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    BSCallback *This = STATUSCLB_THIS(iface);

    TRACE("%p)->(%u %u %u %s)\n", This, ulProgress, ulProgressMax, ulStatusCode,
            debugstr_w(szStatusText));

    switch(ulStatusCode) {
    case BINDSTATUS_MIMETYPEAVAILABLE: {
        int len;

        if(!This->nschannel)
            return S_OK;
        mshtml_free(This->nschannel->content);

        len = WideCharToMultiByte(CP_ACP, 0, szStatusText, -1, NULL, 0, NULL, NULL);
        This->nschannel->content = mshtml_alloc(len*sizeof(WCHAR));
        WideCharToMultiByte(CP_ACP, 0, szStatusText, -1, This->nschannel->content, -1, NULL, NULL);
    }
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hresult, LPCWSTR szError)
{
    BSCallback *This = STATUSCLB_THIS(iface);

    TRACE("(%p)->(%08x %s)\n", This, hresult, debugstr_w(szError));

    IBinding_Release(This->binding);
    This->binding = NULL;

    if(This->nslistener) {
        nsIStreamListener_OnStopRequest(This->nslistener, (nsIRequest*)NSCHANNEL(This->nschannel),
                This->nscontext, NS_OK);

        if(This->nschannel->load_group) {
            nsresult nsres;

            nsres = nsILoadGroup_RemoveRequest(This->nschannel->load_group,
                    (nsIRequest*)NSCHANNEL(This->nschannel), NULL, NS_OK);
            if(NS_FAILED(nsres))
                ERR("RemoveRequest failed: %08x\n", nsres);
        }
    }

    if(This->doc) {
        task_t *task = mshtml_alloc(sizeof(task_t));

        task->doc = This->doc;
        task->task_id = TASK_PARSECOMPLETE;
        task->next = NULL;

        /*
         * This should be done in the worker thread that parses HTML,
         * but we don't have such thread (Gecko parses HTML for us).
         */
        push_task(task);
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    DWORD size;

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;

    size = pbindinfo->cbSize;
    memset(pbindinfo, 0, size);
    pbindinfo->cbSize = size;

    pbindinfo->cbstgmedData = This->post_data_len;
    pbindinfo->dwCodePage = CP_UTF8;
    pbindinfo->dwOptions = 0x80000;

    if(This->post_data) {
        pbindinfo->dwBindVerb = BINDVERB_POST;

        pbindinfo->stgmedData.tymed = TYMED_HGLOBAL;
        pbindinfo->stgmedData.u.hGlobal = This->post_data;
        pbindinfo->stgmedData.pUnkForRelease = (IUnknown*)STATUSCLB(This);
        IBindStatusCallback_AddRef(STATUSCLB(This));
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnDataAvailable(IBindStatusCallback *iface,
        DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%08x %d %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);

    if(This->nslistener) {
        if(!This->nsstream)
            This->nsstream = create_nsprotocol_stream(pstgmed->u.pstm);

        do {
            hres = IStream_Read(pstgmed->u.pstm, This->nsstream->buf, sizeof(This->nsstream->buf),
                         &This->nsstream->buf_size);
            if(!This->nsstream->buf_size)
                break;

            if(!This->readed && This->nsstream->buf_size >= 2 && *(WORD*)This->nsstream->buf == 0xfeff) {
                This->nschannel->charset = mshtml_alloc(sizeof(UTF16_STR));
                memcpy(This->nschannel->charset, UTF16_STR, sizeof(UTF16_STR));
            }

            if(!This->readed) {
                nsres = nsIStreamListener_OnStartRequest(This->nslistener,
                        (nsIRequest*)NSCHANNEL(This->nschannel), This->nscontext);
                if(NS_FAILED(nsres))
                    FIXME("OnStartRequest failed: %08x\n", nsres);
            }

            This->readed += This->nsstream->buf_size;

            nsres = nsIStreamListener_OnDataAvailable(This->nslistener,
                    (nsIRequest*)NSCHANNEL(This->nschannel), This->nscontext,
                    NSINSTREAM(This->nsstream), This->readed, This->nsstream->buf_size);
            if(NS_FAILED(nsres))
                ERR("OnDataAvailable failed: %08x\n", nsres);

            if(This->nsstream->buf_size)
                FIXME("buffer is not empty!\n");
        }while(hres == S_OK);
    }else {
        BYTE buf[1024];
        DWORD read;
        do {
            read = 0;
            hres = IStream_Read(pstgmed->u.pstm, buf, sizeof(buf), &read);
        }while(hres == S_OK && read);
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown *punk)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), punk);
    return E_NOTIMPL;
}

#undef STATUSCLB_THIS

static const IBindStatusCallbackVtbl BindStatusCallbackVtbl = {
    BindStatusCallback_QueryInterface,
    BindStatusCallback_AddRef,
    BindStatusCallback_Release,
    BindStatusCallback_OnStartBinding,
    BindStatusCallback_GetPriority,
    BindStatusCallback_OnLowResource,
    BindStatusCallback_OnProgress,
    BindStatusCallback_OnStopBinding,
    BindStatusCallback_GetBindInfo,
    BindStatusCallback_OnDataAvailable,
    BindStatusCallback_OnObjectAvailable
};

#define HTTPNEG_THIS(iface) DEFINE_THIS(BSCallback, HttpNegotiate2, iface)

static HRESULT WINAPI HttpNegotiate_QueryInterface(IHttpNegotiate2 *iface,
                                                   REFIID riid, void **ppv)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_QueryInterface(STATUSCLB(This), riid, ppv);
}

static ULONG WINAPI HttpNegotiate_AddRef(IHttpNegotiate2 *iface)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_AddRef(STATUSCLB(This));
}

static ULONG WINAPI HttpNegotiate_Release(IHttpNegotiate2 *iface)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_Release(STATUSCLB(This));
}

static HRESULT WINAPI HttpNegotiate_BeginningTransaction(IHttpNegotiate2 *iface,
        LPCWSTR szURL, LPCWSTR szHeaders, DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    DWORD size;

    TRACE("(%p)->(%s %s %d %p)\n", This, debugstr_w(szURL), debugstr_w(szHeaders),
          dwReserved, pszAdditionalHeaders);

    if(!This->headers) {
        *pszAdditionalHeaders = NULL;
        return S_OK;
    }

    size = (strlenW(This->headers)+1)*sizeof(WCHAR);
    *pszAdditionalHeaders = CoTaskMemAlloc(size);
    memcpy(*pszAdditionalHeaders, This->headers, size);

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate2 *iface, DWORD dwResponseCode,
        LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    FIXME("(%p)->(%d %s %s %p)\n", This, dwResponseCode, debugstr_w(szResponseHeaders),
          debugstr_w(szRequestHeaders), pszAdditionalRequestHeaders);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpNegotiate_GetRootSecurityId(IHttpNegotiate2 *iface,
        BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    FIXME("(%p)->(%p %p %ld)\n", This, pbSecurityId, pcbSecurityId, dwReserved);
    return E_NOTIMPL;
}

#undef HTTPNEG

static const IHttpNegotiate2Vtbl HttpNegotiate2Vtbl = {
    HttpNegotiate_QueryInterface,
    HttpNegotiate_AddRef,
    HttpNegotiate_Release,
    HttpNegotiate_BeginningTransaction,
    HttpNegotiate_OnResponse,
    HttpNegotiate_GetRootSecurityId
};

#define BINDINFO_THIS(iface) DEFINE_THIS(BSCallback, InternetBindInfo, iface)

static HRESULT WINAPI InternetBindInfo_QueryInterface(IInternetBindInfo *iface,
                                                      REFIID riid, void **ppv)
{
    BSCallback *This = BINDINFO_THIS(iface);
    return IBindStatusCallback_QueryInterface(STATUSCLB(This), riid, ppv);
}

static ULONG WINAPI InternetBindInfo_AddRef(IInternetBindInfo *iface)
{
    BSCallback *This = BINDINFO_THIS(iface);
    return IBindStatusCallback_AddRef(STATUSCLB(This));
}

static ULONG WINAPI InternetBindInfo_Release(IInternetBindInfo *iface)
{
    BSCallback *This = BINDINFO_THIS(iface);
    return IBindStatusCallback_Release(STATUSCLB(This));
}

static HRESULT WINAPI InternetBindInfo_GetBindInfo(IInternetBindInfo *iface,
                                                   DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BSCallback *This = BINDINFO_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetBindInfo_GetBindString(IInternetBindInfo *iface,
        ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    BSCallback *This = BINDINFO_THIS(iface);
    FIXME("(%p)->(%u %p %u %p)\n", This, ulStringType, ppwzStr, cEl, pcElFetched);
    return E_NOTIMPL;
}

#undef BINDINFO_THIS

static const IInternetBindInfoVtbl InternetBindInfoVtbl = {
    InternetBindInfo_QueryInterface,
    InternetBindInfo_AddRef,
    InternetBindInfo_Release,
    InternetBindInfo_GetBindInfo,
    InternetBindInfo_GetBindString
};

#define SERVPROV_THIS(iface) DEFINE_THIS(BSCallback, ServiceProvider, iface)

static HRESULT WINAPI BSCServiceProvider_QueryInterface(IServiceProvider *iface,
                                                        REFIID riid, void **ppv)
{
    BSCallback *This = SERVPROV_THIS(iface);
    return IBindStatusCallback_QueryInterface(STATUSCLB(This), riid, ppv);
}

static ULONG WINAPI BSCServiceProvider_AddRef(IServiceProvider *iface)
{
    BSCallback *This = SERVPROV_THIS(iface);
    return IBindStatusCallback_AddRef(STATUSCLB(This));
}

static ULONG WINAPI BSCServiceProvider_Release(IServiceProvider *iface)
{
    BSCallback *This = SERVPROV_THIS(iface);
    return IBindStatusCallback_Release(STATUSCLB(This));
}

static HRESULT WINAPI BSCServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    BSCallback *This = SERVPROV_THIS(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

#undef SERVPROV_THIS

static const IServiceProviderVtbl ServiceProviderVtbl = {
    BSCServiceProvider_QueryInterface,
    BSCServiceProvider_AddRef,
    BSCServiceProvider_Release,
    BSCServiceProvider_QueryService
};

BSCallback *create_bscallback(IMoniker *mon)
{
    BSCallback *ret = mshtml_alloc(sizeof(BSCallback));

    ret->lpBindStatusCallbackVtbl = &BindStatusCallbackVtbl;
    ret->lpServiceProviderVtbl    = &ServiceProviderVtbl;
    ret->lpHttpNegotiate2Vtbl     = &HttpNegotiate2Vtbl;
    ret->lpInternetBindInfoVtbl   = &InternetBindInfoVtbl;
    ret->ref = 1;
    ret->post_data = NULL;
    ret->headers = NULL;
    ret->post_data_len = 0;
    ret->readed = 0;
    ret->nschannel = NULL;
    ret->nslistener = NULL;
    ret->nscontext = NULL;
    ret->nsstream = NULL;
    ret->binding = NULL;
    ret->doc = NULL;

    if(mon)
        IMoniker_AddRef(mon);
    ret->mon = mon;

    return ret;
}

static void parse_post_data(nsIInputStream *post_data_stream, LPWSTR *headers_ret,
                            HGLOBAL *post_data_ret, ULONG *post_data_len_ret)
{
    PRUint32 post_data_len = 0, available = 0;
    HGLOBAL post_data = NULL;
    LPWSTR headers = NULL;
    DWORD headers_len = 0, len;
    const char *ptr, *ptr2, *post_data_end;

    nsIInputStream_Available(post_data_stream, &available);
    post_data = GlobalAlloc(0, available+1);
    nsIInputStream_Read(post_data_stream, post_data, available, &post_data_len);
    
    TRACE("post_data = %s\n", debugstr_an(post_data, post_data_len));

    ptr = ptr2 = post_data;
    post_data_end = (const char*)post_data+post_data_len;

    while(ptr < post_data_end && (*ptr != '\r' || ptr[1] != '\n')) {
        while(ptr < post_data_end && (*ptr != '\r' || ptr[1] != '\n'))
            ptr++;

        if(!*ptr) {
            FIXME("*ptr = 0\n");
            return;
        }

        ptr += 2;

        if(ptr-ptr2 >= sizeof(CONTENT_LENGTH)
           && CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                             CONTENT_LENGTH, sizeof(CONTENT_LENGTH)-1,
                             ptr2, sizeof(CONTENT_LENGTH)-1) == CSTR_EQUAL) {
            ptr2 = ptr;
            continue;
        }

        len = MultiByteToWideChar(CP_ACP, 0, ptr2, ptr-ptr2, NULL, 0);

        if(headers)
            headers = mshtml_realloc(headers,(headers_len+len+1)*sizeof(WCHAR));
        else
            headers = mshtml_alloc((len+1)*sizeof(WCHAR));

        len = MultiByteToWideChar(CP_ACP, 0, ptr2, ptr-ptr2, headers+headers_len, -1);
        headers_len += len;

        ptr2 = ptr;
    }

    headers[headers_len] = 0;
    *headers_ret = headers;

    if(ptr >= post_data_end-2) {
        GlobalFree(post_data);
        return;
    }

    ptr += 2;

    if(headers_len) {
        post_data_len -= ptr-(const char*)post_data;
        memmove(post_data, ptr, post_data_len);
        post_data = GlobalReAlloc(post_data, post_data_len+1, 0);
    }

    *post_data_ret = post_data;
    *post_data_len_ret = post_data_len;
}

void hlink_frame_navigate(HTMLDocument *doc, IHlinkFrame *hlink_frame,
                          LPCWSTR uri, nsIInputStream *post_data_stream, DWORD hlnf)
{
    BSCallback *callback;
    IBindCtx *bindctx;
    IMoniker *mon;
    IHlink *hlink;

    callback = create_bscallback(NULL);

    if(post_data_stream) {
        parse_post_data(post_data_stream, &callback->headers, &callback->post_data,
                        &callback->post_data_len);
        TRACE("headers = %s post_data = %s\n", debugstr_w(callback->headers),
              debugstr_an(callback->post_data, callback->post_data_len));
    }

    CreateAsyncBindCtx(0, STATUSCLB(callback), NULL, &bindctx);

    hlink = Hlink_Create();

    CreateURLMoniker(NULL, uri, &mon);
    IHlink_SetMonikerReference(hlink, 0, mon, NULL);

    if(hlnf & HLNF_OPENINNEWWINDOW) {
        static const WCHAR wszBlank[] = {'_','b','l','a','n','k',0};
        IHlink_SetTargetFrameName(hlink, wszBlank); /* FIXME */
    }

    IHlinkFrame_Navigate(hlink_frame, hlnf, bindctx, STATUSCLB(callback), hlink);

    IBindCtx_Release(bindctx);
    IBindStatusCallback_Release(STATUSCLB(callback));
    IMoniker_Release(mon);

}

HRESULT start_binding(BSCallback *bscallback)
{
    IStream *str = NULL;
    IBindCtx *bctx;
    HRESULT hres;

    hres = CreateAsyncBindCtx(0, STATUSCLB(bscallback), NULL, &bctx);
    if(FAILED(hres)) {
        WARN("CreateAsyncBindCtx failed: %08x\n", hres);
        return hres;
    }

    hres = IMoniker_BindToStorage(bscallback->mon, bctx, NULL, &IID_IStream, (void**)&str);
    IBindCtx_Release(bctx);
    if(FAILED(hres)) {
        WARN("BindToStorage failed: %08x\n", hres);
        return hres;
    }

    if(str)
        IStream_Release(str);

    IMoniker_Release(bscallback->mon);
    bscallback->mon = NULL;
    return S_OK;
}

void set_document_bscallback(HTMLDocument *doc, BSCallback *callback)
{
    if(doc->bscallback) {
        if(doc->bscallback->binding)
            IBinding_Abort(doc->bscallback->binding);
        doc->bscallback->doc = NULL;
        IBindStatusCallback_Release(STATUSCLB(doc->bscallback));
    }

    doc->bscallback = callback;

    if(callback) {
        IBindStatusCallback_AddRef(STATUSCLB(callback));
        callback->doc = doc;
    }
}
