/*
 * Copyright (c) 2015 Mark Harmstone
 * Copyright (c) 2015 Andrew Eikum for CodeWeavers
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "ole2.h"
#include "rpcproxy.h"

#include "wine/debug.h"
#include "wine/list.h"
#include <propsys.h>
#include "initguid.h"

#include "mmsystem.h"
#include "xaudio2.h"
#include "devpkey.h"
#include "mmdeviceapi.h"
#include "audioclient.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

WINE_DEFAULT_DEBUG_CHANNEL(xaudio2);

static ALCdevice *(ALC_APIENTRY *palcLoopbackOpenDeviceSOFT)(const ALCchar*);
static void (ALC_APIENTRY *palcRenderSamplesSOFT)(ALCdevice*, ALCvoid*, ALCsizei);

static HINSTANCE instance;

static void dump_fmt(const WAVEFORMATEX *fmt)
{
    TRACE("wFormatTag: 0x%x (", fmt->wFormatTag);
    switch(fmt->wFormatTag){
#define DOCASE(x) case x: TRACE(#x); break;
    DOCASE(WAVE_FORMAT_PCM)
    DOCASE(WAVE_FORMAT_IEEE_FLOAT)
    DOCASE(WAVE_FORMAT_EXTENSIBLE)
#undef DOCASE
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

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, void *pReserved)
{
    TRACE("(%p, %d, %p)\n", hinstDLL, reason, pReserved);

    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        instance = hinstDLL;
        DisableThreadLibraryCalls( hinstDLL );

        if(!alcIsExtensionPresent(NULL, "ALC_SOFT_loopback") ||
                !(palcLoopbackOpenDeviceSOFT = alcGetProcAddress(NULL, "alcLoopbackOpenDeviceSOFT")) ||
                !(palcRenderSamplesSOFT = alcGetProcAddress(NULL, "alcRenderSamplesSOFT"))){
            ERR("XAudio2 requires the ALC_SOFT_loopback extension (OpenAL-Soft >= 1.14)\n");
            return FALSE;
        }

        break;
    }
    return TRUE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    TRACE("\n");
    return __wine_register_resources(instance);
}

HRESULT WINAPI DllUnregisterServer(void)
{
    TRACE("\n");
    return __wine_unregister_resources(instance);
}

typedef struct _XA2Buffer {
    XAUDIO2_BUFFER xa2buffer;
    DWORD offs_bytes;
    UINT32 latest_al_buf;
} XA2Buffer;

typedef struct _IXAudio2Impl IXAudio2Impl;

typedef struct _XA2SourceImpl {
    IXAudio27SourceVoice IXAudio27SourceVoice_iface;
    IXAudio2SourceVoice IXAudio2SourceVoice_iface;

    IXAudio2Impl *xa2;

    BOOL in_use;

    CRITICAL_SECTION lock;

    WAVEFORMATEX *fmt;
    ALenum al_fmt;
    UINT32 submit_blocksize;

    IXAudio2VoiceCallback *cb;

    DWORD nsends;
    XAUDIO2_SEND_DESCRIPTOR *sends;

    BOOL running;

    UINT64 played_frames;

    XA2Buffer buffers[XAUDIO2_MAX_QUEUED_BUFFERS];
    UINT32 first_buf, cur_buf, nbufs, in_al_bytes;

    ALuint al_src;
    /* most cases will only need about 4 AL buffers, but some corner cases
     * could require up to MAX_QUEUED_BUFFERS */
    ALuint al_bufs[XAUDIO2_MAX_QUEUED_BUFFERS];
    DWORD first_al_buf, al_bufs_used;

    struct list entry;
} XA2SourceImpl;

XA2SourceImpl *impl_from_IXAudio2SourceVoice(IXAudio2SourceVoice *iface)
{
    return CONTAINING_RECORD(iface, XA2SourceImpl, IXAudio2SourceVoice_iface);
}

XA2SourceImpl *impl_from_IXAudio27SourceVoice(IXAudio27SourceVoice *iface)
{
    return CONTAINING_RECORD(iface, XA2SourceImpl, IXAudio27SourceVoice_iface);
}

typedef struct _XA2SubmixImpl {
    IXAudio2SubmixVoice IXAudio2SubmixVoice_iface;

    BOOL in_use;

    CRITICAL_SECTION lock;

    struct list entry;
} XA2SubmixImpl;

XA2SubmixImpl *impl_from_IXAudio2SubmixVoice(IXAudio2SubmixVoice *iface)
{
    return CONTAINING_RECORD(iface, XA2SubmixImpl, IXAudio2SubmixVoice_iface);
}

struct _IXAudio2Impl {
    IXAudio27 IXAudio27_iface;
    IXAudio2 IXAudio2_iface;
    IXAudio2MasteringVoice IXAudio2MasteringVoice_iface;

    LONG ref;

    CRITICAL_SECTION lock;

    HANDLE engine, mmevt;
    BOOL stop_engine;

    DWORD version;

    struct list source_voices;
    struct list submix_voices;

    IMMDeviceEnumerator *devenum;

    WCHAR **devids;
    UINT32 ndevs;

    IAudioClient *aclient;
    IAudioRenderClient *render;

    UINT32 period_frames;

    WAVEFORMATEXTENSIBLE fmt;

    ALCdevice *al_device;
    ALCcontext *al_ctx;

    UINT32 ncbs;
    IXAudio2EngineCallback **cbs;

    BOOL running;
};

static inline IXAudio2Impl *impl_from_IXAudio2(IXAudio2 *iface)
{
    return CONTAINING_RECORD(iface, IXAudio2Impl, IXAudio2_iface);
}

static inline IXAudio2Impl *impl_from_IXAudio27(IXAudio27 *iface)
{
    return CONTAINING_RECORD(iface, IXAudio2Impl, IXAudio27_iface);
}

IXAudio2Impl *impl_from_IXAudio2MasteringVoice(IXAudio2MasteringVoice *iface)
{
    return CONTAINING_RECORD(iface, IXAudio2Impl, IXAudio2MasteringVoice_iface);
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

static void WINAPI XA2SRC_GetVoiceDetails(IXAudio2SourceVoice *iface,
        XAUDIO2_VOICE_DETAILS *pVoiceDetails)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %p\n", This, pVoiceDetails);
}

static HRESULT WINAPI XA2SRC_SetOutputVoices(IXAudio2SourceVoice *iface,
        const XAUDIO2_VOICE_SENDS *pSendList)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    int i;
    XAUDIO2_VOICE_SENDS def_send;
    XAUDIO2_SEND_DESCRIPTOR def_desc;

    TRACE("%p, %p\n", This, pSendList);

    if(!pSendList){
        def_desc.Flags = 0;
        def_desc.pOutputVoice = (IXAudio2Voice*)&This->xa2->IXAudio2MasteringVoice_iface;

        def_send.SendCount = 1;
        def_send.pSends = &def_desc;

        pSendList = &def_send;
    }

    if(TRACE_ON(xaudio2)){
        for(i = 0; i < pSendList->SendCount; ++i){
            XAUDIO2_SEND_DESCRIPTOR *desc = &pSendList->pSends[i];
            TRACE("Outputting to: 0x%x, %p\n", desc->Flags, desc->pOutputVoice);
        }
    }

    if(This->nsends < pSendList->SendCount){
        HeapFree(GetProcessHeap(), 0, This->sends);
        This->sends = HeapAlloc(GetProcessHeap(), 0, sizeof(*This->sends) * pSendList->SendCount);
        This->nsends = pSendList->SendCount;
    }else
        memset(This->sends, 0, sizeof(*This->sends) * This->nsends);

    memcpy(This->sends, pSendList->pSends, sizeof(*This->sends) * pSendList->SendCount);

    return S_OK;
}

static HRESULT WINAPI XA2SRC_SetEffectChain(IXAudio2SourceVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %p\n", This, pEffectChain);
    return S_OK;
}

static HRESULT WINAPI XA2SRC_EnableEffect(IXAudio2SourceVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %u, 0x%x\n", This, EffectIndex, OperationSet);
    return S_OK;
}

static HRESULT WINAPI XA2SRC_DisableEffect(IXAudio2SourceVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %u, 0x%x\n", This, EffectIndex, OperationSet);
    return S_OK;
}

static void WINAPI XA2SRC_GetEffectState(IXAudio2SourceVoice *iface,
        UINT32 EffectIndex, BOOL *pEnabled)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %u, %p\n", This, EffectIndex, pEnabled);
}

static HRESULT WINAPI XA2SRC_SetEffectParameters(IXAudio2SourceVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %u, %p, 0x%x, 0x%x\n", This, EffectIndex, pParameters,
            ParametersByteSize, OperationSet);
    return S_OK;
}

static HRESULT WINAPI XA2SRC_GetEffectParameters(IXAudio2SourceVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %u, %p, 0x%x\n", This, EffectIndex, pParameters,
            ParametersByteSize);
    return S_OK;
}

static HRESULT WINAPI XA2SRC_SetFilterParameters(IXAudio2SourceVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %p, 0x%x\n", This, pParameters, OperationSet);
    return S_OK;
}

static void WINAPI XA2SRC_GetFilterParameters(IXAudio2SourceVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %p\n", This, pParameters);
}

static HRESULT WINAPI XA2SRC_SetOutputFilterParameters(IXAudio2SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %p, %p, 0x%x\n", This, pDestinationVoice, pParameters, OperationSet);
    return S_OK;
}

