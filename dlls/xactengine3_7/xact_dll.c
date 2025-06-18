/*
 * Copyright (c) 2018 Ethan Lee for CodeWeavers
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
#include <FACT.h>

#define COBJMACROS
#include "objbase.h"

#if XACT3_VER < 0x0300
#include "xact2wb.h"
#include "initguid.h"
#include "xact.h"
#else
#include "xact3wb.h"
#include "xaudio2.h"
#include "initguid.h"
#include "xact3.h"
#endif
#include "wine/debug.h"
#include "wine/rbtree.h"

WINE_DEFAULT_DEBUG_CHANNEL(xaudio2);

#if XACT3_VER < 0x0300
#define IID_IXACT3Engine IID_IXACTEngine
#define IXACT3Cue IXACTCue
#define IXACT3CueVtbl IXACTCueVtbl
#define IXACT3Engine IXACTEngine
#define IXACT3EngineVtbl IXACTEngineVtbl
#define IXACT3Engine_QueryInterface IXACTEngine_QueryInterface
#define IXACT3SoundBank IXACTSoundBank
#define IXACT3SoundBankVtbl IXACTSoundBankVtbl
#define IXACT3Wave IXACTWave
#define IXACT3WaveVtbl IXACTWaveVtbl
#define IXACT3WaveBank IXACTWaveBank
#define IXACT3WaveBankVtbl IXACTWaveBankVtbl
#endif

#define XACTNOTIFICATIONTYPE_MAX 19  /* XACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT + 1 */

struct wrapper_lookup
{
    struct wine_rb_entry entry;
    void *fact;
    void *xact;
};

typedef struct _XACT3EngineImpl {
    IXACT3Engine IXACT3Engine_iface;

    FACTAudioEngine *fact_engine;

    XACT_READFILE_CALLBACK pReadFile;
    XACT_GETOVERLAPPEDRESULT_CALLBACK pGetOverlappedResult;
    XACT_NOTIFICATION_CALLBACK notification_callback;

    void *contexts[XACTNOTIFICATIONTYPE_MAX];
    struct wine_rb_tree wrapper_lookup;
    CRITICAL_SECTION wrapper_lookup_cs;
} XACT3EngineImpl;

static int wrapper_lookup_compare(const void *key, const struct wine_rb_entry *entry)
{
    struct wrapper_lookup *lookup = WINE_RB_ENTRY_VALUE(entry, struct wrapper_lookup, entry);

    return (key > lookup->fact) - (key < lookup->fact);
}

static void wrapper_lookup_destroy(struct wine_rb_entry *entry, void *context)
{
    struct wrapper_lookup *lookup = WINE_RB_ENTRY_VALUE(entry, struct wrapper_lookup, entry);

    HeapFree(GetProcessHeap(), 0, lookup);
}

static void wrapper_remove_entry(XACT3EngineImpl *engine, void *key)
{
    struct wrapper_lookup *lookup;
    struct wine_rb_entry *entry;

    EnterCriticalSection(&engine->wrapper_lookup_cs);

    entry = wine_rb_get(&engine->wrapper_lookup, key);
    if (!entry)
    {
        LeaveCriticalSection(&engine->wrapper_lookup_cs);

        WARN("cannot find key in wrapper lookup\n");
    }
    else
    {
        wine_rb_remove(&engine->wrapper_lookup, entry);

        LeaveCriticalSection(&engine->wrapper_lookup_cs);

        lookup = WINE_RB_ENTRY_VALUE(entry, struct wrapper_lookup, entry);
        HeapFree(GetProcessHeap(), 0, lookup);
    }
}

static HRESULT wrapper_add_entry(XACT3EngineImpl *engine, void *fact, void *xact)
{
    struct wrapper_lookup *lookup;
    UINT ret;

    lookup = HeapAlloc(GetProcessHeap(), 0, sizeof(*lookup));
    if (!lookup)
    {
        ERR("Failed to allocate wrapper_lookup!\n");
        return E_OUTOFMEMORY;
    }
    lookup->fact = fact;
    lookup->xact = xact;

    EnterCriticalSection(&engine->wrapper_lookup_cs);
    ret = wine_rb_put(&engine->wrapper_lookup, lookup->fact, &lookup->entry);
    LeaveCriticalSection(&engine->wrapper_lookup_cs);

    if (ret)
    {
        WARN("wrapper_lookup already present in the tree??\n");
        HeapFree(GetProcessHeap(), 0, lookup);
    }

    return S_OK;
}

/* Must be protected by engine->wrapper_lookup_cs */
static void* wrapper_find_entry(XACT3EngineImpl *engine, void *faudio)
{
    struct wrapper_lookup *lookup;
    struct wine_rb_entry *entry;

    entry = wine_rb_get(&engine->wrapper_lookup, faudio);
    if (entry)
    {
        lookup = WINE_RB_ENTRY_VALUE(entry, struct wrapper_lookup, entry);
        return lookup->xact;
    }

    WARN("cannot find interface in wrapper lookup\n");
    return NULL;
}

typedef struct _XACT3CueImpl {
    IXACT3Cue IXACT3Cue_iface;
    FACTCue *fact_cue;
    XACT3EngineImpl *engine;
} XACT3CueImpl;

static inline XACT3CueImpl *impl_from_IXACT3Cue(IXACT3Cue *iface)
{
    return CONTAINING_RECORD(iface, XACT3CueImpl, IXACT3Cue_iface);
}

static HRESULT WINAPI IXACT3CueImpl_Play(IXACT3Cue *iface)
{
    XACT3CueImpl *This = impl_from_IXACT3Cue(iface);

    TRACE("(%p)\n", iface);

    return FACTCue_Play(This->fact_cue);
}

static HRESULT WINAPI IXACT3CueImpl_Stop(IXACT3Cue *iface, DWORD dwFlags)
{
    XACT3CueImpl *This = impl_from_IXACT3Cue(iface);

    TRACE("(%p)->(%lu)\n", iface, dwFlags);

    return FACTCue_Stop(This->fact_cue, dwFlags);
}

static HRESULT WINAPI IXACT3CueImpl_GetState(IXACT3Cue *iface, DWORD *pdwState)
{
    XACT3CueImpl *This = impl_from_IXACT3Cue(iface);

    TRACE("(%p)->(%p)\n", iface, pdwState);

    return FACTCue_GetState(This->fact_cue, (uint32_t *)pdwState);
}

static HRESULT WINAPI IXACT3CueImpl_Destroy(IXACT3Cue *iface)
{
    XACT3CueImpl *This = impl_from_IXACT3Cue(iface);
    UINT ret;

    TRACE("(%p)\n", iface);

    ret = FACTCue_Destroy(This->fact_cue);
    if (ret != 0)
        WARN("FACTCue_Destroy returned %d\n", ret);
    wrapper_remove_entry(This->engine, This->fact_cue);
    HeapFree(GetProcessHeap(), 0, This);
    return S_OK;
}

#if XACT3_VER < 0x0300

static HRESULT WINAPI IXACT3CueImpl_GetChannelMap(IXACT3Cue *iface,
        XACTCHANNELMAP *map, DWORD size, DWORD *needed_size)
{
    FIXME("(%p)->(%p, %lu, %p)\n", iface, map, size, needed_size);

    return E_NOTIMPL;
}

static HRESULT WINAPI IXACT3CueImpl_SetChannelMap(IXACT3Cue *iface, XACTCHANNELMAP *map)
{
    FIXME("(%p)->(%p)\n", iface, map);

    return E_NOTIMPL;
}

static HRESULT WINAPI IXACT3CueImpl_GetChannelVolume(IXACT3Cue *iface, XACTCHANNELVOLUME *volume)
{
    FIXME("(%p)->(%p)\n", iface, volume);

    return E_NOTIMPL;
}

static HRESULT WINAPI IXACT3CueImpl_SetChannelVolume(IXACT3Cue *iface, XACTCHANNELVOLUME *volume)
{
    FIXME("(%p)->(%p)\n", iface, volume);

    return E_NOTIMPL;
}

#endif

