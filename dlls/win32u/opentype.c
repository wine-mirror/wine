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
#include "winnls.h"

#include "wine/debug.h"

#include "ntgdi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(font);

#define MS_OTTO_TAG MS_MAKE_TAG('O','T','T','O')
#define MS_HEAD_TAG MS_MAKE_TAG('h','e','a','d')
#define MS_HHEA_TAG MS_MAKE_TAG('h','h','e','a')
#define MS_OS_2_TAG MS_MAKE_TAG('O','S','/','2')
#define MS_EBSC_TAG MS_MAKE_TAG('E','B','S','C')
#define MS_EBDT_TAG MS_MAKE_TAG('E','B','D','T')
#define MS_CBDT_TAG MS_MAKE_TAG('C','B','D','T')
#define MS_NAME_TAG MS_MAKE_TAG('n','a','m','e')
#define MS_CFF__TAG MS_MAKE_TAG('C','F','F',' ')

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_WORD(x)  RtlUshortByteSwap(x)
#define GET_BE_DWORD(x) RtlUlongByteSwap(x)
#endif

#pragma pack(push,2)
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

struct tt_namerecord
{
    WORD platformID;
    WORD encodingID;
    WORD languageID;
    WORD nameID;
    WORD length;
    WORD offset;
};

struct tt_name_v0
{
    WORD format;
    WORD count;
    WORD stringOffset;
    struct tt_namerecord nameRecord[1];
};

struct tt_head
{
    USHORT majorVersion;
    USHORT minorVersion;
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
};
#pragma pack(pop)

enum OPENTYPE_PLATFORM_ID
{
    OPENTYPE_PLATFORM_UNICODE = 0,
    OPENTYPE_PLATFORM_MAC,
    OPENTYPE_PLATFORM_ISO,
    OPENTYPE_PLATFORM_WIN,
    OPENTYPE_PLATFORM_CUSTOM
};

enum TT_NAME_WIN_ENCODING_ID
{
    TT_NAME_WIN_ENCODING_SYMBOL = 0,
    TT_NAME_WIN_ENCODING_UNICODE_BMP,
    TT_NAME_WIN_ENCODING_SJIS,
    TT_NAME_WIN_ENCODING_PRC,
    TT_NAME_WIN_ENCODING_BIG5,
    TT_NAME_WIN_ENCODING_WANSUNG,
    TT_NAME_WIN_ENCODING_JOHAB,
    TT_NAME_WIN_ENCODING_RESERVED1,
    TT_NAME_WIN_ENCODING_RESERVED2,
    TT_NAME_WIN_ENCODING_RESERVED3,
    TT_NAME_WIN_ENCODING_UNICODE_FULL
};

