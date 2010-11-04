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

#include "config.h"

#include "quartz_private.h"
#include "control_private.h"
#include "pin.h"

#include "uuids.h"
#include "vfwmsgs.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "evcode.h"
#include "strmif.h"
#include "dsound.h"
#include "amaudio.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const WCHAR wcsInputPinName[] = {'i','n','p','u','t',' ','p','i','n',0};

static const IBaseFilterVtbl DSoundRender_Vtbl;
static const IPinVtbl DSoundRender_InputPin_Vtbl;
static const IBasicAudioVtbl IBasicAudio_Vtbl;
static const IReferenceClockVtbl IReferenceClock_Vtbl;
static const IMediaSeekingVtbl IMediaSeeking_Vtbl;
static const IAMDirectSoundVtbl IAMDirectSound_Vtbl;
static const IAMFilterMiscFlagsVtbl IAMFilterMiscFlags_Vtbl;

typedef struct DSoundRenderImpl
{
    BaseFilter filter;

    const IBasicAudioVtbl *IBasicAudio_vtbl;
    const IReferenceClockVtbl *IReferenceClock_vtbl;
    const IAMDirectSoundVtbl *IAMDirectSound_vtbl;
    const IAMFilterMiscFlagsVtbl *IAMFilterMiscFlags_vtbl;
    IUnknown *seekthru_unk;

    REFERENCE_TIME rtLastStop;

    BaseInputPin * pInputPin;

    IDirectSound8 *dsound;
    LPDIRECTSOUNDBUFFER dsbuffer;
    DWORD buf_size;
    DWORD write_pos;
    DWORD write_loops;

    DWORD last_play_pos;
    DWORD play_loops;
    DWORD in_loop;

    REFERENCE_TIME play_time;

    HANDLE state_change, blocked;

    LONG volume;
    LONG pan;
} DSoundRenderImpl;

static inline HRESULT DSoundRender_GetPos(DSoundRenderImpl *This, DWORD *pPlayPos, REFERENCE_TIME *pRefTime)
{
    HRESULT hr;

    EnterCriticalSection(&This->filter.csFilter);
    {
        DWORD state;
        DWORD write_pos;

        hr = IDirectSoundBuffer_GetStatus(This->dsbuffer, &state);
        if (SUCCEEDED(hr) && !(state & DSBSTATUS_PLAYING) && This->filter.state == State_Running)
        {
            TRACE("Not playing, kickstarting the engine\n");

            hr = IDirectSoundBuffer_Play(This->dsbuffer, 0, 0, DSBPLAY_LOOPING);
            if (FAILED(hr))
                ERR("Can't play sound buffer (%x)\n", hr);
        }

        if (SUCCEEDED(hr))
            hr = IDirectSoundBuffer_GetCurrentPosition(This->dsbuffer, pPlayPos, &write_pos);
        if (hr == S_OK)
        {
            DWORD play_pos = *pPlayPos;

            if (play_pos < This->last_play_pos)
                This->play_loops++;
            This->last_play_pos = play_pos;

            /* If we really fell behind, start at the next possible position
             * Also happens when just starting playback for the first time,
             * or when flushing
             */
            if ((This->play_loops*This->buf_size)+play_pos >=
                (This->write_loops*This->buf_size)+This->write_pos)
                This->write_pos = write_pos;

            if (pRefTime)
            {
                REFERENCE_TIME play_time;
                play_time = ((REFERENCE_TIME)This->play_loops*10000000) +
                            ((REFERENCE_TIME)play_pos*10000000/This->buf_size);

                /* Don't let time run backwards */
                if(play_time-This->play_time > 0)
                    This->play_time = play_time;
                else
                    hr = S_FALSE;

                *pRefTime = This->play_time;
            }
        }
    }
    LeaveCriticalSection(&This->filter.csFilter);

    return hr;
}

