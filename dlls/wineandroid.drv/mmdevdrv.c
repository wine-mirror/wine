/*
 * Copyright 2015 Andrew Eikum for CodeWeavers
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
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <dlfcn.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "android.h"

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/list.h"

#include "ole2.h"
#include "mmdeviceapi.h"
#include "devpkey.h"
#include "dshow.h"
#include "dsound.h"

#include "initguid.h"
#include "endpointvolume.h"
#include "audiopolicy.h"
#include "audioclient.h"

WINE_DEFAULT_DEBUG_CHANNEL(androidaudio);

#define DECL_FUNCPTR(f) static typeof(f) * p##f
DECL_FUNCPTR( slCreateEngine );
DECL_FUNCPTR( SL_IID_ANDROIDSIMPLEBUFFERQUEUE );
DECL_FUNCPTR( SL_IID_ENGINE );
DECL_FUNCPTR( SL_IID_PLAY );
DECL_FUNCPTR( SL_IID_PLAYBACKRATE );
DECL_FUNCPTR( SL_IID_RECORD );

#define SLCALL_N(obj, func) (*obj)->func(obj)
#define SLCALL(obj, func, ...) (*obj)->func(obj, __VA_ARGS__)

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

static const REFERENCE_TIME DefaultPeriod = 100000;
static const REFERENCE_TIME MinimumPeriod = 50000;

struct ACImpl;
typedef struct ACImpl ACImpl;

typedef struct _AudioSession {
    GUID guid;
    struct list clients;

    IMMDevice *device;

    float master_vol;
    UINT32 channel_count;
    float *channel_vols;
    BOOL mute;

    CRITICAL_SECTION lock;

    struct list entry;
} AudioSession;

typedef struct _AudioSessionWrapper {
    IAudioSessionControl2 IAudioSessionControl2_iface;
    IChannelAudioVolume IChannelAudioVolume_iface;
    ISimpleAudioVolume ISimpleAudioVolume_iface;

    LONG ref;

    ACImpl *client;
    AudioSession *session;
} AudioSessionWrapper;

struct ACImpl {
    IAudioClient3 IAudioClient3_iface;
    IAudioRenderClient IAudioRenderClient_iface;
    IAudioCaptureClient IAudioCaptureClient_iface;
    IAudioClock IAudioClock_iface;
    IAudioClock2 IAudioClock2_iface;
    IAudioStreamVolume IAudioStreamVolume_iface;

    LONG ref;

    IMMDevice *parent;
    IUnknown *pUnkFTMarshal;

    WAVEFORMATEX *fmt;

    EDataFlow dataflow;
    DWORD flags;
    AUDCLNT_SHAREMODE share;
    HANDLE event;
    float *vols;

    SLObjectItf player;
    SLObjectItf recorder;
    SLAndroidSimpleBufferQueueItf bufq;
    SLPlayItf playitf;
    SLRecordItf recorditf;

    BOOL initted, playing;
    UINT64 written_frames, last_pos_frames;
    UINT32 period_us, period_frames, bufsize_frames, held_frames, tmp_buffer_frames, wrap_buffer_frames, in_sl_frames;
    UINT32 oss_bufsize_bytes, lcl_offs_frames; /* offs into local_buffer where valid data starts */

    BYTE *local_buffer, *tmp_buffer, *wrap_buffer;
    LONG32 getbuf_last; /* <0 when using tmp_buffer */
    HANDLE timer;

    CRITICAL_SECTION lock;

    AudioSession *session;
    AudioSessionWrapper *session_wrapper;

    struct list entry;
};

typedef struct _SessionMgr {
    IAudioSessionManager2 IAudioSessionManager2_iface;

    LONG ref;

    IMMDevice *device;
} SessionMgr;

static struct list g_devices = LIST_INIT(g_devices);

static HANDLE g_timer_q;

static CRITICAL_SECTION g_sessions_lock;
static CRITICAL_SECTION_DEBUG g_sessions_lock_debug =
{
    0, 0, &g_sessions_lock,
    { &g_sessions_lock_debug.ProcessLocksList, &g_sessions_lock_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": g_sessions_lock") }
};
static CRITICAL_SECTION g_sessions_lock = { &g_sessions_lock_debug, -1, 0, 0, 0, 0 };
static struct list g_sessions = LIST_INIT(g_sessions);

static AudioSessionWrapper *AudioSessionWrapper_Create(ACImpl *client);

static const IAudioClient3Vtbl AudioClient3_Vtbl;
static const IAudioRenderClientVtbl AudioRenderClient_Vtbl;
static const IAudioCaptureClientVtbl AudioCaptureClient_Vtbl;
static const IAudioSessionControl2Vtbl AudioSessionControl2_Vtbl;
static const ISimpleAudioVolumeVtbl SimpleAudioVolume_Vtbl;
static const IAudioClockVtbl AudioClock_Vtbl;
static const IAudioClock2Vtbl AudioClock2_Vtbl;
static const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl;
static const IChannelAudioVolumeVtbl ChannelAudioVolume_Vtbl;
static const IAudioSessionManager2Vtbl AudioSessionManager2_Vtbl;

static inline ACImpl *impl_from_IAudioClient3(IAudioClient3 *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioClient3_iface);
}

static inline ACImpl *impl_from_IAudioRenderClient(IAudioRenderClient *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioRenderClient_iface);
}

static inline ACImpl *impl_from_IAudioCaptureClient(IAudioCaptureClient *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioCaptureClient_iface);
}

static inline AudioSessionWrapper *impl_from_IAudioSessionControl2(IAudioSessionControl2 *iface)
{
    return CONTAINING_RECORD(iface, AudioSessionWrapper, IAudioSessionControl2_iface);
}

static inline AudioSessionWrapper *impl_from_ISimpleAudioVolume(ISimpleAudioVolume *iface)
{
    return CONTAINING_RECORD(iface, AudioSessionWrapper, ISimpleAudioVolume_iface);
}

static inline AudioSessionWrapper *impl_from_IChannelAudioVolume(IChannelAudioVolume *iface)
{
    return CONTAINING_RECORD(iface, AudioSessionWrapper, IChannelAudioVolume_iface);
}

static inline ACImpl *impl_from_IAudioClock(IAudioClock *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioClock_iface);
}

static inline ACImpl *impl_from_IAudioClock2(IAudioClock2 *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioClock2_iface);
}

static inline ACImpl *impl_from_IAudioStreamVolume(IAudioStreamVolume *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioStreamVolume_iface);
}

static inline SessionMgr *impl_from_IAudioSessionManager2(IAudioSessionManager2 *iface)
{
    return CONTAINING_RECORD(iface, SessionMgr, IAudioSessionManager2_iface);
}

#define LOAD_FUNCPTR(lib, func) do { \
    if ((p##func = dlsym( lib, #func )) == NULL) \
        { ERR( "can't find symbol %s\n", #func); return FALSE; } \
    } while(0)

static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

static BOOL WINAPI load_opensles( INIT_ONCE *once, void *param, void **context )
{
    void *libopensles;

    if (!(libopensles = dlopen( "libOpenSLES.so", RTLD_GLOBAL )))
    {
        ERR( "failed to load libOpenSLES.so: %s\n", dlerror() );
        return FALSE;
    }
    LOAD_FUNCPTR( libopensles, slCreateEngine );
    LOAD_FUNCPTR( libopensles, SL_IID_ANDROIDSIMPLEBUFFERQUEUE );
    LOAD_FUNCPTR( libopensles, SL_IID_ENGINE );
    LOAD_FUNCPTR( libopensles, SL_IID_PLAY );
    LOAD_FUNCPTR( libopensles, SL_IID_PLAYBACKRATE );
    LOAD_FUNCPTR( libopensles, SL_IID_RECORD );

    if (!(g_timer_q = CreateTimerQueue())) return FALSE;

    return TRUE;
}

/* From <dlls/mmdevapi/mmdevapi.h> */
enum DriverPriority {
    Priority_Unavailable = 0,
    Priority_Low,
    Priority_Neutral,
    Priority_Preferred
};

int WINAPI AUDDRV_GetPriority(void)
{
    if (!InitOnceExecuteOnce( &init_once, load_opensles, NULL, NULL ))
        return Priority_Unavailable;

    return Priority_Preferred;
}

static SLObjectItf sl;
static SLEngineItf engine;
static SLObjectItf outputmix;

HRESULT AUDDRV_Init(void)
{
    static const SLEngineOption options[] = { {SL_ENGINEOPTION_THREADSAFE, SL_BOOLEAN_TRUE} };
    SLresult sr;

    sr = pslCreateEngine(&sl, 1, options, 0, NULL, NULL);
    if(sr != SL_RESULT_SUCCESS){
        WARN("slCreateEngine failed: 0x%x\n", sr);
        return E_FAIL;
    }

    sr = SLCALL(sl, Realize, SL_BOOLEAN_FALSE);
    if(sr != SL_RESULT_SUCCESS){
        SLCALL_N(sl, Destroy);
        WARN("Engine Realize failed: 0x%x\n", sr);
        return E_FAIL;
    }

    sr = SLCALL(sl, GetInterface, *pSL_IID_ENGINE, (void*)&engine);
    if(sr != SL_RESULT_SUCCESS){
        SLCALL_N(sl, Destroy);
        WARN("GetInterface failed: 0x%x\n", sr);
        return E_FAIL;
    }

    sr = SLCALL(engine, CreateOutputMix, &outputmix, 0, NULL, NULL);
    if(sr != SL_RESULT_SUCCESS){
        SLCALL_N(sl, Destroy);
        WARN("CreateOutputMix failed: 0x%x\n", sr);
        return E_FAIL;
    }

    sr = SLCALL(outputmix, Realize, SL_BOOLEAN_FALSE);
    if(sr != SL_RESULT_SUCCESS){
        SLCALL_N(outputmix, Destroy);
        SLCALL_N(sl, Destroy);
        WARN("outputmix Realize failed: 0x%x\n", sr);
        return E_FAIL;
    }

    return S_OK;
}

static const GUID outGuid = {0x0a047ace, 0x22b1, 0x4342, {0x98, 0xbb, 0xf8, 0x56, 0x32, 0x26, 0x61, 0x00}};
static const GUID inGuid = {0x0a047ace, 0x22b1, 0x4342, {0x98, 0xbb, 0xf8, 0x56, 0x32, 0x26, 0x61, 0x01}};

HRESULT WINAPI AUDDRV_GetEndpointIDs(EDataFlow flow, WCHAR ***ids, GUID **guids,
        UINT *num, UINT *def_index)
{
    static const WCHAR outName[] = {'A','n','d','r','o','i','d',' ','A','u','d','i','o',' ','O','u','t',0};
    static const WCHAR inName[] = {'A','n','d','r','o','i','d',' ','A','u','d','i','o',' ','I','n',0};

    TRACE("%u %p %p %p %p\n", flow, ids, guids, num, def_index);

    *def_index = 0;
    *num = 1;
    *ids = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR *));
    *guids = HeapAlloc(GetProcessHeap(), 0, sizeof(GUID));
    if(flow == eRender){
        (*ids)[0] = HeapAlloc(GetProcessHeap(), 0, sizeof(outName));
        memcpy((*ids)[0], outName, sizeof(outName));
        memcpy(&(*guids)[0], &outGuid, sizeof(outGuid));
    }else{
        (*ids)[0] = HeapAlloc(GetProcessHeap(), 0, sizeof(inName));
        memcpy((*ids)[0], inName, sizeof(inName));
        memcpy(&(*guids)[0], &inGuid, sizeof(inGuid));
    }

    return S_OK;
}

HRESULT WINAPI AUDDRV_GetAudioEndpoint(GUID *guid, IMMDevice *dev,
        IAudioClient **out)
{
    ACImpl *This;
    HRESULT hr;
    EDataFlow flow;

    TRACE("%s %p %p\n", debugstr_guid(guid), dev, out);

    if(!sl)
        AUDDRV_Init();

    if(IsEqualGUID(guid, &outGuid))
        flow = eRender;
    else if(IsEqualGUID(guid, &inGuid))
        flow = eCapture;
    else
        return AUDCLNT_E_DEVICE_INVALIDATED;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ACImpl));
    if(!This)
        return E_OUTOFMEMORY;

    hr = CoCreateFreeThreadedMarshaler((IUnknown *)&This->IAudioClient3_iface, &This->pUnkFTMarshal);
    if (FAILED(hr)) {
         HeapFree(GetProcessHeap(), 0, This);
         return hr;
    }

    This->dataflow = flow;

    This->IAudioClient3_iface.lpVtbl = &AudioClient3_Vtbl;
    This->IAudioRenderClient_iface.lpVtbl = &AudioRenderClient_Vtbl;
    This->IAudioCaptureClient_iface.lpVtbl = &AudioCaptureClient_Vtbl;
    This->IAudioClock_iface.lpVtbl = &AudioClock_Vtbl;
    This->IAudioClock2_iface.lpVtbl = &AudioClock2_Vtbl;
    This->IAudioStreamVolume_iface.lpVtbl = &AudioStreamVolume_Vtbl;

    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ACImpl.lock");

    This->parent = dev;
    IMMDevice_AddRef(This->parent);

    *out = (IAudioClient *)&This->IAudioClient3_iface;
    IAudioClient3_AddRef(&This->IAudioClient3_iface);

    return S_OK;
}

