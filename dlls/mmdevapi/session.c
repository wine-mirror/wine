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

#include "mmdevapi_private.h"

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

extern void sessions_lock(void);
extern void sessions_unlock(void);

static WCHAR *duplicate_wstr(const WCHAR *str)
{
    const WCHAR *source = str ? str : L"";
    int len = (wcslen(source) + 1) * sizeof(WCHAR);
    WCHAR *ret = CoTaskMemAlloc(len);
    memcpy(ret, source, len);
    return ret;
}

extern void set_stream_volumes(struct audio_client *This);

static struct list sessions = LIST_INIT(sessions);

static inline struct audio_session_wrapper *impl_from_IAudioSessionControl2(IAudioSessionControl2 *iface)
{
    return CONTAINING_RECORD(iface, struct audio_session_wrapper, IAudioSessionControl2_iface);
}

static inline struct audio_session_wrapper *impl_from_IChannelAudioVolume(IChannelAudioVolume *iface)
{
    return CONTAINING_RECORD(iface, struct audio_session_wrapper, IChannelAudioVolume_iface);
}

static inline struct audio_session_wrapper *impl_from_ISimpleAudioVolume(ISimpleAudioVolume *iface)
{
    return CONTAINING_RECORD(iface, struct audio_session_wrapper, ISimpleAudioVolume_iface);
}

static HRESULT WINAPI control_QueryInterface(IAudioSessionControl2 *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioSessionControl) ||
        IsEqualIID(riid, &IID_IAudioSessionControl2))
        *ppv = iface;
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);

    return S_OK;
}

static ULONG WINAPI control_AddRef(IAudioSessionControl2 *iface)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) Refcount now %lu\n", This, ref);
    return ref;
}

static ULONG WINAPI control_Release(IAudioSessionControl2 *iface)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) Refcount now %lu\n", This, ref);

    if (!ref) {
        if (This->client) {
            sessions_lock();
            This->client->session_wrapper = NULL;
            sessions_unlock();
            IAudioClient3_Release(&This->client->IAudioClient3_iface);
        }

        free(This);
    }

    return ref;
}

static HRESULT WINAPI control_GetState(IAudioSessionControl2 *iface, AudioSessionState *state)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    struct is_started_params params;
    struct audio_client *client;

    TRACE("(%p)->(%p)\n", This, state);

    if (!state)
        return NULL_PTR_ERR;

    sessions_lock();

    if (list_empty(&This->session->clients)) {
        *state = AudioSessionStateExpired;
        sessions_unlock();
        return S_OK;
    }

    LIST_FOR_EACH_ENTRY(client, &This->session->clients, struct audio_client, entry) {
        params.stream = client->stream;
        wine_unix_call(is_started, &params);
        if (params.result == S_OK) {
            *state = AudioSessionStateActive;
            sessions_unlock();
            return S_OK;
        }
    }

    sessions_unlock();

    *state = AudioSessionStateInactive;

    return S_OK;
}

static HRESULT WINAPI control_GetDisplayName(IAudioSessionControl2 *iface, WCHAR **name)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    struct audio_session *session = This->session;

    TRACE("(%p)->(%p) - stub\n", This, name);

    if (!name)
        return E_POINTER;

    *name = duplicate_wstr(session->display_name);

    return S_OK;
}

static HRESULT WINAPI control_SetDisplayName(IAudioSessionControl2 *iface, const WCHAR *name,
                                         const GUID *event_context)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    struct audio_session *session = This->session;

    TRACE("(%p)->(%p, %s) - stub\n", This, name, debugstr_guid(event_context));
    FIXME("Ignoring event_context\n");

    if (!name)
        return HRESULT_FROM_WIN32(RPC_X_NULL_REF_POINTER);

    free(session->display_name);
    session->display_name = wcsdup(name);

    return S_OK;
}

static HRESULT WINAPI control_GetIconPath(IAudioSessionControl2 *iface, WCHAR **path)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    FIXME("(%p)->(%p) - stub\n", This, path);
    return E_NOTIMPL;
}

static HRESULT WINAPI control_SetIconPath(IAudioSessionControl2 *iface, const WCHAR *path,
                                      const GUID *session)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    FIXME("(%p)->(%s, %s) - stub\n", This, debugstr_w(path), debugstr_guid(session));
    return E_NOTIMPL;
}

static HRESULT WINAPI control_GetGroupingParam(IAudioSessionControl2 *iface, GUID *group)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    FIXME("(%p)->(%p) - stub\n", This, group);
    return E_NOTIMPL;
}

