/*
 * Direct Sound Audio Renderer
 *
 * Copyright 2004 Christian Costa
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

#include "quartz_private.h"

#include "uuids.h"
#include "vfwmsgs.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "evcode.h"
#include "strmif.h"
#include "dsound.h"
#include "amaudio.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

/* NOTE: buffer can still be filled completely,
 * but we start waiting until only this amount is buffered
 */
static const REFERENCE_TIME DSoundRenderer_Max_Fill = 150 * 10000;

struct dsound_render
{
    struct strmbase_filter filter;
    struct strmbase_passthrough passthrough;
    IAMDirectSound IAMDirectSound_iface;
    IBasicAudio IBasicAudio_iface;
    IQualityControl IQualityControl_iface;
    IUnknown *system_clock;

    struct strmbase_sink sink;

    CRITICAL_SECTION stream_cs;
    /* Signaled when the filter has completed a state change. The filter waits
     * for this event in IBaseFilter::GetState(). */
    HANDLE state_event;
    /* Signaled when a flush or state change occurs, i.e. anything that needs
     * to immediately unblock the streaming thread. */
    HANDLE flush_event;
    REFERENCE_TIME stream_start;
    BOOL eos;

    IDirectSound8 *dsound;
    LPDIRECTSOUNDBUFFER dsbuffer;
    DWORD buf_size;
    DWORD last_playpos, writepos;

    REFERENCE_TIME play_time;

    LONG volume;
    LONG pan;
};

static struct dsound_render *impl_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct dsound_render, sink.pin);
}

static struct dsound_render *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct dsound_render, filter);
}

static struct dsound_render *impl_from_IBasicAudio(IBasicAudio *iface)
{
    return CONTAINING_RECORD(iface, struct dsound_render, IBasicAudio_iface);
}

static struct dsound_render *impl_from_IAMDirectSound(IAMDirectSound *iface)
{
    return CONTAINING_RECORD(iface, struct dsound_render, IAMDirectSound_iface);
}

static REFERENCE_TIME time_from_pos(struct dsound_render *This, DWORD pos)
{
    WAVEFORMATEX *wfx = (WAVEFORMATEX *)This->sink.pin.mt.pbFormat;
    REFERENCE_TIME ret = 10000000;
    ret = ret * pos / wfx->nAvgBytesPerSec;
    return ret;
}

static DWORD pos_from_time(struct dsound_render *This, REFERENCE_TIME time)
{
    WAVEFORMATEX *wfx = (WAVEFORMATEX *)This->sink.pin.mt.pbFormat;
    REFERENCE_TIME ret = time;
    ret *= wfx->nAvgBytesPerSec;
    ret /= 10000000;
    ret -= ret % wfx->nBlockAlign;
    return ret;
}

static void DSoundRender_UpdatePositions(struct dsound_render *This, DWORD *seqwritepos, DWORD *minwritepos)
{
    WAVEFORMATEX *wfx = (WAVEFORMATEX *)This->sink.pin.mt.pbFormat;
    BYTE *buf1, *buf2;
    DWORD size1, size2, playpos, writepos, old_writepos, old_playpos, adv;
    BOOL writepos_set = This->writepos < This->buf_size;

    /* Update position and zero */
    old_writepos = This->writepos;
    old_playpos = This->last_playpos;
    if (old_writepos <= old_playpos)
        old_writepos += This->buf_size;

    IDirectSoundBuffer_GetCurrentPosition(This->dsbuffer, &playpos, &writepos);
    if (old_playpos > playpos) {
        adv = This->buf_size + playpos - old_playpos;
        This->play_time += time_from_pos(This, This->buf_size);
    } else
        adv = playpos - old_playpos;
    This->last_playpos = playpos;
    if (adv) {
        TRACE("Moving from %u to %u: clearing %u bytes\n", old_playpos, playpos, adv);
        IDirectSoundBuffer_Lock(This->dsbuffer, old_playpos, adv, (void**)&buf1, &size1, (void**)&buf2, &size2, 0);
        memset(buf1, wfx->wBitsPerSample == 8 ? 128  : 0, size1);
        memset(buf2, wfx->wBitsPerSample == 8 ? 128  : 0, size2);
        IDirectSoundBuffer_Unlock(This->dsbuffer, buf1, size1, buf2, size2);
    }
    *minwritepos = writepos;
    if (!writepos_set || old_writepos < writepos) {
        if (writepos_set) {
            This->writepos = This->buf_size;
            FIXME("Underrun of data occurred!\n");
        }
        *seqwritepos = writepos;
    } else
        *seqwritepos = This->writepos;
}

