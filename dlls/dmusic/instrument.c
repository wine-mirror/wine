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
#include "dmobject.h"

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

HRESULT instrument_create(IDirectMusicInstrument **ret_iface)
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

static HRESULT read_from_stream(IStream *stream, void *data, ULONG size)
{
    ULONG bytes_read;
    HRESULT hr;

    hr = IStream_Read(stream, data, size, &bytes_read);
    if(FAILED(hr)){
        TRACE("IStream_Read failed: %08lx\n", hr);
        return hr;
    }
    if (bytes_read < size) {
        TRACE("Didn't read full chunk: %lu < %lu\n", bytes_read, size);
        return E_FAIL;
    }

    return S_OK;
}

static inline DWORD subtract_bytes(DWORD len, DWORD bytes)
{
    if(bytes > len){
        TRACE("Apparent mismatch in chunk lengths? %lu bytes remaining, %lu bytes read\n", len, bytes);
        return 0;
    }
    return len - bytes;
}

static inline HRESULT advance_stream(IStream *stream, ULONG bytes)
{
    LARGE_INTEGER move;
    HRESULT ret;

    move.QuadPart = bytes;

    ret = IStream_Seek(stream, move, STREAM_SEEK_CUR, NULL);
    if (FAILED(ret))
        WARN("IStream_Seek failed: %08lx\n", ret);

    return ret;
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

/* Function that loads all instrument data and which is called from IDirectMusicCollection_GetInstrument as in native */
HRESULT instrument_load(IDirectMusicInstrument *iface, IStream *stream)
{
    struct instrument *This = impl_from_IDirectMusicInstrument(iface);
    HRESULT hr;
    DMUS_PRIVATE_CHUNK chunk;
    ULONG length = This->length;

    TRACE("(%p, %p): offset = 0x%s, length = %lu)\n", This, stream, wine_dbgstr_longlong(This->liInstrumentPosition.QuadPart), This->length);

    if (This->loaded)
        return S_OK;

    hr = IStream_Seek(stream, This->liInstrumentPosition, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
    {
        WARN("IStream_Seek failed: %08lx\n", hr);
        return DMUS_E_UNSUPPORTED_STREAM;
    }

    while (length)
    {
        hr = read_from_stream(stream, &chunk, sizeof(chunk));
        if (FAILED(hr))
            goto error;

        length = subtract_bytes(length, sizeof(chunk) + chunk.dwSize);

        switch (chunk.fccID)
        {
            case FOURCC_INSH:
            case FOURCC_DLID:
                TRACE("Chunk %s: %lu bytes\n", debugstr_fourcc(chunk.fccID), chunk.dwSize);

                /* Instrument header and id are already set so just skip */
                hr = advance_stream(stream, chunk.dwSize);
                if (FAILED(hr))
                    goto error;

                break;

            case FOURCC_LIST: {
                DWORD size = chunk.dwSize;

                TRACE("LIST chunk: %lu bytes\n", chunk.dwSize);

                hr = read_from_stream(stream, &chunk.fccID, sizeof(chunk.fccID));
                if (FAILED(hr))
                    goto error;

                size = subtract_bytes(size, sizeof(chunk.fccID));

                switch (chunk.fccID)
                {
                    case FOURCC_LRGN:
                    {
                        static const LARGE_INTEGER zero = {0};
                        struct chunk_entry list_chunk = {.id = FOURCC_LIST, .size = chunk.dwSize, .type = chunk.fccID};
                        TRACE("LRGN chunk (regions list): %lu bytes\n", size);
                        IStream_Seek(stream, zero, STREAM_SEEK_CUR, &list_chunk.offset);
                        list_chunk.offset.QuadPart -= 12;
                        hr = parse_lrgn_list(This, stream, &list_chunk);
                        break;
                    }

                    case FOURCC_LART:
                    {
                        static const LARGE_INTEGER zero = {0};
                        struct chunk_entry list_chunk = {.id = FOURCC_LIST, .size = chunk.dwSize, .type = chunk.fccID};
                        TRACE("LART chunk (articulations list): %lu bytes\n", size);
                        IStream_Seek(stream, zero, STREAM_SEEK_CUR, &list_chunk.offset);
                        list_chunk.offset.QuadPart -= 12;
                        hr = parse_lart_list(This, stream, &list_chunk);
                        break;
                    }

                    default:
                        TRACE("Unknown chunk %s: %lu bytes\n", debugstr_fourcc(chunk.fccID), chunk.dwSize);

                        hr = advance_stream(stream, chunk.dwSize - sizeof(chunk.fccID));
                        if (FAILED(hr))
                            goto error;

                        size = subtract_bytes(size, chunk.dwSize - sizeof(chunk.fccID));
                        break;
                }
                break;
            }

            default:
                TRACE("Unknown chunk %s: %lu bytes\n", debugstr_fourcc(chunk.fccID), chunk.dwSize);

                hr = advance_stream(stream, chunk.dwSize);
                if (FAILED(hr))
                    goto error;

                break;
        }
    }

    This->loaded = TRUE;

    return S_OK;

error:
    return DMUS_E_UNSUPPORTED_STREAM;
}
