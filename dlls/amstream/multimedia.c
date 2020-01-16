/*
 * Multimedia stream object
 *
 * Copyright 2004, 2012 Christian Costa
 * Copyright 2006 Ivan Leo Puoti
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

#include "wine/debug.h"

#define COBJMACROS

#include "winbase.h"
#include "wingdi.h"

#include "amstream_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(amstream);

struct multimedia_stream
{
    IAMMultiMediaStream IAMMultiMediaStream_iface;
    LONG ref;
    IGraphBuilder* pFilterGraph;
    IMediaSeeking* media_seeking;
    IMediaControl* media_control;
    IMediaStreamFilter *filter;
    IPin* ipin;
    STREAM_TYPE StreamType;
    OAEVENT event;
};

static inline struct multimedia_stream *impl_from_IAMMultiMediaStream(IAMMultiMediaStream *iface)
{
    return CONTAINING_RECORD(iface, struct multimedia_stream, IAMMultiMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI multimedia_stream_QueryInterface(IAMMultiMediaStream *iface,
        REFIID riid, void **ppvObject)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMultiMediaStream) ||
        IsEqualGUID(riid, &IID_IAMMultiMediaStream))
    {
        IAMMultiMediaStream_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);

    return E_NOINTERFACE;
}

static ULONG WINAPI multimedia_stream_AddRef(IAMMultiMediaStream *iface)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    TRACE("(%p/%p)\n", iface, This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI multimedia_stream_Release(IAMMultiMediaStream *iface)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)\n", iface, This);

    if (!ref)
    {
        if (This->ipin)
            IPin_Release(This->ipin);
        IMediaStreamFilter_Release(This->filter);
        if (This->media_seeking)
            IMediaSeeking_Release(This->media_seeking);
        if (This->media_control)
            IMediaControl_Release(This->media_control);
        if (This->pFilterGraph)
            IGraphBuilder_Release(This->pFilterGraph);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IMultiMediaStream methods ***/
static HRESULT WINAPI multimedia_stream_GetInformation(IAMMultiMediaStream *iface,
        DWORD *pdwFlags, STREAM_TYPE *pStreamType)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, pdwFlags, pStreamType);

    return E_NOTIMPL;
}

static HRESULT WINAPI multimedia_stream_GetMediaStream(IAMMultiMediaStream *iface,
        REFMSPID id, IMediaStream **stream)
{
    struct multimedia_stream *mmstream = impl_from_IAMMultiMediaStream(iface);

    TRACE("mmstream %p, id %s, stream %p.\n", mmstream, debugstr_guid(id), stream);

    return IMediaStreamFilter_GetMediaStream(mmstream->filter, id, stream);
}

static HRESULT WINAPI multimedia_stream_EnumMediaStreams(IAMMultiMediaStream *iface,
        LONG Index, IMediaStream **ppMediaStream)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%d,%p) stub!\n", This, iface, Index, ppMediaStream);

    return E_NOTIMPL;
}

static HRESULT WINAPI multimedia_stream_GetState(IAMMultiMediaStream *iface, STREAM_STATE *pCurrentState)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, pCurrentState);

    return E_NOTIMPL;
}

static HRESULT WINAPI multimedia_stream_SetState(IAMMultiMediaStream *iface, STREAM_STATE new_state)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p/%p)->(%u)\n", This, iface, new_state);

    if (new_state == STREAMSTATE_RUN)
        hr = IMediaControl_Run(This->media_control);
    else if (new_state == STREAMSTATE_STOP)
        hr = IMediaControl_Stop(This->media_control);

    return hr;
}

static HRESULT WINAPI multimedia_stream_GetTime(IAMMultiMediaStream *iface, STREAM_TIME *pCurrentTime)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, pCurrentTime);

    return E_NOTIMPL;
}

static HRESULT WINAPI multimedia_stream_GetDuration(IAMMultiMediaStream *iface, STREAM_TIME *pDuration)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, pDuration);

    return E_NOTIMPL;
}

static HRESULT WINAPI multimedia_stream_Seek(IAMMultiMediaStream *iface, STREAM_TIME seek_time)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    TRACE("(%p/%p)->(%s)\n", This, iface, wine_dbgstr_longlong(seek_time));

    return IMediaSeeking_SetPositions(This->media_seeking, &seek_time, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
}

static HRESULT WINAPI multimedia_stream_GetEndOfStream(IAMMultiMediaStream *iface, HANDLE *phEOS)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, phEOS);

    return E_NOTIMPL;
}

