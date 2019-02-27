/*
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
 *
 ***************
 *
 * Some versions of XAudio2 add or remove functions to the COM vtables, or
 * incompatibly change structures. This file provides implementations of the
 * older XAudio2 versions onto the new XAudio2 APIs.
 *
 * Below is a list of significant changes to the main XAudio2 interfaces and
 * API. There may be further changes to effects and other parts that Wine
 * doesn't currently implement.
 *
 * 2.0
 *   Initial version
 *
 * 2.1
 *   Change CLSID_XAudio2
 *   Re-order Error codes
 *   Change XAUDIO2_LOOP_INFINITE
 *   Change struct XAUDIO2_PERFORMANCE_DATA
 *   Change IXAudio2Voice::GetOutputMatrix return value to void
 *   Add parameter to IXAudio2VoiceCallback::OnVoiceProcessingPassStart
 *   Change struct XAPO_REGISTRATION_PROPERTIES. CAREFUL when using! Not all
 *       implementations of IXAPO are Wine implementations.
 *
 * 2.2
 *   Change CLSID_XAudio2
 *   No ABI break
 *
 * 2.3
 *   Change CLSID_XAudio2
 *   ABI break:
 *     Change struct XAUDIO2_PERFORMANCE_DATA
 *
 * 2.4
 *   Change CLSID_XAudio2
 *   ABI break:
 *     Add IXAudio2Voice::SetOutputFilterParameters
 *     Add IXAudio2Voice::GetOutputFilterParameters
 *     Add IXAudio2SourceVoice::SetSourceSampleRate
 *     Change struct XAUDIO2_VOICE_SENDS
 *
 * 2.5
 *   Change CLSID_XAudio2
 *   No ABI break
 *
 * 2.6
 *   Change CLSID_XAudio2
 *   No ABI break
 *
 * 2.7
 *   Change CLSID_XAudio2
 *   No ABI break
 *
 * 2.8
 *   Remove CLSID_XAudio2
 *   Change IID_IXAudio2
 *   Add xaudio2_8.XAudio2Create
 *   ABI break:
 *     Remove IXAudio2::GetDeviceCount
 *     Remove IXAudio2::GetDeviceDetails
 *     Remove IXAudio2::Initialize
 *     Change parameter of IXAudio2::CreateMasteringVoice
 *     Add Flags parameter to IXAudio2SourceVoice::GetState
 *     Add IXAudio2MasteringVoice::GetChannelMask
 *     Add DisableLateField member to XAUDIO2FX_REVERB_PARAMETERS
 *     Add ActiveFlags member to XAUDIO2_VOICE_DETAILS
 *
 * 2.9
 *   Change IID_IXAudio2
 *   New flags: XAUDIO2_STOP_ENGINE_WHEN_IDLE, XAUDIO2_1024_QUANTUM,
 *       XAUDIO2_NO_VIRTUAL_AUDIO_CLIENT
 *   ABI break:
 *     Add SideDelay member to XAUDIO2FX_REVERB_PARAMETERS
 */

#include "config.h"

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS

#include <stdarg.h>

#include "xaudio_private.h"

#include "wine/debug.h"

#if XAUDIO2_VER <= 7
WINE_DEFAULT_DEBUG_CHANNEL(xaudio2);
#endif

#if XAUDIO2_VER <= 3
static XAUDIO2_SEND_DESCRIPTOR *convert_send_descriptors23(const XAUDIO23_VOICE_SENDS *sends)
{
    XAUDIO2_SEND_DESCRIPTOR *ret;
    DWORD i;

    ret = HeapAlloc(GetProcessHeap(), 0, sends->OutputCount * sizeof(XAUDIO2_SEND_DESCRIPTOR));

    for(i = 0; i < sends->OutputCount; ++i){
        ret[i].Flags = 0;
        ret[i].pOutputVoice = sends->pOutputVoices[i];
    }

    return ret;
}
#endif

/* BEGIN IXAudio2SourceVoice */
#if XAUDIO2_VER == 0
XA2VoiceImpl *impl_from_IXAudio20SourceVoice(IXAudio20SourceVoice *iface)
{
    return CONTAINING_RECORD(iface, XA2VoiceImpl, IXAudio20SourceVoice_iface);
}

static void WINAPI XA20SRC_GetVoiceDetails(IXAudio20SourceVoice *iface,
        XAUDIO27_VOICE_DETAILS *pVoiceDetails)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    XAUDIO2_VOICE_DETAILS details;

    IXAudio2SourceVoice_GetVoiceDetails(&This->IXAudio2SourceVoice_iface, &details);

    pVoiceDetails->CreationFlags = details.CreationFlags;
    pVoiceDetails->InputChannels = details.InputChannels;
    pVoiceDetails->InputSampleRate = details.InputSampleRate;
}

static HRESULT WINAPI XA20SRC_SetOutputVoices(IXAudio20SourceVoice *iface,
        const XAUDIO23_VOICE_SENDS *pSendList)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    XAUDIO2_VOICE_SENDS sends, *psends = NULL;
    HRESULT hr;

    TRACE("%p, %p\n", This, pSendList);

    if(pSendList){
        sends.SendCount = pSendList->OutputCount;
        sends.pSends = convert_send_descriptors23(pSendList);
        psends = &sends;
    }

    hr = IXAudio2SourceVoice_SetOutputVoices(&This->IXAudio2SourceVoice_iface, psends);

    if(pSendList)
        HeapFree(GetProcessHeap(), 0, sends.pSends);

    return hr;
}

static HRESULT WINAPI XA20SRC_SetEffectChain(IXAudio20SourceVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_SetEffectChain(&This->IXAudio2SourceVoice_iface, pEffectChain);
}

static HRESULT WINAPI XA20SRC_EnableEffect(IXAudio20SourceVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_EnableEffect(&This->IXAudio2SourceVoice_iface,
            EffectIndex, OperationSet);
}

static HRESULT WINAPI XA20SRC_DisableEffect(IXAudio20SourceVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_DisableEffect(&This->IXAudio2SourceVoice_iface,
            EffectIndex, OperationSet);
}

static void WINAPI XA20SRC_GetEffectState(IXAudio20SourceVoice *iface,
        UINT32 EffectIndex, BOOL *pEnabled)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_GetEffectState(&This->IXAudio2SourceVoice_iface,
            EffectIndex, pEnabled);
}

static HRESULT WINAPI XA20SRC_SetEffectParameters(IXAudio20SourceVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_SetEffectParameters(&This->IXAudio2SourceVoice_iface,
            EffectIndex, pParameters, ParametersByteSize, OperationSet);
}

static HRESULT WINAPI XA20SRC_GetEffectParameters(IXAudio20SourceVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_GetEffectParameters(&This->IXAudio2SourceVoice_iface,
            EffectIndex, pParameters, ParametersByteSize);
}

static HRESULT WINAPI XA20SRC_SetFilterParameters(IXAudio20SourceVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_SetFilterParameters(&This->IXAudio2SourceVoice_iface,
            pParameters, OperationSet);
}

static void WINAPI XA20SRC_GetFilterParameters(IXAudio20SourceVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_GetFilterParameters(&This->IXAudio2SourceVoice_iface, pParameters);
}

static HRESULT WINAPI XA20SRC_SetVolume(IXAudio20SourceVoice *iface,
        float Volume, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_SetVolume(&This->IXAudio2SourceVoice_iface,
            Volume, OperationSet);
}

static void WINAPI XA20SRC_GetVolume(IXAudio20SourceVoice *iface,
        float *pVolume)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_GetVolume(&This->IXAudio2SourceVoice_iface, pVolume);
}

static HRESULT WINAPI XA20SRC_SetChannelVolumes(IXAudio20SourceVoice *iface,
        UINT32 Channels, const float *pVolumes, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_SetChannelVolumes(&This->IXAudio2SourceVoice_iface,
            Channels, pVolumes, OperationSet);
}

static void WINAPI XA20SRC_GetChannelVolumes(IXAudio20SourceVoice *iface,
        UINT32 Channels, float *pVolumes)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_GetChannelVolumes(&This->IXAudio2SourceVoice_iface,
            Channels, pVolumes);
}

static HRESULT WINAPI XA20SRC_SetOutputMatrix(IXAudio20SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_SetOutputMatrix(&This->IXAudio2SourceVoice_iface,
            pDestinationVoice, SourceChannels, DestinationChannels,
            pLevelMatrix, OperationSet);
}

static HRESULT WINAPI XA20SRC_GetOutputMatrix(IXAudio20SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    IXAudio2SourceVoice_GetOutputMatrix(&This->IXAudio2SourceVoice_iface,
            pDestinationVoice, SourceChannels, DestinationChannels,
            pLevelMatrix);
    return S_OK;
}

