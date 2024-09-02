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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include "initguid.h"
#include "unix_private.h"

#include "d3d9.h"
#include "mfapi.h"
#include "uuids.h"

#include "wine/debug.h"

#ifdef HAVE_FFMPEG

WINE_DEFAULT_DEBUG_CHANNEL(dmo);

#define TRACE_HEXDUMP( data, size )                                                               \
    if (__WINE_IS_DEBUG_ON(_TRACE, __wine_dbch___default))                                        \
    do {                                                                                          \
        const unsigned char *__ptr, *__end, *__tmp;                                               \
        for (__ptr = (void *)(data), __end = __ptr + (size); __ptr < __end; __ptr += 16)          \
        {                                                                                         \
            char __buf[256], *__lim = __buf + sizeof(__buf), *__out = __buf;                      \
            __out += snprintf( __out, __lim - __out, "%08zx ", (char *)__ptr - (char *)data );    \
            for (__tmp = __ptr; __tmp < __end && __tmp < __ptr + 16; ++__tmp)                     \
                __out += snprintf( __out, __lim - __out, " %02x", *__tmp );                       \
            memset( __out, ' ', (__ptr + 16 - __tmp) * 3 + 2 );                                   \
            __out += (__ptr + 16 - __tmp) * 3 + 2;                                                \
            for (__tmp = __ptr; __tmp < __end && __tmp < __ptr + 16; ++__tmp)                     \
                *__out++ = *__tmp >= ' ' && *__tmp <= '~' ? *__tmp : '.';                         \
            *__out++ = 0;                                                                         \
            TRACE( "%s\n", __buf );                                                               \
        }                                                                                         \
    } while (0)

NTSTATUS media_type_from_codec_params( const AVCodecParameters *params, const AVRational *sar, const AVRational *fps,
                                       UINT32 align, struct media_type *media_type )
{
    TRACE( "codec type %#x, id %#x (%s), tag %#x (%s)\n", params->codec_type, params->codec_id, avcodec_get_name(params->codec_id),
           params->codec_tag, debugstr_fourcc(params->codec_tag) );
    if (params->extradata_size) TRACE_HEXDUMP( params->extradata, params->extradata_size );

    if (params->codec_type == AVMEDIA_TYPE_AUDIO)
    {
        media_type->major = MFMediaType_Audio;
        return STATUS_SUCCESS;
    }

    if (params->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        media_type->major = MFMediaType_Video;
        return STATUS_SUCCESS;
    }

    FIXME( "Unknown type %#x\n", params->codec_type );
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* HAVE_FFMPEG */
