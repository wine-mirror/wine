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

typedef struct {
    WORD platformID;
    WORD encodingID;
    WORD languageID;
    WORD nameID;
    WORD length;
    WORD offset;
} TT_NameRecord;

typedef struct {
    WORD format;
    WORD count;
    WORD stringOffset;
    TT_NameRecord nameRecord[1];
} TT_NAME_V0;

enum OPENTYPE_PLATFORM_ID
{
    OPENTYPE_PLATFORM_UNICODE = 0,
    OPENTYPE_PLATFORM_MAC,
    OPENTYPE_PLATFORM_ISO,
    OPENTYPE_PLATFORM_WIN,
    OPENTYPE_PLATFORM_CUSTOM
};

enum TT_NAME_WINDOWS_ENCODING_ID
{
    TT_NAME_WINDOWS_ENCODING_SYMBOL = 0,
    TT_NAME_WINDOWS_ENCODING_UCS2,
    TT_NAME_WINDOWS_ENCODING_SJIS,
    TT_NAME_WINDOWS_ENCODING_PRC,
    TT_NAME_WINDOWS_ENCODING_BIG5,
    TT_NAME_WINDOWS_ENCODING_WANSUNG,
    TT_NAME_WINDOWS_ENCODING_JOHAB,
    TT_NAME_WINDOWS_ENCODING_RESERVED1,
    TT_NAME_WINDOWS_ENCODING_RESERVED2,
    TT_NAME_WINDOWS_ENCODING_RESERVED3,
    TT_NAME_WINDOWS_ENCODING_UCS4
};

enum TT_NAME_MAC_ENCODING_ID
{
    TT_NAME_MAC_ENCODING_ROMAN = 0,
    TT_NAME_MAC_ENCODING_JAPANESE,
    TT_NAME_MAC_ENCODING_TRAD_CHINESE,
    TT_NAME_MAC_ENCODING_KOREAN,
    TT_NAME_MAC_ENCODING_ARABIC,
    TT_NAME_MAC_ENCODING_HEBREW,
    TT_NAME_MAC_ENCODING_GREEK,
    TT_NAME_MAC_ENCODING_RUSSIAN,
    TT_NAME_MAC_ENCODING_RSYMBOL,
    TT_NAME_MAC_ENCODING_DEVANAGARI,
    TT_NAME_MAC_ENCODING_GURMUKHI,
    TT_NAME_MAC_ENCODING_GUJARATI,
    TT_NAME_MAC_ENCODING_ORIYA,
    TT_NAME_MAC_ENCODING_BENGALI,
    TT_NAME_MAC_ENCODING_TAMIL,
    TT_NAME_MAC_ENCODING_TELUGU,
    TT_NAME_MAC_ENCODING_KANNADA,
    TT_NAME_MAC_ENCODING_MALAYALAM,
    TT_NAME_MAC_ENCODING_SINHALESE,
    TT_NAME_MAC_ENCODING_BURMESE,
    TT_NAME_MAC_ENCODING_KHMER,
    TT_NAME_MAC_ENCODING_THAI,
    TT_NAME_MAC_ENCODING_LAOTIAN,
    TT_NAME_MAC_ENCODING_GEORGIAN,
    TT_NAME_MAC_ENCODING_ARMENIAN,
    TT_NAME_MAC_ENCODING_SIMPL_CHINESE,
    TT_NAME_MAC_ENCODING_TIBETAN,
    TT_NAME_MAC_ENCODING_MONGOLIAN,
    TT_NAME_MAC_ENCODING_GEEZ,
    TT_NAME_MAC_ENCODING_SLAVIC,
    TT_NAME_MAC_ENCODING_VIETNAMESE,
    TT_NAME_MAC_ENCODING_SINDHI,
    TT_NAME_MAC_ENCODING_UNINTERPRETED
};

