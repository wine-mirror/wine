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
 */

#include "config.h"

#include <stdarg.h>

#define NONAMELESSUNION
#define COBJMACROS

#include "xaudio_private.h"
#include "xapofx.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xaudio2);

typedef struct _VUMeterImpl {
    IXAPO IXAPO_iface;
    IXAPOParameters IXAPOParameters_iface;

    LONG ref;

    DWORD version;
} VUMeterImpl;

static VUMeterImpl *VUMeterImpl_from_IXAPO(IXAPO *iface)
{
    return CONTAINING_RECORD(iface, VUMeterImpl, IXAPO_iface);
}

static VUMeterImpl *VUMeterImpl_from_IXAPOParameters(IXAPOParameters *iface)
{
    return CONTAINING_RECORD(iface, VUMeterImpl, IXAPOParameters_iface);
}

static HRESULT WINAPI VUMXAPO_QueryInterface(IXAPO *iface, REFIID riid,
        void **ppvObject)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPO(iface);

    TRACE("%p, %s, %p\n", This, wine_dbgstr_guid(riid), ppvObject);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IXAPO) ||
            IsEqualGUID(riid, &IID_IXAPO27))
        *ppvObject = &This->IXAPO_iface;
    else if(IsEqualGUID(riid, &IID_IXAPOParameters))
        *ppvObject = &This->IXAPOParameters_iface;
    else
        *ppvObject = NULL;

    if(*ppvObject){
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI VUMXAPO_AddRef(IXAPO *iface)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPO(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(): Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI VUMXAPO_Release(IXAPO *iface)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPO(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Refcount now %u\n", This, ref);

    if(!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI VUMXAPO_GetRegistrationProperties(IXAPO *iface,
    XAPO_REGISTRATION_PROPERTIES **props)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPO(iface);
    TRACE("%p, %p\n", This, props);
    /* TODO: check for version == 20 and use XAPO20_REGISTRATION_PROPERTIES */
    return E_NOTIMPL;
}

static HRESULT WINAPI VUMXAPO_IsInputFormatSupported(IXAPO *iface,
        const WAVEFORMATEX *output_fmt, const WAVEFORMATEX *input_fmt,
        WAVEFORMATEX **supported_fmt)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPO(iface);
    TRACE("%p, %p, %p, %p\n", This, output_fmt, input_fmt, supported_fmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI VUMXAPO_IsOutputFormatSupported(IXAPO *iface,
        const WAVEFORMATEX *input_fmt, const WAVEFORMATEX *output_fmt,
        WAVEFORMATEX **supported_fmt)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPO(iface);
    TRACE("%p, %p, %p, %p\n", This, input_fmt, output_fmt, supported_fmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI VUMXAPO_Initialize(IXAPO *iface, const void *data,
        UINT32 data_len)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPO(iface);
    TRACE("%p, %p, %u\n", This, data, data_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI VUMXAPO_Reset(IXAPO *iface)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPO(iface);
    TRACE("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VUMXAPO_LockForProcess(IXAPO *iface,
        UINT32 in_params_count,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS *in_params,
        UINT32 out_params_count,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS *out_params)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPO(iface);
    TRACE("%p, %u, %p, %u, %p\n", This, in_params_count, in_params,
            out_params_count, out_params);
    return E_NOTIMPL;
}

static void WINAPI VUMXAPO_UnlockForProcess(IXAPO *iface)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPO(iface);
    TRACE("%p\n", This);
}

static void WINAPI VUMXAPO_Process(IXAPO *iface, UINT32 in_params_count,
        const XAPO_PROCESS_BUFFER_PARAMETERS *in_params,
        UINT32 out_params_count,
        const XAPO_PROCESS_BUFFER_PARAMETERS *out_params, BOOL enabled)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPO(iface);
    TRACE("%p, %u, %p, %u, %p, %u\n", This, in_params_count, in_params,
            out_params_count, out_params, enabled);
}

static UINT32 WINAPI VUMXAPO_CalcInputFrames(IXAPO *iface, UINT32 output_frames)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPO(iface);
    TRACE("%p, %u\n", This, output_frames);
    return 0;
}

static UINT32 WINAPI VUMXAPO_CalcOutputFrames(IXAPO *iface, UINT32 input_frames)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPO(iface);
    TRACE("%p, %u\n", This, input_frames);
    return 0;
}

static const IXAPOVtbl VUMXAPO_Vtbl = {
    VUMXAPO_QueryInterface,
    VUMXAPO_AddRef,
    VUMXAPO_Release,
    VUMXAPO_GetRegistrationProperties,
    VUMXAPO_IsInputFormatSupported,
    VUMXAPO_IsOutputFormatSupported,
    VUMXAPO_Initialize,
    VUMXAPO_Reset,
    VUMXAPO_LockForProcess,
    VUMXAPO_UnlockForProcess,
    VUMXAPO_Process,
    VUMXAPO_CalcInputFrames,
    VUMXAPO_CalcOutputFrames
};

static HRESULT WINAPI VUMXAPOParams_QueryInterface(IXAPOParameters *iface,
        REFIID riid, void **ppvObject)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPOParameters(iface);
    return VUMXAPO_QueryInterface(&This->IXAPO_iface, riid, ppvObject);
}

