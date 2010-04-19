/*
 * Copyright 2010 Maarten Lankhorst for Codeweavers
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
#define CINTERFACE
#define COBJMACROS
#include "config.h"

#include <stdarg.h>
#ifdef HAVE_AL_AL_H
#include <AL/al.h>
#include <AL/alc.h>
#elif defined(HAVE_OPENAL_AL_H)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "ole2.h"
#include "mmdeviceapi.h"
#include "dshow.h"
#include "dsound.h"
#include "audioclient.h"
#include "endpointvolume.h"
#include "audiopolicy.h"

#include "mmdevapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

#ifdef HAVE_OPENAL

typedef struct ACRender ACRender;
typedef struct ACCapture ACCapture;
typedef struct ACSession ACSession;
typedef struct ASVolume ASVolume;
typedef struct AClock AClock;

typedef struct ACImpl {
    const IAudioClientVtbl *lpVtbl;
    LONG ref;

    MMDevice *parent;
    BOOL init, running;
    CRITICAL_SECTION *crst;
    HANDLE handle;
    DWORD locked, flags, bufsize, pad, padpartial, ofs, psize;
    BYTE *buffer;
    WAVEFORMATEX *pwfx;
    ALuint source;
    INT64 frameswritten;
    REFERENCE_TIME laststamp;
    HANDLE timer_id;
    ALCdevice *capdev;
    ALint format;
} ACImpl;

static const IAudioClientVtbl ACImpl_Vtbl;

static ALint get_format(WAVEFORMATEX *in)
{
    FIXME("stub\n");
    return AL_FORMAT_STEREO_FLOAT32;
}

static REFERENCE_TIME gettime(void) {
    LARGE_INTEGER stamp, freq;
    QueryPerformanceCounter(&stamp);
    QueryPerformanceFrequency(&freq);
    return (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
}

HRESULT AudioClient_Create(MMDevice *parent, IAudioClient **ppv)
{
    ACImpl *This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This));
    *ppv = (IAudioClient*)This;
    if (!*ppv)
        return E_OUTOFMEMORY;
    This->crst = &parent->crst;
    This->lpVtbl = &ACImpl_Vtbl;
    This->ref = 1;
    This->parent = parent;
    return S_OK;
}

static void AudioClient_Destroy(ACImpl *This)
{
    if (This->timer_id)
        DeleteTimerQueueTimer(NULL, This->timer_id, INVALID_HANDLE_VALUE);
    if (This->parent->flow == eRender && This->init) {
        setALContext(This->parent->ctx);
        IAudioClient_Stop((IAudioClient*)This);
        IAudioClient_Reset((IAudioClient*)This);
        palDeleteSources(1, &This->source);
        getALError();
        popALContext();
    }
    if (This->capdev)
        palcCaptureCloseDevice(This->capdev);
    HeapFree(GetProcessHeap(), 0, This->pwfx);
    HeapFree(GetProcessHeap(), 0, This->buffer);
    HeapFree(GetProcessHeap(), 0, This);
}

static void CALLBACK AC_tick(void *data, BOOLEAN fired)
{
    ACImpl *This = data;
    DWORD pad;

    EnterCriticalSection(This->crst);
    if (This->running)
        IAudioClient_GetCurrentPadding((IAudioClient*)This, &pad);
    LeaveCriticalSection(This->crst);
}

/* Open device and set/update internal mixing format based on information
 * openal provides us. if the device cannot be opened, assume 48khz
 * Guessing the frequency is harmless, since if GetMixFormat fails to open
 * the device, then Initialize will likely fail as well
 */
static HRESULT AC_OpenRenderAL(ACImpl *This)
{
    char alname[MAX_PATH];
    MMDevice *cur = This->parent;

    alname[sizeof(alname)-1] = 0;
    if (cur->device)
        return cur->ctx ? S_OK : AUDCLNT_E_SERVICE_NOT_RUNNING;

    WideCharToMultiByte(CP_UNIXCP, 0, cur->alname, -1,
                        alname, sizeof(alname)/sizeof(*alname)-1, NULL, NULL);
    cur->device = palcOpenDevice(alname);
    if (!cur->device) {
        ALCenum err = palcGetError(NULL);
        FIXME("Could not open device %s: 0x%04x\n", alname, err);
        return AUDCLNT_E_DEVICE_IN_USE;
    }
    cur->ctx = palcCreateContext(cur->device, NULL);
    if (!cur->ctx) {
        ALCenum err = palcGetError(cur->device);
        FIXME("Could not create context: 0x%04x\n", err);
        return AUDCLNT_E_SERVICE_NOT_RUNNING;
    }
    if (!cur->device)
        return AUDCLNT_E_DEVICE_IN_USE;
    return S_OK;
}