static HRESULT WINAPI AudioClient_QueryInterface(IAudioClient3 *iface,
        REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;
    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioClient) ||
            IsEqualIID(riid, &IID_IAudioClient2) ||
            IsEqualIID(riid, &IID_IAudioClient3))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->pUnkFTMarshal, riid, ppv);
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioClient_AddRef(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI AudioClient_Release(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    ULONG ref;

    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    if(!ref){
        if(This->timer){
            HANDLE event;
            DWORD wait;
            event = CreateEventW(NULL, TRUE, FALSE, NULL);
            wait = !DeleteTimerQueueTimer(g_timer_q, This->timer, event);
            wait = wait && GetLastError() == ERROR_IO_PENDING;
            if(event && wait)
                WaitForSingleObject(event, INFINITE);
            CloseHandle(event);
        }

        IAudioClient3_Stop(iface);

        IMMDevice_Release(This->parent);
        IUnknown_Release(This->pUnkFTMarshal);
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);

        if(This->recorder)
            SLCALL_N(This->recorder, Destroy);
        if(This->player)
            SLCALL_N(This->player, Destroy);

        if(This->initted){
            EnterCriticalSection(&g_sessions_lock);
            list_remove(&This->entry);
            LeaveCriticalSection(&g_sessions_lock);
        }
        HeapFree(GetProcessHeap(), 0, This->vols);
        HeapFree(GetProcessHeap(), 0, This->local_buffer);
        HeapFree(GetProcessHeap(), 0, This->tmp_buffer);
        HeapFree(GetProcessHeap(), 0, This->wrap_buffer);
        CoTaskMemFree(This->fmt);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static void dump_fmt(const WAVEFORMATEX *fmt)
{
    TRACE("wFormatTag: 0x%x (", fmt->wFormatTag);
    switch(fmt->wFormatTag){
    case WAVE_FORMAT_PCM:
        TRACE("WAVE_FORMAT_PCM");
        break;
    case WAVE_FORMAT_IEEE_FLOAT:
        TRACE("WAVE_FORMAT_IEEE_FLOAT");
        break;
    case WAVE_FORMAT_EXTENSIBLE:
        TRACE("WAVE_FORMAT_EXTENSIBLE");
        break;
    default:
        TRACE("Unknown");
        break;
    }
    TRACE(")\n");

    TRACE("nChannels: %u\n", fmt->nChannels);
    TRACE("nSamplesPerSec: %u\n", fmt->nSamplesPerSec);
    TRACE("nAvgBytesPerSec: %u\n", fmt->nAvgBytesPerSec);
    TRACE("nBlockAlign: %u\n", fmt->nBlockAlign);
    TRACE("wBitsPerSample: %u\n", fmt->wBitsPerSample);
    TRACE("cbSize: %u\n", fmt->cbSize);

    if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
        WAVEFORMATEXTENSIBLE *fmtex = (void*)fmt;
        TRACE("dwChannelMask: %08x\n", fmtex->dwChannelMask);
        TRACE("Samples: %04x\n", fmtex->Samples.wReserved);
        TRACE("SubFormat: %s\n", wine_dbgstr_guid(&fmtex->SubFormat));
    }
}

static DWORD get_channel_mask(unsigned int channels)
{
    switch(channels){
    case 0:
        return 0;
    case 1:
        return KSAUDIO_SPEAKER_MONO;
    case 2:
        return KSAUDIO_SPEAKER_STEREO;
    case 3:
        return KSAUDIO_SPEAKER_STEREO | SPEAKER_LOW_FREQUENCY;
    case 4:
        return KSAUDIO_SPEAKER_QUAD;    /* not _SURROUND */
    case 5:
        return KSAUDIO_SPEAKER_QUAD | SPEAKER_LOW_FREQUENCY;
    case 6:
        return KSAUDIO_SPEAKER_5POINT1; /* not 5POINT1_SURROUND */
    case 7:
        return KSAUDIO_SPEAKER_5POINT1 | SPEAKER_BACK_CENTER;
    case 8:
        return KSAUDIO_SPEAKER_7POINT1_SURROUND; /* Vista deprecates 7POINT1 */
    }
    FIXME("Unknown speaker configuration: %u\n", channels);
    return 0;
}

static WAVEFORMATEX *clone_format(const WAVEFORMATEX *fmt)
{
    WAVEFORMATEX *ret;
    size_t size;

    if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        size = sizeof(WAVEFORMATEXTENSIBLE);
    else
        size = sizeof(WAVEFORMATEX);

    ret = CoTaskMemAlloc(size);
    if(!ret)
        return NULL;

    memcpy(ret, fmt, size);

    ret->cbSize = size - sizeof(WAVEFORMATEX);

    return ret;
}

static void session_init_vols(AudioSession *session, UINT channels)
{
    if(session->channel_count < channels){
        UINT i;

        if(session->channel_vols)
            session->channel_vols = HeapReAlloc(GetProcessHeap(), 0,
                    session->channel_vols, sizeof(float) * channels);
        else
            session->channel_vols = HeapAlloc(GetProcessHeap(), 0,
                    sizeof(float) * channels);
        if(!session->channel_vols)
            return;

        for(i = session->channel_count; i < channels; ++i)
            session->channel_vols[i] = 1.f;

        session->channel_count = channels;
    }
}

static AudioSession *create_session(const GUID *guid, IMMDevice *device,
        UINT num_channels)
{
    AudioSession *ret;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(AudioSession));
    if(!ret)
        return NULL;

    memcpy(&ret->guid, guid, sizeof(GUID));

    ret->device = device;

    list_init(&ret->clients);

    list_add_head(&g_sessions, &ret->entry);

    InitializeCriticalSection(&ret->lock);
    ret->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": AudioSession.lock");

    session_init_vols(ret, num_channels);

    ret->master_vol = 1.f;

    return ret;
}

/* if channels == 0, then this will return or create a session with
 * matching dataflow and GUID. otherwise, channels must also match */
