/* IDirectMusicSegmentState8 Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dmime_private.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

typedef struct IDirectMusicSegmentState8Impl {
    IDirectMusicSegmentState8 IDirectMusicSegmentState8_iface;
    LONG ref;
} IDirectMusicSegmentState8Impl;

static inline IDirectMusicSegmentState8Impl *impl_from_IDirectMusicSegmentState8(IDirectMusicSegmentState8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicSegmentState8Impl, IDirectMusicSegmentState8_iface);
}

static HRESULT WINAPI DirectMusicSegmentState8_QueryInterface(IDirectMusicSegmentState8 *iface, REFIID riid, void **ppobj)
{
    IDirectMusicSegmentState8Impl *This = impl_from_IDirectMusicSegmentState8(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

    if (!ppobj)
        return E_POINTER;

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectMusicSegmentState) ||
        IsEqualIID(riid, &IID_IDirectMusicSegmentState8))
    {
        IDirectMusicSegmentState8_AddRef(iface);
        *ppobj = &This->IDirectMusicSegmentState8_iface;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI DirectMusicSegmentState8_AddRef(IDirectMusicSegmentState8 *iface)
{
    IDirectMusicSegmentState8Impl *This = impl_from_IDirectMusicSegmentState8(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): %ld\n", This, ref);

    DMIME_LockModule();

    return ref;
}

static ULONG WINAPI DirectMusicSegmentState8_Release(IDirectMusicSegmentState8 *iface)
{
    IDirectMusicSegmentState8Impl *This = impl_from_IDirectMusicSegmentState8(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): %ld\n", This, ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    DMIME_UnlockModule();

    return ref;
}

static HRESULT WINAPI DirectMusicSegmentState8_GetRepeats(IDirectMusicSegmentState8 *iface,  DWORD* pdwRepeats)
{
    IDirectMusicSegmentState8Impl *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %p): stub\n", This, pdwRepeats);
    return S_OK;
}

static HRESULT WINAPI DirectMusicSegmentState8_GetSegment(IDirectMusicSegmentState8 *iface, IDirectMusicSegment** ppSegment)
{
    IDirectMusicSegmentState8Impl *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %p): stub\n", This, ppSegment);
    return S_OK;
}

static HRESULT WINAPI DirectMusicSegmentState8_GetStartTime(IDirectMusicSegmentState8 *iface, MUSIC_TIME* pmtStart)
{
    IDirectMusicSegmentState8Impl *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %p): stub\n", This, pmtStart);
    return S_OK;
}

static HRESULT WINAPI DirectMusicSegmentState8_GetSeek(IDirectMusicSegmentState8 *iface, MUSIC_TIME* pmtSeek)
{
    IDirectMusicSegmentState8Impl *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %p): stub\n", This, pmtSeek);
    return S_OK;
}

static HRESULT WINAPI DirectMusicSegmentState8_GetStartPoint(IDirectMusicSegmentState8 *iface, MUSIC_TIME* pmtStart)
{
    IDirectMusicSegmentState8Impl *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %p): stub\n", This, pmtStart);
    return S_OK;
}

static HRESULT WINAPI DirectMusicSegmentState8_SetTrackConfig(IDirectMusicSegmentState8 *iface, REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff) {
    IDirectMusicSegmentState8Impl *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %s, %ld, %ld, %ld, %ld): stub\n", This, debugstr_dmguid(rguidTrackClassID), dwGroupBits, dwIndex, dwFlagsOn, dwFlagsOff);
    return S_OK;
}

static HRESULT WINAPI DirectMusicSegmentState8_GetObjectInPath(IDirectMusicSegmentState8 *iface, DWORD dwPChannel, DWORD dwStage, DWORD dwBuffer, REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, void** ppObject) {
    IDirectMusicSegmentState8Impl *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %ld, %ld, %ld, %s, %ld, %s, %p): stub\n", This, dwPChannel, dwStage, dwBuffer, debugstr_dmguid(guidObject), dwIndex, debugstr_dmguid(iidInterface), ppObject);
    return S_OK;
}

static const IDirectMusicSegmentState8Vtbl DirectMusicSegmentState8Vtbl = {
    DirectMusicSegmentState8_QueryInterface,
    DirectMusicSegmentState8_AddRef,
    DirectMusicSegmentState8_Release,
    DirectMusicSegmentState8_GetRepeats,
    DirectMusicSegmentState8_GetSegment,
    DirectMusicSegmentState8_GetStartTime,
    DirectMusicSegmentState8_GetSeek,
    DirectMusicSegmentState8_GetStartPoint,
    DirectMusicSegmentState8_SetTrackConfig,
    DirectMusicSegmentState8_GetObjectInPath
};

/* for ClassFactory */
HRESULT WINAPI create_dmsegmentstate(REFIID riid, void **ret_iface)
{
    IDirectMusicSegmentState8Impl* obj;
    HRESULT hr;

    *ret_iface = NULL;

    obj = HeapAlloc (GetProcessHeap(), 0, sizeof(IDirectMusicSegmentState8Impl));
    if (!obj)
        return E_OUTOFMEMORY;

    obj->IDirectMusicSegmentState8_iface.lpVtbl = &DirectMusicSegmentState8Vtbl;
    obj->ref = 1;

    hr = IDirectMusicSegmentState8_QueryInterface(&obj->IDirectMusicSegmentState8_iface, riid, ret_iface);
    IDirectMusicSegmentState8_Release(&obj->IDirectMusicSegmentState8_iface);
    return hr;
}