static HRESULT WINAPI IXACT3CueImpl_SetMatrixCoefficients(IXACT3Cue *iface,
        UINT32 uSrcChannelCount, UINT32 uDstChannelCount,
        float *pMatrixCoefficients)
{
    XACT3CueImpl *This = impl_from_IXACT3Cue(iface);

    TRACE("(%p)->(%u, %u, %p)\n", iface, uSrcChannelCount, uDstChannelCount,
            pMatrixCoefficients);

    return FACTCue_SetMatrixCoefficients(This->fact_cue, uSrcChannelCount,
        uDstChannelCount, pMatrixCoefficients);
}

static XACTVARIABLEINDEX WINAPI IXACT3CueImpl_GetVariableIndex(IXACT3Cue *iface,
        PCSTR szFriendlyName)
{
    XACT3CueImpl *This = impl_from_IXACT3Cue(iface);

    TRACE("(%p)->(%s)\n", iface, szFriendlyName);

    return FACTCue_GetVariableIndex(This->fact_cue, szFriendlyName);
}

static HRESULT WINAPI IXACT3CueImpl_SetVariable(IXACT3Cue *iface,
        XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE nValue)
{
    XACT3CueImpl *This = impl_from_IXACT3Cue(iface);

    TRACE("(%p)->(%u, %f)\n", iface, nIndex, nValue);

    return FACTCue_SetVariable(This->fact_cue, nIndex, nValue);
}

static HRESULT WINAPI IXACT3CueImpl_GetVariable(IXACT3Cue *iface,
        XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE *nValue)
{
    XACT3CueImpl *This = impl_from_IXACT3Cue(iface);

    TRACE("(%p)->(%u, %p)\n", iface, nIndex, nValue);

    return FACTCue_GetVariable(This->fact_cue, nIndex, nValue);
}

static HRESULT WINAPI IXACT3CueImpl_Pause(IXACT3Cue *iface, BOOL fPause)
{
    XACT3CueImpl *This = impl_from_IXACT3Cue(iface);

    TRACE("(%p)->(%u)\n", iface, fPause);

    return FACTCue_Pause(This->fact_cue, fPause);
}

#if XACT3_VER >= 0x0205
static HRESULT WINAPI IXACT3CueImpl_GetProperties(IXACT3Cue *iface,
        XACT_CUE_INSTANCE_PROPERTIES **ppProperties)
{
    XACT3CueImpl *This = impl_from_IXACT3Cue(iface);
    FACTCueInstanceProperties *fProps;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", iface, ppProperties);

    hr = FACTCue_GetProperties(This->fact_cue, &fProps);
    if(FAILED(hr))
        return hr;

    *ppProperties = (XACT_CUE_INSTANCE_PROPERTIES*) fProps;
    return hr;
}
#endif

#if XACT3_VER >= 0x0305
static HRESULT WINAPI IXACT3CueImpl_SetOutputVoices(IXACT3Cue *iface,
        const XAUDIO2_VOICE_SENDS *pSendList)
{
    XACT3CueImpl *This = impl_from_IXACT3Cue(iface);
    FIXME("(%p)->(%p): stub!\n", This, pSendList);
    return S_OK;
}

static HRESULT WINAPI IXACT3CueImpl_SetOutputVoiceMatrix(IXACT3Cue *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix)
{
    XACT3CueImpl *This = impl_from_IXACT3Cue(iface);
    FIXME("(%p)->(%p %u %u %p): stub!\n", This, pDestinationVoice, SourceChannels,
            DestinationChannels, pLevelMatrix);
    return S_OK;
}
#endif

static const IXACT3CueVtbl XACT3Cue_Vtbl =
{
    IXACT3CueImpl_Play,
    IXACT3CueImpl_Stop,
    IXACT3CueImpl_GetState,
    IXACT3CueImpl_Destroy,
#if XACT3_VER < 0x0300
    IXACT3CueImpl_GetChannelMap,
    IXACT3CueImpl_SetChannelMap,
    IXACT3CueImpl_GetChannelVolume,
    IXACT3CueImpl_SetChannelVolume,
#endif
    IXACT3CueImpl_SetMatrixCoefficients,
    IXACT3CueImpl_GetVariableIndex,
    IXACT3CueImpl_SetVariable,
    IXACT3CueImpl_GetVariable,
    IXACT3CueImpl_Pause,
#if XACT3_VER >= 0x0205
    IXACT3CueImpl_GetProperties,
#endif
#if XACT3_VER >= 0x0305
    IXACT3CueImpl_SetOutputVoices,
    IXACT3CueImpl_SetOutputVoiceMatrix
#endif
};

typedef struct _XACT3SoundBankImpl {
    IXACT3SoundBank IXACT3SoundBank_iface;

    FACTSoundBank *fact_soundbank;
    XACT3EngineImpl *engine;
} XACT3SoundBankImpl;

static inline XACT3SoundBankImpl *impl_from_IXACT3SoundBank(IXACT3SoundBank *iface)
{
    return CONTAINING_RECORD(iface, XACT3SoundBankImpl, IXACT3SoundBank_iface);
}

static XACTINDEX WINAPI IXACT3SoundBankImpl_GetCueIndex(IXACT3SoundBank *iface,
        PCSTR szFriendlyName)
{
    XACT3SoundBankImpl *This = impl_from_IXACT3SoundBank(iface);

    TRACE("(%p)->(%s)\n", This, szFriendlyName);

    return FACTSoundBank_GetCueIndex(This->fact_soundbank, szFriendlyName);
}

#if XACT3_VER >= 0x0205
static HRESULT WINAPI IXACT3SoundBankImpl_GetNumCues(IXACT3SoundBank *iface,
        XACTINDEX *pnNumCues)
{
    XACT3SoundBankImpl *This = impl_from_IXACT3SoundBank(iface);

    TRACE("(%p)->(%p)\n", This, pnNumCues);

    return FACTSoundBank_GetNumCues(This->fact_soundbank, pnNumCues);
}

static HRESULT WINAPI IXACT3SoundBankImpl_GetCueProperties(IXACT3SoundBank *iface,
        XACTINDEX nCueIndex, XACT_CUE_PROPERTIES *pProperties)
{
    XACT3SoundBankImpl *This = impl_from_IXACT3SoundBank(iface);

    TRACE("(%p)->(%u, %p)\n", This, nCueIndex, pProperties);

    return FACTSoundBank_GetCueProperties(This->fact_soundbank, nCueIndex,
            (FACTCueProperties*) pProperties);
}
#endif

static HRESULT WINAPI IXACT3SoundBankImpl_Prepare(IXACT3SoundBank *iface,
        XACTINDEX nCueIndex, DWORD dwFlags, XACTTIME timeOffset,
        IXACT3Cue** ppCue)
{
    XACT3SoundBankImpl *This = impl_from_IXACT3SoundBank(iface);
    XACT3CueImpl *cue;
    FACTCue *fcue;
    UINT ret;
    HRESULT hr;

    TRACE("(%p)->(%u, 0x%lx, %lu, %p)\n", This, nCueIndex, dwFlags, timeOffset,
            ppCue);

    ret = FACTSoundBank_Prepare(This->fact_soundbank, nCueIndex, dwFlags,
            timeOffset, &fcue);
    if(ret != 0)
    {
        ERR("Failed to CreateCue: %d\n", ret);
        return E_FAIL;
    }

    cue = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*cue));
    if (!cue)
    {
        FACTCue_Destroy(fcue);
        ERR("Failed to allocate XACT3CueImpl!\n");
        return E_OUTOFMEMORY;
    }

    hr = wrapper_add_entry(This->engine, fcue, &cue->IXACT3Cue_iface);
    if (FAILED(hr))
    {
        FACTCue_Destroy(fcue);
        HeapFree(GetProcessHeap(), 0, cue);
        return hr;
    }

    cue->IXACT3Cue_iface.lpVtbl = &XACT3Cue_Vtbl;
    cue->fact_cue = fcue;
    cue->engine = This->engine;
    *ppCue = &cue->IXACT3Cue_iface;

    TRACE("Created Cue: %p\n", cue);

    return S_OK;
}

