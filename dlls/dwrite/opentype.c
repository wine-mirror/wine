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

#include "dwrite_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

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

typedef struct {
    DWORD version;
    WORD numTables;
    WORD searchRange;
    WORD entrySelector;
    WORD rangeShift;
} TTC_SFNT_V1;

typedef struct {
    CHAR tag[4];
    DWORD checkSum;
    DWORD offset;
    DWORD length;
} TT_TableRecord;

typedef struct {
    WORD platformID;
    WORD encodingID;
    DWORD offset;
} CMAP_EncodingRecord;

typedef struct {
    WORD version;
    WORD numTables;
    CMAP_EncodingRecord tables[1];
} CMAP_Header;

typedef struct {
    DWORD startCharCode;
    DWORD endCharCode;
    DWORD startGlyphID;
} CMAP_SegmentedCoverage_group;

typedef struct {
    WORD format;
    WORD reserved;
    DWORD length;
    DWORD language;
    DWORD nGroups;
    CMAP_SegmentedCoverage_group groups[1];
} CMAP_SegmentedCoverage;

typedef struct {
    WORD format;
    WORD length;
    WORD language;
    WORD segCountX2;
    WORD searchRange;
    WORD entrySelector;
    WORD rangeShift;
    WORD endCode[1];
} CMAP_SegmentMapping_0;

enum OPENTYPE_CMAP_TABLE_FORMAT
{
    OPENTYPE_CMAP_TABLE_SEGMENT_MAPPING = 4,
    OPENTYPE_CMAP_TABLE_SEGMENTED_COVERAGE = 12
};

/* PANOSE is 10 bytes in size, need to pack the structure properly */
#include "pshpack2.h"
typedef struct
{
    ULONG version;
    ULONG revision;
    ULONG checksumadj;
    ULONG magic;
    USHORT flags;
    USHORT unitsPerEm;
    ULONGLONG created;
    ULONGLONG modified;
    SHORT xMin;
    SHORT yMin;
    SHORT xMax;
    SHORT yMax;
    USHORT macStyle;
    USHORT lowestRecPPEM;
    SHORT direction_hint;
    SHORT index_format;
    SHORT glyphdata_format;
} TT_HEAD;

typedef struct
{
    ULONG Version;
    ULONG italicAngle;
    SHORT underlinePosition;
    SHORT underlineThickness;
    ULONG fixed_pitch;
    ULONG minmemType42;
    ULONG maxmemType42;
    ULONG minmemType1;
    ULONG maxmemType1;
} TT_POST;

typedef struct
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
    /* version 2 (OpenType 1.2) */
    SHORT sxHeight;
    SHORT sCapHeight;
    USHORT usDefaultChar;
    USHORT usBreakChar;
    USHORT usMaxContext;
} TT_OS2_V2;
#include "poppack.h"

HRESULT opentype_analyze_font(IDWriteFontFileStream *stream, UINT32* font_count, DWRITE_FONT_FILE_TYPE *file_type, DWRITE_FONT_FACE_TYPE *face_type, BOOL *supported)
{
    /* TODO: Do font validation */
    const void *font_data;
    const char* tag;
    void *context;
    HRESULT hr;

    hr = IDWriteFontFileStream_ReadFileFragment(stream, &font_data, 0, sizeof(TTC_Header_V1), &context);
    if (FAILED(hr))
        return hr;

    tag = font_data;
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

    IDWriteFontFileStream_ReleaseFileFragment(stream, context);
    return S_OK;
}