static HRESULT WINAPI control_SetGroupingParam(IAudioSessionControl2 *iface, const GUID *group,
                                           const GUID *session)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    FIXME("(%p)->(%s, %s) - stub\n", This, debugstr_guid(group), debugstr_guid(session));
    return E_NOTIMPL;
}

static HRESULT WINAPI control_RegisterAudioSessionNotification(IAudioSessionControl2 *iface,
                                                           IAudioSessionEvents *events)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    FIXME("(%p)->(%p) - stub\n", This, events);
    return S_OK;
}

static HRESULT WINAPI control_UnregisterAudioSessionNotification(IAudioSessionControl2 *iface,
                                                             IAudioSessionEvents *events)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    FIXME("(%p)->(%p) - stub\n", This, events);
    return S_OK;
}

static HRESULT WINAPI control_GetSessionIdentifier(IAudioSessionControl2 *iface, WCHAR **id)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    FIXME("(%p)->(%p) - stub\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI control_GetSessionInstanceIdentifier(IAudioSessionControl2 *iface, WCHAR **id)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    FIXME("(%p)->(%p) - stub\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI control_GetProcessId(IAudioSessionControl2 *iface, DWORD *pid)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);

    TRACE("(%p)->(%p)\n", This, pid);

    if (!pid)
        return E_POINTER;

    *pid = GetCurrentProcessId();

    return S_OK;
}

static HRESULT WINAPI control_IsSystemSoundsSession(IAudioSessionControl2 *iface)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    TRACE("(%p)\n", This);
    return S_FALSE;
}

static HRESULT WINAPI control_SetDuckingPreference(IAudioSessionControl2 *iface, BOOL optout)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    TRACE("(%p)->(%d)\n", This, optout);
    return S_OK;
}

static const IAudioSessionControl2Vtbl AudioSessionControl2_Vtbl =
{
    control_QueryInterface,
    control_AddRef,
    control_Release,
    control_GetState,
    control_GetDisplayName,
    control_SetDisplayName,
    control_GetIconPath,
    control_SetIconPath,
    control_GetGroupingParam,
    control_SetGroupingParam,
    control_RegisterAudioSessionNotification,
    control_UnregisterAudioSessionNotification,
    control_GetSessionIdentifier,
    control_GetSessionInstanceIdentifier,
    control_GetProcessId,
    control_IsSystemSoundsSession,
    control_SetDuckingPreference
};

static HRESULT WINAPI channelvolume_QueryInterface(IChannelAudioVolume *iface, REFIID riid,
                                                   void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IChannelAudioVolume))
        *ppv = iface;
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);

    return S_OK;
}

static ULONG WINAPI channelvolume_AddRef(IChannelAudioVolume *iface)
{
    struct audio_session_wrapper *This = impl_from_IChannelAudioVolume(iface);
    return IAudioSessionControl2_AddRef(&This->IAudioSessionControl2_iface);
}

static ULONG WINAPI channelvolume_Release(IChannelAudioVolume *iface)
{
    struct audio_session_wrapper *This = impl_from_IChannelAudioVolume(iface);
    return IAudioSessionControl2_Release(&This->IAudioSessionControl2_iface);
}

static HRESULT WINAPI channelvolume_GetChannelCount(IChannelAudioVolume *iface, UINT32 *out)
{
    struct audio_session_wrapper *This = impl_from_IChannelAudioVolume(iface);
    struct audio_session *session = This->session;

    TRACE("(%p)->(%p)\n", session, out);

    if (!out)
        return NULL_PTR_ERR;

    *out = session->channel_count;

    return S_OK;
}

