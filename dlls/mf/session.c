/*
 * Copyright 2017 Nikolay Sivov
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "mfidl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

typedef struct mfsession
{
    IMFMediaSession IMFMediaSession_iface;
    LONG ref;
} mfsession;

static inline mfsession *impl_from_IMFMediaSession(IMFMediaSession *iface)
{
    return CONTAINING_RECORD(iface, mfsession, IMFMediaSession_iface);
}

static HRESULT WINAPI mfsession_QueryInterface(IMFMediaSession *iface, REFIID riid, void **out)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFMediaSession) ||
            IsEqualIID(riid, &IID_IMFMediaEventGenerator) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &This->IMFMediaSession_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI mfsession_AddRef(IMFMediaSession *iface)
{
    mfsession *This = impl_from_IMFMediaSession(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mfsession_Release(IMFMediaSession *iface)
{
    mfsession *This = impl_from_IMFMediaSession(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI mfsession_GetEvent(IMFMediaSession *iface, DWORD flags, IMFMediaEvent **event)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)->(%#x, %p)\n", This, flags, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_BeginGetEvent(IMFMediaSession *iface, IMFAsyncCallback *callback, IUnknown *state)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)->(%p, %p)\n", This, callback, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_EndGetEvent(IMFMediaSession *iface, IMFAsyncResult *result, IMFMediaEvent **event)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)->(%p, %p)\n", This, result, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_QueueEvent(IMFMediaSession *iface, MediaEventType event_type, REFGUID ext_type,
        HRESULT hr, const PROPVARIANT *value)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)->(%d, %s, %#x, %p)\n", This, event_type, debugstr_guid(ext_type), hr, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_SetTopology(IMFMediaSession *iface, DWORD flags, IMFTopology *topology)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)->(%#x, %p)\n", This, flags, topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_ClearTopologies(IMFMediaSession *iface)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_Start(IMFMediaSession *iface, const GUID *format, const PROPVARIANT *start)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)->(%s, %p)\n", This, debugstr_guid(format), start);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_Pause(IMFMediaSession *iface)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_Stop(IMFMediaSession *iface)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_Close(IMFMediaSession *iface)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_Shutdown(IMFMediaSession *iface)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_GetClock(IMFMediaSession *iface, IMFClock **clock)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)->(%p)\n", This, clock);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_GetSessionCapabilities(IMFMediaSession *iface, DWORD *caps)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)->(%p)\n", This, caps);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfsession_GetFullTopology(IMFMediaSession *iface, DWORD flags, TOPOID id, IMFTopology **topology)
{
    mfsession *This = impl_from_IMFMediaSession(iface);

    FIXME("(%p)->(%#x, %s, %p)\n", This, flags, wine_dbgstr_longlong(id), topology);

    return E_NOTIMPL;
}

static const IMFMediaSessionVtbl mfmediasessionvtbl =
{
    mfsession_QueryInterface,
    mfsession_AddRef,
    mfsession_Release,
    mfsession_GetEvent,
    mfsession_BeginGetEvent,
    mfsession_EndGetEvent,
    mfsession_QueueEvent,
    mfsession_SetTopology,
    mfsession_ClearTopologies,
    mfsession_Start,
    mfsession_Pause,
    mfsession_Stop,
    mfsession_Close,
    mfsession_Shutdown,
    mfsession_GetClock,
    mfsession_GetSessionCapabilities,
    mfsession_GetFullTopology,
};

/***********************************************************************
 *      MFCreateTopology (mf.@)
 */
HRESULT WINAPI MFCreateMediaSession(IMFAttributes *config, IMFMediaSession **session)
{
    mfsession *object;

    TRACE("(%p, %p)\n", config, session);

    if (!session)
        return E_POINTER;

    if (config)
        FIXME("session configuration ignored\n");

    object = HeapAlloc(GetProcessHeap(), 0, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFMediaSession_iface.lpVtbl = &mfmediasessionvtbl;
    object->ref = 1;

    *session = &object->IMFMediaSession_iface;

    return S_OK;
}