static void WINAPI XA20SRC_DestroyVoice(IXAudio20SourceVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_DestroyVoice(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA20SRC_Start(IXAudio20SourceVoice *iface, UINT32 Flags,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_Start(&This->IXAudio2SourceVoice_iface, Flags, OperationSet);
}

static HRESULT WINAPI XA20SRC_Stop(IXAudio20SourceVoice *iface, UINT32 Flags,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_Stop(&This->IXAudio2SourceVoice_iface, Flags, OperationSet);
}

static HRESULT WINAPI XA20SRC_SubmitSourceBuffer(IXAudio20SourceVoice *iface,
        const XAUDIO2_BUFFER *pBuffer, const XAUDIO2_BUFFER_WMA *pBufferWMA)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_SubmitSourceBuffer(&This->IXAudio2SourceVoice_iface,
            pBuffer, pBufferWMA);
}

static HRESULT WINAPI XA20SRC_FlushSourceBuffers(IXAudio20SourceVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_FlushSourceBuffers(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA20SRC_Discontinuity(IXAudio20SourceVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_Discontinuity(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA20SRC_ExitLoop(IXAudio20SourceVoice *iface,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_ExitLoop(&This->IXAudio2SourceVoice_iface, OperationSet);
}

static void WINAPI XA20SRC_GetState(IXAudio20SourceVoice *iface,
        XAUDIO2_VOICE_STATE *pVoiceState)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_GetState(&This->IXAudio2SourceVoice_iface, pVoiceState, 0);
}

static HRESULT WINAPI XA20SRC_SetFrequencyRatio(IXAudio20SourceVoice *iface,
        float Ratio, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_SetFrequencyRatio(&This->IXAudio2SourceVoice_iface,
            Ratio, OperationSet);
}

static void WINAPI XA20SRC_GetFrequencyRatio(IXAudio20SourceVoice *iface,
        float *pRatio)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SourceVoice(iface);
    return IXAudio2SourceVoice_GetFrequencyRatio(&This->IXAudio2SourceVoice_iface, pRatio);
}

const IXAudio20SourceVoiceVtbl XAudio20SourceVoice_Vtbl = {
    XA20SRC_GetVoiceDetails,
    XA20SRC_SetOutputVoices,
    XA20SRC_SetEffectChain,
    XA20SRC_EnableEffect,
    XA20SRC_DisableEffect,
    XA20SRC_GetEffectState,
    XA20SRC_SetEffectParameters,
    XA20SRC_GetEffectParameters,
    XA20SRC_SetFilterParameters,
    XA20SRC_GetFilterParameters,
    XA20SRC_SetVolume,
    XA20SRC_GetVolume,
    XA20SRC_SetChannelVolumes,
    XA20SRC_GetChannelVolumes,
    XA20SRC_SetOutputMatrix,
    XA20SRC_GetOutputMatrix,
    XA20SRC_DestroyVoice,
    XA20SRC_Start,
    XA20SRC_Stop,
    XA20SRC_SubmitSourceBuffer,
    XA20SRC_FlushSourceBuffers,
    XA20SRC_Discontinuity,
    XA20SRC_ExitLoop,
    XA20SRC_GetState,
    XA20SRC_SetFrequencyRatio,
    XA20SRC_GetFrequencyRatio,
};

#elif XAUDIO2_VER <= 3

XA2VoiceImpl *impl_from_IXAudio23SourceVoice(IXAudio23SourceVoice *iface)
{
    return CONTAINING_RECORD(iface, XA2VoiceImpl, IXAudio23SourceVoice_iface);
}

static void WINAPI XA23SRC_GetVoiceDetails(IXAudio23SourceVoice *iface,
        XAUDIO27_VOICE_DETAILS *pVoiceDetails)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    XAUDIO2_VOICE_DETAILS details;

    IXAudio2SourceVoice_GetVoiceDetails(&This->IXAudio2SourceVoice_iface, &details);

    pVoiceDetails->CreationFlags = details.CreationFlags;
    pVoiceDetails->InputChannels = details.InputChannels;
    pVoiceDetails->InputSampleRate = details.InputSampleRate;
}

static HRESULT WINAPI XA23SRC_SetOutputVoices(IXAudio23SourceVoice *iface,
        const XAUDIO23_VOICE_SENDS *pSendList)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    XAUDIO2_VOICE_SENDS sends, *psends = NULL;
    HRESULT hr;

    TRACE("%p, %p\n", This, pSendList);

    if(pSendList){
        sends.SendCount = pSendList->OutputCount;
        sends.pSends = convert_send_descriptors23(pSendList);
        psends = &sends;
    }

    hr = IXAudio2SourceVoice_SetOutputVoices(&This->IXAudio2SourceVoice_iface, psends);

    if(pSendList)
        HeapFree(GetProcessHeap(), 0, sends.pSends);

    return hr;
}

static HRESULT WINAPI XA23SRC_SetEffectChain(IXAudio23SourceVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_SetEffectChain(&This->IXAudio2SourceVoice_iface, pEffectChain);
}

static HRESULT WINAPI XA23SRC_EnableEffect(IXAudio23SourceVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_EnableEffect(&This->IXAudio2SourceVoice_iface,
            EffectIndex, OperationSet);
}

static HRESULT WINAPI XA23SRC_DisableEffect(IXAudio23SourceVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_DisableEffect(&This->IXAudio2SourceVoice_iface,
            EffectIndex, OperationSet);
}

static void WINAPI XA23SRC_GetEffectState(IXAudio23SourceVoice *iface,
        UINT32 EffectIndex, BOOL *pEnabled)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_GetEffectState(&This->IXAudio2SourceVoice_iface,
            EffectIndex, pEnabled);
}

static HRESULT WINAPI XA23SRC_SetEffectParameters(IXAudio23SourceVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_SetEffectParameters(&This->IXAudio2SourceVoice_iface,
            EffectIndex, pParameters, ParametersByteSize, OperationSet);
}

static HRESULT WINAPI XA23SRC_GetEffectParameters(IXAudio23SourceVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_GetEffectParameters(&This->IXAudio2SourceVoice_iface,
            EffectIndex, pParameters, ParametersByteSize);
}

static HRESULT WINAPI XA23SRC_SetFilterParameters(IXAudio23SourceVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_SetFilterParameters(&This->IXAudio2SourceVoice_iface,
            pParameters, OperationSet);
}

static void WINAPI XA23SRC_GetFilterParameters(IXAudio23SourceVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_GetFilterParameters(&This->IXAudio2SourceVoice_iface, pParameters);
}

static HRESULT WINAPI XA23SRC_SetVolume(IXAudio23SourceVoice *iface,
        float Volume, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_SetVolume(&This->IXAudio2SourceVoice_iface,
            Volume, OperationSet);
}

static void WINAPI XA23SRC_GetVolume(IXAudio23SourceVoice *iface,
        float *pVolume)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_GetVolume(&This->IXAudio2SourceVoice_iface, pVolume);
}

static HRESULT WINAPI XA23SRC_SetChannelVolumes(IXAudio23SourceVoice *iface,
        UINT32 Channels, const float *pVolumes, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_SetChannelVolumes(&This->IXAudio2SourceVoice_iface,
            Channels, pVolumes, OperationSet);
}

static void WINAPI XA23SRC_GetChannelVolumes(IXAudio23SourceVoice *iface,
        UINT32 Channels, float *pVolumes)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_GetChannelVolumes(&This->IXAudio2SourceVoice_iface,
            Channels, pVolumes);
}

static HRESULT WINAPI XA23SRC_SetOutputMatrix(IXAudio23SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_SetOutputMatrix(&This->IXAudio2SourceVoice_iface,
            pDestinationVoice, SourceChannels, DestinationChannels,
            pLevelMatrix, OperationSet);
}

static void WINAPI XA23SRC_GetOutputMatrix(IXAudio23SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_GetOutputMatrix(&This->IXAudio2SourceVoice_iface,
            pDestinationVoice, SourceChannels, DestinationChannels,
            pLevelMatrix);
}

static void WINAPI XA23SRC_DestroyVoice(IXAudio23SourceVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_DestroyVoice(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA23SRC_Start(IXAudio23SourceVoice *iface, UINT32 Flags,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_Start(&This->IXAudio2SourceVoice_iface, Flags, OperationSet);
}

static HRESULT WINAPI XA23SRC_Stop(IXAudio23SourceVoice *iface, UINT32 Flags,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_Stop(&This->IXAudio2SourceVoice_iface, Flags, OperationSet);
}

static HRESULT WINAPI XA23SRC_SubmitSourceBuffer(IXAudio23SourceVoice *iface,
        const XAUDIO2_BUFFER *pBuffer, const XAUDIO2_BUFFER_WMA *pBufferWMA)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_SubmitSourceBuffer(&This->IXAudio2SourceVoice_iface,
            pBuffer, pBufferWMA);
}

static HRESULT WINAPI XA23SRC_FlushSourceBuffers(IXAudio23SourceVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_FlushSourceBuffers(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA23SRC_Discontinuity(IXAudio23SourceVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_Discontinuity(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA23SRC_ExitLoop(IXAudio23SourceVoice *iface,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_ExitLoop(&This->IXAudio2SourceVoice_iface, OperationSet);
}

static void WINAPI XA23SRC_GetState(IXAudio23SourceVoice *iface,
        XAUDIO2_VOICE_STATE *pVoiceState)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_GetState(&This->IXAudio2SourceVoice_iface, pVoiceState, 0);
}

static HRESULT WINAPI XA23SRC_SetFrequencyRatio(IXAudio23SourceVoice *iface,
        float Ratio, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_SetFrequencyRatio(&This->IXAudio2SourceVoice_iface,
            Ratio, OperationSet);
}

static void WINAPI XA23SRC_GetFrequencyRatio(IXAudio23SourceVoice *iface,
        float *pRatio)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SourceVoice(iface);
    return IXAudio2SourceVoice_GetFrequencyRatio(&This->IXAudio2SourceVoice_iface, pRatio);
}

const IXAudio23SourceVoiceVtbl XAudio23SourceVoice_Vtbl = {
    XA23SRC_GetVoiceDetails,
    XA23SRC_SetOutputVoices,
    XA23SRC_SetEffectChain,
    XA23SRC_EnableEffect,
    XA23SRC_DisableEffect,
    XA23SRC_GetEffectState,
    XA23SRC_SetEffectParameters,
    XA23SRC_GetEffectParameters,
    XA23SRC_SetFilterParameters,
    XA23SRC_GetFilterParameters,
    XA23SRC_SetVolume,
    XA23SRC_GetVolume,
    XA23SRC_SetChannelVolumes,
    XA23SRC_GetChannelVolumes,
    XA23SRC_SetOutputMatrix,
    XA23SRC_GetOutputMatrix,
    XA23SRC_DestroyVoice,
    XA23SRC_Start,
    XA23SRC_Stop,
    XA23SRC_SubmitSourceBuffer,
    XA23SRC_FlushSourceBuffers,
    XA23SRC_Discontinuity,
    XA23SRC_ExitLoop,
    XA23SRC_GetState,
    XA23SRC_SetFrequencyRatio,
    XA23SRC_GetFrequencyRatio,
};

#elif XAUDIO2_VER <= 7

XA2VoiceImpl *impl_from_IXAudio27SourceVoice(IXAudio27SourceVoice *iface)
{
    return CONTAINING_RECORD(iface, XA2VoiceImpl, IXAudio27SourceVoice_iface);
}

static void WINAPI XA27SRC_GetVoiceDetails(IXAudio27SourceVoice *iface,
        XAUDIO27_VOICE_DETAILS *pVoiceDetails)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    XAUDIO2_VOICE_DETAILS details;

    IXAudio2SourceVoice_GetVoiceDetails(&This->IXAudio2SourceVoice_iface, &details);

    pVoiceDetails->CreationFlags = details.CreationFlags;
    pVoiceDetails->InputChannels = details.InputChannels;
    pVoiceDetails->InputSampleRate = details.InputSampleRate;
}

static HRESULT WINAPI XA27SRC_SetOutputVoices(IXAudio27SourceVoice *iface,
        const XAUDIO2_VOICE_SENDS *pSendList)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetOutputVoices(&This->IXAudio2SourceVoice_iface, pSendList);
}

static HRESULT WINAPI XA27SRC_SetEffectChain(IXAudio27SourceVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetEffectChain(&This->IXAudio2SourceVoice_iface, pEffectChain);
}

static HRESULT WINAPI XA27SRC_EnableEffect(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_EnableEffect(&This->IXAudio2SourceVoice_iface, EffectIndex, OperationSet);
}

static HRESULT WINAPI XA27SRC_DisableEffect(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_DisableEffect(&This->IXAudio2SourceVoice_iface, EffectIndex, OperationSet);
}

static void WINAPI XA27SRC_GetEffectState(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, BOOL *pEnabled)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_GetEffectState(&This->IXAudio2SourceVoice_iface, EffectIndex, pEnabled);
}

static HRESULT WINAPI XA27SRC_SetEffectParameters(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetEffectParameters(&This->IXAudio2SourceVoice_iface,
            EffectIndex, pParameters, ParametersByteSize, OperationSet);
}

static HRESULT WINAPI XA27SRC_GetEffectParameters(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_GetEffectParameters(&This->IXAudio2SourceVoice_iface,
            EffectIndex, pParameters, ParametersByteSize);
}

static HRESULT WINAPI XA27SRC_SetFilterParameters(IXAudio27SourceVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetFilterParameters(&This->IXAudio2SourceVoice_iface,
            pParameters, OperationSet);
}

static void WINAPI XA27SRC_GetFilterParameters(IXAudio27SourceVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_GetFilterParameters(&This->IXAudio2SourceVoice_iface, pParameters);
}

static HRESULT WINAPI XA27SRC_SetOutputFilterParameters(IXAudio27SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetOutputFilterParameters(&This->IXAudio2SourceVoice_iface,
            pDestinationVoice, pParameters, OperationSet);
}