enum TT_NAME_MAC_LANGUAGE_ID
{
    TT_NAME_MAC_LANGID_ENGLISH = 0,
    TT_NAME_MAC_LANGID_FRENCH,
    TT_NAME_MAC_LANGID_GERMAN,
    TT_NAME_MAC_LANGID_ITALIAN,
    TT_NAME_MAC_LANGID_DUTCH,
    TT_NAME_MAC_LANGID_SWEDISH,
    TT_NAME_MAC_LANGID_SPANISH,
    TT_NAME_MAC_LANGID_DANISH,
    TT_NAME_MAC_LANGID_PORTUGUESE,
    TT_NAME_MAC_LANGID_NORWEGIAN,
    TT_NAME_MAC_LANGID_HEBREW,
    TT_NAME_MAC_LANGID_JAPANESE,
    TT_NAME_MAC_LANGID_ARABIC,
    TT_NAME_MAC_LANGID_FINNISH,
    TT_NAME_MAC_LANGID_GREEK,
    TT_NAME_MAC_LANGID_ICELANDIC,
    TT_NAME_MAC_LANGID_MALTESE,
    TT_NAME_MAC_LANGID_TURKISH,
    TT_NAME_MAC_LANGID_CROATIAN,
    TT_NAME_MAC_LANGID_TRAD_CHINESE,
    TT_NAME_MAC_LANGID_URDU,
    TT_NAME_MAC_LANGID_HINDI,
    TT_NAME_MAC_LANGID_THAI,
    TT_NAME_MAC_LANGID_KOREAN,
    TT_NAME_MAC_LANGID_LITHUANIAN,
    TT_NAME_MAC_LANGID_POLISH,
    TT_NAME_MAC_LANGID_HUNGARIAN,
    TT_NAME_MAC_LANGID_ESTONIAN,
    TT_NAME_MAC_LANGID_LATVIAN,
    TT_NAME_MAC_LANGID_SAMI,
    TT_NAME_MAC_LANGID_FAROESE,
    TT_NAME_MAC_LANGID_FARSI,
    TT_NAME_MAC_LANGID_RUSSIAN,
    TT_NAME_MAC_LANGID_SIMPL_CHINESE,
    TT_NAME_MAC_LANGID_FLEMISH,
    TT_NAME_MAC_LANGID_GAELIC,
    TT_NAME_MAC_LANGID_ALBANIAN,
    TT_NAME_MAC_LANGID_ROMANIAN,
    TT_NAME_MAC_LANGID_CZECH,
    TT_NAME_MAC_LANGID_SLOVAK,
    TT_NAME_MAC_LANGID_SLOVENIAN,
    TT_NAME_MAC_LANGID_YIDDISH,
    TT_NAME_MAC_LANGID_SERBIAN,
    TT_NAME_MAC_LANGID_MACEDONIAN,
    TT_NAME_MAC_LANGID_BULGARIAN,
    TT_NAME_MAC_LANGID_UKRAINIAN,
    TT_NAME_MAC_LANGID_BYELORUSSIAN,
    TT_NAME_MAC_LANGID_UZBEK,
    TT_NAME_MAC_LANGID_KAZAKH,
    TT_NAME_MAC_LANGID_AZERB_CYR,
    TT_NAME_MAC_LANGID_AZERB_ARABIC,
    TT_NAME_MAC_LANGID_ARMENIAN,
    TT_NAME_MAC_LANGID_GEORGIAN,
    TT_NAME_MAC_LANGID_MOLDAVIAN,
    TT_NAME_MAC_LANGID_KIRGHIZ,
    TT_NAME_MAC_LANGID_TAJIKI,
    TT_NAME_MAC_LANGID_TURKMEN,
    TT_NAME_MAC_LANGID_MONGOLIAN,
    TT_NAME_MAC_LANGID_MONGOLIAN_CYR,
    TT_NAME_MAC_LANGID_PASHTO,
    TT_NAME_MAC_LANGID_KURDISH,
    TT_NAME_MAC_LANGID_KASHMIRI,
    TT_NAME_MAC_LANGID_SINDHI,
    TT_NAME_MAC_LANGID_TIBETAN,
    TT_NAME_MAC_LANGID_NEPALI,
    TT_NAME_MAC_LANGID_SANSKRIT,
    TT_NAME_MAC_LANGID_MARATHI,
    TT_NAME_MAC_LANGID_BENGALI,
    TT_NAME_MAC_LANGID_ASSAMESE,
    TT_NAME_MAC_LANGID_GUJARATI,
    TT_NAME_MAC_LANGID_PUNJABI,
    TT_NAME_MAC_LANGID_ORIYA,
    TT_NAME_MAC_LANGID_MALAYALAM,
    TT_NAME_MAC_LANGID_KANNADA,
    TT_NAME_MAC_LANGID_TAMIL,
    TT_NAME_MAC_LANGID_TELUGU,
    TT_NAME_MAC_LANGID_SINHALESE,
    TT_NAME_MAC_LANGID_BURMESE,
    TT_NAME_MAC_LANGID_KHMER,
    TT_NAME_MAC_LANGID_LAO,
    TT_NAME_MAC_LANGID_VIETNAMESE,
    TT_NAME_MAC_LANGID_INDONESIAN,
    TT_NAME_MAC_LANGID_TAGALONG,
    TT_NAME_MAC_LANGID_MALAY_ROMAN,
    TT_NAME_MAC_LANGID_MALAY_ARABIC,
    TT_NAME_MAC_LANGID_AMHARIC,
    TT_NAME_MAC_LANGID_TIGRINYA,
    TT_NAME_MAC_LANGID_GALLA,
    TT_NAME_MAC_LANGID_SOMALI,
    TT_NAME_MAC_LANGID_SWAHILI,
    TT_NAME_MAC_LANGID_KINYARWANDA,
    TT_NAME_MAC_LANGID_RUNDI,
    TT_NAME_MAC_LANGID_NYANJA,
    TT_NAME_MAC_LANGID_MALAGASY,
    TT_NAME_MAC_LANGID_ESPERANTO,
    TT_NAME_MAC_LANGID_WELSH,
    TT_NAME_MAC_LANGID_BASQUE,
    TT_NAME_MAC_LANGID_CATALAN,
    TT_NAME_MAC_LANGID_LATIN,
    TT_NAME_MAC_LANGID_QUENCHUA,
    TT_NAME_MAC_LANGID_GUARANI,
    TT_NAME_MAC_LANGID_AYMARA,
    TT_NAME_MAC_LANGID_TATAR,
    TT_NAME_MAC_LANGID_UIGHUR,
    TT_NAME_MAC_LANGID_DZONGKHA,
    TT_NAME_MAC_LANGID_JAVANESE,
    TT_NAME_MAC_LANGID_SUNDANESE,
    TT_NAME_MAC_LANGID_GALICIAN,
    TT_NAME_MAC_LANGID_AFRIKAANS,
    TT_NAME_MAC_LANGID_BRETON,
    TT_NAME_MAC_LANGID_INUKTITUT,
    TT_NAME_MAC_LANGID_SCOTTISH_GAELIC,
    TT_NAME_MAC_LANGID_MANX_GAELIC,
    TT_NAME_MAC_LANGID_IRISH_GAELIC,
    TT_NAME_MAC_LANGID_TONGAN,
    TT_NAME_MAC_LANGID_GREEK_POLYTONIC,
    TT_NAME_MAC_LANGID_GREENLANDIC,
    TT_NAME_MAC_LANGID_AZER_ROMAN
};