static HRESULT DSoundRender_GetWritePos(struct dsound_render *This,
        DWORD *ret_writepos, REFERENCE_TIME write_at, DWORD *pfree, DWORD *skip)
{
    WAVEFORMATEX *wfx = (WAVEFORMATEX *)This->sink.pin.mt.pbFormat;
    DWORD writepos, min_writepos, playpos;
    REFERENCE_TIME max_lag = 50 * 10000;
    REFERENCE_TIME cur, writepos_t, delta_t;

    DSoundRender_UpdatePositions(This, &writepos, &min_writepos);
    playpos = This->last_playpos;
    if (This->filter.clock)
    {
        IReferenceClock_GetTime(This->filter.clock, &cur);
        cur -= This->stream_start;
    } else
        write_at = -1;

    if (writepos == min_writepos)
        max_lag = 0;

    *skip = 0;
    if (write_at < 0) {
        *ret_writepos = writepos;
        goto end;
    }

    if (writepos >= playpos)
        writepos_t = cur + time_from_pos(This, writepos - playpos);
    else
        writepos_t = cur + time_from_pos(This, This->buf_size + writepos - playpos);

    /* write_at: Starting time of sample */
    /* cur: current time of play position */
    /* writepos_t: current time of our pointer play position */
    delta_t = write_at - writepos_t;
    if (delta_t >= -max_lag && delta_t <= max_lag) {
        TRACE("Continuing from old position\n");
        *ret_writepos = writepos;
    } else if (delta_t < 0) {
        REFERENCE_TIME past, min_writepos_t;
        WARN("Delta too big %s/%s, overwriting old data or even skipping\n", debugstr_time(delta_t), debugstr_time(max_lag));
        if (min_writepos >= playpos)
            min_writepos_t = cur + time_from_pos(This, min_writepos - playpos);
        else
            min_writepos_t = cur + time_from_pos(This, This->buf_size - playpos + min_writepos);
        past = min_writepos_t - write_at;
        if (past >= 0) {
            DWORD skipbytes = pos_from_time(This, past);
            WARN("Skipping %u bytes\n", skipbytes);
            *skip = skipbytes;
            *ret_writepos = min_writepos;
        } else {
            DWORD aheadbytes = pos_from_time(This, -past);
            WARN("Advancing %u bytes\n", aheadbytes);
            *ret_writepos = (min_writepos + aheadbytes) % This->buf_size;
        }
    } else /* delta_t > 0 */ {
        DWORD aheadbytes;
        WARN("Delta too big %s/%s, too far ahead\n", debugstr_time(delta_t), debugstr_time(max_lag));
        aheadbytes = pos_from_time(This, delta_t);
        WARN("Advancing %u bytes\n", aheadbytes);
        if (delta_t >= DSoundRenderer_Max_Fill)
            return S_FALSE;
        *ret_writepos = (min_writepos + aheadbytes) % This->buf_size;
    }
end:
    if (playpos > *ret_writepos)
        *pfree = playpos - *ret_writepos;
    else if (playpos == *ret_writepos)
        *pfree = This->buf_size - wfx->nBlockAlign;
    else
        *pfree = This->buf_size + playpos - *ret_writepos;
    if (time_from_pos(This, This->buf_size - *pfree) >= DSoundRenderer_Max_Fill) {
        TRACE("Blocked: too full %s / %s\n", debugstr_time(time_from_pos(This, This->buf_size - *pfree)),
                debugstr_time(DSoundRenderer_Max_Fill));
        return S_FALSE;
    }
    return S_OK;
}