static void WINAPI XA27SRC_GetOutputFilterParameters(IXAudio27SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_GetOutputFilterParameters(&This->IXAudio2SourceVoice_iface,
            pDestinationVoice, pParameters);
}

static HRESULT WINAPI XA27SRC_SetVolume(IXAudio27SourceVoice *iface, float Volume,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetVolume(&This->IXAudio2SourceVoice_iface, Volume,
            OperationSet);
}

static void WINAPI XA27SRC_GetVolume(IXAudio27SourceVoice *iface, float *pVolume)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_GetVolume(&This->IXAudio2SourceVoice_iface, pVolume);
}

static HRESULT WINAPI XA27SRC_SetChannelVolumes(IXAudio27SourceVoice *iface,
        UINT32 Channels, const float *pVolumes, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetChannelVolumes(&This->IXAudio2SourceVoice_iface, Channels,
            pVolumes, OperationSet);
}

static void WINAPI XA27SRC_GetChannelVolumes(IXAudio27SourceVoice *iface,
        UINT32 Channels, float *pVolumes)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_GetChannelVolumes(&This->IXAudio2SourceVoice_iface, Channels,
            pVolumes);
}

static HRESULT WINAPI XA27SRC_SetOutputMatrix(IXAudio27SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetOutputMatrix(&This->IXAudio2SourceVoice_iface,
            pDestinationVoice, SourceChannels, DestinationChannels,
            pLevelMatrix, OperationSet);
}

static void WINAPI XA27SRC_GetOutputMatrix(IXAudio27SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_GetOutputMatrix(&This->IXAudio2SourceVoice_iface, pDestinationVoice,
            SourceChannels, DestinationChannels, pLevelMatrix);
}

static void WINAPI XA27SRC_DestroyVoice(IXAudio27SourceVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_DestroyVoice(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA27SRC_Start(IXAudio27SourceVoice *iface, UINT32 Flags,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_Start(&This->IXAudio2SourceVoice_iface, Flags, OperationSet);
}

static HRESULT WINAPI XA27SRC_Stop(IXAudio27SourceVoice *iface, UINT32 Flags,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_Stop(&This->IXAudio2SourceVoice_iface, Flags, OperationSet);
}

static HRESULT WINAPI XA27SRC_SubmitSourceBuffer(IXAudio27SourceVoice *iface,
        const XAUDIO2_BUFFER *pBuffer, const XAUDIO2_BUFFER_WMA *pBufferWMA)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SubmitSourceBuffer(&This->IXAudio2SourceVoice_iface, pBuffer,
            pBufferWMA);
}

static HRESULT WINAPI XA27SRC_FlushSourceBuffers(IXAudio27SourceVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_FlushSourceBuffers(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA27SRC_Discontinuity(IXAudio27SourceVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_Discontinuity(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA27SRC_ExitLoop(IXAudio27SourceVoice *iface, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_ExitLoop(&This->IXAudio2SourceVoice_iface, OperationSet);
}

static void WINAPI XA27SRC_GetState(IXAudio27SourceVoice *iface,
        XAUDIO2_VOICE_STATE *pVoiceState)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_GetState(&This->IXAudio2SourceVoice_iface, pVoiceState, 0);
}

static HRESULT WINAPI XA27SRC_SetFrequencyRatio(IXAudio27SourceVoice *iface,
        float Ratio, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetFrequencyRatio(&This->IXAudio2SourceVoice_iface, Ratio, OperationSet);
}

static void WINAPI XA27SRC_GetFrequencyRatio(IXAudio27SourceVoice *iface, float *pRatio)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_GetFrequencyRatio(&This->IXAudio2SourceVoice_iface, pRatio);
}

static HRESULT WINAPI XA27SRC_SetSourceSampleRate(
    IXAudio27SourceVoice *iface,
    UINT32 NewSourceSampleRate)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetSourceSampleRate(&This->IXAudio2SourceVoice_iface, NewSourceSampleRate);
}

const IXAudio27SourceVoiceVtbl XAudio27SourceVoice_Vtbl = {
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
#endif
/* END IXAudio2SourceVoice */


/* BEGIN IXAudio2SubmixVoice */
#if XAUDIO2_VER == 0
XA2VoiceImpl *impl_from_IXAudio20SubmixVoice(IXAudio20SubmixVoice *iface)
{
    return CONTAINING_RECORD(iface, XA2VoiceImpl, IXAudio20SubmixVoice_iface);
}

static void WINAPI XA20SUB_GetVoiceDetails(IXAudio20SubmixVoice *iface,
        XAUDIO27_VOICE_DETAILS *pVoiceDetails)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    XAUDIO2_VOICE_DETAILS details;

    IXAudio2SubmixVoice_GetVoiceDetails(&This->IXAudio2SubmixVoice_iface, &details);

    pVoiceDetails->CreationFlags = details.CreationFlags;
    pVoiceDetails->InputChannels = details.InputChannels;
    pVoiceDetails->InputSampleRate = details.InputSampleRate;
}

static HRESULT WINAPI XA20SUB_SetOutputVoices(IXAudio20SubmixVoice *iface,
        const XAUDIO23_VOICE_SENDS *pSendList)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    XAUDIO2_VOICE_SENDS sends, *psends = NULL;
    HRESULT hr;

    TRACE("%p, %p\n", This, pSendList);

    if(pSendList){
        sends.SendCount = pSendList->OutputCount;
        sends.pSends = convert_send_descriptors23(pSendList);
        psends = &sends;
    }

    hr = IXAudio2SubmixVoice_SetOutputVoices(&This->IXAudio2SubmixVoice_iface, psends);

    if(pSendList)
        HeapFree(GetProcessHeap(), 0, sends.pSends);

    return hr;
}

static HRESULT WINAPI XA20SUB_SetEffectChain(IXAudio20SubmixVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetEffectChain(&This->IXAudio2SubmixVoice_iface, pEffectChain);
}

static HRESULT WINAPI XA20SUB_EnableEffect(IXAudio20SubmixVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_EnableEffect(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, OperationSet);
}

static HRESULT WINAPI XA20SUB_DisableEffect(IXAudio20SubmixVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_DisableEffect(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, OperationSet);
}

static void WINAPI XA20SUB_GetEffectState(IXAudio20SubmixVoice *iface,
        UINT32 EffectIndex, BOOL *pEnabled)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetEffectState(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, pEnabled);
}

static HRESULT WINAPI XA20SUB_SetEffectParameters(IXAudio20SubmixVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetEffectParameters(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, pParameters, ParametersByteSize, OperationSet);
}

static HRESULT WINAPI XA20SUB_GetEffectParameters(IXAudio20SubmixVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetEffectParameters(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, pParameters, ParametersByteSize);
}

static HRESULT WINAPI XA20SUB_SetFilterParameters(IXAudio20SubmixVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetFilterParameters(&This->IXAudio2SubmixVoice_iface,
            pParameters, OperationSet);
}

static void WINAPI XA20SUB_GetFilterParameters(IXAudio20SubmixVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetFilterParameters(&This->IXAudio2SubmixVoice_iface, pParameters);
}

static HRESULT WINAPI XA20SUB_SetVolume(IXAudio20SubmixVoice *iface,
        float Volume, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetVolume(&This->IXAudio2SubmixVoice_iface,
            Volume, OperationSet);
}

static void WINAPI XA20SUB_GetVolume(IXAudio20SubmixVoice *iface,
        float *pVolume)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetVolume(&This->IXAudio2SubmixVoice_iface, pVolume);
}

static HRESULT WINAPI XA20SUB_SetChannelVolumes(IXAudio20SubmixVoice *iface,
        UINT32 Channels, const float *pVolumes, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetChannelVolumes(&This->IXAudio2SubmixVoice_iface,
            Channels, pVolumes, OperationSet);
}

static void WINAPI XA20SUB_GetChannelVolumes(IXAudio20SubmixVoice *iface,
        UINT32 Channels, float *pVolumes)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetChannelVolumes(&This->IXAudio2SubmixVoice_iface,
            Channels, pVolumes);
}

static HRESULT WINAPI XA20SUB_SetOutputMatrix(IXAudio20SubmixVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SubmixChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetOutputMatrix(&This->IXAudio2SubmixVoice_iface,
            pDestinationVoice, SubmixChannels, DestinationChannels,
            pLevelMatrix, OperationSet);
}

static HRESULT WINAPI XA20SUB_GetOutputMatrix(IXAudio20SubmixVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SubmixChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    IXAudio2SubmixVoice_GetOutputMatrix(&This->IXAudio2SubmixVoice_iface,
            pDestinationVoice, SubmixChannels, DestinationChannels,
            pLevelMatrix);
    return S_OK;
}

static void WINAPI XA20SUB_DestroyVoice(IXAudio20SubmixVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio20SubmixVoice(iface);
    return IXAudio2SubmixVoice_DestroyVoice(&This->IXAudio2SubmixVoice_iface);
}

const IXAudio20SubmixVoiceVtbl XAudio20SubmixVoice_Vtbl = {
    XA20SUB_GetVoiceDetails,
    XA20SUB_SetOutputVoices,
    XA20SUB_SetEffectChain,
    XA20SUB_EnableEffect,
    XA20SUB_DisableEffect,
    XA20SUB_GetEffectState,
    XA20SUB_SetEffectParameters,
    XA20SUB_GetEffectParameters,
    XA20SUB_SetFilterParameters,
    XA20SUB_GetFilterParameters,
    XA20SUB_SetVolume,
    XA20SUB_GetVolume,
    XA20SUB_SetChannelVolumes,
    XA20SUB_GetChannelVolumes,
    XA20SUB_SetOutputMatrix,
    XA20SUB_GetOutputMatrix,
    XA20SUB_DestroyVoice
};

#elif XAUDIO2_VER <= 3

XA2VoiceImpl *impl_from_IXAudio23SubmixVoice(IXAudio23SubmixVoice *iface)
{
    return CONTAINING_RECORD(iface, XA2VoiceImpl, IXAudio23SubmixVoice_iface);
}

static void WINAPI XA23SUB_GetVoiceDetails(IXAudio23SubmixVoice *iface,
        XAUDIO27_VOICE_DETAILS *pVoiceDetails)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    XAUDIO2_VOICE_DETAILS details;

    IXAudio2SubmixVoice_GetVoiceDetails(&This->IXAudio2SubmixVoice_iface, &details);

    pVoiceDetails->CreationFlags = details.CreationFlags;
    pVoiceDetails->InputChannels = details.InputChannels;
    pVoiceDetails->InputSampleRate = details.InputSampleRate;
}

static HRESULT WINAPI XA23SUB_SetOutputVoices(IXAudio23SubmixVoice *iface,
        const XAUDIO23_VOICE_SENDS *pSendList)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    XAUDIO2_VOICE_SENDS sends, *psends = NULL;
    HRESULT hr;

    TRACE("%p, %p\n", This, pSendList);

    if(pSendList){
        sends.SendCount = pSendList->OutputCount;
        sends.pSends = convert_send_descriptors23(pSendList);
        psends = &sends;
    }

    hr = IXAudio2SubmixVoice_SetOutputVoices(&This->IXAudio2SubmixVoice_iface, psends);

    if(pSendList)
        HeapFree(GetProcessHeap(), 0, sends.pSends);

    return hr;
}