HRESULT opentype_get_font_table(IDWriteFontFileStream *stream, DWRITE_FONT_FACE_TYPE type, UINT32 font_index, UINT32 tag,
    const void **table_data, void **table_context, UINT32 *table_size, BOOL *found)
{
    HRESULT hr;
    TTC_SFNT_V1 *font_header = NULL;
    void *sfnt_context;
    TT_TableRecord *table_record = NULL;
    void *table_record_context;
    int table_count, table_offset = 0;
    int i;

    *found = FALSE;

    if (type == DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION) {
        const TTC_Header_V1 *ttc_header;
        void * ttc_context;
        hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&ttc_header, 0, sizeof(*ttc_header), &ttc_context);
        if (SUCCEEDED(hr)) {
            table_offset = GET_BE_DWORD(ttc_header->OffsetTable[0]);
            if (font_index >= GET_BE_DWORD(ttc_header->numFonts))
                hr = E_INVALIDARG;
            else
                hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&font_header, table_offset, sizeof(*font_header), &sfnt_context);
            IDWriteFontFileStream_ReleaseFileFragment(stream, ttc_context);
        }
    }
    else
        hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&font_header, 0, sizeof(*font_header), &sfnt_context);

    if (FAILED(hr))
        return hr;

    table_count = GET_BE_WORD(font_header->numTables);
    table_offset += sizeof(*font_header);
    for (i = 0; i < table_count; i++)
    {
        hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&table_record, table_offset, sizeof(*table_record), &table_record_context);
        if (FAILED(hr))
            break;
        if (DWRITE_MAKE_OPENTYPE_TAG(table_record->tag[0], table_record->tag[1], table_record->tag[2], table_record->tag[3]) == tag)
            break;
        IDWriteFontFileStream_ReleaseFileFragment(stream, table_record_context);
        table_offset += sizeof(*table_record);
    }

    IDWriteFontFileStream_ReleaseFileFragment(stream, sfnt_context);
    if (SUCCEEDED(hr) && i < table_count)
    {
        int offset = GET_BE_DWORD(table_record->offset);
        int length = GET_BE_DWORD(table_record->length);
        IDWriteFontFileStream_ReleaseFileFragment(stream, table_record_context);

        *found = TRUE;
        *table_size = length;
        hr = IDWriteFontFileStream_ReadFileFragment(stream, table_data, offset, length, table_context);
    }

    return hr;
}

/**********
 * CMAP
 **********/

static int compare_group(const void *a, const void* b)
{
    const DWORD *chr = a;
    const CMAP_SegmentedCoverage_group *group = b;

    if (*chr < GET_BE_DWORD(group->startCharCode))
        return -1;
    if (*chr > GET_BE_DWORD(group->endCharCode))
        return 1;
    return 0;
}

static void CMAP4_GetGlyphIndex(CMAP_SegmentMapping_0* format, UINT32 utf32c, UINT16 *pgi)
{
    WORD *startCode;
    SHORT *idDelta;
    WORD *idRangeOffset;
    int segment;

    int segment_count = GET_BE_WORD(format->segCountX2)/2;
    /* This is correct because of the padding before startCode */
    startCode = (WORD*)((BYTE*)format + sizeof(CMAP_SegmentMapping_0) + (sizeof(WORD) * segment_count));
    idDelta = (SHORT*)(((BYTE*)startCode) + (sizeof(WORD) * segment_count));
    idRangeOffset = (WORD*)(((BYTE*)idDelta) + (sizeof(WORD) * segment_count));

    segment = 0;
    while(GET_BE_WORD(format->endCode[segment]) < 0xffff)
    {
        if (utf32c <= GET_BE_WORD(format->endCode[segment]))
            break;
        segment++;
    }
    if (segment >= segment_count)
        return;
    TRACE("Segment %i of %i\n",segment, segment_count);
    if (GET_BE_WORD(startCode[segment]) > utf32c)
        return;
    TRACE("In range %i -> %i\n", GET_BE_WORD(startCode[segment]), GET_BE_WORD(format->endCode[segment]));
    if (GET_BE_WORD(idRangeOffset[segment]) == 0)
    {
        *pgi = (SHORT)(GET_BE_WORD(idDelta[segment])) + utf32c;
    }
    else
    {
        WORD ro = GET_BE_WORD(idRangeOffset[segment])/2;
        WORD co =  (utf32c - GET_BE_WORD(startCode[segment]));
        WORD *index = (WORD*)((BYTE*)&idRangeOffset[segment] + (ro + co));
        *pgi = GET_BE_WORD(*index);
    }
}