/* Names are indexed with TT_NAME_MAC_LANGUAGE_ID values */
static const char name_mac_langid_to_locale[][10] = {
    "en-US",
    "fr-FR",
    "de-DE",
    "it-IT",
    "nl-NL",
    "sv-SE",
    "es-ES",
    "da-DA",
    "pt-PT",
    "no-NO",
    "he-IL",
    "ja-JP",
    "ar-AR",
    "fi-FI",
    "el-GR",
    "is-IS",
    "mt-MT",
    "tr-TR",
    "hr-HR",
    "zh-HK",
    "ur-PK",
    "hi-IN",
    "th-TH",
    "ko-KR",
    "lt-LT",
    "pl-PL",
    "hu-HU",
    "et-EE",
    "lv-LV",
    "se-NO",
    "fo-FO",
    "fa-IR",
    "ru-RU",
    "zh-CN",
    "nl-BE",
    "gd-GB",
    "sq-AL",
    "ro-RO",
    "cs-CZ",
    "sk-SK",
    "sl-SI",
    "",
    "sr-Latn",
    "mk-MK",
    "bg-BG",
    "uk-UA",
    "be-BY",
    "uz-Latn",
    "kk-KZ",
    "az-Cyrl-AZ",
    "az-AZ",
    "hy-AM",
    "ka-GE",
    "",
    "",
    "tg-TJ",
    "tk-TM",
    "mn-Mong",
    "mn-MN",
    "ps-AF",
    "ku-Arab",
    "",
    "sd-Arab",
    "bo-CN",
    "ne-NP",
    "sa-IN",
    "mr-IN",
    "bn-IN",
    "as-IN",
    "gu-IN",
    "pa-Arab",
    "or-IN",
    "ml-IN",
    "kn-IN",
    "ta-LK",
    "te-IN",
    "si-LK",
    "",
    "km-KH",
    "lo-LA",
    "vi-VN",
    "id-ID",
    "",
    "ms-MY",
    "ms-Arab",
    "am-ET",
    "ti-ET",
    "",
    "",
    "sw-KE",
    "rw-RW",
    "",
    "",
    "",
    "",
    "cy-GB",
    "eu-ES",
    "ca-ES",
    "",
    "",
    "",
    "",
    "tt-RU",
    "ug-CN",
    "",
    "",
    "",
    "gl-ES",
    "af-ZA",
    "br-FR",
    "iu-Latn-CA",
    "gd-GB",
    "",
    "ga-IE",
    "",
    "",
    "kl-GL",
    "az-Latn"
};