enum TT_NAME_UNICODE_ENCODING_ID
{
    TT_NAME_UNICODE_ENCODING_1_0 = 0,
    TT_NAME_UNICODE_ENCODING_1_1,
    TT_NAME_UNICODE_ENCODING_ISO_10646,
    TT_NAME_UNICODE_ENCODING_2_0_BMP,
    TT_NAME_UNICODE_ENCODING_2_0_FULL,
    TT_NAME_UNICODE_ENCODING_VAR,
    TT_NAME_UNICODE_ENCODING_FULL,
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

enum OPENTYPE_NAME_ID
{
    OPENTYPE_NAME_COPYRIGHT_NOTICE = 0,
    OPENTYPE_NAME_FAMILY,
    OPENTYPE_NAME_SUBFAMILY,
    OPENTYPE_NAME_UNIQUE_IDENTIFIER,
    OPENTYPE_NAME_FULLNAME,
    OPENTYPE_NAME_VERSION_STRING,
    OPENTYPE_NAME_POSTSCRIPT,
    OPENTYPE_NAME_TRADEMARK,
    OPENTYPE_NAME_MANUFACTURER,
    OPENTYPE_NAME_DESIGNER,
    OPENTYPE_NAME_DESCRIPTION,
    OPENTYPE_NAME_VENDOR_URL,
    OPENTYPE_NAME_DESIGNER_URL,
    OPENTYPE_NAME_LICENSE_DESCRIPTION,
    OPENTYPE_NAME_LICENSE_INFO_URL,
    OPENTYPE_NAME_RESERVED_ID15,
    OPENTYPE_NAME_TYPOGRAPHIC_FAMILY,
    OPENTYPE_NAME_TYPOGRAPHIC_SUBFAMILY,
    OPENTYPE_NAME_COMPATIBLE_FULLNAME,
    OPENTYPE_NAME_SAMPLE_TEXT,
    OPENTYPE_NAME_POSTSCRIPT_CID,
    OPENTYPE_NAME_WWS_FAMILY,
    OPENTYPE_NAME_WWS_SUBFAMILY
};

enum OS2_FSSELECTION
{
    OS2_FSSELECTION_ITALIC           = 1 << 0,
    OS2_FSSELECTION_UNDERSCORE       = 1 << 1,
    OS2_FSSELECTION_NEGATIVE         = 1 << 2,
    OS2_FSSELECTION_OUTLINED         = 1 << 3,
    OS2_FSSELECTION_STRIKEOUT        = 1 << 4,
    OS2_FSSELECTION_BOLD             = 1 << 5,
    OS2_FSSELECTION_REGULAR          = 1 << 6,
    OS2_FSSELECTION_USE_TYPO_METRICS = 1 << 7,
    OS2_FSSELECTION_WWS              = 1 << 8,
    OS2_FSSELECTION_OBLIQUE          = 1 << 9
};

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

static BOOL opentype_get_tt_head( const void *data, size_t size, const struct ttc_sfnt_v1 *ttc_sfnt_v1,
                                  const struct tt_head **tt_head )
{
    UINT32 table_size = sizeof(**tt_head);
    return opentype_get_table_ptr( data, size, ttc_sfnt_v1, MS_HEAD_TAG, (const void **)tt_head, &table_size );
}

static UINT get_name_record_codepage( enum OPENTYPE_PLATFORM_ID platform, USHORT encoding )
{
    switch (platform)
    {
    case OPENTYPE_PLATFORM_UNICODE:
        return 0;
    case OPENTYPE_PLATFORM_MAC:
        switch (encoding)
        {
        case TT_NAME_MAC_ENCODING_ROMAN:
            return 10000;
        case TT_NAME_MAC_ENCODING_JAPANESE:
            return 10001;
        case TT_NAME_MAC_ENCODING_TRAD_CHINESE:
            return 10002;
        case TT_NAME_MAC_ENCODING_KOREAN:
            return 10003;
        case TT_NAME_MAC_ENCODING_ARABIC:
            return 10004;
        case TT_NAME_MAC_ENCODING_HEBREW:
            return 10005;
        case TT_NAME_MAC_ENCODING_GREEK:
            return 10006;
        case TT_NAME_MAC_ENCODING_RUSSIAN:
            return 10007;
        case TT_NAME_MAC_ENCODING_SIMPL_CHINESE:
            return 10008;
        case TT_NAME_MAC_ENCODING_THAI:
            return 10021;
        default:
            WARN( "default ascii encoding used for encoding %d, platform %d\n", encoding, platform );
            return 20127;
        }
        break;
    case OPENTYPE_PLATFORM_WIN:
        switch (encoding)
        {
        case TT_NAME_WIN_ENCODING_SYMBOL:
        case TT_NAME_WIN_ENCODING_UNICODE_BMP:
        case TT_NAME_WIN_ENCODING_UNICODE_FULL:
            return 0;
        case TT_NAME_WIN_ENCODING_SJIS:
            return 932;
        case TT_NAME_WIN_ENCODING_PRC:
            return 936;
        case TT_NAME_WIN_ENCODING_BIG5:
            return 950;
        case TT_NAME_WIN_ENCODING_WANSUNG:
            return 20949;
        case TT_NAME_WIN_ENCODING_JOHAB:
            return 1361;
        default:
            WARN( "default ascii encoding used for encoding %d, platform %d\n", encoding, platform );
            return 20127;
        }
        break;
    default:
        FIXME( "unknown platform %d\n", platform );
        break;
    }

    return 0;
}

static const LANGID mac_langid_table[] =
{
    MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
    MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT),
    MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_ITALIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_DUTCH, SUBLANG_DEFAULT),
    MAKELANGID(LANG_SWEDISH, SUBLANG_DEFAULT),
    MAKELANGID(LANG_SPANISH, SUBLANG_DEFAULT),
    MAKELANGID(LANG_DANISH, SUBLANG_DEFAULT),
    MAKELANGID(LANG_PORTUGUESE, SUBLANG_DEFAULT),
    MAKELANGID(LANG_NORWEGIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT),
    MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT),
    MAKELANGID(LANG_ARABIC, SUBLANG_DEFAULT),
    MAKELANGID(LANG_FINNISH, SUBLANG_DEFAULT),
    MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT),
    MAKELANGID(LANG_ICELANDIC, SUBLANG_DEFAULT),
    MAKELANGID(LANG_MALTESE, SUBLANG_DEFAULT),
    MAKELANGID(LANG_TURKISH, SUBLANG_DEFAULT),
    MAKELANGID(LANG_CROATIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_CHINESE_TRADITIONAL, SUBLANG_DEFAULT),
    MAKELANGID(LANG_URDU, SUBLANG_DEFAULT),
    MAKELANGID(LANG_HINDI, SUBLANG_DEFAULT),
    MAKELANGID(LANG_THAI, SUBLANG_DEFAULT),
    MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_LITHUANIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_POLISH, SUBLANG_DEFAULT),
    MAKELANGID(LANG_HUNGARIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_ESTONIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_LATVIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_SAMI, SUBLANG_DEFAULT),
    MAKELANGID(LANG_FAEROESE, SUBLANG_DEFAULT),
    MAKELANGID(LANG_FARSI, SUBLANG_DEFAULT),
    MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_CHINESE_SIMPLIFIED, SUBLANG_DEFAULT),
    MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH_BELGIAN),
    MAKELANGID(LANG_IRISH, SUBLANG_DEFAULT),
    MAKELANGID(LANG_ALBANIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_ROMANIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_CZECH, SUBLANG_DEFAULT),
    MAKELANGID(LANG_SLOVAK, SUBLANG_DEFAULT),
    MAKELANGID(LANG_SLOVENIAN, SUBLANG_DEFAULT),
    0,
    MAKELANGID(LANG_SERBIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_MACEDONIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_BULGARIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_UKRAINIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_BELARUSIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_UZBEK, SUBLANG_DEFAULT),
    MAKELANGID(LANG_KAZAK, SUBLANG_DEFAULT),
    MAKELANGID(LANG_AZERI, SUBLANG_AZERI_CYRILLIC),
    0,
    MAKELANGID(LANG_ARMENIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_GEORGIAN, SUBLANG_DEFAULT),
    0,
    MAKELANGID(LANG_KYRGYZ, SUBLANG_DEFAULT),
    MAKELANGID(LANG_TAJIK, SUBLANG_DEFAULT),
    MAKELANGID(LANG_TURKMEN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_MONGOLIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_MONGOLIAN, SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA),
    MAKELANGID(LANG_PASHTO, SUBLANG_DEFAULT),
    0,
    MAKELANGID(LANG_KASHMIRI, SUBLANG_DEFAULT),
    MAKELANGID(LANG_SINDHI, SUBLANG_DEFAULT),
    MAKELANGID(LANG_TIBETAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_NEPALI, SUBLANG_DEFAULT),
    MAKELANGID(LANG_SANSKRIT, SUBLANG_DEFAULT),
    MAKELANGID(LANG_MARATHI, SUBLANG_DEFAULT),
    MAKELANGID(LANG_BENGALI, SUBLANG_DEFAULT),
    MAKELANGID(LANG_ASSAMESE, SUBLANG_DEFAULT),
    MAKELANGID(LANG_GUJARATI, SUBLANG_DEFAULT),
    MAKELANGID(LANG_PUNJABI, SUBLANG_DEFAULT),
    MAKELANGID(LANG_ORIYA, SUBLANG_DEFAULT),
    MAKELANGID(LANG_MALAYALAM, SUBLANG_DEFAULT),
    MAKELANGID(LANG_KANNADA, SUBLANG_DEFAULT),
    MAKELANGID(LANG_TAMIL, SUBLANG_DEFAULT),
    MAKELANGID(LANG_TELUGU, SUBLANG_DEFAULT),
    MAKELANGID(LANG_SINHALESE, SUBLANG_DEFAULT),
    0,
    MAKELANGID(LANG_KHMER, SUBLANG_DEFAULT),
    MAKELANGID(LANG_LAO, SUBLANG_DEFAULT),
    MAKELANGID(LANG_VIETNAMESE, SUBLANG_DEFAULT),
    MAKELANGID(LANG_INDONESIAN, SUBLANG_DEFAULT),
    0,
    MAKELANGID(LANG_MALAY, SUBLANG_DEFAULT),
    0,
    MAKELANGID(LANG_AMHARIC, SUBLANG_DEFAULT),
    MAKELANGID(LANG_TIGRIGNA, SUBLANG_DEFAULT),
    0,
    0,
    MAKELANGID(LANG_SWAHILI, SUBLANG_DEFAULT),
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    MAKELANGID(LANG_WELSH, SUBLANG_DEFAULT),
    MAKELANGID(LANG_BASQUE, SUBLANG_DEFAULT),
    MAKELANGID(LANG_CATALAN, SUBLANG_DEFAULT),
    0,
    MAKELANGID(LANG_QUECHUA, SUBLANG_DEFAULT),
    0,
    0,
    MAKELANGID(LANG_TATAR, SUBLANG_DEFAULT),
    MAKELANGID(LANG_UIGHUR, SUBLANG_DEFAULT),
    0,
    0,
    0,
    MAKELANGID(LANG_GALICIAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_AFRIKAANS, SUBLANG_DEFAULT),
    MAKELANGID(LANG_BRETON, SUBLANG_DEFAULT),
    MAKELANGID(LANG_INUKTITUT, SUBLANG_DEFAULT),
    MAKELANGID(LANG_SCOTTISH_GAELIC, SUBLANG_DEFAULT),
    0,
    MAKELANGID(LANG_IRISH, SUBLANG_IRISH_IRELAND),
    0,
    0,
    MAKELANGID(LANG_GREENLANDIC, SUBLANG_DEFAULT),
    MAKELANGID(LANG_AZERI, SUBLANG_AZERI_LATIN),
};