static HRESULT WINAPI XA23SUB_SetEffectChain(IXAudio23SubmixVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetEffectChain(&This->IXAudio2SubmixVoice_iface, pEffectChain);
}

static HRESULT WINAPI XA23SUB_EnableEffect(IXAudio23SubmixVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_EnableEffect(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, OperationSet);
}

static HRESULT WINAPI XA23SUB_DisableEffect(IXAudio23SubmixVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_DisableEffect(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, OperationSet);
}

static void WINAPI XA23SUB_GetEffectState(IXAudio23SubmixVoice *iface,
        UINT32 EffectIndex, BOOL *pEnabled)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetEffectState(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, pEnabled);
}

static HRESULT WINAPI XA23SUB_SetEffectParameters(IXAudio23SubmixVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetEffectParameters(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, pParameters, ParametersByteSize, OperationSet);
}

static HRESULT WINAPI XA23SUB_GetEffectParameters(IXAudio23SubmixVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetEffectParameters(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, pParameters, ParametersByteSize);
}

static HRESULT WINAPI XA23SUB_SetFilterParameters(IXAudio23SubmixVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetFilterParameters(&This->IXAudio2SubmixVoice_iface,
            pParameters, OperationSet);
}

static void WINAPI XA23SUB_GetFilterParameters(IXAudio23SubmixVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetFilterParameters(&This->IXAudio2SubmixVoice_iface, pParameters);
}

static HRESULT WINAPI XA23SUB_SetVolume(IXAudio23SubmixVoice *iface,
        float Volume, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetVolume(&This->IXAudio2SubmixVoice_iface,
            Volume, OperationSet);
}

static void WINAPI XA23SUB_GetVolume(IXAudio23SubmixVoice *iface,
        float *pVolume)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetVolume(&This->IXAudio2SubmixVoice_iface, pVolume);
}

static HRESULT WINAPI XA23SUB_SetChannelVolumes(IXAudio23SubmixVoice *iface,
        UINT32 Channels, const float *pVolumes, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetChannelVolumes(&This->IXAudio2SubmixVoice_iface,
            Channels, pVolumes, OperationSet);
}

static void WINAPI XA23SUB_GetChannelVolumes(IXAudio23SubmixVoice *iface,
        UINT32 Channels, float *pVolumes)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetChannelVolumes(&This->IXAudio2SubmixVoice_iface,
            Channels, pVolumes);
}

static HRESULT WINAPI XA23SUB_SetOutputMatrix(IXAudio23SubmixVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SubmixChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetOutputMatrix(&This->IXAudio2SubmixVoice_iface,
            pDestinationVoice, SubmixChannels, DestinationChannels,
            pLevelMatrix, OperationSet);
}

static void WINAPI XA23SUB_GetOutputMatrix(IXAudio23SubmixVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SubmixChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetOutputMatrix(&This->IXAudio2SubmixVoice_iface,
            pDestinationVoice, SubmixChannels, DestinationChannels,
            pLevelMatrix);
}

static void WINAPI XA23SUB_DestroyVoice(IXAudio23SubmixVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio23SubmixVoice(iface);
    return IXAudio2SubmixVoice_DestroyVoice(&This->IXAudio2SubmixVoice_iface);
}

const IXAudio23SubmixVoiceVtbl XAudio23SubmixVoice_Vtbl = {
    XA23SUB_GetVoiceDetails,
    XA23SUB_SetOutputVoices,
    XA23SUB_SetEffectChain,
    XA23SUB_EnableEffect,
    XA23SUB_DisableEffect,
    XA23SUB_GetEffectState,
    XA23SUB_SetEffectParameters,
    XA23SUB_GetEffectParameters,
    XA23SUB_SetFilterParameters,
    XA23SUB_GetFilterParameters,
    XA23SUB_SetVolume,
    XA23SUB_GetVolume,
    XA23SUB_SetChannelVolumes,
    XA23SUB_GetChannelVolumes,
    XA23SUB_SetOutputMatrix,
    XA23SUB_GetOutputMatrix,
    XA23SUB_DestroyVoice
};

#elif XAUDIO2_VER <= 7

XA2VoiceImpl *impl_from_IXAudio27SubmixVoice(IXAudio27SubmixVoice *iface)
{
    return CONTAINING_RECORD(iface, XA2VoiceImpl, IXAudio27SubmixVoice_iface);
}

static void WINAPI XA27SUB_GetVoiceDetails(IXAudio27SubmixVoice *iface,
        XAUDIO27_VOICE_DETAILS *pVoiceDetails)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    XAUDIO2_VOICE_DETAILS details;

    IXAudio2SubmixVoice_GetVoiceDetails(&This->IXAudio2SubmixVoice_iface, &details);

    pVoiceDetails->CreationFlags = details.CreationFlags;
    pVoiceDetails->InputChannels = details.InputChannels;
    pVoiceDetails->InputSampleRate = details.InputSampleRate;
}

static HRESULT WINAPI XA27SUB_SetOutputVoices(IXAudio27SubmixVoice *iface,
        const XAUDIO2_VOICE_SENDS *pSendList)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetOutputVoices(&This->IXAudio2SubmixVoice_iface, pSendList);
}

static HRESULT WINAPI XA27SUB_SetEffectChain(IXAudio27SubmixVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetEffectChain(&This->IXAudio2SubmixVoice_iface, pEffectChain);
}

static HRESULT WINAPI XA27SUB_EnableEffect(IXAudio27SubmixVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_EnableEffect(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, OperationSet);
}

static HRESULT WINAPI XA27SUB_DisableEffect(IXAudio27SubmixVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_DisableEffect(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, OperationSet);
}

static void WINAPI XA27SUB_GetEffectState(IXAudio27SubmixVoice *iface,
        UINT32 EffectIndex, BOOL *pEnabled)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetEffectState(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, pEnabled);
}

static HRESULT WINAPI XA27SUB_SetEffectParameters(IXAudio27SubmixVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetEffectParameters(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, pParameters, ParametersByteSize, OperationSet);
}

static HRESULT WINAPI XA27SUB_GetEffectParameters(IXAudio27SubmixVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetEffectParameters(&This->IXAudio2SubmixVoice_iface,
            EffectIndex, pParameters, ParametersByteSize);
}

static HRESULT WINAPI XA27SUB_SetFilterParameters(IXAudio27SubmixVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetFilterParameters(&This->IXAudio2SubmixVoice_iface,
            pParameters, OperationSet);
}

static void WINAPI XA27SUB_GetFilterParameters(IXAudio27SubmixVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetFilterParameters(&This->IXAudio2SubmixVoice_iface, pParameters);
}

static HRESULT WINAPI XA27SUB_SetOutputFilterParameters(IXAudio27SubmixVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetOutputFilterParameters(&This->IXAudio2SubmixVoice_iface,
            pDestinationVoice, pParameters, OperationSet);
}

static void WINAPI XA27SUB_GetOutputFilterParameters(IXAudio27SubmixVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    IXAudio2SubmixVoice_GetOutputFilterParameters(&This->IXAudio2SubmixVoice_iface,
            pDestinationVoice, pParameters);
}

static HRESULT WINAPI XA27SUB_SetVolume(IXAudio27SubmixVoice *iface,
        float Volume, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetVolume(&This->IXAudio2SubmixVoice_iface,
            Volume, OperationSet);
}

static void WINAPI XA27SUB_GetVolume(IXAudio27SubmixVoice *iface,
        float *pVolume)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetVolume(&This->IXAudio2SubmixVoice_iface, pVolume);
}

static HRESULT WINAPI XA27SUB_SetChannelVolumes(IXAudio27SubmixVoice *iface,
        UINT32 Channels, const float *pVolumes, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetChannelVolumes(&This->IXAudio2SubmixVoice_iface,
            Channels, pVolumes, OperationSet);
}

static void WINAPI XA27SUB_GetChannelVolumes(IXAudio27SubmixVoice *iface,
        UINT32 Channels, float *pVolumes)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetChannelVolumes(&This->IXAudio2SubmixVoice_iface,
            Channels, pVolumes);
}

static HRESULT WINAPI XA27SUB_SetOutputMatrix(IXAudio27SubmixVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SubmixChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_SetOutputMatrix(&This->IXAudio2SubmixVoice_iface,
            pDestinationVoice, SubmixChannels, DestinationChannels,
            pLevelMatrix, OperationSet);
}

static void WINAPI XA27SUB_GetOutputMatrix(IXAudio27SubmixVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SubmixChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_GetOutputMatrix(&This->IXAudio2SubmixVoice_iface,
            pDestinationVoice, SubmixChannels, DestinationChannels,
            pLevelMatrix);
}

static void WINAPI XA27SUB_DestroyVoice(IXAudio27SubmixVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio27SubmixVoice(iface);
    return IXAudio2SubmixVoice_DestroyVoice(&This->IXAudio2SubmixVoice_iface);
}

const IXAudio27SubmixVoiceVtbl XAudio27SubmixVoice_Vtbl = {
    XA27SUB_GetVoiceDetails,
    XA27SUB_SetOutputVoices,
    XA27SUB_SetEffectChain,
    XA27SUB_EnableEffect,
    XA27SUB_DisableEffect,
    XA27SUB_GetEffectState,
    XA27SUB_SetEffectParameters,
    XA27SUB_GetEffectParameters,
    XA27SUB_SetFilterParameters,
    XA27SUB_GetFilterParameters,
    XA27SUB_SetOutputFilterParameters,
    XA27SUB_GetOutputFilterParameters,
    XA27SUB_SetVolume,
    XA27SUB_GetVolume,
    XA27SUB_SetChannelVolumes,
    XA27SUB_GetChannelVolumes,
    XA27SUB_SetOutputMatrix,
    XA27SUB_GetOutputMatrix,
    XA27SUB_DestroyVoice
};
#endif
/* END IXAudio2SubmixVoice */


/* BEGIN IXAudio2MasteringVoice */
#if XAUDIO2_VER == 0
XA2VoiceImpl *impl_from_IXAudio20MasteringVoice(IXAudio20MasteringVoice *iface)
{
    return CONTAINING_RECORD(iface, XA2VoiceImpl, IXAudio20MasteringVoice_iface);
}

static void WINAPI XA20M_GetVoiceDetails(IXAudio20MasteringVoice *iface,
        XAUDIO27_VOICE_DETAILS *pVoiceDetails)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    XAUDIO2_VOICE_DETAILS details;

    IXAudio2MasteringVoice_GetVoiceDetails(&This->IXAudio2MasteringVoice_iface, &details);

    pVoiceDetails->CreationFlags = details.CreationFlags;
    pVoiceDetails->InputChannels = details.InputChannels;
    pVoiceDetails->InputSampleRate = details.InputSampleRate;
}

static HRESULT WINAPI XA20M_SetOutputVoices(IXAudio20MasteringVoice *iface,
        const XAUDIO23_VOICE_SENDS *pSendList)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    XAUDIO2_VOICE_SENDS sends, *psends = NULL;
    HRESULT hr;

    TRACE("%p, %p\n", This, pSendList);

    if(pSendList){
        sends.SendCount = pSendList->OutputCount;
        sends.pSends = convert_send_descriptors23(pSendList);
        psends = &sends;
    }

    hr = IXAudio2MasteringVoice_SetOutputVoices(&This->IXAudio2MasteringVoice_iface, psends);

    if(pSendList)
        HeapFree(GetProcessHeap(), 0, sends.pSends);

    return hr;
}