static void CMAP12_GetGlyphIndex(CMAP_SegmentedCoverage* format, UINT32 utf32c, UINT16 *pgi)
{
    CMAP_SegmentedCoverage_group *group;

    group = bsearch(&utf32c, format->groups, GET_BE_DWORD(format->nGroups),
                    sizeof(CMAP_SegmentedCoverage_group), compare_group);

    if (group)
    {
        DWORD offset = utf32c - GET_BE_DWORD(group->startCharCode);
        *pgi = GET_BE_DWORD(group->startGlyphID) + offset;
    }
}

void opentype_cmap_get_glyphindex(void *data, UINT32 utf32c, UINT16 *pgi)
{
    CMAP_Header *CMAP_Table = data;
    int i;

    *pgi = 0;

    for (i = 0; i < GET_BE_WORD(CMAP_Table->numTables); i++)
    {
        WORD type;
        WORD *table;

        if (GET_BE_WORD(CMAP_Table->tables[i].platformID) != 3)
            continue;

        table = (WORD*)(((BYTE*)CMAP_Table) + GET_BE_DWORD(CMAP_Table->tables[i].offset));
        type = GET_BE_WORD(*table);
        TRACE("table type %i\n", type);
        /* Break when we find a handled type */
        switch (type)
        {
            case OPENTYPE_CMAP_TABLE_SEGMENT_MAPPING:
                CMAP4_GetGlyphIndex((CMAP_SegmentMapping_0*) table, utf32c, pgi);
                break;
            case OPENTYPE_CMAP_TABLE_SEGMENTED_COVERAGE:
                CMAP12_GetGlyphIndex((CMAP_SegmentedCoverage*) table, utf32c, pgi);
                break;
            default:
                TRACE("table type %i unhandled.\n", type);
        }
    }
}

static UINT32 opentype_cmap_get_unicode_ranges_count(const CMAP_Header *CMAP_Table)
{
    UINT32 count = 0;
    int i;

    for (i = 0; i < GET_BE_WORD(CMAP_Table->numTables); i++) {
        WORD type;
        WORD *table;

        if (GET_BE_WORD(CMAP_Table->tables[i].platformID) != 3)
            continue;

        table = (WORD*)(((BYTE*)CMAP_Table) + GET_BE_DWORD(CMAP_Table->tables[i].offset));
        type = GET_BE_WORD(*table);
        TRACE("table type %i\n", type);

        switch (type)
        {
            case OPENTYPE_CMAP_TABLE_SEGMENT_MAPPING:
            {
                CMAP_SegmentMapping_0 *format = (CMAP_SegmentMapping_0*)table;
                count += GET_BE_WORD(format->segCountX2)/2;
                break;
            }
            case OPENTYPE_CMAP_TABLE_SEGMENTED_COVERAGE:
            {
                CMAP_SegmentedCoverage *format = (CMAP_SegmentedCoverage*)table;
                count += GET_BE_DWORD(format->nGroups);
                break;
            }
            default:
                FIXME("table type %i unhandled.\n", type);
        }
    }

    return count;
}

