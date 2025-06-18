/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2004, 2005 Hans Leidekker
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "icm.h"

#include "mscms_priv.h"

static inline void adjust_endianness32( ULONG *ptr )
{
#ifndef WORDS_BIGENDIAN
    *ptr = RtlUlongByteSwap(*ptr);
#endif
}

static const struct tag_entry *first_tag( const struct profile *profile )
{
    return (const struct tag_entry *)(profile->data + sizeof(PROFILEHEADER) + sizeof(DWORD));
}

void get_profile_header( const struct profile *profile, PROFILEHEADER *header )
{
    unsigned int i;

    memcpy( header, profile->data, sizeof(PROFILEHEADER) );

    /* ICC format is big-endian, swap bytes if necessary */
    for (i = 0; i < sizeof(PROFILEHEADER) / sizeof(ULONG); i++)
        adjust_endianness32( (ULONG *)header + i );
}

void set_profile_header( const struct profile *profile, const PROFILEHEADER *header )
{
    unsigned int i;

    memcpy( profile->data, header, sizeof(PROFILEHEADER) );

    /* ICC format is big-endian, swap bytes if necessary */
    for (i = 0; i < sizeof(PROFILEHEADER) / sizeof(ULONG); i++)
        adjust_endianness32( (ULONG *)profile->data + i );
}

DWORD get_tag_count( const struct profile *profile )
{
    DWORD num_tags = *(DWORD *)(profile->data + sizeof(PROFILEHEADER));
    adjust_endianness32( &num_tags );
    if ((const BYTE *)(first_tag( profile ) + num_tags) > (const BYTE *)profile->data + profile->size)
        return 0;
    return num_tags;
}

BOOL get_tag_entry( const struct profile *profile, DWORD index, struct tag_entry *tag )
{
    const struct tag_entry *entry = first_tag( profile );

    if (index < 1 || index > get_tag_count( profile )) return FALSE;
    *tag = entry[index - 1];
    adjust_endianness32( &tag->sig );
    adjust_endianness32( &tag->offset );
    adjust_endianness32( &tag->size );
    if (tag->offset > profile->size || tag->size > profile->size - tag->offset) return FALSE;
    return TRUE;
}

BOOL get_adjusted_tag( const struct profile *profile, TAGTYPE type, struct tag_entry *tag )
{
    const struct tag_entry *entry = first_tag( profile );
    DWORD sig, i;

    for (i = get_tag_count(profile); i > 0; i--, entry++)
    {
        sig = entry->sig;
        adjust_endianness32( &sig );
        if (sig == type)
        {
            *tag = *entry;
            adjust_endianness32( &tag->sig );
            adjust_endianness32( &tag->offset );
            adjust_endianness32( &tag->size );
            if (tag->offset > profile->size || tag->size > profile->size - tag->offset) return FALSE;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL get_linked_tag( const struct profile *profile, struct tag_entry *ret )
{
    const struct tag_entry *entry = first_tag( profile );
    DWORD sig, offset, size, i;

    for (i = get_tag_count(profile); i > 0; i--, entry++)
    {
        sig = entry->sig;
        adjust_endianness32( &sig );
        if (sig == ret->sig) continue;
        offset = entry->offset;
        size = entry->size;
        adjust_endianness32( &offset );
        adjust_endianness32( &size );
        if (size == ret->size && offset == ret->offset)
        {
            ret->sig = sig;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL get_tag_data( const struct profile *profile, TAGTYPE type, DWORD offset, void *buffer,
                   DWORD *len, BOOL *linked )
{
    struct tag_entry tag;

    if (!get_adjusted_tag( profile, type, &tag )) return FALSE;

    if (!buffer) offset = 0;
    if (offset > tag.size) return FALSE;
    if (*len < tag.size - offset || !buffer)
    {
        *len = tag.size - offset;
        return FALSE;
    }
    memcpy( buffer, profile->data + tag.offset + offset, tag.size - offset );
    *len = tag.size - offset;
    if (linked) *linked = get_linked_tag( profile, &tag );
    return TRUE;
}

BOOL set_tag_data( const struct profile *profile, TAGTYPE type, DWORD offset, const void *buffer, DWORD *len )
{
    struct tag_entry tag;

    if (!get_adjusted_tag( profile, type, &tag )) return FALSE;

    if (offset > tag.size) return FALSE;
    *len = min( tag.size - offset, *len );
    memcpy( profile->data + tag.offset + offset, buffer, *len );
    return TRUE;
}
