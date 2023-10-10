/*
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

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

static DWORD next_track_id;

struct track_entry
{
    struct list entry;

    IDirectMusicTrack *track;
    void *state_data;
    DWORD track_id;
};

static void track_entry_destroy(struct track_entry *entry)
{
    HRESULT hr;

    if (FAILED(hr = IDirectMusicTrack_EndPlay(entry->track, entry->state_data)))
        WARN("track %p EndPlay failed, hr %#lx\n", entry->track, hr);

    IDirectMusicTrack_Release(entry->track);
    free(entry);
}

struct segment_state
{
    IDirectMusicSegmentState8 IDirectMusicSegmentState8_iface;
    LONG ref;

    IDirectMusicSegment *segment;
    MUSIC_TIME start_time;
    MUSIC_TIME start_point;
    MUSIC_TIME end_point;

    struct list tracks;
};

static inline struct segment_state *impl_from_IDirectMusicSegmentState8(IDirectMusicSegmentState8 *iface)
{
    return CONTAINING_RECORD(iface, struct segment_state, IDirectMusicSegmentState8_iface);
}

static HRESULT WINAPI segment_state_QueryInterface(IDirectMusicSegmentState8 *iface, REFIID riid, void **ppobj)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);

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

static ULONG WINAPI segment_state_AddRef(IDirectMusicSegmentState8 *iface)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): %ld\n", This, ref);

    return ref;
}

static ULONG WINAPI segment_state_Release(IDirectMusicSegmentState8 *iface)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): %ld\n", This, ref);

    if (!ref)
    {
        segment_state_end_play((IDirectMusicSegmentState *)iface);
        if (This->segment) IDirectMusicSegment_Release(This->segment);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI segment_state_GetRepeats(IDirectMusicSegmentState8 *iface, DWORD *repeats)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %p): semi-stub\n", This, repeats);
    return S_OK;
}

static HRESULT WINAPI segment_state_GetSegment(IDirectMusicSegmentState8 *iface, IDirectMusicSegment **segment)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);

    TRACE("(%p, %p)\n", This, segment);

    if (!(*segment = This->segment)) return DMUS_E_NOT_FOUND;

    IDirectMusicSegment_AddRef(This->segment);
    return S_OK;
}

static HRESULT WINAPI segment_state_GetStartTime(IDirectMusicSegmentState8 *iface, MUSIC_TIME *start_time)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);

    TRACE("(%p, %p)\n", This, start_time);

    *start_time = This->start_time;
    return S_OK;
}

static HRESULT WINAPI segment_state_GetSeek(IDirectMusicSegmentState8 *iface, MUSIC_TIME *seek_time)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %p): semi-stub\n", This, seek_time);
    *seek_time = 0;
    return S_OK;
}

static HRESULT WINAPI segment_state_GetStartPoint(IDirectMusicSegmentState8 *iface, MUSIC_TIME *start_point)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);

    TRACE("(%p, %p)\n", This, start_point);

    *start_point = This->start_point;
    return S_OK;
}

static HRESULT WINAPI segment_state_SetTrackConfig(IDirectMusicSegmentState8 *iface,
        REFGUID rguidTrackClassID, DWORD dwGroupBits, DWORD dwIndex, DWORD dwFlagsOn, DWORD dwFlagsOff)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %s, %ld, %ld, %ld, %ld): stub\n", This, debugstr_dmguid(rguidTrackClassID), dwGroupBits, dwIndex, dwFlagsOn, dwFlagsOff);
    return S_OK;
}

static HRESULT WINAPI segment_state_GetObjectInPath(IDirectMusicSegmentState8 *iface, DWORD dwPChannel,
        DWORD dwStage, DWORD dwBuffer, REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, void **ppObject)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8(iface);
    FIXME("(%p, %ld, %ld, %ld, %s, %ld, %s, %p): stub\n", This, dwPChannel, dwStage, dwBuffer, debugstr_dmguid(guidObject), dwIndex, debugstr_dmguid(iidInterface), ppObject);
    return S_OK;
}

static const IDirectMusicSegmentState8Vtbl segment_state_vtbl =
{
    segment_state_QueryInterface,
    segment_state_AddRef,
    segment_state_Release,
    segment_state_GetRepeats,
    segment_state_GetSegment,
    segment_state_GetStartTime,
    segment_state_GetSeek,
    segment_state_GetStartPoint,
    segment_state_SetTrackConfig,
    segment_state_GetObjectInPath,
};

/* for ClassFactory */
HRESULT create_dmsegmentstate(REFIID riid, void **ret_iface)
{
    struct segment_state *obj;
    HRESULT hr;

    *ret_iface = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IDirectMusicSegmentState8_iface.lpVtbl = &segment_state_vtbl;
    obj->ref = 1;
    obj->start_time = -1;
    list_init(&obj->tracks);

    TRACE("Created IDirectMusicSegmentState %p\n", obj);
    hr = IDirectMusicSegmentState8_QueryInterface(&obj->IDirectMusicSegmentState8_iface, riid, ret_iface);
    IDirectMusicSegmentState8_Release(&obj->IDirectMusicSegmentState8_iface);
    return hr;
}