static void WINAPI XA2SRC_GetOutputFilterParameters(IXAudio2SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %p, %p\n", This, pDestinationVoice, pParameters);
}

static HRESULT WINAPI XA2SRC_SetVolume(IXAudio2SourceVoice *iface, float Volume,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %f, 0x%x\n", This, Volume, OperationSet);
    return S_OK;
}

static void WINAPI XA2SRC_GetVolume(IXAudio2SourceVoice *iface, float *pVolume)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %p\n", This, pVolume);
}

static HRESULT WINAPI XA2SRC_SetChannelVolumes(IXAudio2SourceVoice *iface,
        UINT32 Channels, const float *pVolumes, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %u, %p, 0x%x\n", This, Channels, pVolumes, OperationSet);
    return S_OK;
}

static void WINAPI XA2SRC_GetChannelVolumes(IXAudio2SourceVoice *iface,
        UINT32 Channels, float *pVolumes)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %u, %p\n", This, Channels, pVolumes);
}

static HRESULT WINAPI XA2SRC_SetOutputMatrix(IXAudio2SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %p, %u, %u, %p, 0x%x\n", This, pDestinationVoice,
            SourceChannels, DestinationChannels, pLevelMatrix, OperationSet);
    return S_OK;
}

static void WINAPI XA2SRC_GetOutputMatrix(IXAudio2SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %p, %u, %u, %p\n", This, pDestinationVoice,
            SourceChannels, DestinationChannels, pLevelMatrix);
}

static void WINAPI XA2SRC_DestroyVoice(IXAudio2SourceVoice *iface)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    ALint processed;

    TRACE("%p\n", This);

    EnterCriticalSection(&This->lock);

    if(!This->in_use){
        LeaveCriticalSection(&This->lock);
        return;
    }

    This->in_use = FALSE;

    This->running = FALSE;

    IXAudio2SourceVoice_Stop(iface, 0, 0);

    alSourceStop(This->al_src);

    /* unqueue all buffers */
    alSourcei(This->al_src, AL_BUFFER, AL_NONE);

    alGetSourcei(This->al_src, AL_BUFFERS_PROCESSED, &processed);

    if(processed > 0){
        ALuint al_buffers[XAUDIO2_MAX_QUEUED_BUFFERS];

        alSourceUnqueueBuffers(This->al_src, processed, al_buffers);
    }

    HeapFree(GetProcessHeap(), 0, This->fmt);

    alDeleteBuffers(XAUDIO2_MAX_QUEUED_BUFFERS, This->al_bufs);
    alDeleteSources(1, &This->al_src);

    This->in_al_bytes = 0;
    This->al_bufs_used = 0;
    This->played_frames = 0;
    This->nbufs = 0;
    This->first_buf = 0;
    This->cur_buf = 0;

    LeaveCriticalSection(&This->lock);
}

static HRESULT WINAPI XA2SRC_Start(IXAudio2SourceVoice *iface, UINT32 Flags,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);

    TRACE("%p, 0x%x, 0x%x\n", This, Flags, OperationSet);

    EnterCriticalSection(&This->lock);

    This->running = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI XA2SRC_Stop(IXAudio2SourceVoice *iface, UINT32 Flags,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);

    TRACE("%p, 0x%x, 0x%x\n", This, Flags, OperationSet);

    EnterCriticalSection(&This->lock);

    This->running = FALSE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static ALenum get_al_format(const WAVEFORMATEX *fmt)
{
    WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE*)fmt;
    if(fmt->wFormatTag == WAVE_FORMAT_PCM ||
            (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))){
        switch(fmt->wBitsPerSample){
        case 8:
            switch(fmt->nChannels){
            case 1:
                return AL_FORMAT_MONO8;
            case 2:
                return AL_FORMAT_STEREO8;
            case 4:
                return AL_FORMAT_QUAD8;
            case 6:
                return AL_FORMAT_51CHN8;
            case 7:
                return AL_FORMAT_61CHN8;
            case 8:
                return AL_FORMAT_71CHN8;
            }
        case 16:
            switch(fmt->nChannels){
            case 1:
                return AL_FORMAT_MONO16;
            case 2:
                return AL_FORMAT_STEREO16;
            case 4:
                return AL_FORMAT_QUAD16;
            case 6:
                return AL_FORMAT_51CHN16;
            case 7:
                return AL_FORMAT_61CHN16;
            case 8:
                return AL_FORMAT_71CHN16;
            }
        }
    }else if(fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
            (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))){
        if(fmt->wBitsPerSample == 32){
            switch(fmt->nChannels){
            case 1:
                return AL_FORMAT_MONO_FLOAT32;
            case 2:
                return AL_FORMAT_STEREO_FLOAT32;
            }
        }
    }
    return 0;
}

static HRESULT WINAPI XA2SRC_SubmitSourceBuffer(IXAudio2SourceVoice *iface,
        const XAUDIO2_BUFFER *pBuffer, const XAUDIO2_BUFFER_WMA *pBufferWMA)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    XA2Buffer *buf;
    UINT32 buf_idx;

    TRACE("%p, %p, %p\n", This, pBuffer, pBufferWMA);

    if(TRACE_ON(xaudio2)){
        TRACE("Flags: 0x%x\n", pBuffer->Flags);
        TRACE("AudioBytes: %u\n", pBuffer->AudioBytes);
        TRACE("pAudioData: %p\n", pBuffer->pAudioData);
        TRACE("PlayBegin: %u\n", pBuffer->PlayBegin);
        TRACE("PlayLength: %u\n", pBuffer->PlayLength);
        TRACE("LoopBegin: %u\n", pBuffer->LoopBegin);
        TRACE("LoopLength: %u\n", pBuffer->LoopLength);
        TRACE("LoopCount: %u\n", pBuffer->LoopCount);
        TRACE("pContext: %p\n", pBuffer->pContext);
    }

    EnterCriticalSection(&This->lock);

    if(This->nbufs >= XAUDIO2_MAX_QUEUED_BUFFERS){
        TRACE("Too many buffers queued!\n");
        LeaveCriticalSection(&This->lock);
        return XAUDIO2_E_INVALID_CALL;
    }

    buf_idx = (This->first_buf + This->nbufs) % XAUDIO2_MAX_QUEUED_BUFFERS;
    buf = &This->buffers[buf_idx];
    memset(buf, 0, sizeof(*buf));

    /* API contract: pAudioData must remain valid until this buffer is played,
     * but pBuffer itself may be reused immediately */
    memcpy(&buf->xa2buffer, pBuffer, sizeof(*pBuffer));

    buf->offs_bytes = 0;
    buf->latest_al_buf = -1;

    ++This->nbufs;

    TRACE("%p: queued buffer %u (%u bytes), now %u buffers held\n",
            This, buf_idx, buf->xa2buffer.AudioBytes, This->nbufs);

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI XA2SRC_FlushSourceBuffers(IXAudio2SourceVoice *iface)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p\n", This);
    return S_OK;
}

static HRESULT WINAPI XA2SRC_Discontinuity(IXAudio2SourceVoice *iface)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p\n", This);
    return S_OK;
}

static HRESULT WINAPI XA2SRC_ExitLoop(IXAudio2SourceVoice *iface, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, 0x%x\n", This, OperationSet);
    return S_OK;
}

static void WINAPI XA2SRC_GetState(IXAudio2SourceVoice *iface,
        XAUDIO2_VOICE_STATE *pVoiceState, UINT32 Flags)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %p, 0x%x\n", This, pVoiceState, Flags);
}

static HRESULT WINAPI XA2SRC_SetFrequencyRatio(IXAudio2SourceVoice *iface,
        float Ratio, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %f, 0x%x\n", This, Ratio, OperationSet);
    return S_OK;
}

static void WINAPI XA2SRC_GetFrequencyRatio(IXAudio2SourceVoice *iface, float *pRatio)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %p\n", This, pRatio);
}

static HRESULT WINAPI XA2SRC_SetSourceSampleRate(
    IXAudio2SourceVoice *iface,
    UINT32 NewSourceSampleRate)
{
    XA2SourceImpl *This = impl_from_IXAudio2SourceVoice(iface);
    TRACE("%p, %u\n", This, NewSourceSampleRate);
    return S_OK;
}

static const IXAudio2SourceVoiceVtbl XAudio2SourceVoice_Vtbl = {
    XA2SRC_GetVoiceDetails,
    XA2SRC_SetOutputVoices,
    XA2SRC_SetEffectChain,
    XA2SRC_EnableEffect,
    XA2SRC_DisableEffect,
    XA2SRC_GetEffectState,
    XA2SRC_SetEffectParameters,
    XA2SRC_GetEffectParameters,
    XA2SRC_SetFilterParameters,
    XA2SRC_GetFilterParameters,
    XA2SRC_SetOutputFilterParameters,
    XA2SRC_GetOutputFilterParameters,
    XA2SRC_SetVolume,
    XA2SRC_GetVolume,
    XA2SRC_SetChannelVolumes,
    XA2SRC_GetChannelVolumes,
    XA2SRC_SetOutputMatrix,
    XA2SRC_GetOutputMatrix,
    XA2SRC_DestroyVoice,
    XA2SRC_Start,
    XA2SRC_Stop,
    XA2SRC_SubmitSourceBuffer,
    XA2SRC_FlushSourceBuffers,
    XA2SRC_Discontinuity,
    XA2SRC_ExitLoop,
    XA2SRC_GetState,
    XA2SRC_SetFrequencyRatio,
    XA2SRC_GetFrequencyRatio,
    XA2SRC_SetSourceSampleRate
};

