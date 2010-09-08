/*
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
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
    DWORD locked, flags, bufsize, pad, padpartial, ofs, psize, candisconnect;
    BYTE *buffer;
    WAVEFORMATEX *pwfx;
    ALuint source;
    INT64 frameswritten;
    REFERENCE_TIME laststamp;
    HANDLE timer_id;
    ALCdevice *dev;
    ALint format;

    ACRender *render;
    ACCapture *capture;
    ACSession *session;
    ASVolume *svolume;
    AClock *clock;
} ACImpl;

struct ACRender {
    const IAudioRenderClientVtbl *lpVtbl;
    LONG ref;
    ACImpl *parent;
};

struct ACCapture {
    const IAudioCaptureClientVtbl *lpVtbl;
    LONG ref;
    ACImpl *parent;
};

struct ACSession {
    const IAudioSessionControl2Vtbl *lpVtbl;
    LONG ref;
    ACImpl *parent;
};

struct ASVolume {
    const ISimpleAudioVolumeVtbl *lpVtbl;
    LONG ref;
    ACImpl *parent;
};

struct AClock {
    const IAudioClockVtbl *lpVtbl;
    const IAudioClock2Vtbl *lp2Vtbl;
    LONG ref;
    ACImpl *parent;
};

static const IAudioClientVtbl ACImpl_Vtbl;
static const IAudioRenderClientVtbl ACRender_Vtbl;
static const IAudioCaptureClientVtbl ACCapture_Vtbl;
static const IAudioSessionControl2Vtbl ACSession_Vtbl;
static const ISimpleAudioVolumeVtbl ASVolume_Vtbl;
static const IAudioClockVtbl AClock_Vtbl;
static const IAudioClock2Vtbl AClock2_Vtbl;

static HRESULT AudioRenderClient_Create(ACImpl *parent, ACRender **ppv);
static void AudioRenderClient_Destroy(ACRender *This);
static HRESULT AudioCaptureClient_Create(ACImpl *parent, ACCapture **ppv);
static void AudioCaptureClient_Destroy(ACCapture *This);
static HRESULT AudioSessionControl_Create(ACImpl *parent, ACSession **ppv);
static void AudioSessionControl_Destroy(ACSession *This);
static HRESULT AudioSimpleVolume_Create(ACImpl *parent, ASVolume **ppv);
static void AudioSimpleVolume_Destroy(ASVolume *This);
static HRESULT AudioClock_Create(ACImpl *parent, AClock **ppv);
static void AudioClock_Destroy(AClock *This);

static int valid_dev(ACImpl *This)
{
    if (!This->dev)
        return 0;
    if (This->parent->flow == eRender && This->dev != This->parent->device)
        return 0;
    return 1;
}

static int get_format_PCM(WAVEFORMATEX *format)
{
    if (format->nChannels > 2) {
        FIXME("nChannels > 2 not documented for WAVE_FORMAT_PCM!\n");
        return 0;
    }

    format->cbSize = 0;

    if (format->nBlockAlign != format->wBitsPerSample/8*format->nChannels) {
        WARN("Invalid nBlockAlign %u, from %u %u\n",
             format->nBlockAlign, format->wBitsPerSample, format->nChannels);
        return 0;
    }

    switch (format->wBitsPerSample) {
        case 8: {
            switch (format->nChannels) {
            case 1: return AL_FORMAT_MONO8;
            case 2: return AL_FORMAT_STEREO8;
            }
        }
        case 16: {
            switch (format->nChannels) {
            case 1: return AL_FORMAT_MONO16;
            case 2: return AL_FORMAT_STEREO16;
            }
        }
    }

    if (!(format->wBitsPerSample % 8))
        WARN("Could not get OpenAL format (%d-bit, %d channels)\n",
             format->wBitsPerSample, format->nChannels);
    return 0;
}

/* Speaker configs */
#define MONO SPEAKER_FRONT_CENTER
#define STEREO (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT)
#define REAR (SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT)
#define QUAD (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT)
#define X5DOT1 (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT)
#define X6DOT1 (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT)
#define X7DOT1 (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT)

