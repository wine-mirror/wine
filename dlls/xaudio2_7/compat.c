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
 */

#include "config.h"

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS

#include <stdarg.h>

#include "xaudio_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xaudio2);

XA2SourceImpl *impl_from_IXAudio27SourceVoice(IXAudio27SourceVoice *iface)
{
    return CONTAINING_RECORD(iface, XA2SourceImpl, IXAudio27SourceVoice_iface);
}

static void WINAPI XA27SRC_GetVoiceDetails(IXAudio27SourceVoice *iface,
        XAUDIO2_VOICE_DETAILS *pVoiceDetails)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_GetVoiceDetails(&This->IXAudio2SourceVoice_iface, pVoiceDetails);
}

static HRESULT WINAPI XA27SRC_SetOutputVoices(IXAudio27SourceVoice *iface,
        const XAUDIO2_VOICE_SENDS *pSendList)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetOutputVoices(&This->IXAudio2SourceVoice_iface, pSendList);
}

static HRESULT WINAPI XA27SRC_SetEffectChain(IXAudio27SourceVoice *iface,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetEffectChain(&This->IXAudio2SourceVoice_iface, pEffectChain);
}

static HRESULT WINAPI XA27SRC_EnableEffect(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_EnableEffect(&This->IXAudio2SourceVoice_iface, EffectIndex, OperationSet);
}

static HRESULT WINAPI XA27SRC_DisableEffect(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_DisableEffect(&This->IXAudio2SourceVoice_iface, EffectIndex, OperationSet);
}

static void WINAPI XA27SRC_GetEffectState(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, BOOL *pEnabled)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_GetEffectState(&This->IXAudio2SourceVoice_iface, EffectIndex, pEnabled);
}

static HRESULT WINAPI XA27SRC_SetEffectParameters(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, const void *pParameters, UINT32 ParametersByteSize,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetEffectParameters(&This->IXAudio2SourceVoice_iface,
            EffectIndex, pParameters, ParametersByteSize, OperationSet);
}

static HRESULT WINAPI XA27SRC_GetEffectParameters(IXAudio27SourceVoice *iface,
        UINT32 EffectIndex, void *pParameters, UINT32 ParametersByteSize)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_GetEffectParameters(&This->IXAudio2SourceVoice_iface,
            EffectIndex, pParameters, ParametersByteSize);
}

static HRESULT WINAPI XA27SRC_SetFilterParameters(IXAudio27SourceVoice *iface,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetFilterParameters(&This->IXAudio2SourceVoice_iface,
            pParameters, OperationSet);
}

static void WINAPI XA27SRC_GetFilterParameters(IXAudio27SourceVoice *iface,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_GetFilterParameters(&This->IXAudio2SourceVoice_iface, pParameters);
}

static HRESULT WINAPI XA27SRC_SetOutputFilterParameters(IXAudio27SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        const XAUDIO2_FILTER_PARAMETERS *pParameters, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetOutputFilterParameters(&This->IXAudio2SourceVoice_iface,
            pDestinationVoice, pParameters, OperationSet);
}

static void WINAPI XA27SRC_GetOutputFilterParameters(IXAudio27SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice,
        XAUDIO2_FILTER_PARAMETERS *pParameters)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_GetOutputFilterParameters(&This->IXAudio2SourceVoice_iface,
            pDestinationVoice, pParameters);
}

static HRESULT WINAPI XA27SRC_SetVolume(IXAudio27SourceVoice *iface, float Volume,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetVolume(&This->IXAudio2SourceVoice_iface, Volume,
            OperationSet);
}

static void WINAPI XA27SRC_GetVolume(IXAudio27SourceVoice *iface, float *pVolume)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_GetVolume(&This->IXAudio2SourceVoice_iface, pVolume);
}