static void WINAPI XA27SRC_GetVoiceDetails(IXAudio27SourceVoice *iface,
        XAUDIO2_VOICE_DETAILS *pVoiceDetails)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_GetVoiceDetails(&This->IXAudio2SourceVoice_iface, pVoiceDetails);
}

static HRESULT WINAPI XA27SRC_SetOutputVoices(IXAudio27SourceVoice *iface,
        const XAUDIO2_VOICE_SENDS *pSendList)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_SetOutputVoices(&This->IXAudio2SourceVoice_iface, pSendList);
}

static HRESULT WINAPI XA27SRC_SetEffectChain(IXAudio27SourceVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_SetEffectChain(&This->IXAudio2SourceVoice_iface, pEffectChain);
}

static HRESULT WINAPI XA27SRC_EnableEffect(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_EnableEffect(&This->IXAudio2SourceVoice_iface, EffectIndex, OperationSet);
}

static HRESULT WINAPI XA27SRC_DisableEffect(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_DisableEffect(&This->IXAudio2SourceVoice_iface, EffectIndex, OperationSet);
}

static void WINAPI XA27SRC_GetEffectState(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, BOOL *pEnabled)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    XA2SRC_GetEffectState(&This->IXAudio2SourceVoice_iface, EffectIndex, pEnabled);
}

static HRESULT WINAPI XA27SRC_SetEffectParameters(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_SetEffectParameters(&This->IXAudio2SourceVoice_iface,
            EffectIndex, pParameters, ParametersByteSize, OperationSet);
}

static HRESULT WINAPI XA27SRC_GetEffectParameters(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_GetEffectParameters(&This->IXAudio2SourceVoice_iface,
            EffectIndex, pParameters, ParametersByteSize);
}

static HRESULT WINAPI XA27SRC_SetFilterParameters(IXAudio27SourceVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_SetFilterParameters(&This->IXAudio2SourceVoice_iface,
            pParameters, OperationSet);
}

static void WINAPI XA27SRC_GetFilterParameters(IXAudio27SourceVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    XA2SRC_GetFilterParameters(&This->IXAudio2SourceVoice_iface, pParameters);
}

static HRESULT WINAPI XA27SRC_SetOutputFilterParameters(IXAudio27SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_SetOutputFilterParameters(&This->IXAudio2SourceVoice_iface,
            pDestinationVoice, pParameters, OperationSet);
}

static void WINAPI XA27SRC_GetOutputFilterParameters(IXAudio27SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    XA2SRC_GetOutputFilterParameters(&This->IXAudio2SourceVoice_iface,
            pDestinationVoice, pParameters);
}

static HRESULT WINAPI XA27SRC_SetVolume(IXAudio27SourceVoice *iface, float Volume,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_SetVolume(&This->IXAudio2SourceVoice_iface, Volume,
            OperationSet);
}

static void WINAPI XA27SRC_GetVolume(IXAudio27SourceVoice *iface, float *pVolume)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    XA2SRC_GetVolume(&This->IXAudio2SourceVoice_iface, pVolume);
}

static HRESULT WINAPI XA27SRC_SetChannelVolumes(IXAudio27SourceVoice *iface,
        UINT32 Channels, const float *pVolumes, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_SetChannelVolumes(&This->IXAudio2SourceVoice_iface, Channels,
            pVolumes, OperationSet);
}

static void WINAPI XA27SRC_GetChannelVolumes(IXAudio27SourceVoice *iface,
        UINT32 Channels, float *pVolumes)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    XA2SRC_GetChannelVolumes(&This->IXAudio2SourceVoice_iface, Channels,
            pVolumes);
}

static HRESULT WINAPI XA27SRC_SetOutputMatrix(IXAudio27SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_SetOutputMatrix(&This->IXAudio2SourceVoice_iface,
            pDestinationVoice, SourceChannels, DestinationChannels,
            pLevelMatrix, OperationSet);
}

static void WINAPI XA27SRC_GetOutputMatrix(IXAudio27SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    XA2SRC_GetOutputMatrix(&This->IXAudio2SourceVoice_iface, pDestinationVoice,
            SourceChannels, DestinationChannels, pLevelMatrix);
}

static void WINAPI XA27SRC_DestroyVoice(IXAudio27SourceVoice *iface)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    XA2SRC_DestroyVoice(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA27SRC_Start(IXAudio27SourceVoice *iface, UINT32 Flags,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_Start(&This->IXAudio2SourceVoice_iface, Flags, OperationSet);
}

static HRESULT WINAPI XA27SRC_Stop(IXAudio27SourceVoice *iface, UINT32 Flags,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_Stop(&This->IXAudio2SourceVoice_iface, Flags, OperationSet);
}

static HRESULT WINAPI XA27SRC_SubmitSourceBuffer(IXAudio27SourceVoice *iface,
        const XAUDIO2_BUFFER *pBuffer, const XAUDIO2_BUFFER_WMA *pBufferWMA)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_SubmitSourceBuffer(&This->IXAudio2SourceVoice_iface, pBuffer,
            pBufferWMA);
}

static HRESULT WINAPI XA27SRC_FlushSourceBuffers(IXAudio27SourceVoice *iface)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_FlushSourceBuffers(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA27SRC_Discontinuity(IXAudio27SourceVoice *iface)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_Discontinuity(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA27SRC_ExitLoop(IXAudio27SourceVoice *iface, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_ExitLoop(&This->IXAudio2SourceVoice_iface, OperationSet);
}

static void WINAPI XA27SRC_GetState(IXAudio27SourceVoice *iface,
        XAUDIO2_VOICE_STATE *pVoiceState)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_GetState(&This->IXAudio2SourceVoice_iface, pVoiceState, 0);
}

static HRESULT WINAPI XA27SRC_SetFrequencyRatio(IXAudio27SourceVoice *iface,
        float Ratio, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_SetFrequencyRatio(&This->IXAudio2SourceVoice_iface, Ratio, OperationSet);
}

static void WINAPI XA27SRC_GetFrequencyRatio(IXAudio27SourceVoice *iface, float *pRatio)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_GetFrequencyRatio(&This->IXAudio2SourceVoice_iface, pRatio);
}

static HRESULT WINAPI XA27SRC_SetSourceSampleRate(
    IXAudio27SourceVoice *iface,
    UINT32 NewSourceSampleRate)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return XA2SRC_SetSourceSampleRate(&This->IXAudio2SourceVoice_iface, NewSourceSampleRate);
}

static const IXAudio27SourceVoiceVtbl XAudio27SourceVoice_Vtbl = {
    XA27SRC_GetVoiceDetails,
    XA27SRC_SetOutputVoices,
    XA27SRC_SetEffectChain,
    XA27SRC_EnableEffect,
    XA27SRC_DisableEffect,
    XA27SRC_GetEffectState,
    XA27SRC_SetEffectParameters,
    XA27SRC_GetEffectParameters,
    XA27SRC_SetFilterParameters,
    XA27SRC_GetFilterParameters,
    XA27SRC_SetOutputFilterParameters,
    XA27SRC_GetOutputFilterParameters,
    XA27SRC_SetVolume,
    XA27SRC_GetVolume,
    XA27SRC_SetChannelVolumes,
    XA27SRC_GetChannelVolumes,
    XA27SRC_SetOutputMatrix,
    XA27SRC_GetOutputMatrix,
    XA27SRC_DestroyVoice,
    XA27SRC_Start,
    XA27SRC_Stop,
    XA27SRC_SubmitSourceBuffer,
    XA27SRC_FlushSourceBuffers,
    XA27SRC_Discontinuity,
    XA27SRC_ExitLoop,
    XA27SRC_GetState,
    XA27SRC_SetFrequencyRatio,
    XA27SRC_GetFrequencyRatio,
    XA27SRC_SetSourceSampleRate
};

static void WINAPI XA2M_GetVoiceDetails(IXAudio2MasteringVoice *iface,
        XAUDIO2_VOICE_DETAILS *pVoiceDetails)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %p\n", This, pVoiceDetails);
}

static HRESULT WINAPI XA2M_SetOutputVoices(IXAudio2MasteringVoice *iface,
        const XAUDIO2_VOICE_SENDS *pSendList)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %p\n", This, pSendList);
    return S_OK;
}

static HRESULT WINAPI XA2M_SetEffectChain(IXAudio2MasteringVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %p\n", This, pEffectChain);
    return S_OK;
}

static HRESULT WINAPI XA2M_EnableEffect(IXAudio2MasteringVoice *iface, UINT32 EffectIndex,
        UINT32 OperationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %u, 0x%x\n", This, EffectIndex, OperationSet);
    return S_OK;
}

static HRESULT WINAPI XA2M_DisableEffect(IXAudio2MasteringVoice *iface, UINT32 EffectIndex,
        UINT32 OperationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %u, 0x%x\n", This, EffectIndex, OperationSet);
    return S_OK;
}

static void WINAPI XA2M_GetEffectState(IXAudio2MasteringVoice *iface, UINT32 EffectIndex,
        BOOL *pEnabled)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %u, %p\n", This, EffectIndex, pEnabled);
}

static HRESULT WINAPI XA2M_SetEffectParameters(IXAudio2MasteringVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %u, %p, 0x%x, 0x%x\n", This, EffectIndex, pParameters,
            ParametersByteSize, OperationSet);
    return S_OK;
}