static HRESULT DSoundRender_SendSampleData(DSoundRenderImpl* This, const BYTE *data, DWORD size)
{
    HRESULT hr = S_OK;
    LPBYTE lpbuf1 = NULL;
    LPBYTE lpbuf2 = NULL;
    DWORD dwsize1 = 0;
    DWORD dwsize2 = 0;
    DWORD size2;
    DWORD play_pos,buf_free;

    do {

        hr = DSoundRender_GetPos(This, &play_pos, NULL);
        if (hr != DS_OK)
        {
            ERR("GetPos returned error: %x\n", hr);
            break;
        }
        if (This->write_pos <= play_pos)
             buf_free = play_pos-This->write_pos;
        else
             buf_free = This->buf_size - This->write_pos + play_pos;

        /* Wait for enough of the buffer to empty before filling it */
        if(buf_free < This->buf_size/20)
        {
            DWORD ret;
            This->in_loop = 1;
            LeaveCriticalSection(&This->filter.csFilter);
            ret = WaitForSingleObject(This->blocked, 50);
            if (ret != WAIT_TIMEOUT)
                ERR("%x\n", ret);
            EnterCriticalSection(&This->filter.csFilter);
            This->in_loop = 0;
            if (This->pInputPin->flushing)
                return VFW_E_WRONG_STATE;
            if (This->filter.state == State_Stopped)
                return VFW_E_WRONG_STATE;
            continue;
        }

        size2 = min(buf_free, size);
        hr = IDirectSoundBuffer_Lock(This->dsbuffer, This->write_pos, size2, (LPVOID *)&lpbuf1, &dwsize1, (LPVOID *)&lpbuf2, &dwsize2, 0);
        if (hr != DS_OK) {
            ERR("Unable to lock sound buffer! (%x)\n", hr);
            break;
        }
        /* TRACE("write_pos=%d, size=%d, sz1=%d, sz2=%d\n", This->write_pos, size2, dwsize1, dwsize2); */

        memcpy(lpbuf1, data, dwsize1);
        if (dwsize2)
            memcpy(lpbuf2, data + dwsize1, dwsize2);

        hr = IDirectSoundBuffer_Unlock(This->dsbuffer, lpbuf1, dwsize1, lpbuf2, dwsize2);
        if (hr != DS_OK)
            ERR("Unable to unlock sound buffer! (%x)\n", hr);

        size -= dwsize1 + dwsize2;
        data += dwsize1 + dwsize2;
        This->write_pos += dwsize1 + dwsize2;
        if (This->write_pos >= This->buf_size)
        {
            This->write_pos -= This->buf_size;
            This->write_loops++;
        }
    } while (size && This->filter.state == State_Running);

    return hr;
}

static HRESULT WINAPI DSoundRender_Receive(BaseInputPin *pin, IMediaSample * pSample)
{
    DSoundRenderImpl *This = (DSoundRenderImpl*)pin->pin.pinInfo.pFilter;
    LPBYTE pbSrcStream = NULL;
    LONG cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;
    AM_MEDIA_TYPE *amt;

    TRACE("%p %p\n", pin, pSample);

    /* Slightly incorrect, Pause completes when a frame is received so we should signal
     * pause completion here, but for sound playing a single frame doesn't make sense
     */

    EnterCriticalSection(&This->filter.csFilter);

    if (This->pInputPin->end_of_stream || This->pInputPin->flushing)
    {
        LeaveCriticalSection(&This->filter.csFilter);
        return S_FALSE;
    }

    if (This->filter.state == State_Stopped)
    {
        LeaveCriticalSection(&This->filter.csFilter);
        return VFW_E_WRONG_STATE;
    }

    if (IMediaSample_GetMediaType(pSample, &amt) == S_OK)
    {
        AM_MEDIA_TYPE *orig = &This->pInputPin->pin.mtCurrent;
        WAVEFORMATEX *origfmt = (WAVEFORMATEX *)orig->pbFormat;
        WAVEFORMATEX *newfmt = (WAVEFORMATEX *)amt->pbFormat;

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
                {
                    LeaveCriticalSection(&This->filter.csFilter);
                    return VFW_E_TYPE_NOT_ACCEPTED;
                }
                FreeMediaType(orig);
                CopyMediaType(orig, amt);
                IMediaSample_SetMediaType(pSample, NULL);
            }
        }
        else
        {
            LeaveCriticalSection(&This->filter.csFilter);
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        LeaveCriticalSection(&This->filter.csFilter);
        return hr;
    }

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (FAILED(hr))
        ERR("Cannot get sample time (%x)\n", hr);
    else
        MediaSeekingPassThru_RegisterMediaTime(This->seekthru_unk, tStart);

    if (This->rtLastStop != tStart && (IMediaSample_IsDiscontinuity(pSample) == S_FALSE))
        WARN("Unexpected discontinuity: Last: %u.%03u, tStart: %u.%03u\n",
            (DWORD)(This->rtLastStop / 10000000), (DWORD)((This->rtLastStop / 10000)%1000),
            (DWORD)(tStart / 10000000), (DWORD)((tStart / 10000)%1000));
    This->rtLastStop = tStop;

    if (IMediaSample_IsPreroll(pSample) == S_OK)
    {
        TRACE("Preroll!\n");
        LeaveCriticalSection(&This->filter.csFilter);
        return S_OK;
    }

    if (This->filter.state == State_Paused)
    {
        SetEvent(This->state_change);
        LeaveCriticalSection(&This->filter.csFilter);
        WaitForSingleObject(This->blocked, INFINITE);
        EnterCriticalSection(&This->filter.csFilter);
        if (This->filter.state == State_Stopped)
        {
            LeaveCriticalSection(&This->filter.csFilter);
            return VFW_E_WRONG_STATE;
        }

        if (This->filter.state == State_Paused)
        {
            /* Assuming we return because of flushing */
            TRACE("Flushing\n");
            LeaveCriticalSection(&This->filter.csFilter);
            return S_OK;
        }
        SetEvent(This->state_change);
    }

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);
    TRACE("Sample data ptr = %p, size = %d\n", pbSrcStream, cbSrcStream);

#if 0 /* For debugging purpose */
    {
        int i;
        for(i = 0; i < cbSrcStream; i++)
        {
	    if ((i!=0) && !(i%16))
                TRACE("\n");
            TRACE("%02x ", pbSrcStream[i]);
        }
        TRACE("\n");
    }