static ULONG WINAPI VUMXAPOParams_AddRef(IXAPOParameters *iface)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPOParameters(iface);
    return VUMXAPO_AddRef(&This->IXAPO_iface);
}

static ULONG WINAPI VUMXAPOParams_Release(IXAPOParameters *iface)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPOParameters(iface);
    return VUMXAPO_Release(&This->IXAPO_iface);
}

static void WINAPI VUMXAPOParams_SetParameters(IXAPOParameters *iface,
        const void *params, UINT32 params_len)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPOParameters(iface);
    TRACE("%p, %p, %u\n", This, params, params_len);
}

static void WINAPI VUMXAPOParams_GetParameters(IXAPOParameters *iface,
        void *params, UINT32 params_len)
{
    VUMeterImpl *This = VUMeterImpl_from_IXAPOParameters(iface);
    TRACE("%p, %p, %u\n", This, params, params_len);
}

static const IXAPOParametersVtbl VUMXAPOParameters_Vtbl = {
    VUMXAPOParams_QueryInterface,
    VUMXAPOParams_AddRef,
    VUMXAPOParams_Release,
    VUMXAPOParams_SetParameters,
    VUMXAPOParams_GetParameters
};

typedef struct _ReverbImpl {
    IXAPO IXAPO_iface;
    IXAPOParameters IXAPOParameters_iface;

    LONG ref;

    DWORD version;
} ReverbImpl;

static ReverbImpl *ReverbImpl_from_IXAPO(IXAPO *iface)
{
    return CONTAINING_RECORD(iface, ReverbImpl, IXAPO_iface);
}

static ReverbImpl *ReverbImpl_from_IXAPOParameters(IXAPOParameters *iface)
{
    return CONTAINING_RECORD(iface, ReverbImpl, IXAPOParameters_iface);
}