static HRESULT WINAPI XA2M_GetEffectParameters(IXAudio2MasteringVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %u, %p, 0x%x\n", This, EffectIndex, pParameters,
            ParametersByteSize);
    return S_OK;
}

static HRESULT WINAPI XA2M_SetFilterParameters(IXAudio2MasteringVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %p, 0x%x\n", This, pParameters, OperationSet);
    return S_OK;
}

static void WINAPI XA2M_GetFilterParameters(IXAudio2MasteringVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %p\n", This, pParameters);
}

static HRESULT WINAPI XA2M_SetOutputFilterParameters(IXAudio2MasteringVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %p, %p, 0x%x\n", This, pDestinationVoice, pParameters, OperationSet);
    return S_OK;
}

static void WINAPI XA2M_GetOutputFilterParameters(IXAudio2MasteringVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %p, %p\n", This, pDestinationVoice, pParameters);
}

static HRESULT WINAPI XA2M_SetVolume(IXAudio2MasteringVoice *iface, float Volume,
        UINT32 OperationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %f, 0x%x\n", This, Volume, OperationSet);
    return S_OK;
}

static void WINAPI XA2M_GetVolume(IXAudio2MasteringVoice *iface, float *pVolume)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %p\n", This, pVolume);
}

static HRESULT WINAPI XA2M_SetChannelVolumes(IXAudio2MasteringVoice *iface, UINT32 Channels,
        const float *pVolumes, UINT32 OperationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %u, %p, 0x%x\n", This, Channels, pVolumes, OperationSet);
    return S_OK;
}

static void WINAPI XA2M_GetChannelVolumes(IXAudio2MasteringVoice *iface, UINT32 Channels,
        float *pVolumes)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %u, %p\n", This, Channels, pVolumes);
}

static HRESULT WINAPI XA2M_SetOutputMatrix(IXAudio2MasteringVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %p, %u, %u, %p, 0x%x\n", This, pDestinationVoice,
            SourceChannels, DestinationChannels, pLevelMatrix, OperationSet);
    return S_OK;
}

static void WINAPI XA2M_GetOutputMatrix(IXAudio2MasteringVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p, %p, %u, %u, %p\n", This, pDestinationVoice,
            SourceChannels, DestinationChannels, pLevelMatrix);
}

static void WINAPI XA2M_DestroyVoice(IXAudio2MasteringVoice *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);

    TRACE("%p\n", This);

    EnterCriticalSection(&This->lock);

    if(!This->aclient){
        LeaveCriticalSection(&This->lock);
        return;
    }

    IAudioRenderClient_Release(This->render);
    This->render = NULL;

    IAudioClient_Release(This->aclient);
    This->aclient = NULL;

    alcCloseDevice(This->al_device);
    This->al_device = NULL;

    alcDestroyContext(This->al_ctx);
    This->al_ctx = NULL;

    LeaveCriticalSection(&This->lock);
}

/* not present in XAudio2 2.7 */
static void WINAPI XA2M_GetChannelMask(IXAudio2MasteringVoice *iface,
        DWORD *pChannelMask)
{
    IXAudio2Impl *This = impl_from_IXAudio2MasteringVoice(iface);
    TRACE("%p %p\n", This, pChannelMask);
}

static const struct IXAudio2MasteringVoiceVtbl XAudio2MasteringVoice_Vtbl = {
    XA2M_GetVoiceDetails,
    XA2M_SetOutputVoices,
    XA2M_SetEffectChain,
    XA2M_EnableEffect,
    XA2M_DisableEffect,
    XA2M_GetEffectState,
    XA2M_SetEffectParameters,
    XA2M_GetEffectParameters,
    XA2M_SetFilterParameters,
    XA2M_GetFilterParameters,
    XA2M_SetOutputFilterParameters,
    XA2M_GetOutputFilterParameters,
    XA2M_SetVolume,
    XA2M_GetVolume,
    XA2M_SetChannelVolumes,
    XA2M_GetChannelVolumes,
    XA2M_SetOutputMatrix,
    XA2M_GetOutputMatrix,
    XA2M_DestroyVoice,
    XA2M_GetChannelMask
};

static void WINAPI XA2SUB_GetVoiceDetails(IXAudio2SubmixVoice *iface,
        XAUDIO2_VOICE_DETAILS *pVoiceDetails)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %p\n", This, pVoiceDetails);
}

static HRESULT WINAPI XA2SUB_SetOutputVoices(IXAudio2SubmixVoice *iface,
        const XAUDIO2_VOICE_SENDS *pSendList)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %p\n", This, pSendList);
    return S_OK;
}

static HRESULT WINAPI XA2SUB_SetEffectChain(IXAudio2SubmixVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %p\n", This, pEffectChain);
    return S_OK;
}

static HRESULT WINAPI XA2SUB_EnableEffect(IXAudio2SubmixVoice *iface, UINT32 EffectIndex,
        UINT32 OperationSet)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %u, 0x%x\n", This, EffectIndex, OperationSet);
    return S_OK;
}

static HRESULT WINAPI XA2SUB_DisableEffect(IXAudio2SubmixVoice *iface, UINT32 EffectIndex,
        UINT32 OperationSet)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %u, 0x%x\n", This, EffectIndex, OperationSet);
    return S_OK;
}

static void WINAPI XA2SUB_GetEffectState(IXAudio2SubmixVoice *iface, UINT32 EffectIndex,
        BOOL *pEnabled)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %u, %p\n", This, EffectIndex, pEnabled);
}

static HRESULT WINAPI XA2SUB_SetEffectParameters(IXAudio2SubmixVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %u, %p, 0x%x, 0x%x\n", This, EffectIndex, pParameters,
            ParametersByteSize, OperationSet);
    return S_OK;
}

static HRESULT WINAPI XA2SUB_GetEffectParameters(IXAudio2SubmixVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %u, %p, 0x%x\n", This, EffectIndex, pParameters,
            ParametersByteSize);
    return S_OK;
}

static HRESULT WINAPI XA2SUB_SetFilterParameters(IXAudio2SubmixVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %p, 0x%x\n", This, pParameters, OperationSet);
    return S_OK;
}

static void WINAPI XA2SUB_GetFilterParameters(IXAudio2SubmixVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %p\n", This, pParameters);
}

static HRESULT WINAPI XA2SUB_SetOutputFilterParameters(IXAudio2SubmixVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %p, %p, 0x%x\n", This, pDestinationVoice, pParameters, OperationSet);
    return S_OK;
}

static void WINAPI XA2SUB_GetOutputFilterParameters(IXAudio2SubmixVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %p, %p\n", This, pDestinationVoice, pParameters);
}

static HRESULT WINAPI XA2SUB_SetVolume(IXAudio2SubmixVoice *iface, float Volume,
        UINT32 OperationSet)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %f, 0x%x\n", This, Volume, OperationSet);
    return S_OK;
}

static void WINAPI XA2SUB_GetVolume(IXAudio2SubmixVoice *iface, float *pVolume)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %p\n", This, pVolume);
}

static HRESULT WINAPI XA2SUB_SetChannelVolumes(IXAudio2SubmixVoice *iface, UINT32 Channels,
        const float *pVolumes, UINT32 OperationSet)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %u, %p, 0x%x\n", This, Channels, pVolumes, OperationSet);
    return S_OK;
}

static void WINAPI XA2SUB_GetChannelVolumes(IXAudio2SubmixVoice *iface, UINT32 Channels,
        float *pVolumes)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %u, %p\n", This, Channels, pVolumes);
}

static HRESULT WINAPI XA2SUB_SetOutputMatrix(IXAudio2SubmixVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %p, %u, %u, %p, 0x%x\n", This, pDestinationVoice,
            SourceChannels, DestinationChannels, pLevelMatrix, OperationSet);
    return S_OK;
}

static void WINAPI XA2SUB_GetOutputMatrix(IXAudio2SubmixVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);
    TRACE("%p, %p, %u, %u, %p\n", This, pDestinationVoice,
            SourceChannels, DestinationChannels, pLevelMatrix);
}

static void WINAPI XA2SUB_DestroyVoice(IXAudio2SubmixVoice *iface)
{
    XA2SubmixImpl *This = impl_from_IXAudio2SubmixVoice(iface);

    TRACE("%p\n", This);

    EnterCriticalSection(&This->lock);

    This->in_use = FALSE;

    LeaveCriticalSection(&This->lock);
}

static const struct IXAudio2SubmixVoiceVtbl XAudio2SubmixVoice_Vtbl = {
    XA2SUB_GetVoiceDetails,
    XA2SUB_SetOutputVoices,
    XA2SUB_SetEffectChain,
    XA2SUB_EnableEffect,
    XA2SUB_DisableEffect,
    XA2SUB_GetEffectState,
    XA2SUB_SetEffectParameters,
    XA2SUB_GetEffectParameters,
    XA2SUB_SetFilterParameters,
    XA2SUB_GetFilterParameters,
    XA2SUB_SetOutputFilterParameters,
    XA2SUB_GetOutputFilterParameters,
    XA2SUB_SetVolume,
    XA2SUB_GetVolume,
    XA2SUB_SetChannelVolumes,
    XA2SUB_GetChannelVolumes,
    XA2SUB_SetOutputMatrix,
    XA2SUB_GetOutputMatrix,
    XA2SUB_DestroyVoice
};

