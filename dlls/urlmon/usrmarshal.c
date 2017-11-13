/*
 * Copyright 2009 Piotr Caban for CodeWeavers
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

#define NONAMELESSUNION

#include "urlmon_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

HRESULT CALLBACK IWinInetHttpInfo_QueryInfo_Proxy(IWinInetHttpInfo* This,
    DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf, DWORD *pdwFlags,
    DWORD *pdwReserved)
{
    TRACE("(%p %x %p %p %p %p)\n", This, dwOption, pBuffer, pcbBuf, pdwFlags, pdwReserved);
    return IWinInetHttpInfo_RemoteQueryInfo_Proxy(This, dwOption, pBuffer, pcbBuf, pdwFlags, pdwReserved);
}

HRESULT __RPC_STUB IWinInetHttpInfo_QueryInfo_Stub(IWinInetHttpInfo* This,
    DWORD dwOption, BYTE *pBuffer, DWORD *pcbBuf, DWORD *pdwFlags,
    DWORD *pdwReserved)
{
    TRACE("(%p %x %p %p %p %p)\n", This, dwOption, pBuffer, pcbBuf, pdwFlags, pdwReserved);
    return IWinInetHttpInfo_QueryInfo(This, dwOption, pBuffer, pcbBuf, pdwFlags, pdwReserved);
}

HRESULT CALLBACK IWinInetInfo_QueryOption_Proxy(IWinInetInfo* This,
        DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf)
{
    TRACE("(%p %x %p %p)\n", This, dwOption, pBuffer, pcbBuf);
    return IWinInetInfo_RemoteQueryOption_Proxy(This, dwOption, pBuffer, pcbBuf);
}

HRESULT __RPC_STUB IWinInetInfo_QueryOption_Stub(IWinInetInfo* This,
        DWORD dwOption, BYTE *pBuffer, DWORD *pcbBuf)
{
    TRACE("(%p %x %p %p)\n", This, dwOption, pBuffer, pcbBuf);
    return IWinInetInfo_QueryOption(This, dwOption, pBuffer, pcbBuf);
}

HRESULT CALLBACK IBindHost_MonikerBindToStorage_Proxy(IBindHost* This,
        IMoniker *moniker, IBindCtx *bc, IBindStatusCallback *bsc,
        REFIID riid, void **obj)
{
    TRACE("(%p %p %p %p %s %p)\n", This, moniker, bc, bsc, debugstr_guid(riid), obj);
    return IBindHost_RemoteMonikerBindToStorage_Proxy(This, moniker, bc, bsc, riid, (IUnknown**)obj);
}

HRESULT __RPC_STUB IBindHost_MonikerBindToStorage_Stub(IBindHost* This,
        IMoniker *moniker, IBindCtx *bc, IBindStatusCallback *bsc,
        REFIID riid, IUnknown **obj)
{
    TRACE("(%p %p %p %p %s %p)\n", This, moniker, bc, bsc, debugstr_guid(riid), obj);
    return IBindHost_MonikerBindToStorage(This, moniker, bc, bsc, riid, (void**)obj);
}

HRESULT CALLBACK IBindHost_MonikerBindToObject_Proxy(IBindHost* This,
        IMoniker *moniker, IBindCtx *bc, IBindStatusCallback *bsc,
        REFIID riid, void **obj)
{
    TRACE("(%p %p %p %p %s %p)\n", This, moniker, bc, bsc, debugstr_guid(riid), obj);
    return IBindHost_RemoteMonikerBindToObject_Proxy(This, moniker, bc, bsc, riid, (IUnknown**)obj);
}

HRESULT __RPC_STUB IBindHost_MonikerBindToObject_Stub(IBindHost* This,
        IMoniker *moniker, IBindCtx *bc, IBindStatusCallback *bsc,
        REFIID riid, IUnknown **obj)
{
    TRACE("(%p %p %p %p %s %p)\n", This, moniker, bc, bsc, debugstr_guid(riid), obj);
    return IBindHost_MonikerBindToObject(This, moniker, bc, bsc, riid, (void**)obj);
}

static HRESULT marshal_stgmed(STGMEDIUM *stgmed, RemSTGMEDIUM **ret)
{
    RemSTGMEDIUM *rem_stgmed;
    IStream *stream = NULL;
    ULONG size = 0;
    HRESULT hres = S_OK;

    if((stgmed->tymed == TYMED_ISTREAM && stgmed->u.pstm) || stgmed->pUnkForRelease) {
        hres = CreateStreamOnHGlobal(NULL, TRUE, &stream);
        if(FAILED(hres))
            return hres;
    }

    switch(stgmed->tymed) {
    case TYMED_NULL:
        break;
    case TYMED_ISTREAM:
        if(stgmed->u.pstm)
            hres = CoMarshalInterface(stream, &IID_IStream, (IUnknown*)stgmed->u.pstm,
                                      MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL);
        break;
    default:
        FIXME("unsupported tymed %u\n", stgmed->tymed);
        break;
    }

    if(SUCCEEDED(hres) && stgmed->pUnkForRelease)
        hres = CoMarshalInterface(stream, &IID_IUnknown, stgmed->pUnkForRelease,
                                  MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL);
    if(FAILED(hres)) {
        if(stream)
            IStream_Release(stream);
        return hres;
    }

    if(stream) {
        LARGE_INTEGER zero;
        ULARGE_INTEGER off;

        zero.QuadPart = 0;
        IStream_Seek(stream, zero, STREAM_SEEK_CUR, &off);
        size = off.QuadPart;
        IStream_Seek(stream, zero, STREAM_SEEK_SET, &off);
    }

    rem_stgmed = heap_alloc_zero(FIELD_OFFSET(RemSTGMEDIUM, data[size]));
    if(!rem_stgmed) {
        if(stream)
            IStream_Release(stream);
        return E_OUTOFMEMORY;
    }

    rem_stgmed->tymed = stgmed->tymed;
    rem_stgmed->dwHandleType = 0;
    rem_stgmed->pData = stgmed->u.pstm != NULL;
    rem_stgmed->pUnkForRelease = stgmed->pUnkForRelease != NULL;
    rem_stgmed->cbData = size;
    if(stream) {
        IStream_Read(stream, rem_stgmed->data, size, &size);
        IStream_Release(stream);
    }

    *ret = rem_stgmed;
    return S_OK;
}

static HRESULT unmarshal_stgmed(RemSTGMEDIUM *rem_stgmed, STGMEDIUM *stgmed)
{
    IStream *stream = NULL;
    HRESULT hres = S_OK;

    stgmed->tymed = rem_stgmed->tymed;

    if((stgmed->tymed == TYMED_ISTREAM && rem_stgmed->pData) || rem_stgmed->pUnkForRelease) {
        LARGE_INTEGER zero;

        hres = CreateStreamOnHGlobal(NULL, TRUE, &stream);
        if(FAILED(hres))
            return hres;

        hres = IStream_Write(stream, rem_stgmed->data, rem_stgmed->cbData, NULL);
        if(FAILED(hres)) {
            IStream_Release(stream);
            return hres;
        }

        zero.QuadPart = 0;
        IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);
    }

    switch(stgmed->tymed) {
    case TYMED_NULL:
        break;
    case TYMED_ISTREAM:
        if(rem_stgmed->pData)
            hres = CoUnmarshalInterface(stream, &IID_IStream, (void**)&stgmed->u.pstm);
        break;
    default:
        FIXME("unsupported tymed %u\n", stgmed->tymed);
        break;
    }

    if(SUCCEEDED(hres) && rem_stgmed->pUnkForRelease)
        hres = CoUnmarshalInterface(stream, &IID_IUnknown, (void**)&stgmed->pUnkForRelease);
    if(stream)
        IStream_Release(stream);
    return hres;
}

HRESULT CALLBACK IBindStatusCallbackEx_GetBindInfoEx_Proxy(
        IBindStatusCallbackEx* This, DWORD *grfBINDF, BINDINFO *pbindinfo,
        DWORD *grfBINDF2, DWORD *pdwReserved)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IBindStatusCallbackEx_GetBindInfoEx_Stub(
        IBindStatusCallbackEx* This, DWORD *grfBINDF, RemBINDINFO *pbindinfo,
        RemSTGMEDIUM *pstgmed, DWORD *grfBINDF2, DWORD *pdwReserved)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IBindStatusCallback_GetBindInfo_Proxy(
        IBindStatusCallback* This, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IBindStatusCallback_GetBindInfo_Stub(
        IBindStatusCallback* This, DWORD *grfBINDF,
        RemBINDINFO *pbindinfo, RemSTGMEDIUM *pstgmed)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT CALLBACK IBindStatusCallback_OnDataAvailable_Proxy(
        IBindStatusCallback* This, DWORD grfBSCF, DWORD dwSize,
        FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    RemFORMATETC rem_formatetc;
    RemSTGMEDIUM *rem_stgmed;
    HRESULT hres;

    TRACE("(%p)->(%x %u %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);

    hres = marshal_stgmed(pstgmed, &rem_stgmed);
    if(FAILED(hres))
        return hres;

    rem_formatetc.cfFormat = pformatetc->cfFormat;
    rem_formatetc.ptd = 0;
    rem_formatetc.dwAspect = pformatetc->dwAspect;
    rem_formatetc.lindex = pformatetc->lindex;
    rem_formatetc.tymed = pformatetc->tymed;

    hres = IBindStatusCallback_RemoteOnDataAvailable_Proxy(This, grfBSCF, dwSize, &rem_formatetc, rem_stgmed);

    heap_free(rem_stgmed);
    return hres;
}

HRESULT __RPC_STUB IBindStatusCallback_OnDataAvailable_Stub(
        IBindStatusCallback* This, DWORD grfBSCF, DWORD dwSize,
        RemFORMATETC *pformatetc, RemSTGMEDIUM *pstgmed)
{
    STGMEDIUM stgmed = { TYMED_NULL };
    FORMATETC formatetc;
    HRESULT hres;

    TRACE("(%p)->(%x %u %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);

    hres = unmarshal_stgmed(pstgmed, &stgmed);
    if(FAILED(hres))
        return hres;

    formatetc.cfFormat = pformatetc->cfFormat;
    formatetc.ptd = NULL;
    formatetc.dwAspect = pformatetc->dwAspect;
    formatetc.lindex = pformatetc->lindex;
    formatetc.tymed = pformatetc->tymed;

    hres = IBindStatusCallback_OnDataAvailable(This, grfBSCF, dwSize, &formatetc, &stgmed);

    ReleaseStgMedium(&stgmed);
    return hres;
}

HRESULT CALLBACK IBinding_GetBindResult_Proxy(IBinding* This,
        CLSID *pclsidProtocol, DWORD *pdwResult,
        LPOLESTR *pszResult, DWORD *pdwReserved)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IBinding_GetBindResult_Stub(IBinding* This,
        CLSID *pclsidProtocol, DWORD *pdwResult,
        LPOLESTR *pszResult, DWORD dwReserved)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}
