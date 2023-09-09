/*
 * IDirectMusicInstrument Implementation
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

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

static const GUID IID_IDirectMusicInstrumentPRIVATE = { 0xbcb20080, 0xa40c, 0x11d1, { 0x86, 0xbc, 0x00, 0xc0, 0x4f, 0xbf, 0x8f, 0xef } };

struct articulation
{
    struct list entry;
    CONNECTIONLIST list;
    CONNECTION connections[];
};

C_ASSERT(sizeof(struct articulation) == offsetof(struct articulation, connections[0]));

static HRESULT WINAPI instrument_QueryInterface(LPDIRECTMUSICINSTRUMENT iface, REFIID riid, LPVOID *ret_iface)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectMusicInstrument))
    {
        *ret_iface = iface;
        IDirectMusicInstrument_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IDirectMusicInstrumentPRIVATE))
    {
        /* it seems to me that this interface is only basic IUnknown, without any
         * other inherited functions... *sigh* this is the worst scenario, since it means
         * that whoever calls it knows the layout of original implementation table and therefore
         * tries to get data by direct access... expect crashes
         */
        FIXME("*sigh*... requested private/unspecified interface\n");

        *ret_iface = iface;
        IDirectMusicInstrument_AddRef(iface);
        return S_OK;
    }

    WARN("(%p)->(%s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);

    return E_NOINTERFACE;
}