static HRESULT DSoundRender_HandleEndOfStream(struct dsound_render *This)
{
    while (This->filter.state == State_Running)
    {
        DWORD pos1, pos2;
        DSoundRender_UpdatePositions(This, &pos1, &pos2);
        if (pos1 == pos2)
            break;

        LeaveCriticalSection(&This->stream_cs);
        WaitForSingleObject(This->flush_event, 10);
        EnterCriticalSection(&This->stream_cs);
    }

    return S_OK;
}

static HRESULT DSoundRender_SendSampleData(struct dsound_render *This,
        REFERENCE_TIME tStart, REFERENCE_TIME tStop, const BYTE *data, DWORD size)
{
    HRESULT hr;

    while (size && This->filter.state != State_Stopped) {
        DWORD writepos, skip = 0, free, size1, size2, ret;
        BYTE *buf1, *buf2;

        if (This->filter.state == State_Running)
            hr = DSoundRender_GetWritePos(This, &writepos, tStart, &free, &skip);
        else
            hr = S_FALSE;

        if (hr != S_OK) {
            LeaveCriticalSection(&This->stream_cs);
            ret = WaitForSingleObject(This->flush_event, 10);
            EnterCriticalSection(&This->stream_cs);
            if (This->sink.flushing || This->filter.state == State_Stopped)
                return This->filter.state == State_Paused ? S_OK : VFW_E_WRONG_STATE;
            if (ret != WAIT_TIMEOUT)
                ERR("%x\n", ret);
            continue;
        }
        tStart = -1;

        if (skip)
            FIXME("Sample dropped %u of %u bytes\n", skip, size);
        if (skip >= size)
            return S_OK;
        data += skip;
        size -= skip;

        hr = IDirectSoundBuffer_Lock(This->dsbuffer, writepos, min(free, size), (void**)&buf1, &size1, (void**)&buf2, &size2, 0);
        if (hr != DS_OK) {
            ERR("Unable to lock sound buffer! (%x)\n", hr);
            break;
        }
        memcpy(buf1, data, size1);
        if (size2)
            memcpy(buf2, data+size1, size2);
        IDirectSoundBuffer_Unlock(This->dsbuffer, buf1, size1, buf2, size2);
        This->writepos = (writepos + size1 + size2) % This->buf_size;
        TRACE("Wrote %u bytes at %u, next at %u - (%u/%u)\n", size1+size2, writepos, This->writepos, free, size);
        data += size1 + size2;
        size -= size1 + size2;
    }
    return S_OK;
}

static HRESULT WINAPI DSoundRender_PrepareReceive(struct dsound_render *This, IMediaSample *pSample)
{
    HRESULT hr;
    AM_MEDIA_TYPE *amt;

    if (IMediaSample_GetMediaType(pSample, &amt) == S_OK)
    {
        AM_MEDIA_TYPE *orig = &This->sink.pin.mt;
        WAVEFORMATEX *origfmt = (WAVEFORMATEX *)orig->pbFormat;
        WAVEFORMATEX *newfmt = (WAVEFORMATEX *)amt->pbFormat;

        TRACE("Format change.\n");
        strmbase_dump_media_type(amt);

        if (origfmt->wFormatTag == newfmt->wFormatTag &&
            origfmt->nChannels == newfmt->nChannels &&
            origfmt->nBlockAlign == newfmt->nBlockAlign &&
            origfmt->wBitsPerSample == newfmt->wBitsPerSample &&
            origfmt->cbSize ==  newfmt->cbSize)
        {
            if (origfmt->nSamplesPerSec != newfmt->nSamplesPerSec)
            {
                hr = IDirectSoundBuffer_SetFrequency(This->dsbuffer,
                                                     newfmt->nSamplesPerSec);
                if (FAILED(hr))
                    return VFW_E_TYPE_NOT_ACCEPTED;
                FreeMediaType(orig);
                CopyMediaType(orig, amt);
                IMediaSample_SetMediaType(pSample, NULL);
            }
        }
        else
            return VFW_E_TYPE_NOT_ACCEPTED;
    }
    return S_OK;
}