static HRESULT WINAPI IXACT3SoundBankImpl_Play(IXACT3SoundBank *iface,
        XACTINDEX nCueIndex, DWORD dwFlags, XACTTIME timeOffset,
        IXACT3Cue** ppCue)
{
    XACT3SoundBankImpl *This = impl_from_IXACT3SoundBank(iface);
    XACT3CueImpl *cue;
    FACTCue *fcue;
    HRESULT hr;

    TRACE("(%p)->(%u, 0x%lx, %lu, %p)\n", This, nCueIndex, dwFlags, timeOffset,
            ppCue);

    /* If the application doesn't want a handle, don't generate one at all.
     * Let the engine handle that memory instead.
     * -flibit
     */
    if (ppCue == NULL){
        hr = FACTSoundBank_Play(This->fact_soundbank, nCueIndex, dwFlags,
                timeOffset, NULL);
    }else{
        hr = FACTSoundBank_Play(This->fact_soundbank, nCueIndex, dwFlags,
                timeOffset, &fcue);
        if(FAILED(hr))
            return hr;

        cue = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*cue));
        if (!cue)
        {
            FACTCue_Destroy(fcue);
            ERR("Failed to allocate XACT3CueImpl!\n");
            return E_OUTOFMEMORY;
        }

        hr = wrapper_add_entry(This->engine, fcue, &cue->IXACT3Cue_iface);
        if (FAILED(hr))
        {
            FACTCue_Destroy(fcue);
            HeapFree(GetProcessHeap(), 0, cue);
            return hr;
        }

        cue->IXACT3Cue_iface.lpVtbl = &XACT3Cue_Vtbl;
        cue->fact_cue = fcue;
        cue->engine = This->engine;
        *ppCue = &cue->IXACT3Cue_iface;
    }

    return hr;
}

static HRESULT WINAPI IXACT3SoundBankImpl_Stop(IXACT3SoundBank *iface,
        XACTINDEX nCueIndex, DWORD dwFlags)
{
    XACT3SoundBankImpl *This = impl_from_IXACT3SoundBank(iface);

    TRACE("(%p)->(%lu)\n", This, dwFlags);

    return FACTSoundBank_Stop(This->fact_soundbank, nCueIndex, dwFlags);
}

static HRESULT WINAPI IXACT3SoundBankImpl_Destroy(IXACT3SoundBank *iface)
{
    XACT3SoundBankImpl *This = impl_from_IXACT3SoundBank(iface);
    HRESULT hr;

    TRACE("(%p)\n", This);

    hr = FACTSoundBank_Destroy(This->fact_soundbank);

    wrapper_remove_entry(This->engine, This->fact_soundbank);
    HeapFree(GetProcessHeap(), 0, This);
    return hr;
}

static HRESULT WINAPI IXACT3SoundBankImpl_GetState(IXACT3SoundBank *iface,
        DWORD *pdwState)
{
    XACT3SoundBankImpl *This = impl_from_IXACT3SoundBank(iface);

    TRACE("(%p)->(%p)\n", This, pdwState);

    return FACTSoundBank_GetState(This->fact_soundbank, (uint32_t *)pdwState);
}

static const IXACT3SoundBankVtbl XACT3SoundBank_Vtbl =
{
    IXACT3SoundBankImpl_GetCueIndex,
#if XACT3_VER >= 0x0205
    IXACT3SoundBankImpl_GetNumCues,
    IXACT3SoundBankImpl_GetCueProperties,
#endif
    IXACT3SoundBankImpl_Prepare,
    IXACT3SoundBankImpl_Play,
    IXACT3SoundBankImpl_Stop,
    IXACT3SoundBankImpl_Destroy,
    IXACT3SoundBankImpl_GetState
};

#if XACT3_VER >= 0x0205

typedef struct _XACT3WaveImpl {
    IXACT3Wave IXACT3Wave_iface;

    FACTWave *fact_wave;
    XACT3EngineImpl *engine;
} XACT3WaveImpl;

static inline XACT3WaveImpl *impl_from_IXACT3Wave(IXACT3Wave *iface)
{
    return CONTAINING_RECORD(iface, XACT3WaveImpl, IXACT3Wave_iface);
}

static HRESULT WINAPI IXACT3WaveImpl_Destroy(IXACT3Wave *iface)
{
    XACT3WaveImpl *This = impl_from_IXACT3Wave(iface);
    HRESULT hr;

    TRACE("(%p)\n", This);

    hr = FACTWave_Destroy(This->fact_wave);
    wrapper_remove_entry(This->engine, This->fact_wave);
    HeapFree(GetProcessHeap(), 0, This);
    return hr;
}

static HRESULT WINAPI IXACT3WaveImpl_Play(IXACT3Wave *iface)
{
    XACT3WaveImpl *This = impl_from_IXACT3Wave(iface);

    TRACE("(%p)\n", This);

    return FACTWave_Play(This->fact_wave);
}

static HRESULT WINAPI IXACT3WaveImpl_Stop(IXACT3Wave *iface, DWORD dwFlags)
{
    XACT3WaveImpl *This = impl_from_IXACT3Wave(iface);

    TRACE("(%p)->(0x%lx)\n", This, dwFlags);

    return FACTWave_Stop(This->fact_wave, dwFlags);
}

static HRESULT WINAPI IXACT3WaveImpl_Pause(IXACT3Wave *iface, BOOL fPause)
{
    XACT3WaveImpl *This = impl_from_IXACT3Wave(iface);

    TRACE("(%p)->(%u)\n", This, fPause);

    return FACTWave_Pause(This->fact_wave, fPause);
}

static HRESULT WINAPI IXACT3WaveImpl_GetState(IXACT3Wave *iface, DWORD *pdwState)
{
    XACT3WaveImpl *This = impl_from_IXACT3Wave(iface);

    TRACE("(%p)->(%p)\n", This, pdwState);

    return FACTWave_GetState(This->fact_wave, (uint32_t *)pdwState);
}

static HRESULT WINAPI IXACT3WaveImpl_SetPitch(IXACT3Wave *iface, XACTPITCH pitch)
{
    XACT3WaveImpl *This = impl_from_IXACT3Wave(iface);

    TRACE("(%p)->(%d)\n", This, pitch);

    return FACTWave_SetPitch(This->fact_wave, pitch);
}

static HRESULT WINAPI IXACT3WaveImpl_SetVolume(IXACT3Wave *iface, XACTVOLUME volume)
{
    XACT3WaveImpl *This = impl_from_IXACT3Wave(iface);

    TRACE("(%p)->(%f)\n", This, volume);

    return FACTWave_SetVolume(This->fact_wave, volume);
}

static HRESULT WINAPI IXACT3WaveImpl_SetMatrixCoefficients(IXACT3Wave *iface,
        UINT32 uSrcChannelCount, UINT32 uDstChannelCount,
        float *pMatrixCoefficients)
{
    XACT3WaveImpl *This = impl_from_IXACT3Wave(iface);

    TRACE("(%p)->(%u, %u, %p)\n", This, uSrcChannelCount, uDstChannelCount,
            pMatrixCoefficients);

    return FACTWave_SetMatrixCoefficients(This->fact_wave, uSrcChannelCount,
            uDstChannelCount, pMatrixCoefficients);
}

static HRESULT WINAPI IXACT3WaveImpl_GetProperties(IXACT3Wave *iface,
    XACT_WAVE_INSTANCE_PROPERTIES *pProperties)
{
    XACT3WaveImpl *This = impl_from_IXACT3Wave(iface);

    TRACE("(%p)->(%p)\n", This, pProperties);

    return FACTWave_GetProperties(This->fact_wave,
            (FACTWaveInstanceProperties*) pProperties);
}

static const IXACT3WaveVtbl XACT3Wave_Vtbl =
{
    IXACT3WaveImpl_Destroy,
    IXACT3WaveImpl_Play,
    IXACT3WaveImpl_Stop,
    IXACT3WaveImpl_Pause,
    IXACT3WaveImpl_GetState,
    IXACT3WaveImpl_SetPitch,
    IXACT3WaveImpl_SetVolume,
    IXACT3WaveImpl_SetMatrixCoefficients,
    IXACT3WaveImpl_GetProperties
};

#endif