static LANGID get_name_record_langid( enum OPENTYPE_PLATFORM_ID platform, USHORT encoding, USHORT language )
{
    switch (platform)
    {
    case OPENTYPE_PLATFORM_WIN:
        return language;
    case OPENTYPE_PLATFORM_MAC:
        if (language < ARRAY_SIZE(mac_langid_table)) return mac_langid_table[language];
        WARN( "invalid mac lang id %d\n", language );
        break;
    case OPENTYPE_PLATFORM_UNICODE:
        switch (encoding)
        {
        case TT_NAME_UNICODE_ENCODING_1_0:
        case TT_NAME_UNICODE_ENCODING_ISO_10646:
        case TT_NAME_UNICODE_ENCODING_2_0_BMP:
            if (language < ARRAY_SIZE(mac_langid_table)) return mac_langid_table[language];
            WARN( "invalid unicode lang id %d\n", language );
            break;
        default:
            break;
        }
        break;
    default:
        FIXME( "unknown platform %d\n", platform );
        break;
    }

    return MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
}

static BOOL opentype_enum_font_names( const struct tt_name_v0 *header, enum OPENTYPE_PLATFORM_ID platform,
                                      enum OPENTYPE_NAME_ID name, opentype_enum_names_cb callback, void *user )
{
    const char *name_data;
    USHORT i, name_count, encoding, language, length, offset;
    USHORT platform_id = GET_BE_WORD( platform ), name_id = GET_BE_WORD( name );
    LANGID langid;
    BOOL ret = FALSE;