static int get_format_EXT(WAVEFORMATEX *format)
{
    WAVEFORMATEXTENSIBLE *wfe;

    if(format->cbSize < sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)) {
        WARN("Invalid cbSize specified for WAVE_FORMAT_EXTENSIBLE (%d)\n", format->cbSize);
        return 0;
    }

    wfe = (WAVEFORMATEXTENSIBLE*)format;
    wfe->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
    if (wfe->Samples.wValidBitsPerSample &&
        wfe->Samples.wValidBitsPerSample != format->wBitsPerSample) {
        FIXME("wValidBitsPerSample(%u) != wBitsPerSample(%u) unsupported\n",
              wfe->Samples.wValidBitsPerSample, format->wBitsPerSample);
        return 0;
    }

    TRACE("Extensible values:\n"
          "    Samples     = %d\n"
          "    ChannelMask = 0x%08x\n"
          "    SubFormat   = %s\n",
          wfe->Samples.wReserved, wfe->dwChannelMask,
          debugstr_guid(&wfe->SubFormat));

    if (wfe->dwChannelMask != MONO
        && wfe->dwChannelMask != STEREO
        && !palIsExtensionPresent("AL_EXT_MCFORMATS")) {
        /* QUAD PCM might still work, special case */
        if (palIsExtensionPresent("AL_LOKI_quadriphonic")
            && IsEqualGUID(&wfe->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)
            && wfe->dwChannelMask == QUAD) {
            if (format->wBitsPerSample == 16)
                return AL_FORMAT_QUAD16_LOKI;
            else if (format->wBitsPerSample == 8)
                return AL_FORMAT_QUAD8_LOKI;
        }
        WARN("Not all formats available\n");
        return 0;
    }

    if(IsEqualGUID(&wfe->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)) {
        if (format->wBitsPerSample == 8) {
            switch (wfe->dwChannelMask) {
            case   MONO: return AL_FORMAT_MONO8;
            case STEREO: return AL_FORMAT_STEREO8;
            case   REAR: return AL_FORMAT_REAR8;
            case   QUAD: return AL_FORMAT_QUAD8;
            case X5DOT1: return AL_FORMAT_51CHN8;
            case X6DOT1: return AL_FORMAT_61CHN8;
            case X7DOT1: return AL_FORMAT_71CHN8;
            default: break;
            }
        } else if (format->wBitsPerSample  == 16) {
            switch (wfe->dwChannelMask) {
            case   MONO: return AL_FORMAT_MONO16;
            case STEREO: return AL_FORMAT_STEREO16;
            case   REAR: return AL_FORMAT_REAR16;
            case   QUAD: return AL_FORMAT_QUAD16;
            case X5DOT1: return AL_FORMAT_51CHN16;
            case X6DOT1: return AL_FORMAT_61CHN16;
            case X7DOT1: return AL_FORMAT_71CHN16;
            default: break;
            }
        }
        else if (!(format->wBitsPerSample  % 8))
            ERR("Could not get OpenAL PCM format (%d-bit, mask 0x%08x)\n",
                format->wBitsPerSample, wfe->dwChannelMask);
        return 0;
    }
    else if(IsEqualGUID(&wfe->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
        if (format->wBitsPerSample != 32) {
            WARN("Invalid valid bits %u/32\n", format->wBitsPerSample);
            return 0;
        }
        switch (wfe->dwChannelMask) {
        case   MONO: return AL_FORMAT_MONO_FLOAT32;
        case STEREO: return AL_FORMAT_STEREO_FLOAT32;
        case   REAR: return AL_FORMAT_REAR32;
        case   QUAD: return AL_FORMAT_QUAD32;
        case X5DOT1: return AL_FORMAT_51CHN32;
        case X6DOT1: return AL_FORMAT_61CHN32;
        case X7DOT1: return AL_FORMAT_71CHN32;
        default:
            ERR("Could not get OpenAL float format (%d-bit, mask 0x%08x)\n",
                format->wBitsPerSample, wfe->dwChannelMask);
            return 0;
        }
    }
    else if (!IsEqualGUID(&wfe->SubFormat, &GUID_NULL))
        ERR("Unhandled extensible format: %s\n", debugstr_guid(&wfe->SubFormat));
    return 0;
}