typedef struct _XACT3WaveBankImpl {
    IXACT3WaveBank IXACT3WaveBank_iface;

    FACTWaveBank *fact_wavebank;
    struct _XACT3EngineImpl *engine;
} XACT3WaveBankImpl;

static inline XACT3WaveBankImpl *impl_from_IXACT3WaveBank(IXACT3WaveBank *iface)
{
    return CONTAINING_RECORD(iface, XACT3WaveBankImpl, IXACT3WaveBank_iface);
}

static HRESULT WINAPI IXACT3WaveBankImpl_Destroy(IXACT3WaveBank *iface)
{
    XACT3WaveBankImpl *This = impl_from_IXACT3WaveBank(iface);
    HRESULT hr;

    TRACE("(%p)\n", This);

    hr = FACTWaveBank_Destroy(This->fact_wavebank);

    wrapper_remove_entry(This->engine, This->fact_wavebank);

    HeapFree(GetProcessHeap(), 0, This);
    return hr;
}

#if XACT3_VER >= 0x0205

static HRESULT WINAPI IXACT3WaveBankImpl_GetNumWaves(IXACT3WaveBank *iface,
        XACTINDEX *pnNumWaves)
{
    XACT3WaveBankImpl *This = impl_from_IXACT3WaveBank(iface);

    TRACE("(%p)->(%p)\n", This, pnNumWaves);

    return FACTWaveBank_GetNumWaves(This->fact_wavebank, pnNumWaves);
}

static XACTINDEX WINAPI IXACT3WaveBankImpl_GetWaveIndex(IXACT3WaveBank *iface,
        PCSTR szFriendlyName)
{
    XACT3WaveBankImpl *This = impl_from_IXACT3WaveBank(iface);

    TRACE("(%p)->(%s)\n", This, szFriendlyName);

    return FACTWaveBank_GetWaveIndex(This->fact_wavebank, szFriendlyName);
}

static HRESULT WINAPI IXACT3WaveBankImpl_GetWaveProperties(IXACT3WaveBank *iface,
        XACTINDEX nWaveIndex, XACT_WAVE_PROPERTIES *pWaveProperties)
{
    XACT3WaveBankImpl *This = impl_from_IXACT3WaveBank(iface);

    TRACE("(%p)->(%u, %p)\n", This, nWaveIndex, pWaveProperties);

    return FACTWaveBank_GetWaveProperties(This->fact_wavebank, nWaveIndex,
            (FACTWaveProperties*) pWaveProperties);
}

static HRESULT WINAPI IXACT3WaveBankImpl_Prepare(IXACT3WaveBank *iface,
        XACTINDEX nWaveIndex, DWORD dwFlags, DWORD dwPlayOffset,
        XACTLOOPCOUNT nLoopCount, IXACT3Wave** ppWave)
{
    XACT3WaveBankImpl *This = impl_from_IXACT3WaveBank(iface);
    XACT3WaveImpl *wave;
    FACTWave *fwave;
    UINT ret;
    HRESULT hr;

    TRACE("(%p)->(0x%x, %lu, 0x%lx, %u, %p)\n", This, nWaveIndex, dwFlags,
            dwPlayOffset, nLoopCount, ppWave);

    ret = FACTWaveBank_Prepare(This->fact_wavebank, nWaveIndex, dwFlags,
            dwPlayOffset, nLoopCount, &fwave);
    if(ret != 0)
    {
        ERR("Failed to CreateWave: %d\n", ret);
        return E_FAIL;
    }

    wave = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wave));
    if (!wave)
    {
        FACTWave_Destroy(fwave);
        ERR("Failed to allocate XACT3WaveImpl!\n");
        return E_OUTOFMEMORY;
    }

    hr = wrapper_add_entry(This->engine, fwave, &wave->IXACT3Wave_iface);
    if (FAILED(hr))
    {
        FACTWave_Destroy(fwave);
        HeapFree(GetProcessHeap(), 0, wave);
        return hr;
    }

    wave->IXACT3Wave_iface.lpVtbl = &XACT3Wave_Vtbl;
    wave->fact_wave = fwave;
    wave->engine = This->engine;
    *ppWave = &wave->IXACT3Wave_iface;

    TRACE("Created Wave: %p\n", wave);

    return S_OK;
}

static HRESULT WINAPI IXACT3WaveBankImpl_Play(IXACT3WaveBank *iface,
        XACTINDEX nWaveIndex, DWORD dwFlags, DWORD dwPlayOffset,
        XACTLOOPCOUNT nLoopCount, IXACT3Wave** ppWave)
{
    XACT3WaveBankImpl *This = impl_from_IXACT3WaveBank(iface);
    XACT3WaveImpl *wave;
    FACTWave *fwave;
    HRESULT hr;

    TRACE("(%p)->(0x%x, %lu, 0x%lx, %u, %p)\n", This, nWaveIndex, dwFlags, dwPlayOffset,
            nLoopCount, ppWave);

    /* If the application doesn't want a handle, don't generate one at all.
     * Let the engine handle that memory instead.
     * -flibit
     */
    if (ppWave == NULL){
        hr = FACTWaveBank_Play(This->fact_wavebank, nWaveIndex, dwFlags,
                dwPlayOffset, nLoopCount, NULL);
    }else{
        hr = FACTWaveBank_Play(This->fact_wavebank, nWaveIndex, dwFlags,
                dwPlayOffset, nLoopCount, &fwave);
        if(FAILED(hr))
            return hr;

        wave = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wave));
        if (!wave)
        {
            FACTWave_Destroy(fwave);
            ERR("Failed to allocate XACT3WaveImpl!\n");
            return E_OUTOFMEMORY;
        }

        hr = wrapper_add_entry(This->engine, fwave, &wave->IXACT3Wave_iface);
        if (FAILED(hr))
        {
            FACTWave_Destroy(fwave);
            HeapFree(GetProcessHeap(), 0, wave);
            return hr;
        }

        wave->IXACT3Wave_iface.lpVtbl = &XACT3Wave_Vtbl;
        wave->fact_wave = fwave;
        wave->engine = This->engine;
        *ppWave = &wave->IXACT3Wave_iface;
    }

    return hr;
}

static HRESULT WINAPI IXACT3WaveBankImpl_Stop(IXACT3WaveBank *iface,
        XACTINDEX nWaveIndex, DWORD dwFlags)
{
    XACT3WaveBankImpl *This = impl_from_IXACT3WaveBank(iface);

    TRACE("(%p)->(%u, %lu)\n", This, nWaveIndex, dwFlags);

    return FACTWaveBank_Stop(This->fact_wavebank, nWaveIndex, dwFlags);
}

#endif

static HRESULT WINAPI IXACT3WaveBankImpl_GetState(IXACT3WaveBank *iface,
        DWORD *pdwState)
{
    XACT3WaveBankImpl *This = impl_from_IXACT3WaveBank(iface);

    TRACE("(%p)->(%p)\n", This, pdwState);

    return FACTWaveBank_GetState(This->fact_wavebank, (uint32_t *)pdwState);
}

static const IXACT3WaveBankVtbl XACT3WaveBank_Vtbl =
{
    IXACT3WaveBankImpl_Destroy,
#if XACT3_VER >= 0x0205
    IXACT3WaveBankImpl_GetNumWaves,
    IXACT3WaveBankImpl_GetWaveIndex,
    IXACT3WaveBankImpl_GetWaveProperties,
    IXACT3WaveBankImpl_Prepare,
    IXACT3WaveBankImpl_Play,
    IXACT3WaveBankImpl_Stop,
#endif
    IXACT3WaveBankImpl_GetState
};

typedef struct wrap_readfile_struct {
    XACT3EngineImpl *engine;
    HANDLE file;
} wrap_readfile_struct;

static int32_t FACTCALL wrap_readfile(
    void* hFile,
    void* lpBuffer,
    uint32_t nNumberOfBytesRead,
    uint32_t *lpNumberOfBytesRead,
    FACTOverlapped *lpOverlapped)
{
    wrap_readfile_struct *wrap = (wrap_readfile_struct*) hFile;
    return wrap->engine->pReadFile(wrap->file, lpBuffer, nNumberOfBytesRead,
            (DWORD *)lpNumberOfBytesRead, (LPOVERLAPPED)lpOverlapped);
}