enum OPENTYPE_STRING_ID
{
    OPENTYPE_STRING_COPYRIGHT_NOTICE = 0,
    OPENTYPE_STRING_FAMILY_NAME,
    OPENTYPE_STRING_SUBFAMILY_NAME,
    OPENTYPE_STRING_UNIQUE_IDENTIFIER,
    OPENTYPE_STRING_FULL_FONTNAME,
    OPENTYPE_STRING_VERSION_STRING,
    OPENTYPE_STRING_POSTSCRIPT_FONTNAME,
    OPENTYPE_STRING_TRADEMARK,
    OPENTYPE_STRING_MANUFACTURER,
    OPENTYPE_STRING_DESIGNER,
    OPENTYPE_STRING_DESCRIPTION,
    OPENTYPE_STRING_VENDOR_URL,
    OPENTYPE_STRING_DESIGNER_URL,
    OPENTYPE_STRING_LICENSE_DESCRIPTION,
    OPENTYPE_STRING_LICENSE_INFO_URL,
    OPENTYPE_STRING_RESERVED_ID15,
    OPENTYPE_STRING_PREFERRED_FAMILY_NAME,
    OPENTYPE_STRING_PREFERRED_SUBFAMILY_NAME,
    OPENTYPE_STRING_COMPATIBLE_FULLNAME,
    OPENTYPE_STRING_SAMPLE_TEXT,
    OPENTYPE_STRING_POSTSCRIPT_CID_NAME,
    OPENTYPE_STRING_WWS_FAMILY_NAME,
    OPENTYPE_STRING_WWS_SUBFAMILY_NAME
};

