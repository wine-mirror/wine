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

struct winedmo_stream
{
    NTSTATUS (CDECL *p_seek)( struct winedmo_stream *stream, UINT64 *pos );
    NTSTATUS (CDECL *p_read)( struct winedmo_stream *stream, BYTE *buffer, ULONG *size );
};

struct winedmo_demuxer { UINT64 handle; };

NTSTATUS CDECL winedmo_demuxer_check( const char *mime_type );
NTSTATUS CDECL winedmo_demuxer_create( const WCHAR *url, struct winedmo_stream *stream, UINT64 *stream_size,
                                       WCHAR *mime_type, struct winedmo_demuxer *demuxer );
NTSTATUS CDECL winedmo_demuxer_destroy( struct winedmo_demuxer *demuxer );

#endif /* __WINE_WINEDMO_H */