static HRESULT WINAPI DSoundRender_DoRenderSample(struct dsound_render *This, IMediaSample *pSample)
{
    LPBYTE pbSrcStream = NULL;
    LONG cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        return hr;
    }

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (FAILED(hr)) {
        ERR("Cannot get sample time (%x)\n", hr);
        tStart = tStop = -1;
    }

    if (IMediaSample_IsPreroll(pSample) == S_OK)
    {
        TRACE("Preroll!\n");
        return S_OK;
    }

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);
    TRACE("Sample data ptr = %p, size = %d\n", pbSrcStream, cbSrcStream);

    return DSoundRender_SendSampleData(This, tStart, tStop, pbSrcStream, cbSrcStream);
}

static HRESULT WINAPI dsound_render_sink_Receive(struct strmbase_sink *iface, IMediaSample *sample)
{
    struct dsound_render *filter = impl_from_strmbase_pin(&iface->pin);
    REFERENCE_TIME start, stop;
    HRESULT hr;

    if (filter->eos || filter->sink.flushing)
        return S_FALSE;

    if (filter->filter.state == State_Stopped)
        return VFW_E_WRONG_STATE;

    if (FAILED(hr = DSoundRender_PrepareReceive(filter, sample)))
        return hr;

    EnterCriticalSection(&filter->stream_cs);

    if (filter->filter.clock && SUCCEEDED(IMediaSample_GetTime(sample, &start, &stop)))
        strmbase_passthrough_update_time(&filter->passthrough, start);

    if (filter->filter.state == State_Paused)
        SetEvent(filter->state_event);

    hr = DSoundRender_DoRenderSample(filter, sample);

    LeaveCriticalSection(&filter->stream_cs);
    return hr;
}

static HRESULT dsound_render_sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct dsound_render *filter = impl_from_strmbase_pin(iface);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT dsound_render_sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE * pmt)
{
    if (!IsEqualIID(&pmt->majortype, &MEDIATYPE_Audio))
        return S_FALSE;

    return S_OK;
}

static HRESULT dsound_render_sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct dsound_render *This = impl_from_strmbase_pin(&iface->pin);
    const WAVEFORMATEX *format = (WAVEFORMATEX *)mt->pbFormat;
    HRESULT hr = S_OK;
    DSBUFFERDESC buf_desc;

    This->buf_size = format->nAvgBytesPerSec;

    memset(&buf_desc,0,sizeof(DSBUFFERDESC));
    buf_desc.dwSize = sizeof(DSBUFFERDESC);
    buf_desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN |
                       DSBCAPS_CTRLFREQUENCY | DSBCAPS_GLOBALFOCUS |
                       DSBCAPS_GETCURRENTPOSITION2;
    buf_desc.dwBufferBytes = This->buf_size;
    buf_desc.lpwfxFormat = (WAVEFORMATEX *)format;
    hr = IDirectSound8_CreateSoundBuffer(This->dsound, &buf_desc, &This->dsbuffer, NULL);
    This->writepos = This->buf_size;
    if (FAILED(hr))
        ERR("Can't create sound buffer (%x)\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IDirectSoundBuffer_SetVolume(This->dsbuffer, This->volume);
        if (FAILED(hr))
            ERR("Can't set volume to %d (%x)\n", This->volume, hr);

        hr = IDirectSoundBuffer_SetPan(This->dsbuffer, This->pan);
        if (FAILED(hr))
            ERR("Can't set pan to %d (%x)\n", This->pan, hr);
        hr = S_OK;
    }

    if (FAILED(hr) && hr != VFW_E_ALREADY_CONNECTED)
    {
        if (This->dsbuffer)
            IDirectSoundBuffer_Release(This->dsbuffer);
        This->dsbuffer = NULL;
    }

    return hr;
}

static void dsound_render_sink_disconnect(struct strmbase_sink *iface)
{
    struct dsound_render *This = impl_from_strmbase_pin(&iface->pin);

    TRACE("(%p)->()\n", iface);

    if (This->dsbuffer)
        IDirectSoundBuffer_Release(This->dsbuffer);
    This->dsbuffer = NULL;
}