static HRESULT get_audio_session(const GUID *sessionguid,
        IMMDevice *device, UINT channels, AudioSession **out)
{
    AudioSession *session;

    if(!sessionguid || IsEqualGUID(sessionguid, &GUID_NULL)){
        *out = create_session(&GUID_NULL, device, channels);
        if(!*out)
            return E_OUTOFMEMORY;

        return S_OK;
    }

    *out = NULL;
    LIST_FOR_EACH_ENTRY(session, &g_sessions, AudioSession, entry){
        if(session->device == device &&
                IsEqualGUID(sessionguid, &session->guid)){
            session_init_vols(session, channels);
            *out = session;
            break;
        }
    }

    if(!*out){
        *out = create_session(sessionguid, device, channels);
        if(!*out)
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

static HRESULT waveformat_to_pcm(ACImpl *This, const WAVEFORMATEX *fmt, SLAndroidDataFormat_PCM_EX *pcm)
{
    if(fmt->nSamplesPerSec < 8000 || fmt->nSamplesPerSec > 48000)
        return AUDCLNT_E_UNSUPPORTED_FORMAT;

    pcm->formatType = SL_ANDROID_DATAFORMAT_PCM_EX;

    pcm->sampleRate = fmt->nSamplesPerSec * 1000; /* sampleRate is in milli-Hz */
    pcm->bitsPerSample = fmt->wBitsPerSample;
    pcm->containerSize = fmt->wBitsPerSample;

    if(fmt->wFormatTag == WAVE_FORMAT_PCM ||
            (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&((WAVEFORMATEXTENSIBLE*)fmt)->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))){
        if(pcm->bitsPerSample == 8)
            pcm->representation = SL_ANDROID_PCM_REPRESENTATION_UNSIGNED_INT;
        else if(pcm->bitsPerSample == 16)
            pcm->representation = SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT;
        else
            return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }else if(fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
                (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                 IsEqualGUID(&((WAVEFORMATEXTENSIBLE*)fmt)->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))){
        if(pcm->bitsPerSample == 32)
            pcm->representation = SL_ANDROID_PCM_REPRESENTATION_FLOAT;
        else
            return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }else
        return AUDCLNT_E_UNSUPPORTED_FORMAT;

    /* only up to stereo */
    pcm->numChannels = fmt->nChannels;
    if(pcm->numChannels == 1)
        pcm->channelMask = SL_SPEAKER_FRONT_CENTER;
    else if(This->dataflow == eRender && pcm->numChannels == 2)
        pcm->channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    else
        return AUDCLNT_E_UNSUPPORTED_FORMAT;

    pcm->endianness = SL_BYTEORDER_LITTLEENDIAN;

    return S_OK;
}

static HRESULT try_open_render_device(SLAndroidDataFormat_PCM_EX *pcm, unsigned int num_buffers, SLObjectItf *out)
{
    SLresult sr;
    SLDataSource source;
    SLDataSink sink;
    SLDataLocator_OutputMix loc_outmix;
    SLboolean required[2];
    SLInterfaceID iids[2];
    SLDataLocator_AndroidSimpleBufferQueue loc_bq;
    SLObjectItf player;

    loc_bq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    loc_bq.numBuffers = num_buffers;
    source.pLocator = &loc_bq;
    source.pFormat = pcm;

    loc_outmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    loc_outmix.outputMix = outputmix;
    sink.pLocator = &loc_outmix;
    sink.pFormat = NULL;

    required[0] = SL_BOOLEAN_TRUE;
    iids[0] = *pSL_IID_ANDROIDSIMPLEBUFFERQUEUE;
    required[1] = SL_BOOLEAN_TRUE;
    iids[1] = *pSL_IID_PLAYBACKRATE;

    sr = SLCALL(engine, CreateAudioPlayer, &player, &source, &sink,
            2, iids, required);
    if(sr != SL_RESULT_SUCCESS){
        WARN("CreateAudioPlayer failed: 0x%x\n", sr);
        return E_FAIL;
    }

    sr = SLCALL(player, Realize, SL_BOOLEAN_FALSE);
    if(sr != SL_RESULT_SUCCESS){
        SLCALL_N(player, Destroy);
        WARN("Player Realize failed: 0x%x\n", sr);
        return E_FAIL;
    }

    if(out)
        *out = player;
    else
        SLCALL_N(player, Destroy);

    return S_OK;
}

static HRESULT try_open_capture_device(SLAndroidDataFormat_PCM_EX *pcm, unsigned int num_buffers, SLObjectItf *out)
{
    SLresult sr;
    SLDataSource source;
    SLDataSink sink;
    SLDataLocator_IODevice loc_mic;
    SLboolean required[1];
    SLInterfaceID iids[1];
    SLDataLocator_AndroidSimpleBufferQueue loc_bq;
    SLObjectItf recorder;

    loc_mic.locatorType = SL_DATALOCATOR_IODEVICE;
    loc_mic.deviceType = SL_IODEVICE_AUDIOINPUT;
    loc_mic.deviceID = SL_DEFAULTDEVICEID_AUDIOINPUT;
    loc_mic.device = NULL;
    source.pLocator = &loc_mic;
    source.pFormat = NULL;

    loc_bq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    loc_bq.numBuffers = num_buffers;
    sink.pLocator = &loc_bq;
    sink.pFormat = pcm;

    required[0] = SL_BOOLEAN_TRUE;
    iids[0] = *pSL_IID_ANDROIDSIMPLEBUFFERQUEUE;

    sr = SLCALL(engine, CreateAudioRecorder, &recorder, &source, &sink,
            1, iids, required);
    if(sr != SL_RESULT_SUCCESS){
        WARN("CreateAudioRecorder failed: 0x%x\n", sr);
        return E_FAIL;
    }

    sr = SLCALL(recorder, Realize, SL_BOOLEAN_FALSE);
    if(sr != SL_RESULT_SUCCESS){
        SLCALL_N(recorder, Destroy);
        WARN("Recorder Realize failed: 0x%x\n", sr);
        return E_FAIL;
    }

    if(out)
        *out = recorder;
    else
        SLCALL_N(recorder, Destroy);

    return S_OK;
}

static HRESULT WINAPI AudioClient_Initialize(IAudioClient3 *iface,
        AUDCLNT_SHAREMODE mode, DWORD flags, REFERENCE_TIME duration,
        REFERENCE_TIME period, const WAVEFORMATEX *fmt,
        const GUID *sessionguid)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    int i, num_buffers;
    HRESULT hr;
    SLresult sr;
    SLAndroidDataFormat_PCM_EX pcm;

    TRACE("(%p)->(%x, %x, %s, %s, %p, %s)\n", This, mode, flags,
          wine_dbgstr_longlong(duration), wine_dbgstr_longlong(period), fmt, debugstr_guid(sessionguid));

    if(!fmt)
        return E_POINTER;

    dump_fmt(fmt);

    if(mode != AUDCLNT_SHAREMODE_SHARED && mode != AUDCLNT_SHAREMODE_EXCLUSIVE)
        return E_INVALIDARG;

    if(flags & ~(AUDCLNT_STREAMFLAGS_CROSSPROCESS |
                AUDCLNT_STREAMFLAGS_LOOPBACK |
                AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                AUDCLNT_STREAMFLAGS_NOPERSIST |
                AUDCLNT_STREAMFLAGS_RATEADJUST |
                AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED |
                AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE |
                AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED |
                AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY |
                AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM)){
        FIXME("Unknown flags: %08x\n", flags);
        return E_INVALIDARG;
    }

    if(mode == AUDCLNT_SHAREMODE_SHARED){
        period = DefaultPeriod;
        if( duration < 3 * period)
            duration = 3 * period;
    }else{
        if(!period)
            period = DefaultPeriod; /* not minimum */
        if(period < MinimumPeriod || period > 5000000)
            return AUDCLNT_E_INVALID_DEVICE_PERIOD;
        if(duration > 20000000) /* the smaller the period, the lower this limit */
            return AUDCLNT_E_BUFFER_SIZE_ERROR;
        if(flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK){
            if(duration != period)
                return AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL;
            FIXME("EXCLUSIVE mode with EVENTCALLBACK\n");
            return AUDCLNT_E_DEVICE_IN_USE;
        }else{
            if( duration < 8 * period)
                duration = 8 * period; /* may grow above 2s */
        }
    }

    EnterCriticalSection(&This->lock);

    if(This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_ALREADY_INITIALIZED;
    }

    This->period_us = period / 10;
    This->period_frames = MulDiv(fmt->nSamplesPerSec, period, 10000000);

    This->bufsize_frames = MulDiv(duration, fmt->nSamplesPerSec, 10000000);
    if(mode == AUDCLNT_SHAREMODE_EXCLUSIVE)
        This->bufsize_frames -= This->bufsize_frames % This->period_frames;
    else if(This->bufsize_frames % This->period_frames != 0)
        /* hack: round up to integer multiple */
        This->bufsize_frames += This->period_frames - This->bufsize_frames % This->period_frames;

    hr = waveformat_to_pcm(This, fmt, &pcm);
    if(FAILED(hr)){
        LeaveCriticalSection(&This->lock);
        return hr;
    }

    num_buffers = This->bufsize_frames / This->period_frames;

    if(This->dataflow == eRender){
        hr = try_open_render_device(&pcm, num_buffers, &This->player);
        if(FAILED(hr)){
            LeaveCriticalSection(&This->lock);
            return hr;
        }

        sr = SLCALL(This->player, GetInterface, *pSL_IID_ANDROIDSIMPLEBUFFERQUEUE, &This->bufq);
        if(sr != SL_RESULT_SUCCESS){
            SLCALL_N(This->player, Destroy);
            This->player = NULL;
            WARN("Player GetInterface(BufferQueue) failed: 0x%x\n", sr);
            LeaveCriticalSection(&This->lock);
            return E_FAIL;
        }

        sr = SLCALL(This->player, GetInterface, *pSL_IID_PLAY, &This->playitf);
        if(sr != SL_RESULT_SUCCESS){
            SLCALL_N(This->player, Destroy);
            This->player = NULL;
            WARN("Player GetInterface(Play) failed: 0x%x\n", sr);
            LeaveCriticalSection(&This->lock);
            return E_FAIL;
        }
    }else{
        hr = try_open_capture_device(&pcm, num_buffers, &This->recorder);
        if(FAILED(hr)){
            LeaveCriticalSection(&This->lock);
            return hr;
        }

        sr = SLCALL(This->recorder, GetInterface, *pSL_IID_ANDROIDSIMPLEBUFFERQUEUE, &This->bufq);
        if(sr != SL_RESULT_SUCCESS){
            SLCALL_N(This->recorder, Destroy);
            This->recorder = NULL;
            WARN("Recorder GetInterface(BufferQueue) failed: 0x%x\n", sr);
            LeaveCriticalSection(&This->lock);
            return E_FAIL;
        }

        sr = SLCALL(This->recorder, GetInterface, *pSL_IID_RECORD, &This->recorditf);
        if(sr != SL_RESULT_SUCCESS){
            SLCALL_N(This->recorder, Destroy);
            This->recorder = NULL;
            WARN("Recorder GetInterface(Record) failed: 0x%x\n", sr);
            LeaveCriticalSection(&This->lock);
            return E_FAIL;
        }
    }

    This->fmt = clone_format(fmt);
    if(!This->fmt){
        if(This->player){
            SLCALL_N(This->player, Destroy);
            This->player = NULL;
        }
        if(This->recorder){
            SLCALL_N(This->recorder, Destroy);
            This->recorder = NULL;
        }
        LeaveCriticalSection(&This->lock);
        return E_OUTOFMEMORY;
    }

    This->local_buffer = HeapAlloc(GetProcessHeap(), 0,
            This->bufsize_frames * fmt->nBlockAlign);
    if(!This->local_buffer){
        CoTaskMemFree(This->fmt);
        This->fmt = NULL;
        if(This->player){
            SLCALL_N(This->player, Destroy);
            This->player = NULL;
        }
        if(This->recorder){
            SLCALL_N(This->recorder, Destroy);
            This->recorder = NULL;
        }
        LeaveCriticalSection(&This->lock);
        return E_OUTOFMEMORY;
    }

    if(This->dataflow == eCapture){
        while(This->in_sl_frames < This->bufsize_frames){
            TRACE("enqueueing: %u frames from %u\n", This->period_frames, (This->lcl_offs_frames + This->in_sl_frames) % This->bufsize_frames);
            sr = SLCALL(This->bufq, Enqueue,
                    This->local_buffer + ((This->lcl_offs_frames + This->in_sl_frames) % This->bufsize_frames) * This->fmt->nBlockAlign,
                    This->period_frames * This->fmt->nBlockAlign);
            if(sr != SL_RESULT_SUCCESS)
                WARN("Enqueue failed: 0x%x\n", sr);
            This->in_sl_frames += This->period_frames;
        }
    }

    This->vols = HeapAlloc(GetProcessHeap(), 0, fmt->nChannels * sizeof(float));
    if(!This->vols){
        CoTaskMemFree(This->fmt);
        This->fmt = NULL;
        if(This->player){
            SLCALL_N(This->player, Destroy);
            This->player = NULL;
        }
        if(This->recorder){
            SLCALL_N(This->recorder, Destroy);
            This->recorder = NULL;
        }
        LeaveCriticalSection(&This->lock);
        return E_OUTOFMEMORY;
    }

    for(i = 0; i < fmt->nChannels; ++i)
        This->vols[i] = 1.f;

    This->share = mode;
    This->flags = flags;
    This->oss_bufsize_bytes = 0;

    EnterCriticalSection(&g_sessions_lock);

    hr = get_audio_session(sessionguid, This->parent, fmt->nChannels,
            &This->session);
    if(FAILED(hr)){
        LeaveCriticalSection(&g_sessions_lock);
        HeapFree(GetProcessHeap(), 0, This->vols);
        This->vols = NULL;
        CoTaskMemFree(This->fmt);
        This->fmt = NULL;
        if(This->player){
            SLCALL_N(This->player, Destroy);
            This->player = NULL;
        }
        if(This->recorder){
            SLCALL_N(This->recorder, Destroy);
            This->recorder = NULL;
        }
        LeaveCriticalSection(&This->lock);
        return hr;
    }

    list_add_tail(&This->session->clients, &This->entry);

    LeaveCriticalSection(&g_sessions_lock);

    This->initted = TRUE;

    TRACE("numBuffers: %u, bufsize: %u, period: %u\n", num_buffers,
            This->bufsize_frames, This->period_frames);

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetBufferSize(IAudioClient3 *iface,
        UINT32 *frames)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%p)\n", This, frames);

    if(!frames)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    *frames = This->bufsize_frames;

    TRACE("buffer size: %u\n", *frames);

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetStreamLatency(IAudioClient3 *iface,
        REFERENCE_TIME *latency)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%p)\n", This, latency);

    if(!latency)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    /* pretend we process audio in Period chunks, so max latency includes
     * the period time.  Some native machines add .6666ms in shared mode. */
    *latency = This->period_us * 10 + 6666;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetCurrentPadding(IAudioClient3 *iface,
        UINT32 *numpad)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%p)\n", This, numpad);

    if(!numpad)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    *numpad = This->held_frames;

    TRACE("padding: %u\n", *numpad);

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_IsFormatSupported(IAudioClient3 *iface,
        AUDCLNT_SHAREMODE mode, const WAVEFORMATEX *pwfx,
        WAVEFORMATEX **outpwfx)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    SLAndroidDataFormat_PCM_EX pcm;
    HRESULT hr;

    TRACE("(%p)->(%x, %p, %p)\n", This, mode, pwfx, outpwfx);

    if(!pwfx || (mode == AUDCLNT_SHAREMODE_SHARED && !outpwfx))
        return E_POINTER;

    if(mode != AUDCLNT_SHAREMODE_SHARED && mode != AUDCLNT_SHAREMODE_EXCLUSIVE)
        return E_INVALIDARG;

    if(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
            pwfx->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
        return E_INVALIDARG;

    dump_fmt(pwfx);

    if(outpwfx)
        *outpwfx = NULL;

    hr = waveformat_to_pcm(This, pwfx, &pcm);
    if(SUCCEEDED(hr)){
        if(This->dataflow == eRender){
            hr = try_open_render_device(&pcm, 10, NULL);
        }else{
            hr = try_open_capture_device(&pcm, 10, NULL);
        }
    }

    if(FAILED(hr)){
        if(outpwfx){
            hr = IAudioClient3_GetMixFormat(iface, outpwfx);
            if(FAILED(hr))
                return hr;
            return S_FALSE;
        }

        hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
    }

    TRACE("returning: %08x\n", hr);

    return hr;
}