static HRESULT WINAPI IXAudio2Impl_QueryInterface(IXAudio2 *iface, REFIID riid,
        void **ppvObject)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObject);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IXAudio2))
        *ppvObject = &This->IXAudio2_iface;
    else if(IsEqualGUID(riid, &IID_IXAudio27))
        *ppvObject = &This->IXAudio27_iface;
    else
        *ppvObject = NULL;

    if(*ppvObject){
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    FIXME("(%p)->(%s,%p), not found\n", This,debugstr_guid(riid), ppvObject);

    return E_NOINTERFACE;
}

static ULONG WINAPI IXAudio2Impl_AddRef(IXAudio2 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(): Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI IXAudio2Impl_Release(IXAudio2 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Refcount now %u\n", This, ref);

    if (!ref) {
        int i;
        XA2SourceImpl *src, *src2;
        XA2SubmixImpl *sub, *sub2;

        if(This->engine){
            This->stop_engine = TRUE;
            SetEvent(This->mmevt);
            WaitForSingleObject(This->engine, INFINITE);
            CloseHandle(This->engine);
        }

        LIST_FOR_EACH_ENTRY_SAFE(src, src2, &This->source_voices, XA2SourceImpl, entry){
            HeapFree(GetProcessHeap(), 0, src->sends);
            IXAudio2SourceVoice_DestroyVoice(&src->IXAudio2SourceVoice_iface);
            DeleteCriticalSection(&src->lock);
            HeapFree(GetProcessHeap(), 0, src);
        }

        LIST_FOR_EACH_ENTRY_SAFE(sub, sub2, &This->submix_voices, XA2SubmixImpl, entry){
            IXAudio2SubmixVoice_DestroyVoice(&sub->IXAudio2SubmixVoice_iface);
            DeleteCriticalSection(&sub->lock);
            HeapFree(GetProcessHeap(), 0, sub);
        }

        IXAudio2MasteringVoice_DestroyVoice(&This->IXAudio2MasteringVoice_iface);

        if(This->devenum)
            IMMDeviceEnumerator_Release(This->devenum);
        for(i = 0; i < This->ndevs; ++i)
            CoTaskMemFree(This->devids[i]);
        HeapFree(GetProcessHeap(), 0, This->devids);
        HeapFree(GetProcessHeap(), 0, This->cbs);

        CloseHandle(This->mmevt);

        DeleteCriticalSection(&This->lock);

        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI IXAudio2Impl_RegisterForCallbacks(IXAudio2 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);
    int i;

    TRACE("(%p)->(%p)\n", This, pCallback);

    EnterCriticalSection(&This->lock);

    for(i = 0; i < This->ncbs; ++i){
        if(!This->cbs[i] || This->cbs[i] == pCallback){
            This->cbs[i] = pCallback;
            LeaveCriticalSection(&This->lock);
            return S_OK;
        }
    }

    This->ncbs *= 2;
    This->cbs = HeapReAlloc(GetProcessHeap(), 0, This->cbs, This->ncbs * sizeof(*This->cbs));

    This->cbs[i] = pCallback;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static void WINAPI IXAudio2Impl_UnregisterForCallbacks(IXAudio2 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);
    int i;

    TRACE("(%p)->(%p)\n", This, pCallback);

    EnterCriticalSection(&This->lock);

    for(i = 0; i < This->ncbs; ++i){
        if(This->cbs[i] == pCallback)
            break;
    }

    for(; i < This->ncbs - 1 && This->cbs[i + 1]; ++i)
        This->cbs[i] = This->cbs[i + 1];

    if(i < This->ncbs)
        This->cbs[i] = NULL;

    LeaveCriticalSection(&This->lock);
}

static WAVEFORMATEX *copy_waveformat(const WAVEFORMATEX *wfex)
{
    WAVEFORMATEX *pwfx;

    if(wfex->wFormatTag == WAVE_FORMAT_PCM){
        pwfx = HeapAlloc(GetProcessHeap(), 0, sizeof(WAVEFORMATEX));
        CopyMemory(pwfx, wfex, sizeof(PCMWAVEFORMAT));
        pwfx->cbSize = 0;
    }else{
        pwfx = HeapAlloc(GetProcessHeap(), 0, sizeof(WAVEFORMATEX) + wfex->cbSize);
        CopyMemory(pwfx, wfex, sizeof(WAVEFORMATEX) + wfex->cbSize);
    }

    return pwfx;
}

static HRESULT WINAPI IXAudio2Impl_CreateSourceVoice(IXAudio2 *iface,
        IXAudio2SourceVoice **ppSourceVoice, const WAVEFORMATEX *pSourceFormat,
        UINT32 flags, float maxFrequencyRatio,
        IXAudio2VoiceCallback *pCallback, const XAUDIO2_VOICE_SENDS *pSendList,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);
    XA2SourceImpl *src;
    HRESULT hr;

    TRACE("(%p)->(%p, %p, 0x%x, %f, %p, %p, %p)\n", This, ppSourceVoice,
            pSourceFormat, flags, maxFrequencyRatio, pCallback, pSendList,
            pEffectChain);

    dump_fmt(pSourceFormat);

    EnterCriticalSection(&This->lock);

    LIST_FOR_EACH_ENTRY(src, &This->source_voices, XA2SourceImpl, entry){
        if(!src->in_use)
            break;
    }

    if(&src->entry == &This->source_voices){
        src = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*src));
        if(!src){
            LeaveCriticalSection(&This->lock);
            return E_OUTOFMEMORY;
        }

        list_add_head(&This->source_voices, &src->entry);

        src->IXAudio27SourceVoice_iface.lpVtbl = &XAudio27SourceVoice_Vtbl;
        src->IXAudio2SourceVoice_iface.lpVtbl = &XAudio2SourceVoice_Vtbl;

        InitializeCriticalSection(&src->lock);
        src->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": XA2SourceImpl.lock");

        src->xa2 = This;
    }

    src->in_use = TRUE;
    src->running = FALSE;

    LeaveCriticalSection(&This->lock);

    src->cb = pCallback;

    src->al_fmt = get_al_format(pSourceFormat);
    if(!src->al_fmt){
        src->in_use = FALSE;
        WARN("OpenAL can't convert this format!\n");
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }

    src->submit_blocksize = pSourceFormat->nBlockAlign;

    src->fmt = copy_waveformat(pSourceFormat);

    hr = XA2SRC_SetOutputVoices(&src->IXAudio2SourceVoice_iface, pSendList);
    if(FAILED(hr)){
        src->in_use = FALSE;
        return hr;
    }

    alGenSources(1, &src->al_src);
    alGenBuffers(XAUDIO2_MAX_QUEUED_BUFFERS, src->al_bufs);

    alSourcePlay(src->al_src);

    if(This->version == 27)
        *ppSourceVoice = (IXAudio2SourceVoice*)&src->IXAudio27SourceVoice_iface;
    else
        *ppSourceVoice = &src->IXAudio2SourceVoice_iface;

    TRACE("Created source voice: %p\n", src);

    return S_OK;
}

static HRESULT WINAPI IXAudio2Impl_CreateSubmixVoice(IXAudio2 *iface,
        IXAudio2SubmixVoice **ppSubmixVoice, UINT32 inputChannels,
        UINT32 inputSampleRate, UINT32 flags, UINT32 processingStage,
        const XAUDIO2_VOICE_SENDS *pSendList,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);
    XA2SubmixImpl *sub;

    TRACE("(%p)->(%p, %u, %u, 0x%x, %u, %p, %p)\n", This, ppSubmixVoice,
            inputChannels, inputSampleRate, flags, processingStage, pSendList,
            pEffectChain);

    EnterCriticalSection(&This->lock);

    LIST_FOR_EACH_ENTRY(sub, &This->submix_voices, XA2SubmixImpl, entry){
        if(!sub->in_use)
            break;
    }

    if(&sub->entry == &This->submix_voices){
        sub = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*sub));
        if(!sub){
            LeaveCriticalSection(&This->lock);
            return E_OUTOFMEMORY;
        }

        list_add_head(&This->submix_voices, &sub->entry);

        sub->IXAudio2SubmixVoice_iface.lpVtbl = &XAudio2SubmixVoice_Vtbl;

        InitializeCriticalSection(&sub->lock);
        sub->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": XA2SubmixImpl.lock");
    }

    sub->in_use = TRUE;

    LeaveCriticalSection(&This->lock);

    *ppSubmixVoice = &sub->IXAudio2SubmixVoice_iface;

    TRACE("Created submix voice: %p\n", sub);

    return S_OK;
}

static ALenum al_get_loopback_format(const WAVEFORMATEXTENSIBLE *fmt)
{
    if(fmt->Format.wFormatTag == WAVE_FORMAT_PCM ||
            (fmt->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmt->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))){
        switch(fmt->Format.wBitsPerSample){
        case 8:
            return ALC_UNSIGNED_BYTE_SOFT;
        case 16:
            return ALC_SHORT_SOFT;
        case 32:
            return ALC_INT_SOFT;
        }
    }else if(fmt->Format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
            (fmt->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmt->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))){
        if(fmt->Format.wBitsPerSample == 32)
            return ALC_FLOAT_SOFT;
    }
    return 0;
}