static HRESULT WINAPI channelvolume_SetChannelVolume(IChannelAudioVolume *iface, UINT32 index,
                                                     float level, const GUID *context)
{
    struct audio_session_wrapper *This = impl_from_IChannelAudioVolume(iface);
    struct audio_session *session = This->session;
    struct audio_client *client;

    TRACE("(%p)->(%d, %f, %s)\n", session, index, level, wine_dbgstr_guid(context));

    if (level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if (index >= session->channel_count)
        return E_INVALIDARG;

    if (context)
        FIXME("Notifications not supported yet\n");

    sessions_lock();

    session->channel_vols[index] = level;

    LIST_FOR_EACH_ENTRY(client, &session->clients, struct audio_client, entry)
        set_stream_volumes(client);

    sessions_unlock();

    return S_OK;
}

static HRESULT WINAPI channelvolume_GetChannelVolume(IChannelAudioVolume *iface, UINT32 index,
                                                     float *level)
{
    struct audio_session_wrapper *This = impl_from_IChannelAudioVolume(iface);
    struct audio_session *session = This->session;

    TRACE("(%p)->(%d, %p)\n", session, index, level);

    if (!level)
        return NULL_PTR_ERR;

    if (index >= session->channel_count)
        return E_INVALIDARG;

    *level = session->channel_vols[index];

    return S_OK;
}

static HRESULT WINAPI channelvolume_SetAllVolumes(IChannelAudioVolume *iface, UINT32 count,
                                                  const float *levels, const GUID *context)
{
    struct audio_session_wrapper *This = impl_from_IChannelAudioVolume(iface);
    struct audio_session *session = This->session;
    struct audio_client *client;
    unsigned int i;

    TRACE("(%p)->(%d, %p, %s)\n", session, count, levels, wine_dbgstr_guid(context));

    if (!levels)
        return NULL_PTR_ERR;

    if (count != session->channel_count)
        return E_INVALIDARG;

    if (context)
        FIXME("Notifications not supported yet\n");

    sessions_lock();

    for (i = 0; i < count; ++i)
        session->channel_vols[i] = levels[i];

    LIST_FOR_EACH_ENTRY(client, &session->clients, struct audio_client, entry)
        set_stream_volumes(client);

    sessions_unlock();

    return S_OK;
}

static HRESULT WINAPI channelvolume_GetAllVolumes(IChannelAudioVolume *iface, UINT32 count,
                                                  float *levels)
{
    struct audio_session_wrapper *This = impl_from_IChannelAudioVolume(iface);
    struct audio_session *session = This->session;
    unsigned int i;

    TRACE("(%p)->(%d, %p)\n", session, count, levels);

    if (!levels)
        return NULL_PTR_ERR;

    if (count != session->channel_count)
        return E_INVALIDARG;

    for (i = 0; i < count; ++i)
        levels[i] = session->channel_vols[i];

    return S_OK;
}

static const IChannelAudioVolumeVtbl ChannelAudioVolume_Vtbl =
{
    channelvolume_QueryInterface,
    channelvolume_AddRef,
    channelvolume_Release,
    channelvolume_GetChannelCount,
    channelvolume_SetChannelVolume,
    channelvolume_GetChannelVolume,
    channelvolume_SetAllVolumes,
    channelvolume_GetAllVolumes
};

static HRESULT WINAPI simplevolume_QueryInterface(ISimpleAudioVolume *iface, REFIID riid,
                                                  void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_ISimpleAudioVolume))
        *ppv = iface;
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);

    return S_OK;
}

static ULONG WINAPI simplevolume_AddRef(ISimpleAudioVolume *iface)
{
    struct audio_session_wrapper *This = impl_from_ISimpleAudioVolume(iface);
    return IAudioSessionControl2_AddRef(&This->IAudioSessionControl2_iface);
}

static ULONG WINAPI simplevolume_Release(ISimpleAudioVolume *iface)
{
    struct audio_session_wrapper *This = impl_from_ISimpleAudioVolume(iface);
    return IAudioSessionControl2_Release(&This->IAudioSessionControl2_iface);
}