static int32_t FACTCALL wrap_getoverlappedresult(
    void* hFile,
    FACTOverlapped *lpOverlapped,
    uint32_t *lpNumberOfBytesTransferred,
    int32_t bWait)
{
    wrap_readfile_struct *wrap = (wrap_readfile_struct*) hFile;
    return wrap->engine->pGetOverlappedResult(wrap->file, (LPOVERLAPPED)lpOverlapped,
            (DWORD *)lpNumberOfBytesTransferred, bWait);
}

static inline XACT3EngineImpl *impl_from_IXACT3Engine(IXACT3Engine *iface)
{
    return CONTAINING_RECORD(iface, XACT3EngineImpl, IXACT3Engine_iface);
}

static HRESULT WINAPI IXACT3EngineImpl_QueryInterface(IXACT3Engine *iface,
        REFIID riid, void **ppvObject)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObject);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IXACT3Engine)){
        *ppvObject = &This->IXACT3Engine_iface;
    }
    else
        *ppvObject = NULL;

    if (*ppvObject){
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    FIXME("(%p)->(%s,%p), not found\n", This, debugstr_guid(riid), ppvObject);

    return E_NOINTERFACE;
}

static ULONG WINAPI IXACT3EngineImpl_AddRef(IXACT3Engine *iface)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);
    ULONG ref = FACTAudioEngine_AddRef(This->fact_engine);
    TRACE("(%p)->(): Refcount now %lu\n", This, ref);
    return ref;
}

static ULONG WINAPI IXACT3EngineImpl_Release(IXACT3Engine *iface)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);
    ULONG ref = FACTAudioEngine_Release(This->fact_engine);

    TRACE("(%p)->(): Refcount now %lu\n", This, ref);

    if (!ref)
    {
        DeleteCriticalSection(&This->wrapper_lookup_cs);
        wine_rb_destroy(&This->wrapper_lookup, wrapper_lookup_destroy, NULL);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI IXACT3EngineImpl_GetRendererCount(IXACT3Engine *iface,
        XACTINDEX *pnRendererCount)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);

    TRACE("(%p)->(%p)\n", This, pnRendererCount);

    return FACTAudioEngine_GetRendererCount(This->fact_engine, pnRendererCount);
}

static HRESULT WINAPI IXACT3EngineImpl_GetRendererDetails(IXACT3Engine *iface,
        XACTINDEX nRendererIndex, XACT_RENDERER_DETAILS *pRendererDetails)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);

    TRACE("(%p)->(%d, %p)\n", This, nRendererIndex, pRendererDetails);

    return FACTAudioEngine_GetRendererDetails(This->fact_engine,
            nRendererIndex, (FACTRendererDetails*) pRendererDetails);
}

#if XACT3_VER >= 0x0205

static HRESULT WINAPI IXACT3EngineImpl_GetFinalMixFormat(IXACT3Engine *iface,
        WAVEFORMATEXTENSIBLE *pFinalMixFormat)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);

    TRACE("(%p)->(%p)\n", This, pFinalMixFormat);

    return FACTAudioEngine_GetFinalMixFormat(This->fact_engine,
            (FAudioWaveFormatExtensible*) pFinalMixFormat);
}

#endif

static XACTNOTIFICATIONTYPE xact_notification_type_from_fact(uint8_t type)
{
    /* we can't use a switch statement, because the constants are static const
     * variables, and some compilers can't deal with that */
#define X(a) if (type == FACTNOTIFICATIONTYPE_##a) return XACTNOTIFICATIONTYPE_##a
    X(CUEPREPARED);
    X(CUEPLAY);
    X(CUESTOP);
    X(CUEDESTROYED);
    X(MARKER);
    X(SOUNDBANKDESTROYED);
    X(WAVEBANKDESTROYED);
    X(LOCALVARIABLECHANGED);
    X(GLOBALVARIABLECHANGED);
    X(GUICONNECTED);
    X(GUIDISCONNECTED);
    X(WAVEPLAY);
    X(WAVESTOP);
    X(WAVEBANKPREPARED);
    X(WAVEBANKSTREAMING_INVALIDCONTENT);
#if XACT3_VER >= 0x0205
    X(WAVEPREPARED);
    X(WAVELOOPED);
    X(WAVEDESTROYED);
#endif
#undef X

    FIXME("unknown type %#x\n", type);
    return 0;
}

static void FACTCALL fact_notification_cb(const FACTNotification *notification)
{
    XACT3EngineImpl *engine = (XACT3EngineImpl *)notification->pvContext;
    XACT_NOTIFICATION xnotification;

    TRACE("notification %d, context %p\n", notification->type, notification->pvContext);

    /* Older versions of FAudio don't pass through the context */
    if (!engine)
    {
        WARN("Notification context is NULL\n");
        return;
    }

    xnotification.type = xact_notification_type_from_fact(notification->type);
    xnotification.timeStamp = notification->timeStamp;
    xnotification.pvContext = engine->contexts[notification->type];

    EnterCriticalSection(&engine->wrapper_lookup_cs);
    if (notification->type == XACTNOTIFICATIONTYPE_WAVEBANKPREPARED
            || notification->type == XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED)
    {
        xnotification.waveBank.pWaveBank = wrapper_find_entry(engine, notification->waveBank.pWaveBank);
    }
    else if(notification->type == XACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED)
    {
        xnotification.soundBank.pSoundBank = wrapper_find_entry(engine, notification->soundBank.pSoundBank);
    }
    else if (notification->type == XACTNOTIFICATIONTYPE_WAVESTOP
#if XACT3_VER >= 0x0205
             || notification->type == XACTNOTIFICATIONTYPE_WAVEDESTROYED
             || notification->type == XACTNOTIFICATIONTYPE_WAVELOOPED
             || notification->type == XACTNOTIFICATIONTYPE_WAVEPLAY
             || notification->type == XACTNOTIFICATIONTYPE_WAVEPREPARED)
#else
             )
#endif
    {
        xnotification.wave.cueIndex = notification->wave.cueIndex;
        xnotification.wave.pCue = wrapper_find_entry(engine, notification->wave.pCue);
        xnotification.wave.pSoundBank = wrapper_find_entry(engine, notification->wave.pSoundBank);
#if XACT3_VER >= 0x0205
        xnotification.wave.pWave = wrapper_find_entry(engine, notification->wave.pWave);
#endif
        xnotification.wave.pWaveBank = wrapper_find_entry(engine, notification->wave.pWaveBank);
    }
    else if (notification->type == XACTNOTIFICATIONTYPE_CUEPLAY ||
             notification->type == XACTNOTIFICATIONTYPE_CUEPREPARED ||
             notification->type == XACTNOTIFICATIONTYPE_CUESTOP ||
             notification->type == XACTNOTIFICATIONTYPE_CUEDESTROYED)
    {
        xnotification.cue.pCue = wrapper_find_entry(engine, notification->cue.pCue);
        xnotification.cue.cueIndex = notification->cue.cueIndex;
        xnotification.cue.pSoundBank = wrapper_find_entry(engine, notification->cue.pSoundBank);
    }
    else
    {
        LeaveCriticalSection(&engine->wrapper_lookup_cs);
        FIXME("Unsupported callback type %d\n", notification->type);
        return;
    }
    LeaveCriticalSection(&engine->wrapper_lookup_cs);

    engine->notification_callback(&xnotification);
}

static HRESULT WINAPI IXACT3EngineImpl_Initialize(IXACT3Engine *iface,
        const XACT_RUNTIME_PARAMETERS *pParams)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);
    FACTRuntimeParameters params;
    UINT ret;

    TRACE("(%p)->(%p)\n", This, pParams);

    memset(&params, 0, sizeof(FACTRuntimeParameters));
    /* Explicitly copy to the FAudio structure as the packing is wrong under 64 bits */
    params.lookAheadTime = pParams->lookAheadTime;
    params.pGlobalSettingsBuffer = pParams->pGlobalSettingsBuffer;
    params.globalSettingsBufferSize = pParams->globalSettingsBufferSize;
    params.globalSettingsFlags = pParams->globalSettingsFlags;
    params.globalSettingsAllocAttributes = pParams->globalSettingsAllocAttributes;
    params.pRendererID = (int16_t*)pParams->pRendererID;
    params.pXAudio2 = NULL;
    params.pMasteringVoice = NULL;

