/*
 * Implementation of IMediaStream Interfaces
 *
 * Copyright 2005, 2008, 2012 Christian Costa
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
#include "amstream.h"

#include "ddstream.h"

WINE_DEFAULT_DEBUG_CHANNEL(amstream);

static HRESULT ddrawstreamsample_create(IDirectDrawMediaStream *parent, IDirectDrawStreamSample **ddraw_stream_sample);
static HRESULT audiostreamsample_create(IAudioMediaStream *parent, IAudioData *audio_data, IAudioStreamSample **audio_stream_sample);

typedef struct {
    IDirectDrawMediaStream IDirectDrawMediaStream_iface;
    LONG ref;
    IMultiMediaStream* Parent;
    MSPID PurposeId;
    STREAM_TYPE StreamType;
} IDirectDrawMediaStreamImpl;

static inline IDirectDrawMediaStreamImpl *impl_from_IDirectDrawMediaStream(IDirectDrawMediaStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawMediaStreamImpl, IDirectDrawMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectDrawMediaStreamImpl_QueryInterface(IDirectDrawMediaStream *iface,
        REFIID riid, void **ppv)
{
    IDirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMediaStream) ||
        IsEqualGUID(riid, &IID_IDirectDrawMediaStream))
    {
        IDirectDrawMediaStream_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectDrawMediaStreamImpl_AddRef(IDirectDrawMediaStream *iface)
{
    IDirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    TRACE("(%p/%p)\n", iface, This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IDirectDrawMediaStreamImpl_Release(IDirectDrawMediaStream *iface)
{
    IDirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)\n", iface, This);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IMediaStream methods ***/