/*** IAMMultiMediaStream methods ***/
static HRESULT WINAPI multimedia_stream_Initialize(IAMMultiMediaStream *iface,
        STREAM_TYPE StreamType, DWORD dwFlags, IGraphBuilder *pFilterGraph)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);
    HRESULT hr = S_OK;

    TRACE("(%p/%p)->(%x,%x,%p)\n", This, iface, (DWORD)StreamType, dwFlags, pFilterGraph);

    if (pFilterGraph)
    {
        This->pFilterGraph = pFilterGraph;
        IGraphBuilder_AddRef(This->pFilterGraph);
    }
    else
    {
        hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (LPVOID*)&This->pFilterGraph);
    }

    if (SUCCEEDED(hr))
    {
        This->StreamType = StreamType;
        hr = IGraphBuilder_QueryInterface(This->pFilterGraph, &IID_IMediaSeeking, (void**)&This->media_seeking);
        if (SUCCEEDED(hr))
            hr = IGraphBuilder_QueryInterface(This->pFilterGraph, &IID_IMediaControl, (void**)&This->media_control);
        if (SUCCEEDED(hr))
            hr = IGraphBuilder_AddFilter(This->pFilterGraph, (IBaseFilter*)This->filter, L"MediaStreamFilter");
        if (SUCCEEDED(hr))
        {
            IMediaEventEx* media_event = NULL;
            hr = IGraphBuilder_QueryInterface(This->pFilterGraph, &IID_IMediaEventEx, (void**)&media_event);
            if (SUCCEEDED(hr))
                hr = IMediaEventEx_GetEventHandle(media_event, &This->event);
            if (SUCCEEDED(hr))
                hr = IMediaEventEx_SetNotifyFlags(media_event, AM_MEDIAEVENT_NONOTIFY);
            if (media_event)
                IMediaEventEx_Release(media_event);
        }
    }

    if (FAILED(hr))
    {
        if (This->media_seeking)
            IMediaSeeking_Release(This->media_seeking);
        This->media_seeking = NULL;
        if (This->media_control)
            IMediaControl_Release(This->media_control);
        This->media_control = NULL;
        if (This->pFilterGraph)
            IGraphBuilder_Release(This->pFilterGraph);
        This->pFilterGraph = NULL;
    }

    return hr;
}

static HRESULT WINAPI multimedia_stream_GetFilterGraph(IAMMultiMediaStream *iface,
        IGraphBuilder **ppGraphBuilder)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppGraphBuilder);

    if (!ppGraphBuilder)
        return E_POINTER;

    if (This->pFilterGraph)
        return IGraphBuilder_QueryInterface(This->pFilterGraph, &IID_IGraphBuilder, (void**)ppGraphBuilder);
    else
        *ppGraphBuilder = NULL;

    return S_OK;
}

static HRESULT WINAPI multimedia_stream_GetFilter(IAMMultiMediaStream *iface,
        IMediaStreamFilter **filter)
{
    struct multimedia_stream *mmstream = impl_from_IAMMultiMediaStream(iface);

    TRACE("mmstream %p, filter %p.\n", mmstream, filter);

    if (!filter)
        return E_POINTER;

    IMediaStreamFilter_AddRef(*filter = mmstream->filter);

    return S_OK;
}

static HRESULT WINAPI multimedia_stream_AddMediaStream(IAMMultiMediaStream *iface,
        IUnknown *stream_object, const MSPID *PurposeId, DWORD dwFlags, IMediaStream **ret_stream)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);
    HRESULT hr;
    IAMMediaStream* pStream;

    TRACE("mmstream %p, stream_object %p, id %s, flags %#x, ret_stream %p.\n",
            This, stream_object, debugstr_guid(PurposeId), dwFlags, ret_stream);

    if (!IsEqualGUID(PurposeId, &MSPID_PrimaryVideo) && !IsEqualGUID(PurposeId, &MSPID_PrimaryAudio))
        return MS_E_PURPOSEID;

    if (dwFlags & AMMSF_ADDDEFAULTRENDERER)
    {
        if (IsEqualGUID(PurposeId, &MSPID_PrimaryVideo))
        {
            /* Default renderer not supported by video stream */
            return MS_E_PURPOSEID;
        }
        else
        {
            IBaseFilter* dsoundrender_filter;

            /* Create the default renderer for audio */
            hr = CoCreateInstance(&CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&dsoundrender_filter);
            if (SUCCEEDED(hr))
            {
                 hr = IGraphBuilder_AddFilter(This->pFilterGraph, dsoundrender_filter, NULL);
                 IBaseFilter_Release(dsoundrender_filter);
            }

            /* No media stream created when the default renderer is used */
            return hr;
        }
    }

    if (IsEqualGUID(PurposeId, &MSPID_PrimaryVideo))
        hr = ddraw_stream_create((IMultiMediaStream*)iface, PurposeId, stream_object, This->StreamType, &pStream);
    else
        hr = audio_stream_create((IMultiMediaStream*)iface, PurposeId, stream_object, This->StreamType, &pStream);

    if (SUCCEEDED(hr))
    {
        /* Add stream to the media stream filter */
        IMediaStreamFilter_AddMediaStream(This->filter, pStream);
        if (ret_stream)
            *ret_stream = (IMediaStream *)pStream;
        else
            IAMMediaStream_Release(pStream);
    }

    return hr;
}