#if XACT3_VER >= 0x0300
    /* FIXME: pXAudio2 and pMasteringVoice are pointers to
     * IXAudio2/IXAudio2MasteringVoice objects. FACT wants pointers to
     * FAudio/FAudioMasteringVoice objects. In Wine's XAudio2 implementation, we
     * actually have them available, but only if you access their internal data.
     * For now we can just force these pointers to NULL, as XACT simply
     * generates its own engine and endpoint in that situation. These parameters
     * are mostly an optimization for games with multiple XACT3Engines that want
     * a single engine running everything.
     * -flibit
     */
    if (pParams->pXAudio2 != NULL){
        FIXME("pXAudio2 parameter not supported!\n");

        if (pParams->pMasteringVoice != NULL){
            FIXME("pMasteringVoice parameter not supported!\n");
        }
    }
#endif

    /* Force Windows I/O, do NOT use the FACT default! */
    This->pReadFile = (XACT_READFILE_CALLBACK)
            pParams->fileIOCallbacks.readFileCallback;
    This->pGetOverlappedResult = (XACT_GETOVERLAPPEDRESULT_CALLBACK)
            pParams->fileIOCallbacks.getOverlappedResultCallback;
    if (This->pReadFile == NULL)
        This->pReadFile = (XACT_READFILE_CALLBACK) ReadFile;
    if (This->pGetOverlappedResult == NULL)
        This->pGetOverlappedResult = (XACT_GETOVERLAPPEDRESULT_CALLBACK)
                GetOverlappedResult;
    params.fileIOCallbacks.readFileCallback = wrap_readfile;
    params.fileIOCallbacks.getOverlappedResultCallback = wrap_getoverlappedresult;
    params.fnNotificationCallback = fact_notification_cb;

    This->notification_callback = (XACT_NOTIFICATION_CALLBACK)pParams->fnNotificationCallback;

    ret = FACTAudioEngine_Initialize(This->fact_engine, &params);
    if (ret != 0)
        WARN("FACTAudioEngine_Initialize returned %d\n", ret);

    return !ret ? S_OK : E_FAIL;
}

static HRESULT WINAPI IXACT3EngineImpl_ShutDown(IXACT3Engine *iface)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);

    TRACE("(%p)\n", This);

    return FACTAudioEngine_ShutDown(This->fact_engine);
}

static HRESULT WINAPI IXACT3EngineImpl_DoWork(IXACT3Engine *iface)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);

    TRACE("(%p)\n", This);

    return FACTAudioEngine_DoWork(This->fact_engine);
}

static HRESULT WINAPI IXACT3EngineImpl_CreateSoundBank(IXACT3Engine *iface,
        const void* pvBuffer, DWORD dwSize, DWORD dwFlags,
        DWORD dwAllocAttributes, IXACT3SoundBank **ppSoundBank)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);
    XACT3SoundBankImpl *sb;
    FACTSoundBank *fsb;
    UINT ret;
    HRESULT hr;

    TRACE("(%p)->(%p, %lu, 0x%lx, 0x%lx, %p)\n", This, pvBuffer, dwSize, dwFlags,
            dwAllocAttributes, ppSoundBank);

    ret = FACTAudioEngine_CreateSoundBank(This->fact_engine, pvBuffer, dwSize,
            dwFlags, dwAllocAttributes, &fsb);
    if(ret != 0)
    {
        ERR("Failed to CreateSoundBank: %d\n", ret);
        return E_FAIL;
    }

    sb = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*sb));
    if (!sb)
    {
        FACTSoundBank_Destroy(fsb);
        ERR("Failed to allocate XACT3SoundBankImpl!\n");
        return E_OUTOFMEMORY;
    }

    hr = wrapper_add_entry(This, fsb, &sb->IXACT3SoundBank_iface);
    if (FAILED(hr))
    {
        FACTSoundBank_Destroy(fsb);
        HeapFree(GetProcessHeap(), 0, sb);
        return hr;
    }

    sb->IXACT3SoundBank_iface.lpVtbl = &XACT3SoundBank_Vtbl;
    sb->fact_soundbank = fsb;
    sb->engine = This;
    *ppSoundBank = &sb->IXACT3SoundBank_iface;

    TRACE("Created SoundBank: %p\n", sb);

    return S_OK;
}

static HRESULT WINAPI IXACT3EngineImpl_CreateInMemoryWaveBank(IXACT3Engine *iface,
        const void* pvBuffer, DWORD dwSize, DWORD dwFlags,
        DWORD dwAllocAttributes, IXACT3WaveBank **ppWaveBank)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);
    XACT3WaveBankImpl *wb;
    FACTWaveBank *fwb;
    HRESULT hr;
    UINT ret;

    TRACE("(%p)->(%p, %lu, 0x%lx, 0x%lx, %p)\n", This, pvBuffer, dwSize, dwFlags,
            dwAllocAttributes, ppWaveBank);

    ret = FACTAudioEngine_CreateInMemoryWaveBank(This->fact_engine, pvBuffer,
            dwSize, dwFlags, dwAllocAttributes, &fwb);
    if(ret != 0)
    {
        ERR("Failed to CreateWaveBank: %d\n", ret);
        return E_FAIL;
    }

    wb = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wb));
    if (!wb)
    {
        FACTWaveBank_Destroy(fwb);
        ERR("Failed to allocate XACT3WaveBankImpl!\n");
        return E_OUTOFMEMORY;
    }

    hr = wrapper_add_entry(This, fwb, &wb->IXACT3WaveBank_iface);
    if (FAILED(hr))
    {
        FACTWaveBank_Destroy(fwb);
        HeapFree(GetProcessHeap(), 0, wb);
        return hr;
    }

    wb->IXACT3WaveBank_iface.lpVtbl = &XACT3WaveBank_Vtbl;
    wb->fact_wavebank = fwb;
    wb->engine = This;
    *ppWaveBank = &wb->IXACT3WaveBank_iface;

    TRACE("Created in-memory WaveBank: %p\n", wb);

    return S_OK;
}

static HRESULT WINAPI IXACT3EngineImpl_CreateStreamingWaveBank(IXACT3Engine *iface,
        const XACT_WAVEBANK_STREAMING_PARAMETERS *pParms,
        IXACT3WaveBank **ppWaveBank)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);
    FACTStreamingParameters fakeParms;
    wrap_readfile_struct *fake;
    XACT3WaveBankImpl *wb;
    FACTWaveBank *fwb;
    UINT ret;
    HRESULT hr;

    TRACE("(%p)->(%p, %p)\n", This, pParms, ppWaveBank);

    /* We have to wrap the file to fix up the callbacks! */
    fake = (wrap_readfile_struct*) CoTaskMemAlloc(
            sizeof(wrap_readfile_struct));
    fake->engine = This;
    fake->file = pParms->file;
    fakeParms.file = fake;
    fakeParms.flags = pParms->flags;
    fakeParms.offset = pParms->offset;
    fakeParms.packetSize = pParms->packetSize;

    ret = FACTAudioEngine_CreateStreamingWaveBank(This->fact_engine, &fakeParms,
            &fwb);
    if(ret != 0)
    {
        ERR("Failed to CreateWaveBank: %d\n", ret);
        return E_FAIL;
    }

    wb = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wb));
    if (!wb)
    {
        FACTWaveBank_Destroy(fwb);
        ERR("Failed to allocate XACT3WaveBankImpl!\n");
        return E_OUTOFMEMORY;
    }

    hr = wrapper_add_entry(This, fwb, &wb->IXACT3WaveBank_iface);
    if (FAILED(hr))
    {
        FACTWaveBank_Destroy(fwb);
        HeapFree(GetProcessHeap(), 0, wb);
        return hr;
    }

    wb->IXACT3WaveBank_iface.lpVtbl = &XACT3WaveBank_Vtbl;
    wb->fact_wavebank = fwb;
    wb->engine = This;
    *ppWaveBank = &wb->IXACT3WaveBank_iface;

    TRACE("Created streaming WaveBank: %p\n", wb);

    return S_OK;
}

