/*
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

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

extern void sessions_lock(void) DECLSPEC_HIDDEN;
extern void sessions_unlock(void) DECLSPEC_HIDDEN;

extern const IAudioClient3Vtbl AudioClient3_Vtbl;

static inline struct audio_session_wrapper *impl_from_IAudioSessionControl2(IAudioSessionControl2 *iface)
{
    return CONTAINING_RECORD(iface, struct audio_session_wrapper, IAudioSessionControl2_iface);
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

        HeapFree(GetProcessHeap(), 0, This);
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
        WINE_UNIX_CALL(is_started, &params);
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
    FIXME("(%p)->(%p) - stub\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI control_SetDisplayName(IAudioSessionControl2 *iface, const WCHAR *name,
                                         const GUID *session)
{
    struct audio_session_wrapper *This = impl_from_IAudioSessionControl2(iface);
    FIXME("(%p)->(%p, %s) - stub\n", This, name, debugstr_guid(session));
    return E_NOTIMPL;
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

const IAudioSessionControl2Vtbl AudioSessionControl2_Vtbl =
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
