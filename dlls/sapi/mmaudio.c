/*
 * Speech API (SAPI) winmm audio implementation.
 *
 * Copyright 2023 Shaun Ren for CodeWeavers
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
#include "objbase.h"

#include "sapiddk.h"
#include "sperror.h"

#include "initguid.h"

#include "wine/debug.h"

#include "sapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sapi);

DEFINE_GUID(SPDFID_Text, 0x7ceef9f9, 0x3d13, 0x11d2, 0x9e, 0xe7, 0x00, 0xc0, 0x4f, 0x79, 0x73, 0x96);
DEFINE_GUID(SPDFID_WaveFormatEx, 0xc31adbae, 0x527f, 0x4ff5, 0xa2, 0x30, 0xf6, 0x2b, 0xb6, 0x1f, 0xf7, 0x0c);

enum flow_type { FLOW_IN, FLOW_OUT };

struct mmaudio
{
    ISpEventSource ISpEventSource_iface;
    ISpEventSink ISpEventSink_iface;
    ISpObjectWithToken ISpObjectWithToken_iface;
    ISpMMSysAudio ISpMMSysAudio_iface;
    LONG ref;

    enum flow_type flow;
    ISpObjectToken *token;
    UINT device_id;
    WAVEFORMATEX *wfx;
    CRITICAL_SECTION cs;
};

static inline struct mmaudio *impl_from_ISpEventSource(ISpEventSource *iface)
{
    return CONTAINING_RECORD(iface, struct mmaudio, ISpEventSource_iface);
}

static inline struct mmaudio *impl_from_ISpEventSink(ISpEventSink *iface)
{
    return CONTAINING_RECORD(iface, struct mmaudio, ISpEventSink_iface);
}

static inline struct mmaudio *impl_from_ISpObjectWithToken(ISpObjectWithToken *iface)
{
    return CONTAINING_RECORD(iface, struct mmaudio, ISpObjectWithToken_iface);
}

static inline struct mmaudio *impl_from_ISpMMSysAudio(ISpMMSysAudio *iface)
{
    return CONTAINING_RECORD(iface, struct mmaudio, ISpMMSysAudio_iface);
}

static HRESULT WINAPI event_source_QueryInterface(ISpEventSource *iface, REFIID iid, void **obj)
{
    struct mmaudio *This = impl_from_ISpEventSource(iface);

    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(iid), obj);

    return ISpMMSysAudio_QueryInterface(&This->ISpMMSysAudio_iface, iid, obj);
}

static ULONG WINAPI event_source_AddRef(ISpEventSource *iface)
{
    struct mmaudio *This = impl_from_ISpEventSource(iface);

    TRACE("(%p).\n", iface);

    return ISpMMSysAudio_AddRef(&This->ISpMMSysAudio_iface);
}

static ULONG WINAPI event_source_Release(ISpEventSource *iface)
{
    struct mmaudio *This = impl_from_ISpEventSource(iface);

    TRACE("(%p).\n", iface);

    return ISpMMSysAudio_Release(&This->ISpMMSysAudio_iface);
}

static HRESULT WINAPI event_source_SetNotifySink(ISpEventSource *iface, ISpNotifySink *sink)
{
    FIXME("(%p, %p): stub.\n", iface, sink);

    return E_NOTIMPL;
}

static HRESULT WINAPI event_source_SetNotifyWindowMessage(ISpEventSource *iface, HWND hwnd,
                                                          UINT msg, WPARAM wparam, LPARAM lparam)
{
    FIXME("(%p, %p, %u, %Ix, %Ix): stub.\n", iface, hwnd, msg, wparam, lparam);

    return E_NOTIMPL;
}

static HRESULT WINAPI event_source_SetNotifyCallbackFunction(ISpEventSource *iface, SPNOTIFYCALLBACK *callback,
                                                             WPARAM wparam, LPARAM lparam)
{
    FIXME("(%p, %p, %Ix, %Ix): stub.\n", iface, callback, wparam, lparam);

    return E_NOTIMPL;
}

static HRESULT WINAPI event_source_SetNotifyCallbackInterface(ISpEventSource *iface, ISpNotifyCallback *callback,
                                                              WPARAM wparam, LPARAM lparam)
{
    FIXME("(%p, %p, %Ix, %Ix): stub.\n", iface, callback, wparam, lparam);

    return E_NOTIMPL;
}

static HRESULT WINAPI event_source_SetNotifyWin32Event(ISpEventSource *iface)
{
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI event_source_WaitForNotifyEvent(ISpEventSource *iface, DWORD milliseconds)
{
    FIXME("(%p, %ld): stub.\n", iface, milliseconds);

    return E_NOTIMPL;
}

static HANDLE WINAPI event_source_GetNotifyEventHandle(ISpEventSource *iface)
{
    FIXME("(%p): stub.\n", iface);

    return NULL;
}

static HRESULT WINAPI event_source_SetInterest(ISpEventSource *iface, ULONGLONG event, ULONGLONG queued)
{
    FIXME("(%p, %s, %s): stub.\n", iface, wine_dbgstr_longlong(event), wine_dbgstr_longlong(queued));

    return E_NOTIMPL;
}

static HRESULT WINAPI event_source_GetEvents(ISpEventSource *iface, ULONG count, SPEVENT *array, ULONG *fetched)
{
    FIXME("(%p, %lu, %p, %p): stub.\n", iface, count, array, fetched);

    return E_NOTIMPL;
}

static HRESULT WINAPI event_source_GetInfo(ISpEventSource *iface, SPEVENTSOURCEINFO *info)
{
    FIXME("(%p, %p): stub.\n", iface, info);

    return E_NOTIMPL;
}

static const ISpEventSourceVtbl event_source_vtbl =
{
    event_source_QueryInterface,
    event_source_AddRef,
    event_source_Release,
    event_source_SetNotifySink,
    event_source_SetNotifyWindowMessage,
    event_source_SetNotifyCallbackFunction,
    event_source_SetNotifyCallbackInterface,
    event_source_SetNotifyWin32Event,
    event_source_WaitForNotifyEvent,
    event_source_GetNotifyEventHandle,
    event_source_SetInterest,
    event_source_GetEvents,
    event_source_GetInfo
};

static HRESULT WINAPI event_sink_QueryInterface(ISpEventSink *iface, REFIID iid, void **obj)
{
    struct mmaudio *This = impl_from_ISpEventSink(iface);

    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(iid), obj);

    return ISpMMSysAudio_QueryInterface(&This->ISpMMSysAudio_iface, iid, obj);
}

static ULONG WINAPI event_sink_AddRef(ISpEventSink *iface)
{
    struct mmaudio *This = impl_from_ISpEventSink(iface);

    TRACE("(%p).\n", iface);

    return ISpMMSysAudio_AddRef(&This->ISpMMSysAudio_iface);
}

static ULONG WINAPI event_sink_Release(ISpEventSink *iface)
{
    struct mmaudio *This = impl_from_ISpEventSink(iface);

    TRACE("(%p).\n", iface);

    return ISpMMSysAudio_Release(&This->ISpMMSysAudio_iface);
}

static HRESULT WINAPI event_sink_AddEvents(ISpEventSink *iface, const SPEVENT *events, ULONG count)
{
    FIXME("(%p, %p, %lu).\n", iface, events, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI event_sink_GetEventInterest(ISpEventSink *iface, ULONGLONG *interest)
{
    FIXME("(%p, %p).\n", iface, interest);

    return E_NOTIMPL;
}

static const ISpEventSinkVtbl event_sink_vtbl =
{
    event_sink_QueryInterface,
    event_sink_AddRef,
    event_sink_Release,
    event_sink_AddEvents,
    event_sink_GetEventInterest
};

static HRESULT WINAPI objwithtoken_QueryInterface(ISpObjectWithToken *iface, REFIID iid, void **obj)
{
    struct mmaudio *This = impl_from_ISpObjectWithToken(iface);

    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(iid), obj);

    return ISpMMSysAudio_QueryInterface(&This->ISpMMSysAudio_iface, iid, obj);
}

static ULONG WINAPI objwithtoken_AddRef(ISpObjectWithToken *iface)
{
    struct mmaudio *This = impl_from_ISpObjectWithToken(iface);

    TRACE("(%p).\n", iface);

    return ISpMMSysAudio_AddRef(&This->ISpMMSysAudio_iface);
}

static ULONG WINAPI objwithtoken_Release(ISpObjectWithToken *iface)
{
    struct mmaudio *This = impl_from_ISpObjectWithToken(iface);

    TRACE("(%p).\n", iface);

    return ISpMMSysAudio_Release(&This->ISpMMSysAudio_iface);
}

static HRESULT WINAPI objwithtoken_SetObjectToken(ISpObjectWithToken *iface, ISpObjectToken *token)
{
    struct mmaudio *This = impl_from_ISpObjectWithToken(iface);

    FIXME("(%p, %p): semi-stub.\n", iface, token);

    if (!token)
        return E_INVALIDARG;
    if (This->token)
        return SPERR_ALREADY_INITIALIZED;

    This->token = token;
    return S_OK;
}

static HRESULT WINAPI objwithtoken_GetObjectToken(ISpObjectWithToken *iface, ISpObjectToken **token)
{
    struct mmaudio *This = impl_from_ISpObjectWithToken(iface);

    TRACE("(%p, %p).\n", iface, token);

    if (!token)
        return E_POINTER;

    *token = This->token;
    if (*token)
    {
        ISpObjectToken_AddRef(*token);
        return S_OK;
    }
    else
        return S_FALSE;
}

static const ISpObjectWithTokenVtbl objwithtoken_vtbl =
{
    objwithtoken_QueryInterface,
    objwithtoken_AddRef,
    objwithtoken_Release,
    objwithtoken_SetObjectToken,
    objwithtoken_GetObjectToken
};

static HRESULT WINAPI mmsysaudio_QueryInterface(ISpMMSysAudio *iface, REFIID iid, void **obj)
{
    struct mmaudio *This = impl_from_ISpMMSysAudio(iface);

    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(iid), obj);

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_ISequentialStream) ||
        IsEqualIID(iid, &IID_IStream) ||
        IsEqualIID(iid, &IID_ISpStreamFormat) ||
        IsEqualIID(iid, &IID_ISpAudio) ||
        IsEqualIID(iid, &IID_ISpMMSysAudio))
        *obj = &This->ISpMMSysAudio_iface;
    else if (IsEqualIID(iid, &IID_ISpEventSource))
        *obj = &This->ISpEventSource_iface;
    else if (IsEqualIID(iid, &IID_ISpEventSink))
        *obj = &This->ISpEventSink_iface;
    else if (IsEqualIID(iid, &IID_ISpObjectWithToken))
        *obj = &This->ISpObjectWithToken_iface;
    else
    {
        *obj = NULL;
        FIXME("interface %s not implemented.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI mmsysaudio_AddRef(ISpMMSysAudio *iface)
{
    struct mmaudio *This = impl_from_ISpMMSysAudio(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): ref=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI mmsysaudio_Release(ISpMMSysAudio *iface)
{
    struct mmaudio *This = impl_from_ISpMMSysAudio(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): ref=%lu\n", iface, ref);

    if (!ref)
    {
        if (This->token) ISpObjectToken_Release(This->token);
        heap_free(This->wfx);
        DeleteCriticalSection(&This->cs);

        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI mmsysaudio_Read(ISpMMSysAudio *iface, void *pv, ULONG cb, ULONG *cb_read)
{
    FIXME("(%p, %p, %lu, %p): stub.\n", iface, pv, cb, cb_read);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_Write(ISpMMSysAudio *iface, const void *pv, ULONG cb, ULONG *cb_written)
{
    FIXME("(%p, %p, %lu, %p): stub.\n", iface, pv, cb, cb_written);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_Seek(ISpMMSysAudio *iface, LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER *new_pos)
{
    FIXME("(%p, %s, %lu, %p): stub.\n", iface, wine_dbgstr_longlong(move.QuadPart), origin, new_pos);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_SetSize(ISpMMSysAudio *iface, ULARGE_INTEGER new_size)
{
    FIXME("(%p, %s): stub.\n", iface, wine_dbgstr_longlong(new_size.QuadPart));

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_CopyTo(ISpMMSysAudio *iface, IStream *stream, ULARGE_INTEGER cb,
                                        ULARGE_INTEGER *cb_read, ULARGE_INTEGER *cb_written)
{
    FIXME("(%p, %p, %s, %p, %p): stub.\n", iface, stream, wine_dbgstr_longlong(cb.QuadPart), cb_read, cb_written);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_Commit(ISpMMSysAudio *iface, DWORD flags)
{
    FIXME("(%p, %#lx): stub.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_Revert(ISpMMSysAudio *iface)
{
    FIXME("(%p).\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_LockRegion(ISpMMSysAudio *iface, ULARGE_INTEGER offset, ULARGE_INTEGER cb,
                                            DWORD lock_type)
{
    FIXME("(%p, %s, %s, %#lx): stub.\n", iface, wine_dbgstr_longlong(offset.QuadPart),
          wine_dbgstr_longlong(cb.QuadPart), lock_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_UnlockRegion(ISpMMSysAudio *iface, ULARGE_INTEGER offset, ULARGE_INTEGER cb,
                                              DWORD lock_type)
{
    FIXME("(%p, %s, %s, %#lx): stub.\n", iface, wine_dbgstr_longlong(offset.QuadPart),
          wine_dbgstr_longlong(cb.QuadPart), lock_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_Stat(ISpMMSysAudio *iface, STATSTG *statstg, DWORD flags)
{
    FIXME("(%p, %p, %#lx): stub.\n", iface, statstg, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_Clone(ISpMMSysAudio *iface, IStream **stream)
{
    FIXME("(%p, %p): stub.\n", iface, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_GetFormat(ISpMMSysAudio *iface, GUID *format, WAVEFORMATEX **wfx)
{
    struct mmaudio *This = impl_from_ISpMMSysAudio(iface);

    TRACE("(%p, %p, %p).\n", iface, format, wfx);

    if (!format || !wfx)
        return E_POINTER;

    EnterCriticalSection(&This->cs);

    if (!(*wfx = CoTaskMemAlloc(sizeof(WAVEFORMATEX) + This->wfx->cbSize)))
    {
        LeaveCriticalSection(&This->cs);
        return E_OUTOFMEMORY;
    }
    *format = SPDFID_WaveFormatEx;
    memcpy(*wfx, This->wfx, sizeof(WAVEFORMATEX) + This->wfx->cbSize);

    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI mmsysaudio_SetState(ISpMMSysAudio *iface, SPAUDIOSTATE state, ULONGLONG reserved)
{
    FIXME("(%p, %u, %s): stub.\n", iface, state, wine_dbgstr_longlong(reserved));

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_SetFormat(ISpMMSysAudio *iface, const GUID *guid, const WAVEFORMATEX *wfx)
{
    struct mmaudio *This = impl_from_ISpMMSysAudio(iface);
    MMRESULT res;
    WAVEFORMATEX *new_wfx;

    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(guid), wfx);

    if (!guid || !wfx || !IsEqualGUID(guid, &SPDFID_WaveFormatEx))
        return E_INVALIDARG;

    EnterCriticalSection(&This->cs);

    /* Determine whether the device supports the requested format. */
    res = waveOutOpen(NULL, This->device_id, wfx, 0, 0, WAVE_FORMAT_QUERY);
    if (res != MMSYSERR_NOERROR)
    {
        LeaveCriticalSection(&This->cs);
        return res == WAVERR_BADFORMAT ? SPERR_UNSUPPORTED_FORMAT : SPERR_GENERIC_MMSYS_ERROR;
    }

    if (!(new_wfx = heap_alloc(sizeof(*wfx) + wfx->cbSize)))
    {
        LeaveCriticalSection(&This->cs);
        return E_OUTOFMEMORY;
    }
    memcpy(new_wfx, wfx, sizeof(*wfx) + wfx->cbSize);
    heap_free(This->wfx);
    This->wfx = new_wfx;

    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI mmsysaudio_GetStatus(ISpMMSysAudio *iface, SPAUDIOSTATUS *status)
{
    FIXME("(%p, %p): stub.\n", iface, status);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_SetBufferInfo(ISpMMSysAudio *iface, const SPAUDIOBUFFERINFO *info)
{
    FIXME("(%p, %p): stub.\n", iface, info);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_GetBufferInfo(ISpMMSysAudio *iface, SPAUDIOBUFFERINFO *info)
{
    FIXME("(%p, %p): stub.\n", iface, info);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_GetDefaultFormat(ISpMMSysAudio *iface, GUID *guid, WAVEFORMATEX **wfx)
{
    FIXME("(%p, %p, %p): stub.\n", iface, guid, wfx);

    return E_NOTIMPL;
}

static HANDLE WINAPI mmsysaudio_EventHandle(ISpMMSysAudio *iface)
{
    FIXME("(%p): stub.\n", iface);

    return NULL;
}

static HRESULT WINAPI mmsysaudio_GetVolumeLevel(ISpMMSysAudio *iface, ULONG *level)
{
    FIXME("(%p, %p): stub.\n", iface, level);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_SetVolumeLevel(ISpMMSysAudio *iface, ULONG level)
{
    FIXME("(%p, %lu): stub.\n", iface, level);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_GetBufferNotifySize(ISpMMSysAudio *iface, ULONG *size)
{
    FIXME("(%p, %p): stub.\n", iface, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_SetBufferNotifySize(ISpMMSysAudio *iface, ULONG size)
{
    FIXME("(%p, %lu): stub.\n", iface, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_GetDeviceId(ISpMMSysAudio *iface, UINT *id)
{
    struct mmaudio *This = impl_from_ISpMMSysAudio(iface);

    TRACE("(%p, %p).\n", iface, id);

    if (!id) return E_POINTER;

    EnterCriticalSection(&This->cs);
    *id = This->device_id;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI mmsysaudio_SetDeviceId(ISpMMSysAudio *iface, UINT id)
{
    struct mmaudio *This = impl_from_ISpMMSysAudio(iface);

    TRACE("(%p, %u).\n", iface, id);

    if (id != WAVE_MAPPER && id >= waveOutGetNumDevs())
        return E_INVALIDARG;

    EnterCriticalSection(&This->cs);
    This->device_id = id;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI mmsysaudio_GetMMHandle(ISpMMSysAudio *iface, void **handle)
{
    FIXME("(%p, %p): stub.\n", iface, handle);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_GetLineId(ISpMMSysAudio *iface, UINT *id)
{
    FIXME("(%p, %p): stub.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI mmsysaudio_SetLineId(ISpMMSysAudio *iface, UINT id)
{
    FIXME("(%p, %u): stub.\n", iface, id);

    return E_NOTIMPL;
}

static const ISpMMSysAudioVtbl mmsysaudio_vtbl =
{
    mmsysaudio_QueryInterface,
    mmsysaudio_AddRef,
    mmsysaudio_Release,
    mmsysaudio_Read,
    mmsysaudio_Write,
    mmsysaudio_Seek,
    mmsysaudio_SetSize,
    mmsysaudio_CopyTo,
    mmsysaudio_Commit,
    mmsysaudio_Revert,
    mmsysaudio_LockRegion,
    mmsysaudio_UnlockRegion,
    mmsysaudio_Stat,
    mmsysaudio_Clone,
    mmsysaudio_GetFormat,
    mmsysaudio_SetState,
    mmsysaudio_SetFormat,
    mmsysaudio_GetStatus,
    mmsysaudio_SetBufferInfo,
    mmsysaudio_GetBufferInfo,
    mmsysaudio_GetDefaultFormat,
    mmsysaudio_EventHandle,
    mmsysaudio_GetVolumeLevel,
    mmsysaudio_SetVolumeLevel,
    mmsysaudio_GetBufferNotifySize,
    mmsysaudio_SetBufferNotifySize,
    mmsysaudio_GetDeviceId,
    mmsysaudio_SetDeviceId,
    mmsysaudio_GetMMHandle,
    mmsysaudio_GetLineId,
    mmsysaudio_SetLineId
};

static HRESULT mmaudio_create(IUnknown *outer, REFIID iid, void **obj, enum flow_type flow)
{
    struct mmaudio *This;
    HRESULT hr;

    if (flow != FLOW_OUT)
    {
        FIXME("flow %d not implemented.\n", flow);
        return E_NOTIMPL;
    }

    if (!(This = heap_alloc_zero(sizeof(*This))))
        return E_OUTOFMEMORY;
    This->ISpEventSource_iface.lpVtbl = &event_source_vtbl;
    This->ISpEventSink_iface.lpVtbl = &event_sink_vtbl;
    This->ISpObjectWithToken_iface.lpVtbl = &objwithtoken_vtbl;
    This->ISpMMSysAudio_iface.lpVtbl = &mmsysaudio_vtbl;
    This->ref = 1;

    This->flow = flow;
    This->token = NULL;
    This->device_id = WAVE_MAPPER;

    if (!(This->wfx = heap_alloc(sizeof(*This->wfx))))
    {
        heap_free(This);
        return E_OUTOFMEMORY;
    }
    This->wfx->wFormatTag = WAVE_FORMAT_PCM;
    This->wfx->nChannels = 1;
    This->wfx->nSamplesPerSec = 22050;
    This->wfx->nAvgBytesPerSec = 22050 * 2;
    This->wfx->nBlockAlign = 2;
    This->wfx->wBitsPerSample = 16;
    This->wfx->cbSize = 0;

    InitializeCriticalSection(&This->cs);

    hr = ISpMMSysAudio_QueryInterface(&This->ISpMMSysAudio_iface, iid, obj);
    ISpMMSysAudio_Release(&This->ISpMMSysAudio_iface);
    return hr;
}

HRESULT mmaudio_out_create(IUnknown *outer, REFIID iid, void **obj)
{
    return mmaudio_create(outer, iid, obj, FLOW_OUT);
}