static HRESULT dsound_render_sink_eos(struct strmbase_sink *iface)
{
    struct dsound_render *filter = impl_from_strmbase_pin(&iface->pin);
    IFilterGraph *graph = filter->filter.graph;
    IMediaEventSink *event_sink;
    void *buffer;
    DWORD size;

    EnterCriticalSection(&filter->stream_cs);

    filter->eos = TRUE;

    if (graph && SUCCEEDED(IFilterGraph_QueryInterface(graph,
            &IID_IMediaEventSink, (void **)&event_sink)))
    {
        IMediaEventSink_Notify(event_sink, EC_COMPLETE, S_OK,
                (LONG_PTR)&filter->filter.IBaseFilter_iface);
        IMediaEventSink_Release(event_sink);
    }
    strmbase_passthrough_eos(&filter->passthrough);
    SetEvent(filter->state_event);

    DSoundRender_HandleEndOfStream(filter);

    IDirectSoundBuffer_Lock(filter->dsbuffer, 0, 0, &buffer, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER);
    memset(buffer, 0, size);
    IDirectSoundBuffer_Unlock(filter->dsbuffer, buffer, size, NULL, 0);

    LeaveCriticalSection(&filter->stream_cs);
    return S_OK;
}

static HRESULT dsound_render_sink_begin_flush(struct strmbase_sink *iface)
{
    struct dsound_render *filter = impl_from_strmbase_pin(&iface->pin);

    SetEvent(filter->flush_event);
    return S_OK;
}

static HRESULT dsound_render_sink_end_flush(struct strmbase_sink *iface)
{
    struct dsound_render *filter = impl_from_strmbase_pin(&iface->pin);

    EnterCriticalSection(&filter->stream_cs);

    filter->eos = FALSE;
    strmbase_passthrough_invalidate_time(&filter->passthrough);
    ResetEvent(filter->flush_event);

    if (filter->dsbuffer)
    {
        void *buffer;
        DWORD size;

        /* Force a reset */
        IDirectSoundBuffer_Lock(filter->dsbuffer, 0, 0, &buffer, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER);
        memset(buffer, 0, size);
        IDirectSoundBuffer_Unlock(filter->dsbuffer, buffer, size, NULL, 0);
        filter->writepos = filter->buf_size;
    }

    LeaveCriticalSection(&filter->stream_cs);
    return S_OK;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_interface = dsound_render_sink_query_interface,
    .base.pin_query_accept = dsound_render_sink_query_accept,
    .pfnReceive = dsound_render_sink_Receive,
    .sink_connect = dsound_render_sink_connect,
    .sink_disconnect = dsound_render_sink_disconnect,
    .sink_eos = dsound_render_sink_eos,
    .sink_begin_flush = dsound_render_sink_begin_flush,
    .sink_end_flush = dsound_render_sink_end_flush,
};

static void dsound_render_destroy(struct strmbase_filter *iface)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    if (filter->dsbuffer)
        IDirectSoundBuffer_Release(filter->dsbuffer);
    filter->dsbuffer = NULL;
    if (filter->dsound)
        IDirectSound8_Release(filter->dsound);
    filter->dsound = NULL;

    if (filter->sink.pin.peer)
        IPin_Disconnect(filter->sink.pin.peer);
    IPin_Disconnect(&filter->sink.pin.IPin_iface);
    strmbase_sink_cleanup(&filter->sink);

    filter->stream_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->stream_cs);

    CloseHandle(filter->state_event);
    CloseHandle(filter->flush_event);

    strmbase_passthrough_cleanup(&filter->passthrough);
    strmbase_filter_cleanup(&filter->filter);
    free(filter);

    InterlockedDecrement(&object_locks);
}

static struct strmbase_pin *dsound_render_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    if (index == 0)
        return &filter->sink.pin;
    return NULL;
}