static ALint get_format(WAVEFORMATEX *in)
{
    int ret = 0;
    if (in->wFormatTag == WAVE_FORMAT_PCM)
        ret = get_format_PCM(in);
    else if (in->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        ret = get_format_EXT(in);
    return ret;
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
    if (This->render)
        AudioRenderClient_Destroy(This->render);
    if (This->capture)
        AudioCaptureClient_Destroy(This->capture);
    if (This->session)
        AudioSessionControl_Destroy(This->session);
    if (This->svolume)
        AudioSimpleVolume_Destroy(This->svolume);
    if (This->clock)
        AudioClock_Destroy(This->clock);
    if (!valid_dev(This))
        TRACE("Not destroying device since none exists\n");
    else if (This->parent->flow == eRender) {
        setALContext(This->parent->ctx);
        IAudioClient_Stop((IAudioClient*)This);
        IAudioClient_Reset((IAudioClient*)This);
        palDeleteSources(1, &This->source);
        getALError();
        popALContext();
    }
    else if (This->parent->flow == eCapture)
        palcCaptureCloseDevice(This->dev);
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
        WARN("Could not open device %s: 0x%04x\n", alname, err);
        return AUDCLNT_E_DEVICE_IN_USE;
    }
    cur->ctx = palcCreateContext(cur->device, NULL);
    if (!cur->ctx) {
        ALCenum err = palcGetError(cur->device);
        ERR("Could not create context: 0x%04x\n", err);
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
    if (This->dev) {
        FIXME("Attempting to open device while already open\n");
        return S_OK;
    }
    WideCharToMultiByte(CP_UNIXCP, 0, This->parent->alname, -1,
                        alname, sizeof(alname)/sizeof(*alname)-1, NULL, NULL);
    This->dev = palcCaptureOpenDevice(alname, freq, This->format, size);
    if (!This->dev) {
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
    if (FAILED(hr) || pwfx2) {
        WARN("Format not supported, or had to be modified!\n");
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }
    EnterCriticalSection(This->crst);
    HeapFree(GetProcessHeap(), 0, This->pwfx);
    This->pwfx = HeapAlloc(GetProcessHeap(), 0, sizeof(*pwfx) + pwfx->cbSize);
    if (!This->pwfx) {
        hr = E_OUTOFMEMORY;
        goto out;
    }
    memcpy(This->pwfx, pwfx, sizeof(*pwfx) + pwfx->cbSize);
    if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE *wfe = (WAVEFORMATEXTENSIBLE *)This->pwfx;
        switch (pwfx->nChannels) {
            case 1: wfe->dwChannelMask = MONO; break;
            case 2: wfe->dwChannelMask = STEREO; break;
            case 4: wfe->dwChannelMask = QUAD; break;
            case 6: wfe->dwChannelMask = X5DOT1; break;
            case 7: wfe->dwChannelMask = X6DOT1; break;
            case 8: wfe->dwChannelMask = X7DOT1; break;
        default:
            ERR("How did we end up with %i channels?\n", pwfx->nChannels);
            hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
            goto out;
        }
    }

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
        This->dev = This->parent->device;
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

    This->candisconnect = palcIsExtensionPresent(This->dev, "ALC_EXT_disconnect");
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

static int disconnected(ACImpl *This)
{
    if (!This->candisconnect)
        return 0;
    if (This->parent->flow == eRender) {
        if (This->parent->device) {
            ALCint con = 1;
            palcGetIntegerv(This->parent->device, ALC_CONNECTED, 1, &con);
            palcGetError(This->parent->device);
            if (!con) {
                palcCloseDevice(This->parent->device);
                This->parent->device = NULL;
                This->parent->ctx = NULL;
                This->dev = NULL;
            }
        }

        if (!This->parent->device && FAILED(AC_OpenRenderAL(This))) {
            This->pad -= This->padpartial;
            This->padpartial = 0;
            return 1;
        }
        if (This->parent->device != This->dev) {
            WARN("Emptying buffer after newly reconnected!\n");
            This->pad -= This->padpartial;
            This->padpartial = 0;

            This->dev = This->parent->device;
            setALContext(This->parent->ctx);
            palGenSources(1, &This->source);
            palSourcei(This->source, AL_LOOPING, AL_FALSE);
            getALError();

            if (This->render && !This->locked && This->pad) {
                UINT pad = This->pad;
                BYTE *data;
                This->pad = 0;

                /* Probably will cause sound glitches, who cares? */
                IAudioRenderClient_GetBuffer((IAudioRenderClient *)This->render, pad, &data);
                IAudioRenderClient_ReleaseBuffer((IAudioRenderClient *)This->render, pad, 0);
            }
            popALContext();
        }
    } else {
        if (This->dev) {
            ALCint con = 1;
            palcGetIntegerv(This->dev, ALC_CONNECTED, 1, &con);
            palcGetError(This->dev);
            if (!con) {
                palcCaptureCloseDevice(This->dev);
                This->dev = NULL;
            }
        }

        if (!This->dev) {
            if (FAILED(AC_OpenCaptureAL(This)))
                return 1;

            WARN("Emptying buffer after newly reconnected!\n");
            This->pad = This->ofs = 0;
            if (This->running)
                palcCaptureStart(This->dev);
        }
    }
    return 0;
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
    if (disconnected(This)) {
        REFERENCE_TIME time = gettime(), period;

        WARN("No device found, faking increment\n");
        IAudioClient_GetDevicePeriod(iface, &period, NULL);
        while (This->running && time - This->laststamp >= period) {
            This->laststamp += period;

            if (This->parent->flow == eCapture) {
                This->pad += This->psize;
                if (This->pad > This->bufsize)
                    This->pad = This->bufsize;
            } else {
                if (This->pad <= This->psize) {
                    This->pad = 0;
                    break;
                } else
                    This->pad -= This->psize;
            }
        }

        if (This->parent->flow == eCapture)
            *numpad = This->pad >= This->psize ? This->psize : 0;
        else
            *numpad = This->pad;
    } else if (This->parent->flow == eRender) {
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
        palcGetIntegerv(This->dev, ALC_CAPTURE_SAMPLES, 1, &avail);
        if (avail) {
            DWORD ofs = This->ofs + This->pad;
            BYTE *buf1;
            ofs %= This->bufsize;
            buf1 = This->buffer + (ofs * block);
            This->laststamp = gettime();
            if (This->handle)
                SetEvent(This->handle);

            if (ofs + avail <= This->bufsize)
                palcCaptureSamples(This->dev, buf1, avail);
            else {
                DWORD part1 = This->bufsize - ofs;
                palcCaptureSamples(This->dev, buf1, part1);
                palcCaptureSamples(This->dev, This->buffer, avail - part1);
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
                This->pad = This->bufsize;
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
    WAVEFORMATEX *tmp;
    DWORD mask;
    DWORD size;
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

    if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        size = sizeof(WAVEFORMATEXTENSIBLE);
    else if (pwfx->wFormatTag == WAVE_FORMAT_PCM)
        size = sizeof(WAVEFORMATEX);
    else
        return AUDCLNT_E_UNSUPPORTED_FORMAT;

    if (pwfx->nSamplesPerSec < 8000
        || pwfx->nSamplesPerSec > 192000)
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    if (pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE
        || !IsEqualIID(&((WAVEFORMATEXTENSIBLE*)pwfx)->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
        if (pwfx->wBitsPerSample > 16)
            return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }

    switch (pwfx->nChannels) {
        case 1: mask = MONO; break;
        case 2: mask = STEREO; break;
        case 4: mask = QUAD; break;
        case 6: mask = X5DOT1; break;
        case 7: mask = X6DOT1; break;
        case 8: mask = X7DOT1; break;
        default:
            TRACE("Unsupported channel count %i\n", pwfx->nChannels);
            return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }
    tmp = CoTaskMemAlloc(size);
    if (outpwfx)
        *outpwfx = tmp;
    if (!tmp)
        return E_OUTOFMEMORY;

    memcpy(tmp, pwfx, size);
    tmp->nBlockAlign = tmp->nChannels * tmp->wBitsPerSample / 8;
    tmp->nAvgBytesPerSec = tmp->nBlockAlign * tmp->nSamplesPerSec;
    tmp->cbSize = size - sizeof(WAVEFORMATEX);
    if (tmp->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE *ex = (WAVEFORMATEXTENSIBLE*)tmp;

        if (ex->Samples.wValidBitsPerSample)
            ex->Samples.wValidBitsPerSample = ex->Format.wBitsPerSample;

        /* Rear is a special allowed case */
        if (ex->dwChannelMask
            && !(ex->Format.nChannels == 2 && ex->dwChannelMask == REAR))
            ex->dwChannelMask = mask;
    }

    if (memcmp(pwfx, tmp, size)) {
        if (outpwfx)
            return S_FALSE;
        CoTaskMemFree(tmp);
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }
    if (outpwfx)
        *outpwfx = NULL;
    CoTaskMemFree(tmp);
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
    if (!valid_dev(This))
        WARN("No valid device\n");
    else if (This->parent->flow == eRender) {
        setALContext(This->parent->ctx);
        palSourcePlay(This->source);
        getALError();
        popALContext();
    }
    else
        palcCaptureStart(This->dev);

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
    if (!valid_dev(This))
        WARN("No valid device\n");
    else if (This->parent->flow == eRender) {
        ALint state;
        setALContext(This->parent->ctx);
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
        palcCaptureStop(This->dev);
    This->running = FALSE;
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
    if (!valid_dev(This))
        WARN("No valid device\n");
    else if (This->parent->flow == eRender) {
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
        palcGetIntegerv(This->dev, ALC_CAPTURE_SAMPLES, 1, &avail);
        if (avail)
            palcCaptureSamples(This->dev, This->buffer, avail);
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

    if (IsEqualIID(riid, &IID_IAudioRenderClient)) {
        if (This->parent->flow != eRender)
            return AUDCLNT_E_WRONG_ENDPOINT_TYPE;
        if (!This->render)
            hr = AudioRenderClient_Create(This, &This->render);
        *ppv = This->render;
    } else if (IsEqualIID(riid, &IID_IAudioCaptureClient)) {
        if (This->parent->flow != eCapture)
            return AUDCLNT_E_WRONG_ENDPOINT_TYPE;
        if (!This->capture)
            hr = AudioCaptureClient_Create(This, &This->capture);
        *ppv = This->capture;
    } else if (IsEqualIID(riid, &IID_IAudioSessionControl)) {
        if (!This->session)
            hr = AudioSessionControl_Create(This, &This->session);
        *ppv = This->session;
    } else if (IsEqualIID(riid, &IID_ISimpleAudioVolume)) {
        if (!This->svolume)
            hr = AudioSimpleVolume_Create(This, &This->svolume);
        *ppv = This->svolume;
    } else if (IsEqualIID(riid, &IID_IAudioClock)) {
        if (!This->clock)
            hr = AudioClock_Create(This, &This->clock);
        *ppv = This->clock;
    }

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

static HRESULT AudioRenderClient_Create(ACImpl *parent, ACRender **ppv)
{
    ACRender *This;

    This = *ppv = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;
    This->lpVtbl = &ACRender_Vtbl;
    This->ref = 0;
    This->parent = parent;
    return S_OK;
}

static void AudioRenderClient_Destroy(ACRender *This)
{
    This->parent->render = NULL;
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI ACR_QueryInterface(IAudioRenderClient *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IAudioRenderClient))
        *ppv = iface;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ACR_AddRef(IAudioRenderClient *iface)
{
    ACRender *This = (ACRender*)iface;
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI ACR_Release(IAudioRenderClient *iface)
{
    ACRender *This = (ACRender*)iface;
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    if (!ref)
        AudioRenderClient_Destroy(This);
    return ref;
}

static HRESULT WINAPI ACR_GetBuffer(IAudioRenderClient *iface, UINT32 frames, BYTE **data)
{
    ACRender *This = (ACRender*)iface;
    DWORD free, framesize;
    TRACE("(%p)->(%u,%p)\n", This, frames, data);

    if (!data)
        return E_POINTER;
    if (!frames)
        return S_OK;
    *data = NULL;
    if (This->parent->locked) {
        ERR("Locked\n");
        return AUDCLNT_E_OUT_OF_ORDER;
    }
    AC_GetCurrentPadding((IAudioClient*)This->parent, &free);
    if (This->parent->bufsize-free < frames) {
        ERR("Too large: %u %u %u\n", This->parent->bufsize, free, frames);
        return AUDCLNT_E_BUFFER_TOO_LARGE;
    }
    EnterCriticalSection(This->parent->crst);
    This->parent->locked = frames;
    framesize = This->parent->pwfx->nBlockAlign;

    /* Exact offset doesn't matter, offset could be 0 forever
     * but increasing it is easier to debug */
    if (This->parent->ofs + frames > This->parent->bufsize)
        This->parent->ofs = 0;
    *data = This->parent->buffer + This->parent->ofs * framesize;

    LeaveCriticalSection(This->parent->crst);
    return S_OK;
}

static HRESULT WINAPI ACR_ReleaseBuffer(IAudioRenderClient *iface, UINT32 written, DWORD flags)
{
    ACRender *This = (ACRender*)iface;
    BYTE *buf = This->parent->buffer;
    DWORD framesize = This->parent->pwfx->nBlockAlign;
    DWORD ofs = This->parent->ofs;
    DWORD bufsize = This->parent->bufsize;
    DWORD freq = This->parent->pwfx->nSamplesPerSec;
    DWORD bpp = This->parent->pwfx->wBitsPerSample;
    ALuint albuf;

    TRACE("(%p)->(%u,%x)\n", This, written, flags);

    if (This->parent->locked < written)
        return AUDCLNT_E_INVALID_SIZE;

    if (flags & ~AUDCLNT_BUFFERFLAGS_SILENT)
        return E_INVALIDARG;

    if (!written) {
        if (This->parent->locked)
            FIXME("Handled right?\n");
        This->parent->locked = 0;
        return S_OK;
    }

    if (!This->parent->locked)
        return AUDCLNT_E_OUT_OF_ORDER;

    EnterCriticalSection(This->parent->crst);

    This->parent->ofs += written;
    This->parent->ofs %= bufsize;
    This->parent->pad += written;
    This->parent->frameswritten += written;
    This->parent->locked = 0;

    ofs *= framesize;
    written *= framesize;
    bufsize *= framesize;

    if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
        memset(buf+ofs, bpp != 8 ? 0 : 128, written);
    TRACE("buf: %p, ofs: %x, written %u, freq %u\n", buf, ofs, written, freq);
    if (!valid_dev(This->parent))
        goto out;

    setALContext(This->parent->parent->ctx);
    palGenBuffers(1, &albuf);
    palBufferData(albuf, This->parent->format, buf+ofs, written, freq);
    palSourceQueueBuffers(This->parent->source, 1, &albuf);
    TRACE("Queued %u\n", albuf);

    if (This->parent->running) {
        ALint state = AL_PLAYING, done = 0, padpart = 0;

        palGetSourcei(This->parent->source, AL_BUFFERS_PROCESSED, &done);
        palGetSourcei(This->parent->source, AL_BYTE_OFFSET, &padpart);
        palGetSourcei(This->parent->source, AL_SOURCE_STATE, &state);
        padpart /= framesize;

        if (state == AL_STOPPED) {
            padpart = This->parent->pad;
            /* Buffer might have been processed in the small window
             * between first and third call */
            palGetSourcei(This->parent->source, AL_BUFFERS_PROCESSED, &done);
        }
        if (done || This->parent->padpartial != padpart)
            This->parent->laststamp = gettime();
        This->parent->padpartial = padpart;

        while (done--) {
            ALint size, bits, chan;
            ALuint which;

            palSourceUnqueueBuffers(This->parent->source, 1, &which);
            palGetBufferi(which, AL_SIZE, &size);
            palGetBufferi(which, AL_BITS, &bits);
            palGetBufferi(which, AL_CHANNELS, &chan);
            size /= bits * chan / 8;
            if (size > This->parent->pad) {
                ERR("Overflow!\n");
                size = This->parent->pad;
            }
            This->parent->pad -= size;
            This->parent->padpartial -= size;
            TRACE("Unqueued %u\n", which);
            palDeleteBuffers(1, &which);
        }

        if (state != AL_PLAYING) {
            ERR("Starting from %x\n", state);
            palSourcePlay(This->parent->source);
        }
        getALError();
    }
    getALError();
    popALContext();
out:
    LeaveCriticalSection(This->parent->crst);

    return S_OK;
}

static const IAudioRenderClientVtbl ACRender_Vtbl = {
    ACR_QueryInterface,
    ACR_AddRef,
    ACR_Release,
    ACR_GetBuffer,
    ACR_ReleaseBuffer
};

static HRESULT AudioCaptureClient_Create(ACImpl *parent, ACCapture **ppv)
{
    ACCapture *This;
    This = *ppv = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;
    This->lpVtbl = &ACCapture_Vtbl;
    This->ref = 0;
    This->parent = parent;
    return S_OK;
}

static void AudioCaptureClient_Destroy(ACCapture *This)
{
    This->parent->capture = NULL;
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI ACC_QueryInterface(IAudioCaptureClient *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IAudioCaptureClient))
        *ppv = iface;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ACC_AddRef(IAudioCaptureClient *iface)
{
    ACCapture *This = (ACCapture*)iface;
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI ACC_Release(IAudioCaptureClient *iface)
{
    ACCapture *This = (ACCapture*)iface;
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    if (!ref)
        AudioCaptureClient_Destroy(This);
    return ref;
}

static HRESULT WINAPI ACC_GetBuffer(IAudioCaptureClient *iface, BYTE **data, UINT32 *frames, DWORD *flags, UINT64 *devpos, UINT64 *qpcpos)
{
    ACCapture *This = (ACCapture*)iface;
    HRESULT hr;
    DWORD block = This->parent->pwfx->nBlockAlign;
    DWORD ofs;
    TRACE("(%p)->(%p,%p,%p,%p,%p)\n", This, data, frames, flags, devpos, qpcpos);

    if (!data)
        return E_POINTER;
    if (!frames)
        return E_POINTER;
    if (!flags) {
        FIXME("Flags can be null?\n");
        return E_POINTER;
    }
    EnterCriticalSection(This->parent->crst);
    hr = AUDCLNT_E_OUT_OF_ORDER;
    if (This->parent->locked)
        goto out;
    IAudioCaptureClient_GetNextPacketSize(iface, frames);
    ofs = This->parent->ofs;
    if ( (ofs*block) % This->parent->psize)
        ERR("Unaligned offset %u with %u\n", ofs*block, This->parent->psize);
    *data = This->parent->buffer + ofs * block;
    This->parent->locked = *frames;
    if (devpos)
        *devpos = This->parent->frameswritten - This->parent->pad;
    if (qpcpos)
        *qpcpos = This->parent->laststamp;
    if (*frames)
        hr = S_OK;
    else
        hr = AUDCLNT_S_BUFFER_EMPTY;
out:
    LeaveCriticalSection(This->parent->crst);
    TRACE("Returning %08x %i\n", hr, *frames);
    return hr;
}

static HRESULT WINAPI ACC_ReleaseBuffer(IAudioCaptureClient *iface, UINT32 written)
{
    ACCapture *This = (ACCapture*)iface;
    HRESULT hr = S_OK;
    EnterCriticalSection(This->parent->crst);
    if (!written || written == This->parent->locked) {
        This->parent->locked = 0;
        This->parent->ofs += written;
        This->parent->ofs %= This->parent->bufsize;
        This->parent->pad -= written;
    } else if (!This->parent->locked)
        hr = AUDCLNT_E_OUT_OF_ORDER;
    else
        hr = AUDCLNT_E_INVALID_SIZE;
    LeaveCriticalSection(This->parent->crst);
    return hr;
}

static HRESULT WINAPI ACC_GetNextPacketSize(IAudioCaptureClient *iface, UINT32 *frames)
{
    ACCapture *This = (ACCapture*)iface;

    return AC_GetCurrentPadding((IAudioClient*)This->parent, frames);
}

static const IAudioCaptureClientVtbl ACCapture_Vtbl =
{
    ACC_QueryInterface,
    ACC_AddRef,
    ACC_Release,
    ACC_GetBuffer,
    ACC_ReleaseBuffer,
    ACC_GetNextPacketSize
};

static HRESULT AudioSessionControl_Create(ACImpl *parent, ACSession **ppv)
{
    ACSession *This;
    This = *ppv = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;
    This->lpVtbl = &ACSession_Vtbl;
    This->ref = 0;
    This->parent = parent;
    return S_OK;
}

static void AudioSessionControl_Destroy(ACSession *This)
{
    This->parent->session = NULL;
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI ACS_QueryInterface(IAudioSessionControl2 *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IAudioSessionControl)
        || IsEqualIID(riid, &IID_IAudioSessionControl2))
        *ppv = iface;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ACS_AddRef(IAudioSessionControl2 *iface)
{
    ACSession *This = (ACSession*)iface;
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI ACS_Release(IAudioSessionControl2 *iface)
{
    ACSession *This = (ACSession*)iface;
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    if (!ref)
        AudioSessionControl_Destroy(This);
    return ref;
}

static HRESULT WINAPI ACS_GetState(IAudioSessionControl2 *iface, AudioSessionState *state)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)->(%p)\n", This, state);

    if (!state)
        return E_POINTER;
    *state = This->parent->parent->state;
    return E_NOTIMPL;
}

static HRESULT WINAPI ACS_GetDisplayName(IAudioSessionControl2 *iface, WCHAR **name)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)->(%p)\n", This, name);
    FIXME("stub\n");
    if (name)
        *name = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ACS_SetDisplayName(IAudioSessionControl2 *iface, const WCHAR *name, const GUID *session)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)->(%p,%s)\n", This, name, debugstr_guid(session));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ACS_GetIconPath(IAudioSessionControl2 *iface, WCHAR **path)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)->(%p)\n", This, path);
    FIXME("stub\n");
    if (path)
        *path = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ACS_SetIconPath(IAudioSessionControl2 *iface, const WCHAR *path, const GUID *session)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)->(%p,%s)\n", This, path, debugstr_guid(session));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ACS_GetGroupingParam(IAudioSessionControl2 *iface, GUID *group)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)->(%p)\n", This, group);
    FIXME("stub\n");
    if (group)
        *group = GUID_NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ACS_SetGroupingParam(IAudioSessionControl2 *iface, GUID *group, const GUID *session)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)->(%s,%s)\n", This, debugstr_guid(group), debugstr_guid(session));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ACS_RegisterAudioSessionNotification(IAudioSessionControl2 *iface, IAudioSessionEvents *events)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)->(%p)\n", This, events);
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI ACS_UnregisterAudioSessionNotification(IAudioSessionControl2 *iface, IAudioSessionEvents *events)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)->(%p)\n", This, events);
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI ACS_GetSessionIdentifier(IAudioSessionControl2 *iface, WCHAR **id)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)->(%p)\n", This, id);
    FIXME("stub\n");
    if (id)
        *id = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ACS_GetSessionInstanceIdentifier(IAudioSessionControl2 *iface, WCHAR **id)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)->(%p)\n", This, id);
    FIXME("stub\n");
    if (id)
        *id = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI ACS_GetProcessId(IAudioSessionControl2 *iface, DWORD *pid)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)->(%p)\n", This, pid);

    if (!pid)
        return E_POINTER;
    *pid = GetCurrentProcessId();
    return S_OK;
}