static HRESULT WINAPI AudioClient_GetMixFormat(IAudioClient3 *iface,
        WAVEFORMATEX **pwfx)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    WAVEFORMATEXTENSIBLE *fmt;

    TRACE("(%p)->(%p)\n", This, pwfx);

    if(!pwfx)
        return E_POINTER;
    *pwfx = NULL;

    fmt = CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
    if(!fmt)
        return E_OUTOFMEMORY;

    fmt->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    fmt->Format.wBitsPerSample = 16;
    fmt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    if(This->dataflow == eRender)
        fmt->Format.nChannels = 2;
    else
        fmt->Format.nChannels = 1;
    fmt->Format.nSamplesPerSec = 48000; /* TODO: query supported? recording? */
    fmt->Format.nBlockAlign = (fmt->Format.wBitsPerSample *
            fmt->Format.nChannels) / 8;
    fmt->Format.nAvgBytesPerSec = fmt->Format.nSamplesPerSec *
        fmt->Format.nBlockAlign;
    fmt->Samples.wValidBitsPerSample = fmt->Format.wBitsPerSample;
    fmt->dwChannelMask = get_channel_mask(fmt->Format.nChannels);
    fmt->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

    *pwfx = (WAVEFORMATEX*)fmt;
    dump_fmt(*pwfx);

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetDevicePeriod(IAudioClient3 *iface,
        REFERENCE_TIME *defperiod, REFERENCE_TIME *minperiod)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%p, %p)\n", This, defperiod, minperiod);

    if(!defperiod && !minperiod)
        return E_POINTER;

    if(defperiod)
        *defperiod = DefaultPeriod;
    if(minperiod)
        *minperiod = MinimumPeriod;

    return S_OK;
}

static void silence_buffer(ACImpl *This, BYTE *buffer, UINT32 frames)
{
    WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE*)This->fmt;
    if((This->fmt->wFormatTag == WAVE_FORMAT_PCM ||
            (This->fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))) &&
            This->fmt->wBitsPerSample == 8)
        memset(buffer, 128, frames * This->fmt->nBlockAlign);
    else
        memset(buffer, 0, frames * This->fmt->nBlockAlign);
}

static void sl_read_data(ACImpl *This)
{
    SLAndroidSimpleBufferQueueState state;
    SLresult sr;
    SLuint32 elapsed;

    memset(&state, 0, sizeof(state));

    sr = SLCALL(This->bufq, GetState, &state);
    if(sr != SL_RESULT_SUCCESS){
        WARN("GetState failed: 0x%x\n", sr);
        return;
    }
    TRACE("got: count: %u, index: %u, held: %u, in_sl: %u\n", state.count, state.index, This->held_frames, This->in_sl_frames);

    elapsed = This->in_sl_frames - state.count * This->period_frames;
    This->held_frames += elapsed;
    This->in_sl_frames = state.count * This->period_frames;

    if(This->held_frames == This->bufsize_frames){
        /* overrun */
        TRACE("overrun??\n");
        This->lcl_offs_frames += This->period_frames;
        This->held_frames -= This->period_frames;
    }

    TRACE("good range: %u, %u\n", This->lcl_offs_frames, This->lcl_offs_frames + This->held_frames);
    TRACE("held: %u, in_sl: %u\n", This->held_frames, This->in_sl_frames);
    while(This->held_frames + This->in_sl_frames < This->bufsize_frames){
        TRACE("enqueueing: %u frames from %u\n", This->period_frames, (This->lcl_offs_frames + This->held_frames + This->in_sl_frames) % This->bufsize_frames);
        sr = SLCALL(This->bufq, Enqueue,
                This->local_buffer + ((This->lcl_offs_frames + This->held_frames + This->in_sl_frames) % This->bufsize_frames) * This->fmt->nBlockAlign,
                This->period_frames * This->fmt->nBlockAlign);
        if(sr != SL_RESULT_SUCCESS)
            WARN("Enqueue failed: 0x%x\n", sr);
        This->in_sl_frames += This->period_frames;
    }
}

static DWORD wrap_enqueue(ACImpl *This)
{
    DWORD to_enqueue = min(This->held_frames, This->period_frames);
    DWORD offs = (This->lcl_offs_frames + This->in_sl_frames) % This->bufsize_frames;
    BYTE *buf = This->local_buffer + offs * This->fmt->nBlockAlign;

    if(offs + to_enqueue > This->bufsize_frames){
        DWORD chunk = This->bufsize_frames - offs;

        if(This->wrap_buffer_frames < to_enqueue){
            HeapFree(GetProcessHeap(), 0, This->wrap_buffer);
            This->wrap_buffer = HeapAlloc(GetProcessHeap(), 0, to_enqueue * This->fmt->nBlockAlign);
            This->wrap_buffer_frames = to_enqueue;
        }

        memcpy(This->wrap_buffer, This->local_buffer + offs * This->fmt->nBlockAlign, chunk * This->fmt->nBlockAlign);
        memcpy(This->wrap_buffer + chunk * This->fmt->nBlockAlign, This->local_buffer, (to_enqueue - chunk) * This->fmt->nBlockAlign);

        buf = This->wrap_buffer;
    }

    SLCALL(This->bufq, Enqueue, buf,
            to_enqueue * This->fmt->nBlockAlign);

    return to_enqueue;
}