static HRESULT WINAPI IXAudio2Impl_CreateMasteringVoice(IXAudio2 *iface,
        IXAudio2MasteringVoice **ppMasteringVoice, UINT32 inputChannels,
        UINT32 inputSampleRate, UINT32 flags, const WCHAR *deviceId,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain,
        AUDIO_STREAM_CATEGORY streamCategory)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);
    IMMDevice *dev;
    HRESULT hr;
    WAVEFORMATEX *fmt;
    ALCint attrs[7];
    REFERENCE_TIME period;

    TRACE("(%p)->(%p, %u, %u, 0x%x, %s, %p, 0x%x)\n", This,
            ppMasteringVoice, inputChannels, inputSampleRate, flags,
            wine_dbgstr_w(deviceId), pEffectChain, streamCategory);

    if(flags != 0)
        WARN("Unknown flags set: 0x%x\n", flags);

    if(pEffectChain)
        WARN("Effect chain is unimplemented\n");

    EnterCriticalSection(&This->lock);

    /* there can only be one Mastering Voice, so just build it into XA2 */
    if(This->aclient){
        LeaveCriticalSection(&This->lock);
        return XAUDIO2_E_INVALID_CALL;
    }

    if(!deviceId){
        if(This->ndevs == 0){
            LeaveCriticalSection(&This->lock);
            return ERROR_NOT_FOUND;
        }
        deviceId = This->devids[0];
    }

    hr = IMMDeviceEnumerator_GetDevice(This->devenum, deviceId, &dev);
    if(FAILED(hr)){
        WARN("GetDevice failed: %08x\n", hr);
        hr = XAUDIO2_E_DEVICE_INVALIDATED;
        goto exit;
    }

    hr = IMMDevice_Activate(dev, &IID_IAudioClient,
            CLSCTX_INPROC_SERVER, NULL, (void**)&This->aclient);
    if(FAILED(hr)){
        WARN("Activate(IAudioClient) failed: %08x\n", hr);
        IMMDevice_Release(dev);
        hr = XAUDIO2_E_DEVICE_INVALIDATED;
        goto exit;
    }

    IMMDevice_Release(dev);

    hr = IAudioClient_GetMixFormat(This->aclient, &fmt);
    if(FAILED(hr)){
        WARN("GetMixFormat failed: %08x\n", hr);
        hr = XAUDIO2_E_DEVICE_INVALIDATED;
        goto exit;
    }

    if(sizeof(WAVEFORMATEX) + fmt->cbSize > sizeof(WAVEFORMATEXTENSIBLE)){
        FIXME("Mix format doesn't fit into WAVEFORMATEXTENSIBLE!\n");
        hr = XAUDIO2_E_DEVICE_INVALIDATED;
        goto exit;
    }

    if(inputChannels == XAUDIO2_DEFAULT_CHANNELS)
        inputChannels = fmt->nChannels;
    if(inputSampleRate == XAUDIO2_DEFAULT_SAMPLERATE)
        inputSampleRate = fmt->nSamplesPerSec;

    memcpy(&This->fmt, fmt, sizeof(WAVEFORMATEX) + fmt->cbSize);
    This->fmt.Format.nChannels = inputChannels;
    This->fmt.Format.nSamplesPerSec = inputSampleRate;
    This->fmt.Format.nBlockAlign = This->fmt.Format.nChannels * This->fmt.Format.wBitsPerSample / 8;
    This->fmt.Format.nAvgBytesPerSec = This->fmt.Format.nSamplesPerSec * This->fmt.Format.nBlockAlign;
    This->fmt.dwChannelMask = get_channel_mask(This->fmt.Format.nChannels);

    CoTaskMemFree(fmt);
    fmt = NULL;

    hr = IAudioClient_IsFormatSupported(This->aclient,
            AUDCLNT_SHAREMODE_SHARED, &This->fmt.Format, &fmt);
    if(hr == S_FALSE){
        if(sizeof(WAVEFORMATEX) + fmt->cbSize > sizeof(WAVEFORMATEXTENSIBLE)){
            FIXME("Mix format doesn't fit into WAVEFORMATEXTENSIBLE!\n");
            hr = XAUDIO2_E_DEVICE_INVALIDATED;
            goto exit;
        }
        memcpy(&This->fmt, fmt, sizeof(WAVEFORMATEX) + fmt->cbSize);
    }

    CoTaskMemFree(fmt);

    hr = IAudioClient_Initialize(This->aclient, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK, inputSampleRate /* 1s buffer */,
            0, &This->fmt.Format, NULL);
    if(FAILED(hr)){
        WARN("Initialize failed: %08x\n", hr);
        hr = XAUDIO2_E_DEVICE_INVALIDATED;
        goto exit;
    }

    hr = IAudioClient_GetDevicePeriod(This->aclient, &period, NULL);
    if(FAILED(hr)){
        WARN("GetDevicePeriod failed: %08x\n", hr);
        hr = XAUDIO2_E_DEVICE_INVALIDATED;
        goto exit;
    }

    This->period_frames = MulDiv(period, inputSampleRate, 10000000);

    hr = IAudioClient_SetEventHandle(This->aclient, This->mmevt);
    if(FAILED(hr)){
        WARN("Initialize failed: %08x\n", hr);
        hr = XAUDIO2_E_DEVICE_INVALIDATED;
        goto exit;
    }

    hr = IAudioClient_GetService(This->aclient, &IID_IAudioRenderClient,
            (void**)&This->render);
    if(FAILED(hr)){
        WARN("GetService(IAudioRenderClient) failed: %08x\n", hr);
        hr = XAUDIO2_E_DEVICE_INVALIDATED;
        goto exit;
    }

    /* setup openal context */
    attrs[0] = ALC_FORMAT_CHANNELS_SOFT;
    switch(inputChannels){
    case 1:
        attrs[1] = ALC_MONO_SOFT;
        break;
    case 2:
        attrs[1] = ALC_STEREO_SOFT;
        break;
    case 4:
        attrs[1] = ALC_QUAD_SOFT;
        break;
    case 6:
        attrs[1] = ALC_5POINT1_SOFT;
        break;
    case 7:
        attrs[1] = ALC_6POINT1_SOFT;
        break;
    case 8:
        attrs[1] = ALC_7POINT1_SOFT;
        break;
    default:
        WARN("OpenAL doesn't support %u channels\n", inputChannels);
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }
    attrs[2] = ALC_FREQUENCY;
    attrs[3] = inputSampleRate;
    attrs[4] = ALC_FORMAT_TYPE_SOFT;
    attrs[5] = al_get_loopback_format(&This->fmt);
    attrs[6] = 0;

    if(!attrs[5]){
        WARN("OpenAL can't output samples in this format\n");
        hr = XAUDIO2_E_DEVICE_INVALIDATED;
        goto exit;
    }

    This->al_device = palcLoopbackOpenDeviceSOFT(NULL);
    if(!This->al_device){
        WARN("alcLoopbackOpenDeviceSOFT failed\n");
        hr = XAUDIO2_E_DEVICE_INVALIDATED;
        goto exit;
    }

    This->al_ctx = alcCreateContext(This->al_device, attrs);
    if(!This->al_ctx){
        WARN("alcCreateContext failed\n");
        hr = XAUDIO2_E_DEVICE_INVALIDATED;
        goto exit;
    }

    if(alcMakeContextCurrent(This->al_ctx) == ALC_FALSE){
        WARN("alcMakeContextCurrent failed\n");
        hr = XAUDIO2_E_DEVICE_INVALIDATED;
        goto exit;
    }

    IAudioClient_Start(This->aclient);

    *ppMasteringVoice = &This->IXAudio2MasteringVoice_iface;

exit:
    if(FAILED(hr)){
        if(This->render){
            IAudioRenderClient_Release(This->render);
            This->render = NULL;
        }
        if(This->aclient){
            IAudioClient_Release(This->aclient);
            This->aclient = NULL;
        }
        if(This->al_ctx){
            alcDestroyContext(This->al_ctx);
            This->al_ctx = NULL;
        }
        if(This->al_device){
            alcCloseDevice(This->al_device);
            This->al_device = NULL;
        }
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

static DWORD WINAPI engine_threadproc(void *arg);

static HRESULT WINAPI IXAudio2Impl_StartEngine(IXAudio2 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    TRACE("(%p)->()\n", This);

    This->running = TRUE;

    if(!This->engine)
        This->engine = CreateThread(NULL, 0, engine_threadproc, This, 0, NULL);

    return S_OK;
}

static void WINAPI IXAudio2Impl_StopEngine(IXAudio2 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    TRACE("(%p)->()\n", This);

    This->running = FALSE;
}

static HRESULT WINAPI IXAudio2Impl_CommitChanges(IXAudio2 *iface,
        UINT32 operationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(0x%x): stub!\n", This, operationSet);

    return E_NOTIMPL;
}

static void WINAPI IXAudio2Impl_GetPerformanceData(IXAudio2 *iface,
        XAUDIO2_PERFORMANCE_DATA *pPerfData)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p): stub!\n", This, pPerfData);

    memset(pPerfData, 0, sizeof(*pPerfData));
}

static void WINAPI IXAudio2Impl_SetDebugConfiguration(IXAudio2 *iface,
        const XAUDIO2_DEBUG_CONFIGURATION *pDebugConfiguration,
        void *pReserved)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p, %p): stub!\n", This, pDebugConfiguration, pReserved);
}

/* XAudio2 2.8 */
static const IXAudio2Vtbl XAudio2_Vtbl =
{
    IXAudio2Impl_QueryInterface,
    IXAudio2Impl_AddRef,
    IXAudio2Impl_Release,
    IXAudio2Impl_RegisterForCallbacks,
    IXAudio2Impl_UnregisterForCallbacks,
    IXAudio2Impl_CreateSourceVoice,
    IXAudio2Impl_CreateSubmixVoice,
    IXAudio2Impl_CreateMasteringVoice,
    IXAudio2Impl_StartEngine,
    IXAudio2Impl_StopEngine,
    IXAudio2Impl_CommitChanges,
    IXAudio2Impl_GetPerformanceData,
    IXAudio2Impl_SetDebugConfiguration
};

