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

#include <wine/debug.h>
#include <wine/list.h>

#include "mmdevapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

extern HRESULT get_audio_session_wrapper(const GUID *guid, IMMDevice *device,
                                         struct audio_session_wrapper **out);

static CRITICAL_SECTION g_sessions_lock;
static CRITICAL_SECTION_DEBUG g_sessions_lock_debug =
{
    0, 0, &g_sessions_lock,
    { &g_sessions_lock_debug.ProcessLocksList, &g_sessions_lock_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": g_sessions_lock") }
};
static CRITICAL_SECTION g_sessions_lock = { &g_sessions_lock_debug, -1, 0, 0, 0, 0 };

void sessions_lock(void)
{
    EnterCriticalSection(&g_sessions_lock);
}

void sessions_unlock(void)
{
    LeaveCriticalSection(&g_sessions_lock);
}

static inline struct session_mgr *impl_from_IAudioSessionManager2(IAudioSessionManager2 *iface)
{
    return CONTAINING_RECORD(iface, struct session_mgr, IAudioSessionManager2_iface);
}

static HRESULT WINAPI ASM_QueryInterface(IAudioSessionManager2 *iface, REFIID riid, void **ppv)
{
    struct session_mgr *This = impl_from_IAudioSessionManager2(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioSessionManager) ||
        IsEqualIID(riid, &IID_IAudioSessionManager2))
        *ppv = &This->IAudioSessionManager2_iface;
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);

    return S_OK;
}

static ULONG WINAPI ASM_AddRef(IAudioSessionManager2 *iface)
{
    struct session_mgr *This = impl_from_IAudioSessionManager2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) new ref %lu\n", This, ref);
    return ref;
}

static ULONG WINAPI ASM_Release(IAudioSessionManager2 *iface)
{
    struct session_mgr *This = impl_from_IAudioSessionManager2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) new ref %lu\n", This, ref);

    if (!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI ASM_GetAudioSessionControl(IAudioSessionManager2 *iface,
                                                 const GUID *guid, DWORD flags,
                                                 IAudioSessionControl **out)
{
    struct session_mgr *This = impl_from_IAudioSessionManager2(iface);
    AudioSessionWrapper *wrapper;
    HRESULT hr;

    TRACE("(%p)->(%s, %lx, %p)\n", This, debugstr_guid(guid), flags, out);

    hr = get_audio_session_wrapper(guid, This->device, &wrapper);
    if (FAILED(hr))
        return hr;

    *out = (IAudioSessionControl*)&wrapper->IAudioSessionControl2_iface;

    return S_OK;
}

static HRESULT WINAPI ASM_GetSimpleAudioVolume(IAudioSessionManager2 *iface,
                                               const GUID *guid, DWORD flags,
                                               ISimpleAudioVolume **out)
{
    struct session_mgr *This = impl_from_IAudioSessionManager2(iface);
    AudioSessionWrapper *wrapper;
    HRESULT hr;

    TRACE("(%p)->(%s, %lx, %p)\n", This, debugstr_guid(guid), flags, out);

    hr = get_audio_session_wrapper(guid, This->device, &wrapper);
    if (FAILED(hr))
        return hr;

    *out = &wrapper->ISimpleAudioVolume_iface;

    return S_OK;
}

static HRESULT WINAPI ASM_GetSessionEnumerator(IAudioSessionManager2 *iface,
                                               IAudioSessionEnumerator **out)
{
    struct session_mgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, out);
    return E_NOTIMPL;
}

static HRESULT WINAPI ASM_RegisterSessionNotification(IAudioSessionManager2 *iface,
                                                      IAudioSessionNotification *notification)
{
    struct session_mgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static HRESULT WINAPI ASM_UnregisterSessionNotification(IAudioSessionManager2 *iface,
                                                        IAudioSessionNotification *notification)
{
    struct session_mgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static HRESULT WINAPI ASM_RegisterDuckNotification(IAudioSessionManager2 *iface,
                                                   const WCHAR *session_id,
                                                   IAudioVolumeDuckNotification *notification)
{
    struct session_mgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%s, %p) - stub\n", This, debugstr_w(session_id), notification);
    return E_NOTIMPL;
}

static HRESULT WINAPI ASM_UnregisterDuckNotification(IAudioSessionManager2 *iface,
                                                     IAudioVolumeDuckNotification *notification)
{
    struct session_mgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static const IAudioSessionManager2Vtbl AudioSessionManager2_Vtbl =
{
    ASM_QueryInterface,
    ASM_AddRef,
    ASM_Release,
    ASM_GetAudioSessionControl,
    ASM_GetSimpleAudioVolume,
    ASM_GetSessionEnumerator,
    ASM_RegisterSessionNotification,
    ASM_UnregisterSessionNotification,
    ASM_RegisterDuckNotification,
    ASM_UnregisterDuckNotification
};

HRESULT AudioSessionManager_Create(IMMDevice *device, IAudioSessionManager2 **ppv)
{
    struct session_mgr *This;

    This = calloc(1, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->IAudioSessionManager2_iface.lpVtbl = &AudioSessionManager2_Vtbl;
    This->device = device;
    This->ref = 1;

    *ppv = &This->IAudioSessionManager2_iface;

    return S_OK;
}