static void sl_write_data(ACImpl *This)
{
    SLAndroidSimpleBufferQueueState state;
    SLresult sr;
    SLuint32 elapsed;

    memset(&state, 0, sizeof(state));

    sr = SLCALL(This->bufq, GetState, &state);
    if(sr != SL_RESULT_SUCCESS){
        WARN("GetState failed: 0x%x\n", sr);
        return;
    }
    TRACE("got: count: %u, index: %u\n", state.count, state.index);

    elapsed = This->in_sl_frames - state.count * This->period_frames;

    if(elapsed > This->held_frames)
        This->held_frames = 0;
    else
        This->held_frames -= elapsed;

    This->lcl_offs_frames += elapsed;
    This->lcl_offs_frames %= This->bufsize_frames;

    This->in_sl_frames = state.count * This->period_frames;

    while(This->held_frames >= This->in_sl_frames + This->period_frames){
        /* have at least a period to write, so write it */
        TRACE("enqueueing: %u frames from %u\n", This->period_frames, (This->lcl_offs_frames + This->in_sl_frames) % This->bufsize_frames);
        This->in_sl_frames += wrap_enqueue(This);
    }

    if(This->held_frames && This->in_sl_frames < This->period_frames * 3){
        /* write out the last bit with a partial period */
        TRACE("enqueueing partial period: %u frames from %u\n", This->held_frames, (This->lcl_offs_frames + This->in_sl_frames) % This->bufsize_frames);
        This->in_sl_frames += wrap_enqueue(This);
    }

    TRACE("done with enqueue, lcl_offs: %u, in_sl: %u, held: %u\n", This->lcl_offs_frames, This->in_sl_frames, This->held_frames);
}

static void CALLBACK sl_period_callback(void *user, BOOLEAN timer)
{
    ACImpl *This = user;

    EnterCriticalSection(&This->lock);

    if(This->playing){
        if(This->dataflow == eRender)
            sl_write_data(This);
        else if(This->dataflow == eCapture)
            sl_read_data(This);
    }

    LeaveCriticalSection(&This->lock);

    if(This->event)
        SetEvent(This->event);
}

