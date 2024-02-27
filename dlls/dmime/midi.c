/*
 * Copyright (C) 2024 Yuxuan Shui for CodeWeavers
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

#include "dmusic_midi.h"
#include "dmime_private.h"
#include "dmusic_band.h"
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_WORD(x) RtlUshortByteSwap(x)
#define GET_BE_DWORD(x) RtlUlongByteSwap(x)
#endif

struct midi_event
{
    MUSIC_TIME delta_time;
    BYTE status;
    BYTE data[2];
};

struct midi_parser
{
    IDirectMusicTrack *bandtrack;
    MUSIC_TIME time;
    IStream *stream;
    DWORD division;
};

static HRESULT stream_read_at_most(IStream *stream, void *buffer, ULONG size, ULONG *bytes_left)
{
    HRESULT hr;
    ULONG read = 0;
    if (size > *bytes_left) hr = IStream_Read(stream, buffer, *bytes_left, &read);
    else hr = IStream_Read(stream, buffer, size, &read);
    if (hr != S_OK) return hr;
    *bytes_left -= read;
    if (read < size) return S_FALSE;
    return S_OK;
}

static HRESULT read_variable_length_number(IStream *stream, DWORD *out, ULONG *bytes_left)
{
    BYTE byte;
    HRESULT hr = S_OK;

    *out = 0;
    do
    {
        hr = stream_read_at_most(stream, &byte, 1, bytes_left);
        if (hr != S_OK) return hr;
        *out = (*out << 7) | (byte & 0x7f);
    } while (byte & 0x80);
    return S_OK;
}

static HRESULT read_midi_event(IStream *stream, struct midi_event *event, BYTE *last_status, ULONG *bytes_left)
{
    BYTE byte, status_type, meta_type;
    DWORD length;
    LARGE_INTEGER offset;
    HRESULT hr = S_OK;
    DWORD delta_time;

    if ((hr = read_variable_length_number(stream, &delta_time, bytes_left)) != S_OK) return hr;
    event->delta_time = delta_time;

    if ((hr = stream_read_at_most(stream, &byte, 1, bytes_left)) != S_OK) return hr;

    if (byte & 0x80)
    {
        event->status = *last_status = byte;
        if ((hr = stream_read_at_most(stream, &byte, 1, bytes_left)) != S_OK) return hr;
    }
    else event->status = *last_status;

    if (event->status == MIDI_META)
    {
        meta_type = byte;

        if ((hr = read_variable_length_number(stream, &length, bytes_left)) != S_OK) return hr;

        switch (meta_type)
        {
        default:
            if (*bytes_left < length) return S_FALSE;
            offset.QuadPart = length;
            if (FAILED(hr = IStream_Seek(stream, offset, STREAM_SEEK_CUR, NULL))) return hr;
            FIXME("MIDI meta event type %#02x, length %lu, time +%lu. not supported\n", meta_type,
                    length, delta_time);
            *bytes_left -= length;
        }
        TRACE("MIDI meta event type %#02x, length %lu, time +%lu\n", meta_type, length, delta_time);
        return S_OK;
    }
    else if (event->status == MIDI_SYSEX1 || event->status == MIDI_SYSEX2)
    {
        if (byte & 0x80)
        {
            if ((hr = read_variable_length_number(stream, &length, bytes_left)) != S_OK) return hr;
            length = length << 8 | (byte & 0x7f);
        }
        else length = byte;

        if (*bytes_left < length) return S_FALSE;
        offset.QuadPart = length;
        if (FAILED(hr = IStream_Seek(stream, offset, STREAM_SEEK_CUR, NULL))) return hr;
        *bytes_left -= length;
        FIXME("MIDI sysex event type %#02x, length %lu, time +%lu. not supported\n", event->status,
                length, delta_time);
        return S_OK;
    }

    status_type = event->status & 0xf0;
    if (status_type == MIDI_PROGRAM_CHANGE)
    {
        event->data[0] = byte;
        TRACE("MIDI program change event status %#02x, data: %#02x, time +%lu\n", event->status,
                event->data[0], delta_time);
    }
    else
    {
        if (status_type != MIDI_CHANNEL_PRESSURE && (hr = stream_read_at_most(stream, &byte, 1, bytes_left)) != S_OK)
            return hr;
        FIXME("MIDI event status %#02x, time +%lu, not supported\n", event->status, delta_time);
    }

    return S_OK;
}

static HRESULT midi_parser_handle_program_change(struct midi_parser *parser, struct midi_event *event)
{
    HRESULT hr;
    DMUS_IO_INSTRUMENT instrument;
    IDirectMusicBand *band;
    DMUS_BAND_PARAM band_param;
    MUSIC_TIME dmusic_time = (ULONGLONG)parser->time * DMUS_PPQ / parser->division;
    instrument.dwPChannel = event->status & 0xf;
    instrument.dwFlags = DMUS_IO_INST_PATCH;
    instrument.dwPatch = event->data[0];
    if (FAILED(hr = CoCreateInstance(&CLSID_DirectMusicBand, NULL, CLSCTX_INPROC_SERVER,
                       &IID_IDirectMusicBand, (void **)&band)))
        return hr;
    hr = band_add_instrument(band, &instrument);

    if (SUCCEEDED(hr))
    {
        TRACE("Adding band at time %lu\n", dmusic_time);
        band_param.pBand = band;
        band_param.mtTimePhysical = dmusic_time;
        hr = IDirectMusicTrack_SetParam(parser->bandtrack, &GUID_BandParam, dmusic_time, &band_param);
    }
    else WARN("Failed to add instrument to band\n");

    IDirectMusicBand_Release(band);
    return hr;
}

static HRESULT midi_parser_parse(struct midi_parser *parser, IDirectMusicSegment8 *segment)
{
    WORD i = 0;
    HRESULT hr;
    MUSIC_TIME music_length = 0;

    TRACE("(%p, %p): semi-stub\n", parser, segment);

    for (i = 0;; i++)
    {
        BYTE magic[4] = {0}, last_status = 0;
        DWORD length_be;
        ULONG length;
        ULONG read = 0;
        struct midi_event event = {0};

        TRACE("Start parsing track %u\n", i);
        if ((hr = IStream_Read(parser->stream, magic, sizeof(magic), &read)) != S_OK) break;
        if (read < sizeof(magic)) break;
        if (memcmp(magic, "MTrk", 4) != 0) break;

        if ((hr = IStream_Read(parser->stream, &length_be, sizeof(length_be), &read)) != S_OK)
            break;
        if (read < sizeof(length_be)) break;
        length = GET_BE_DWORD(length_be);
        TRACE("Track %u, length %lu bytes\n", i, length);

        while ((hr = read_midi_event(parser->stream, &event, &last_status, &length)) == S_OK)
        {
            parser->time += event.delta_time;
            if ((event.status & 0xf0) == MIDI_PROGRAM_CHANGE)
                hr = midi_parser_handle_program_change(parser, &event);
            if (FAILED(hr)) break;
        }

        if (FAILED(hr)) break;
        TRACE("End of track %u\n", i);
        if (parser->time > music_length) music_length = parser->time;
        parser->time = 0;
    }

    TRACE("End of file\n");

    music_length = (ULONGLONG)music_length * DMUS_PPQ / parser->division + 1;
    if (SUCCEEDED(hr)) hr = IDirectMusicSegment8_SetLength(segment, music_length);
    if (SUCCEEDED(hr)) hr = IDirectMusicSegment8_InsertTrack(segment, parser->bandtrack, 0xffff);
    return hr;
}

static void midi_parser_destroy(struct midi_parser *parser)
{
    IStream_Release(parser->stream);
    IDirectMusicTrack_Release(parser->bandtrack);
    free(parser);
}

static HRESULT midi_parser_new(IStream *stream, struct midi_parser **out_parser)
{
    LARGE_INTEGER offset;
    DWORD length;
    HRESULT hr;
    WORD format, number_of_tracks, division;
    struct midi_parser *parser;

    *out_parser = NULL;

    /* Skip over the 'MThd' magic number. */
    offset.QuadPart = 4;
    if (FAILED(hr = IStream_Seek(stream, offset, STREAM_SEEK_SET, NULL))) return hr;

    if (FAILED(hr = IStream_Read(stream, &length, sizeof(length), NULL))) return hr;
    length = GET_BE_DWORD(length);

    if (FAILED(hr = IStream_Read(stream, &format, sizeof(format), NULL))) return hr;
    format = GET_BE_WORD(format);

    if (FAILED(hr = IStream_Read(stream, &number_of_tracks, sizeof(number_of_tracks), NULL)))
        return hr;

    number_of_tracks = GET_BE_WORD(number_of_tracks);
    if (FAILED(hr = IStream_Read(stream, &division, sizeof(division), NULL))) return hr;
    division = GET_BE_WORD(division);
    if (division & 0x8000)
    {
        WARN("SMPTE time division not implemented yet\n");
        return DMUS_E_UNSUPPORTED_STREAM;
    }

    TRACE("MIDI format %u, %u tracks, division %u\n", format, number_of_tracks, division);

    parser = calloc(1, sizeof(struct midi_parser));
    if (!parser) return E_OUTOFMEMORY;
    parser->stream = stream;
    parser->division = division;
    hr = CoCreateInstance(&CLSID_DirectMusicBandTrack, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicTrack, (void **)&parser->bandtrack);
    if (FAILED(hr))
    {
        free(parser);
        return hr;
    }

    *out_parser = parser;
    IStream_AddRef(stream);
    return hr;
}

HRESULT parse_midi(IStream *stream, IDirectMusicSegment8 *segment)
{
    struct midi_parser *parser;
    HRESULT hr;

    if (FAILED(hr = midi_parser_new(stream, &parser))) return hr;
    hr = midi_parser_parse(parser, segment);
    midi_parser_destroy(parser);
    return hr;
}