static HRESULT AC_OpenCaptureAL(ACImpl *This)
{
    char alname[MAX_PATH];
    ALint freq, size;

    freq = This->pwfx->nSamplesPerSec;
    size = This->bufsize;

    alname[sizeof(alname)-1] = 0;
    if (This->capdev) {
        FIXME("Attempting to open device while already open\n");
        return S_OK;
    }
    WideCharToMultiByte(CP_UNIXCP, 0, This->parent->alname, -1,
                        alname, sizeof(alname)/sizeof(*alname)-1, NULL, NULL);
    This->capdev = palcCaptureOpenDevice(alname, freq, This->format, size);
    if (!This->capdev) {
        ALCenum err = palcGetError(NULL);
        FIXME("Could not open device %s with buf size %u: 0x%04x\n",
              alname, This->bufsize, err);
        return AUDCLNT_E_DEVICE_IN_USE;
    }
    return S_OK;
}

static HRESULT WINAPI AC_QueryInterface(IAudioClient *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IAudioClient))
        *ppv = iface;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AC_AddRef(IAudioClient *iface)
{
    ACImpl *This = (ACImpl*)iface;
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI AC_Release(IAudioClient *iface)
{
    ACImpl *This = (ACImpl*)iface;
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    if (!ref)
        AudioClient_Destroy(This);
    return ref;
}

static HRESULT WINAPI AC_Initialize(IAudioClient *iface, AUDCLNT_SHAREMODE mode, DWORD flags, REFERENCE_TIME duration, REFERENCE_TIME period, const WAVEFORMATEX *pwfx, const GUID *sessionguid)
{
    ACImpl *This = (ACImpl*)iface;
    HRESULT hr = S_OK;
    WAVEFORMATEX *pwfx2;
    REFERENCE_TIME time, bufsize;

    TRACE("(%p)->(%x,%x,%u,%u,%p,%s)\n", This, mode, flags, (int)duration, (int)period, pwfx, debugstr_guid(sessionguid));
    if (This->init)
        return AUDCLNT_E_ALREADY_INITIALIZED;
    if (mode != AUDCLNT_SHAREMODE_SHARED
        && mode != AUDCLNT_SHAREMODE_EXCLUSIVE) {
        WARN("Unknown mode %x\n", mode);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if (flags & ~(AUDCLNT_STREAMFLAGS_CROSSPROCESS
                  |AUDCLNT_STREAMFLAGS_LOOPBACK
                  |AUDCLNT_STREAMFLAGS_EVENTCALLBACK
                  |AUDCLNT_STREAMFLAGS_NOPERSIST
                  |AUDCLNT_STREAMFLAGS_RATEADJUST
                  |AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED
                  |AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE
                  |AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED)) {
        WARN("Unknown flags 0x%08x\n", flags);
        return E_INVALIDARG;
    }
    if (flags)
        WARN("Flags 0x%08x ignored\n", flags);
    if (!pwfx)
        return E_POINTER;
    if (sessionguid)
        WARN("Session guid %s ignored\n", debugstr_guid(sessionguid));

    hr = IAudioClient_IsFormatSupported(iface, mode, pwfx, &pwfx2);
    CoTaskMemFree(pwfx2);
    if (FAILED(hr) || pwfx2)
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    EnterCriticalSection(This->crst);
    HeapFree(GetProcessHeap(), 0, This->pwfx);
    This->pwfx = HeapAlloc(GetProcessHeap(), 0, sizeof(*pwfx) + pwfx->cbSize);
    if (!This->pwfx) {
        hr = E_OUTOFMEMORY;
        goto out;
    }
    memcpy(This->pwfx, pwfx, sizeof(*pwfx) + pwfx->cbSize);

    hr = IAudioClient_GetDevicePeriod(iface, &time, NULL);
    if (FAILED(hr))
        goto out;

    This->psize = (DWORD64)This->pwfx->nSamplesPerSec * time / (DWORD64)10000000;
    if (duration > 20000000)
        duration = 20000000;

    bufsize = duration / time * This->psize;
    if (duration % time)
        bufsize += This->psize;
    This->bufsize = bufsize;
    This->psize *= This->pwfx->nBlockAlign;
    bufsize *= pwfx->nBlockAlign;

    This->format = get_format(This->pwfx);
    if (This->parent->flow == eRender) {
        char silence[32];
        ALuint buf = 0, towrite;

        hr = AC_OpenRenderAL(This);
        if (FAILED(hr))
            goto out;

        /* Test the returned format */
        towrite = sizeof(silence);
        towrite -= towrite % This->pwfx->nBlockAlign;
        if (This->pwfx->wBitsPerSample != 8)
            memset(silence, 0, sizeof(silence));
        else
            memset(silence, 128, sizeof(silence));
        setALContext(This->parent->ctx);
        getALError();
        palGenBuffers(1, &buf);
        palBufferData(buf, This->format, silence, towrite, This->pwfx->nSamplesPerSec);
        palDeleteBuffers(1, &buf);
        if (palGetError())
            hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
        else if (!This->source) {
            palGenSources(1, &This->source);
            palSourcei(This->source, AL_LOOPING, AL_FALSE);
            getALError();
        }
        popALContext();
    }
    else
        hr = AC_OpenCaptureAL(This);

    if (FAILED(hr))
        goto out;

    This->buffer = HeapAlloc(GetProcessHeap(), 0, bufsize);
    if (!This->buffer) {
        hr = E_OUTOFMEMORY;
        goto out;
    }
    This->flags = flags;
    This->handle = NULL;
    This->running = FALSE;
    This->init = TRUE;
out:
    LeaveCriticalSection(This->crst);
    return hr;
}

static HRESULT WINAPI AC_GetBufferSize(IAudioClient *iface, UINT32 *frames)
{
    ACImpl *This = (ACImpl*)iface;
    TRACE("(%p)->(%p)\n", This, frames);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (!frames)
        return E_POINTER;
    *frames = This->bufsize;
    return S_OK;
}

static HRESULT WINAPI AC_GetStreamLatency(IAudioClient *iface, REFERENCE_TIME *latency)
{
    ACImpl *This = (ACImpl*)iface;
    TRACE("(%p)->(%p)\n", This, latency);

    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;

    if (!latency)
        return E_POINTER;

    *latency = 50000;

    return S_OK;
}

static HRESULT WINAPI AC_GetCurrentPadding(IAudioClient *iface, UINT32 *numpad)
{
    ACImpl *This = (ACImpl*)iface;
    ALint avail = 0;

    TRACE("(%p)->(%p)\n", This, numpad);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (!numpad)
        return E_POINTER;
    EnterCriticalSection(This->crst);
    if (This->parent->flow == eRender) {
        UINT64 played = 0;
        ALint state, padpart;
        setALContext(This->parent->ctx);

        palGetSourcei(This->source, AL_BYTE_OFFSET, &padpart);
        palGetSourcei(This->source, AL_SOURCE_STATE, &state);
        padpart /= This->pwfx->nBlockAlign;
        if (state == AL_STOPPED && This->running)
            padpart = This->pad;
        if (This->running && This->padpartial != padpart) {
            This->padpartial = padpart;
            This->laststamp = gettime();
#if 0 /* Manipulative lie */
        } else if (This->running) {
            ALint size = This->pad - padpart;
            if (size > This->psize)
                size = This->psize;
            played = (gettime() - This->laststamp)*8;
            played = played * This->pwfx->nSamplesPerSec / 10000000;
            if (played > size)
                played = size;
#endif
        }
        *numpad = This->pad - This->padpartial - played;
        if (This->handle && *numpad + This->psize <= This->bufsize)
            SetEvent(This->handle);
        getALError();
        popALContext();
    } else {
        DWORD block = This->pwfx->nBlockAlign;
        DWORD psize = This->psize / block;
        palcGetIntegerv(This->capdev, ALC_CAPTURE_SAMPLES, 1, &avail);
        if (avail) {
            DWORD ofs = This->ofs + This->pad;
            BYTE *buf1;
            ofs %= This->bufsize;
            buf1 = This->buffer + (ofs * block);
            This->laststamp = gettime();
            if (This->handle)
                SetEvent(This->handle);

            if (ofs + avail <= This->bufsize)
                palcCaptureSamples(This->capdev, buf1, avail);
            else {
                DWORD part1 = This->bufsize - This->ofs;
                palcCaptureSamples(This->capdev, buf1, part1);
                palcCaptureSamples(This->capdev, This->buffer, avail - part1);
            }
            This->pad += avail;
            This->frameswritten += avail;
            /* Increase ofs if the app forgets to read */
            if (This->pad > This->bufsize) {
                DWORD rest;
                WARN("Overflowed! %u bytes\n", This->pad - This->bufsize);
                This->ofs += This->pad - This->bufsize;
                rest = This->ofs % psize;
                if (rest)
                    This->ofs += psize - rest;
                This->ofs %= This->bufsize;
                This->pad = This->bufsize - rest;
            }
        }
        if (This->pad >= psize)
            *numpad = psize;
        else
            *numpad = 0;
    }
    LeaveCriticalSection(This->crst);

    TRACE("%u queued\n", *numpad);
    return S_OK;
}

static HRESULT WINAPI AC_IsFormatSupported(IAudioClient *iface, AUDCLNT_SHAREMODE mode, const WAVEFORMATEX *pwfx, WAVEFORMATEX **outpwfx)
{
    ACImpl *This = (ACImpl*)iface;
    TRACE("(%p)->(%x,%p,%p)\n", This, mode, pwfx, outpwfx);
    if (!pwfx)
        return E_POINTER;

    if (mode == AUDCLNT_SHAREMODE_SHARED && !outpwfx)
        return E_POINTER;
    if (mode != AUDCLNT_SHAREMODE_SHARED
        && mode != AUDCLNT_SHAREMODE_EXCLUSIVE) {
        WARN("Unknown mode %x\n", mode);
        return E_INVALIDARG;
    }
    if (pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE
        && pwfx->wFormatTag != WAVE_FORMAT_PCM)
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    if (pwfx->nSamplesPerSec < 8000
        || pwfx->nSamplesPerSec > 192000)
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    if (pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE
        || !IsEqualIID(&((WAVEFORMATEXTENSIBLE*)pwfx)->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
        if (pwfx->wBitsPerSample > 16)
            return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }
    if (outpwfx)
        *outpwfx = NULL;
    return S_OK;
}

static HRESULT WINAPI AC_GetMixFormat(IAudioClient *iface, WAVEFORMATEX **pwfx)
{
    ACImpl *This = (ACImpl*)iface;
    PROPVARIANT pv = { VT_EMPTY };
    HRESULT hr = S_OK;

    TRACE("(%p)->(%p)\n", This, pwfx);
    if (!pwfx)
        return E_POINTER;

    hr = MMDevice_GetPropValue(&This->parent->devguid, This->parent->flow,
                               &PKEY_AudioEngine_DeviceFormat, &pv);
    *pwfx = (WAVEFORMATEX*)pv.u.blob.pBlobData;
    if (SUCCEEDED(hr) && pv.vt == VT_EMPTY)
        return E_FAIL;

    TRACE("Returning 0x%08x\n", hr);
    return hr;
}

static HRESULT WINAPI AC_GetDevicePeriod(IAudioClient *iface, REFERENCE_TIME *defperiod, REFERENCE_TIME *minperiod)
{
    ACImpl *This = (ACImpl*)iface;

    TRACE("(%p)->(%p)\n", This, minperiod);
    if (!defperiod && !minperiod)
        return E_POINTER;

    if (minperiod)
        *minperiod = 30000;
    if (defperiod)
        *defperiod = 200000;
    return S_OK;
}

static HRESULT WINAPI AC_Start(IAudioClient *iface)
{
    ACImpl *This = (ACImpl*)iface;
    HRESULT hr;
    REFERENCE_TIME refresh;

    TRACE("(%p)\n", This);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (This->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) {
        if (!This->handle)
            return AUDCLNT_E_EVENTHANDLE_NOT_SET;
        FIXME("Event handles not fully tested\n");
    }
    EnterCriticalSection(This->crst);
    if (This->running) {
        hr = AUDCLNT_E_NOT_STOPPED;
        goto out;
    }
    if (This->parent->flow == eRender) {
        setALContext(This->parent->ctx);
        palSourcePlay(This->source);
        getALError();
        popALContext();
    }
    else
        palcCaptureStart(This->capdev);

    AC_GetDevicePeriod(iface, &refresh, NULL);
    if (!This->timer_id && This->handle)
        CreateTimerQueueTimer(&This->timer_id, NULL, AC_tick, This,
                              refresh / 20000, refresh / 20000,
                              WT_EXECUTEINTIMERTHREAD);
    /* Set to 0, otherwise risk running the clock backwards
     * This will cause AudioClock::GetPosition to return the maximum
     * possible value for the current buffer
     */
    This->laststamp = 0;
    This->running = TRUE;
    hr = S_OK;
out:
    LeaveCriticalSection(This->crst);
    return hr;
}

static HRESULT WINAPI AC_Stop(IAudioClient *iface)
{
    ACImpl *This = (ACImpl*)iface;
    HANDLE timer_id;
    TRACE("(%p)\n", This);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (!This->running)
        return S_FALSE;
    EnterCriticalSection(This->crst);
    if (This->parent->flow == eRender) {
        ALint state;
        setALContext(This->parent->ctx);
        This->running = FALSE;
        palSourcePause(This->source);
        while (1) {
            state = AL_STOPPED;
            palGetSourcei(This->source, AL_SOURCE_STATE, &state);
            if (state != AL_PLAYING)
                break;
            Sleep(1);
        }
        getALError();
        popALContext();
    }
    else
        palcCaptureStop(This->capdev);
    timer_id = This->timer_id;
    This->timer_id = 0;
    LeaveCriticalSection(This->crst);
    if (timer_id)
        DeleteTimerQueueTimer(NULL, timer_id, INVALID_HANDLE_VALUE);
    return S_OK;
}

static HRESULT WINAPI AC_Reset(IAudioClient *iface)
{
    ACImpl *This = (ACImpl*)iface;
    HRESULT hr = S_OK;
    TRACE("(%p)\n", This);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (This->running)
        return AUDCLNT_E_NOT_STOPPED;
    EnterCriticalSection(This->crst);
    if (This->locked) {
        hr = AUDCLNT_E_BUFFER_OPERATION_PENDING;
        goto out;
    }
    if (This->parent->flow == eRender) {
        ALuint buf;
        ALint n = 0;
        setALContext(This->parent->ctx);
        palSourceStop(This->source);
        palGetSourcei(This->source, AL_BUFFERS_PROCESSED, &n);
        while (n--) {
            palSourceUnqueueBuffers(This->source, 1, &buf);
            palDeleteBuffers(1, &buf);
        }
        getALError();
        popALContext();
    } else {
        ALint avail = 0;
        palcGetIntegerv(This->capdev, ALC_CAPTURE_SAMPLES, 1, &avail);
        if (avail)
            palcCaptureSamples(This->capdev, This->buffer, avail);
    }
    This->pad = This->padpartial = 0;
    This->ofs = 0;
    This->frameswritten = 0;
out:
    LeaveCriticalSection(This->crst);
    return hr;
}

static HRESULT WINAPI AC_SetEventHandle(IAudioClient *iface, HANDLE handle)
{
    ACImpl *This = (ACImpl*)iface;
    TRACE("(%p)\n", This);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (!handle)
        return E_INVALIDARG;
    if (!(This->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK))
        return AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED;
    This->handle = handle;
    return S_OK;
}

static HRESULT WINAPI AC_GetService(IAudioClient *iface, REFIID riid, void **ppv)
{
    ACImpl *This = (ACImpl*)iface;
    HRESULT hr = S_OK;
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppv);
    if (!This->init)
        return AUDCLNT_E_NOT_INITIALIZED;
    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (FAILED(hr))
        return hr;

    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("stub %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static const IAudioClientVtbl ACImpl_Vtbl =
{
    AC_QueryInterface,
    AC_AddRef,
    AC_Release,
    AC_Initialize,
    AC_GetBufferSize,
    AC_GetStreamLatency,
    AC_GetCurrentPadding,
    AC_IsFormatSupported,
    AC_GetMixFormat,
    AC_GetDevicePeriod,
    AC_Start,
    AC_Stop,
    AC_Reset,
    AC_SetEventHandle,
    AC_GetService
};

#endif