#if XACT3_VER >= 0x0205

static HRESULT WINAPI IXACT3EngineImpl_PrepareInMemoryWave(IXACT3Engine *iface,
        DWORD dwFlags, WAVEBANKENTRY entry, DWORD *pdwSeekTable,
        BYTE *pbWaveData, DWORD dwPlayOffset, XACTLOOPCOUNT nLoopCount,
        IXACT3Wave **ppWave)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);
    FIXME("(%p): stub!\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IXACT3EngineImpl_PrepareStreamingWave(IXACT3Engine *iface,
        DWORD dwFlags, WAVEBANKENTRY entry,
        XACT_STREAMING_PARAMETERS streamingParams, DWORD dwAlignment,
        DWORD *pdwSeekTable, DWORD dwPlayOffset, XACTLOOPCOUNT nLoopCount,
        IXACT3Wave **ppWave)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);
    FIXME("(%p): stub!\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IXACT3EngineImpl_PrepareWave(IXACT3Engine *iface,
        DWORD dwFlags, PCSTR szWavePath, WORD wStreamingPacketSize,
        DWORD dwAlignment, DWORD dwPlayOffset, XACTLOOPCOUNT nLoopCount,
        IXACT3Wave **ppWave)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);
    XACT3WaveImpl *wave;
    FACTWave *fwave = NULL;
    UINT ret;
    HRESULT hr;

    TRACE("(%p)->(0x%08lx, %s, %d, %ld, %ld, %d, %p)\n", This, dwFlags, debugstr_a(szWavePath),
          wStreamingPacketSize, dwAlignment, dwPlayOffset, nLoopCount, ppWave);

    ret = FACTAudioEngine_PrepareWave(This->fact_engine, dwFlags, szWavePath, wStreamingPacketSize,
            dwAlignment, dwPlayOffset, nLoopCount, &fwave);
    if(ret != 0 || !fwave)
    {
        ERR("Failed to CreateWave: %d (%p)\n", ret, fwave);
        return E_FAIL;
    }

    wave = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wave));
    if (!wave)
    {
        FACTWave_Destroy(fwave);
        return E_OUTOFMEMORY;
    }

    hr = wrapper_add_entry(This, fwave, &wave->IXACT3Wave_iface);
    if (FAILED(hr))
    {
        FACTWave_Destroy(fwave);
        HeapFree(GetProcessHeap(), 0, wave);
        return hr;
    }

    wave->IXACT3Wave_iface.lpVtbl = &XACT3Wave_Vtbl;
    wave->fact_wave = fwave;
    wave->engine = This;
    *ppWave = &wave->IXACT3Wave_iface;

    TRACE("Created Wave: %p\n", wave);

    return S_OK;
}

#endif

enum {
    NOTIFY_SoundBank = 0x01,
    NOTIFY_WaveBank  = 0x02,
    NOTIFY_Cue       = 0x04,
    NOTIFY_Wave      = 0x08,
    NOTIFY_cueIndex  = 0x10,
    NOTIFY_waveIndex = 0x20
};

/* these constants don't have the same values across xactengine versions */
static uint8_t fact_notification_type_from_xact(XACTNOTIFICATIONTYPE type)
{
    /* we can't use a switch statement, because the constants are static const
     * variables, and some compilers can't deal with that */
#define X(a) if (type == XACTNOTIFICATIONTYPE_##a) return FACTNOTIFICATIONTYPE_##a
    X(CUEPREPARED);
    X(CUEPLAY);
    X(CUESTOP);
    X(CUEDESTROYED);
    X(MARKER);
    X(SOUNDBANKDESTROYED);
    X(WAVEBANKDESTROYED);
    X(LOCALVARIABLECHANGED);
    X(GLOBALVARIABLECHANGED);
    X(GUICONNECTED);
    X(GUIDISCONNECTED);
    X(WAVEPLAY);
    X(WAVESTOP);
    X(WAVEBANKPREPARED);
    X(WAVEBANKSTREAMING_INVALIDCONTENT);
#if XACT3_VER >= 0x0205
    X(WAVEPREPARED);
    X(WAVELOOPED);
    X(WAVEDESTROYED);
#endif
#undef X

    FIXME("unknown type %#x\n", type);
    return 0;
}

static inline void unwrap_notificationdesc(FACTNotificationDescription *fd,
        const XACT_NOTIFICATION_DESCRIPTION *xd)
{
    DWORD flags = 0;

    TRACE("Type %d\n", xd->type);

    memset(fd, 0, sizeof(*fd));

    fd->type = fact_notification_type_from_xact(xd->type);

    /* we can't use a switch statement, because the constants are static const
     * variables, and some compilers can't deal with that */

    /* Supports SoundBank, Cue index, Cue instance */
    if (fd->type == FACTNOTIFICATIONTYPE_CUEPREPARED || fd->type == FACTNOTIFICATIONTYPE_CUEPLAY ||
        fd->type == FACTNOTIFICATIONTYPE_CUESTOP || fd->type == FACTNOTIFICATIONTYPE_CUEDESTROYED ||
        fd->type == FACTNOTIFICATIONTYPE_MARKER || fd->type == FACTNOTIFICATIONTYPE_LOCALVARIABLECHANGED)
    {
        flags = NOTIFY_SoundBank | NOTIFY_cueIndex | NOTIFY_Cue;
    }
    /* Supports WaveBank */
    else if (fd->type == FACTNOTIFICATIONTYPE_WAVEBANKDESTROYED || fd->type == FACTNOTIFICATIONTYPE_WAVEBANKPREPARED ||
             fd->type == FACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT)
    {
        flags = NOTIFY_WaveBank;
    }
    /* Supports NOTIFY_SoundBank */
    else if (fd->type == FACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED)
    {
        flags = NOTIFY_SoundBank;
    }
    /* Supports SoundBank, SoundBank, Cue index, Cue instance, WaveBank, Wave instance */
    else if (fd->type == FACTNOTIFICATIONTYPE_WAVEPLAY || fd->type == FACTNOTIFICATIONTYPE_WAVESTOP ||
             fd->type == FACTNOTIFICATIONTYPE_WAVELOOPED)
    {
        flags = NOTIFY_SoundBank | NOTIFY_cueIndex | NOTIFY_Cue | NOTIFY_WaveBank | NOTIFY_Wave;
    }
    /* Supports WaveBank, Wave index, Wave instance */
    else if (fd->type == FACTNOTIFICATIONTYPE_WAVEPREPARED || fd->type == FACTNOTIFICATIONTYPE_WAVEDESTROYED)
    {
        flags = NOTIFY_WaveBank | NOTIFY_waveIndex | NOTIFY_Wave;
    }

    /* We have to unwrap the FACT object first! */
    fd->flags = xd->flags;
    if (flags & NOTIFY_cueIndex)
        fd->cueIndex = xd->cueIndex;
#if XACT3_VER >= 0x0205
    if (flags & NOTIFY_waveIndex)
        fd->waveIndex = xd->waveIndex;
#endif

    if (flags & NOTIFY_Cue && xd->pCue != NULL)
    {
        XACT3CueImpl *cue = impl_from_IXACT3Cue(xd->pCue);
        if (cue)
            fd->pCue = cue->fact_cue;
    }

    if (flags & NOTIFY_SoundBank && xd->pSoundBank != NULL)
    {
        XACT3SoundBankImpl *sound = impl_from_IXACT3SoundBank(xd->pSoundBank);
        if (sound)
            fd->pSoundBank = sound->fact_soundbank;
    }

    if (flags & NOTIFY_WaveBank && xd->pWaveBank != NULL)
    {
        XACT3WaveBankImpl *bank = impl_from_IXACT3WaveBank(xd->pWaveBank);
        if (bank)
            fd->pWaveBank = bank->fact_wavebank;
    }

#if XACT3_VER >= 0x0205
    if (flags & NOTIFY_Wave && xd->pWave != NULL)
    {
        XACT3WaveImpl *wave = impl_from_IXACT3Wave(xd->pWave);
        if (wave)
            fd->pWave = wave->fact_wave;
    }
#endif
}