static HRESULT WINAPI RVBXAPO_QueryInterface(IXAPO *iface, REFIID riid, void **ppvObject)
{
    ReverbImpl *This = ReverbImpl_from_IXAPO(iface);

    TRACE("%p, %s, %p\n", This, wine_dbgstr_guid(riid), ppvObject);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IXAPO) ||
            IsEqualGUID(riid, &IID_IXAPO27))
        *ppvObject = &This->IXAPO_iface;
    else if(IsEqualGUID(riid, &IID_IXAPOParameters))
        *ppvObject = &This->IXAPOParameters_iface;
    else
        *ppvObject = NULL;

    if(*ppvObject){
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI RVBXAPO_AddRef(IXAPO *iface)
{
    ReverbImpl *This = ReverbImpl_from_IXAPO(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(): Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI RVBXAPO_Release(IXAPO *iface)
{
    ReverbImpl *This = ReverbImpl_from_IXAPO(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Refcount now %u\n", This, ref);

    if(!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI RVBXAPO_GetRegistrationProperties(IXAPO *iface,
    XAPO_REGISTRATION_PROPERTIES **props)
{
    ReverbImpl *This = ReverbImpl_from_IXAPO(iface);
    TRACE("%p, %p\n", This, props);
    /* TODO: check for version == 20 and use XAPO20_REGISTRATION_PROPERTIES */
    return E_NOTIMPL;
}

static HRESULT WINAPI RVBXAPO_IsInputFormatSupported(IXAPO *iface,
        const WAVEFORMATEX *output_fmt, const WAVEFORMATEX *input_fmt,
        WAVEFORMATEX **supported_fmt)
{
    ReverbImpl *This = ReverbImpl_from_IXAPO(iface);
    TRACE("%p, %p, %p, %p\n", This, output_fmt, input_fmt, supported_fmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI RVBXAPO_IsOutputFormatSupported(IXAPO *iface,
        const WAVEFORMATEX *input_fmt, const WAVEFORMATEX *output_fmt,
        WAVEFORMATEX **supported_fmt)
{
    ReverbImpl *This = ReverbImpl_from_IXAPO(iface);
    TRACE("%p, %p, %p, %p\n", This, input_fmt, output_fmt, supported_fmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI RVBXAPO_Initialize(IXAPO *iface, const void *data,
        UINT32 data_len)
{
    ReverbImpl *This = ReverbImpl_from_IXAPO(iface);
    TRACE("%p, %p, %u\n", This, data, data_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI RVBXAPO_Reset(IXAPO *iface)
{
    ReverbImpl *This = ReverbImpl_from_IXAPO(iface);
    TRACE("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI RVBXAPO_LockForProcess(IXAPO *iface, UINT32 in_params_count,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS *in_params,
        UINT32 out_params_count,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS *out_params)
{
    ReverbImpl *This = ReverbImpl_from_IXAPO(iface);
    TRACE("%p, %u, %p, %u, %p\n", This, in_params_count, in_params,
            out_params_count, out_params);
    return E_NOTIMPL;
}

static void WINAPI RVBXAPO_UnlockForProcess(IXAPO *iface)
{
    ReverbImpl *This = ReverbImpl_from_IXAPO(iface);
    TRACE("%p\n", This);
}

static void WINAPI RVBXAPO_Process(IXAPO *iface, UINT32 in_params_count,
        const XAPO_PROCESS_BUFFER_PARAMETERS *in_params,
        UINT32 out_params_count,
        const XAPO_PROCESS_BUFFER_PARAMETERS *out_params, BOOL enabled)
{
    ReverbImpl *This = ReverbImpl_from_IXAPO(iface);
    TRACE("%p, %u, %p, %u, %p, %u\n", This, in_params_count, in_params,
            out_params_count, out_params, enabled);
}

static UINT32 WINAPI RVBXAPO_CalcInputFrames(IXAPO *iface, UINT32 output_frames)
{
    ReverbImpl *This = ReverbImpl_from_IXAPO(iface);
    TRACE("%p, %u\n", This, output_frames);
    return 0;
}

static UINT32 WINAPI RVBXAPO_CalcOutputFrames(IXAPO *iface, UINT32 input_frames)
{
    ReverbImpl *This = ReverbImpl_from_IXAPO(iface);
    TRACE("%p, %u\n", This, input_frames);
    return 0;
}

static const IXAPOVtbl RVBXAPO_Vtbl = {
    RVBXAPO_QueryInterface,
    RVBXAPO_AddRef,
    RVBXAPO_Release,
    RVBXAPO_GetRegistrationProperties,
    RVBXAPO_IsInputFormatSupported,
    RVBXAPO_IsOutputFormatSupported,
    RVBXAPO_Initialize,
    RVBXAPO_Reset,
    RVBXAPO_LockForProcess,
    RVBXAPO_UnlockForProcess,
    RVBXAPO_Process,
    RVBXAPO_CalcInputFrames,
    RVBXAPO_CalcOutputFrames
};

static HRESULT WINAPI RVBXAPOParams_QueryInterface(IXAPOParameters *iface,
        REFIID riid, void **ppvObject)
{
    ReverbImpl *This = ReverbImpl_from_IXAPOParameters(iface);
    return RVBXAPO_QueryInterface(&This->IXAPO_iface, riid, ppvObject);
}

static ULONG WINAPI RVBXAPOParams_AddRef(IXAPOParameters *iface)
{
    ReverbImpl *This = ReverbImpl_from_IXAPOParameters(iface);
    return RVBXAPO_AddRef(&This->IXAPO_iface);
}

static ULONG WINAPI RVBXAPOParams_Release(IXAPOParameters *iface)
{
    ReverbImpl *This = ReverbImpl_from_IXAPOParameters(iface);
    return RVBXAPO_Release(&This->IXAPO_iface);
}

static void WINAPI RVBXAPOParams_SetParameters(IXAPOParameters *iface,
        const void *params, UINT32 params_len)
{
    ReverbImpl *This = ReverbImpl_from_IXAPOParameters(iface);
    TRACE("%p, %p, %u\n", This, params, params_len);
}

static void WINAPI RVBXAPOParams_GetParameters(IXAPOParameters *iface, void *params,
        UINT32 params_len)
{
    ReverbImpl *This = ReverbImpl_from_IXAPOParameters(iface);
    TRACE("%p, %p, %u\n", This, params, params_len);
}

static const IXAPOParametersVtbl RVBXAPOParameters_Vtbl = {
    RVBXAPOParams_QueryInterface,
    RVBXAPOParams_AddRef,
    RVBXAPOParams_Release,
    RVBXAPOParams_SetParameters,
    RVBXAPOParams_GetParameters
};

typedef struct _EQImpl {
    IXAPO IXAPO_iface;
    IXAPOParameters IXAPOParameters_iface;

    LONG ref;

    DWORD version;
} EQImpl;

static EQImpl *EQImpl_from_IXAPO(IXAPO *iface)
{
    return CONTAINING_RECORD(iface, EQImpl, IXAPO_iface);
}

static EQImpl *EQImpl_from_IXAPOParameters(IXAPOParameters *iface)
{
    return CONTAINING_RECORD(iface, EQImpl, IXAPOParameters_iface);
}

static HRESULT WINAPI EQXAPO_QueryInterface(IXAPO *iface, REFIID riid, void **ppvObject)
{
    EQImpl *This = EQImpl_from_IXAPO(iface);

    TRACE("%p, %s, %p\n", This, wine_dbgstr_guid(riid), ppvObject);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IXAPO) ||
            IsEqualGUID(riid, &IID_IXAPO27))
        *ppvObject = &This->IXAPO_iface;
    else if(IsEqualGUID(riid, &IID_IXAPOParameters))
        *ppvObject = &This->IXAPOParameters_iface;
    else
        *ppvObject = NULL;

    if(*ppvObject){
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI EQXAPO_AddRef(IXAPO *iface)
{
    EQImpl *This = EQImpl_from_IXAPO(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(): Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI EQXAPO_Release(IXAPO *iface)
{
    EQImpl *This = EQImpl_from_IXAPO(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Refcount now %u\n", This, ref);

    if(!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI EQXAPO_GetRegistrationProperties(IXAPO *iface,
    XAPO_REGISTRATION_PROPERTIES **props)
{
    EQImpl *This = EQImpl_from_IXAPO(iface);
    TRACE("%p, %p\n", This, props);
    /* TODO: check for version == 20 and use XAPO20_REGISTRATION_PROPERTIES */
    return E_NOTIMPL;
}

static HRESULT WINAPI EQXAPO_IsInputFormatSupported(IXAPO *iface,
        const WAVEFORMATEX *output_fmt, const WAVEFORMATEX *input_fmt,
        WAVEFORMATEX **supported_fmt)
{
    EQImpl *This = EQImpl_from_IXAPO(iface);
    TRACE("%p, %p, %p, %p\n", This, output_fmt, input_fmt, supported_fmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI EQXAPO_IsOutputFormatSupported(IXAPO *iface,
        const WAVEFORMATEX *input_fmt, const WAVEFORMATEX *output_fmt,
        WAVEFORMATEX **supported_fmt)
{
    EQImpl *This = EQImpl_from_IXAPO(iface);
    TRACE("%p, %p, %p, %p\n", This, input_fmt, output_fmt, supported_fmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI EQXAPO_Initialize(IXAPO *iface, const void *data,
        UINT32 data_len)
{
    EQImpl *This = EQImpl_from_IXAPO(iface);
    TRACE("%p, %p, %u\n", This, data, data_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI EQXAPO_Reset(IXAPO *iface)
{
    EQImpl *This = EQImpl_from_IXAPO(iface);
    TRACE("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI EQXAPO_LockForProcess(IXAPO *iface, UINT32 in_params_count,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS *in_params,
        UINT32 out_params_count,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS *out_params)
{
    EQImpl *This = EQImpl_from_IXAPO(iface);
    TRACE("%p, %u, %p, %u, %p\n", This, in_params_count, in_params,
            out_params_count, out_params);
    return E_NOTIMPL;
}

static void WINAPI EQXAPO_UnlockForProcess(IXAPO *iface)
{
    EQImpl *This = EQImpl_from_IXAPO(iface);
    TRACE("%p\n", This);
}

static void WINAPI EQXAPO_Process(IXAPO *iface, UINT32 in_params_count,
        const XAPO_PROCESS_BUFFER_PARAMETERS *in_params,
        UINT32 out_params_count,
        const XAPO_PROCESS_BUFFER_PARAMETERS *out_params, BOOL enabled)
{
    EQImpl *This = EQImpl_from_IXAPO(iface);
    TRACE("%p, %u, %p, %u, %p, %u\n", This, in_params_count, in_params,
            out_params_count, out_params, enabled);
}

static UINT32 WINAPI EQXAPO_CalcInputFrames(IXAPO *iface, UINT32 output_frames)
{
    EQImpl *This = EQImpl_from_IXAPO(iface);
    TRACE("%p, %u\n", This, output_frames);
    return 0;
}

static UINT32 WINAPI EQXAPO_CalcOutputFrames(IXAPO *iface, UINT32 input_frames)
{
    EQImpl *This = EQImpl_from_IXAPO(iface);
    TRACE("%p, %u\n", This, input_frames);
    return 0;
}

static const IXAPOVtbl EQXAPO_Vtbl = {
    EQXAPO_QueryInterface,
    EQXAPO_AddRef,
    EQXAPO_Release,
    EQXAPO_GetRegistrationProperties,
    EQXAPO_IsInputFormatSupported,
    EQXAPO_IsOutputFormatSupported,
    EQXAPO_Initialize,
    EQXAPO_Reset,
    EQXAPO_LockForProcess,
    EQXAPO_UnlockForProcess,
    EQXAPO_Process,
    EQXAPO_CalcInputFrames,
    EQXAPO_CalcOutputFrames
};

static HRESULT WINAPI EQXAPOParams_QueryInterface(IXAPOParameters *iface,
        REFIID riid, void **ppvObject)
{
    EQImpl *This = EQImpl_from_IXAPOParameters(iface);
    return EQXAPO_QueryInterface(&This->IXAPO_iface, riid, ppvObject);
}

static ULONG WINAPI EQXAPOParams_AddRef(IXAPOParameters *iface)
{
    EQImpl *This = EQImpl_from_IXAPOParameters(iface);
    return EQXAPO_AddRef(&This->IXAPO_iface);
}

static ULONG WINAPI EQXAPOParams_Release(IXAPOParameters *iface)
{
    EQImpl *This = EQImpl_from_IXAPOParameters(iface);
    return EQXAPO_Release(&This->IXAPO_iface);
}

static void WINAPI EQXAPOParams_SetParameters(IXAPOParameters *iface,
        const void *params, UINT32 params_len)
{
    EQImpl *This = EQImpl_from_IXAPOParameters(iface);
    TRACE("%p, %p, %u\n", This, params, params_len);
}

static void WINAPI EQXAPOParams_GetParameters(IXAPOParameters *iface, void *params,
        UINT32 params_len)
{
    EQImpl *This = EQImpl_from_IXAPOParameters(iface);
    TRACE("%p, %p, %u\n", This, params, params_len);
}

static const IXAPOParametersVtbl EQXAPOParameters_Vtbl = {
    EQXAPOParams_QueryInterface,
    EQXAPOParams_AddRef,
    EQXAPOParams_Release,
    EQXAPOParams_SetParameters,
    EQXAPOParams_GetParameters
};

struct xapo_cf {
    IClassFactory IClassFactory_iface;
    LONG ref;
    DWORD version;
    const CLSID *class;
};

struct xapo_cf *xapo_impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct xapo_cf, IClassFactory_iface);
}

static HRESULT WINAPI xapocf_QueryInterface(IClassFactory *iface, REFIID riid, void **ppobj)
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

static ULONG WINAPI xapocf_AddRef(IClassFactory *iface)
{
    struct xapo_cf *This = xapo_impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(): Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI xapocf_Release(IClassFactory *iface)
{
    struct xapo_cf *This = xapo_impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->(): Refcount now %u\n", This, ref);
    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

static HRESULT WINAPI xapocf_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
        REFIID riid, void **ppobj)
{
    struct xapo_cf *This = xapo_impl_from_IClassFactory(iface);
    HRESULT hr;

    TRACE("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if(pOuter)
        return CLASS_E_NOAGGREGATION;

    if(IsEqualGUID(This->class, &CLSID_AudioVolumeMeter)){
        VUMeterImpl *object;

        object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
        if(!object)
            return E_OUTOFMEMORY;

        object->IXAPO_iface.lpVtbl = &VUMXAPO_Vtbl;
        object->IXAPOParameters_iface.lpVtbl = &VUMXAPOParameters_Vtbl;
        object->version = This->version;

        hr = IXAPO_QueryInterface(&object->IXAPO_iface, riid, ppobj);
        if(FAILED(hr)){
            HeapFree(GetProcessHeap(), 0, object);
            return hr;
        }
    }else if(IsEqualGUID(This->class, &CLSID_AudioReverb)){
        ReverbImpl *object;

        object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
        if(!object)
            return E_OUTOFMEMORY;

        object->IXAPO_iface.lpVtbl = &RVBXAPO_Vtbl;
        object->IXAPOParameters_iface.lpVtbl = &RVBXAPOParameters_Vtbl;
        object->version = This->version;

        hr = IXAPO_QueryInterface(&object->IXAPO_iface, riid, ppobj);
        if(FAILED(hr)){
            HeapFree(GetProcessHeap(), 0, object);
            return hr;
        }
    }else if(IsEqualGUID(This->class, &CLSID_FXEQ)){
        EQImpl *object;

        object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
        if(!object)
            return E_OUTOFMEMORY;

        object->IXAPO_iface.lpVtbl = &EQXAPO_Vtbl;
        object->IXAPOParameters_iface.lpVtbl = &EQXAPOParameters_Vtbl;
        object->version = This->version;

        hr = IXAPO_QueryInterface(&object->IXAPO_iface, riid, ppobj);
        if(FAILED(hr)){
            HeapFree(GetProcessHeap(), 0, object);
            return hr;
        }
    }else
        return E_INVALIDARG;

    return S_OK;
}

static HRESULT WINAPI xapocf_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("(static)->(%d): stub!\n", dolock);
    return S_OK;
}

static const IClassFactoryVtbl xapo_Vtbl =
{
    xapocf_QueryInterface,
    xapocf_AddRef,
    xapocf_Release,
    xapocf_CreateInstance,
    xapocf_LockServer
};

IClassFactory *make_xapo_factory(REFCLSID clsid, DWORD version)
{
    struct xapo_cf *ret = HeapAlloc(GetProcessHeap(), 0, sizeof(struct xapo_cf));
    ret->IClassFactory_iface.lpVtbl = &xapo_Vtbl;
    ret->version = version;
    ret->class = clsid;
    ret->ref = 0;
    return &ret->IClassFactory_iface;
}