static const UINT16 dwriteid_to_opentypeid[DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_CID_NAME+1] =
{
    (UINT16)-1, /* DWRITE_INFORMATIONAL_STRING_NONE is not used */
    OPENTYPE_STRING_COPYRIGHT_NOTICE,
    OPENTYPE_STRING_VERSION_STRING,
    OPENTYPE_STRING_TRADEMARK,
    OPENTYPE_STRING_MANUFACTURER,
    OPENTYPE_STRING_DESIGNER,
    OPENTYPE_STRING_DESIGNER_URL,
    OPENTYPE_STRING_DESCRIPTION,
    OPENTYPE_STRING_VENDOR_URL,
    OPENTYPE_STRING_LICENSE_DESCRIPTION,
    OPENTYPE_STRING_LICENSE_INFO_URL,
    OPENTYPE_STRING_FAMILY_NAME,
    OPENTYPE_STRING_SUBFAMILY_NAME,
    OPENTYPE_STRING_PREFERRED_FAMILY_NAME,
    OPENTYPE_STRING_PREFERRED_SUBFAMILY_NAME,
    OPENTYPE_STRING_SAMPLE_TEXT,
    OPENTYPE_STRING_FULL_FONTNAME,
    OPENTYPE_STRING_POSTSCRIPT_FONTNAME,
    OPENTYPE_STRING_POSTSCRIPT_CID_NAME
};

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

    if (found) *found = FALSE;
    if (table_size) *table_size = 0;

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

        if (found) *found = TRUE;
        if (table_size) *table_size = length;
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

        if (*pgi) return;
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

static UINT get_name_record_codepage(enum OPENTYPE_PLATFORM_ID platform, USHORT encoding)
{
    UINT codepage = 0;

    switch (platform) {
    case OPENTYPE_PLATFORM_MAC:
        switch (encoding)
        {
            case TT_NAME_MAC_ENCODING_ROMAN:
                codepage = 10000;
                break;
            case TT_NAME_MAC_ENCODING_JAPANESE:
                codepage = 10001;
                break;
            case TT_NAME_MAC_ENCODING_TRAD_CHINESE:
                codepage = 10002;
                break;
            case TT_NAME_MAC_ENCODING_KOREAN:
                codepage = 10003;
                break;
            case TT_NAME_MAC_ENCODING_ARABIC:
                codepage = 10004;
                break;
            case TT_NAME_MAC_ENCODING_HEBREW:
                codepage = 10005;
                break;
            case TT_NAME_MAC_ENCODING_GREEK:
                codepage = 10006;
                break;
            case TT_NAME_MAC_ENCODING_RUSSIAN:
                codepage = 10007;
                break;
            case TT_NAME_MAC_ENCODING_SIMPL_CHINESE:
                codepage = 10008;
                break;
            case TT_NAME_MAC_ENCODING_THAI:
                codepage = 10021;
                break;
            default:
                FIXME("encoding %u not handled, platform %d.\n", encoding, platform);
                break;
        }
        break;
    case OPENTYPE_PLATFORM_WIN:
        switch (encoding)
        {
            case TT_NAME_WINDOWS_ENCODING_SYMBOL:
            case TT_NAME_WINDOWS_ENCODING_UCS2:
                break;
            case TT_NAME_WINDOWS_ENCODING_SJIS:
                codepage = 932;
                break;
            case TT_NAME_WINDOWS_ENCODING_PRC:
                codepage = 936;
                break;
            case TT_NAME_WINDOWS_ENCODING_BIG5:
                codepage = 950;
                break;
            case TT_NAME_WINDOWS_ENCODING_WANSUNG:
                codepage = 20949;
                break;
            case TT_NAME_WINDOWS_ENCODING_JOHAB:
                codepage = 1361;
                break;
            default:
                FIXME("encoding %u not handled, platform %d.\n", encoding, platform);
                break;
        }
        break;
    default:
        FIXME("unknown platform %d\n", platform);
    }

    return codepage;
}

