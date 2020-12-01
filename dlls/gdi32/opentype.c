/*
 * Copyright 2020 Rémi Bernon for CodeWeavers
 * Copyright 2014 Aric Stewart for CodeWeavers
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

#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"

#include "gdi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(font);

#define MS_OTTO_TAG MS_MAKE_TAG('O','T','T','O')
#define MS_HEAD_TAG MS_MAKE_TAG('h','e','a','d')
#define MS_HHEA_TAG MS_MAKE_TAG('h','h','e','a')
#define MS_OS_2_TAG MS_MAKE_TAG('O','S','/','2')
#define MS_EBSC_TAG MS_MAKE_TAG('E','B','S','C')
#define MS_EBDT_TAG MS_MAKE_TAG('E','B','D','T')
#define MS_CBDT_TAG MS_MAKE_TAG('C','B','D','T')

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_WORD(x)  RtlUshortByteSwap(x)
#define GET_BE_DWORD(x) RtlUlongByteSwap(x)
#endif

#include "pshpack2.h"
struct ttc_header_v1
{
    CHAR TTCTag[4];
    DWORD Version;
    DWORD numFonts;
    DWORD OffsetTable[1];
};

struct ttc_sfnt_v1
{
    DWORD version;
    WORD numTables;
    WORD searchRange;
    WORD entrySelector;
    WORD rangeShift;
};

struct tt_tablerecord
{
    DWORD tag;
    DWORD checkSum;
    DWORD offset;
    DWORD length;
};

struct tt_os2_v1
{
    USHORT version;
    SHORT xAvgCharWidth;
    USHORT usWeightClass;
    USHORT usWidthClass;
    SHORT fsType;
    SHORT ySubscriptXSize;
    SHORT ySubscriptYSize;
    SHORT ySubscriptXOffset;
    SHORT ySubscriptYOffset;
    SHORT ySuperscriptXSize;
    SHORT ySuperscriptYSize;
    SHORT ySuperscriptXOffset;
    SHORT ySuperscriptYOffset;
    SHORT yStrikeoutSize;
    SHORT yStrikeoutPosition;
    SHORT sFamilyClass;
    PANOSE panose;
    ULONG ulUnicodeRange1;
    ULONG ulUnicodeRange2;
    ULONG ulUnicodeRange3;
    ULONG ulUnicodeRange4;
    CHAR achVendID[4];
    USHORT fsSelection;
    USHORT usFirstCharIndex;
    USHORT usLastCharIndex;
    /* According to the Apple spec, original version didn't have the below fields,
     * version numbers were taken from the OpenType spec.
     */
    /* version 0 (TrueType 1.5) */
    USHORT sTypoAscender;
    USHORT sTypoDescender;
    USHORT sTypoLineGap;
    USHORT usWinAscent;
    USHORT usWinDescent;
    /* version 1 (TrueType 1.66) */
    ULONG ulCodePageRange1;
    ULONG ulCodePageRange2;
};
#include "poppack.h"

static BOOL opentype_get_table_ptr( const void *data, size_t size, const struct ttc_sfnt_v1 *ttc_sfnt_v1,
                                    UINT32 table_tag, const void **table_ptr, UINT32 *table_size )
{
    const struct tt_tablerecord *table_record;
    UINT16 i, table_count;
    UINT32 offset, length;

    if (!ttc_sfnt_v1) return FALSE;

    table_record = (const struct tt_tablerecord *)(ttc_sfnt_v1 + 1);
    table_count = GET_BE_WORD( ttc_sfnt_v1->numTables );
    for (i = 0; i < table_count; i++, table_record++)
    {
        if (table_record->tag != table_tag) continue;
        offset = GET_BE_DWORD( table_record->offset );
        length = GET_BE_DWORD( table_record->length );
        if (size < offset + length) return FALSE;
        if (table_size && length < *table_size) return FALSE;

        if (table_ptr) *table_ptr = (const char *)data + offset;
        if (table_size) *table_size = length;
        return TRUE;
    }

    return FALSE;
}

static BOOL opentype_get_tt_os2_v1( const void *data, size_t size, const struct ttc_sfnt_v1 *ttc_sfnt_v1,
                                    const struct tt_os2_v1 **tt_os2_v1 )
{
    UINT32 table_size = sizeof(**tt_os2_v1);
    return opentype_get_table_ptr( data, size, ttc_sfnt_v1, MS_OS_2_TAG, (const void **)tt_os2_v1, &table_size );
}

BOOL opentype_get_ttc_sfnt_v1( const void *data, size_t size, DWORD index, DWORD *count, const struct ttc_sfnt_v1 **ttc_sfnt_v1 )
{
    const struct ttc_header_v1 *ttc_header_v1 = data;
    const struct tt_os2_v1 *tt_os2_v1;
    UINT32 offset, fourcc;

    *ttc_sfnt_v1 = NULL;
    *count = 1;

    if (size < sizeof(fourcc)) return FALSE;
    memcpy( &fourcc, data, sizeof(fourcc) );

    switch (fourcc)
    {
    default:
        WARN( "unsupported font format %x\n", fourcc );
        return FALSE;
    case MS_TTCF_TAG:
        if (size < sizeof(ttc_header_v1)) return FALSE;
        if (index >= (*count = GET_BE_DWORD( ttc_header_v1->numFonts ))) return FALSE;
        offset = GET_BE_DWORD( ttc_header_v1->OffsetTable[index] );
        break;
    case 0x00000100:
    case MS_OTTO_TAG:
        offset = 0;
        break;
    }

    if (size < offset + sizeof(**ttc_sfnt_v1)) return FALSE;
    *ttc_sfnt_v1 = (const struct ttc_sfnt_v1 *)((const char *)data + offset);

    if (!opentype_get_table_ptr( data, size, *ttc_sfnt_v1, MS_HEAD_TAG, NULL, NULL ))
    {
        WARN( "unsupported sfnt font: missing head table.\n" );
        return FALSE;
    }

    if (!opentype_get_table_ptr( data, size, *ttc_sfnt_v1, MS_HHEA_TAG, NULL, NULL ))
    {
        WARN( "unsupported sfnt font: missing hhea table.\n" );
        return FALSE;
    }

    if (!opentype_get_tt_os2_v1( data, size, *ttc_sfnt_v1, &tt_os2_v1 ))
    {
        WARN( "unsupported sfnt font: missing OS/2 table.\n" );
        return FALSE;
    }

    /* Wine uses ttfs as an intermediate step in building its bitmap fonts;
       we don't want to load these. */
    if (!memcmp( tt_os2_v1->achVendID, "Wine", sizeof(tt_os2_v1->achVendID) ) &&
        opentype_get_table_ptr( data, size, *ttc_sfnt_v1, MS_EBSC_TAG, NULL, NULL ))
    {
        TRACE( "ignoring wine bitmap-only sfnt font.\n" );
        return FALSE;
    }

    if (opentype_get_table_ptr( data, size, *ttc_sfnt_v1, MS_EBDT_TAG, NULL, NULL ) ||
        opentype_get_table_ptr( data, size, *ttc_sfnt_v1, MS_CBDT_TAG, NULL, NULL ))
    {
        WARN( "unsupported sfnt font: embedded bitmap data.\n" );
        return FALSE;
    }

    return TRUE;
}