static HRESULT WINAPI XA20M_SetEffectChain(IXAudio20MasteringVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetEffectChain(&This->IXAudio2MasteringVoice_iface, pEffectChain);
}

static HRESULT WINAPI XA20M_EnableEffect(IXAudio20MasteringVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_EnableEffect(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, OperationSet);
}

static HRESULT WINAPI XA20M_DisableEffect(IXAudio20MasteringVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_DisableEffect(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, OperationSet);
}

static void WINAPI XA20M_GetEffectState(IXAudio20MasteringVoice *iface,
        UINT32 EffectIndex, BOOL *pEnabled)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetEffectState(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, pEnabled);
}

static HRESULT WINAPI XA20M_SetEffectParameters(IXAudio20MasteringVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetEffectParameters(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, pParameters, ParametersByteSize, OperationSet);
}

static HRESULT WINAPI XA20M_GetEffectParameters(IXAudio20MasteringVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetEffectParameters(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, pParameters, ParametersByteSize);
}

static HRESULT WINAPI XA20M_SetFilterParameters(IXAudio20MasteringVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetFilterParameters(&This->IXAudio2MasteringVoice_iface,
            pParameters, OperationSet);
}

static void WINAPI XA20M_GetFilterParameters(IXAudio20MasteringVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetFilterParameters(&This->IXAudio2MasteringVoice_iface, pParameters);
}

static HRESULT WINAPI XA20M_SetVolume(IXAudio20MasteringVoice *iface,
        float Volume, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetVolume(&This->IXAudio2MasteringVoice_iface,
            Volume, OperationSet);
}

static void WINAPI XA20M_GetVolume(IXAudio20MasteringVoice *iface,
        float *pVolume)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetVolume(&This->IXAudio2MasteringVoice_iface, pVolume);
}

static HRESULT WINAPI XA20M_SetChannelVolumes(IXAudio20MasteringVoice *iface,
        UINT32 Channels, const float *pVolumes, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetChannelVolumes(&This->IXAudio2MasteringVoice_iface,
            Channels, pVolumes, OperationSet);
}

static void WINAPI XA20M_GetChannelVolumes(IXAudio20MasteringVoice *iface,
        UINT32 Channels, float *pVolumes)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetChannelVolumes(&This->IXAudio2MasteringVoice_iface,
            Channels, pVolumes);
}

static HRESULT WINAPI XA20M_SetOutputMatrix(IXAudio20MasteringVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 MasteringChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetOutputMatrix(&This->IXAudio2MasteringVoice_iface,
            pDestinationVoice, MasteringChannels, DestinationChannels,
            pLevelMatrix, OperationSet);
}

static HRESULT WINAPI XA20M_GetOutputMatrix(IXAudio20MasteringVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 MasteringChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    IXAudio2MasteringVoice_GetOutputMatrix(&This->IXAudio2MasteringVoice_iface,
            pDestinationVoice, MasteringChannels, DestinationChannels,
            pLevelMatrix);
    return S_OK;
}

static void WINAPI XA20M_DestroyVoice(IXAudio20MasteringVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio20MasteringVoice(iface);
    return IXAudio2MasteringVoice_DestroyVoice(&This->IXAudio2MasteringVoice_iface);
}

const IXAudio20MasteringVoiceVtbl XAudio20MasteringVoice_Vtbl = {
    XA20M_GetVoiceDetails,
    XA20M_SetOutputVoices,
    XA20M_SetEffectChain,
    XA20M_EnableEffect,
    XA20M_DisableEffect,
    XA20M_GetEffectState,
    XA20M_SetEffectParameters,
    XA20M_GetEffectParameters,
    XA20M_SetFilterParameters,
    XA20M_GetFilterParameters,
    XA20M_SetVolume,
    XA20M_GetVolume,
    XA20M_SetChannelVolumes,
    XA20M_GetChannelVolumes,
    XA20M_SetOutputMatrix,
    XA20M_GetOutputMatrix,
    XA20M_DestroyVoice
};

#elif XAUDIO2_VER <= 3

XA2VoiceImpl *impl_from_IXAudio23MasteringVoice(IXAudio23MasteringVoice *iface)
{
    return CONTAINING_RECORD(iface, XA2VoiceImpl, IXAudio23MasteringVoice_iface);
}

static void WINAPI XA23M_GetVoiceDetails(IXAudio23MasteringVoice *iface,
        XAUDIO27_VOICE_DETAILS *pVoiceDetails)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    XAUDIO2_VOICE_DETAILS details;

    IXAudio2MasteringVoice_GetVoiceDetails(&This->IXAudio2MasteringVoice_iface, &details);

    pVoiceDetails->CreationFlags = details.CreationFlags;
    pVoiceDetails->InputChannels = details.InputChannels;
    pVoiceDetails->InputSampleRate = details.InputSampleRate;
}

static HRESULT WINAPI XA23M_SetOutputVoices(IXAudio23MasteringVoice *iface,
        const XAUDIO23_VOICE_SENDS *pSendList)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    XAUDIO2_VOICE_SENDS sends, *psends = NULL;
    HRESULT hr;

    TRACE("%p, %p\n", This, pSendList);

    if(pSendList){
        sends.SendCount = pSendList->OutputCount;
        sends.pSends = convert_send_descriptors23(pSendList);
        psends = &sends;
    }

    hr = IXAudio2MasteringVoice_SetOutputVoices(&This->IXAudio2MasteringVoice_iface, psends);

    if(pSendList)
        HeapFree(GetProcessHeap(), 0, sends.pSends);

    return hr;
}

static HRESULT WINAPI XA23M_SetEffectChain(IXAudio23MasteringVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetEffectChain(&This->IXAudio2MasteringVoice_iface, pEffectChain);
}

static HRESULT WINAPI XA23M_EnableEffect(IXAudio23MasteringVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_EnableEffect(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, OperationSet);
}

static HRESULT WINAPI XA23M_DisableEffect(IXAudio23MasteringVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_DisableEffect(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, OperationSet);
}

static void WINAPI XA23M_GetEffectState(IXAudio23MasteringVoice *iface,
        UINT32 EffectIndex, BOOL *pEnabled)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetEffectState(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, pEnabled);
}

static HRESULT WINAPI XA23M_SetEffectParameters(IXAudio23MasteringVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetEffectParameters(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, pParameters, ParametersByteSize, OperationSet);
}

static HRESULT WINAPI XA23M_GetEffectParameters(IXAudio23MasteringVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetEffectParameters(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, pParameters, ParametersByteSize);
}

static HRESULT WINAPI XA23M_SetFilterParameters(IXAudio23MasteringVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetFilterParameters(&This->IXAudio2MasteringVoice_iface,
            pParameters, OperationSet);
}

static void WINAPI XA23M_GetFilterParameters(IXAudio23MasteringVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetFilterParameters(&This->IXAudio2MasteringVoice_iface, pParameters);
}

static HRESULT WINAPI XA23M_SetVolume(IXAudio23MasteringVoice *iface,
        float Volume, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetVolume(&This->IXAudio2MasteringVoice_iface,
            Volume, OperationSet);
}

static void WINAPI XA23M_GetVolume(IXAudio23MasteringVoice *iface,
        float *pVolume)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetVolume(&This->IXAudio2MasteringVoice_iface, pVolume);
}

static HRESULT WINAPI XA23M_SetChannelVolumes(IXAudio23MasteringVoice *iface,
        UINT32 Channels, const float *pVolumes, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetChannelVolumes(&This->IXAudio2MasteringVoice_iface,
            Channels, pVolumes, OperationSet);
}

static void WINAPI XA23M_GetChannelVolumes(IXAudio23MasteringVoice *iface,
        UINT32 Channels, float *pVolumes)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetChannelVolumes(&This->IXAudio2MasteringVoice_iface,
            Channels, pVolumes);
}

static HRESULT WINAPI XA23M_SetOutputMatrix(IXAudio23MasteringVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 MasteringChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetOutputMatrix(&This->IXAudio2MasteringVoice_iface,
            pDestinationVoice, MasteringChannels, DestinationChannels,
            pLevelMatrix, OperationSet);
}

static void WINAPI XA23M_GetOutputMatrix(IXAudio23MasteringVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 MasteringChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetOutputMatrix(&This->IXAudio2MasteringVoice_iface,
            pDestinationVoice, MasteringChannels, DestinationChannels,
            pLevelMatrix);
}

static void WINAPI XA23M_DestroyVoice(IXAudio23MasteringVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio23MasteringVoice(iface);
    return IXAudio2MasteringVoice_DestroyVoice(&This->IXAudio2MasteringVoice_iface);
}

const IXAudio23MasteringVoiceVtbl XAudio23MasteringVoice_Vtbl = {
    XA23M_GetVoiceDetails,
    XA23M_SetOutputVoices,
    XA23M_SetEffectChain,
    XA23M_EnableEffect,
    XA23M_DisableEffect,
    XA23M_GetEffectState,
    XA23M_SetEffectParameters,
    XA23M_GetEffectParameters,
    XA23M_SetFilterParameters,
    XA23M_GetFilterParameters,
    XA23M_SetVolume,
    XA23M_GetVolume,
    XA23M_SetChannelVolumes,
    XA23M_GetChannelVolumes,
    XA23M_SetOutputMatrix,
    XA23M_GetOutputMatrix,
    XA23M_DestroyVoice
};

#elif XAUDIO2_VER <= 7

XA2VoiceImpl *impl_from_IXAudio27MasteringVoice(IXAudio27MasteringVoice *iface)
{
    return CONTAINING_RECORD(iface, XA2VoiceImpl, IXAudio27MasteringVoice_iface);
}

static void WINAPI XA27M_GetVoiceDetails(IXAudio27MasteringVoice *iface,
        XAUDIO27_VOICE_DETAILS *pVoiceDetails)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    XAUDIO2_VOICE_DETAILS details;

    IXAudio2MasteringVoice_GetVoiceDetails(&This->IXAudio2MasteringVoice_iface, &details);

    pVoiceDetails->CreationFlags = details.CreationFlags;
    pVoiceDetails->InputChannels = details.InputChannels;
    pVoiceDetails->InputSampleRate = details.InputSampleRate;
}

static HRESULT WINAPI XA27M_SetOutputVoices(IXAudio27MasteringVoice *iface,
        const XAUDIO2_VOICE_SENDS *pSendList)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetOutputVoices(&This->IXAudio2MasteringVoice_iface, pSendList);
}

static HRESULT WINAPI XA27M_SetEffectChain(IXAudio27MasteringVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetEffectChain(&This->IXAudio2MasteringVoice_iface, pEffectChain);
}

static HRESULT WINAPI XA27M_EnableEffect(IXAudio27MasteringVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_EnableEffect(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, OperationSet);
}

static HRESULT WINAPI XA27M_DisableEffect(IXAudio27MasteringVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_DisableEffect(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, OperationSet);
}

static void WINAPI XA27M_GetEffectState(IXAudio27MasteringVoice *iface,
        UINT32 EffectIndex, BOOL *pEnabled)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetEffectState(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, pEnabled);
}