static HRESULT WINAPI AudioClient_Start(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    SLresult sr;

    TRACE("(%p)\n", This);

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if((This->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) && !This->event){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_EVENTHANDLE_NOT_SET;
    }

    if(This->playing){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_STOPPED;
    }

    if(This->dataflow == eRender){
        sr = SLCALL(This->playitf, SetPlayState, SL_PLAYSTATE_PLAYING);
        if(sr != SL_RESULT_SUCCESS){
            WARN("SetPlayState failed: 0x%x\n", sr);
            LeaveCriticalSection(&This->lock);
            return E_FAIL;
        }
    }else{
        sr = SLCALL(This->recorditf, SetRecordState, SL_RECORDSTATE_RECORDING);
        if(sr != SL_RESULT_SUCCESS){
            WARN("SetRecordState failed: 0x%x\n", sr);
            LeaveCriticalSection(&This->lock);
            return E_FAIL;
        }
    }

    if(!This->timer){
        if(!CreateTimerQueueTimer(&This->timer, g_timer_q,
                    sl_period_callback, This, 0, This->period_us / 1000,
                    WT_EXECUTEINTIMERTHREAD))
            WARN("Unable to create period timer: %u\n", GetLastError());
    }

    This->playing = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_Stop(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    SLresult sr;

    TRACE("(%p)\n", This);

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(!This->playing){
        LeaveCriticalSection(&This->lock);
        return S_FALSE;
    }

    if(This->dataflow == eRender){
        sr = SLCALL(This->playitf, SetPlayState, SL_PLAYSTATE_PAUSED);
        if(sr != SL_RESULT_SUCCESS){
            WARN("SetPlayState failed: 0x%x\n", sr);
            LeaveCriticalSection(&This->lock);
            return E_FAIL;
        }
    }else{
        sr = SLCALL(This->recorditf, SetRecordState, SL_RECORDSTATE_STOPPED);
        if(sr != SL_RESULT_SUCCESS){
            WARN("SetRecordState failed: 0x%x\n", sr);
            LeaveCriticalSection(&This->lock);
            return E_FAIL;
        }
    }

    This->playing = FALSE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_Reset(IAudioClient3 *iface)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    SLresult sr;

    TRACE("(%p)\n", This);

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(This->playing){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_STOPPED;
    }

    if(This->getbuf_last){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_BUFFER_OPERATION_PENDING;
    }

    sr = SLCALL_N(This->bufq, Clear);
    if(sr != SL_RESULT_SUCCESS){
        WARN("Clear failed: 0x%x\n", sr);
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    This->lcl_offs_frames = 0;
    This->in_sl_frames = 0;

    if(This->dataflow == eRender){
        This->written_frames = 0;
        This->last_pos_frames = 0;
    }else{
        This->written_frames += This->held_frames;
        while(This->in_sl_frames < This->bufsize_frames){
            TRACE("enqueueing: %u frames from %u\n", This->period_frames, (This->lcl_offs_frames + This->in_sl_frames) % This->bufsize_frames);
            sr = SLCALL(This->bufq, Enqueue,
                    This->local_buffer + ((This->lcl_offs_frames + This->in_sl_frames) % This->bufsize_frames) * This->fmt->nBlockAlign,
                    This->period_frames * This->fmt->nBlockAlign);
            if(sr != SL_RESULT_SUCCESS)
                WARN("Enqueue failed: 0x%x\n", sr);
            This->in_sl_frames += This->period_frames;
        }
    }

    This->held_frames = 0;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_SetEventHandle(IAudioClient3 *iface,
        HANDLE event)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%p)\n", This, event);

    if(!event)
        return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(!(This->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK)){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED;
    }

    if (This->event){
        LeaveCriticalSection(&This->lock);
        FIXME("called twice\n");
        return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    }

    This->event = event;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetService(IAudioClient3 *iface, REFIID riid,
        void **ppv)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(IsEqualIID(riid, &IID_IAudioRenderClient)){
        if(This->dataflow != eRender){
            LeaveCriticalSection(&This->lock);
            return AUDCLNT_E_WRONG_ENDPOINT_TYPE;
        }
        IAudioRenderClient_AddRef(&This->IAudioRenderClient_iface);
        *ppv = &This->IAudioRenderClient_iface;
    }else if(IsEqualIID(riid, &IID_IAudioCaptureClient)){
        if(This->dataflow != eCapture){
            LeaveCriticalSection(&This->lock);
            return AUDCLNT_E_WRONG_ENDPOINT_TYPE;
        }
        IAudioCaptureClient_AddRef(&This->IAudioCaptureClient_iface);
        *ppv = &This->IAudioCaptureClient_iface;
    }else if(IsEqualIID(riid, &IID_IAudioClock)){
        IAudioClock_AddRef(&This->IAudioClock_iface);
        *ppv = &This->IAudioClock_iface;
    }else if(IsEqualIID(riid, &IID_IAudioStreamVolume)){
        IAudioStreamVolume_AddRef(&This->IAudioStreamVolume_iface);
        *ppv = &This->IAudioStreamVolume_iface;
    }else if(IsEqualIID(riid, &IID_IAudioSessionControl)){
        if(!This->session_wrapper){
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if(!This->session_wrapper){
                LeaveCriticalSection(&This->lock);
                return E_OUTOFMEMORY;
            }
        }else
            IAudioSessionControl2_AddRef(&This->session_wrapper->IAudioSessionControl2_iface);

        *ppv = &This->session_wrapper->IAudioSessionControl2_iface;
    }else if(IsEqualIID(riid, &IID_IChannelAudioVolume)){
        if(!This->session_wrapper){
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if(!This->session_wrapper){
                LeaveCriticalSection(&This->lock);
                return E_OUTOFMEMORY;
            }
        }else
            IChannelAudioVolume_AddRef(&This->session_wrapper->IChannelAudioVolume_iface);

        *ppv = &This->session_wrapper->IChannelAudioVolume_iface;
    }else if(IsEqualIID(riid, &IID_ISimpleAudioVolume)){
        if(!This->session_wrapper){
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if(!This->session_wrapper){
                LeaveCriticalSection(&This->lock);
                return E_OUTOFMEMORY;
            }
        }else
            ISimpleAudioVolume_AddRef(&This->session_wrapper->ISimpleAudioVolume_iface);

        *ppv = &This->session_wrapper->ISimpleAudioVolume_iface;
    }

    if(*ppv){
        LeaveCriticalSection(&This->lock);
        return S_OK;
    }

    LeaveCriticalSection(&This->lock);

    FIXME("stub %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI AudioClient_IsOffloadCapable(IAudioClient3 *iface,
        AUDIO_STREAM_CATEGORY category, BOOL *offload_capable)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    TRACE("(%p)->(0x%x, %p)\n", This, category, offload_capable);

    if(!offload_capable)
        return E_INVALIDARG;

    *offload_capable = FALSE;

    return S_OK;
}

static HRESULT WINAPI AudioClient_SetClientProperties(IAudioClient3 *iface,
        const AudioClientProperties *prop)
{
    ACImpl *This = impl_from_IAudioClient3(iface);
    const Win8AudioClientProperties *legacy_prop = (const Win8AudioClientProperties *)prop;

    TRACE("(%p)->(%p)\n", This, prop);

    if(!legacy_prop)
        return E_POINTER;

    if(legacy_prop->cbSize == sizeof(AudioClientProperties)){
        TRACE("{ bIsOffload: %u, eCategory: 0x%x, Options: 0x%x }\n",
                legacy_prop->bIsOffload,
                legacy_prop->eCategory,
                prop->Options);
    }else if(legacy_prop->cbSize == sizeof(Win8AudioClientProperties)){
        TRACE("{ bIsOffload: %u, eCategory: 0x%x }\n",
                legacy_prop->bIsOffload,
                legacy_prop->eCategory);
    }else{
        WARN("Unsupported Size = %d\n", legacy_prop->cbSize);
        return E_INVALIDARG;
    }


    if(legacy_prop->bIsOffload)
        return AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE;

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetBufferSizeLimits(IAudioClient3 *iface,
        const WAVEFORMATEX *format, BOOL event_driven, REFERENCE_TIME *min_duration,
        REFERENCE_TIME *max_duration)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    FIXME("(%p)->(%p, %u, %p, %p)\n", This, format, event_driven, min_duration, max_duration);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioClient_GetSharedModeEnginePeriod(IAudioClient3 *iface,
        const WAVEFORMATEX *format, UINT32 *default_period_frames, UINT32 *unit_period_frames,
        UINT32 *min_period_frames, UINT32 *max_period_frames)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    FIXME("(%p)->(%p, %p, %p, %p, %p)\n", This, format, default_period_frames, unit_period_frames,
            min_period_frames, max_period_frames);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioClient_GetCurrentSharedModeEnginePeriod(IAudioClient3 *iface,
        WAVEFORMATEX **cur_format, UINT32 *cur_period_frames)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    FIXME("(%p)->(%p, %p)\n", This, cur_format, cur_period_frames);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioClient_InitializeSharedAudioStream(IAudioClient3 *iface,
        DWORD flags, UINT32 period_frames, const WAVEFORMATEX *format,
        const GUID *session_guid)
{
    ACImpl *This = impl_from_IAudioClient3(iface);

    FIXME("(%p)->(0x%x, %u, %p, %s)\n", This, flags, period_frames, format, debugstr_guid(session_guid));

    return E_NOTIMPL;
}

static const IAudioClient3Vtbl AudioClient3_Vtbl =
{
    AudioClient_QueryInterface,
    AudioClient_AddRef,
    AudioClient_Release,
    AudioClient_Initialize,
    AudioClient_GetBufferSize,
    AudioClient_GetStreamLatency,
    AudioClient_GetCurrentPadding,
    AudioClient_IsFormatSupported,
    AudioClient_GetMixFormat,
    AudioClient_GetDevicePeriod,
    AudioClient_Start,
    AudioClient_Stop,
    AudioClient_Reset,
    AudioClient_SetEventHandle,
    AudioClient_GetService,
    AudioClient_IsOffloadCapable,
    AudioClient_SetClientProperties,
    AudioClient_GetBufferSizeLimits,
    AudioClient_GetSharedModeEnginePeriod,
    AudioClient_GetCurrentSharedModeEnginePeriod,
    AudioClient_InitializeSharedAudioStream,
};

static HRESULT WINAPI AudioRenderClient_QueryInterface(
        IAudioRenderClient *iface, REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioRenderClient))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->pUnkFTMarshal, riid, ppv);
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioRenderClient_AddRef(IAudioRenderClient *iface)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    return AudioClient_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI AudioRenderClient_Release(IAudioRenderClient *iface)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    return AudioClient_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI AudioRenderClient_GetBuffer(IAudioRenderClient *iface,
        UINT32 frames, BYTE **data)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    UINT32 write_pos;

    TRACE("(%p)->(%u, %p)\n", This, frames, data);

    if(!data)
        return E_POINTER;

    *data = NULL;

    EnterCriticalSection(&This->lock);

    if(This->getbuf_last){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    if(!frames){
        LeaveCriticalSection(&This->lock);
        return S_OK;
    }

    if(This->held_frames + frames > This->bufsize_frames){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_BUFFER_TOO_LARGE;
    }

    write_pos =
        (This->lcl_offs_frames + This->held_frames) % This->bufsize_frames;
    if(write_pos + frames > This->bufsize_frames){
        if(This->tmp_buffer_frames < frames){
            DWORD alloc = frames < This->period_frames ? This->period_frames : frames;
            HeapFree(GetProcessHeap(), 0, This->tmp_buffer);
            This->tmp_buffer = HeapAlloc(GetProcessHeap(), 0,
                    alloc * This->fmt->nBlockAlign);
            if(!This->tmp_buffer){
                LeaveCriticalSection(&This->lock);
                return E_OUTOFMEMORY;
            }
            This->tmp_buffer_frames = alloc;
        }
        *data = This->tmp_buffer;
        This->getbuf_last = -frames;
    }else{
        *data = This->local_buffer + write_pos * This->fmt->nBlockAlign;
        This->getbuf_last = frames;
    }

    silence_buffer(This, *data, frames);

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static void oss_wrap_buffer(ACImpl *This, BYTE *buffer, UINT32 written_frames)
{
    UINT32 write_offs_frames =
        (This->lcl_offs_frames + This->held_frames) % This->bufsize_frames;
    UINT32 write_offs_bytes = write_offs_frames * This->fmt->nBlockAlign;
    UINT32 chunk_frames = This->bufsize_frames - write_offs_frames;
    UINT32 chunk_bytes = chunk_frames * This->fmt->nBlockAlign;
    UINT32 written_bytes = written_frames * This->fmt->nBlockAlign;

    if(written_bytes <= chunk_bytes){
        memcpy(This->local_buffer + write_offs_bytes, buffer, written_bytes);
    }else{
        memcpy(This->local_buffer + write_offs_bytes, buffer, chunk_bytes);
        memcpy(This->local_buffer, buffer + chunk_bytes,
                written_bytes - chunk_bytes);
    }
}

static HRESULT WINAPI AudioRenderClient_ReleaseBuffer(
        IAudioRenderClient *iface, UINT32 written_frames, DWORD flags)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    BYTE *buffer;

    TRACE("(%p)->(%u, %x)\n", This, written_frames, flags);

    EnterCriticalSection(&This->lock);

    if(!written_frames){
        This->getbuf_last = 0;
        LeaveCriticalSection(&This->lock);
        return S_OK;
    }

    if(!This->getbuf_last){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    if(written_frames > (This->getbuf_last >= 0 ? This->getbuf_last : -This->getbuf_last)){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_INVALID_SIZE;
    }

    if(This->getbuf_last >= 0)
        buffer = This->local_buffer + This->fmt->nBlockAlign *
          ((This->lcl_offs_frames + This->held_frames) % This->bufsize_frames);
    else
        buffer = This->tmp_buffer;

    if(flags & AUDCLNT_BUFFERFLAGS_SILENT)
        silence_buffer(This, buffer, written_frames);

    if(This->getbuf_last < 0)
        oss_wrap_buffer(This, buffer, written_frames);

    This->held_frames += written_frames;
    This->written_frames += written_frames;
    This->getbuf_last = 0;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static const IAudioRenderClientVtbl AudioRenderClient_Vtbl = {
    AudioRenderClient_QueryInterface,
    AudioRenderClient_AddRef,
    AudioRenderClient_Release,
    AudioRenderClient_GetBuffer,
    AudioRenderClient_ReleaseBuffer
};

static HRESULT WINAPI AudioCaptureClient_QueryInterface(
        IAudioCaptureClient *iface, REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioCaptureClient))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->pUnkFTMarshal, riid, ppv);
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioCaptureClient_AddRef(IAudioCaptureClient *iface)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI AudioCaptureClient_Release(IAudioCaptureClient *iface)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI AudioCaptureClient_GetBuffer(IAudioCaptureClient *iface,
        BYTE **data, UINT32 *frames, DWORD *flags, UINT64 *devpos,
        UINT64 *qpcpos)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);

    TRACE("(%p)->(%p, %p, %p, %p, %p)\n", This, data, frames, flags,
            devpos, qpcpos);

    if(!data)
        return E_POINTER;

    *data = NULL;

    if(!frames || !flags)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    if(This->getbuf_last){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    if(This->held_frames < This->period_frames){
        *frames = 0;
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_S_BUFFER_EMPTY;
    }

    *flags = 0;

    *frames = This->period_frames;

    if(This->lcl_offs_frames + *frames > This->bufsize_frames){
        UINT32 chunk_bytes, offs_bytes, frames_bytes;
        if(This->tmp_buffer_frames < *frames){
            HeapFree(GetProcessHeap(), 0, This->tmp_buffer);
            This->tmp_buffer = HeapAlloc(GetProcessHeap(), 0,
                    *frames * This->fmt->nBlockAlign);
            if(!This->tmp_buffer){
                LeaveCriticalSection(&This->lock);
                return E_OUTOFMEMORY;
            }
            This->tmp_buffer_frames = *frames;
        }

        *data = This->tmp_buffer;
        chunk_bytes = (This->bufsize_frames - This->lcl_offs_frames) *
            This->fmt->nBlockAlign;
        offs_bytes = This->lcl_offs_frames * This->fmt->nBlockAlign;
        frames_bytes = *frames * This->fmt->nBlockAlign;
        memcpy(This->tmp_buffer, This->local_buffer + offs_bytes, chunk_bytes);
        memcpy(This->tmp_buffer + chunk_bytes, This->local_buffer,
                frames_bytes - chunk_bytes);
    }else
        *data = This->local_buffer +
            This->lcl_offs_frames * This->fmt->nBlockAlign;
    TRACE("returning %u from %u\n", This->period_frames, This->lcl_offs_frames);

    This->getbuf_last = *frames;

    if(devpos)
       *devpos = This->written_frames;
    if(qpcpos){
        LARGE_INTEGER stamp, freq;
        QueryPerformanceCounter(&stamp);
        QueryPerformanceFrequency(&freq);
        *qpcpos = (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
    }

    LeaveCriticalSection(&This->lock);

    return *frames ? S_OK : AUDCLNT_S_BUFFER_EMPTY;
}

static HRESULT WINAPI AudioCaptureClient_ReleaseBuffer(
        IAudioCaptureClient *iface, UINT32 done)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);

    TRACE("(%p)->(%u)\n", This, done);

    EnterCriticalSection(&This->lock);

    if(!done){
        This->getbuf_last = 0;
        LeaveCriticalSection(&This->lock);
        return S_OK;
    }

    if(!This->getbuf_last){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    if(This->getbuf_last != done){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_INVALID_SIZE;
    }

    This->written_frames += done;
    This->held_frames -= done;
    This->lcl_offs_frames += done;
    This->lcl_offs_frames %= This->bufsize_frames;
    This->getbuf_last = 0;
    TRACE("lcl: %u, held: %u\n", This->lcl_offs_frames, This->held_frames);

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioCaptureClient_GetNextPacketSize(
        IAudioCaptureClient *iface, UINT32 *frames)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);

    TRACE("(%p)->(%p)\n", This, frames);

    if(!frames)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    *frames = This->held_frames < This->period_frames ? 0 : This->period_frames;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static const IAudioCaptureClientVtbl AudioCaptureClient_Vtbl =
{
    AudioCaptureClient_QueryInterface,
    AudioCaptureClient_AddRef,
    AudioCaptureClient_Release,
    AudioCaptureClient_GetBuffer,
    AudioCaptureClient_ReleaseBuffer,
    AudioCaptureClient_GetNextPacketSize
};

