/*              DirectShow Media Detector object (QEDIT.DLL)
 *
 * Copyright 2008 Google (Lei Zhang)
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

#include "qedit_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qedit);

typedef struct MediaDetImpl {
    const IMediaDetVtbl *MediaDet_Vtbl;
    LONG refCount;
    IGraphBuilder *graph;
    IBaseFilter *source;
} MediaDetImpl;

static void MD_cleanup(MediaDetImpl *This)
{
    if (This->source) IBaseFilter_Release(This->source);
    This->source = NULL;
    if (This->graph) IGraphBuilder_Release(This->graph);
    This->graph = NULL;
}

static ULONG WINAPI MediaDet_AddRef(IMediaDet* iface)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);
    TRACE("(%p)->() AddRef from %d\n", This, refCount - 1);
    return refCount;
}

static ULONG WINAPI MediaDet_Release(IMediaDet* iface)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->refCount);
    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (refCount == 0)
    {
        MD_cleanup(This);
        CoTaskMemFree(This);
        return 0;
    }

    return refCount;
}

static HRESULT WINAPI MediaDet_QueryInterface(IMediaDet* iface, REFIID riid,
                                              void **ppvObject)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IMediaDet)) {
        MediaDet_AddRef(iface);
        *ppvObject = This;
        return S_OK;
    }
    *ppvObject = NULL;
    WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ppvObject);
    return E_NOINTERFACE;
}

static HRESULT WINAPI MediaDet_get_Filter(IMediaDet* iface, IUnknown **pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_put_Filter(IMediaDet* iface, IUnknown *newVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, newVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_OutputStreams(IMediaDet* iface, long *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_CurrentStream(IMediaDet* iface, long *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_put_CurrentStream(IMediaDet* iface, long newVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%ld): not implemented!\n", This, newVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_StreamType(IMediaDet* iface, GUID *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, debugstr_guid(pVal));
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_StreamTypeB(IMediaDet* iface, BSTR *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_StreamLength(IMediaDet* iface, double *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_Filename(IMediaDet* iface, BSTR *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    IFileSourceFilter *file;
    LPOLESTR name;
    HRESULT hr;

    TRACE("(%p)\n", This);

    if (!pVal)
        return E_POINTER;

    *pVal = NULL;
    /* MSDN says it should return E_FAIL if no file is open, but tests
       show otherwise.  */
    if (!This->source)
        return S_OK;

    hr = IBaseFilter_QueryInterface(This->source, &IID_IFileSourceFilter,
                                    (void **) &file);
    if (FAILED(hr))
        return hr;

    hr = IFileSourceFilter_GetCurFile(file, &name, NULL);
    IFileSourceFilter_Release(file);
    if (FAILED(hr))
        return hr;

    *pVal = SysAllocString(name);
    CoTaskMemFree(name);
    if (!*pVal)
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI MediaDet_put_Filename(IMediaDet* iface, BSTR newVal)
{
    static const WCHAR reader[] = {'R','e','a','d','e','r',0};
    MediaDetImpl *This = (MediaDetImpl *)iface;
    IGraphBuilder *gb;
    IBaseFilter *bf;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_w(newVal));

    if (This->graph)
    {
        WARN("MSDN says not to call this method twice\n");
        MD_cleanup(This);
    }

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IGraphBuilder, (void **) &gb);
    if (FAILED(hr))
        return hr;

    hr = IGraphBuilder_AddSourceFilter(gb, newVal, reader, &bf);
    if (FAILED(hr))
    {
        IGraphBuilder_Release(gb);
        return hr;
    }

    This->graph = gb;
    This->source = bf;
    return S_OK;
}

static HRESULT WINAPI MediaDet_GetBitmapBits(IMediaDet* iface,
                                             double StreamTime,
                                             long *pBufferSize, char *pBuffer,
                                             long Width, long Height)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%f %p %p %ld %ld): not implemented!\n", This, StreamTime, pBufferSize, pBuffer,
          Width, Height);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_WriteBitmapBits(IMediaDet* iface,
                                               double StreamTime, long Width,
                                               long Height, BSTR Filename)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%f %ld %ld %p): not implemented!\n", This, StreamTime, Width, Height, Filename);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_StreamMediaType(IMediaDet* iface,
                                                   AM_MEDIA_TYPE *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_GetSampleGrabber(IMediaDet* iface,
                                                ISampleGrabber **ppVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, ppVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_FrameRate(IMediaDet* iface, double *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_EnterBitmapGrabMode(IMediaDet* iface,
                                                   double SeekTime)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%f): not implemented!\n", This, SeekTime);
    return E_NOTIMPL;
}

static const IMediaDetVtbl IMediaDet_VTable =
{
    MediaDet_QueryInterface,
    MediaDet_AddRef,
    MediaDet_Release,
    MediaDet_get_Filter,
    MediaDet_put_Filter,
    MediaDet_get_OutputStreams,
    MediaDet_get_CurrentStream,
    MediaDet_put_CurrentStream,
    MediaDet_get_StreamType,
    MediaDet_get_StreamTypeB,
    MediaDet_get_StreamLength,
    MediaDet_get_Filename,
    MediaDet_put_Filename,
    MediaDet_GetBitmapBits,
    MediaDet_WriteBitmapBits,
    MediaDet_get_StreamMediaType,
    MediaDet_GetSampleGrabber,
    MediaDet_get_FrameRate,
    MediaDet_EnterBitmapGrabMode,
};

HRESULT MediaDet_create(IUnknown * pUnkOuter, LPVOID * ppv) {
    MediaDetImpl* obj = NULL;

    TRACE("(%p,%p)\n", ppv, pUnkOuter);

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    obj = CoTaskMemAlloc(sizeof(MediaDetImpl));
    if (NULL == obj) {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }
    ZeroMemory(obj, sizeof(MediaDetImpl));

    obj->refCount = 1;
    obj->MediaDet_Vtbl = &IMediaDet_VTable;
    obj->graph = NULL;
    obj->source = NULL;
    *ppv = obj;

    return S_OK;
}