#endif

    hr = DSoundRender_SendSampleData(This, pbSrcStream, cbSrcStream);
    SetEvent(This->state_change);
    LeaveCriticalSection(&This->filter.csFilter);
    return hr;
}

static HRESULT WINAPI DSoundRender_CheckMediaType(BasePin *iface, const AM_MEDIA_TYPE * pmt)
{
    WAVEFORMATEX* format;

    if (!IsEqualIID(&pmt->majortype, &MEDIATYPE_Audio))
        return S_FALSE;

    format =  (WAVEFORMATEX*)pmt->pbFormat;
    TRACE("Format = %p\n", format);
    TRACE("wFormatTag = %x %x\n", format->wFormatTag, WAVE_FORMAT_PCM);
    TRACE("nChannels = %d\n", format->nChannels);
    TRACE("nSamplesPerSec = %d\n", format->nAvgBytesPerSec);
    TRACE("nAvgBytesPerSec = %d\n", format->nAvgBytesPerSec);
    TRACE("nBlockAlign = %d\n", format->nBlockAlign);
    TRACE("wBitsPerSample = %d\n", format->wBitsPerSample);

    if (!IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_PCM))
        return S_FALSE;

    return S_OK;
}

static IPin* WINAPI DSoundRender_GetPin(BaseFilter *iface, int pos)
{
    DSoundRenderImpl *This = (DSoundRenderImpl *)iface;

    if (pos >= 1 || pos < 0)
        return NULL;

    IPin_AddRef((IPin*)This->pInputPin);
    return (IPin*)This->pInputPin;
}

static LONG WINAPI DSoundRender_GetPinCount(BaseFilter *iface)
{
    /* Our pins are static */
    return 1;
}

static const BaseFilterFuncTable BaseFuncTable = {
    DSoundRender_GetPin,
    DSoundRender_GetPinCount
};

static const  BasePinFuncTable input_BaseFuncTable = {
    DSoundRender_CheckMediaType,
    NULL,
    BasePinImpl_GetMediaTypeVersion,
    BasePinImpl_GetMediaType
};

static const BaseInputPinFuncTable input_BaseInputFuncTable = {
    DSoundRender_Receive
};


HRESULT DSoundRender_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    PIN_INFO piInput;
    DSoundRenderImpl * pDSoundRender;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;
    
    pDSoundRender = CoTaskMemAlloc(sizeof(DSoundRenderImpl));
    if (!pDSoundRender)
        return E_OUTOFMEMORY;
    ZeroMemory(pDSoundRender, sizeof(DSoundRenderImpl));

    BaseFilter_Init(&pDSoundRender->filter, &DSoundRender_Vtbl, &CLSID_DSoundRender, (DWORD_PTR)(__FILE__ ": DSoundRenderImpl.csFilter"), &BaseFuncTable);

    pDSoundRender->IBasicAudio_vtbl = &IBasicAudio_Vtbl;
    pDSoundRender->IReferenceClock_vtbl = &IReferenceClock_Vtbl;
    pDSoundRender->IAMDirectSound_vtbl = &IAMDirectSound_Vtbl;
    pDSoundRender->IAMFilterMiscFlags_vtbl = &IAMFilterMiscFlags_Vtbl;

    /* construct input pin */
    piInput.dir = PINDIR_INPUT;
    piInput.pFilter = (IBaseFilter *)pDSoundRender;
    lstrcpynW(piInput.achName, wcsInputPinName, sizeof(piInput.achName) / sizeof(piInput.achName[0]));
    hr = BaseInputPin_Construct(&DSoundRender_InputPin_Vtbl, &piInput, &input_BaseFuncTable, &input_BaseInputFuncTable, &pDSoundRender->filter.csFilter, NULL, (IPin **)&pDSoundRender->pInputPin);

    if (SUCCEEDED(hr))
    {
        hr = DirectSoundCreate8(NULL, &pDSoundRender->dsound, NULL);
        if (FAILED(hr))
            ERR("Cannot create Direct Sound object (%x)\n", hr);
        else
            IDirectSound_SetCooperativeLevel(pDSoundRender->dsound, GetDesktopWindow(), DSSCL_PRIORITY);
    }

    if (SUCCEEDED(hr))
    {
        ISeekingPassThru *passthru;
        pDSoundRender->state_change = CreateEventW(NULL, TRUE, TRUE, NULL);
        pDSoundRender->blocked = CreateEventW(NULL, TRUE, TRUE, NULL);
        hr = CoCreateInstance(&CLSID_SeekingPassThru, (IUnknown*)pDSoundRender, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&pDSoundRender->seekthru_unk);
        if (!pDSoundRender->state_change || !pDSoundRender->blocked || FAILED(hr))
        {
            IUnknown_Release((IUnknown *)pDSoundRender);
            return HRESULT_FROM_WIN32(GetLastError());
        }

        IUnknown_QueryInterface(pDSoundRender->seekthru_unk, &IID_ISeekingPassThru, (void**)&passthru);
        ISeekingPassThru_Init(passthru, TRUE, (IPin*)pDSoundRender->pInputPin);
        ISeekingPassThru_Release(passthru);
        *ppv = pDSoundRender;
    }
    else
    {
        if (pDSoundRender->pInputPin)
            IPin_Release((IPin*)pDSoundRender->pInputPin);
        BaseFilterImpl_Release((IBaseFilter*)pDSoundRender);
        CoTaskMemFree(pDSoundRender);
    }

    return hr;
}

