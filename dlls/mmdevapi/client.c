/*
 * Copyright 2011-2012 Maarten Lankhorst
 * Copyright 2010-2011 Maarten Lankhorst for CodeWeavers
 * Copyright 2011 Andrew Eikum for CodeWeavers
 * Copyright 2022 Huw Davies
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

#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <winternl.h>

#include <wine/debug.h>
#include <wine/unixlib.h>

#include "mmdevdrv.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

extern void sessions_lock(void) DECLSPEC_HIDDEN;
extern void sessions_unlock(void) DECLSPEC_HIDDEN;

void set_stream_volumes(struct audio_client *This)
{
    struct set_volumes_params params;

    params.stream          = This->stream;
    params.master_volume   = (This->session->mute ? 0.0f : This->session->master_vol);
    params.volumes         = This->vols;
    params.session_volumes = This->session->channel_vols;

    WINE_UNIX_CALL(set_volumes, &params);
}

static inline struct audio_client *impl_from_IAudioStreamVolume(IAudioStreamVolume *iface)
{
    return CONTAINING_RECORD(iface, struct audio_client, IAudioStreamVolume_iface);
}

static HRESULT WINAPI streamvolume_QueryInterface(IAudioStreamVolume *iface, REFIID riid,
                                                  void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioStreamVolume))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IMarshal)) {
        struct audio_client *This = impl_from_IAudioStreamVolume(iface);
        return IUnknown_QueryInterface(This->marshal, riid, ppv);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);

    return S_OK;
}

static ULONG WINAPI streamvolume_AddRef(IAudioStreamVolume *iface)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);
    return IAudioClient3_AddRef(&This->IAudioClient3_iface);
}

static ULONG WINAPI streamvolume_Release(IAudioStreamVolume *iface)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);
    return IAudioClient3_Release(&This->IAudioClient3_iface);
}

static HRESULT WINAPI streamvolume_GetChannelCount(IAudioStreamVolume *iface, UINT32 *out)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%p)\n", This, out);

    if (!out)
        return E_POINTER;

    *out = This->channel_count;

    return S_OK;
}

static HRESULT WINAPI streamvolume_SetChannelVolume(IAudioStreamVolume *iface, UINT32 index,
                                                    float level)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%d, %f)\n", This, index, level);

    if (level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    if (index >= This->channel_count)
        return E_INVALIDARG;

    sessions_lock();

    This->vols[index] = level;
    set_stream_volumes(This);

    sessions_unlock();

    return S_OK;
}

static HRESULT WINAPI streamvolume_GetChannelVolume(IAudioStreamVolume *iface, UINT32 index,
                                                    float *level)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%d, %p)\n", This, index, level);

    if (!level)
        return E_POINTER;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    if (index >= This->channel_count)
        return E_INVALIDARG;

    *level = This->vols[index];

    return S_OK;
}

static HRESULT WINAPI streamvolume_SetAllVolumes(IAudioStreamVolume *iface, UINT32 count,
                                                 const float *levels)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);
    unsigned int i;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if (!levels)
        return E_POINTER;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    if (count != This->channel_count)
        return E_INVALIDARG;

    sessions_lock();

    for (i = 0; i < count; ++i)
        This->vols[i] = levels[i];
    set_stream_volumes(This);

    sessions_unlock();

    return S_OK;
}

static HRESULT WINAPI streamvolume_GetAllVolumes(IAudioStreamVolume *iface, UINT32 count,
                                                 float *levels)
{
    struct audio_client *This = impl_from_IAudioStreamVolume(iface);
    unsigned int i;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if (!levels)
        return E_POINTER;

    if (!This->stream)
        return AUDCLNT_E_NOT_INITIALIZED;

    if (count != This->channel_count)
        return E_INVALIDARG;

    sessions_lock();

    for (i = 0; i < count; ++i)
        levels[i] = This->vols[i];

    sessions_unlock();

    return S_OK;
}

const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl =
{
    streamvolume_QueryInterface,
    streamvolume_AddRef,
    streamvolume_Release,
    streamvolume_GetChannelCount,
    streamvolume_SetChannelVolume,
    streamvolume_GetChannelVolume,
    streamvolume_SetAllVolumes,
    streamvolume_GetAllVolumes
};