HRESULT segment_state_create(IDirectMusicSegment *segment, MUSIC_TIME start_time,
        IDirectMusicPerformance *performance, IDirectMusicSegmentState **ret_iface)
{
    IDirectMusicSegmentState *iface;
    struct segment_state *This;
    IDirectMusicTrack *track;
    HRESULT hr;
    UINT i;

    TRACE("(%p, %lu, %p)\n", segment, start_time, ret_iface);

    if (FAILED(hr = create_dmsegmentstate(&IID_IDirectMusicSegmentState, (void **)&iface))) return hr;
    This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);

    This->segment = segment;
    IDirectMusicSegment_AddRef(This->segment);

    This->start_time = start_time;
    hr = IDirectMusicSegment_GetStartPoint(segment, &This->start_point);
    if (SUCCEEDED(hr)) hr = IDirectMusicSegment_GetLength(segment, &This->end_point);

    for (i = 0; SUCCEEDED(hr); i++)
    {
        DWORD track_id = ++next_track_id;
        struct track_entry *entry;

        if ((hr = IDirectMusicSegment_GetTrack(segment, &GUID_NULL, -1, i, &track)) != S_OK)
        {
            if (hr == DMUS_E_NOT_FOUND) hr = S_OK;
            break;
        }

        if (!(entry = malloc(sizeof(*entry))))
            hr = E_OUTOFMEMORY;
        else if (SUCCEEDED(hr = IDirectMusicTrack_InitPlay(track, iface, performance,
                &entry->state_data, track_id, 0)))
        {
            entry->track = track;
            entry->track_id = track_id;
            list_add_tail(&This->tracks, &entry->entry);
        }

        if (FAILED(hr))
        {
            WARN("Failed to initialize track %p, hr %#lx\n", track, hr);
            IDirectMusicTrack_Release(track);
            free(entry);
        }
    }

    if (SUCCEEDED(hr)) *ret_iface = iface;
    else IDirectMusicSegmentState_Release(iface);
    return hr;
}

HRESULT segment_state_play(IDirectMusicSegmentState *iface, IDirectMusicPerformance *performance)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);
    DWORD track_flags = DMUS_TRACKF_DIRTY | DMUS_TRACKF_START | DMUS_TRACKF_SEEK;
    MUSIC_TIME start_time = This->start_point, end_time = This->end_point;
    struct track_entry *entry;
    HRESULT hr = S_OK;

    LIST_FOR_EACH_ENTRY(entry, &This->tracks, struct track_entry, entry)
    {
        if (FAILED(hr = IDirectMusicTrack_Play(entry->track, entry->state_data, start_time,
                end_time, 0, track_flags, performance, iface, entry->track_id)))
        {
            WARN("Failed to play track %p, hr %#lx\n", entry->track, hr);
            break;
        }
    }

    return hr;
}

HRESULT segment_state_end_play(IDirectMusicSegmentState *iface)
{
    struct segment_state *This = impl_from_IDirectMusicSegmentState8((IDirectMusicSegmentState8 *)iface);
    struct track_entry *entry, *next;

    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &This->tracks, struct track_entry, entry)
    {
        list_remove(&entry->entry);
        track_entry_destroy(entry);
    }

    return S_OK;
}