static ULONG WINAPI instrument_AddRef(LPDIRECTMUSICINSTRUMENT iface)
{
    struct instrument *This = impl_from_IDirectMusicInstrument(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI instrument_Release(LPDIRECTMUSICINSTRUMENT iface)
{
    struct instrument *This = impl_from_IDirectMusicInstrument(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", iface, ref);

    if (!ref)
    {
        struct articulation *articulation, *next_articulation;
        struct region *region, *next_region;

        LIST_FOR_EACH_ENTRY_SAFE(articulation, next_articulation, &This->articulations, struct articulation, entry)
        {
            list_remove(&articulation->entry);
            free(articulation);
        }

        LIST_FOR_EACH_ENTRY_SAFE(region, next_region, &This->regions, struct region, entry)
        {
            list_remove(&region->entry);
            free(region);
        }

        free(This);
    }

    return ref;
}

static HRESULT WINAPI instrument_GetPatch(LPDIRECTMUSICINSTRUMENT iface, DWORD* pdwPatch)
{
    struct instrument *This = impl_from_IDirectMusicInstrument(iface);

    TRACE("(%p)->(%p)\n", This, pdwPatch);

    *pdwPatch = MIDILOCALE2Patch(&This->header.Locale);

    return S_OK;
}

static HRESULT WINAPI instrument_SetPatch(LPDIRECTMUSICINSTRUMENT iface, DWORD dwPatch)
{
    struct instrument *This = impl_from_IDirectMusicInstrument(iface);

    TRACE("(%p, %ld): stub\n", This, dwPatch);

    Patch2MIDILOCALE(dwPatch, &This->header.Locale);

    return S_OK;
}

static const IDirectMusicInstrumentVtbl instrument_vtbl =
{
    instrument_QueryInterface,
    instrument_AddRef,
    instrument_Release,
    instrument_GetPatch,
    instrument_SetPatch,
};

static HRESULT instrument_create(IDirectMusicInstrument **ret_iface)
{
    struct instrument *instrument;

    *ret_iface = NULL;
    if (!(instrument = calloc(1, sizeof(*instrument)))) return E_OUTOFMEMORY;
    instrument->IDirectMusicInstrument_iface.lpVtbl = &instrument_vtbl;
    instrument->ref = 1;
    list_init(&instrument->articulations);
    list_init(&instrument->regions);

    TRACE("Created DirectMusicInstrument %p\n", instrument);
    *ret_iface = &instrument->IDirectMusicInstrument_iface;
    return S_OK;
}

static HRESULT parse_art1_chunk(struct instrument *This, IStream *stream, struct chunk_entry *chunk)
{
    struct articulation *articulation;
    CONNECTIONLIST list;
    HRESULT hr;
    UINT size;

    if (chunk->size < sizeof(list)) return E_INVALIDARG;
    if (FAILED(hr = stream_read(stream, &list, sizeof(list)))) return hr;
    if (chunk->size != list.cbSize + sizeof(CONNECTION) * list.cConnections) return E_INVALIDARG;
    if (list.cbSize != sizeof(list)) return E_INVALIDARG;

    size = offsetof(struct articulation, connections[list.cConnections]);
    if (!(articulation = malloc(size))) return E_OUTOFMEMORY;
    articulation->list = list;

    size = sizeof(CONNECTION) * list.cConnections;
    if (FAILED(hr = stream_read(stream, articulation->connections, size))) free(articulation);
    else list_add_tail(&This->articulations, &articulation->entry);

    return hr;
}

static HRESULT parse_lart_list(struct instrument *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case FOURCC_ART1:
            hr = parse_art1_chunk(This, stream, &chunk);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

static HRESULT parse_rgn_chunk(struct instrument *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    struct region *region;
    HRESULT hr;

    if (!(region = malloc(sizeof(*region)))) return E_OUTOFMEMORY;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case FOURCC_RGNH:
            hr = stream_chunk_get_data(stream, &chunk, &region->header, sizeof(region->header));
            break;

        case FOURCC_WSMP:
            if (chunk.size < sizeof(region->wave_sample)) hr = E_INVALIDARG;
            else hr = stream_read(stream, &region->wave_sample, sizeof(region->wave_sample));
            if (SUCCEEDED(hr) && region->wave_sample.cSampleLoops)
            {
                if (region->wave_sample.cSampleLoops > 1) FIXME("More than one wave loop is not implemented\n");
                if (chunk.size != sizeof(WSMPL) + region->wave_sample.cSampleLoops * sizeof(WLOOP)) hr = E_INVALIDARG;
                else hr = stream_read(stream, &region->wave_loop, sizeof(region->wave_loop));
            }
            break;

        case FOURCC_WLNK:
            hr = stream_chunk_get_data(stream, &chunk, &region->wave_link, sizeof(region->wave_link));
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    if (FAILED(hr)) free(region);
    else list_add_tail(&This->regions, &region->entry);

    return hr;
}

static HRESULT parse_lrgn_list(struct instrument *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case MAKE_IDTYPE(FOURCC_LIST, FOURCC_RGN):
            hr = parse_rgn_chunk(This, stream, &chunk);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

static HRESULT parse_ins_chunk(struct instrument *This, IStream *stream, struct chunk_entry *parent)
{
    struct chunk_entry chunk = {.parent = parent};
    HRESULT hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
        case FOURCC_INSH:
            hr = stream_chunk_get_data(stream, &chunk, &This->header, sizeof(This->header));
            break;

        case FOURCC_DLID:
            hr = stream_chunk_get_data(stream, &chunk, &This->id, sizeof(This->id));
            break;

        case MAKE_IDTYPE(FOURCC_LIST, FOURCC_LRGN):
            hr = parse_lrgn_list(This, stream, &chunk);
            break;

        case MAKE_IDTYPE(FOURCC_LIST, FOURCC_LART):
            hr = parse_lart_list(This, stream, &chunk);
            break;

        default:
            FIXME("Ignoring chunk %s %s\n", debugstr_fourcc(chunk.id), debugstr_fourcc(chunk.type));
            break;
        }

        if (FAILED(hr)) break;
    }

    return hr;
}

HRESULT instrument_create_from_chunk(IStream *stream, struct chunk_entry *parent,
        IDirectMusicInstrument **ret_iface)
{
    IDirectMusicInstrument *iface;
    struct instrument *This;
    HRESULT hr;

    TRACE("(%p, %p)\n", stream, ret_iface);

    if (FAILED(hr = instrument_create(&iface))) return hr;
    This = impl_from_IDirectMusicInstrument(iface);

    if (FAILED(hr = parse_ins_chunk(This, stream, parent)))
    {
        IDirectMusicInstrument_Release(iface);
        return DMUS_E_UNSUPPORTED_STREAM;
    }

    if (TRACE_ON(dmusic))
    {
        TRACE("Created DirectMusicInstrument (%p) ***\n", This);
        if (!IsEqualGUID(&This->id, &GUID_NULL))
            TRACE(" - GUID = %s\n", debugstr_dmguid(&This->id));
        TRACE(" - Instrument header:\n");
        TRACE("    - cRegions: %ld\n", This->header.cRegions);
        TRACE("    - Locale:\n");
        TRACE("       - ulBank: %ld\n", This->header.Locale.ulBank);
        TRACE("       - ulInstrument: %ld\n", This->header.Locale.ulInstrument);
        TRACE("       => dwPatch: %ld\n", MIDILOCALE2Patch(&This->header.Locale));
    }

    *ret_iface = iface;
    return S_OK;
}