static HRESULT WINAPI XA27M_SetEffectParameters(IXAudio27MasteringVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetEffectParameters(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, pParameters, ParametersByteSize, OperationSet);
}

static HRESULT WINAPI XA27M_GetEffectParameters(IXAudio27MasteringVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetEffectParameters(&This->IXAudio2MasteringVoice_iface,
            EffectIndex, pParameters, ParametersByteSize);
}

static HRESULT WINAPI XA27M_SetFilterParameters(IXAudio27MasteringVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetFilterParameters(&This->IXAudio2MasteringVoice_iface,
            pParameters, OperationSet);
}

static void WINAPI XA27M_GetFilterParameters(IXAudio27MasteringVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetFilterParameters(&This->IXAudio2MasteringVoice_iface, pParameters);
}

static HRESULT WINAPI XA27M_SetOutputFilterParameters(IXAudio27MasteringVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetOutputFilterParameters(&This->IXAudio2MasteringVoice_iface,
            pDestinationVoice, pParameters, OperationSet);
}

static void WINAPI XA27M_GetOutputFilterParameters(IXAudio27MasteringVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    IXAudio2MasteringVoice_GetOutputFilterParameters(&This->IXAudio2MasteringVoice_iface,
            pDestinationVoice, pParameters);
}

static HRESULT WINAPI XA27M_SetVolume(IXAudio27MasteringVoice *iface,
        float Volume, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetVolume(&This->IXAudio2MasteringVoice_iface,
            Volume, OperationSet);
}

static void WINAPI XA27M_GetVolume(IXAudio27MasteringVoice *iface,
        float *pVolume)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetVolume(&This->IXAudio2MasteringVoice_iface, pVolume);
}

static HRESULT WINAPI XA27M_SetChannelVolumes(IXAudio27MasteringVoice *iface,
        UINT32 Channels, const float *pVolumes, UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetChannelVolumes(&This->IXAudio2MasteringVoice_iface,
            Channels, pVolumes, OperationSet);
}

static void WINAPI XA27M_GetChannelVolumes(IXAudio27MasteringVoice *iface,
        UINT32 Channels, float *pVolumes)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetChannelVolumes(&This->IXAudio2MasteringVoice_iface,
            Channels, pVolumes);
}

static HRESULT WINAPI XA27M_SetOutputMatrix(IXAudio27MasteringVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 MasteringChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_SetOutputMatrix(&This->IXAudio2MasteringVoice_iface,
            pDestinationVoice, MasteringChannels, DestinationChannels,
            pLevelMatrix, OperationSet);
}

static void WINAPI XA27M_GetOutputMatrix(IXAudio27MasteringVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 MasteringChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_GetOutputMatrix(&This->IXAudio2MasteringVoice_iface,
            pDestinationVoice, MasteringChannels, DestinationChannels,
            pLevelMatrix);
}

static void WINAPI XA27M_DestroyVoice(IXAudio27MasteringVoice *iface)
{
    XA2VoiceImpl *This = impl_from_IXAudio27MasteringVoice(iface);
    return IXAudio2MasteringVoice_DestroyVoice(&This->IXAudio2MasteringVoice_iface);
}

const IXAudio27MasteringVoiceVtbl XAudio27MasteringVoice_Vtbl = {
    XA27M_GetVoiceDetails,
    XA27M_SetOutputVoices,
    XA27M_SetEffectChain,
    XA27M_EnableEffect,
    XA27M_DisableEffect,
    XA27M_GetEffectState,
    XA27M_SetEffectParameters,
    XA27M_GetEffectParameters,
    XA27M_SetFilterParameters,
    XA27M_GetFilterParameters,
    XA27M_SetOutputFilterParameters,
    XA27M_GetOutputFilterParameters,
    XA27M_SetVolume,
    XA27M_GetVolume,
    XA27M_SetChannelVolumes,
    XA27M_GetChannelVolumes,
    XA27M_SetOutputMatrix,
    XA27M_GetOutputMatrix,
    XA27M_DestroyVoice
};
#endif
/* END IXAudio2MasteringVoice */


/* BEGIN IXAudio2 */
#if XAUDIO2_VER == 0
static inline IXAudio2Impl *impl_from_IXAudio20(IXAudio20 *iface)
{
    return CONTAINING_RECORD(iface, IXAudio2Impl, IXAudio20_iface);
}

static HRESULT WINAPI XA20_QueryInterface(IXAudio20 *iface, REFIID riid,
        void **ppvObject)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    return IXAudio2_QueryInterface(&This->IXAudio2_iface, riid, ppvObject);
}

static ULONG WINAPI XA20_AddRef(IXAudio20 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    return IXAudio2_AddRef(&This->IXAudio2_iface);
}

static ULONG WINAPI XA20_Release(IXAudio20 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    return IXAudio2_Release(&This->IXAudio2_iface);
}

static HRESULT WINAPI XA20_GetDeviceCount(IXAudio20 *iface, UINT32 *pCount)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    TRACE("%p, %p\n", This, pCount);
    return FAudio_GetDeviceCount(This->faudio, pCount);
}

static HRESULT WINAPI XA20_GetDeviceDetails(IXAudio20 *iface, UINT32 index,
        XAUDIO2_DEVICE_DETAILS *pDeviceDetails)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    TRACE("%p, %u, %p\n", This, index, pDeviceDetails);
    return FAudio_GetDeviceDetails(This->faudio, index, (FAudioDeviceDetails *)pDeviceDetails);
}

static HRESULT WINAPI XA20_Initialize(IXAudio20 *iface, UINT32 flags,
        XAUDIO2_PROCESSOR processor)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    TRACE("(%p)->(0x%x, 0x%x)\n", This, flags, processor);
    return xaudio2_initialize(This, flags, processor);
}

static HRESULT WINAPI XA20_RegisterForCallbacks(IXAudio20 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    return IXAudio2_RegisterForCallbacks(&This->IXAudio2_iface, pCallback);
}

static void WINAPI XA20_UnregisterForCallbacks(IXAudio20 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    return IXAudio2_UnregisterForCallbacks(&This->IXAudio2_iface, pCallback);
}

static HRESULT WINAPI XA20_CreateSourceVoice(IXAudio20 *iface,
        IXAudio2SourceVoice **ppSourceVoice, const WAVEFORMATEX *pSourceFormat,
        UINT32 flags, float maxFrequencyRatio,
        IXAudio2VoiceCallback *pCallback, const XAUDIO23_VOICE_SENDS *pSendList,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    XAUDIO2_VOICE_SENDS sends, *psends = NULL;
    HRESULT hr;

    if(pSendList){
        sends.SendCount = pSendList->OutputCount;
        sends.pSends = convert_send_descriptors23(pSendList);
        psends = &sends;
    }

    hr = IXAudio2_CreateSourceVoice(&This->IXAudio2_iface, ppSourceVoice,
            pSourceFormat, flags, maxFrequencyRatio, pCallback, psends,
            pEffectChain);

    if(pSendList)
        HeapFree(GetProcessHeap(), 0, sends.pSends);

    return hr;
}

static HRESULT WINAPI XA20_CreateSubmixVoice(IXAudio20 *iface,
        IXAudio2SubmixVoice **ppSubmixVoice, UINT32 inputChannels,
        UINT32 inputSampleRate, UINT32 flags, UINT32 processingStage,
        const XAUDIO23_VOICE_SENDS *pSendList,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    XAUDIO2_VOICE_SENDS sends, *psends = NULL;
    HRESULT hr;

    if(pSendList){
        sends.SendCount = pSendList->OutputCount;
        sends.pSends = convert_send_descriptors23(pSendList);
        psends = &sends;
    }

    hr = IXAudio2_CreateSubmixVoice(&This->IXAudio2_iface, ppSubmixVoice,
            inputChannels, inputSampleRate, flags, processingStage, psends,
            pEffectChain);

    if(pSendList)
        HeapFree(GetProcessHeap(), 0, sends.pSends);

    return hr;
}

static HRESULT WINAPI XA20_CreateMasteringVoice(IXAudio20 *iface,
        IXAudio2MasteringVoice **ppMasteringVoice, UINT32 inputChannels,
        UINT32 inputSampleRate, UINT32 flags, UINT32 deviceIndex,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);

    TRACE("(%p)->(%p, %u, %u, 0x%x, %u, %p)\n", This, ppMasteringVoice,
            inputChannels, inputSampleRate, flags, deviceIndex,
            pEffectChain);

    EnterCriticalSection(&This->lock);

    /* XAUDIO2_VER == 0 */
    *ppMasteringVoice = (IXAudio2MasteringVoice*)&This->mst.IXAudio20MasteringVoice_iface;

    EnterCriticalSection(&This->mst.lock);

    if(This->mst.in_use){
        LeaveCriticalSection(&This->mst.lock);
        LeaveCriticalSection(&This->lock);
        return COMPAT_E_INVALID_CALL;
    }

    LeaveCriticalSection(&This->lock);

    This->mst.effect_chain = wrap_effect_chain(pEffectChain);

    pthread_mutex_lock(&This->mst.engine_lock);

    This->mst.engine_thread = CreateThread(NULL, 0, &engine_thread, &This->mst, 0, NULL);

    pthread_cond_wait(&This->mst.engine_done, &This->mst.engine_lock);

    pthread_mutex_unlock(&This->mst.engine_lock);

    FAudio_SetEngineProcedureEXT(This->faudio, &engine_cb, &This->mst);

    FAudio_CreateMasteringVoice(This->faudio, &This->mst.faudio_voice, inputChannels,
            inputSampleRate, flags, deviceIndex, This->mst.effect_chain);

    This->mst.in_use = TRUE;

    LeaveCriticalSection(&This->mst.lock);

    return S_OK;
}

static HRESULT WINAPI XA20_StartEngine(IXAudio20 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    return IXAudio2_StartEngine(&This->IXAudio2_iface);
}

static void WINAPI XA20_StopEngine(IXAudio20 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    return IXAudio2_StopEngine(&This->IXAudio2_iface);
}

static HRESULT WINAPI XA20_CommitChanges(IXAudio20 *iface, UINT32 operationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    return IXAudio2_CommitChanges(&This->IXAudio2_iface, operationSet);
}

static void WINAPI XA20_GetPerformanceData(IXAudio20 *iface,
        XAUDIO20_PERFORMANCE_DATA *pPerfData)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    XAUDIO2_PERFORMANCE_DATA data;

    IXAudio2_GetPerformanceData(&This->IXAudio2_iface, &data);

    pPerfData->AudioCyclesSinceLastQuery = data.AudioCyclesSinceLastQuery;
    pPerfData->TotalCyclesSinceLastQuery = data.TotalCyclesSinceLastQuery;
    pPerfData->MinimumCyclesPerQuantum = data.MinimumCyclesPerQuantum;
    pPerfData->MaximumCyclesPerQuantum = data.MaximumCyclesPerQuantum;
    pPerfData->MemoryUsageInBytes = data.MemoryUsageInBytes;
    pPerfData->CurrentLatencyInSamples = data.CurrentLatencyInSamples;
    pPerfData->GlitchesSinceLastQuery = data.GlitchesSinceEngineStarted - This->last_query_glitches;
    This->last_query_glitches = data.GlitchesSinceEngineStarted;
    pPerfData->ActiveSourceVoiceCount = data.ActiveSourceVoiceCount;
    pPerfData->TotalSourceVoiceCount = data.TotalSourceVoiceCount;

    pPerfData->ActiveSubmixVoiceCount = data.ActiveSubmixVoiceCount;
    pPerfData->TotalSubmixVoiceCount = data.ActiveSubmixVoiceCount;

    pPerfData->ActiveXmaSourceVoices = data.ActiveXmaSourceVoices;
    pPerfData->ActiveXmaStreams = data.ActiveXmaStreams;
}