static HRESULT WINAPI IDirectDrawMediaStreamImpl_GetMultiMediaStream(IDirectDrawMediaStream *iface,
        IMultiMediaStream** ppMultiMediaStream)
{
    IDirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, ppMultiMediaStream);

    return S_FALSE;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_GetInformation(IDirectDrawMediaStream *iface,
        MSPID *pPurposeId, STREAM_TYPE *pType)
{
    IDirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, pPurposeId, pType);

    if (pPurposeId)
        *pPurposeId = This->PurposeId;
    if (pType)
        *pType = This->StreamType;

    return S_OK;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_SetSameFormat(IDirectDrawMediaStream *iface,
        IMediaStream *pStreamThatHasDesiredFormat, DWORD dwFlags)
{
    IDirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x) stub!\n", This, iface, pStreamThatHasDesiredFormat, dwFlags);

    return S_FALSE;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_AllocateSample(IDirectDrawMediaStream *iface,
        DWORD dwFlags, IStreamSample **ppSample)
{
    IDirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    FIXME("(%p/%p)->(%x,%p) stub!\n", This, iface, dwFlags, ppSample);

    return S_FALSE;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_CreateSharedSample(IDirectDrawMediaStream *iface,
        IStreamSample *pExistingSample, DWORD dwFlags, IStreamSample **ppSample)
{
    IDirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p) stub!\n", This, iface, pExistingSample, dwFlags, ppSample);

    return S_FALSE;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_SendEndOfStream(IDirectDrawMediaStream *iface,
        DWORD dwFlags)
{
    IDirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    FIXME("(%p/%p)->(%x) stub!\n", This, iface, dwFlags);

    return S_FALSE;
}

/*** IDirectDrawMediaStream methods ***/
static HRESULT WINAPI IDirectDrawMediaStreamImpl_GetFormat(IDirectDrawMediaStream *iface,
        DDSURFACEDESC *current_format, IDirectDrawPalette **palette,
        DDSURFACEDESC *desired_format, DWORD *flags)
{
    FIXME("(%p)->(%p,%p,%p,%p) stub!\n", iface, current_format, palette, desired_format,
            flags);

    return MS_E_NOSTREAM;

}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_SetFormat(IDirectDrawMediaStream *iface,
        const DDSURFACEDESC *pDDSurfaceDesc, IDirectDrawPalette *pDirectDrawPalette)
{
    FIXME("(%p)->(%p,%p) stub!\n", iface, pDDSurfaceDesc, pDirectDrawPalette);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_GetDirectDraw(IDirectDrawMediaStream *iface,
        IDirectDraw **ppDirectDraw)
{
    FIXME("(%p)->(%p) stub!\n", iface, ppDirectDraw);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_SetDirectDraw(IDirectDrawMediaStream *iface,
        IDirectDraw *pDirectDraw)
{
    FIXME("(%p)->(%p) stub!\n", iface, pDirectDraw);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_CreateSample(IDirectDrawMediaStream *iface,
        IDirectDrawSurface *pSurface, const RECT *pRect, DWORD dwFlags,
        IDirectDrawStreamSample **ppSample)
{
    TRACE("(%p)->(%p,%p,%x,%p)\n", iface, pSurface, pRect, dwFlags, ppSample);

    return ddrawstreamsample_create(iface, ppSample);
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_GetTimePerFrame(IDirectDrawMediaStream *iface,
        STREAM_TIME *pFrameTime)
{
    FIXME("(%p)->(%p) stub!\n", iface, pFrameTime);

    return E_NOTIMPL;
}

static const struct IDirectDrawMediaStreamVtbl DirectDrawMediaStream_Vtbl =
{
    /*** IUnknown methods ***/
    IDirectDrawMediaStreamImpl_QueryInterface,
    IDirectDrawMediaStreamImpl_AddRef,
    IDirectDrawMediaStreamImpl_Release,
    /*** IMediaStream methods ***/
    IDirectDrawMediaStreamImpl_GetMultiMediaStream,
    IDirectDrawMediaStreamImpl_GetInformation,
    IDirectDrawMediaStreamImpl_SetSameFormat,
    IDirectDrawMediaStreamImpl_AllocateSample,
    IDirectDrawMediaStreamImpl_CreateSharedSample,
    IDirectDrawMediaStreamImpl_SendEndOfStream,
    /*** IDirectDrawMediaStream methods ***/
    IDirectDrawMediaStreamImpl_GetFormat,
    IDirectDrawMediaStreamImpl_SetFormat,
    IDirectDrawMediaStreamImpl_GetDirectDraw,
    IDirectDrawMediaStreamImpl_SetDirectDraw,
    IDirectDrawMediaStreamImpl_CreateSample,
    IDirectDrawMediaStreamImpl_GetTimePerFrame
};

HRESULT ddrawmediastream_create(IMultiMediaStream *Parent, const MSPID *pPurposeId,
        STREAM_TYPE StreamType, IMediaStream **ppMediaStream)
{
    IDirectDrawMediaStreamImpl *object;

    TRACE("(%p,%s,%p)\n", Parent, debugstr_guid(pPurposeId), ppMediaStream);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawMediaStreamImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->IDirectDrawMediaStream_iface.lpVtbl = &DirectDrawMediaStream_Vtbl;
    object->ref = 1;

    object->Parent = Parent;
    object->PurposeId = *pPurposeId;
    object->StreamType = StreamType;

    *ppMediaStream = (IMediaStream*)&object->IDirectDrawMediaStream_iface;

    return S_OK;
}

typedef struct {
    IAudioMediaStream IAudioMediaStream_iface;
    LONG ref;
    IMultiMediaStream* parent;
    MSPID purpose_id;
    STREAM_TYPE stream_type;
} IAudioMediaStreamImpl;

static inline IAudioMediaStreamImpl *impl_from_IAudioMediaStream(IAudioMediaStream *iface)
{
    return CONTAINING_RECORD(iface, IAudioMediaStreamImpl, IAudioMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IAudioMediaStreamImpl_QueryInterface(IAudioMediaStream *iface,
        REFIID riid, void **ret_iface)
{
    IAudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMediaStream) ||
        IsEqualGUID(riid, &IID_IAudioMediaStream))
    {
        IAudioMediaStream_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    *ret_iface = NULL;

    ERR("(%p/%p)->(%s,%p),not found\n", iface, This, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAudioMediaStreamImpl_AddRef(IAudioMediaStream *iface)
{
    IAudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p): new ref = %u\n", iface, This, ref);

    return ref;
}

static ULONG WINAPI IAudioMediaStreamImpl_Release(IAudioMediaStream *iface)
{
    IAudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p): new ref = %u\n", iface, This, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IMediaStream methods ***/
static HRESULT WINAPI IAudioMediaStreamImpl_GetMultiMediaStream(IAudioMediaStream *iface,
        IMultiMediaStream** multimedia_stream)
{
    IAudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", iface, This, multimedia_stream);

    return S_FALSE;
}

static HRESULT WINAPI IAudioMediaStreamImpl_GetInformation(IAudioMediaStream *iface,
        MSPID *purpose_id, STREAM_TYPE *type)
{
    IAudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    TRACE("(%p/%p)->(%p,%p)\n", iface, This, purpose_id, type);

    if (purpose_id)
        *purpose_id = This->purpose_id;
    if (type)
        *type = This->stream_type;

    return S_OK;
}

static HRESULT WINAPI IAudioMediaStreamImpl_SetSameFormat(IAudioMediaStream *iface,
        IMediaStream *stream_format, DWORD flags)
{
    IAudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x) stub!\n", iface, This, stream_format, flags);

    return S_FALSE;
}

static HRESULT WINAPI IAudioMediaStreamImpl_AllocateSample(IAudioMediaStream *iface,
        DWORD flags, IStreamSample **sample)
{
    IAudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%x,%p) stub!\n", iface, This, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI IAudioMediaStreamImpl_CreateSharedSample(IAudioMediaStream *iface,
        IStreamSample *existing_sample, DWORD flags, IStreamSample **sample)
{
    IAudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p) stub!\n", iface, This, existing_sample, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI IAudioMediaStreamImpl_SendEndOfStream(IAudioMediaStream *iface,
        DWORD flags)
{
    IAudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%x) stub!\n", iface, This, flags);

    return S_FALSE;
}

/*** IAudioMediaStream methods ***/
static HRESULT WINAPI IAudioMediaStreamImpl_GetFormat(IAudioMediaStream *iface, WAVEFORMATEX *wave_format_current)
{
    IAudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", iface, This, wave_format_current);

    if (!wave_format_current)
        return E_POINTER;

    return MS_E_NOSTREAM;

}

static HRESULT WINAPI IAudioMediaStreamImpl_SetFormat(IAudioMediaStream *iface, const WAVEFORMATEX *wave_format)
{
    IAudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", iface, This, wave_format);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioMediaStreamImpl_CreateSample(IAudioMediaStream *iface, IAudioData *audio_data,
                                                         DWORD flags, IAudioStreamSample **sample)
{
    IAudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    TRACE("(%p/%p)->(%p,%u,%p)\n", iface, This, audio_data, flags, sample);

    if (!audio_data)
        return E_POINTER;

    return audiostreamsample_create(iface, audio_data, sample);
}

static const struct IAudioMediaStreamVtbl AudioMediaStream_Vtbl =
{
    /*** IUnknown methods ***/
    IAudioMediaStreamImpl_QueryInterface,
    IAudioMediaStreamImpl_AddRef,
    IAudioMediaStreamImpl_Release,
    /*** IMediaStream methods ***/
    IAudioMediaStreamImpl_GetMultiMediaStream,
    IAudioMediaStreamImpl_GetInformation,
    IAudioMediaStreamImpl_SetSameFormat,
    IAudioMediaStreamImpl_AllocateSample,
    IAudioMediaStreamImpl_CreateSharedSample,
    IAudioMediaStreamImpl_SendEndOfStream,
    /*** IAudioMediaStream methods ***/
    IAudioMediaStreamImpl_GetFormat,
    IAudioMediaStreamImpl_SetFormat,
    IAudioMediaStreamImpl_CreateSample
};

HRESULT audiomediastream_create(IMultiMediaStream *parent, const MSPID *purpose_id,
        STREAM_TYPE stream_type, IMediaStream **media_stream)
{
    IAudioMediaStreamImpl *object;

    TRACE("(%p,%s,%p)\n", parent, debugstr_guid(purpose_id), media_stream);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IAudioMediaStreamImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->IAudioMediaStream_iface.lpVtbl = &AudioMediaStream_Vtbl;
    object->ref = 1;

    object->parent = parent;
    object->purpose_id = *purpose_id;
    object->stream_type = stream_type;

    *media_stream = (IMediaStream*)&object->IAudioMediaStream_iface;

    return S_OK;
}

typedef struct {
    IDirectDrawStreamSample IDirectDrawStreamSample_iface;
    LONG ref;
    IMediaStream *parent;
} IDirectDrawStreamSampleImpl;

static inline IDirectDrawStreamSampleImpl *impl_from_IDirectDrawStreamSample(IDirectDrawStreamSample *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawStreamSampleImpl, IDirectDrawStreamSample_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectDrawStreamSampleImpl_QueryInterface(IDirectDrawStreamSample *iface,
        REFIID riid, void **ret_iface)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IStreamSample) ||
        IsEqualGUID(riid, &IID_IDirectDrawStreamSample))
    {
        IDirectDrawStreamSample_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    *ret_iface = NULL;

    ERR("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectDrawStreamSampleImpl_AddRef(IDirectDrawStreamSample *iface)
{
    IDirectDrawStreamSampleImpl *This = impl_from_IDirectDrawStreamSample(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirectDrawStreamSampleImpl_Release(IDirectDrawStreamSample *iface)
{
    IDirectDrawStreamSampleImpl *This = impl_from_IDirectDrawStreamSample(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IStreamSample methods ***/
static HRESULT WINAPI IDirectDrawStreamSampleImpl_GetMediaStream(IDirectDrawStreamSample *iface, IMediaStream **media_stream)
{
    FIXME("(%p)->(%p): stub\n", iface, media_stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawStreamSampleImpl_GetSampleTimes(IDirectDrawStreamSample *iface, STREAM_TIME *start_time,
                                                                 STREAM_TIME *end_time, STREAM_TIME *current_time)
{
    FIXME("(%p)->(%p,%p,%p): stub\n", iface, start_time, end_time, current_time);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawStreamSampleImpl_SetSampleTimes(IDirectDrawStreamSample *iface, const STREAM_TIME *start_time,
                                                                 const STREAM_TIME *end_time)
{
    FIXME("(%p)->(%p,%p): stub\n", iface, start_time, end_time);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawStreamSampleImpl_Update(IDirectDrawStreamSample *iface, DWORD flags, HANDLE event,
                                                         PAPCFUNC func_APC, DWORD APC_data)
{
    FIXME("(%p)->(%x,%p,%p,%u): stub\n", iface, flags, event, func_APC, APC_data);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawStreamSampleImpl_CompletionStatus(IDirectDrawStreamSample *iface, DWORD flags, DWORD milliseconds)
{
    FIXME("(%p)->(%x,%u): stub\n", iface, flags, milliseconds);

    return E_NOTIMPL;
}

/*** IDirectDrawStreamSample methods ***/
static HRESULT WINAPI IDirectDrawStreamSampleImpl_GetSurface(IDirectDrawStreamSample *iface, IDirectDrawSurface **ddraw_surface,
                                                             RECT *rect)
{
    FIXME("(%p)->(%p,%p): stub\n", iface, ddraw_surface, rect);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawStreamSampleImpl_SetRect(IDirectDrawStreamSample *iface, const RECT *rect)
{
    FIXME("(%p)->(%p): stub\n", iface, rect);

    return E_NOTIMPL;
}

static const struct IDirectDrawStreamSampleVtbl DirectDrawStreamSample_Vtbl =
{
    /*** IUnknown methods ***/
    IDirectDrawStreamSampleImpl_QueryInterface,
    IDirectDrawStreamSampleImpl_AddRef,
    IDirectDrawStreamSampleImpl_Release,
    /*** IStreamSample methods ***/
    IDirectDrawStreamSampleImpl_GetMediaStream,
    IDirectDrawStreamSampleImpl_GetSampleTimes,
    IDirectDrawStreamSampleImpl_SetSampleTimes,
    IDirectDrawStreamSampleImpl_Update,
    IDirectDrawStreamSampleImpl_CompletionStatus,
    /*** IDirectDrawStreamSample methods ***/
    IDirectDrawStreamSampleImpl_GetSurface,
    IDirectDrawStreamSampleImpl_SetRect
};

static HRESULT ddrawstreamsample_create(IDirectDrawMediaStream *parent, IDirectDrawStreamSample **ddraw_stream_sample)
{
    IDirectDrawStreamSampleImpl *object;

    TRACE("(%p)\n", ddraw_stream_sample);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawStreamSampleImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->IDirectDrawStreamSample_iface.lpVtbl = &DirectDrawStreamSample_Vtbl;
    object->ref = 1;
    object->parent = (IMediaStream*)parent;

    *ddraw_stream_sample = (IDirectDrawStreamSample*)&object->IDirectDrawStreamSample_iface;

    return S_OK;
}

typedef struct {
    IAudioStreamSample IAudioStreamSample_iface;
    LONG ref;
    IMediaStream *parent;
    IAudioData *audio_data;
} IAudioStreamSampleImpl;

static inline IAudioStreamSampleImpl *impl_from_IAudioStreamSample(IAudioStreamSample *iface)
{
    return CONTAINING_RECORD(iface, IAudioStreamSampleImpl, IAudioStreamSample_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IAudioStreamSampleImpl_QueryInterface(IAudioStreamSample *iface,
        REFIID riid, void **ret_iface)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IStreamSample) ||
        IsEqualGUID(riid, &IID_IAudioStreamSample))
    {
        IAudioStreamSample_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    *ret_iface = NULL;

    ERR("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAudioStreamSampleImpl_AddRef(IAudioStreamSample *iface)
{
    IAudioStreamSampleImpl *This = impl_from_IAudioStreamSample(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IAudioStreamSampleImpl_Release(IAudioStreamSample *iface)
{
    IAudioStreamSampleImpl *This = impl_from_IAudioStreamSample(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IStreamSample methods ***/
static HRESULT WINAPI IAudioStreamSampleImpl_GetMediaStream(IAudioStreamSample *iface, IMediaStream **media_stream)
{
    FIXME("(%p)->(%p): stub\n", iface, media_stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioStreamSampleImpl_GetSampleTimes(IAudioStreamSample *iface, STREAM_TIME *start_time,
                                                                 STREAM_TIME *end_time, STREAM_TIME *current_time)
{
    FIXME("(%p)->(%p,%p,%p): stub\n", iface, start_time, end_time, current_time);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioStreamSampleImpl_SetSampleTimes(IAudioStreamSample *iface, const STREAM_TIME *start_time,
                                                                 const STREAM_TIME *end_time)
{
    FIXME("(%p)->(%p,%p): stub\n", iface, start_time, end_time);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioStreamSampleImpl_Update(IAudioStreamSample *iface, DWORD flags, HANDLE event,
                                                         PAPCFUNC func_APC, DWORD APC_data)
{
    FIXME("(%p)->(%x,%p,%p,%u): stub\n", iface, flags, event, func_APC, APC_data);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioStreamSampleImpl_CompletionStatus(IAudioStreamSample *iface, DWORD flags, DWORD milliseconds)
{
    FIXME("(%p)->(%x,%u): stub\n", iface, flags, milliseconds);

    return E_NOTIMPL;
}

/*** IAudioStreamSample methods ***/
static HRESULT WINAPI IAudioStreamSampleImpl_GetAudioData(IAudioStreamSample *iface, IAudioData **audio_data)
{
    FIXME("(%p)->(%p): stub\n", iface, audio_data);

    return E_NOTIMPL;
}

static const struct IAudioStreamSampleVtbl AudioStreamSample_Vtbl =
{
    /*** IUnknown methods ***/
    IAudioStreamSampleImpl_QueryInterface,
    IAudioStreamSampleImpl_AddRef,
    IAudioStreamSampleImpl_Release,
    /*** IStreamSample methods ***/
    IAudioStreamSampleImpl_GetMediaStream,
    IAudioStreamSampleImpl_GetSampleTimes,
    IAudioStreamSampleImpl_SetSampleTimes,
    IAudioStreamSampleImpl_Update,
    IAudioStreamSampleImpl_CompletionStatus,
    /*** IAudioStreamSample methods ***/
    IAudioStreamSampleImpl_GetAudioData
};

static HRESULT audiostreamsample_create(IAudioMediaStream *parent, IAudioData *audio_data, IAudioStreamSample **audio_stream_sample)
{
    IAudioStreamSampleImpl *object;

    TRACE("(%p)\n", audio_stream_sample);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IAudioStreamSampleImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->IAudioStreamSample_iface.lpVtbl = &AudioStreamSample_Vtbl;
    object->ref = 1;
    object->parent = (IMediaStream*)parent;
    object->audio_data = audio_data;

    *audio_stream_sample = (IAudioStreamSample*)&object->IAudioStreamSample_iface;

    return S_OK;
}