static HRESULT WINAPI DSoundRender_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    DSoundRenderImpl *This = (DSoundRenderImpl *)iface;
    TRACE("(%p, %p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IMediaFilter))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IBasicAudio))
        *ppv = &This->IBasicAudio_vtbl;
    else if (IsEqualIID(riid, &IID_IReferenceClock))
        *ppv = &This->IReferenceClock_vtbl;
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
        return IUnknown_QueryInterface(This->seekthru_unk, riid, ppv);
    else if (IsEqualIID(riid, &IID_IAMDirectSound))
        *ppv = &This->IAMDirectSound_vtbl;
    else if (IsEqualIID(riid, &IID_IAMFilterMiscFlags))
        *ppv = &This->IAMFilterMiscFlags_vtbl;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    if (!IsEqualIID(riid, &IID_IPin) && !IsEqualIID(riid, &IID_IVideoWindow))
        FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI DSoundRender_Release(IBaseFilter * iface)
{
    DSoundRenderImpl *This = (DSoundRenderImpl *)iface;
    ULONG refCount = BaseFilterImpl_Release(iface);

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
    {
        IPin *pConnectedTo;

        if (This->dsbuffer)
            IDirectSoundBuffer_Release(This->dsbuffer);
        This->dsbuffer = NULL;
        if (This->dsound)
            IDirectSound_Release(This->dsound);
        This->dsound = NULL;
       
        if (SUCCEEDED(IPin_ConnectedTo((IPin *)This->pInputPin, &pConnectedTo)))
        {
            IPin_Disconnect(pConnectedTo);
            IPin_Release(pConnectedTo);
        }
        IPin_Disconnect((IPin *)This->pInputPin);

        IPin_Release((IPin *)This->pInputPin);

        This->IBasicAudio_vtbl = NULL;
        if (This->seekthru_unk)
            IUnknown_Release(This->seekthru_unk);

        CloseHandle(This->state_change);
        CloseHandle(This->blocked);

        TRACE("Destroying Audio Renderer\n");
        CoTaskMemFree(This);
        
        return 0;
    }
    else
        return refCount;
}

/** IMediaFilter methods **/

static HRESULT WINAPI DSoundRender_Stop(IBaseFilter * iface)
{
    HRESULT hr = S_OK;
    DSoundRenderImpl *This = (DSoundRenderImpl *)iface;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->filter.csFilter);
    {
        DWORD state = 0;
        if (This->dsbuffer)
        {
            hr = IDirectSoundBuffer_GetStatus(This->dsbuffer, &state);
            if (SUCCEEDED(hr))
            {
                if (state & DSBSTATUS_PLAYING)
                    hr = IDirectSoundBuffer_Stop(This->dsbuffer);
            }
        }
        if (SUCCEEDED(hr))
            This->filter.state = State_Stopped;

        /* Complete our transition */
        SetEvent(This->state_change);
        SetEvent(This->blocked);
        MediaSeekingPassThru_ResetMediaTime(This->seekthru_unk);
    }
    LeaveCriticalSection(&This->filter.csFilter);
    
    return hr;
}

static HRESULT WINAPI DSoundRender_Pause(IBaseFilter * iface)
{
    HRESULT hr = S_OK;
    DSoundRenderImpl *This = (DSoundRenderImpl *)iface;
    
    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->filter.csFilter);
    if (This->filter.state != State_Paused)
    {
        DWORD state = 0;
        if (This->filter.state == State_Stopped)
        {
            This->pInputPin->end_of_stream = 0;
        }

        if (This->dsbuffer)
        {
            hr = IDirectSoundBuffer_GetStatus(This->dsbuffer, &state);
            if (SUCCEEDED(hr))
            {
                if (state & DSBSTATUS_PLAYING)
                    hr = IDirectSoundBuffer_Stop(This->dsbuffer);
            }
        }
        if (SUCCEEDED(hr))
            This->filter.state = State_Paused;

        ResetEvent(This->blocked);
        ResetEvent(This->state_change);
    }
    LeaveCriticalSection(&This->filter.csFilter);

    return hr;
}

