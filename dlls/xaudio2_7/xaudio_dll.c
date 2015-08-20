/*
 * Copyright (c) 2015 Mark Harmstone
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
#include "winuser.h"

#include "ole2.h"
#include "rpcproxy.h"

#include "wine/debug.h"
#include <propsys.h>
#include "initguid.h"

#include "xaudio2.h"

WINE_DEFAULT_DEBUG_CHANNEL(xaudio2);

static HINSTANCE instance;

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

typedef struct {
    IXAudio27 IXAudio27_iface;
    IXAudio2 IXAudio2_iface;
    LONG ref;

    DWORD version;
} IXAudio2Impl;

static inline IXAudio2Impl *impl_from_IXAudio2(IXAudio2 *iface)
{
    return CONTAINING_RECORD(iface, IXAudio2Impl, IXAudio2_iface);
}

static inline IXAudio2Impl *impl_from_IXAudio27(IXAudio27 *iface)
{
    return CONTAINING_RECORD(iface, IXAudio2Impl, IXAudio27_iface);
}

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

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI IXAudio2Impl_RegisterForCallbacks(IXAudio2 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p): stub!\n", This, pCallback);

    return E_NOTIMPL;
}

static void WINAPI IXAudio2Impl_UnregisterForCallbacks(IXAudio2 *iface,
        IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p): stub!\n", This, pCallback);
}

static HRESULT WINAPI IXAudio2Impl_CreateSourceVoice(IXAudio2 *iface,
        IXAudio2SourceVoice **ppSourceVoice, const WAVEFORMATEX *pSourceFormat,
        UINT32 flags, float maxFrequencyRatio,
        IXAudio2VoiceCallback *pCallback, const XAUDIO2_VOICE_SENDS *pSendList,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p, %p, 0x%x, %f, %p, %p, %p): stub!\n", This, ppSourceVoice,
            pSourceFormat, flags, maxFrequencyRatio, pCallback, pSendList,
            pEffectChain);

    return E_NOTIMPL;
}

static HRESULT WINAPI IXAudio2Impl_CreateSubmixVoice(IXAudio2 *iface,
        IXAudio2SubmixVoice **ppSubmixVoice, UINT32 inputChannels,
        UINT32 inputSampleRate, UINT32 flags, UINT32 processingStage,
        const XAUDIO2_VOICE_SENDS *pSendList,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p, %u, %u, 0x%x, %u, %p, %p): stub!\n", This, ppSubmixVoice,
            inputChannels, inputSampleRate, flags, processingStage, pSendList,
            pEffectChain);

    return E_NOTIMPL;
}

static HRESULT WINAPI IXAudio2Impl_CreateMasteringVoice(IXAudio2 *iface,
        IXAudio2MasteringVoice **ppMasteringVoice, UINT32 inputChannels,
        UINT32 inputSampleRate, UINT32 flags, const WCHAR *deviceId,
        const XAUDIO2_EFFECT_CHAIN *pEffectChain,
        AUDIO_STREAM_CATEGORY streamCategory)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p, %u, %u, 0x%x, %s, %p, 0x%x): stub!\n", This,
            ppMasteringVoice, inputChannels, inputSampleRate, flags,
            wine_dbgstr_w(deviceId), pEffectChain, streamCategory);

    return E_NOTIMPL;
}

static HRESULT WINAPI IXAudio2Impl_StartEngine(IXAudio2 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(): stub!\n", This);

    return E_NOTIMPL;
}

static void WINAPI IXAudio2Impl_StopEngine(IXAudio2 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(): stub!\n", This);
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
    TRACE("(%p)->(%p)\n", This, pCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI XA27_GetDeviceDetails(IXAudio27 *iface, UINT32 index,
        XAUDIO2_DEVICE_DETAILS *pDeviceDetails)
{
    IXAudio2Impl *This = impl_from_IXAudio27(iface);
    TRACE("(%p)->(%u, %p)\n", This, index, pDeviceDetails);
    return E_NOTIMPL;
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
    /* TODO: Convert DeviceIndex to DeviceId */
    return IXAudio2Impl_CreateMasteringVoice(&This->IXAudio2_iface,
            ppMasteringVoice, inputChannels, inputSampleRate, flags, 0,
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

    if(IsEqualGUID(riid, &IID_IXAudio27))
        object->version = 27;
    else
        object->version = 28;

    hr = IXAudio2_QueryInterface(&object->IXAudio2_iface, riid, ppobj);
    if(FAILED(hr))
        HeapFree(GetProcessHeap(), 0, object);
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
