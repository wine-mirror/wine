/*
 *    Methods for dealing with opentype font tables
 *
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

#define COBJMACROS

#include "dwrite.h"
#include "dwrite_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

#define DWRITE_MAKE_OPENTYPE_TAG(ch0, ch1, ch2, ch3) \
                    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
                    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))

#define MS_TTCF_TAG DWRITE_MAKE_OPENTYPE_TAG('t','t','c','f')
#define MS_OTTO_TAG DWRITE_MAKE_OPENTYPE_TAG('O','T','T','O')

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_WORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define GET_BE_DWORD(x) MAKELONG(GET_BE_WORD(HIWORD(x)), GET_BE_WORD(LOWORD(x)))
#endif

typedef struct {
    CHAR TTCTag[4];
    DWORD Version;
    DWORD numFonts;
    DWORD OffsetTable[1];
} TTC_Header_V1;

HRESULT analyze_opentype_font(const void* font_data, UINT32* font_count, DWRITE_FONT_FILE_TYPE *file_type, DWRITE_FONT_FACE_TYPE *face_type, BOOL *supported)
{
    /* TODO: Do font validation */
    const char* tag = font_data;

    *supported = FALSE;
    *file_type = DWRITE_FONT_FILE_TYPE_UNKNOWN;
    if (face_type)
        *face_type = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    *font_count = 0;

    if (DWRITE_MAKE_OPENTYPE_TAG(tag[0], tag[1], tag[2], tag[3]) == MS_TTCF_TAG)
    {
        const TTC_Header_V1 *header = font_data;
        *font_count = GET_BE_DWORD(header->numFonts);
        *file_type = DWRITE_FONT_FILE_TYPE_TRUETYPE_COLLECTION;
        if (face_type)
            *face_type = DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION;
        *supported = TRUE;
    }
    else if (GET_BE_DWORD(*(DWORD*)font_data) == 0x10000)
    {
        *font_count = 1;
        *file_type = DWRITE_FONT_FILE_TYPE_TRUETYPE;
        if (face_type)
            *face_type = DWRITE_FONT_FACE_TYPE_TRUETYPE;
        *supported = TRUE;
    }
    else if (DWRITE_MAKE_OPENTYPE_TAG(tag[0], tag[1], tag[2], tag[3]) == MS_OTTO_TAG)
    {
        *file_type = DWRITE_FONT_FILE_TYPE_CFF;
    }
    return S_OK;
}