static HRESULT WINAPI ACS_IsSystemSoundsSession(IAudioSessionControl2 *iface)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)\n", This);

    return S_FALSE;
}

static HRESULT WINAPI ACS_SetDuckingPreference(IAudioSessionControl2 *iface, BOOL optout)
{
    ACSession *This = (ACSession*)iface;
    TRACE("(%p)\n", This);

    return S_OK;
}

static const IAudioSessionControl2Vtbl ACSession_Vtbl =
{
    ACS_QueryInterface,
    ACS_AddRef,
    ACS_Release,
    ACS_GetState,
    ACS_GetDisplayName,
    ACS_SetDisplayName,
    ACS_GetIconPath,
    ACS_SetIconPath,
    ACS_GetGroupingParam,
    ACS_SetGroupingParam,
    ACS_RegisterAudioSessionNotification,
    ACS_UnregisterAudioSessionNotification,
    ACS_GetSessionIdentifier,
    ACS_GetSessionInstanceIdentifier,
    ACS_GetProcessId,
    ACS_IsSystemSoundsSession,
    ACS_SetDuckingPreference
};

static HRESULT AudioSimpleVolume_Create(ACImpl *parent, ASVolume **ppv)
{
    ASVolume *This;
    This = *ppv = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;
    This->lpVtbl = &ASVolume_Vtbl;
    This->ref = 0;
    This->parent = parent;
    return S_OK;
}