static HRESULT WINAPI IXACT3EngineImpl_RegisterNotification(IXACT3Engine *iface,
        const XACT_NOTIFICATION_DESCRIPTION *pNotificationDesc)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);
    FACTNotificationDescription fdesc;

    TRACE("(%p)->(%p)\n", This, pNotificationDesc);

    if (pNotificationDesc->type < XACTNOTIFICATIONTYPE_CUEPREPARED ||
        pNotificationDesc->type > XACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT)
        return E_INVALIDARG;

    unwrap_notificationdesc(&fdesc, pNotificationDesc);
    This->contexts[pNotificationDesc->type] = pNotificationDesc->pvContext;
    fdesc.pvContext = This;
    return FACTAudioEngine_RegisterNotification(This->fact_engine, &fdesc);
}

static HRESULT WINAPI IXACT3EngineImpl_UnRegisterNotification(IXACT3Engine *iface,
        const XACT_NOTIFICATION_DESCRIPTION *pNotificationDesc)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);
    FACTNotificationDescription fdesc;

    TRACE("(%p)->(%p)\n", This, pNotificationDesc);

    if (pNotificationDesc->type < XACTNOTIFICATIONTYPE_CUEPREPARED ||
        pNotificationDesc->type > XACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT)
        return S_OK;

    unwrap_notificationdesc(&fdesc, pNotificationDesc);
    fdesc.pvContext = This;
    return FACTAudioEngine_UnRegisterNotification(This->fact_engine, &fdesc);
}

static XACTCATEGORY WINAPI IXACT3EngineImpl_GetCategory(IXACT3Engine *iface,
        PCSTR szFriendlyName)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);

    TRACE("(%p)->(%s)\n", This, szFriendlyName);

    return FACTAudioEngine_GetCategory(This->fact_engine, szFriendlyName);
}

static HRESULT WINAPI IXACT3EngineImpl_Stop(IXACT3Engine *iface,
        XACTCATEGORY nCategory, DWORD dwFlags)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);

    TRACE("(%p)->(%u, 0x%lx)\n", This, nCategory, dwFlags);

    return FACTAudioEngine_Stop(This->fact_engine, nCategory, dwFlags);
}

static HRESULT WINAPI IXACT3EngineImpl_SetVolume(IXACT3Engine *iface,
        XACTCATEGORY nCategory, XACTVOLUME nVolume)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);

    TRACE("(%p)->(%u, %f)\n", This, nCategory, nVolume);

    return FACTAudioEngine_SetVolume(This->fact_engine, nCategory, nVolume);
}

static HRESULT WINAPI IXACT3EngineImpl_Pause(IXACT3Engine *iface,
        XACTCATEGORY nCategory, BOOL fPause)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);

    TRACE("(%p)->(%u, %u)\n", This, nCategory, fPause);

    return FACTAudioEngine_Pause(This->fact_engine, nCategory, fPause);
}

static XACTVARIABLEINDEX WINAPI IXACT3EngineImpl_GetGlobalVariableIndex(
        IXACT3Engine *iface, PCSTR szFriendlyName)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);

    TRACE("(%p)->(%s)\n", This, szFriendlyName);

    return FACTAudioEngine_GetGlobalVariableIndex(This->fact_engine,
            szFriendlyName);
}

static HRESULT WINAPI IXACT3EngineImpl_SetGlobalVariable(IXACT3Engine *iface,
        XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE nValue)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);

    TRACE("(%p)->(%u, %f)\n", This, nIndex, nValue);

    return FACTAudioEngine_SetGlobalVariable(This->fact_engine, nIndex, nValue);
}

static HRESULT WINAPI IXACT3EngineImpl_GetGlobalVariable(IXACT3Engine *iface,
        XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE *nValue)
{
    XACT3EngineImpl *This = impl_from_IXACT3Engine(iface);

    TRACE("(%p)->(%u, %p)\n", This, nIndex, nValue);

    return FACTAudioEngine_GetGlobalVariable(This->fact_engine, nIndex, nValue);
}

static const IXACT3EngineVtbl XACT3Engine_Vtbl =
{
    IXACT3EngineImpl_QueryInterface,
    IXACT3EngineImpl_AddRef,
    IXACT3EngineImpl_Release,
    IXACT3EngineImpl_GetRendererCount,
    IXACT3EngineImpl_GetRendererDetails,
#if XACT3_VER >= 0x0205
    IXACT3EngineImpl_GetFinalMixFormat,
#endif
    IXACT3EngineImpl_Initialize,
    IXACT3EngineImpl_ShutDown,
    IXACT3EngineImpl_DoWork,
    IXACT3EngineImpl_CreateSoundBank,
    IXACT3EngineImpl_CreateInMemoryWaveBank,
    IXACT3EngineImpl_CreateStreamingWaveBank,
#if XACT3_VER >= 0x0205
    IXACT3EngineImpl_PrepareWave,
    IXACT3EngineImpl_PrepareInMemoryWave,
    IXACT3EngineImpl_PrepareStreamingWave,
#endif
    IXACT3EngineImpl_RegisterNotification,
    IXACT3EngineImpl_UnRegisterNotification,
    IXACT3EngineImpl_GetCategory,
    IXACT3EngineImpl_Stop,
    IXACT3EngineImpl_SetVolume,
    IXACT3EngineImpl_Pause,
    IXACT3EngineImpl_GetGlobalVariableIndex,
    IXACT3EngineImpl_SetGlobalVariable,
    IXACT3EngineImpl_GetGlobalVariable
};

void* XACT_Internal_Malloc(size_t size)
{
    return CoTaskMemAlloc(size);
}

void XACT_Internal_Free(void* ptr)
{
    return CoTaskMemFree(ptr);
}

void* XACT_Internal_Realloc(void* ptr, size_t size)
{
    return CoTaskMemRealloc(ptr, size);
}

static HRESULT WINAPI XACT3CF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppobj)
{
    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppobj = iface;
        return S_OK;
    }

    *ppobj = NULL;
    WARN("(%p)->(%s, %p): interface not found\n", iface, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI XACT3CF_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI XACT3CF_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI XACT3CF_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                               REFIID riid, void **ppobj)
{
    HRESULT hr;
    XACT3EngineImpl *object;

    TRACE("(%p)->(%p,%s,%p)\n", iface, pOuter, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if(pOuter)
        return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if(!object)
        return E_OUTOFMEMORY;

    object->IXACT3Engine_iface.lpVtbl = &XACT3Engine_Vtbl;

    FACTCreateEngineWithCustomAllocatorEXT(
        0,
        &object->fact_engine,
        XACT_Internal_Malloc,
        XACT_Internal_Free,
        XACT_Internal_Realloc
    );

    hr = IXACT3Engine_QueryInterface(&object->IXACT3Engine_iface, riid, ppobj);
    if(FAILED(hr)){
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    wine_rb_init(&object->wrapper_lookup, wrapper_lookup_compare);
    InitializeCriticalSection(&object->wrapper_lookup_cs);

    return hr;
}

static HRESULT WINAPI XACT3CF_LockServer(IClassFactory *iface, BOOL dolock)
{
    TRACE("(%p)->(%d): stub!\n", iface, dolock);
    return S_OK;
}

static const IClassFactoryVtbl XACT3CF_Vtbl =
{
    XACT3CF_QueryInterface,
    XACT3CF_AddRef,
    XACT3CF_Release,
    XACT3CF_CreateInstance,
    XACT3CF_LockServer
};

static IClassFactory XACTFactory = { &XACT3CF_Vtbl };

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, void *pReserved)
{
    TRACE("(%p, %ld, %p)\n", hinstDLL, reason, pReserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinstDLL );
        TRACE("Using FAudio version %d\n", FAudioLinkedVersion() );
        break;
    }
    return TRUE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualGUID(rclsid, &CLSID_XACTEngine))
    {
        TRACE("(%s, %s, %p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&XACTFactory, riid, ppv);
    }

    FIXME("Unknown class %s\n", debugstr_guid(rclsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