    switch (GET_BE_WORD( header->format ))
    {
    case 0:
    case 1:
        break;
    default:
        FIXME( "unsupported name format %d\n", GET_BE_WORD( header->format ) );
        return FALSE;
    }

    name_data = (const char *)header + GET_BE_WORD( header->stringOffset );
    name_count = GET_BE_WORD( header->count );
    for (i = 0; i < name_count; i++)
    {
        const struct tt_namerecord *record = &header->nameRecord[i];
        struct opentype_name opentype_name;

        if (record->nameID != name_id) continue;
        if (record->platformID != platform_id) continue;

        language = GET_BE_WORD( record->languageID );
        if (language >= 0x8000)
        {
            FIXME( "handle name format 1\n" );
            continue;
        }

        encoding = GET_BE_WORD( record->encodingID );
        offset = GET_BE_WORD( record->offset );
        length = GET_BE_WORD( record->length );
        langid = get_name_record_langid( platform, encoding, language );

        opentype_name.codepage = get_name_record_codepage( platform, encoding );
        opentype_name.length = length;
        opentype_name.bytes = name_data + offset;

        if ((ret = callback( langid, &opentype_name, user ))) break;
    }

    return ret;
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

BOOL opentype_get_tt_name_v0( const void *data, size_t size, const struct ttc_sfnt_v1 *ttc_sfnt_v1,
                              const struct tt_name_v0 **tt_name_v0 )
{
    UINT32 table_size = sizeof(**tt_name_v0);
    return opentype_get_table_ptr( data, size, ttc_sfnt_v1, MS_NAME_TAG, (const void **)tt_name_v0, &table_size );
}

BOOL opentype_enum_family_names( const struct tt_name_v0 *header, opentype_enum_names_cb callback, void *user )
{
    if (opentype_enum_font_names( header, OPENTYPE_PLATFORM_WIN, OPENTYPE_NAME_FAMILY, callback, user ))
        return TRUE;
    if (opentype_enum_font_names( header, OPENTYPE_PLATFORM_MAC, OPENTYPE_NAME_FAMILY, callback, user ))
        return TRUE;
    if (opentype_enum_font_names( header, OPENTYPE_PLATFORM_UNICODE, OPENTYPE_NAME_FAMILY, callback, user ))
        return TRUE;
    return FALSE;
}

BOOL opentype_enum_style_names( const struct tt_name_v0 *header, opentype_enum_names_cb callback, void *user )
{
    if (opentype_enum_font_names( header, OPENTYPE_PLATFORM_WIN, OPENTYPE_NAME_SUBFAMILY, callback, user ))
        return TRUE;
    if (opentype_enum_font_names( header, OPENTYPE_PLATFORM_MAC, OPENTYPE_NAME_SUBFAMILY, callback, user ))
        return TRUE;
    if (opentype_enum_font_names( header, OPENTYPE_PLATFORM_UNICODE, OPENTYPE_NAME_SUBFAMILY, callback, user ))
        return TRUE;
    return FALSE;
}

BOOL opentype_enum_full_names( const struct tt_name_v0 *header, opentype_enum_names_cb callback, void *user )
{
    if (opentype_enum_font_names( header, OPENTYPE_PLATFORM_WIN, OPENTYPE_NAME_FULLNAME, callback, user ))
        return TRUE;
    if (opentype_enum_font_names( header, OPENTYPE_PLATFORM_MAC, OPENTYPE_NAME_FULLNAME, callback, user ))
        return TRUE;
    if (opentype_enum_font_names( header, OPENTYPE_PLATFORM_UNICODE, OPENTYPE_NAME_FULLNAME, callback, user ))
        return TRUE;
    return FALSE;
}

BOOL opentype_get_properties( const void *data, size_t size, const struct ttc_sfnt_v1 *ttc_sfnt_v1,
                              DWORD *version, FONTSIGNATURE *fs, DWORD *ntm_flags, UINT *weight )
{
    const struct tt_os2_v1 *tt_os2_v1;
    const struct tt_head *tt_head;
    const void *cff_header;
    UINT32 table_size = 0;
    USHORT idx, selection;
    DWORD flags = 0;

    if (!opentype_get_tt_head( data, size, ttc_sfnt_v1, &tt_head )) return FALSE;
    if (!opentype_get_tt_os2_v1( data, size, ttc_sfnt_v1, &tt_os2_v1 )) return FALSE;

    *version = GET_BE_DWORD( tt_head->revision );

    fs->fsUsb[0] = GET_BE_DWORD( tt_os2_v1->ulUnicodeRange1 );
    fs->fsUsb[1] = GET_BE_DWORD( tt_os2_v1->ulUnicodeRange2 );
    fs->fsUsb[2] = GET_BE_DWORD( tt_os2_v1->ulUnicodeRange3 );
    fs->fsUsb[3] = GET_BE_DWORD( tt_os2_v1->ulUnicodeRange4 );

    if (tt_os2_v1->version == 0)
    {
        idx = GET_BE_WORD( tt_os2_v1->usFirstCharIndex );
        if (idx >= 0xf000 && idx < 0xf100) fs->fsCsb[0] = FS_SYMBOL;
        else fs->fsCsb[0] = FS_LATIN1;
        fs->fsCsb[1] = 0;
    }
    else
    {
        fs->fsCsb[0] = GET_BE_DWORD( tt_os2_v1->ulCodePageRange1 );
        fs->fsCsb[1] = GET_BE_DWORD( tt_os2_v1->ulCodePageRange2 );
    }

    selection = GET_BE_WORD( tt_os2_v1->fsSelection );

    if (selection & OS2_FSSELECTION_ITALIC) flags |= NTM_ITALIC;
    if (selection & OS2_FSSELECTION_BOLD) flags |= NTM_BOLD;
    if (selection & OS2_FSSELECTION_REGULAR) flags |= NTM_REGULAR;
    if (flags == 0) flags = NTM_REGULAR;
    *weight = GET_BE_WORD( tt_os2_v1->usWeightClass );

    if (opentype_get_table_ptr( data, size, ttc_sfnt_v1, MS_CFF__TAG, &cff_header, &table_size ))
        flags |= NTM_PS_OPENTYPE;

    *ntm_flags = flags;
    return TRUE;
}