static HRESULT WINAPI XA27_QueryInterface(IXAudio27 *iface, REFIID riid,
        void **ppvObject)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2Impl_QueryInterface(&This->IXAudio2_iface, riid, ppvObject);
}

static ULONG WINAPI XA27_AddRef(IXAudio27 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2Impl_AddRef(&This->IXAudio2_iface);
}

static ULONG WINAPI XA27_Release(IXAudio27 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2Impl_Release(&This->IXAudio2_iface);
}

static HRESULT WINAPI XA27_GetDeviceCount(IXAudio27 *iface, UINT32 *pCount)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);

    TRACE("%p, %p\n", This, pCount);

    *pCount = This->ndevs;

    return S_OK;
}

static HRESULT WINAPI XA27_GetDeviceDetails(IXAudio27 *iface, UINT32 index,
        XAUDIO2_DEVICE_DETAILS *pDeviceDetails)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    HRESULT hr;
    IMMDevice *dev;
    IAudioClient *client;
    IPropertyStore *ps;
    WAVEFORMATEX *wfx;
    PROPVARIANT var;

    TRACE("%p, %u, %p\n", This, index, pDeviceDetails);

    if(index >= This->ndevs)
        return E_INVALIDARG;

    hr = IMMDeviceEnumerator_GetDevice(This->devenum, This->devids[index], &dev);
    if(FAILED(hr)){
        WARN("GetDevice failed: %08x\n", hr);
        return hr;
    }

    hr = IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_INPROC_SERVER,
            NULL, (void**)&client);
    if(FAILED(hr)){
        WARN("Activate failed: %08x\n", hr);
        IMMDevice_Release(dev);
        return hr;
    }

    hr = IMMDevice_OpenPropertyStore(dev, STGM_READ, &ps);
    if(FAILED(hr)){
        WARN("OpenPropertyStore failed: %08x\n", hr);
        IAudioClient_Release(client);
        IMMDevice_Release(dev);
        return hr;
    }

    PropVariantInit(&var);

    hr = IPropertyStore_GetValue(ps, (PROPERTYKEY*)&DEVPKEY_Device_FriendlyName, &var);
    if(FAILED(hr)){
        WARN("GetValue failed: %08x\n", hr);
        goto done;
    }

    lstrcpynW(pDeviceDetails->DisplayName, var.u.pwszVal, sizeof(pDeviceDetails->DisplayName)/sizeof(WCHAR));

    PropVariantClear(&var);

    hr = IAudioClient_GetMixFormat(client, &wfx);
    if(FAILED(hr)){
        WARN("GetMixFormat failed: %08x\n", hr);
        goto done;
    }

    lstrcpyW(pDeviceDetails->DeviceID, This->devids[index]);

    if(index == 0)
        pDeviceDetails->Role = GlobalDefaultDevice;
    else
        pDeviceDetails->Role = NotDefaultDevice;

    if(sizeof(WAVEFORMATEX) + wfx->cbSize > sizeof(pDeviceDetails->OutputFormat)){
        FIXME("AudioClient format is too large to fit into WAVEFORMATEXTENSIBLE!\n");
        CoTaskMemFree(wfx);
        hr = E_FAIL;
        goto done;
    }
    memcpy(&pDeviceDetails->OutputFormat, wfx, sizeof(WAVEFORMATEX) + wfx->cbSize);

    CoTaskMemFree(wfx);

done:
    IPropertyStore_Release(ps);
    IAudioClient_Release(client);
    IMMDevice_Release(dev);

    return hr;
}

static HRESULT WINAPI XA27_Initialize(IXAudio27 *iface, UINT32 flags,
        XAUDIO2_PROCESSOR processor)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    TRACE("(%p)->(0x%x, 0x%x)\n", This, flags, processor);
    return S_OK;
}

static HRESULT WINAPI XA27_RegisterForCallbacks(IXAudio27 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2Impl_RegisterForCallbacks(&This->IXAudio2_iface, pCallback);
}

static void WINAPI XA27_UnregisterForCallbacks(IXAudio27 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    IXAudio2Impl_UnregisterForCallbacks(&This->IXAudio2_iface, pCallback);
}

static HRESULT WINAPI XA27_CreateSourceVoice(IXAudio27 *iface,
        IXAudio2SourceVoice **ppSourceVoice, const WAVEFORMATEX *pSourceFormat,
        UINT32 flags, float maxFrequencyRatio,
        IXAudio2VoiceCallback *pCallback, const XAUDIO2_VOICE_SENDS *pSendList,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2Impl_CreateSourceVoice(&This->IXAudio2_iface, ppSourceVoice,
            pSourceFormat, flags, maxFrequencyRatio, pCallback, pSendList,
            pEffectChain);
}

static HRESULT WINAPI XA27_CreateSubmixVoice(IXAudio27 *iface,
        IXAudio2SubmixVoice **ppSubmixVoice, UINT32 inputChannels,
        UINT32 inputSampleRate, UINT32 flags, UINT32 processingStage,
        const XAUDIO2_VOICE_SENDS *pSendList,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2Impl_CreateSubmixVoice(&This->IXAudio2_iface, ppSubmixVoice,
            inputChannels, inputSampleRate, flags, processingStage, pSendList,
            pEffectChain);
}

static HRESULT WINAPI XA27_CreateMasteringVoice(IXAudio27 *iface,
        IXAudio2MasteringVoice **ppMasteringVoice, UINT32 inputChannels,
        UINT32 inputSampleRate, UINT32 flags, UINT32 deviceIndex,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);

    TRACE("(%p)->(%p, %u, %u, 0x%x, %u, %p)\n", This, ppMasteringVoice,
            inputChannels, inputSampleRate, flags, deviceIndex,
            pEffectChain);

    if(deviceIndex >= This->ndevs)
        return E_INVALIDARG;

    return IXAudio2Impl_CreateMasteringVoice(&This->IXAudio2_iface, ppMasteringVoice,
            inputChannels, inputSampleRate, flags, This->devids[deviceIndex],
            pEffectChain, AudioCategory_GameEffects);
}

static HRESULT WINAPI XA27_StartEngine(IXAudio27 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2Impl_StartEngine(&This->IXAudio2_iface);
}

static void WINAPI XA27_StopEngine(IXAudio27 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2Impl_StopEngine(&This->IXAudio2_iface);
}

static HRESULT WINAPI XA27_CommitChanges(IXAudio27 *iface, UINT32 operationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2Impl_CommitChanges(&This->IXAudio2_iface, operationSet);
}

static void WINAPI XA27_GetPerformanceData(IXAudio27 *iface,
        XAUDIO2_PERFORMANCE_DATA *pPerfData)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2Impl_GetPerformanceData(&This->IXAudio2_iface, pPerfData);
}

static void WINAPI XA27_SetDebugConfiguration(IXAudio27 *iface,
        const XAUDIO2_DEBUG_CONFIGURATION *pDebugConfiguration,
        void *pReserved)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2Impl_SetDebugConfiguration(&This->IXAudio2_iface,
            pDebugConfiguration, pReserved);
}

static const IXAudio27Vtbl XAudio27_Vtbl = {
    XA27_QueryInterface,
    XA27_AddRef,
    XA27_Release,
    XA27_GetDeviceCount,
    XA27_GetDeviceDetails,
    XA27_Initialize,
    XA27_RegisterForCallbacks,
    XA27_UnregisterForCallbacks,
    XA27_CreateSourceVoice,
    XA27_CreateSubmixVoice,
    XA27_CreateMasteringVoice,
    XA27_StartEngine,
    XA27_StopEngine,
    XA27_CommitChanges,
    XA27_GetPerformanceData,
    XA27_SetDebugConfiguration
};

static HRESULT WINAPI XAudio2CF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppobj)
{
    if(IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *ppobj = iface;
        return S_OK;
    }

    *ppobj = NULL;
    WARN("(%p)->(%s, %p): interface not found\n", iface, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI XAudio2CF_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI XAudio2CF_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT initialize_mmdevices(IXAudio2Impl *This)
{
    IMMDeviceCollection *devcoll;
    UINT devcount;
    HRESULT hr;

    if(!This->devenum){
        hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL,
                CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&This->devenum);
        if(FAILED(hr))
            return hr;
    }

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(This->devenum, eRender,
            DEVICE_STATE_ACTIVE, &devcoll);
    if(FAILED(hr)){
        return hr;
    }

    hr = IMMDeviceCollection_GetCount(devcoll, &devcount);
    if(FAILED(hr)){
        IMMDeviceCollection_Release(devcoll);
        return hr;
    }

    if(devcount > 0){
        UINT i, count = 1;
        IMMDevice *dev, *def_dev;

        /* make sure that device 0 is the default device */
        IMMDeviceEnumerator_GetDefaultAudioEndpoint(This->devenum, eRender, eConsole, &def_dev);

        This->devids = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR *) * devcount);

        for(i = 0; i < devcount; ++i){
            hr = IMMDeviceCollection_Item(devcoll, i, &dev);
            if(SUCCEEDED(hr)){
                UINT idx;

                if(dev == def_dev)
                    idx = 0;
                else{
                    idx = count;
                    ++count;
                }

                hr = IMMDevice_GetId(dev, &This->devids[idx]);
                if(FAILED(hr)){
                    WARN("GetId failed: %08x\n", hr);
                    HeapFree(GetProcessHeap(), 0, This->devids);
                    This->devids = NULL;
                    IMMDevice_Release(dev);
                    return hr;
                }

                IMMDevice_Release(dev);
            }else{
                WARN("Item failed: %08x\n", hr);
                HeapFree(GetProcessHeap(), 0, This->devids);
                This->devids = NULL;
                IMMDeviceCollection_Release(devcoll);
                return hr;
            }
        }
    }

    IMMDeviceCollection_Release(devcoll);

    This->ndevs = devcount;

    return S_OK;
}

