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

#include "dmime_private.h"
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_WORD(x) RtlUshortByteSwap(x)
#define GET_BE_DWORD(x) RtlUlongByteSwap(x)
#endif

struct midi_parser
{
    IStream *stream;
};

HRESULT midi_parser_next_track(struct midi_parser *parser, IDirectMusicTrack **out_track, MUSIC_TIME *out_length)
{
    TRACE("(%p, %p, %p): stub\n", parser, out_track, out_length);
    return S_FALSE;
}

HRESULT midi_parser_new(IStream *stream, struct midi_parser **out_parser)
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
    if (length != 6)
    {
        WARN("Invalid MIDI header length %lu\n", length);
        return DMUS_E_UNSUPPORTED_STREAM;
    }

    if (FAILED(hr = IStream_Read(stream, &format, sizeof(format), NULL))) return hr;
    format = GET_BE_WORD(format);
    if (format > 2)
    {
        WARN("Invalid MIDI format %u\n", format);
        return DMUS_E_UNSUPPORTED_STREAM;
    }
    if (format == 2)
    {
        FIXME("MIDI format 2 not implemented yet\n");
        return DMUS_E_UNSUPPORTED_STREAM;
    }

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
    IStream_AddRef(stream);
    *out_parser = parser;

    return hr;
}

void midi_parser_destroy(struct midi_parser *parser)
{
    IStream_Release(parser->stream);
    free(parser);
}