static HRESULT WINAPI DSoundRender_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    HRESULT hr = S_OK;
    DSoundRenderImpl *This = (DSoundRenderImpl *)iface;

    TRACE("(%p/%p)->(%s)\n", This, iface, wine_dbgstr_longlong(tStart));

    EnterCriticalSection(&This->filter.csFilter);
    if (This->pInputPin->pin.pConnectedTo)
    {
        This->filter.rtStreamStart = tStart;
        if (This->filter.state == State_Paused)
        {
            /* Unblock our thread, state changing from paused to running doesn't need a reset for state change */
            SetEvent(This->blocked);
        }
        else if (This->filter.state == State_Stopped)
        {
            ResetEvent(This->state_change);
            This->pInputPin->end_of_stream = 0;
        }
        ResetEvent(This->blocked);
    } else if (This->filter.filterInfo.pGraph) {
        IMediaEventSink *pEventSink;
        hr = IFilterGraph_QueryInterface(This->filter.filterInfo.pGraph, &IID_IMediaEventSink, (LPVOID*)&pEventSink);
        if (SUCCEEDED(hr))
        {
            hr = IMediaEventSink_Notify(pEventSink, EC_COMPLETE, S_OK, (LONG_PTR)This);
            IMediaEventSink_Release(pEventSink);
        }
        hr = S_OK;
    }
    if (SUCCEEDED(hr))
        This->filter.state = State_Running;
    LeaveCriticalSection(&This->filter.csFilter);

    return hr;
}

static HRESULT WINAPI DSoundRender_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    HRESULT hr;
    DSoundRenderImpl *This = (DSoundRenderImpl *)iface;

    TRACE("(%p/%p)->(%d, %p)\n", This, iface, dwMilliSecsTimeout, pState);

    if (WaitForSingleObject(This->state_change, dwMilliSecsTimeout) == WAIT_TIMEOUT)
        hr = VFW_S_STATE_INTERMEDIATE;
    else
        hr = S_OK;

    BaseFilterImpl_GetState(iface, dwMilliSecsTimeout, pState);

    return hr;
}

/** IBaseFilter implementation **/

static HRESULT WINAPI DSoundRender_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    DSoundRenderImpl *This = (DSoundRenderImpl *)iface;

    TRACE("(%p/%p)->(%s,%p)\n", This, iface, debugstr_w(Id), ppPin);
    
    FIXME("DSoundRender::FindPin(...)\n");

    /* FIXME: critical section */

    return E_NOTIMPL;
}

