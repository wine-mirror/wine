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
    IClassFactory IClassFactory_iface;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, IUnknown **ppObj);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

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
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    HRESULT hr;
    IUnknown *punk;

    TRACE("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);

    *ppobj = NULL;
    hr = This->pfnCreateInstance(pOuter, &punk);
    if (FAILED(hr))
        return hr;

    hr = IUnknown_QueryInterface(punk, riid, ppobj);
    IUnknown_Release(punk);
    return hr;
}

static HRESULT WINAPI XAudio2CF_LockServer(IClassFactory *iface, BOOL dolock)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    FIXME("(%p)->(%d): stub!\n", This, dolock);
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

typedef struct {
    IXAudio2 IXAudio2Impl_iface;
    LONG ref;
} IXAudio2Impl;

static const struct IXAudio2Vtbl XAudio2_Vtbl;

HRESULT XAudio2_create(IUnknown *pUnkOuter, IUnknown **ppObj)
{
    IXAudio2Impl *object;

    TRACE("(%p, %p)\n", pUnkOuter, ppObj);

    if(pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IXAudio2Impl));
    if(!object)
        return E_OUTOFMEMORY;

    object->IXAudio2Impl_iface.lpVtbl = &XAudio2_Vtbl;
    object->ref = 1;

    *ppObj = (IUnknown *)&object->IXAudio2Impl_iface;

    return S_OK;
}

static IClassFactoryImpl xaudio2_cf = { { &XAudio2CF_Vtbl }, XAudio2_create };

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    IClassFactory *factory = NULL;

    TRACE("(%s, %s, %p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if(IsEqualGUID(rclsid, &CLSID_XAudio2)) {
        factory = &xaudio2_cf.IClassFactory_iface;
    }
    if(!factory) return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface(factory, riid, ppv);
}

/*** IUnknown methods ***/
static inline IXAudio2Impl *impl_from_IXAudio2(IXAudio2 *iface)
{
    return CONTAINING_RECORD(iface, IXAudio2Impl, IXAudio2Impl_iface);
}

static HRESULT WINAPI IXAudio2Impl_QueryInterface(IXAudio2 *iface, REFIID riid, void **ppvObject)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IXAudio2))
    {
        IXAudio2_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }

    FIXME("(%p)->(%s,%p), not found\n", This,debugstr_guid(riid), ppvObject);

    return E_NOINTERFACE;
}

static ULONG WINAPI IXAudio2Impl_AddRef(IXAudio2 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    TRACE("(%p)->()\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IXAudio2Impl_Release(IXAudio2 *iface)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->()\n", This);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IXAudio2 methods ***/
static HRESULT WINAPI IXAudio2Impl_GetDeviceCount(IXAudio2 *iface, UINT32 *pCount)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p): stub!\n", This, pCount);

    return E_NOTIMPL;
}

static HRESULT WINAPI IXAudio2Impl_GetDeviceDetails(IXAudio2 *iface, UINT32 index, XAUDIO2_DEVICE_DETAILS *pDeviceDetails)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%u, %p): stub!\n", This, index, pDeviceDetails);

    return E_NOTIMPL;
}

static HRESULT WINAPI IXAudio2Impl_Initialize(IXAudio2 *iface, UINT32 flags, XAUDIO2_PROCESSOR processor)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%u, %u): stub!\n", This, flags, processor);

    return S_OK;
}

static HRESULT WINAPI IXAudio2Impl_RegisterForCallbacks(IXAudio2 *iface, IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p): stub!\n", This, pCallback);

    return E_NOTIMPL;
}

static void WINAPI IXAudio2Impl_UnregisterForCallbacks(IXAudio2 *iface, IXAudio2EngineCallback *pCallback)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p): stub!\n", This, pCallback);
}

static HRESULT WINAPI IXAudio2Impl_CreateSourceVoice(IXAudio2 *iface, IXAudio2SourceVoice **ppSourceVoice, const WAVEFORMATEX *pSourceFormat, UINT32 flags, float maxFrequencyRatio, IXAudio2VoiceCallback *pCallback, const XAUDIO2_VOICE_SENDS *pSendList, const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p, %p, %u, %f, %p, %p, %p): stub!\n", This, ppSourceVoice, pSourceFormat, flags, maxFrequencyRatio, pCallback, pSendList, pEffectChain);

    return E_NOTIMPL;
}

static HRESULT WINAPI IXAudio2Impl_CreateSubmixVoice(IXAudio2 *iface, IXAudio2SubmixVoice **ppSubmixVoice, UINT32 inputChannels, UINT32 inputSampleRate, UINT32 flags, UINT32 processingStage, const XAUDIO2_VOICE_SENDS *pSendList, const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p, %u, %u, %u, %u, %p, %p): stub!\n", This, ppSubmixVoice, inputChannels, inputSampleRate, flags, processingStage, pSendList, pEffectChain);

    return E_NOTIMPL;
}

static HRESULT WINAPI IXAudio2Impl_CreateMasteringVoice(IXAudio2 *iface, IXAudio2MasteringVoice **ppMasteringVoice, UINT32 inputChannels, UINT32 inputSampleRate, UINT32 flags, UINT32 deviceIndex, const XAUDIO2_EFFECT_CHAIN *pEffectChain)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p, %u, %u, %u, %u, %p): stub!\n", This, ppMasteringVoice, inputChannels, inputSampleRate, flags, deviceIndex, pEffectChain);

    return S_OK;
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

static HRESULT WINAPI IXAudio2Impl_CommitChanges(IXAudio2 *iface, UINT32 operationSet)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%u): stub!\n", This, operationSet);

    return E_NOTIMPL;
}

static void WINAPI IXAudio2Impl_GetPerformanceData(IXAudio2 *iface, XAUDIO2_PERFORMANCE_DATA *pPerfData)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p): stub!\n", This, pPerfData);
}

static void WINAPI IXAudio2Impl_SetDebugConfiguration(IXAudio2 *iface, const XAUDIO2_DEBUG_CONFIGURATION *pDebugConfiguration, void *pReserved)
{
    IXAudio2Impl *This = impl_from_IXAudio2(iface);

    FIXME("(%p)->(%p, %p): stub!\n", This, pDebugConfiguration, pReserved);
}

static const IXAudio2Vtbl XAudio2_Vtbl =
{
    IXAudio2Impl_QueryInterface,
    IXAudio2Impl_AddRef,
    IXAudio2Impl_Release,
    IXAudio2Impl_GetDeviceCount,
    IXAudio2Impl_GetDeviceDetails,
    IXAudio2Impl_Initialize,
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