static HRESULT WINAPI XA27SRC_SetChannelVolumes(IXAudio27SourceVoice *iface,
        UINT32 Channels, const float *pVolumes, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetChannelVolumes(&This->IXAudio2SourceVoice_iface, Channels,
            pVolumes, OperationSet);
}

static void WINAPI XA27SRC_GetChannelVolumes(IXAudio27SourceVoice *iface,
        UINT32 Channels, float *pVolumes)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_GetChannelVolumes(&This->IXAudio2SourceVoice_iface, Channels,
            pVolumes);
}

static HRESULT WINAPI XA27SRC_SetOutputMatrix(IXAudio27SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, const float *pLevelMatrix,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetOutputMatrix(&This->IXAudio2SourceVoice_iface,
            pDestinationVoice, SourceChannels, DestinationChannels,
            pLevelMatrix, OperationSet);
}

static void WINAPI XA27SRC_GetOutputMatrix(IXAudio27SourceVoice *iface,
        IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels,
        UINT32 DestinationChannels, float *pLevelMatrix)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_GetOutputMatrix(&This->IXAudio2SourceVoice_iface, pDestinationVoice,
            SourceChannels, DestinationChannels, pLevelMatrix);
}

static void WINAPI XA27SRC_DestroyVoice(IXAudio27SourceVoice *iface)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    IXAudio2SourceVoice_DestroyVoice(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA27SRC_Start(IXAudio27SourceVoice *iface, UINT32 Flags,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_Start(&This->IXAudio2SourceVoice_iface, Flags, OperationSet);
}

static HRESULT WINAPI XA27SRC_Stop(IXAudio27SourceVoice *iface, UINT32 Flags,
        UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_Stop(&This->IXAudio2SourceVoice_iface, Flags, OperationSet);
}

static HRESULT WINAPI XA27SRC_SubmitSourceBuffer(IXAudio27SourceVoice *iface,
        const XAUDIO2_BUFFER *pBuffer, const XAUDIO2_BUFFER_WMA *pBufferWMA)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SubmitSourceBuffer(&This->IXAudio2SourceVoice_iface, pBuffer,
            pBufferWMA);
}

static HRESULT WINAPI XA27SRC_FlushSourceBuffers(IXAudio27SourceVoice *iface)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_FlushSourceBuffers(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA27SRC_Discontinuity(IXAudio27SourceVoice *iface)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_Discontinuity(&This->IXAudio2SourceVoice_iface);
}

static HRESULT WINAPI XA27SRC_ExitLoop(IXAudio27SourceVoice *iface, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_ExitLoop(&This->IXAudio2SourceVoice_iface, OperationSet);
}

static void WINAPI XA27SRC_GetState(IXAudio27SourceVoice *iface,
        XAUDIO2_VOICE_STATE *pVoiceState)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_GetState(&This->IXAudio2SourceVoice_iface, pVoiceState, 0);
}

static HRESULT WINAPI XA27SRC_SetFrequencyRatio(IXAudio27SourceVoice *iface,
        float Ratio, UINT32 OperationSet)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_SetFrequencyRatio(&This->IXAudio2SourceVoice_iface, Ratio, OperationSet);
}

static void WINAPI XA27SRC_GetFrequencyRatio(IXAudio27SourceVoice *iface, float *pRatio)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
    return IXAudio2SourceVoice_GetFrequencyRatio(&This->IXAudio2SourceVoice_iface, pRatio);
}

static HRESULT WINAPI XA27SRC_SetSourceSampleRate(
    IXAudio27SourceVoice *iface,
    UINT32 NewSourceSampleRate)
{
    XA2SourceImpl *This = impl_from_IXAudio27SourceVoice(iface);
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

    if(deviceIndex >= This->ndevs)
        return E_INVALIDARG;

    return IXAudio2_CreateMasteringVoice(&This->IXAudio2_iface, ppMasteringVoice,
            inputChannels, inputSampleRate, flags, This->devids[deviceIndex],
            pEffectChain, AudioCategory_GameEffects);
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