static HRESULT WINAPI XAudio2CF_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                               REFIID riid, void **ppobj)
{
    HRESULT hr;
    IXAudio2Impl *object;

    TRACE("(static)->(%p,%s,%p)\n", pOuter, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if(pOuter)
        return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if(!object)
        return E_OUTOFMEMORY;

    object->IXAudio27_iface.lpVtbl = &XAudio27_Vtbl;
    object->IXAudio2_iface.lpVtbl = &XAudio2_Vtbl;
    object->IXAudio2MasteringVoice_iface.lpVtbl = &XAudio2MasteringVoice_Vtbl;

    if(IsEqualGUID(riid, &IID_IXAudio27))
        object->version = 27;
    else
        object->version = 28;

    list_init(&object->source_voices);
    list_init(&object->submix_voices);

    object->mmevt = CreateEventW(NULL, FALSE, FALSE, NULL);
    InitializeCriticalSection(&object->lock);
    object->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": IXAudio2Impl.lock");

    hr = IXAudio2_QueryInterface(&object->IXAudio2_iface, riid, ppobj);
    if(FAILED(hr)){
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    hr = initialize_mmdevices(object);
    if(FAILED(hr)){
        IUnknown_Release((IUnknown*)*ppobj);
        return hr;
    }

    object->ncbs = 4;
    object->cbs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, object->ncbs * sizeof(*object->cbs));

    IXAudio2_StartEngine(&object->IXAudio2_iface);

    return hr;
}

static HRESULT WINAPI XAudio2CF_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("(static)->(%d): stub!\n", dolock);
    return S_OK;
}

static const IClassFactoryVtbl XAudio2CF_Vtbl =
{
    XAudio2CF_QueryInterface,
    XAudio2CF_AddRef,
    XAudio2CF_Release,
    XAudio2CF_CreateInstance,
    XAudio2CF_LockServer
};

static IClassFactory xaudio2_cf = { &XAudio2CF_Vtbl };

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    IClassFactory *factory = NULL;

    TRACE("(%s, %s, %p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if(IsEqualGUID(rclsid, &CLSID_XAudio2)) {
        factory = &xaudio2_cf;
    }
    if(!factory) return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface(factory, riid, ppv);
}

/* returns TRUE if there is more data avilable in the buffer, FALSE if the
 * buffer's data has all been queued */
static BOOL xa2buffer_queue_period(XA2SourceImpl *src, XA2Buffer *buf, ALuint al_buf)
{
    UINT32 submit_bytes;
    const BYTE *submit_buf = NULL;

    if(buf->offs_bytes >= buf->xa2buffer.AudioBytes){
        WARN("Shouldn't happen: Trying to push frames from a spent buffer?\n");
        return FALSE;
    }

    submit_bytes = min(src->xa2->period_frames * src->submit_blocksize, buf->xa2buffer.AudioBytes - buf->offs_bytes);
    submit_buf = buf->xa2buffer.pAudioData + buf->offs_bytes;
    buf->offs_bytes += submit_bytes;

    alBufferData(al_buf, src->al_fmt, submit_buf, submit_bytes,
            src->fmt->nSamplesPerSec);

    alSourceQueueBuffers(src->al_src, 1, &al_buf);

    src->in_al_bytes += submit_bytes;
    src->al_bufs_used++;

    buf->latest_al_buf = al_buf;

    TRACE("queueing %u bytes, now %u in AL\n", submit_bytes, src->in_al_bytes);

    return buf->offs_bytes < buf->xa2buffer.AudioBytes;
}

static void update_source_state(XA2SourceImpl *src)
{
    int i;
    ALint processed;
    ALint bufpos;

    alGetSourcei(src->al_src, AL_BUFFERS_PROCESSED, &processed);

    if(processed > 0){
        ALuint al_buffers[XAUDIO2_MAX_QUEUED_BUFFERS];

        alSourceUnqueueBuffers(src->al_src, processed, al_buffers);
        src->first_al_buf += processed;
        src->first_al_buf %= XAUDIO2_MAX_QUEUED_BUFFERS;
        src->al_bufs_used -= processed;

        for(i = 0; i < processed; ++i){
            ALint bufsize;

            alGetBufferi(al_buffers[i], AL_SIZE, &bufsize);

            src->in_al_bytes -= bufsize;
            src->played_frames += bufsize / src->submit_blocksize;

            if(al_buffers[i] == src->buffers[src->first_buf].latest_al_buf){
                DWORD old_buf = src->first_buf;

                src->first_buf++;
                src->first_buf %= XAUDIO2_MAX_QUEUED_BUFFERS;
                src->nbufs--;

                TRACE("%p: done with buffer %u\n", src, old_buf);

                if(src->cb){
                    IXAudio2VoiceCallback_OnBufferEnd(src->cb,
                            src->buffers[old_buf].xa2buffer.pContext);

                    if(src->nbufs > 0)
                        IXAudio2VoiceCallback_OnBufferStart(src->cb,
                                src->buffers[src->first_buf].xa2buffer.pContext);
                }
            }
        }
    }

    alGetSourcei(src->al_src, AL_BYTE_OFFSET, &bufpos);

    /* maintain 4 periods in AL */
    while(src->cur_buf != (src->first_buf + src->nbufs) % XAUDIO2_MAX_QUEUED_BUFFERS &&
            src->in_al_bytes - bufpos < 4 * src->xa2->period_frames * src->submit_blocksize){
        TRACE("%p: going to queue a period from buffer %u\n", src, src->cur_buf);

        /* starting from an empty buffer */
        if(src->cb && src->cur_buf == src->first_buf && src->buffers[src->cur_buf].offs_bytes == 0)
            IXAudio2VoiceCallback_OnBufferStart(src->cb,
                    src->buffers[src->first_buf].xa2buffer.pContext);

        if(!xa2buffer_queue_period(src, &src->buffers[src->cur_buf],
                    src->al_bufs[(src->first_al_buf + src->al_bufs_used) % XAUDIO2_MAX_QUEUED_BUFFERS])){
            /* buffer is spent, move on */
            src->cur_buf++;
            src->cur_buf %= XAUDIO2_MAX_QUEUED_BUFFERS;
        }
    }
}

static void do_engine_tick(IXAudio2Impl *This)
{
    BYTE *buf;
    XA2SourceImpl *src;
    HRESULT hr;
    UINT32 nframes, i, pad;

    /* maintain up to 3 periods in mmdevapi */
    hr = IAudioClient_GetCurrentPadding(This->aclient, &pad);
    if(FAILED(hr)){
        WARN("GetCurrentPadding failed: 0x%x\n", hr);
        return;
    }

    nframes = This->period_frames * 3 - pad;
    TRACE("going to render %u frames\n", nframes);

    if(!nframes)
        return;

    for(i = 0; i < This->ncbs && This->cbs[i]; ++i)
        IXAudio2EngineCallback_OnProcessingPassStart(This->cbs[i]);

    LIST_FOR_EACH_ENTRY(src, &This->source_voices, XA2SourceImpl, entry){
        ALint st = 0;

        EnterCriticalSection(&src->lock);

        if(!src->in_use || !src->running){
            LeaveCriticalSection(&src->lock);
            continue;
        }

        if(src->cb)
            /* TODO: detect incoming underrun and inform callback */
            IXAudio2VoiceCallback_OnVoiceProcessingPassStart(src->cb, 0);

        update_source_state(src);

        alGetSourcei(src->al_src, AL_SOURCE_STATE, &st);
        if(st != AL_PLAYING)
            alSourcePlay(src->al_src);

        if(src->cb)
            IXAudio2VoiceCallback_OnVoiceProcessingPassEnd(src->cb);

        LeaveCriticalSection(&src->lock);
    }

    hr = IAudioRenderClient_GetBuffer(This->render, nframes, &buf);
    if(FAILED(hr))
        WARN("GetBuffer failed: %08x\n", hr);

    palcRenderSamplesSOFT(This->al_device, buf, nframes);

    hr = IAudioRenderClient_ReleaseBuffer(This->render, nframes, 0);
    if(FAILED(hr))
        WARN("ReleaseBuffer failed: %08x\n", hr);

    for(i = 0; i < This->ncbs && This->cbs[i]; ++i)
        IXAudio2EngineCallback_OnProcessingPassEnd(This->cbs[i]);
}

static DWORD WINAPI engine_threadproc(void *arg)
{
    IXAudio2Impl *This = arg;
    while(1){
        WaitForSingleObject(This->mmevt, INFINITE);

        if(This->stop_engine)
            break;

        if(!This->running)
            continue;

        EnterCriticalSection(&This->lock);

        do_engine_tick(This);

        LeaveCriticalSection(&This->lock);
    }
    return 0;
}