static HRESULT dsound_render_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IAMDirectSound))
        *out = &filter->IAMDirectSound_iface;
    else if (IsEqualGUID(iid, &IID_IBasicAudio))
        *out = &filter->IBasicAudio_iface;
    else if (IsEqualGUID(iid, &IID_IMediaPosition))
        *out = &filter->passthrough.IMediaPosition_iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &filter->passthrough.IMediaSeeking_iface;
    else if (IsEqualGUID(iid, &IID_IQualityControl))
        *out = &filter->IQualityControl_iface;
    else if (IsEqualGUID(iid, &IID_IReferenceClock))
        return IUnknown_QueryInterface(filter->system_clock, iid, out);
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT dsound_render_init_stream(struct strmbase_filter *iface)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    if (filter->sink.pin.peer)
        ResetEvent(filter->state_event);
    filter->eos = FALSE;
    ResetEvent(filter->flush_event);

    return filter->sink.pin.peer ? S_FALSE : S_OK;
}

static HRESULT dsound_render_start_stream(struct strmbase_filter *iface, REFERENCE_TIME start)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    filter->stream_start = start;

    SetEvent(filter->state_event);

    if (filter->sink.pin.peer)
    {
        filter->eos = FALSE;
        IDirectSoundBuffer_Play(filter->dsbuffer, 0, 0, DSBPLAY_LOOPING);
    }

    return S_OK;
}

static HRESULT dsound_render_stop_stream(struct strmbase_filter *iface)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    if (filter->sink.pin.peer)
    {
        IDirectSoundBuffer_Stop(filter->dsbuffer);
        filter->writepos = filter->buf_size;
    }
    return S_OK;
}

static HRESULT dsound_render_cleanup_stream(struct strmbase_filter *iface)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    strmbase_passthrough_invalidate_time(&filter->passthrough);
    SetEvent(filter->state_event);
    SetEvent(filter->flush_event);

    return S_OK;
}

static HRESULT dsound_render_wait_state(struct strmbase_filter *iface, DWORD timeout)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    if (WaitForSingleObject(filter->state_event, timeout) == WAIT_TIMEOUT)
        return VFW_S_STATE_INTERMEDIATE;
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_destroy = dsound_render_destroy,
    .filter_get_pin = dsound_render_get_pin,
    .filter_query_interface = dsound_render_query_interface,
    .filter_init_stream = dsound_render_init_stream,
    .filter_start_stream = dsound_render_start_stream,
    .filter_stop_stream = dsound_render_stop_stream,
    .filter_cleanup_stream = dsound_render_cleanup_stream,
    .filter_wait_state = dsound_render_wait_state,
};

/*** IUnknown methods ***/
static HRESULT WINAPI Basicaudio_QueryInterface(IBasicAudio *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    struct dsound_render *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppvObj);

    return IUnknown_QueryInterface(This->filter.outer_unk, riid, ppvObj);
}

static ULONG WINAPI Basicaudio_AddRef(IBasicAudio *iface) {
    struct dsound_render *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_AddRef(This->filter.outer_unk);
}

static ULONG WINAPI Basicaudio_Release(IBasicAudio *iface) {
    struct dsound_render *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_Release(This->filter.outer_unk);
}

HRESULT WINAPI basic_audio_GetTypeInfoCount(IBasicAudio *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

HRESULT WINAPI basic_audio_GetTypeInfo(IBasicAudio *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#x, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IBasicAudio_tid, typeinfo);
}

HRESULT WINAPI basic_audio_GetIDsOfNames(IBasicAudio *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#x, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicAudio_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI basic_audio_Invoke(IBasicAudio *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %d, iid %s, lcid %#x, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicAudio_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI Basicaudio_put_Volume(IBasicAudio *iface,
                                            LONG lVolume) {
    struct dsound_render *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, lVolume);

    if (lVolume > DSBVOLUME_MAX || lVolume < DSBVOLUME_MIN)
        return E_INVALIDARG;

    if (This->dsbuffer) {
        if (FAILED(IDirectSoundBuffer_SetVolume(This->dsbuffer, lVolume)))
            return E_FAIL;
    }

    This->volume = lVolume;
    return S_OK;
}

static HRESULT WINAPI Basicaudio_get_Volume(IBasicAudio *iface,
                                            LONG *plVolume) {
    struct dsound_render *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, plVolume);

    if (!plVolume)
        return E_POINTER;

    *plVolume = This->volume;
    return S_OK;
}