static HRESULT WINAPI simplevolume_SetMasterVolume(ISimpleAudioVolume *iface, float level,
                                                  const GUID *context)
{
    struct audio_session_wrapper *This = impl_from_ISimpleAudioVolume(iface);
    struct audio_session *session = This->session;
    struct audio_client *client;

    TRACE("(%p)->(%f, %s)\n", session, level, wine_dbgstr_guid(context));

    if (level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if (context)
        FIXME("Notifications not supported yet\n");

    sessions_lock();

    session->master_vol = level;

    LIST_FOR_EACH_ENTRY(client, &session->clients, struct audio_client, entry)
        set_stream_volumes(client);

    sessions_unlock();

    return S_OK;
}

static HRESULT WINAPI simplevolume_GetMasterVolume(ISimpleAudioVolume *iface, float *level)
{
    struct audio_session_wrapper *This = impl_from_ISimpleAudioVolume(iface);
    struct audio_session *session = This->session;

    TRACE("(%p)->(%p)\n", session, level);

    if (!level)
        return NULL_PTR_ERR;

    *level = session->master_vol;

    return S_OK;
}

static HRESULT WINAPI simplevolume_SetMute(ISimpleAudioVolume *iface, BOOL mute,
                                           const GUID *context)
{
    struct audio_session_wrapper *This = impl_from_ISimpleAudioVolume(iface);
    struct audio_session *session = This->session;
    struct audio_client *client;

    TRACE("(%p)->(%u, %s)\n", session, mute, debugstr_guid(context));

    if (context)
        FIXME("Notifications not supported yet\n");

    sessions_lock();

    session->mute = mute;

    LIST_FOR_EACH_ENTRY(client, &session->clients, struct audio_client, entry)
        set_stream_volumes(client);

    sessions_unlock();

    return S_OK;
}

static HRESULT WINAPI simplevolume_GetMute(ISimpleAudioVolume *iface, BOOL *mute)
{
    struct audio_session_wrapper *This = impl_from_ISimpleAudioVolume(iface);
    struct audio_session *session = This->session;

    TRACE("(%p)->(%p)\n", session, mute);

    if (!mute)
        return NULL_PTR_ERR;

    *mute = session->mute;

    return S_OK;
}

static const ISimpleAudioVolumeVtbl SimpleAudioVolume_Vtbl =
{
    simplevolume_QueryInterface,
    simplevolume_AddRef,
    simplevolume_Release,
    simplevolume_SetMasterVolume,
    simplevolume_GetMasterVolume,
    simplevolume_SetMute,
    simplevolume_GetMute
};

static void session_init_vols(struct audio_session *session, UINT channels)
{
    if (session->channel_count < channels) {
        UINT i;

        session->channel_vols = realloc(session->channel_vols, sizeof(float) * channels);
        if (!session->channel_vols)
            return;

        for (i = session->channel_count; i < channels; i++)
            session->channel_vols[i] = 1.f;

        session->channel_count = channels;
    }
}

static struct audio_session *session_create(const GUID *guid, IMMDevice *device, UINT channels)
{
    struct audio_session *ret = calloc(1, sizeof(struct audio_session));
    if (!ret)
        return NULL;

    memcpy(&ret->guid, guid, sizeof(GUID));

    ret->device = device;

    list_init(&ret->clients);

    list_add_head(&sessions, &ret->entry);

    session_init_vols(ret, channels);

    ret->master_vol = 1.f;

    return ret;
}

struct audio_session_wrapper *session_wrapper_create(struct audio_client *client)
{
    struct audio_session_wrapper *ret;

    ret = calloc(1, sizeof(struct audio_session_wrapper));
    if (!ret)
        return NULL;

    ret->IAudioSessionControl2_iface.lpVtbl = &AudioSessionControl2_Vtbl;
    ret->IChannelAudioVolume_iface.lpVtbl   = &ChannelAudioVolume_Vtbl;
    ret->ISimpleAudioVolume_iface.lpVtbl    = &SimpleAudioVolume_Vtbl;

    ret->ref    = 1;
    ret->client = client;

    if (client) {
        ret->session = client->session;
        IAudioClient3_AddRef(&client->IAudioClient3_iface);
    }

    return ret;
}

/* If channels == 0, then this will return or create a session with
 * matching dataflow and GUID. Otherwise, channels must also match. */
HRESULT get_audio_session(const GUID *guid, IMMDevice *device, UINT channels,
                          struct audio_session **out)
{
    struct audio_session *session;

    TRACE("(%s, %p, %u, %p)\n", debugstr_guid(guid), device, channels, out);

    if (!guid || IsEqualGUID(guid, &GUID_NULL)) {
        *out = session_create(&GUID_NULL, device, channels);
        if (!*out)
            return E_OUTOFMEMORY;

        return S_OK;
    }

    *out = NULL;
    LIST_FOR_EACH_ENTRY(session, &sessions, struct audio_session, entry) {
        if (session->device == device && IsEqualGUID(guid, &session->guid)) {
            session_init_vols(session, channels);
            *out = session;
            break;
        }
    }

    if (!*out) {
        *out = session_create(guid, device, channels);
        if (!*out)
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT get_audio_session_wrapper(const GUID *guid, IMMDevice *device,
                                  struct audio_session_wrapper **out)
{
    struct audio_session *session;

    const HRESULT hr = get_audio_session(guid, device, 0, &session);
    if (FAILED(hr))
        return hr;

    *out = session_wrapper_create(NULL);
    if (!*out)
        return E_OUTOFMEMORY;

    (*out)->session = session;

    return S_OK;
}
