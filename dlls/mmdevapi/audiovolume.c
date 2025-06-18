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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"

#include "ole2.h"
#include "mmdeviceapi.h"
#include "mmsystem.h"
#include "dsound.h"
#include "audioclient.h"
#include "endpointvolume.h"
#include "audiopolicy.h"
#include "spatialaudioclient.h"

#include "mmdevapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

typedef struct AEVImpl {
    IAudioEndpointVolumeEx IAudioEndpointVolumeEx_iface;
    LONG ref;
    float master_vol;
    BOOL mute;
} AEVImpl;

static inline AEVImpl *impl_from_IAudioEndpointVolumeEx(IAudioEndpointVolumeEx *iface)
{
    return CONTAINING_RECORD(iface, AEVImpl, IAudioEndpointVolumeEx_iface);
}

static void AudioEndpointVolume_Destroy(AEVImpl *This)
{
    free(This);
}

static HRESULT WINAPI AEV_QueryInterface(IAudioEndpointVolumeEx *iface, REFIID riid, void **ppv)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppv);
    if (!ppv)
        return E_POINTER;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioEndpointVolume) ||
        IsEqualIID(riid, &IID_IAudioEndpointVolumeEx)) {
        *ppv = &This->IAudioEndpointVolumeEx_iface;
    }
    else
        return E_NOINTERFACE;
    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI AEV_AddRef(IAudioEndpointVolumeEx *iface)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) new ref %lu\n", This, ref);
    return ref;
}

static ULONG WINAPI AEV_Release(IAudioEndpointVolumeEx *iface)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) new ref %lu\n", This, ref);
    if (!ref)
        AudioEndpointVolume_Destroy(This);
    return ref;
}

static HRESULT WINAPI AEV_RegisterControlChangeNotify(IAudioEndpointVolumeEx *iface, IAudioEndpointVolumeCallback *notify)
{
    TRACE("(%p)->(%p)\n", iface, notify);
    if (!notify)
        return E_POINTER;
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI AEV_UnregisterControlChangeNotify(IAudioEndpointVolumeEx *iface, IAudioEndpointVolumeCallback *notify)
{
    TRACE("(%p)->(%p)\n", iface, notify);
    if (!notify)
        return E_POINTER;
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI AEV_GetChannelCount(IAudioEndpointVolumeEx *iface, UINT *count)
{
    TRACE("(%p)->(%p)\n", iface, count);
    if (!count)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_SetMasterVolumeLevel(IAudioEndpointVolumeEx *iface, float leveldb, const GUID *ctx)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);

    TRACE("(%p)->(%f,%s)\n", iface, leveldb, debugstr_guid(ctx));

    if(leveldb < -100.f || leveldb > 0.f)
        return E_INVALIDARG;

    This->master_vol = leveldb;

    return S_OK;
}

static HRESULT WINAPI AEV_SetMasterVolumeLevelScalar(IAudioEndpointVolumeEx *iface, float level, const GUID *ctx)
{
    TRACE("(%p)->(%f,%s)\n", iface, level, debugstr_guid(ctx));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_GetMasterVolumeLevel(IAudioEndpointVolumeEx *iface, float *leveldb)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);

    TRACE("(%p)->(%p)\n", iface, leveldb);

    if (!leveldb)
        return E_POINTER;

    *leveldb = This->master_vol;

    return S_OK;
}

static HRESULT WINAPI AEV_GetMasterVolumeLevelScalar(IAudioEndpointVolumeEx *iface, float *level)
{
    TRACE("(%p)->(%p)\n", iface, level);
    if (!level)
        return E_POINTER;
    FIXME("stub\n");
    *level = 1.0;
    return S_OK;
}