static const IBaseFilterVtbl DSoundRender_Vtbl =
{
    DSoundRender_QueryInterface,
    BaseFilterImpl_AddRef,
    DSoundRender_Release,
    BaseFilterImpl_GetClassID,
    DSoundRender_Stop,
    DSoundRender_Pause,
    DSoundRender_Run,
    DSoundRender_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    DSoundRender_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

static HRESULT WINAPI DSoundRender_InputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    BaseInputPin *This = (BaseInputPin *)iface;
    PIN_DIRECTION pindirReceive;
    DSoundRenderImpl *DSImpl;
    HRESULT hr = S_OK;

    TRACE("(%p)->(%p, %p)\n", This, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    EnterCriticalSection(This->pin.pCritSec);
    {
        DSImpl = (DSoundRenderImpl*)This->pin.pinInfo.pFilter;
        DSImpl->rtLastStop = -1;

        if (This->pin.pConnectedTo)
            hr = VFW_E_ALREADY_CONNECTED;

        if (SUCCEEDED(hr) && This->pin.pFuncsTable->pfnCheckMediaType((BasePin*)This, pmt) != S_OK)
            hr = VFW_E_TYPE_NOT_ACCEPTED;

        if (SUCCEEDED(hr))
        {
            IPin_QueryDirection(pReceivePin, &pindirReceive);

            if (pindirReceive != PINDIR_OUTPUT)
            {
                ERR("Can't connect from non-output pin\n");
                hr = VFW_E_INVALID_DIRECTION;
            }
        }

        if (SUCCEEDED(hr))
        {
            WAVEFORMATEX *format;
            DSBUFFERDESC buf_desc;

            TRACE("MajorType %s\n", debugstr_guid(&pmt->majortype));
            TRACE("SubType %s\n", debugstr_guid(&pmt->subtype));
            TRACE("Format %s\n", debugstr_guid(&pmt->formattype));
            TRACE("Size %d\n", pmt->cbFormat);

            format = (WAVEFORMATEX*)pmt->pbFormat;

            DSImpl->buf_size = format->nAvgBytesPerSec;

            memset(&buf_desc,0,sizeof(DSBUFFERDESC));
            buf_desc.dwSize = sizeof(DSBUFFERDESC);
            buf_desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN |
                               DSBCAPS_CTRLFREQUENCY |
                               DSBCAPS_GETCURRENTPOSITION2;
            buf_desc.dwBufferBytes = DSImpl->buf_size;
            buf_desc.lpwfxFormat = format;
            hr = IDirectSound_CreateSoundBuffer(DSImpl->dsound, &buf_desc, &DSImpl->dsbuffer, NULL);
            if (FAILED(hr))
                ERR("Can't create sound buffer (%x)\n", hr);
        }

        if (SUCCEEDED(hr))
        {
            hr = IDirectSoundBuffer_SetVolume(DSImpl->dsbuffer, DSImpl->volume);
            if (FAILED(hr))
                ERR("Can't set volume to %d (%x)\n", DSImpl->volume, hr);

            hr = IDirectSoundBuffer_SetPan(DSImpl->dsbuffer, DSImpl->pan);
            if (FAILED(hr))
                ERR("Can't set pan to %d (%x)\n", DSImpl->pan, hr);

            DSImpl->write_pos = 0;
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            CopyMediaType(&This->pin.mtCurrent, pmt);
            This->pin.pConnectedTo = pReceivePin;
            IPin_AddRef(pReceivePin);
        }
        else if (hr != VFW_E_ALREADY_CONNECTED)
        {
            if (DSImpl->dsbuffer)
                IDirectSoundBuffer_Release(DSImpl->dsbuffer);
            DSImpl->dsbuffer = NULL;
        }
    }
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

static HRESULT WINAPI DSoundRender_InputPin_Disconnect(IPin * iface)
{
    BasePin *This = (BasePin*)iface;
    DSoundRenderImpl *DSImpl;

    TRACE("(%p)->()\n", iface);

    DSImpl = (DSoundRenderImpl*)This->pinInfo.pFilter;
    if (DSImpl->dsbuffer)
        IDirectSoundBuffer_Release(DSImpl->dsbuffer);
    DSImpl->dsbuffer = NULL;

    return BasePinImpl_Disconnect(iface);
}

static HRESULT WINAPI DSoundRender_InputPin_EndOfStream(IPin * iface)
{
    BaseInputPin* This = (BaseInputPin*)iface;
    DSoundRenderImpl *me = (DSoundRenderImpl*)This->pin.pinInfo.pFilter;
    IMediaEventSink* pEventSink;
    BYTE *silence;
    HRESULT hr;

    EnterCriticalSection(This->pin.pCritSec);

    TRACE("(%p/%p)->()\n", This, iface);
    hr = BaseInputPinImpl_EndOfStream(iface);
    if (hr != S_OK)
    {
        ERR("%08x\n", hr);
        LeaveCriticalSection(This->pin.pCritSec);
        return hr;
    }

    silence = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, me->buf_size);
    if (silence)
    {
        memset(silence, 0, me->buf_size);
        DSoundRender_SendSampleData(me, silence, me->buf_size);
        HeapFree(GetProcessHeap(), 0, silence);
    }

    if (me->filter.filterInfo.pGraph)
    {
        hr = IFilterGraph_QueryInterface(me->filter.filterInfo.pGraph, &IID_IMediaEventSink, (LPVOID*)&pEventSink);
        if (SUCCEEDED(hr))
        {
            hr = IMediaEventSink_Notify(pEventSink, EC_COMPLETE, S_OK, (LONG_PTR)me);
            IMediaEventSink_Release(pEventSink);
        }
    }
    MediaSeekingPassThru_EOS(me->seekthru_unk);
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

static HRESULT WINAPI DSoundRender_InputPin_BeginFlush(IPin * iface)
{
    BaseInputPin *This = (BaseInputPin *)iface;
    DSoundRenderImpl *pFilter = (DSoundRenderImpl *)This->pin.pinInfo.pFilter;
    HRESULT hr;

    TRACE("\n");

    EnterCriticalSection(This->pin.pCritSec);
    hr = BaseInputPinImpl_BeginFlush(iface);
    SetEvent(pFilter->blocked);
    LeaveCriticalSection(This->pin.pCritSec);

    return hr;
}

static HRESULT WINAPI DSoundRender_InputPin_EndFlush(IPin * iface)
{
    BaseInputPin *This = (BaseInputPin *)iface;
    DSoundRenderImpl *pFilter = (DSoundRenderImpl *)This->pin.pinInfo.pFilter;
    HRESULT hr;

    TRACE("\n");

    EnterCriticalSection(This->pin.pCritSec);
    if (pFilter->in_loop) {
        ResetEvent(pFilter->state_change);
        LeaveCriticalSection(This->pin.pCritSec);
        WaitForSingleObject(pFilter->state_change, -1);
        EnterCriticalSection(This->pin.pCritSec);
    }
    if (pFilter->filter.state != State_Stopped)
        ResetEvent(pFilter->blocked);

    if (pFilter->dsbuffer)
    {
        LPBYTE buffer;
        DWORD size;
        IDirectSoundBuffer_Stop(pFilter->dsbuffer);

        /* Force a reset */
        IDirectSoundBuffer_SetCurrentPosition(pFilter->dsbuffer, 0);
        pFilter->write_pos = pFilter->last_play_pos = 0;
        ++pFilter->play_loops;
        pFilter->write_loops = pFilter->play_loops;

        IDirectSoundBuffer_Lock(pFilter->dsbuffer, 0, 0, (LPVOID *)&buffer, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER);
        memset(buffer, 0, size);
        IDirectSoundBuffer_Unlock(pFilter->dsbuffer, buffer, size, NULL, 0);
    }
    hr = BaseInputPinImpl_EndFlush(iface);
    LeaveCriticalSection(This->pin.pCritSec);
    MediaSeekingPassThru_ResetMediaTime(pFilter->seekthru_unk);

    return hr;
}

static const IPinVtbl DSoundRender_InputPin_Vtbl =
{
    BaseInputPinImpl_QueryInterface,
    BasePinImpl_AddRef,
    BaseInputPinImpl_Release,
    BaseInputPinImpl_Connect,
    DSoundRender_InputPin_ReceiveConnection,
    DSoundRender_InputPin_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BaseInputPinImpl_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    DSoundRender_InputPin_EndOfStream,
    DSoundRender_InputPin_BeginFlush,
    DSoundRender_InputPin_EndFlush,
    BaseInputPinImpl_NewSegment
};

/*** IUnknown methods ***/
static HRESULT WINAPI Basicaudio_QueryInterface(IBasicAudio *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    ICOM_THIS_MULTI(DSoundRenderImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return DSoundRender_QueryInterface((IBaseFilter*)This, riid, ppvObj);
}

static ULONG WINAPI Basicaudio_AddRef(IBasicAudio *iface) {
    ICOM_THIS_MULTI(DSoundRenderImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return BaseFilterImpl_AddRef((IBaseFilter*)This);
}

static ULONG WINAPI Basicaudio_Release(IBasicAudio *iface) {
    ICOM_THIS_MULTI(DSoundRenderImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return DSoundRender_Release((IBaseFilter*)This);
}

/*** IDispatch methods ***/
static HRESULT WINAPI Basicaudio_GetTypeInfoCount(IBasicAudio *iface,
						  UINT*pctinfo) {
    ICOM_THIS_MULTI(DSoundRenderImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI Basicaudio_GetTypeInfo(IBasicAudio *iface,
					     UINT iTInfo,
					     LCID lcid,
					     ITypeInfo**ppTInfo) {
    ICOM_THIS_MULTI(DSoundRenderImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%d, %d, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI Basicaudio_GetIDsOfNames(IBasicAudio *iface,
					       REFIID riid,
					       LPOLESTR*rgszNames,
					       UINT cNames,
					       LCID lcid,
					       DISPID*rgDispId) {
    ICOM_THIS_MULTI(DSoundRenderImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p, %d, %d, %p): stub !!!\n", This, iface, debugstr_guid(riid), riid, rgszNames, cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI Basicaudio_Invoke(IBasicAudio *iface,
					DISPID dispIdMember,
					REFIID riid,
					LCID lcid,
					WORD wFlags,
					DISPPARAMS*pDispParams,
					VARIANT*pVarResult,
					EXCEPINFO*pExepInfo,
					UINT*puArgErr) {
    ICOM_THIS_MULTI(DSoundRenderImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%d, %s (%p), %d, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember, debugstr_guid(riid), riid, lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

/*** IBasicAudio methods ***/
static HRESULT WINAPI Basicaudio_put_Volume(IBasicAudio *iface,
                                            LONG lVolume) {
    ICOM_THIS_MULTI(DSoundRenderImpl, IBasicAudio_vtbl, iface);

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
    ICOM_THIS_MULTI(DSoundRenderImpl, IBasicAudio_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, plVolume);

    if (!plVolume)
        return E_POINTER;

    *plVolume = This->volume;
    return S_OK;
}

static HRESULT WINAPI Basicaudio_put_Balance(IBasicAudio *iface,
                                             LONG lBalance) {
    ICOM_THIS_MULTI(DSoundRenderImpl, IBasicAudio_vtbl, iface);

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
    ICOM_THIS_MULTI(DSoundRenderImpl, IBasicAudio_vtbl, iface);

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
    Basicaudio_GetTypeInfoCount,
    Basicaudio_GetTypeInfo,
    Basicaudio_GetIDsOfNames,
    Basicaudio_Invoke,
    Basicaudio_put_Volume,
    Basicaudio_get_Volume,
    Basicaudio_put_Balance,
    Basicaudio_get_Balance
};


/*** IUnknown methods ***/
static HRESULT WINAPI ReferenceClock_QueryInterface(IReferenceClock *iface,
						REFIID riid,
						LPVOID*ppvObj)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IReferenceClock_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return DSoundRender_QueryInterface((IBaseFilter*)This, riid, ppvObj);
}

static ULONG WINAPI ReferenceClock_AddRef(IReferenceClock *iface)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IReferenceClock_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return BaseFilterImpl_AddRef((IBaseFilter*)This);
}

static ULONG WINAPI ReferenceClock_Release(IReferenceClock *iface)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IReferenceClock_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return DSoundRender_Release((IBaseFilter*)This);
}

/*** IReferenceClock methods ***/
static HRESULT WINAPI ReferenceClock_GetTime(IReferenceClock *iface,
                                             REFERENCE_TIME *pTime)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IReferenceClock_vtbl, iface);
    HRESULT hr = E_FAIL;
    DWORD play_pos;

    TRACE("(%p/%p)->(%p)\n", This, iface, pTime);

    if (This->dsbuffer)
        hr = DSoundRender_GetPos(This, &play_pos, pTime);
    if (FAILED(hr))
        ERR("Could not get reference time (%x)!\n", hr);

    return hr;
}

static HRESULT WINAPI ReferenceClock_AdviseTime(IReferenceClock *iface,
                                                REFERENCE_TIME rtBaseTime,
                                                REFERENCE_TIME rtStreamTime,
                                                HEVENT hEvent,
                                                DWORD_PTR *pdwAdviseCookie)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IReferenceClock_vtbl, iface);

    FIXME("(%p/%p)->(%s, %s, %p, %p): stub!\n", This, iface, wine_dbgstr_longlong(rtBaseTime), wine_dbgstr_longlong(rtStreamTime), (void*)hEvent, pdwAdviseCookie);

    return E_NOTIMPL;
}

static HRESULT WINAPI ReferenceClock_AdvisePeriodic(IReferenceClock *iface,
                                                    REFERENCE_TIME rtBaseTime,
                                                    REFERENCE_TIME rtStreamTime,
                                                    HSEMAPHORE hSemaphore,
                                                    DWORD_PTR *pdwAdviseCookie)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IReferenceClock_vtbl, iface);

    FIXME("(%p/%p)->(%s, %s, %p, %p): stub!\n", This, iface, wine_dbgstr_longlong(rtBaseTime), wine_dbgstr_longlong(rtStreamTime), (void*)hSemaphore, pdwAdviseCookie);

    return E_NOTIMPL;
}

static HRESULT WINAPI ReferenceClock_Unadvise(IReferenceClock *iface,
                                              DWORD_PTR dwAdviseCookie)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IReferenceClock_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub!\n", This, iface, (void*)dwAdviseCookie);

    return S_FALSE;
}

static const IReferenceClockVtbl IReferenceClock_Vtbl =
{
    ReferenceClock_QueryInterface,
    ReferenceClock_AddRef,
    ReferenceClock_Release,
    ReferenceClock_GetTime,
    ReferenceClock_AdviseTime,
    ReferenceClock_AdvisePeriodic,
    ReferenceClock_Unadvise
};

/*** IUnknown methods ***/
static HRESULT WINAPI AMDirectSound_QueryInterface(IAMDirectSound *iface,
						REFIID riid,
						LPVOID*ppvObj)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IAMDirectSound_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return DSoundRender_QueryInterface((IBaseFilter*)This, riid, ppvObj);
}

