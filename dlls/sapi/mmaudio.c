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
    SPAUDIOSTATE state;
    WAVEFORMATEX *wfx;
    union
    {
        HWAVEIN in;
        HWAVEOUT out;
    } hwave;
    HANDLE event;
    struct async_queue queue;
    CRITICAL_SECTION cs;

    size_t pending_buf_count;
    CRITICAL_SECTION pending_cs;
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

    ISpObjectToken_AddRef(token);
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
        ISpMMSysAudio_SetState(iface, SPAS_CLOSED, 0);

        async_wait_queue_empty(&This->queue, INFINITE);
        async_cancel_queue(&This->queue);

        if (This->token) ISpObjectToken_Release(This->token);
        free(This->wfx);
        CloseHandle(This->event);
        DeleteCriticalSection(&This->pending_cs);
        DeleteCriticalSection(&This->cs);

        free(This);
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
    struct mmaudio *This = impl_from_ISpMMSysAudio(iface);
    HRESULT hr = S_OK;
    WAVEHDR *buf;

    TRACE("(%p, %p, %lu, %p).\n", iface, pv, cb, cb_written);

    if (This->flow != FLOW_OUT)
        return STG_E_ACCESSDENIED;

    if (cb_written)
        *cb_written = 0;

    EnterCriticalSection(&This->cs);

    if (This->state == SPAS_CLOSED || This->state == SPAS_STOP)
    {
        LeaveCriticalSection(&This->cs);
        return SP_AUDIO_STOPPED;
    }

    if (!(buf = malloc(sizeof(WAVEHDR) + cb)))
    {
        LeaveCriticalSection(&This->cs);
        return E_OUTOFMEMORY;
    }
    memcpy((char *)(buf + 1), pv, cb);
    buf->lpData = (char *)(buf + 1);
    buf->dwBufferLength = cb;
    buf->dwFlags = 0;

    if (waveOutPrepareHeader(This->hwave.out, buf, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
    {
        LeaveCriticalSection(&This->cs);
        free(buf);
        return E_FAIL;
    }

    waveOutWrite(This->hwave.out, buf, sizeof(WAVEHDR));

    EnterCriticalSection(&This->pending_cs);
    ++This->pending_buf_count;
    LeaveCriticalSection(&This->pending_cs);

    ResetEvent(This->event);

    LeaveCriticalSection(&This->cs);

    if (cb_written)
        *cb_written = cb;

    return hr;
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

struct free_buf_task
{
    struct async_task task;
    struct mmaudio *audio;
    WAVEHDR *buf;
};

static void free_out_buf_proc(struct async_task *task)
{
    struct free_buf_task *fbt = (struct free_buf_task *)task;
    size_t buf_count;

    TRACE("(%p).\n", task);

    waveOutUnprepareHeader(fbt->audio->hwave.out, fbt->buf, sizeof(WAVEHDR));
    free(fbt->buf);

    EnterCriticalSection(&fbt->audio->pending_cs);
    buf_count = --fbt->audio->pending_buf_count;
    LeaveCriticalSection(&fbt->audio->pending_cs);
    if (!buf_count)
        SetEvent(fbt->audio->event);
}

static void CALLBACK wave_out_proc(HWAVEOUT hwo, UINT msg, DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR param2)
{
    struct mmaudio *This = (struct mmaudio *)instance;
    struct free_buf_task *task;

    TRACE("(%p, %#x, %08Ix, %08Ix, %08Ix).\n", hwo, msg, instance, param1, param2);

    switch (msg)
    {
    case WOM_DONE:
        if (!(task = malloc(sizeof(*task))))
        {
            ERR("failed to allocate free_buf_task.\n");
            break;
        }
        task->task.proc = free_out_buf_proc;
        task->audio = This;
        task->buf = (WAVEHDR *)param1;
        async_queue_task(&This->queue, (struct async_task *)task);
        break;

    default:
        break;
    }
}

static HRESULT WINAPI mmsysaudio_SetState(ISpMMSysAudio *iface, SPAUDIOSTATE state, ULONGLONG reserved)
{
    struct mmaudio *This = impl_from_ISpMMSysAudio(iface);
    HRESULT hr = S_OK;

    TRACE("(%p, %u, %s).\n", iface, state, wine_dbgstr_longlong(reserved));

    if (state != SPAS_CLOSED && state != SPAS_RUN)
    {
        FIXME("state %#x not implemented.\n", state);
        return E_NOTIMPL;
    }

    EnterCriticalSection(&This->cs);

    if (This->state == state)
        goto done;

    if (This->state == SPAS_CLOSED)
    {
        if (FAILED(hr = async_start_queue(&This->queue)))
        {
            ERR("Failed to start async queue: %#lx.\n", hr);
            goto done;
        }

        if (waveOutOpen(&This->hwave.out, This->device_id, This->wfx, (DWORD_PTR)wave_out_proc,
                        (DWORD_PTR)This, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
        {
            hr = SPERR_GENERIC_MMSYS_ERROR;
            goto done;
        }
    }

    if (state == SPAS_CLOSED && This->state != SPAS_CLOSED)
    {
        waveOutReset(This->hwave.out);
        /* Wait until all buffers are freed. */
        WaitForSingleObject(This->event, INFINITE);

        if (waveOutClose(This->hwave.out) != MMSYSERR_NOERROR)
        {
            hr = SPERR_GENERIC_MMSYS_ERROR;
            goto done;
        }
    }

    This->state = state;

done:
    LeaveCriticalSection(&This->cs);
    return hr;
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

    if (!memcmp(wfx, This->wfx, sizeof(*wfx)) && !memcmp(wfx + 1, This->wfx + 1, wfx->cbSize))
    {
        LeaveCriticalSection(&This->cs);
        return S_OK;
    }

    if (This->state != SPAS_CLOSED)
    {
        LeaveCriticalSection(&This->cs);
        return SPERR_DEVICE_BUSY;
    }

    /* Determine whether the device supports the requested format. */
    res = waveOutOpen(NULL, This->device_id, wfx, 0, 0, WAVE_FORMAT_QUERY);
    if (res != MMSYSERR_NOERROR)
    {
        LeaveCriticalSection(&This->cs);
        return res == WAVERR_BADFORMAT ? SPERR_UNSUPPORTED_FORMAT : SPERR_GENERIC_MMSYS_ERROR;
    }

    if (!(new_wfx = malloc(sizeof(*wfx) + wfx->cbSize)))
    {
        LeaveCriticalSection(&This->cs);
        return E_OUTOFMEMORY;
    }
    memcpy(new_wfx, wfx, sizeof(*wfx) + wfx->cbSize);
    free(This->wfx);
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
    struct mmaudio *This = impl_from_ISpMMSysAudio(iface);

    TRACE("(%p).\n", iface);

    return This->event;
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

    if (id == This->device_id)
    {
        LeaveCriticalSection(&This->cs);
        return S_OK;
    }
    if (This->state != SPAS_CLOSED)
    {
        LeaveCriticalSection(&This->cs);
        return SPERR_DEVICE_BUSY;
    }
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

    if (!(This = calloc(1, sizeof(*This))))
        return E_OUTOFMEMORY;
    This->ISpEventSource_iface.lpVtbl = &event_source_vtbl;
    This->ISpEventSink_iface.lpVtbl = &event_sink_vtbl;
    This->ISpObjectWithToken_iface.lpVtbl = &objwithtoken_vtbl;
    This->ISpMMSysAudio_iface.lpVtbl = &mmsysaudio_vtbl;
    This->ref = 1;

    This->flow = flow;
    This->token = NULL;
    This->device_id = WAVE_MAPPER;
    This->state = SPAS_CLOSED;

    if (!(This->wfx = malloc(sizeof(*This->wfx))))
    {
        free(This);
        return E_OUTOFMEMORY;
    }
    This->wfx->wFormatTag = WAVE_FORMAT_PCM;
    This->wfx->nChannels = 1;
    This->wfx->nSamplesPerSec = 22050;
    This->wfx->nAvgBytesPerSec = 22050 * 2;
    This->wfx->nBlockAlign = 2;
    This->wfx->wBitsPerSample = 16;
    This->wfx->cbSize = 0;

    This->pending_buf_count = 0;
    This->event = CreateEventW(NULL, TRUE, TRUE, NULL);

    InitializeCriticalSection(&This->cs);
    InitializeCriticalSection(&This->pending_cs);

    hr = ISpMMSysAudio_QueryInterface(&This->ISpMMSysAudio_iface, iid, obj);
    ISpMMSysAudio_Release(&This->ISpMMSysAudio_iface);
    return hr;
}

HRESULT mmaudio_out_create(IUnknown *outer, REFIID iid, void **obj)
{
    return mmaudio_create(outer, iid, obj, FLOW_OUT);
}