static HRESULT WINAPI AudioClock_QueryInterface(IAudioClock *iface,
        REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IAudioClock))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IAudioClock2))
        *ppv = &This->IAudioClock2_iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioClock_AddRef(IAudioClock *iface)
{
    ACImpl *This = impl_from_IAudioClock(iface);
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI AudioClock_Release(IAudioClock *iface)
{
    ACImpl *This = impl_from_IAudioClock(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI AudioClock_GetFrequency(IAudioClock *iface, UINT64 *freq)
{
    ACImpl *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%p)\n", This, freq);

    if(This->share == AUDCLNT_SHAREMODE_SHARED)
        *freq = (UINT64)This->fmt->nSamplesPerSec * This->fmt->nBlockAlign;
    else
        *freq = This->fmt->nSamplesPerSec;

    return S_OK;
}

static HRESULT WINAPI AudioClock_GetPosition(IAudioClock *iface, UINT64 *pos,
        UINT64 *qpctime)
{
    ACImpl *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%p, %p)\n", This, pos, qpctime);

    if(!pos)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    if(This->dataflow == eRender){
        *pos = This->written_frames - This->held_frames;
        if(*pos < This->last_pos_frames)
            *pos = This->last_pos_frames;
    }else if(This->dataflow == eCapture){
        *pos = This->written_frames - This->held_frames;
    }

    This->last_pos_frames = *pos;

    TRACE("returning: 0x%s\n", wine_dbgstr_longlong(*pos));
    if(This->share == AUDCLNT_SHAREMODE_SHARED)
        *pos *= This->fmt->nBlockAlign;

    LeaveCriticalSection(&This->lock);

    if(qpctime){
        LARGE_INTEGER stamp, freq;
        QueryPerformanceCounter(&stamp);
        QueryPerformanceFrequency(&freq);
        *qpctime = (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
    }

    return S_OK;
}

static HRESULT WINAPI AudioClock_GetCharacteristics(IAudioClock *iface,
        DWORD *chars)
{
    ACImpl *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%p)\n", This, chars);

    if(!chars)
        return E_POINTER;

    *chars = AUDIOCLOCK_CHARACTERISTIC_FIXED_FREQ;

    return S_OK;
}

static const IAudioClockVtbl AudioClock_Vtbl =
{
    AudioClock_QueryInterface,
    AudioClock_AddRef,
    AudioClock_Release,
    AudioClock_GetFrequency,
    AudioClock_GetPosition,
    AudioClock_GetCharacteristics
};

static HRESULT WINAPI AudioClock2_QueryInterface(IAudioClock2 *iface,
        REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioClock2(iface);
    return IAudioClock_QueryInterface(&This->IAudioClock_iface, riid, ppv);
}

static ULONG WINAPI AudioClock2_AddRef(IAudioClock2 *iface)
{
    ACImpl *This = impl_from_IAudioClock2(iface);
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI AudioClock2_Release(IAudioClock2 *iface)
{
    ACImpl *This = impl_from_IAudioClock2(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI AudioClock2_GetDevicePosition(IAudioClock2 *iface,
        UINT64 *pos, UINT64 *qpctime)
{
    ACImpl *This = impl_from_IAudioClock2(iface);

    FIXME("(%p)->(%p, %p)\n", This, pos, qpctime);

    return E_NOTIMPL;
}

static const IAudioClock2Vtbl AudioClock2_Vtbl =
{
    AudioClock2_QueryInterface,
    AudioClock2_AddRef,
    AudioClock2_Release,
    AudioClock2_GetDevicePosition
};

static AudioSessionWrapper *AudioSessionWrapper_Create(ACImpl *client)
{
    AudioSessionWrapper *ret;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(AudioSessionWrapper));
    if(!ret)
        return NULL;

    ret->IAudioSessionControl2_iface.lpVtbl = &AudioSessionControl2_Vtbl;
    ret->ISimpleAudioVolume_iface.lpVtbl = &SimpleAudioVolume_Vtbl;
    ret->IChannelAudioVolume_iface.lpVtbl = &ChannelAudioVolume_Vtbl;

    ret->ref = 1;

    ret->client = client;
    if(client){
        ret->session = client->session;
        AudioClient_AddRef(&client->IAudioClient3_iface);
    }

    return ret;
}

static HRESULT WINAPI AudioSessionControl_QueryInterface(
        IAudioSessionControl2 *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioSessionControl) ||
            IsEqualIID(riid, &IID_IAudioSessionControl2))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioSessionControl_AddRef(IAudioSessionControl2 *iface)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI AudioSessionControl_Release(IAudioSessionControl2 *iface)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    if(!ref){
        if(This->client){
            EnterCriticalSection(&This->client->lock);
            This->client->session_wrapper = NULL;
            LeaveCriticalSection(&This->client->lock);
            AudioClient_Release(&This->client->IAudioClient3_iface);
        }
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI AudioSessionControl_GetState(IAudioSessionControl2 *iface,
        AudioSessionState *state)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);
    ACImpl *client;

    TRACE("(%p)->(%p)\n", This, state);

    if(!state)
        return NULL_PTR_ERR;

    EnterCriticalSection(&g_sessions_lock);

    if(list_empty(&This->session->clients)){
        *state = AudioSessionStateExpired;
        LeaveCriticalSection(&g_sessions_lock);
        return S_OK;
    }

    LIST_FOR_EACH_ENTRY(client, &This->session->clients, ACImpl, entry){
        EnterCriticalSection(&client->lock);
        if(client->playing){
            *state = AudioSessionStateActive;
            LeaveCriticalSection(&client->lock);
            LeaveCriticalSection(&g_sessions_lock);
            return S_OK;
        }
        LeaveCriticalSection(&client->lock);
    }

    LeaveCriticalSection(&g_sessions_lock);

    *state = AudioSessionStateInactive;

    return S_OK;
}

static HRESULT WINAPI AudioSessionControl_GetDisplayName(
        IAudioSessionControl2 *iface, WCHAR **name)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_SetDisplayName(
        IAudioSessionControl2 *iface, const WCHAR *name, const GUID *session)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p, %s) - stub\n", This, name, debugstr_guid(session));

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_GetIconPath(
        IAudioSessionControl2 *iface, WCHAR **path)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_SetIconPath(
        IAudioSessionControl2 *iface, const WCHAR *path, const GUID *session)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p, %s) - stub\n", This, path, debugstr_guid(session));

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_GetGroupingParam(
        IAudioSessionControl2 *iface, GUID *group)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, group);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_SetGroupingParam(
        IAudioSessionControl2 *iface, const GUID *group, const GUID *session)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%s, %s) - stub\n", This, debugstr_guid(group),
            debugstr_guid(session));

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_RegisterAudioSessionNotification(
        IAudioSessionControl2 *iface, IAudioSessionEvents *events)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, events);

    return S_OK;
}

static HRESULT WINAPI AudioSessionControl_UnregisterAudioSessionNotification(
        IAudioSessionControl2 *iface, IAudioSessionEvents *events)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, events);

    return S_OK;
}

static HRESULT WINAPI AudioSessionControl_GetSessionIdentifier(
        IAudioSessionControl2 *iface, WCHAR **id)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_GetSessionInstanceIdentifier(
        IAudioSessionControl2 *iface, WCHAR **id)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_GetProcessId(
        IAudioSessionControl2 *iface, DWORD *pid)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    TRACE("(%p)->(%p)\n", This, pid);

    if(!pid)
        return E_POINTER;

    *pid = GetCurrentProcessId();

    return S_OK;
}

static HRESULT WINAPI AudioSessionControl_IsSystemSoundsSession(
        IAudioSessionControl2 *iface)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    TRACE("(%p)\n", This);

    return S_FALSE;
}

static HRESULT WINAPI AudioSessionControl_SetDuckingPreference(
        IAudioSessionControl2 *iface, BOOL optout)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    TRACE("(%p)->(%d)\n", This, optout);

    return S_OK;
}

static const IAudioSessionControl2Vtbl AudioSessionControl2_Vtbl =
{
    AudioSessionControl_QueryInterface,
    AudioSessionControl_AddRef,
    AudioSessionControl_Release,
    AudioSessionControl_GetState,
    AudioSessionControl_GetDisplayName,
    AudioSessionControl_SetDisplayName,
    AudioSessionControl_GetIconPath,
    AudioSessionControl_SetIconPath,
    AudioSessionControl_GetGroupingParam,
    AudioSessionControl_SetGroupingParam,
    AudioSessionControl_RegisterAudioSessionNotification,
    AudioSessionControl_UnregisterAudioSessionNotification,
    AudioSessionControl_GetSessionIdentifier,
    AudioSessionControl_GetSessionInstanceIdentifier,
    AudioSessionControl_GetProcessId,
    AudioSessionControl_IsSystemSoundsSession,
    AudioSessionControl_SetDuckingPreference
};

static HRESULT WINAPI SimpleAudioVolume_QueryInterface(
        ISimpleAudioVolume *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_ISimpleAudioVolume))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI SimpleAudioVolume_AddRef(ISimpleAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    return AudioSessionControl_AddRef(&This->IAudioSessionControl2_iface);
}

static ULONG WINAPI SimpleAudioVolume_Release(ISimpleAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    return AudioSessionControl_Release(&This->IAudioSessionControl2_iface);
}

static HRESULT WINAPI SimpleAudioVolume_SetMasterVolume(
        ISimpleAudioVolume *iface, float level, const GUID *context)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%f, %s)\n", session, level, wine_dbgstr_guid(context));

    if(level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if(context)
        FIXME("Notifications not supported yet\n");

    EnterCriticalSection(&session->lock);

    session->master_vol = level;

    TRACE("OSS doesn't support setting volume\n");

    LeaveCriticalSection(&session->lock);

    return S_OK;
}

static HRESULT WINAPI SimpleAudioVolume_GetMasterVolume(
        ISimpleAudioVolume *iface, float *level)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%p)\n", session, level);

    if(!level)
        return NULL_PTR_ERR;

    *level = session->master_vol;

    return S_OK;
}

