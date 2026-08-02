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

struct session_enum
{
    IAudioSessionEnumerator IAudioSessionEnumerator_iface;
    IMMDevice *device;
    GUID *sessions;
    int session_count;
    LONG ref;
};

static struct session_enum *impl_from_IAudioSessionEnumerator(IAudioSessionEnumerator *iface)
{
    return CONTAINING_RECORD(iface, struct session_enum, IAudioSessionEnumerator_iface);
}

static HRESULT WINAPI enumerator_QueryInterface(IAudioSessionEnumerator *iface, REFIID riid, void **ppv)
{
    struct session_enum *enumerator = impl_from_IAudioSessionEnumerator(iface);

    TRACE("(%p)->(%s, %p)\n", enumerator, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioSessionEnumerator))
        *ppv = &enumerator->IAudioSessionEnumerator_iface;
    else {
        WARN("Unknown iface %s.\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);

    return S_OK;
}

static ULONG WINAPI enumerator_AddRef(IAudioSessionEnumerator *iface)
{
    struct session_enum *enumerator = impl_from_IAudioSessionEnumerator(iface);
    ULONG ref = InterlockedIncrement(&enumerator->ref);
    TRACE("(%p) new ref %lu\n", enumerator, ref);
    return ref;
}

static ULONG WINAPI enumerator_Release(IAudioSessionEnumerator *iface)
{
    struct session_enum *enumerator = impl_from_IAudioSessionEnumerator(iface);
    ULONG ref = InterlockedDecrement(&enumerator->ref);
    TRACE("(%p) new ref %lu\n", enumerator, ref);

    if (!ref)
    {
        IMMDevice_Release(enumerator->device);
        free(enumerator->sessions);
        free(enumerator);
    }

    return ref;
}

static HRESULT WINAPI enumerator_GetCount(IAudioSessionEnumerator *iface, int *count)
{
    struct session_enum *enumerator = impl_from_IAudioSessionEnumerator(iface);

    TRACE("%p -> %p.\n", iface, count);

    if (!count) return E_POINTER;
    *count = enumerator->session_count;
    return S_OK;
}

static HRESULT WINAPI enumerator_GetSession(IAudioSessionEnumerator *iface, int index, IAudioSessionControl **session)
{
    struct session_enum *enumerator = impl_from_IAudioSessionEnumerator(iface);
    struct audio_session_wrapper *session_wrapper;
    HRESULT hr;

    TRACE("%p -> %d %p.\n", iface, index, session);

    if (!session) return E_POINTER;
    if (index >= enumerator->session_count)
        return E_FAIL;

    *session = NULL;
    sessions_lock();
    hr = get_audio_session_wrapper(&enumerator->sessions[index], enumerator->device, &session_wrapper);
    sessions_unlock();
    if (FAILED(hr))
        return hr;
    *session = (IAudioSessionControl *)&session_wrapper->IAudioSessionControl2_iface;
    return S_OK;
}

static const IAudioSessionEnumeratorVtbl IAudioSessionEnumerator_vtbl =
{
    enumerator_QueryInterface,
    enumerator_AddRef,
    enumerator_Release,
    enumerator_GetCount,
    enumerator_GetSession,
};

static HRESULT create_session_enumerator(IMMDevice *device, IAudioSessionEnumerator **ppv)
{
    struct session_enum *enumerator;
    HRESULT hr;

    if (!(enumerator = calloc(1, sizeof(*enumerator))))
        return E_OUTOFMEMORY;

    sessions_lock();
    hr = get_audio_sessions(device, &enumerator->sessions, &enumerator->session_count);
    sessions_unlock();
    if (FAILED(hr))
    {
        free(enumerator);
        return hr;
    }
    enumerator->IAudioSessionEnumerator_iface.lpVtbl = &IAudioSessionEnumerator_vtbl;
    IMMDevice_AddRef(device);
    enumerator->device = device;
    enumerator->ref = 1;
    *ppv = &enumerator->IAudioSessionEnumerator_iface;
    return S_OK;
}

struct session_mgr
{
    IAudioSessionManager2 IAudioSessionManager2_iface;
    IMMDevice *device;
    LONG ref;
};

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

    TRACE("(%p)->(%p).\n", This, out);

    return create_session_enumerator(This->device, out);
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