static void WINAPI XA20_SetDebugConfiguration(IXAudio20 *iface,
        const XAUDIO2_DEBUG_CONFIGURATION *pDebugConfiguration,
        void *pReserved)
{
    IXAudio2Impl *This = impl_from_IXAudio20(iface);
    return IXAudio2_SetDebugConfiguration(&This->IXAudio2_iface,
            pDebugConfiguration, pReserved);
}

const IXAudio20Vtbl XAudio20_Vtbl = {
    XA20_QueryInterface,
    XA20_AddRef,
    XA20_Release,
    XA20_GetDeviceCount,
    XA20_GetDeviceDetails,
    XA20_Initialize,
    XA20_RegisterForCallbacks,
    XA20_UnregisterForCallbacks,
    XA20_CreateSourceVoice,
    XA20_CreateSubmixVoice,
    XA20_CreateMasteringVoice,
    XA20_StartEngine,
    XA20_StopEngine,
    XA20_CommitChanges,
    XA20_GetPerformanceData,
    XA20_SetDebugConfiguration
};

#elif XAUDIO2_VER <= 2

static inline IXAudio2Impl *impl_from_IXAudio22(IXAudio22 *iface)
{
    return CONTAINING_RECORD(iface, IXAudio2Impl, IXAudio22_iface);
}

static HRESULT WINAPI XA22_QueryInterface(IXAudio22 *iface, REFIID riid,
        void **ppvObject)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    return IXAudio2_QueryInterface(&This->IXAudio2_iface, riid, ppvObject);
}

static ULONG WINAPI XA22_AddRef(IXAudio22 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    return IXAudio2_AddRef(&This->IXAudio2_iface);
}

static ULONG WINAPI XA22_Release(IXAudio22 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    return IXAudio2_Release(&This->IXAudio2_iface);
}

static HRESULT WINAPI XA22_GetDeviceCount(IXAudio22 *iface, UINT32 *pCount)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    TRACE("%p, %p\n", This, pCount);
    return FAudio_GetDeviceCount(This->faudio, pCount);
}

static HRESULT WINAPI XA22_GetDeviceDetails(IXAudio22 *iface, UINT32 index,
        XAUDIO2_DEVICE_DETAILS *pDeviceDetails)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    TRACE("%p, %u, %p\n", This, index, pDeviceDetails);
    return FAudio_GetDeviceDetails(This->faudio, index, (FAudioDeviceDetails *)pDeviceDetails);
}

static HRESULT WINAPI XA22_Initialize(IXAudio22 *iface, UINT32 flags,
        XAUDIO2_PROCESSOR processor)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    TRACE("(%p)->(0x%x, 0x%x)\n", This, flags, processor);
    return xaudio2_initialize(This, flags, processor);
}

static HRESULT WINAPI XA22_RegisterForCallbacks(IXAudio22 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    return IXAudio2_RegisterForCallbacks(&This->IXAudio2_iface, pCallback);
}

static void WINAPI XA22_UnregisterForCallbacks(IXAudio22 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    IXAudio2_UnregisterForCallbacks(&This->IXAudio2_iface, pCallback);
}

static HRESULT WINAPI XA22_CreateSourceVoice(IXAudio22 *iface,
        IXAudio2SourceVoice **ppSourceVoice, const WAVEFORMATEX *pSourceFormat,
        UINT32 flags, float maxFrequencyRatio,
        IXAudio2VoiceCallback *pCallback, const XAUDIO23_VOICE_SENDS *pSendList,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    XAUDIO2_VOICE_SENDS sends, *psends = NULL;
    HRESULT hr;

    if(pSendList){
        sends.SendCount = pSendList->OutputCount;
        sends.pSends = convert_send_descriptors23(pSendList);
        psends = &sends;
    }

    hr = IXAudio2_CreateSourceVoice(&This->IXAudio2_iface, ppSourceVoice,
            pSourceFormat, flags, maxFrequencyRatio, pCallback, psends,
            pEffectChain);

    if(pSendList)
        HeapFree(GetProcessHeap(), 0, sends.pSends);

    return hr;
}

static HRESULT WINAPI XA22_CreateSubmixVoice(IXAudio22 *iface,
        IXAudio2SubmixVoice **ppSubmixVoice, UINT32 inputChannels,
        UINT32 inputSampleRate, UINT32 flags, UINT32 processingStage,
        const XAUDIO23_VOICE_SENDS *pSendList,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    XAUDIO2_VOICE_SENDS sends, *psends = NULL;
    HRESULT hr;

    if(pSendList){
        sends.SendCount = pSendList->OutputCount;
        sends.pSends = convert_send_descriptors23(pSendList);
        psends = &sends;
    }

    hr = IXAudio2_CreateSubmixVoice(&This->IXAudio2_iface, ppSubmixVoice,
            inputChannels, inputSampleRate, flags, processingStage, psends,
            pEffectChain);

    if(pSendList)
        HeapFree(GetProcessHeap(), 0, sends.pSends);

    return hr;
}

static HRESULT WINAPI XA22_CreateMasteringVoice(IXAudio22 *iface,
        IXAudio2MasteringVoice **ppMasteringVoice, UINT32 inputChannels,
        UINT32 inputSampleRate, UINT32 flags, UINT32 deviceIndex,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);

    TRACE("(%p)->(%p, %u, %u, 0x%x, %u, %p)\n", This, ppMasteringVoice,
            inputChannels, inputSampleRate, flags, deviceIndex,
            pEffectChain);

    EnterCriticalSection(&This->lock);

    /* 1 <= XAUDIO2_VER <= 2 */
    *ppMasteringVoice = (IXAudio2MasteringVoice*)&This->mst.IXAudio23MasteringVoice_iface;

    EnterCriticalSection(&This->mst.lock);

    if(This->mst.in_use){
        LeaveCriticalSection(&This->mst.lock);
        LeaveCriticalSection(&This->lock);
        return COMPAT_E_INVALID_CALL;
    }

    LeaveCriticalSection(&This->lock);

    This->mst.effect_chain = wrap_effect_chain(pEffectChain);

    pthread_mutex_lock(&This->mst.engine_lock);

    This->mst.engine_thread = CreateThread(NULL, 0, &engine_thread, &This->mst, 0, NULL);

    pthread_cond_wait(&This->mst.engine_done, &This->mst.engine_lock);

    pthread_mutex_unlock(&This->mst.engine_lock);

    FAudio_SetEngineProcedureEXT(This->faudio, &engine_cb, &This->mst);

    FAudio_CreateMasteringVoice(This->faudio, &This->mst.faudio_voice, inputChannels,
            inputSampleRate, flags, deviceIndex, This->mst.effect_chain);

    This->mst.in_use = TRUE;

    LeaveCriticalSection(&This->mst.lock);

    return S_OK;
}

static HRESULT WINAPI XA22_StartEngine(IXAudio22 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    return IXAudio2_StartEngine(&This->IXAudio2_iface);
}

static void WINAPI XA22_StopEngine(IXAudio22 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    return IXAudio2_StopEngine(&This->IXAudio2_iface);
}

static HRESULT WINAPI XA22_CommitChanges(IXAudio22 *iface, UINT32 operationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    return IXAudio2_CommitChanges(&This->IXAudio2_iface, operationSet);
}

static void WINAPI XA22_GetPerformanceData(IXAudio22 *iface,
        XAUDIO22_PERFORMANCE_DATA *pPerfData)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    XAUDIO2_PERFORMANCE_DATA data;

    IXAudio2_GetPerformanceData(&This->IXAudio2_iface, &data);

    pPerfData->AudioCyclesSinceLastQuery = data.AudioCyclesSinceLastQuery;
    pPerfData->TotalCyclesSinceLastQuery = data.TotalCyclesSinceLastQuery;
    pPerfData->MinimumCyclesPerQuantum = data.MinimumCyclesPerQuantum;
    pPerfData->MaximumCyclesPerQuantum = data.MaximumCyclesPerQuantum;
    pPerfData->MemoryUsageInBytes = data.MemoryUsageInBytes;
    pPerfData->CurrentLatencyInSamples = data.CurrentLatencyInSamples;
    pPerfData->GlitchesSinceEngineStarted = data.GlitchesSinceEngineStarted;
    pPerfData->ActiveSourceVoiceCount = data.ActiveSourceVoiceCount;
    pPerfData->TotalSourceVoiceCount = data.TotalSourceVoiceCount;

    pPerfData->ActiveSubmixVoiceCount = data.ActiveSubmixVoiceCount;
    pPerfData->TotalSubmixVoiceCount = data.ActiveSubmixVoiceCount;

    pPerfData->ActiveXmaSourceVoices = data.ActiveXmaSourceVoices;
    pPerfData->ActiveXmaStreams = data.ActiveXmaStreams;
}

static void WINAPI XA22_SetDebugConfiguration(IXAudio22 *iface,
        const XAUDIO2_DEBUG_CONFIGURATION *pDebugConfiguration,
        void *pReserved)
{
    IXAudio2Impl *This = impl_from_IXAudio22(iface);
    return IXAudio2_SetDebugConfiguration(&This->IXAudio2_iface,
            pDebugConfiguration, pReserved);
}

const IXAudio22Vtbl XAudio22_Vtbl = {
    XA22_QueryInterface,
    XA22_AddRef,
    XA22_Release,
    XA22_GetDeviceCount,
    XA22_GetDeviceDetails,
    XA22_Initialize,
    XA22_RegisterForCallbacks,
    XA22_UnregisterForCallbacks,
    XA22_CreateSourceVoice,
    XA22_CreateSubmixVoice,
    XA22_CreateMasteringVoice,
    XA22_StartEngine,
    XA22_StopEngine,
    XA22_CommitChanges,
    XA22_GetPerformanceData,
    XA22_SetDebugConfiguration
};

#elif XAUDIO2_VER <= 3

static inline IXAudio2Impl *impl_from_IXAudio23(IXAudio23 *iface)
{
    return CONTAINING_RECORD(iface, IXAudio2Impl, IXAudio23_iface);
}

static HRESULT WINAPI XA23_QueryInterface(IXAudio23 *iface, REFIID riid,
        void **ppvObject)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    return IXAudio2_QueryInterface(&This->IXAudio2_iface, riid, ppvObject);
}

static ULONG WINAPI XA23_AddRef(IXAudio23 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    return IXAudio2_AddRef(&This->IXAudio2_iface);
}

static ULONG WINAPI XA23_Release(IXAudio23 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    return IXAudio2_Release(&This->IXAudio2_iface);
}

static HRESULT WINAPI XA23_GetDeviceCount(IXAudio23 *iface, UINT32 *pCount)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    TRACE("%p, %p\n", This, pCount);
    return FAudio_GetDeviceCount(This->faudio, pCount);
}

