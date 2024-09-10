/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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

#ifndef __WINE_WINEDMO_H
#define __WINE_WINEDMO_H

#include <stddef.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"

#include "mfapi.h"

union winedmo_format
{
    WAVEFORMATEX audio;
    MFVIDEOFORMAT video;
};

struct winedmo_stream
{
    NTSTATUS (CDECL *p_seek)( struct winedmo_stream *stream, UINT64 *pos );
    NTSTATUS (CDECL *p_read)( struct winedmo_stream *stream, BYTE *buffer, ULONG *size );
};

struct winedmo_demuxer { UINT64 handle; };

NTSTATUS CDECL winedmo_demuxer_check( const char *mime_type );
NTSTATUS CDECL winedmo_demuxer_create( const WCHAR *url, struct winedmo_stream *stream, UINT64 stream_size, INT64 *duration,
                                       UINT *stream_count, WCHAR *mime_type, struct winedmo_demuxer *demuxer );
NTSTATUS CDECL winedmo_demuxer_destroy( struct winedmo_demuxer *demuxer );
NTSTATUS CDECL winedmo_demuxer_read( struct winedmo_demuxer demuxer, UINT *stream, DMO_OUTPUT_DATA_BUFFER *buffer, UINT *buffer_size );
NTSTATUS CDECL winedmo_demuxer_seek( struct winedmo_demuxer demuxer, INT64 timestamp );
NTSTATUS CDECL winedmo_demuxer_stream_lang( struct winedmo_demuxer demuxer, UINT stream, WCHAR *buffer, UINT len );
NTSTATUS CDECL winedmo_demuxer_stream_name( struct winedmo_demuxer demuxer, UINT stream, WCHAR *buffer, UINT len );
NTSTATUS CDECL winedmo_demuxer_stream_type( struct winedmo_demuxer demuxer, UINT stream,
                                            GUID *major, union winedmo_format **format );

#endif /* __WINE_WINEDMO_H */