static HRESULT WINAPI SimpleAudioVolume_SetMute(ISimpleAudioVolume *iface,
        BOOL mute, const GUID *context)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%u, %p)\n", session, mute, context);

    EnterCriticalSection(&session->lock);

    session->mute = mute;

    LeaveCriticalSection(&session->lock);

    return S_OK;
}

static HRESULT WINAPI SimpleAudioVolume_GetMute(ISimpleAudioVolume *iface,
        BOOL *mute)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%p)\n", session, mute);

    if(!mute)
        return NULL_PTR_ERR;

    *mute = This->session->mute;

    return S_OK;
}

static const ISimpleAudioVolumeVtbl SimpleAudioVolume_Vtbl  =
{
    SimpleAudioVolume_QueryInterface,
    SimpleAudioVolume_AddRef,
    SimpleAudioVolume_Release,
    SimpleAudioVolume_SetMasterVolume,
    SimpleAudioVolume_GetMasterVolume,
    SimpleAudioVolume_SetMute,
    SimpleAudioVolume_GetMute
};

static HRESULT WINAPI AudioStreamVolume_QueryInterface(
        IAudioStreamVolume *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioStreamVolume))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioStreamVolume_AddRef(IAudioStreamVolume *iface)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI AudioStreamVolume_Release(IAudioStreamVolume *iface)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI AudioStreamVolume_GetChannelCount(
        IAudioStreamVolume *iface, UINT32 *out)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%p)\n", This, out);

    if(!out)
        return E_POINTER;

    *out = This->fmt->nChannels;

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_SetChannelVolume(
        IAudioStreamVolume *iface, UINT32 index, float level)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%d, %f)\n", This, index, level);

    if(level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if(index >= This->fmt->nChannels)
        return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    This->vols[index] = level;

    TRACE("OSS doesn't support setting volume\n");

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_GetChannelVolume(
        IAudioStreamVolume *iface, UINT32 index, float *level)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%d, %p)\n", This, index, level);

    if(!level)
        return E_POINTER;

    if(index >= This->fmt->nChannels)
        return E_INVALIDARG;

    *level = This->vols[index];

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_SetAllVolumes(
        IAudioStreamVolume *iface, UINT32 count, const float *levels)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    int i;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if(!levels)
        return E_POINTER;

    if(count != This->fmt->nChannels)
        return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    for(i = 0; i < count; ++i)
        This->vols[i] = levels[i];

    TRACE("OSS doesn't support setting volume\n");

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_GetAllVolumes(
        IAudioStreamVolume *iface, UINT32 count, float *levels)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    int i;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if(!levels)
        return E_POINTER;

    if(count != This->fmt->nChannels)
        return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    for(i = 0; i < count; ++i)
        levels[i] = This->vols[i];

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl =
{
    AudioStreamVolume_QueryInterface,
    AudioStreamVolume_AddRef,
    AudioStreamVolume_Release,
    AudioStreamVolume_GetChannelCount,
    AudioStreamVolume_SetChannelVolume,
    AudioStreamVolume_GetChannelVolume,
    AudioStreamVolume_SetAllVolumes,
    AudioStreamVolume_GetAllVolumes
};

static HRESULT WINAPI ChannelAudioVolume_QueryInterface(
        IChannelAudioVolume *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IChannelAudioVolume))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ChannelAudioVolume_AddRef(IChannelAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    return AudioSessionControl_AddRef(&This->IAudioSessionControl2_iface);
}

static ULONG WINAPI ChannelAudioVolume_Release(IChannelAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    return AudioSessionControl_Release(&This->IAudioSessionControl2_iface);
}

static HRESULT WINAPI ChannelAudioVolume_GetChannelCount(
        IChannelAudioVolume *iface, UINT32 *out)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%p)\n", session, out);

    if(!out)
        return NULL_PTR_ERR;

    *out = session->channel_count;

    return S_OK;
}

static HRESULT WINAPI ChannelAudioVolume_SetChannelVolume(
        IChannelAudioVolume *iface, UINT32 index, float level,
        const GUID *context)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%d, %f, %s)\n", session, index, level,
            wine_dbgstr_guid(context));

    if(level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if(index >= session->channel_count)
        return E_INVALIDARG;

    if(context)
        FIXME("Notifications not supported yet\n");

    EnterCriticalSection(&session->lock);

    session->channel_vols[index] = level;

    TRACE("OSS doesn't support setting volume\n");

    LeaveCriticalSection(&session->lock);

    return S_OK;
}

static HRESULT WINAPI ChannelAudioVolume_GetChannelVolume(
        IChannelAudioVolume *iface, UINT32 index, float *level)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%d, %p)\n", session, index, level);

    if(!level)
        return NULL_PTR_ERR;

    if(index >= session->channel_count)
        return E_INVALIDARG;

    *level = session->channel_vols[index];

    return S_OK;
}

static HRESULT WINAPI ChannelAudioVolume_SetAllVolumes(
        IChannelAudioVolume *iface, UINT32 count, const float *levels,
        const GUID *context)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;
    int i;

    TRACE("(%p)->(%d, %p, %s)\n", session, count, levels,
            wine_dbgstr_guid(context));

    if(!levels)
        return NULL_PTR_ERR;

    if(count != session->channel_count)
        return E_INVALIDARG;

    if(context)
        FIXME("Notifications not supported yet\n");

    EnterCriticalSection(&session->lock);

    for(i = 0; i < count; ++i)
        session->channel_vols[i] = levels[i];

    TRACE("OSS doesn't support setting volume\n");

    LeaveCriticalSection(&session->lock);

    return S_OK;
}

static HRESULT WINAPI ChannelAudioVolume_GetAllVolumes(
        IChannelAudioVolume *iface, UINT32 count, float *levels)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;
    int i;

    TRACE("(%p)->(%d, %p)\n", session, count, levels);

    if(!levels)
        return NULL_PTR_ERR;

    if(count != session->channel_count)
        return E_INVALIDARG;

    for(i = 0; i < count; ++i)
        levels[i] = session->channel_vols[i];

    return S_OK;
}

static const IChannelAudioVolumeVtbl ChannelAudioVolume_Vtbl =
{
    ChannelAudioVolume_QueryInterface,
    ChannelAudioVolume_AddRef,
    ChannelAudioVolume_Release,
    ChannelAudioVolume_GetChannelCount,
    ChannelAudioVolume_SetChannelVolume,
    ChannelAudioVolume_GetChannelVolume,
    ChannelAudioVolume_SetAllVolumes,
    ChannelAudioVolume_GetAllVolumes
};

static HRESULT WINAPI AudioSessionManager_QueryInterface(IAudioSessionManager2 *iface,
        REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioSessionManager) ||
            IsEqualIID(riid, &IID_IAudioSessionManager2))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioSessionManager_AddRef(IAudioSessionManager2 *iface)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI AudioSessionManager_Release(IAudioSessionManager2 *iface)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    if(!ref)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

static HRESULT WINAPI AudioSessionManager_GetAudioSessionControl(
        IAudioSessionManager2 *iface, const GUID *session_guid, DWORD flags,
        IAudioSessionControl **out)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    AudioSession *session;
    AudioSessionWrapper *wrapper;
    HRESULT hr;

    TRACE("(%p)->(%s, %x, %p)\n", This, debugstr_guid(session_guid),
            flags, out);

    hr = get_audio_session(session_guid, This->device, 0, &session);
    if(FAILED(hr))
        return hr;

    wrapper = AudioSessionWrapper_Create(NULL);
    if(!wrapper)
        return E_OUTOFMEMORY;

    wrapper->session = session;

    *out = (IAudioSessionControl*)&wrapper->IAudioSessionControl2_iface;

    return S_OK;
}

static HRESULT WINAPI AudioSessionManager_GetSimpleAudioVolume(
        IAudioSessionManager2 *iface, const GUID *session_guid, DWORD flags,
        ISimpleAudioVolume **out)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    AudioSession *session;
    AudioSessionWrapper *wrapper;
    HRESULT hr;

    TRACE("(%p)->(%s, %x, %p)\n", This, debugstr_guid(session_guid),
            flags, out);

    hr = get_audio_session(session_guid, This->device, 0, &session);
    if(FAILED(hr))
        return hr;

    wrapper = AudioSessionWrapper_Create(NULL);
    if(!wrapper)
        return E_OUTOFMEMORY;

    wrapper->session = session;

    *out = &wrapper->ISimpleAudioVolume_iface;

    return S_OK;
}

static HRESULT WINAPI AudioSessionManager_GetSessionEnumerator(
        IAudioSessionManager2 *iface, IAudioSessionEnumerator **out)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, out);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionManager_RegisterSessionNotification(
        IAudioSessionManager2 *iface, IAudioSessionNotification *notification)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionManager_UnregisterSessionNotification(
        IAudioSessionManager2 *iface, IAudioSessionNotification *notification)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionManager_RegisterDuckNotification(
        IAudioSessionManager2 *iface, const WCHAR *session_id,
        IAudioVolumeDuckNotification *notification)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionManager_UnregisterDuckNotification(
        IAudioSessionManager2 *iface,
        IAudioVolumeDuckNotification *notification)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static const IAudioSessionManager2Vtbl AudioSessionManager2_Vtbl =
{
    AudioSessionManager_QueryInterface,
    AudioSessionManager_AddRef,
    AudioSessionManager_Release,
    AudioSessionManager_GetAudioSessionControl,
    AudioSessionManager_GetSimpleAudioVolume,
    AudioSessionManager_GetSessionEnumerator,
    AudioSessionManager_RegisterSessionNotification,
    AudioSessionManager_UnregisterSessionNotification,
    AudioSessionManager_RegisterDuckNotification,
    AudioSessionManager_UnregisterDuckNotification
};

HRESULT WINAPI AUDDRV_GetAudioSessionManager(IMMDevice *device,
        IAudioSessionManager2 **out)
{
    SessionMgr *This;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SessionMgr));
    if(!This)
        return E_OUTOFMEMORY;

    This->IAudioSessionManager2_iface.lpVtbl = &AudioSessionManager2_Vtbl;
    This->device = device;
    This->ref = 1;

    *out = &This->IAudioSessionManager2_iface;

    return S_OK;
}