static HRESULT WINAPI Basicaudio_put_Balance(IBasicAudio *iface,
                                             LONG lBalance) {
    struct dsound_render *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, lBalance);

    if (lBalance < DSBPAN_LEFT || lBalance > DSBPAN_RIGHT)
        return E_INVALIDARG;

    if (This->dsbuffer) {
        if (FAILED(IDirectSoundBuffer_SetPan(This->dsbuffer, lBalance)))
            return E_FAIL;
    }

    This->pan = lBalance;
    return S_OK;
}

static HRESULT WINAPI Basicaudio_get_Balance(IBasicAudio *iface,
                                             LONG *plBalance) {
    struct dsound_render *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, plBalance);

    if (!plBalance)
        return E_POINTER;

    *plBalance = This->pan;
    return S_OK;
}

static const IBasicAudioVtbl IBasicAudio_Vtbl =
{
    Basicaudio_QueryInterface,
    Basicaudio_AddRef,
    Basicaudio_Release,
    basic_audio_GetTypeInfoCount,
    basic_audio_GetTypeInfo,
    basic_audio_GetIDsOfNames,
    basic_audio_Invoke,
    Basicaudio_put_Volume,
    Basicaudio_get_Volume,
    Basicaudio_put_Balance,
    Basicaudio_get_Balance
};

/*** IUnknown methods ***/
static HRESULT WINAPI AMDirectSound_QueryInterface(IAMDirectSound *iface,
						REFIID riid,
						LPVOID*ppvObj)
{
    struct dsound_render *This = impl_from_IAMDirectSound(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppvObj);

    return IUnknown_QueryInterface(This->filter.outer_unk, riid, ppvObj);
}