static ULONG WINAPI AMDirectSound_AddRef(IAMDirectSound *iface)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IAMDirectSound_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return BaseFilterImpl_AddRef((IBaseFilter*)This);
}

static ULONG WINAPI AMDirectSound_Release(IAMDirectSound *iface)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IAMDirectSound_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return DSoundRender_Release((IBaseFilter*)This);
}

/*** IAMDirectSound methods ***/
static HRESULT WINAPI AMDirectSound_GetDirectSoundInterface(IAMDirectSound *iface,  IDirectSound **ds)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IAMDirectSound_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, ds);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_GetPrimaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer **buf)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IAMDirectSound_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_GetSecondaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer **buf)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IAMDirectSound_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_ReleaseDirectSoundInterface(IAMDirectSound *iface, IDirectSound *ds)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IAMDirectSound_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, ds);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_ReleasePrimaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer *buf)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IAMDirectSound_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_ReleaseSecondaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer *buf)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IAMDirectSound_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_SetFocusWindow(IAMDirectSound *iface, HWND hwnd, BOOL bgsilent)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IAMDirectSound_vtbl, iface);

    FIXME("(%p/%p)->(%p,%d): stub\n", This, iface, hwnd, bgsilent);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_GetFocusWindow(IAMDirectSound *iface, HWND hwnd)
{
    ICOM_THIS_MULTI(DSoundRenderImpl, IAMDirectSound_vtbl, iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, hwnd);

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

static DSoundRenderImpl *from_IAMFilterMiscFlags(IAMFilterMiscFlags *iface) {
    return (DSoundRenderImpl*)((char*)iface - offsetof(DSoundRenderImpl, IAMFilterMiscFlags_vtbl));
}

static HRESULT WINAPI AMFilterMiscFlags_QueryInterface(IAMFilterMiscFlags *iface, const REFIID riid, void **ppv) {
    DSoundRenderImpl *This = from_IAMFilterMiscFlags(iface);
    return IUnknown_QueryInterface((IUnknown*)This, riid, ppv);
}

static ULONG WINAPI AMFilterMiscFlags_AddRef(IAMFilterMiscFlags *iface) {
    DSoundRenderImpl *This = from_IAMFilterMiscFlags(iface);
    return IUnknown_AddRef((IUnknown*)This);
}

static ULONG WINAPI AMFilterMiscFlags_Release(IAMFilterMiscFlags *iface) {
    DSoundRenderImpl *This = from_IAMFilterMiscFlags(iface);
    return IUnknown_Release((IUnknown*)This);
}

static ULONG WINAPI AMFilterMiscFlags_GetMiscFlags(IAMFilterMiscFlags *iface) {
    return AM_FILTER_MISC_FLAGS_IS_RENDERER;
}

static const IAMFilterMiscFlagsVtbl IAMFilterMiscFlags_Vtbl = {
    AMFilterMiscFlags_QueryInterface,
    AMFilterMiscFlags_AddRef,
    AMFilterMiscFlags_Release,
    AMFilterMiscFlags_GetMiscFlags
};