static void get_name_record_locale(enum OPENTYPE_PLATFORM_ID platform, USHORT lang_id, WCHAR *locale, USHORT locale_len)
{
    static const WCHAR enusW[] = {'e','n','-','U','S',0};

    switch (platform) {
    case OPENTYPE_PLATFORM_MAC:
    {
        const char *locale_name = NULL;

        if (lang_id > TT_NAME_MAC_LANGID_AZER_ROMAN)
            ERR("invalid mac lang id %d\n", lang_id);
        else if (!name_mac_langid_to_locale[lang_id][0])
            FIXME("failed to map mac lang id %d to locale name\n", lang_id);
        else
            locale_name = name_mac_langid_to_locale[lang_id];

        if (locale_name)
            MultiByteToWideChar(CP_ACP, 0, name_mac_langid_to_locale[lang_id], -1, locale, locale_len);
        else
            strcpyW(locale, enusW);
        break;
    }
    case OPENTYPE_PLATFORM_WIN:
        if (!LCIDToLocaleName(MAKELCID(lang_id, SORT_DEFAULT), locale, locale_len, 0)) {
            FIXME("failed to get locale name for lcid=0x%08x\n", MAKELCID(lang_id, SORT_DEFAULT));
            strcpyW(locale, enusW);
        }
        break;
    default:
        FIXME("unknown platform %d\n", platform);
    }
}

HRESULT opentype_get_font_strings_from_id(const void *table_data, DWRITE_INFORMATIONAL_STRING_ID id, IDWriteLocalizedStrings **strings)
{
    const TT_NAME_V0 *header;
    BYTE *storage_area = 0;
    USHORT count = 0;
    UINT16 name_id;
    BOOL exists;
    HRESULT hr;
    int i;

    if (!table_data)
        return E_FAIL;

    hr = create_localizedstrings(strings);
    if (FAILED(hr)) return hr;

    header = table_data;
    storage_area = (LPBYTE)table_data + GET_BE_WORD(header->stringOffset);
    count = GET_BE_WORD(header->count);

    name_id = dwriteid_to_opentypeid[id];

    exists = FALSE;
    for (i = 0; i < count; i++) {
        const TT_NameRecord *record = &header->nameRecord[i];
        USHORT lang_id, length, offset, encoding, platform;

        if (GET_BE_WORD(record->nameID) != name_id)
            continue;

        exists = TRUE;

        /* Right now only accept unicode and windows encoded fonts */
        platform = GET_BE_WORD(record->platformID);
        if (platform != OPENTYPE_PLATFORM_UNICODE &&
            platform != OPENTYPE_PLATFORM_MAC &&
            platform != OPENTYPE_PLATFORM_WIN)
        {
            FIXME("platform %i not supported\n", platform);
            continue;
        }

        lang_id = GET_BE_WORD(record->languageID);
        length = GET_BE_WORD(record->length);
        offset = GET_BE_WORD(record->offset);
        encoding = GET_BE_WORD(record->encodingID);

        if (lang_id < 0x8000) {
            WCHAR locale[LOCALE_NAME_MAX_LENGTH];
            WCHAR *name_string;
            UINT codepage;

            codepage = get_name_record_codepage(platform, encoding);
            get_name_record_locale(platform, lang_id, locale, sizeof(locale)/sizeof(WCHAR));

            if (codepage) {
                DWORD len = MultiByteToWideChar(codepage, 0, (LPSTR)(storage_area + offset), length, NULL, 0);
                name_string = heap_alloc(sizeof(WCHAR) * (len+1));
                MultiByteToWideChar(codepage, 0, (LPSTR)(storage_area + offset), length, name_string, len);
                name_string[len] = 0;
            }
            else {
                int i;

                length /= sizeof(WCHAR);
                name_string = heap_strdupnW((LPWSTR)(storage_area + offset), length);
                for (i = 0; i < length; i++)
                    name_string[i] = GET_BE_WORD(name_string[i]);
            }

            TRACE("string %s for locale %s found\n", debugstr_w(name_string), debugstr_w(locale));
            add_localizedstring(*strings, locale, name_string);
            heap_free(name_string);
        }
        else {
            FIXME("handle NAME format 1");
            continue;
        }
    }

    if (!exists) {
        IDWriteLocalizedStrings_Release(*strings);
        *strings = NULL;
    }

    return hr;
}