static HRESULT WINAPI AEV_SetChannelVolumeLevel(IAudioEndpointVolumeEx *iface, UINT chan, float leveldb, const GUID *ctx)
{
    TRACE("(%p)->(%f,%s)\n", iface, leveldb, debugstr_guid(ctx));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_SetChannelVolumeLevelScalar(IAudioEndpointVolumeEx *iface, UINT chan, float level, const GUID *ctx)
{
    TRACE("(%p)->(%u,%f,%s)\n", iface, chan, level, debugstr_guid(ctx));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_GetChannelVolumeLevel(IAudioEndpointVolumeEx *iface, UINT chan, float *leveldb)
{
    TRACE("(%p)->(%u,%p)\n", iface, chan, leveldb);
    if (!leveldb)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_GetChannelVolumeLevelScalar(IAudioEndpointVolumeEx *iface, UINT chan, float *level)
{
    TRACE("(%p)->(%u,%p)\n", iface, chan, level);
    if (!level)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_SetMute(IAudioEndpointVolumeEx *iface, BOOL mute, const GUID *ctx)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);
    HRESULT ret;

    TRACE("(%p)->(%u,%s)\n", iface, mute, debugstr_guid(ctx));

    ret = This->mute == mute ? S_FALSE : S_OK;

    This->mute = mute;

    return ret;
}

static HRESULT WINAPI AEV_GetMute(IAudioEndpointVolumeEx *iface, BOOL *mute)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);

    TRACE("(%p)->(%p)\n", iface, mute);

    if (!mute)
        return E_POINTER;

    *mute = This->mute;

    return S_OK;
}

static HRESULT WINAPI AEV_GetVolumeStepInfo(IAudioEndpointVolumeEx *iface, UINT *stepsize, UINT *stepcount)
{
    TRACE("(%p)->(%p,%p)\n", iface, stepsize, stepcount);
    if (!stepsize && !stepcount)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_VolumeStepUp(IAudioEndpointVolumeEx *iface, const GUID *ctx)
{
    TRACE("(%p)->(%s)\n", iface, debugstr_guid(ctx));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_VolumeStepDown(IAudioEndpointVolumeEx *iface, const GUID *ctx)
{
    TRACE("(%p)->(%s)\n", iface, debugstr_guid(ctx));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_QueryHardwareSupport(IAudioEndpointVolumeEx *iface, DWORD *mask)
{
    TRACE("(%p)->(%p)\n", iface, mask);
    if (!mask)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_GetVolumeRange(IAudioEndpointVolumeEx *iface, float *mindb, float *maxdb, float *inc)
{
    TRACE("(%p)->(%p,%p,%p)\n", iface, mindb, maxdb, inc);

    if (!mindb || !maxdb || !inc)
        return E_POINTER;

    *mindb = -100.f;
    *maxdb = 0.f;
    *inc = 1.f;

    return S_OK;
}

static HRESULT WINAPI AEV_GetVolumeRangeChannel(IAudioEndpointVolumeEx *iface, UINT chan, float *mindb, float *maxdb, float *inc)
{
    TRACE("(%p)->(%p,%p,%p)\n", iface, mindb, maxdb, inc);
    if (!mindb || !maxdb || !inc)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static const IAudioEndpointVolumeExVtbl AEVImpl_Vtbl = {
    AEV_QueryInterface,
    AEV_AddRef,
    AEV_Release,
    AEV_RegisterControlChangeNotify,
    AEV_UnregisterControlChangeNotify,
    AEV_GetChannelCount,
    AEV_SetMasterVolumeLevel,
    AEV_SetMasterVolumeLevelScalar,
    AEV_GetMasterVolumeLevel,
    AEV_GetMasterVolumeLevelScalar,
    AEV_SetChannelVolumeLevel,
    AEV_SetChannelVolumeLevelScalar,
    AEV_GetChannelVolumeLevel,
    AEV_GetChannelVolumeLevelScalar,
    AEV_SetMute,
    AEV_GetMute,
    AEV_GetVolumeStepInfo,
    AEV_VolumeStepUp,
    AEV_VolumeStepDown,
    AEV_QueryHardwareSupport,
    AEV_GetVolumeRange,
    AEV_GetVolumeRangeChannel
};

HRESULT AudioEndpointVolume_Create(MMDevice *parent, IAudioEndpointVolumeEx **ppv)
{
    AEVImpl *This;

    *ppv = NULL;
    This = calloc(1, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;
    This->IAudioEndpointVolumeEx_iface.lpVtbl = &AEVImpl_Vtbl;
    This->ref = 1;

    *ppv = &This->IAudioEndpointVolumeEx_iface;
    return S_OK;
}