static void AudioSimpleVolume_Destroy(ASVolume *This)
{
    This->parent->svolume = NULL;
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI ASV_QueryInterface(ISimpleAudioVolume *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_ISimpleAudioVolume))
        *ppv = iface;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ASV_AddRef(ISimpleAudioVolume *iface)
{
    ASVolume *This = (ASVolume*)iface;
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI ASV_Release(ISimpleAudioVolume *iface)
{
    ASVolume *This = (ASVolume*)iface;
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    if (!ref)
        AudioSimpleVolume_Destroy(This);
    return ref;
}

static HRESULT WINAPI ASV_SetMasterVolume(ISimpleAudioVolume *iface, float level, const GUID *context)
{
    ASVolume *This = (ASVolume*)iface;
    TRACE("(%p)->(%f,%p)\n", This, level, context);

    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI ASV_GetMasterVolume(ISimpleAudioVolume *iface, float *level)
{
    ASVolume *This = (ASVolume*)iface;
    TRACE("(%p)->(%p)\n", This, level);

    *level = 1.f;
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI ASV_SetMute(ISimpleAudioVolume *iface, BOOL mute, const GUID *context)
{
    ASVolume *This = (ASVolume*)iface;
    TRACE("(%p)->(%u,%p)\n", This, mute, context);

    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI ASV_GetMute(ISimpleAudioVolume *iface, BOOL *mute)
{
    ASVolume *This = (ASVolume*)iface;
    TRACE("(%p)->(%p)\n", This, mute);

    *mute = 0;
    FIXME("stub\n");
    return S_OK;
}

static const ISimpleAudioVolumeVtbl ASVolume_Vtbl  =
{
    ASV_QueryInterface,
    ASV_AddRef,
    ASV_Release,
    ASV_SetMasterVolume,
    ASV_GetMasterVolume,
    ASV_SetMute,
    ASV_GetMute
};

static HRESULT AudioClock_Create(ACImpl *parent, AClock **ppv)
{
    AClock *This;
    This = *ppv = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;
    This->lpVtbl = &AClock_Vtbl;
    This->lp2Vtbl = &AClock2_Vtbl;
    This->ref = 0;
    This->parent = parent;
    return S_OK;
}

static void AudioClock_Destroy(AClock *This)
{
    This->parent->clock = NULL;
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI AClock_QueryInterface(IAudioClock *iface, REFIID riid, void **ppv)
{
    AClock *This = (AClock*)iface;
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IAudioClock))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IAudioClock2))
        *ppv = &This->lp2Vtbl;
    if (*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AClock_AddRef(IAudioClock *iface)
{
    AClock *This = (AClock*)iface;
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI AClock_Release(IAudioClock *iface)
{
    AClock *This = (AClock*)iface;
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    if (!ref)
        AudioClock_Destroy(This);
    return ref;
}

static HRESULT WINAPI AClock_GetFrequency(IAudioClock *iface, UINT64 *freq)
{
    AClock *This = (AClock*)iface;
    TRACE("(%p)->(%p)\n", This, freq);

    *freq = (UINT64)This->parent->pwfx->nSamplesPerSec;
    return S_OK;
}

static HRESULT WINAPI AClock_GetPosition(IAudioClock *iface, UINT64 *pos, UINT64 *qpctime)
{
    AClock *This = (AClock*)iface;
    DWORD pad;

    TRACE("(%p)->(%p,%p)\n", This, pos, qpctime);

    if (!pos)
        return E_POINTER;

    EnterCriticalSection(This->parent->crst);
    AC_GetCurrentPadding((IAudioClient*)This->parent, &pad);
    *pos = This->parent->frameswritten - pad;
    if (qpctime)
        *qpctime = gettime();
    LeaveCriticalSection(This->parent->crst);

    return S_OK;
}

static HRESULT WINAPI AClock_GetCharacteristics(IAudioClock *iface, DWORD *chars)
{
    AClock *This = (AClock*)iface;
    TRACE("(%p)->(%p)\n", This, chars);

    if (!chars)
        return E_POINTER;
    *chars = AUDIOCLOCK_CHARACTERISTIC_FIXED_FREQ;
    return S_OK;
}

static const IAudioClockVtbl AClock_Vtbl =
{
    AClock_QueryInterface,
    AClock_AddRef,
    AClock_Release,
    AClock_GetFrequency,
    AClock_GetPosition,
    AClock_GetCharacteristics
};

static AClock *get_clock_from_clock2(IAudioClock2 *iface)
{
    return (AClock*)((char*)iface - offsetof(AClock,lp2Vtbl));
}

static HRESULT WINAPI AClock2_QueryInterface(IAudioClock2 *iface, REFIID riid, void **ppv)
{
    AClock *This = get_clock_from_clock2(iface);
    return IUnknown_QueryInterface((IUnknown*)This, riid, ppv);
}

static ULONG WINAPI AClock2_AddRef(IAudioClock2 *iface)
{
    AClock *This = get_clock_from_clock2(iface);
    return IUnknown_AddRef((IUnknown*)This);
}

static ULONG WINAPI AClock2_Release(IAudioClock2 *iface)
{
    AClock *This = get_clock_from_clock2(iface);
    return IUnknown_Release((IUnknown*)This);
}

static HRESULT WINAPI AClock2_GetPosition(IAudioClock2 *iface, UINT64 *pos, UINT64 *qpctime)
{
    AClock *This = get_clock_from_clock2(iface);
    return AClock_GetPosition((IAudioClock*)This, pos, qpctime);
}

static const IAudioClock2Vtbl AClock2_Vtbl =
{
    AClock2_QueryInterface,
    AClock2_AddRef,
    AClock2_Release,
    AClock2_GetPosition
};

#endif