HRESULT opentype_cmap_get_unicode_ranges(void *data, UINT32 max_count, DWRITE_UNICODE_RANGE *ranges, UINT32 *count)
{
    CMAP_Header *CMAP_Table = data;
    int i, k = 0;

    if (!CMAP_Table)
        return E_FAIL;

    *count = opentype_cmap_get_unicode_ranges_count(CMAP_Table);

    for (i = 0; i < GET_BE_WORD(CMAP_Table->numTables) && k < max_count; i++)
    {
        WORD type;
        WORD *table;
        int j;

        if (GET_BE_WORD(CMAP_Table->tables[i].platformID) != 3)
            continue;

        table = (WORD*)(((BYTE*)CMAP_Table) + GET_BE_DWORD(CMAP_Table->tables[i].offset));
        type = GET_BE_WORD(*table);
        TRACE("table type %i\n", type);

        switch (type)
        {
            case OPENTYPE_CMAP_TABLE_SEGMENT_MAPPING:
            {
                CMAP_SegmentMapping_0 *format = (CMAP_SegmentMapping_0*)table;
                UINT16 segment_count = GET_BE_WORD(format->segCountX2)/2;
                UINT16 *startCode = (WORD*)((BYTE*)format + sizeof(CMAP_SegmentMapping_0) + (sizeof(WORD) * segment_count));

                for (j = 0; j < segment_count && GET_BE_WORD(format->endCode[j]) < 0xffff && k < max_count; j++, k++) {
                    ranges[k].first = GET_BE_WORD(startCode[j]);
                    ranges[k].last  = GET_BE_WORD(format->endCode[j]);
                }
                break;
            }
            case OPENTYPE_CMAP_TABLE_SEGMENTED_COVERAGE:
            {
                CMAP_SegmentedCoverage *format = (CMAP_SegmentedCoverage*)table;
                for (j = 0; j < GET_BE_DWORD(format->nGroups) && k < max_count; j++, k++) {
                    ranges[k].first = GET_BE_DWORD(format->groups[j].startCharCode);
                    ranges[k].last  = GET_BE_DWORD(format->groups[j].endCharCode);
                }
                break;
            }
            default:
                FIXME("table type %i unhandled.\n", type);
        }
    }

    return *count > max_count ? E_NOT_SUFFICIENT_BUFFER : S_OK;
}

VOID get_font_properties(LPCVOID os2, LPCVOID head, LPCVOID post, DWRITE_FONT_METRICS *metrics, DWRITE_FONT_STRETCH *stretch, DWRITE_FONT_WEIGHT *weight, DWRITE_FONT_STYLE *style)
{
    TT_OS2_V2 *tt_os2 = (TT_OS2_V2*)os2;
    TT_HEAD *tt_head = (TT_HEAD*)head;
    TT_POST *tt_post = (TT_POST*)post;

    /* default stretch, weight and style to normal */
    *stretch = DWRITE_FONT_STRETCH_NORMAL;
    *weight = DWRITE_FONT_WEIGHT_NORMAL;
    *style = DWRITE_FONT_STYLE_NORMAL;

    memset(metrics, 0, sizeof(*metrics));

    /* DWRITE_FONT_STRETCH enumeration values directly match font data values */
    if (tt_os2)
    {
        if (GET_BE_WORD(tt_os2->usWidthClass) <= DWRITE_FONT_STRETCH_ULTRA_EXPANDED)
            *stretch = GET_BE_WORD(tt_os2->usWidthClass);

        *weight = GET_BE_WORD(tt_os2->usWeightClass);
        TRACE("stretch=%d, weight=%d\n", *stretch, *weight);

        metrics->ascent    = GET_BE_WORD(tt_os2->sTypoAscender);
        metrics->descent   = GET_BE_WORD(tt_os2->sTypoDescender);
        metrics->lineGap   = GET_BE_WORD(tt_os2->sTypoLineGap);
        metrics->capHeight = GET_BE_WORD(tt_os2->sCapHeight);
        metrics->xHeight   = GET_BE_WORD(tt_os2->sxHeight);
        metrics->strikethroughPosition  = GET_BE_WORD(tt_os2->yStrikeoutPosition);
        metrics->strikethroughThickness = GET_BE_WORD(tt_os2->yStrikeoutSize);
    }

    if (tt_head)
    {
        USHORT macStyle = GET_BE_WORD(tt_head->macStyle);
        metrics->designUnitsPerEm = GET_BE_WORD(tt_head->unitsPerEm);
        if (macStyle & 0x0002)
            *style = DWRITE_FONT_STYLE_ITALIC;

    }

    if (tt_post)
    {
        metrics->underlinePosition = GET_BE_WORD(tt_post->underlinePosition);
        metrics->underlineThickness = GET_BE_WORD(tt_post->underlineThickness);
    }
}