static HRESULT WINAPI multimedia_stream_OpenFile(IAMMultiMediaStream *iface,
        const WCHAR *filename, DWORD flags)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);
    HRESULT ret = S_OK;
    IBaseFilter *BaseFilter = NULL;
    IEnumPins *EnumPins = NULL;
    IPin *ipin;
    PIN_DIRECTION pin_direction;

    TRACE("(%p/%p)->(%s,%x)\n", This, iface, debugstr_w(filename), flags);

    if (!filename)
        return E_POINTER;

    /* If Initialize was not called before, we do it here */
    if (!This->pFilterGraph)
        ret = IAMMultiMediaStream_Initialize(iface, STREAMTYPE_READ, 0, NULL);

    if (SUCCEEDED(ret))
        ret = IGraphBuilder_AddSourceFilter(This->pFilterGraph, filename, L"Source", &BaseFilter);

    if (SUCCEEDED(ret))
        ret = IBaseFilter_EnumPins(BaseFilter, &EnumPins);

    if (SUCCEEDED(ret))
        ret = IEnumPins_Next(EnumPins, 1, &ipin, NULL);

    if (SUCCEEDED(ret))
    {
        ret = IPin_QueryDirection(ipin, &pin_direction);
        if (ret == S_OK && pin_direction == PINDIR_OUTPUT)
            This->ipin = ipin;
    }

    if (SUCCEEDED(ret) && !(flags & AMMSF_NORENDER))
        ret = IGraphBuilder_Render(This->pFilterGraph, This->ipin);

    if (EnumPins)
        IEnumPins_Release(EnumPins);
    if (BaseFilter)
        IBaseFilter_Release(BaseFilter);
    return ret;
}

static HRESULT WINAPI multimedia_stream_OpenMoniker(IAMMultiMediaStream *iface,
        IBindCtx *pCtx, IMoniker *pMoniker, DWORD dwFlags)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p,%p,%x) stub!\n", This, iface, pCtx, pMoniker, dwFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI multimedia_stream_Render(IAMMultiMediaStream *iface, DWORD dwFlags)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%x) partial stub!\n", This, iface, dwFlags);

    if(dwFlags != AMMSF_NOCLOCK)
        return E_INVALIDARG;

    return IGraphBuilder_Render(This->pFilterGraph, This->ipin);
}

static const IAMMultiMediaStreamVtbl multimedia_stream_vtbl =
{
    multimedia_stream_QueryInterface,
    multimedia_stream_AddRef,
    multimedia_stream_Release,
    multimedia_stream_GetInformation,
    multimedia_stream_GetMediaStream,
    multimedia_stream_EnumMediaStreams,
    multimedia_stream_GetState,
    multimedia_stream_SetState,
    multimedia_stream_GetTime,
    multimedia_stream_GetDuration,
    multimedia_stream_Seek,
    multimedia_stream_GetEndOfStream,
    multimedia_stream_Initialize,
    multimedia_stream_GetFilterGraph,
    multimedia_stream_GetFilter,
    multimedia_stream_AddMediaStream,
    multimedia_stream_OpenFile,
    multimedia_stream_OpenMoniker,
    multimedia_stream_Render
};

HRESULT multimedia_stream_create(IUnknown *outer, void **out)
{
    struct multimedia_stream *object;
    HRESULT hr;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IAMMultiMediaStream_iface.lpVtbl = &multimedia_stream_vtbl;
    object->ref = 1;

    if (FAILED(hr = CoCreateInstance(&CLSID_MediaStreamFilter, NULL,
            CLSCTX_INPROC_SERVER, &IID_IMediaStreamFilter, (void **)&object->filter)))
    {
        ERR("Failed to create stream filter, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    TRACE("Created multimedia stream %p.\n", object);
    *out = &object->IAMMultiMediaStream_iface;

    return S_OK;
}