static ULONG WINAPI AMDirectSound_AddRef(IAMDirectSound *iface)
{
    struct dsound_render *This = impl_from_IAMDirectSound(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_AddRef(This->filter.outer_unk);
}

static ULONG WINAPI AMDirectSound_Release(IAMDirectSound *iface)
{
    struct dsound_render *This = impl_from_IAMDirectSound(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_Release(This->filter.outer_unk);
}

/*** IAMDirectSound methods ***/
static HRESULT WINAPI AMDirectSound_GetDirectSoundInterface(IAMDirectSound *iface,  IDirectSound **ds)
{
    struct dsound_render *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, ds);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_GetPrimaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer **buf)
{
    struct dsound_render *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_GetSecondaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer **buf)
{
    struct dsound_render *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_ReleaseDirectSoundInterface(IAMDirectSound *iface, IDirectSound *ds)
{
    struct dsound_render *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, ds);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_ReleasePrimaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer *buf)
{
    struct dsound_render *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_ReleaseSecondaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer *buf)
{
    struct dsound_render *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_SetFocusWindow(IAMDirectSound *iface, HWND hwnd, BOOL bgaudible)
{
    struct dsound_render *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p,%d): stub\n", This, iface, hwnd, bgaudible);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_GetFocusWindow(IAMDirectSound *iface, HWND *hwnd, BOOL *bgaudible)
{
    struct dsound_render *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", This, iface, hwnd, bgaudible);

    return E_NOTIMPL;
}

static const IAMDirectSoundVtbl IAMDirectSound_Vtbl =
{
    AMDirectSound_QueryInterface,
    AMDirectSound_AddRef,
    AMDirectSound_Release,
    AMDirectSound_GetDirectSoundInterface,
    AMDirectSound_GetPrimaryBufferInterface,
    AMDirectSound_GetSecondaryBufferInterface,
    AMDirectSound_ReleaseDirectSoundInterface,
    AMDirectSound_ReleasePrimaryBufferInterface,
    AMDirectSound_ReleaseSecondaryBufferInterface,
    AMDirectSound_SetFocusWindow,
    AMDirectSound_GetFocusWindow
};

static struct dsound_render *impl_from_IQualityControl(IQualityControl *iface)
{
    return CONTAINING_RECORD(iface, struct dsound_render, IQualityControl_iface);
}

static HRESULT WINAPI dsound_render_qc_QueryInterface(IQualityControl *iface,
        REFIID iid, void **out)
{
    struct dsound_render *filter = impl_from_IQualityControl(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, iid, out);
}

static ULONG WINAPI dsound_render_qc_AddRef(IQualityControl *iface)
{
    struct dsound_render *filter = impl_from_IQualityControl(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI dsound_render_qc_Release(IQualityControl *iface)
{
    struct dsound_render *filter = impl_from_IQualityControl(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static HRESULT WINAPI dsound_render_qc_Notify(IQualityControl *iface,
        IBaseFilter *sender, Quality q)
{
    struct dsound_render *filter = impl_from_IQualityControl(iface);

    FIXME("filter %p, sender %p, type %#x, proportion %u, late %s, timestamp %s, stub!\n",
            filter, sender, q.Type, q.Proportion, debugstr_time(q.Late), debugstr_time(q.TimeStamp));

    return E_NOTIMPL;
}

static HRESULT WINAPI dsound_render_qc_SetSink(IQualityControl *iface, IQualityControl *sink)
{
    struct dsound_render *filter = impl_from_IQualityControl(iface);

    FIXME("filter %p, sink %p, stub!\n", filter, sink);

    return E_NOTIMPL;
}

static const IQualityControlVtbl dsound_render_qc_vtbl =
{
    dsound_render_qc_QueryInterface,
    dsound_render_qc_AddRef,
    dsound_render_qc_Release,
    dsound_render_qc_Notify,
    dsound_render_qc_SetSink,
};

HRESULT dsound_render_create(IUnknown *outer, IUnknown **out)
{
    static const DSBUFFERDESC buffer_desc = {
        .dwSize = sizeof(DSBUFFERDESC),
        .dwFlags = DSBCAPS_PRIMARYBUFFER,
    };

    struct dsound_render *object;
    IDirectSoundBuffer *buffer;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_DSoundRender, &filter_ops);

    if (FAILED(hr = system_clock_create(&object->filter.IUnknown_inner, &object->system_clock)))
    {
        strmbase_filter_cleanup(&object->filter);
        free(object);
        return hr;
    }

    if (FAILED(hr = DirectSoundCreate8(NULL, &object->dsound, NULL)))
    {
        IUnknown_Release(object->system_clock);
        strmbase_filter_cleanup(&object->filter);
        free(object);
        return hr;
    }

    if (FAILED(hr = IDirectSound8_SetCooperativeLevel(object->dsound,
            GetDesktopWindow(), DSSCL_PRIORITY)))
    {
        IDirectSound8_Release(object->dsound);
        IUnknown_Release(object->system_clock);
        strmbase_filter_cleanup(&object->filter);
        free(object);
        return hr;
    }

    if (SUCCEEDED(hr = IDirectSound8_CreateSoundBuffer(object->dsound,
            &buffer_desc, &buffer, NULL)))
    {
        IDirectSoundBuffer_Play(buffer, 0, 0, DSBPLAY_LOOPING);
        IDirectSoundBuffer_Release(buffer);
    }

    strmbase_passthrough_init(&object->passthrough, (IUnknown *)&object->filter.IBaseFilter_iface);
    ISeekingPassThru_Init(&object->passthrough.ISeekingPassThru_iface, TRUE, &object->sink.pin.IPin_iface);

    strmbase_sink_init(&object->sink, &object->filter, L"Audio Input pin (rendered)", &sink_ops, NULL);

    InitializeCriticalSection(&object->stream_cs);
    object->stream_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__": dsound_render.stream_cs");
    object->state_event = CreateEventW(NULL, TRUE, TRUE, NULL);
    object->flush_event = CreateEventW(NULL, TRUE, TRUE, NULL);

    object->IBasicAudio_iface.lpVtbl = &IBasicAudio_Vtbl;
    object->IAMDirectSound_iface.lpVtbl = &IAMDirectSound_Vtbl;
    object->IQualityControl_iface.lpVtbl = &dsound_render_qc_vtbl;

    TRACE("Created DirectSound renderer %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