static HRESULT WINAPI XA23_GetDeviceDetails(IXAudio23 *iface, UINT32 index,
        XAUDIO2_DEVICE_DETAILS *pDeviceDetails)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    TRACE("%p, %u, %p\n", This, index, pDeviceDetails);
    return FAudio_GetDeviceDetails(This->faudio, index, (FAudioDeviceDetails *)pDeviceDetails);
}

static HRESULT WINAPI XA23_Initialize(IXAudio23 *iface, UINT32 flags,
        XAUDIO2_PROCESSOR processor)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    TRACE("(%p)->(0x%x, 0x%x)\n", This, flags, processor);
    return xaudio2_initialize(This, flags, processor);
}

static HRESULT WINAPI XA23_RegisterForCallbacks(IXAudio23 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    return IXAudio2_RegisterForCallbacks(&This->IXAudio2_iface, pCallback);
}

static void WINAPI XA23_UnregisterForCallbacks(IXAudio23 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    IXAudio2_UnregisterForCallbacks(&This->IXAudio2_iface, pCallback);
}

static HRESULT WINAPI XA23_CreateSourceVoice(IXAudio23 *iface,
        IXAudio2SourceVoice **ppSourceVoice, const WAVEFORMATEX *pSourceFormat,
        UINT32 flags, float maxFrequencyRatio,
        IXAudio2VoiceCallback *pCallback, const XAUDIO23_VOICE_SENDS *pSendList,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    XAUDIO2_VOICE_SENDS sends, *psends = NULL;
    HRESULT hr;

    if(pSendList){
        sends.SendCount = pSendList->OutputCount;
        sends.pSends = convert_send_descriptors23(pSendList);
        psends = &sends;
    }

    hr = IXAudio2_CreateSourceVoice(&This->IXAudio2_iface, ppSourceVoice,
            pSourceFormat, flags, maxFrequencyRatio, pCallback, psends,
            pEffectChain);

    if(pSendList)
        HeapFree(GetProcessHeap(), 0, sends.pSends);

    return hr;
}

static HRESULT WINAPI XA23_CreateSubmixVoice(IXAudio23 *iface,
        IXAudio2SubmixVoice **ppSubmixVoice, UINT32 inputChannels,
        UINT32 inputSampleRate, UINT32 flags, UINT32 processingStage,
        const XAUDIO23_VOICE_SENDS *pSendList,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    XAUDIO2_VOICE_SENDS sends, *psends = NULL;
    HRESULT hr;

    if(pSendList){
        sends.SendCount = pSendList->OutputCount;
        sends.pSends = convert_send_descriptors23(pSendList);
        psends = &sends;
    }

    hr = IXAudio2_CreateSubmixVoice(&This->IXAudio2_iface, ppSubmixVoice,
            inputChannels, inputSampleRate, flags, processingStage, psends,
            pEffectChain);

    if(pSendList)
        HeapFree(GetProcessHeap(), 0, sends.pSends);

    return hr;
}

static HRESULT WINAPI XA23_CreateMasteringVoice(IXAudio23 *iface,
        IXAudio2MasteringVoice **ppMasteringVoice, UINT32 inputChannels,
        UINT32 inputSampleRate, UINT32 flags, UINT32 deviceIndex,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);

    TRACE("(%p)->(%p, %u, %u, 0x%x, %u, %p)\n", This, ppMasteringVoice,
            inputChannels, inputSampleRate, flags, deviceIndex,
            pEffectChain);

    EnterCriticalSection(&This->lock);

    /* XAUDIO2_VER == 3 */
    *ppMasteringVoice = (IXAudio2MasteringVoice*)&This->mst.IXAudio23MasteringVoice_iface;

    EnterCriticalSection(&This->mst.lock);

    if(This->mst.in_use){
        LeaveCriticalSection(&This->mst.lock);
        LeaveCriticalSection(&This->lock);
        return COMPAT_E_INVALID_CALL;
    }

    LeaveCriticalSection(&This->lock);

    This->mst.effect_chain = wrap_effect_chain(pEffectChain);

    pthread_mutex_lock(&This->mst.engine_lock);

    This->mst.engine_thread = CreateThread(NULL, 0, &engine_thread, &This->mst, 0, NULL);

    pthread_cond_wait(&This->mst.engine_done, &This->mst.engine_lock);

    pthread_mutex_unlock(&This->mst.engine_lock);

    FAudio_SetEngineProcedureEXT(This->faudio, &engine_cb, &This->mst);

    FAudio_CreateMasteringVoice(This->faudio, &This->mst.faudio_voice, inputChannels,
            inputSampleRate, flags, deviceIndex, This->mst.effect_chain);

    This->mst.in_use = TRUE;

    LeaveCriticalSection(&This->mst.lock);

    return S_OK;
}

static HRESULT WINAPI XA23_StartEngine(IXAudio23 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    return IXAudio2_StartEngine(&This->IXAudio2_iface);
}

static void WINAPI XA23_StopEngine(IXAudio23 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    return IXAudio2_StopEngine(&This->IXAudio2_iface);
}

static HRESULT WINAPI XA23_CommitChanges(IXAudio23 *iface, UINT32 operationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    return IXAudio2_CommitChanges(&This->IXAudio2_iface, operationSet);
}

static void WINAPI XA23_GetPerformanceData(IXAudio23 *iface,
        XAUDIO2_PERFORMANCE_DATA *pPerfData)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    return IXAudio2_GetPerformanceData(&This->IXAudio2_iface, pPerfData);
}

static void WINAPI XA23_SetDebugConfiguration(IXAudio23 *iface,
        const XAUDIO2_DEBUG_CONFIGURATION *pDebugConfiguration,
        void *pReserved)
{
    IXAudio2Impl *This = impl_from_IXAudio23(iface);
    return IXAudio2_SetDebugConfiguration(&This->IXAudio2_iface,
            pDebugConfiguration, pReserved);
}

const IXAudio23Vtbl XAudio23_Vtbl = {
    XA23_QueryInterface,
    XA23_AddRef,
    XA23_Release,
    XA23_GetDeviceCount,
    XA23_GetDeviceDetails,
    XA23_Initialize,
    XA23_RegisterForCallbacks,
    XA23_UnregisterForCallbacks,
    XA23_CreateSourceVoice,
    XA23_CreateSubmixVoice,
    XA23_CreateMasteringVoice,
    XA23_StartEngine,
    XA23_StopEngine,
    XA23_CommitChanges,
    XA23_GetPerformanceData,
    XA23_SetDebugConfiguration
};

#elif XAUDIO2_VER <= 7

static inline IXAudio2Impl *impl_from_IXAudio27(IXAudio27 *iface)
{
    return CONTAINING_RECORD(iface, IXAudio2Impl, IXAudio27_iface);
}

static HRESULT WINAPI XA27_QueryInterface(IXAudio27 *iface, REFIID riid,
        void **ppvObject)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2_QueryInterface(&This->IXAudio2_iface, riid, ppvObject);
}

static ULONG WINAPI XA27_AddRef(IXAudio27 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2_AddRef(&This->IXAudio2_iface);
}

static ULONG WINAPI XA27_Release(IXAudio27 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2_Release(&This->IXAudio2_iface);
}

static HRESULT WINAPI XA27_GetDeviceCount(IXAudio27 *iface, UINT32 *pCount)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    TRACE("%p, %p\n", This, pCount);
    return FAudio_GetDeviceCount(This->faudio, pCount);
}

static HRESULT WINAPI XA27_GetDeviceDetails(IXAudio27 *iface, UINT32 index,
        XAUDIO2_DEVICE_DETAILS *pDeviceDetails)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    TRACE("%p, %u, %p\n", This, index, pDeviceDetails);
    return FAudio_GetDeviceDetails(This->faudio, index, (FAudioDeviceDetails *)pDeviceDetails);
}

static HRESULT WINAPI XA27_Initialize(IXAudio27 *iface, UINT32 flags,
        XAUDIO2_PROCESSOR processor)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    TRACE("(%p)->(0x%x, 0x%x)\n", This, flags, processor);
    return xaudio2_initialize(This, flags, processor);
}

static HRESULT WINAPI XA27_RegisterForCallbacks(IXAudio27 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2_RegisterForCallbacks(&This->IXAudio2_iface, pCallback);
}

static void WINAPI XA27_UnregisterForCallbacks(IXAudio27 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    IXAudio2_UnregisterForCallbacks(&This->IXAudio2_iface, pCallback);
}

static HRESULT WINAPI XA27_CreateSourceVoice(IXAudio27 *iface,
        IXAudio2SourceVoice **ppSourceVoice, const WAVEFORMATEX *pSourceFormat,
        UINT32 flags, float maxFrequencyRatio,
        IXAudio2VoiceCallback *pCallback, const XAUDIO2_VOICE_SENDS *pSendList,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2_CreateSourceVoice(&This->IXAudio2_iface, ppSourceVoice,
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
    return IXAudio2_CreateSubmixVoice(&This->IXAudio2_iface, ppSubmixVoice,
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

    EnterCriticalSection(&This->lock);

    /* 4 <= XAUDIO2_VER <= 7 */
    *ppMasteringVoice = (IXAudio2MasteringVoice*)&This->mst.IXAudio27MasteringVoice_iface;

    EnterCriticalSection(&This->mst.lock);

    if(This->mst.in_use){
        LeaveCriticalSection(&This->mst.lock);
        LeaveCriticalSection(&This->lock);
        return COMPAT_E_INVALID_CALL;
    }

    LeaveCriticalSection(&This->lock);

    This->mst.effect_chain = wrap_effect_chain(pEffectChain);

    pthread_mutex_lock(&This->mst.engine_lock);

    This->mst.engine_thread = CreateThread(NULL, 0, &engine_thread, &This->mst, 0, NULL);

    pthread_cond_wait(&This->mst.engine_done, &This->mst.engine_lock);

    pthread_mutex_unlock(&This->mst.engine_lock);

    FAudio_SetEngineProcedureEXT(This->faudio, &engine_cb, &This->mst);

    FAudio_CreateMasteringVoice(This->faudio, &This->mst.faudio_voice, inputChannels,
            inputSampleRate, flags, deviceIndex, This->mst.effect_chain);

    This->mst.in_use = TRUE;

    LeaveCriticalSection(&This->mst.lock);

    return S_OK;
}

static HRESULT WINAPI XA27_StartEngine(IXAudio27 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2_StartEngine(&This->IXAudio2_iface);
}

static void WINAPI XA27_StopEngine(IXAudio27 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2_StopEngine(&This->IXAudio2_iface);
}

static HRESULT WINAPI XA27_CommitChanges(IXAudio27 *iface, UINT32 operationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2_CommitChanges(&This->IXAudio2_iface, operationSet);
}

static void WINAPI XA27_GetPerformanceData(IXAudio27 *iface,
        XAUDIO2_PERFORMANCE_DATA *pPerfData)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2_GetPerformanceData(&This->IXAudio2_iface, pPerfData);
}

static void WINAPI XA27_SetDebugConfiguration(IXAudio27 *iface,
        const XAUDIO2_DEBUG_CONFIGURATION *pDebugConfiguration,
        void *pReserved)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    return IXAudio2_SetDebugConfiguration(&This->IXAudio2_iface,
            pDebugConfiguration, pReserved);
}

const IXAudio27Vtbl XAudio27_Vtbl = {
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
#endif
/* END IXAudio2 */
