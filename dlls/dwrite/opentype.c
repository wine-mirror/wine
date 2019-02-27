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
#define NONAMELESSUNION

#include "dwrite_private.h"
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

#define MS_HEAD_TAG DWRITE_MAKE_OPENTYPE_TAG('h','e','a','d')
#define MS_HHEA_TAG DWRITE_MAKE_OPENTYPE_TAG('h','h','e','a')
#define MS_OTTO_TAG DWRITE_MAKE_OPENTYPE_TAG('O','T','T','O')
#define MS_OS2_TAG  DWRITE_MAKE_OPENTYPE_TAG('O','S','/','2')
#define MS_POST_TAG DWRITE_MAKE_OPENTYPE_TAG('p','o','s','t')
#define MS_TTCF_TAG DWRITE_MAKE_OPENTYPE_TAG('t','t','c','f')
#define MS_GDEF_TAG DWRITE_MAKE_OPENTYPE_TAG('G','D','E','F')
#define MS_GPOS_TAG DWRITE_MAKE_OPENTYPE_TAG('G','P','O','S')
#define MS_GSUB_TAG DWRITE_MAKE_OPENTYPE_TAG('G','S','U','B')
#define MS_NAME_TAG DWRITE_MAKE_OPENTYPE_TAG('n','a','m','e')
#define MS_GLYF_TAG DWRITE_MAKE_OPENTYPE_TAG('g','l','y','f')
#define MS_CFF__TAG DWRITE_MAKE_OPENTYPE_TAG('C','F','F',' ')
#define MS_CFF2_TAG DWRITE_MAKE_OPENTYPE_TAG('C','F','F','2')
#define MS_COLR_TAG DWRITE_MAKE_OPENTYPE_TAG('C','O','L','R')
#define MS_SVG__TAG DWRITE_MAKE_OPENTYPE_TAG('S','V','G',' ')
#define MS_SBIX_TAG DWRITE_MAKE_OPENTYPE_TAG('s','b','i','x')
#define MS_MAXP_TAG DWRITE_MAKE_OPENTYPE_TAG('m','a','x','p')
#define MS_CBLC_TAG DWRITE_MAKE_OPENTYPE_TAG('C','B','L','C')

/* 'sbix' formats */
#define MS_PNG__TAG DWRITE_MAKE_OPENTYPE_TAG('p','n','g',' ')
#define MS_JPG__TAG DWRITE_MAKE_OPENTYPE_TAG('j','p','g',' ')
#define MS_TIFF_TAG DWRITE_MAKE_OPENTYPE_TAG('t','i','f','f')

#define MS_WOFF_TAG DWRITE_MAKE_OPENTYPE_TAG('w','O','F','F')
#define MS_WOF2_TAG DWRITE_MAKE_OPENTYPE_TAG('w','O','F','2')

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_WORD(x)  RtlUshortByteSwap(x)
#define GET_BE_DWORD(x) RtlUlongByteSwap(x)
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
    DWORD tag;
    DWORD checkSum;
    DWORD offset;
    DWORD length;
} TT_TableRecord;

struct cmap_encoding_record
{
    WORD platformID;
    WORD encodingID;
    DWORD offset;
};

struct cmap_header
{
    WORD version;
    WORD num_tables;
    struct cmap_encoding_record tables[1];
} CMAP_Header;

typedef struct {
    DWORD startCharCode;
    DWORD endCharCode;
    DWORD startGlyphID;
} CMAP_SegmentedCoverage_group;

struct cmap_segmented_coverage
{
    WORD format;
    WORD reserved;
    DWORD length;
    DWORD language;
    DWORD num_groups;
    CMAP_SegmentedCoverage_group groups[1];
};

struct cmap_segment_mapping
{
    WORD format;
    WORD length;
    WORD language;
    WORD seg_count_x2;
    WORD search_range;
    WORD entry_selector;
    WORD range_shift;
    WORD end_code[1];
};

enum OPENTYPE_CMAP_TABLE_FORMAT
{
    OPENTYPE_CMAP_TABLE_SEGMENT_MAPPING = 4,
    OPENTYPE_CMAP_TABLE_SEGMENTED_COVERAGE = 12
};

/* PANOSE is 10 bytes in size, need to pack the structure properly */
#include "pshpack2.h"
typedef struct
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
} TT_HEAD;

enum TT_HEAD_MACSTYLE
{
    TT_HEAD_MACSTYLE_BOLD      = 1 << 0,
    TT_HEAD_MACSTYLE_ITALIC    = 1 << 1,
    TT_HEAD_MACSTYLE_UNDERLINE = 1 << 2,
    TT_HEAD_MACSTYLE_OUTLINE   = 1 << 3,
    TT_HEAD_MACSTYLE_SHADOW    = 1 << 4,
    TT_HEAD_MACSTYLE_CONDENSED = 1 << 5,
    TT_HEAD_MACSTYLE_EXTENDED  = 1 << 6,
};

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

typedef struct {
    USHORT majorVersion;
    USHORT minorVersion;
    SHORT  ascender;
    SHORT  descender;
    SHORT  linegap;
    USHORT advanceWidthMax;
    SHORT  minLeftSideBearing;
    SHORT  minRightSideBearing;
    SHORT  xMaxExtent;
    SHORT  caretSlopeRise;
    SHORT  caretSlopeRun;
    SHORT  caretOffset;
    SHORT  reserved[4];
    SHORT  metricDataFormat;
    USHORT numberOfHMetrics;
} TT_HHEA;

struct sbix_header
{
    WORD version;
    WORD flags;
    DWORD num_strikes;
    DWORD strike_offset[1];
};

struct sbix_strike
{
    WORD ppem;
    WORD ppi;
    DWORD glyphdata_offsets[1];
};

struct sbix_glyph_data
{
    WORD originOffsetX;
    WORD originOffsetY;
    DWORD graphic_type;
    BYTE data[1];
};

struct maxp
{
    DWORD version;
    WORD num_glyphs;
};

struct cblc_header
{
    WORD major_version;
    WORD minor_version;
    DWORD num_sizes;
};

typedef struct {
    BYTE res[12];
} sbitLineMetrics;

struct cblc_bitmapsize_table
{
    DWORD indexSubTableArrayOffset;
    DWORD indexTablesSize;
    DWORD numberofIndexSubTables;
    DWORD colorRef;
    sbitLineMetrics hori;
    sbitLineMetrics vert;
    WORD startGlyphIndex;
    WORD endGlyphIndex;
    BYTE ppemX;
    BYTE ppemY;
    BYTE bit_depth;
    BYTE flags;
};

struct gasp_range
{
    WORD max_ppem;
    WORD flags;
};

struct gasp_header
{
    WORD version;
    WORD num_ranges;
    struct gasp_range ranges[1];
};

enum OS2_FSSELECTION {
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

struct vdmx_header
{
    WORD version;
    WORD num_recs;
    WORD num_ratios;
};

struct vdmx_ratio
{
    BYTE bCharSet;
    BYTE xRatio;
    BYTE yStartRatio;
    BYTE yEndRatio;
};

struct vdmx_vtable
{
    WORD yPelHeight;
    SHORT yMax;
    SHORT yMin;
};

struct vdmx_group
{
    WORD recs;
    BYTE startsz;
    BYTE endsz;
    struct vdmx_vtable entries[1];
};

struct ot_feature_record
{
    DWORD tag;
    WORD offset;
};

struct ot_feature_list
{
    WORD feature_count;
    struct ot_feature_record features[1];
};

struct ot_langsys
{
    WORD lookup_order; /* Reserved */
    WORD required_feature_index;
    WORD feature_count;
    WORD feature_index[1];
};

struct ot_langsys_record
{
    CHAR tag[4];
    WORD langsys;
};

struct ot_script
{
    WORD default_langsys;
    WORD langsys_count;
    struct ot_langsys_record langsys[1];
} OT_Script;

struct ot_script_record
{
    CHAR tag[4];
    WORD script;
};

struct ot_script_list
{
    WORD script_count;
    struct ot_script_record scripts[1];
};

enum ot_gdef_class
{
    GDEF_CLASS_UNCLASSIFIED = 0,
    GDEF_CLASS_BASE = 1,
    GDEF_CLASS_LIGATURE = 2,
    GDEF_CLASS_MARK = 3,
    GDEF_CLASS_COMPONENT = 4,
    GDEF_CLASS_MAX = GDEF_CLASS_COMPONENT,
};

struct gdef_header
{
    DWORD version;
    WORD classdef;
    WORD attach_list;
    WORD ligcaret_list;
    WORD markattach_classdef;
};

struct ot_gdef_classdef_format1
{
    WORD format;
    WORD start_glyph;
    WORD glyph_count;
    WORD classes[1];
};

struct ot_gdef_class_range
{
    WORD start_glyph;
    WORD end_glyph;
    WORD glyph_class;
};

struct ot_gdef_classdef_format2
{
    WORD format;
    WORD range_count;
    struct ot_gdef_class_range ranges[1];
};

struct gpos_gsub_header
{
    DWORD version;
    WORD script_list;
    WORD feature_list;
    WORD lookup_list;
};

enum gsub_gpos_lookup_flags
{
    LOOKUP_FLAG_RTL = 0x1,
    LOOKUP_FLAG_IGNORE_BASE = 0x2,
    LOOKUP_FLAG_IGNORE_LIGATURES = 0x4,
    LOOKUP_FLAG_IGNORE_MARKS = 0x8,

    LOOKUP_FLAG_IGNORE_MASK = 0xe,
};

enum gpos_lookup_type
{
    GPOS_LOOKUP_SINGLE_ADJUSTMENT = 1,
    GPOS_LOOKUP_PAIR_ADJUSTMENT = 2,
    GPOS_LOOKUP_CURSIVE_ATTACHMENT = 3,
    GPOS_LOOKUP_MARK_TO_BASE_ATTACHMENT = 4,
    GPOS_LOOKUP_MARK_TO_LIGATURE_ATTACHMENT = 5,
    GPOS_LOOKUP_MARK_TO_MARK_ATTACHMENT = 6,
    GPOS_LOOKUP_CONTEXTUAL_POSITION = 7,
    GPOS_LOOKUP_CONTEXTUAL_CHAINING_POSITION = 8,
    GPOS_LOOKUP_EXTENSION_POSITION = 9,
};

enum gpos_value_format
{
    GPOS_VALUE_X_PLACEMENT = 0x1,
    GPOS_VALUE_Y_PLACEMENT = 0x2,
    GPOS_VALUE_X_ADVANCE = 0x4,
    GPOS_VALUE_Y_ADVANCE = 0x8,
    GPOS_VALUE_X_PLACEMENT_DEVICE = 0x10,
    GPOS_VALUE_Y_PLACEMENT_DEVICE = 0x20,
    GPOS_VALUE_X_ADVANCE_DEVICE = 0x40,
    GPOS_VALUE_Y_ADVANCE_DEVICE = 0x80,
};

enum OPENTYPE_PLATFORM_ID
{
    OPENTYPE_PLATFORM_UNICODE = 0,
    OPENTYPE_PLATFORM_MAC,
    OPENTYPE_PLATFORM_ISO,
    OPENTYPE_PLATFORM_WIN,
    OPENTYPE_PLATFORM_CUSTOM
};

struct ot_gpos_extensionpos_format1
{
    WORD format;
    WORD lookup_type;
    DWORD extension_offset;
};

struct ot_feature
{
    WORD feature_params;
    WORD lookup_count;
    WORD lookuplist_index[1];
};

struct ot_lookup_list
{
    WORD lookup_count;
    WORD lookup[1];
};

struct ot_lookup_table
{
    WORD lookup_type;
    WORD flags;
    WORD subtable_count;
    WORD subtable[1];
};

#define GLYPH_NOT_COVERED (~0u)

struct ot_coverage_format1
{
    WORD format;
    WORD glyph_count;
    WORD glyphs[1];
};

struct ot_coverage_range
{
    WORD start_glyph;
    WORD end_glyph;
    WORD startcoverage_index;
};

struct ot_coverage_format2
{
    WORD format;
    WORD range_count;
    struct ot_coverage_range ranges[1];
};

struct ot_gpos_device_table
{
    WORD start_size;
    WORD end_size;
    WORD format;
    WORD values[1];
};

struct ot_gpos_singlepos_format1
{
    WORD format;
    WORD coverage;
    WORD value_format;
    WORD value[1];
};

struct ot_gpos_singlepos_format2
{
    WORD format;
    WORD coverage;
    WORD value_format;
    WORD value_count;
    WORD values[1];
};

struct ot_gpos_pairvalue
{
    WORD second_glyph;
    BYTE data[1];
};

struct ot_gpos_pairset
{
    WORD pairvalue_count;
    struct ot_gpos_pairvalue pairvalues[1];
};

struct ot_gpos_pairpos_format1
{
    WORD format;
    WORD coverage;
    WORD value_format1;
    WORD value_format2;
    WORD pairset_count;
    WORD pairsets[1];
};

struct ot_gpos_pairpos_format2
{
    WORD format;
    WORD coverage;
    WORD value_format1;
    WORD value_format2;
    WORD class_def1;
    WORD class_def2;
    WORD class1_count;
    WORD class2_count;
    WORD values[1];
};

struct ot_gpos_anchor_format1
{
    WORD format;
    short x_coord;
    short y_coord;
};

struct ot_gpos_anchor_format2
{
    WORD format;
    short x_coord;
    short y_coord;
    WORD anchor_point;
};

struct ot_gpos_anchor_format3
{
    WORD format;
    short x_coord;
    short y_coord;
    WORD x_dev_offset;
    WORD y_dev_offset;
};

struct ot_gpos_cursive_format1
{
    WORD format;
    WORD coverage;
    WORD count;
    WORD anchors[1];
};

struct ot_gpos_mark_record
{
    WORD mark_class;
    WORD mark_anchor;
};

struct ot_gpos_mark_array
{
    WORD count;
    struct ot_gpos_mark_record records[1];
};

struct ot_gpos_base_array
{
    WORD count;
    WORD offsets[1];
};

struct ot_gpos_mark_to_base_format1
{
    WORD format;
    WORD mark_coverage;
    WORD base_coverage;
    WORD mark_class_count;
    WORD mark_array;
    WORD base_array;
};

struct ot_gpos_mark_to_lig_format1
{
    WORD format;
    WORD mark_coverage;
    WORD lig_coverage;
    WORD mark_class_count;
    WORD mark_array;
    WORD lig_array;
};

struct ot_gpos_mark_to_mark_format1
{
    WORD format;
    WORD mark1_coverage;
    WORD mark2_coverage;
    WORD mark_class_count;
    WORD mark1_array;
    WORD mark2_array;
};

typedef struct {
    WORD SubstFormat;
    WORD Coverage;
    WORD DeltaGlyphID;
} GSUB_SingleSubstFormat1;

typedef struct {
    WORD SubstFormat;
    WORD Coverage;
    WORD GlyphCount;
    WORD Substitute[1];
} GSUB_SingleSubstFormat2;

typedef struct {
    WORD SubstFormat;
    WORD ExtensionLookupType;
    DWORD ExtensionOffset;
} GSUB_ExtensionPosFormat1;

#include "poppack.h"

enum gsub_lookup_type
{
    GSUB_LOOKUP_SINGLE_SUBST = 1,
    GSUB_LOOKUP_EXTENSION_SUBST = 7,
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
    TT_NAME_MAC_LANGID_TAGALOG,
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
    TT_NAME_MAC_LANGID_WELSH = 128,
    TT_NAME_MAC_LANGID_BASQUE,
    TT_NAME_MAC_LANGID_CATALAN,
    TT_NAME_MAC_LANGID_LATIN,
    TT_NAME_MAC_LANGID_QUECHUA,
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
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
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

/* CPAL table */
struct cpal_header_0
{
    USHORT version;
    USHORT num_palette_entries;
    USHORT num_palettes;
    USHORT num_color_records;
    ULONG offset_first_color_record;
    USHORT color_record_indices[1];
};

struct cpal_color_record
{
    BYTE blue;
    BYTE green;
    BYTE red;
    BYTE alpha;
};

/* COLR table */
struct colr_header
{
    USHORT version;
    USHORT num_baseglyph_records;
    ULONG offset_baseglyph_records;
    ULONG offset_layer_records;
    USHORT num_layer_records;
};

struct colr_baseglyph_record
{
    USHORT glyph;
    USHORT first_layer_index;
    USHORT num_layers;
};

struct colr_layer_record
{
    USHORT glyph;
    USHORT palette_index;
};

static const void *table_read_ensure(const struct dwrite_fonttable *table, unsigned int offset, unsigned int size)
{
    if (size > table->size || offset > table->size - size)
        return NULL;

    return table->data + offset;
}

static WORD table_read_be_word(const struct dwrite_fonttable *table, unsigned int offset)
{
    const WORD *ptr = table_read_ensure(table, offset, sizeof(*ptr));
    return ptr ? GET_BE_WORD(*ptr) : 0;
}

static DWORD table_read_be_dword(const struct dwrite_fonttable *table, unsigned int offset)
{
    const DWORD *ptr = table_read_ensure(table, offset, sizeof(*ptr));
    return ptr ? GET_BE_DWORD(*ptr) : 0;
}

static DWORD table_read_dword(const struct dwrite_fonttable *table, unsigned int offset)
{
    const DWORD *ptr = table_read_ensure(table, offset, sizeof(*ptr));
    return ptr ? *ptr : 0;
}

BOOL is_face_type_supported(DWRITE_FONT_FACE_TYPE type)
{
    return (type == DWRITE_FONT_FACE_TYPE_CFF) ||
           (type == DWRITE_FONT_FACE_TYPE_TRUETYPE) ||
           (type == DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION) ||
           (type == DWRITE_FONT_FACE_TYPE_RAW_CFF);
}

typedef HRESULT (*dwrite_fontfile_analyzer)(IDWriteFontFileStream *stream, UINT32 *font_count, DWRITE_FONT_FILE_TYPE *file_type,
    DWRITE_FONT_FACE_TYPE *face_type);

static HRESULT opentype_ttc_analyzer(IDWriteFontFileStream *stream, UINT32 *font_count, DWRITE_FONT_FILE_TYPE *file_type,
    DWRITE_FONT_FACE_TYPE *face_type)
{
    static const DWORD ttctag = MS_TTCF_TAG;
    const TTC_Header_V1 *header;
    void *context;
    HRESULT hr;

    hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&header, 0, sizeof(header), &context);
    if (FAILED(hr))
        return hr;

    if (!memcmp(header->TTCTag, &ttctag, sizeof(ttctag))) {
        *font_count = GET_BE_DWORD(header->numFonts);
        *file_type = DWRITE_FONT_FILE_TYPE_OPENTYPE_COLLECTION;
        *face_type = DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION;
    }

    IDWriteFontFileStream_ReleaseFileFragment(stream, context);

    return *file_type != DWRITE_FONT_FILE_TYPE_UNKNOWN ? S_OK : S_FALSE;
}

static HRESULT opentype_ttf_analyzer(IDWriteFontFileStream *stream, UINT32 *font_count, DWRITE_FONT_FILE_TYPE *file_type,
    DWRITE_FONT_FACE_TYPE *face_type)
{
    const DWORD *header;
    void *context;
    HRESULT hr;

    hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&header, 0, sizeof(*header), &context);
    if (FAILED(hr))
        return hr;

    if (GET_BE_DWORD(*header) == 0x10000) {
        *font_count = 1;
        *file_type = DWRITE_FONT_FILE_TYPE_TRUETYPE;
        *face_type = DWRITE_FONT_FACE_TYPE_TRUETYPE;
    }

    IDWriteFontFileStream_ReleaseFileFragment(stream, context);

    return *file_type != DWRITE_FONT_FILE_TYPE_UNKNOWN ? S_OK : S_FALSE;
}

static HRESULT opentype_otf_analyzer(IDWriteFontFileStream *stream, UINT32 *font_count, DWRITE_FONT_FILE_TYPE *file_type,
    DWRITE_FONT_FACE_TYPE *face_type)
{
    const DWORD *header;
    void *context;
    HRESULT hr;

    hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&header, 0, sizeof(*header), &context);
    if (FAILED(hr))
        return hr;

    if (GET_BE_DWORD(*header) == MS_OTTO_TAG) {
        *font_count = 1;
        *file_type = DWRITE_FONT_FILE_TYPE_CFF;
        *face_type = DWRITE_FONT_FACE_TYPE_CFF;
    }

    IDWriteFontFileStream_ReleaseFileFragment(stream, context);

    return *file_type != DWRITE_FONT_FILE_TYPE_UNKNOWN ? S_OK : S_FALSE;
}

static HRESULT opentype_type1_analyzer(IDWriteFontFileStream *stream, UINT32 *font_count, DWRITE_FONT_FILE_TYPE *file_type,
    DWRITE_FONT_FACE_TYPE *face_type)
{
#include "pshpack1.h"
    /* Specified in Adobe TechNote #5178 */
    struct pfm_header {
        WORD  dfVersion;
        DWORD dfSize;
        char  data0[95];
        DWORD dfDevice;
        char  data1[12];
    };
#include "poppack.h"
    struct type1_header {
        WORD tag;
        char data[14];
    };
    const struct type1_header *header;
    void *context;
    HRESULT hr;

    hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&header, 0, sizeof(*header), &context);
    if (FAILED(hr))
        return hr;

    /* tag is followed by plain text section */
    if (header->tag == 0x8001 &&
        (!memcmp(header->data, "%!PS-AdobeFont", 14) ||
         !memcmp(header->data, "%!FontType", 10))) {
        *font_count = 1;
        *file_type = DWRITE_FONT_FILE_TYPE_TYPE1_PFB;
        *face_type = DWRITE_FONT_FACE_TYPE_TYPE1;
    }

    IDWriteFontFileStream_ReleaseFileFragment(stream, context);

    /* let's see if it's a .pfm metrics file */
    if (*file_type == DWRITE_FONT_FILE_TYPE_UNKNOWN) {
        const struct pfm_header *pfm_header;
        UINT64 filesize;
        DWORD offset;
        BOOL header_checked;

        hr = IDWriteFontFileStream_GetFileSize(stream, &filesize);
        if (FAILED(hr))
            return hr;

        hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&pfm_header, 0, sizeof(*pfm_header), &context);
        if (FAILED(hr))
            return hr;

        offset = pfm_header->dfDevice;
        header_checked = pfm_header->dfVersion == 0x100 && pfm_header->dfSize == filesize;
        IDWriteFontFileStream_ReleaseFileFragment(stream, context);

        /* as a last test check static string in PostScript information section */
        if (header_checked) {
            static const char postscript[] = "PostScript";
            char *devtype_name;

            hr = IDWriteFontFileStream_ReadFileFragment(stream, (const void**)&devtype_name, offset, sizeof(postscript), &context);
            if (FAILED(hr))
                return hr;

            if (!memcmp(devtype_name, postscript, sizeof(postscript))) {
                *font_count = 1;
                *file_type = DWRITE_FONT_FILE_TYPE_TYPE1_PFM;
                *face_type = DWRITE_FONT_FACE_TYPE_TYPE1;
            }

            IDWriteFontFileStream_ReleaseFileFragment(stream, context);
        }
    }

    return *file_type != DWRITE_FONT_FILE_TYPE_UNKNOWN ? S_OK : S_FALSE;
}

HRESULT opentype_analyze_font(IDWriteFontFileStream *stream, BOOL *supported, DWRITE_FONT_FILE_TYPE *file_type,
        DWRITE_FONT_FACE_TYPE *face_type, UINT32 *face_count)
{
    static dwrite_fontfile_analyzer fontfile_analyzers[] = {
        opentype_ttf_analyzer,
        opentype_otf_analyzer,
        opentype_ttc_analyzer,
        opentype_type1_analyzer,
        NULL
    };
    dwrite_fontfile_analyzer *analyzer = fontfile_analyzers;
    DWRITE_FONT_FACE_TYPE face;
    HRESULT hr;

    if (!face_type)
        face_type = &face;

    *file_type = DWRITE_FONT_FILE_TYPE_UNKNOWN;
    *face_type = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    *face_count = 0;

    while (*analyzer) {
        hr = (*analyzer)(stream, face_count, file_type, face_type);
        if (FAILED(hr))
            return hr;

        if (hr == S_OK)
            break;

        analyzer++;
    }

    *supported = is_face_type_supported(*face_type);
    return S_OK;
}

HRESULT opentype_get_font_table(struct file_stream_desc *stream_desc, UINT32 tag, const void **table_data,
    void **table_context, UINT32 *table_size, BOOL *found)
{
    void *table_directory_context, *sfnt_context;
    TT_TableRecord *table_record = NULL;
    TTC_SFNT_V1 *font_header = NULL;
    UINT32 table_offset = 0;
    UINT16 table_count;
    HRESULT hr;

    if (found) *found = FALSE;
    if (table_size) *table_size = 0;

    *table_data = NULL;
    *table_context = NULL;

    if (stream_desc->face_type == DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION) {
        const TTC_Header_V1 *ttc_header;
        void * ttc_context;
        hr = IDWriteFontFileStream_ReadFileFragment(stream_desc->stream, (const void**)&ttc_header, 0, sizeof(*ttc_header), &ttc_context);
        if (SUCCEEDED(hr)) {
            if (stream_desc->face_index >= GET_BE_DWORD(ttc_header->numFonts))
                hr = E_INVALIDARG;
            else {
                table_offset = GET_BE_DWORD(ttc_header->OffsetTable[stream_desc->face_index]);
                hr = IDWriteFontFileStream_ReadFileFragment(stream_desc->stream, (const void**)&font_header, table_offset, sizeof(*font_header), &sfnt_context);
            }
            IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, ttc_context);
        }
    }
    else
        hr = IDWriteFontFileStream_ReadFileFragment(stream_desc->stream, (const void**)&font_header, 0, sizeof(*font_header), &sfnt_context);

    if (FAILED(hr))
        return hr;

    table_count = GET_BE_WORD(font_header->numTables);
    table_offset += sizeof(*font_header);

    IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, sfnt_context);

    hr = IDWriteFontFileStream_ReadFileFragment(stream_desc->stream, (const void **)&table_record, table_offset,
            table_count * sizeof(*table_record), &table_directory_context);
    if (hr == S_OK) {
        UINT16 i;

        for (i = 0; i < table_count; i++) {
            if (table_record->tag == tag) {
                UINT32 offset = GET_BE_DWORD(table_record->offset);
                UINT32 length = GET_BE_DWORD(table_record->length);

                if (found)
                    *found = TRUE;
                if (table_size)
                    *table_size = length;
                hr = IDWriteFontFileStream_ReadFileFragment(stream_desc->stream, table_data, offset,
                        length, table_context);
                break;
            }
            table_record++;
        }

        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, table_directory_context);
    }

    return hr;
}

/**********
 * CMAP
 **********/

static unsigned int opentype_cmap_get_unicode_ranges_count(const struct dwrite_fonttable *cmap)
{
    unsigned int i, num_tables, count = 0;
    const struct cmap_header *header;

    num_tables = table_read_be_word(cmap, FIELD_OFFSET(struct cmap_header, num_tables));
    header = table_read_ensure(cmap, 0, FIELD_OFFSET(struct cmap_header, tables[num_tables]));

    if (!header)
        return 0;

    for (i = 0; i < num_tables; ++i)
    {
        unsigned int format, offset;

        if (GET_BE_WORD(header->tables[i].platformID) != 3)
            continue;

        offset = GET_BE_DWORD(header->tables[i].offset);
        format = table_read_be_word(cmap, offset);

        switch (format)
        {
            case OPENTYPE_CMAP_TABLE_SEGMENT_MAPPING:
            {
                count += table_read_be_word(cmap, offset + FIELD_OFFSET(struct cmap_segment_mapping, seg_count_x2)) / 2;
                break;
            }
            case OPENTYPE_CMAP_TABLE_SEGMENTED_COVERAGE:
            {
                count += table_read_be_dword(cmap, offset + FIELD_OFFSET(struct cmap_segmented_coverage, num_groups));
                break;
            }
            default:
                FIXME("table format %u is not supported.\n", format);
        }
    }

    return count;
}

HRESULT opentype_cmap_get_unicode_ranges(const struct dwrite_fonttable *cmap, unsigned int max_count,
        DWRITE_UNICODE_RANGE *ranges, unsigned int *count)
{
    unsigned int i, num_tables, k = 0;
    const struct cmap_header *header;

    if (!cmap->exists)
        return E_FAIL;

    *count = opentype_cmap_get_unicode_ranges_count(cmap);

    num_tables = table_read_be_word(cmap, FIELD_OFFSET(struct cmap_header, num_tables));
    header = table_read_ensure(cmap, 0, FIELD_OFFSET(struct cmap_header, tables[num_tables]));

    if (!header)
        return S_OK;

    for (i = 0; i < num_tables && k < max_count; ++i)
    {
        unsigned int j, offset, format;

        if (GET_BE_WORD(header->tables[i].platformID) != 3)
            continue;

        offset = GET_BE_DWORD(header->tables[i].offset);

        format = table_read_be_word(cmap, offset);
        switch (format)
        {
            case OPENTYPE_CMAP_TABLE_SEGMENT_MAPPING:
            {
                unsigned int segment_count = table_read_be_word(cmap, offset +
                        FIELD_OFFSET(struct cmap_segment_mapping, seg_count_x2)) / 2;
                const UINT16 *start_code = table_read_ensure(cmap, offset,
                        FIELD_OFFSET(struct cmap_segment_mapping, end_code[segment_count]) +
                        2 /* reservedPad */ +
                        2 * segment_count /* start code array */);
                const UINT16 *end_code = table_read_ensure(cmap, offset,
                        FIELD_OFFSET(struct cmap_segment_mapping, end_code[segment_count]));

                if (!start_code || !end_code)
                    continue;

                for (j = 0; j < segment_count && GET_BE_WORD(end_code[j]) != 0xffff && k < max_count; ++j, ++k)
                {
                    ranges[k].first = GET_BE_WORD(start_code[j]);
                    ranges[k].last = GET_BE_WORD(end_code[j]);
                }
                break;
            }
            case OPENTYPE_CMAP_TABLE_SEGMENTED_COVERAGE:
            {
                unsigned int num_groups = table_read_be_dword(cmap, offset +
                        FIELD_OFFSET(struct cmap_segmented_coverage, num_groups));
                const struct cmap_segmented_coverage *coverage;

                coverage = table_read_ensure(cmap, offset,
                        FIELD_OFFSET(struct cmap_segmented_coverage, groups[num_groups]));

                for (j = 0; j < num_groups && k < max_count; j++, k++)
                {
                    ranges[k].first = GET_BE_DWORD(coverage->groups[j].startCharCode);
                    ranges[k].last = GET_BE_DWORD(coverage->groups[j].endCharCode);
                }
                break;
            }
            default:
                FIXME("table format %u unhandled.\n", format);
        }
    }

    return *count > max_count ? E_NOT_SUFFICIENT_BUFFER : S_OK;
}

void opentype_get_font_metrics(struct file_stream_desc *stream_desc, DWRITE_FONT_METRICS1 *metrics, DWRITE_CARET_METRICS *caret)
{
    void *os2_context, *head_context, *post_context, *hhea_context;
    const TT_OS2_V2 *tt_os2;
    const TT_HEAD *tt_head;
    const TT_POST *tt_post;
    const TT_HHEA *tt_hhea;

    memset(metrics, 0, sizeof(*metrics));

    opentype_get_font_table(stream_desc, MS_OS2_TAG,  (const void**)&tt_os2, &os2_context, NULL, NULL);
    opentype_get_font_table(stream_desc, MS_HEAD_TAG, (const void**)&tt_head, &head_context, NULL, NULL);
    opentype_get_font_table(stream_desc, MS_POST_TAG, (const void**)&tt_post, &post_context, NULL, NULL);
    opentype_get_font_table(stream_desc, MS_HHEA_TAG, (const void**)&tt_hhea, &hhea_context, NULL, NULL);

    if (tt_head) {
        metrics->designUnitsPerEm = GET_BE_WORD(tt_head->unitsPerEm);
        metrics->glyphBoxLeft = GET_BE_WORD(tt_head->xMin);
        metrics->glyphBoxTop = GET_BE_WORD(tt_head->yMax);
        metrics->glyphBoxRight = GET_BE_WORD(tt_head->xMax);
        metrics->glyphBoxBottom = GET_BE_WORD(tt_head->yMin);
    }

    if (caret) {
        if (tt_hhea) {
            caret->slopeRise = GET_BE_WORD(tt_hhea->caretSlopeRise);
            caret->slopeRun = GET_BE_WORD(tt_hhea->caretSlopeRun);
            caret->offset = GET_BE_WORD(tt_hhea->caretOffset);
        }
        else {
            caret->slopeRise = 0;
            caret->slopeRun = 0;
            caret->offset = 0;
        }
    }

    if (tt_os2) {
        USHORT version = GET_BE_WORD(tt_os2->version);

        metrics->ascent  = GET_BE_WORD(tt_os2->usWinAscent);
        /* Some fonts have usWinDescent value stored as signed short, which could be wrongly
           interpreted as large unsigned value. */
        metrics->descent = abs((SHORT)GET_BE_WORD(tt_os2->usWinDescent));

        /* line gap is estimated using two sets of ascender/descender values and 'hhea' line gap */
        if (tt_hhea) {
            SHORT descender = (SHORT)GET_BE_WORD(tt_hhea->descender);
            INT32 linegap;

            linegap = GET_BE_WORD(tt_hhea->ascender) + abs(descender) + GET_BE_WORD(tt_hhea->linegap) -
                metrics->ascent - metrics->descent;
            metrics->lineGap = linegap > 0 ? linegap : 0;
        }

        metrics->strikethroughPosition  = GET_BE_WORD(tt_os2->yStrikeoutPosition);
        metrics->strikethroughThickness = GET_BE_WORD(tt_os2->yStrikeoutSize);
        metrics->subscriptPositionX = GET_BE_WORD(tt_os2->ySubscriptXOffset);
        /* Y offset is stored as positive offset below baseline */
        metrics->subscriptPositionY = -GET_BE_WORD(tt_os2->ySubscriptYOffset);
        metrics->subscriptSizeX = GET_BE_WORD(tt_os2->ySubscriptXSize);
        metrics->subscriptSizeY = GET_BE_WORD(tt_os2->ySubscriptYSize);
        metrics->superscriptPositionX = GET_BE_WORD(tt_os2->ySuperscriptXOffset);
        metrics->superscriptPositionY = GET_BE_WORD(tt_os2->ySuperscriptYOffset);
        metrics->superscriptSizeX = GET_BE_WORD(tt_os2->ySuperscriptXSize);
        metrics->superscriptSizeY = GET_BE_WORD(tt_os2->ySuperscriptYSize);

        /* version 2 fields */
        if (version >= 2) {
            metrics->capHeight = GET_BE_WORD(tt_os2->sCapHeight);
            metrics->xHeight   = GET_BE_WORD(tt_os2->sxHeight);
        }

        if (GET_BE_WORD(tt_os2->fsSelection) & OS2_FSSELECTION_USE_TYPO_METRICS) {
            SHORT descent = GET_BE_WORD(tt_os2->sTypoDescender);
            metrics->ascent = GET_BE_WORD(tt_os2->sTypoAscender);
            metrics->descent = descent < 0 ? -descent : 0;
            metrics->lineGap = GET_BE_WORD(tt_os2->sTypoLineGap);
            metrics->hasTypographicMetrics = TRUE;
        }
    }
    else {
        metrics->strikethroughPosition = metrics->designUnitsPerEm / 3;
        if (tt_hhea) {
            metrics->ascent = GET_BE_WORD(tt_hhea->ascender);
            metrics->descent = abs((SHORT)GET_BE_WORD(tt_hhea->descender));
        }
    }

    if (tt_post) {
        metrics->underlinePosition = GET_BE_WORD(tt_post->underlinePosition);
        metrics->underlineThickness = GET_BE_WORD(tt_post->underlineThickness);
    }

    if (metrics->underlineThickness == 0)
        metrics->underlineThickness = metrics->designUnitsPerEm / 14;
    if (metrics->strikethroughThickness == 0)
        metrics->strikethroughThickness = metrics->underlineThickness;

    /* estimate missing metrics */
    if (metrics->xHeight == 0)
        metrics->xHeight = metrics->designUnitsPerEm / 2;
    if (metrics->capHeight == 0)
        metrics->capHeight = metrics->designUnitsPerEm * 7 / 10;

    if (tt_os2)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, os2_context);
    if (tt_head)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, head_context);
    if (tt_post)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, post_context);
    if (tt_hhea)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, hhea_context);
}

void opentype_get_font_properties(struct file_stream_desc *stream_desc, struct dwrite_font_props *props)
{
    void *os2_context, *head_context;
    const TT_OS2_V2 *tt_os2;
    const TT_HEAD *tt_head;

    opentype_get_font_table(stream_desc, MS_OS2_TAG,  (const void**)&tt_os2, &os2_context, NULL, NULL);
    opentype_get_font_table(stream_desc, MS_HEAD_TAG, (const void**)&tt_head, &head_context, NULL, NULL);

    /* default stretch, weight and style to normal */
    props->stretch = DWRITE_FONT_STRETCH_NORMAL;
    props->weight = DWRITE_FONT_WEIGHT_NORMAL;
    props->style = DWRITE_FONT_STYLE_NORMAL;
    memset(&props->panose, 0, sizeof(props->panose));
    memset(&props->fontsig, 0, sizeof(props->fontsig));
    memset(&props->lf, 0, sizeof(props->lf));

    /* DWRITE_FONT_STRETCH enumeration values directly match font data values */
    if (tt_os2) {
        USHORT version = GET_BE_WORD(tt_os2->version);
        USHORT fsSelection = GET_BE_WORD(tt_os2->fsSelection);
        USHORT usWeightClass = GET_BE_WORD(tt_os2->usWeightClass);
        USHORT usWidthClass = GET_BE_WORD(tt_os2->usWidthClass);

        if (usWidthClass > DWRITE_FONT_STRETCH_UNDEFINED && usWidthClass <= DWRITE_FONT_STRETCH_ULTRA_EXPANDED)
            props->stretch = usWidthClass;

        if (usWeightClass >= 1 && usWeightClass <= 9)
            usWeightClass *= 100;

        if (usWeightClass > DWRITE_FONT_WEIGHT_ULTRA_BLACK)
            props->weight = DWRITE_FONT_WEIGHT_ULTRA_BLACK;
        else if (usWeightClass > 0)
            props->weight = usWeightClass;

        if (version >= 4 && (fsSelection & OS2_FSSELECTION_OBLIQUE))
            props->style = DWRITE_FONT_STYLE_OBLIQUE;
        else if (fsSelection & OS2_FSSELECTION_ITALIC)
            props->style = DWRITE_FONT_STYLE_ITALIC;
        props->lf.lfItalic = !!(fsSelection & OS2_FSSELECTION_ITALIC);

        memcpy(&props->panose, &tt_os2->panose, sizeof(props->panose));

        /* FONTSIGNATURE */
        props->fontsig.fsUsb[0] = GET_BE_DWORD(tt_os2->ulUnicodeRange1);
        props->fontsig.fsUsb[1] = GET_BE_DWORD(tt_os2->ulUnicodeRange2);
        props->fontsig.fsUsb[2] = GET_BE_DWORD(tt_os2->ulUnicodeRange3);
        props->fontsig.fsUsb[3] = GET_BE_DWORD(tt_os2->ulUnicodeRange4);

        if (GET_BE_WORD(tt_os2->version) == 0) {
            props->fontsig.fsCsb[0] = 0;
            props->fontsig.fsCsb[1] = 0;
        }
        else {
            props->fontsig.fsCsb[0] = GET_BE_DWORD(tt_os2->ulCodePageRange1);
            props->fontsig.fsCsb[1] = GET_BE_DWORD(tt_os2->ulCodePageRange2);
        }
    }
    else if (tt_head) {
        USHORT macStyle = GET_BE_WORD(tt_head->macStyle);

        if (macStyle & TT_HEAD_MACSTYLE_CONDENSED)
            props->stretch = DWRITE_FONT_STRETCH_CONDENSED;
        else if (macStyle & TT_HEAD_MACSTYLE_EXTENDED)
            props->stretch = DWRITE_FONT_STRETCH_EXPANDED;

        if (macStyle & TT_HEAD_MACSTYLE_BOLD)
            props->weight = DWRITE_FONT_WEIGHT_BOLD;

        if (macStyle & TT_HEAD_MACSTYLE_ITALIC) {
            props->style = DWRITE_FONT_STYLE_ITALIC;
            props->lf.lfItalic = 1;
        }
    }

    props->lf.lfWeight = props->weight;

    TRACE("stretch=%d, weight=%d, style %d\n", props->stretch, props->weight, props->style);

    if (tt_os2)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, os2_context);
    if (tt_head)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, head_context);
}

static UINT get_name_record_codepage(enum OPENTYPE_PLATFORM_ID platform, USHORT encoding)
{
    UINT codepage = 0;

    switch (platform) {
    case OPENTYPE_PLATFORM_UNICODE:
        break;
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
            WARN("invalid mac lang id %d\n", lang_id);
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
    case OPENTYPE_PLATFORM_UNICODE:
        strcpyW(locale, enusW);
        break;
    default:
        FIXME("unknown platform %d\n", platform);
    }
}

static BOOL opentype_decode_namerecord(const TT_NAME_V0 *header, BYTE *storage_area, USHORT recid, IDWriteLocalizedStrings *strings)
{
    const TT_NameRecord *record = &header->nameRecord[recid];
    USHORT lang_id, length, offset, encoding, platform;
    BOOL ret = FALSE;

    platform = GET_BE_WORD(record->platformID);
    lang_id = GET_BE_WORD(record->languageID);
    length = GET_BE_WORD(record->length);
    offset = GET_BE_WORD(record->offset);
    encoding = GET_BE_WORD(record->encodingID);

    if (lang_id < 0x8000) {
        WCHAR locale[LOCALE_NAME_MAX_LENGTH];
        WCHAR *name_string;
        UINT codepage;

        codepage = get_name_record_codepage(platform, encoding);
        get_name_record_locale(platform, lang_id, locale, ARRAY_SIZE(locale));

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
        add_localizedstring(strings, locale, name_string);
        heap_free(name_string);
        ret = TRUE;
    }
    else
        FIXME("handle NAME format 1\n");

    return ret;
}

static HRESULT opentype_get_font_strings_from_id(const void *table_data, enum OPENTYPE_STRING_ID id, IDWriteLocalizedStrings **strings)
{
    const TT_NAME_V0 *header;
    BYTE *storage_area = 0;
    USHORT count = 0;
    int i, candidate;
    WORD format;
    BOOL exists;
    HRESULT hr;

    if (!table_data)
        return E_FAIL;

    hr = create_localizedstrings(strings);
    if (FAILED(hr)) return hr;

    header = table_data;
    format = GET_BE_WORD(header->format);

    switch (format) {
    case 0:
    case 1:
        break;
    default:
        FIXME("unsupported NAME format %d\n", format);
    }

    storage_area = (LPBYTE)table_data + GET_BE_WORD(header->stringOffset);
    count = GET_BE_WORD(header->count);

    exists = FALSE;
    candidate = -1;
    for (i = 0; i < count; i++) {
        const TT_NameRecord *record = &header->nameRecord[i];
        USHORT platform;

        if (GET_BE_WORD(record->nameID) != id)
            continue;

        /* Right now only accept unicode and windows encoded fonts */
        platform = GET_BE_WORD(record->platformID);
        if (platform != OPENTYPE_PLATFORM_UNICODE &&
            platform != OPENTYPE_PLATFORM_MAC &&
            platform != OPENTYPE_PLATFORM_WIN)
        {
            FIXME("platform %i not supported\n", platform);
            continue;
        }

        /* Skip such entries for now, fonts tend to duplicate those strings as
           WIN platform entries. If font does not have WIN or MAC entry for this id, we will
           use this Unicode platform entry while assuming en-US locale. */
        if (platform == OPENTYPE_PLATFORM_UNICODE) {
            candidate = i;
            continue;
        }

        if (!opentype_decode_namerecord(header, storage_area, i, *strings))
            continue;

        exists = TRUE;
    }

    if (!exists) {
        if (candidate != -1)
            exists = opentype_decode_namerecord(header, storage_area, candidate, *strings);
        else {
            IDWriteLocalizedStrings_Release(*strings);
            *strings = NULL;
        }
    }

    return exists ? S_OK : E_FAIL;
}

/* Provides a conversion from DWRITE to OpenType name ids, input id should be valid, it's not checked. */
HRESULT opentype_get_font_info_strings(const void *table_data, DWRITE_INFORMATIONAL_STRING_ID id, IDWriteLocalizedStrings **strings)
{
    return opentype_get_font_strings_from_id(table_data, dwriteid_to_opentypeid[id], strings);
}

/* FamilyName locating order is WWS Family Name -> Preferred Family Name -> Family Name. If font claims to
   have 'Preferred Family Name' in WWS format, then WWS name is not used. */
HRESULT opentype_get_font_familyname(struct file_stream_desc *stream_desc, IDWriteLocalizedStrings **names)
{
    const TT_OS2_V2 *tt_os2;
    void *os2_context, *name_context;
    const void *name_table;
    HRESULT hr;

    opentype_get_font_table(stream_desc, MS_OS2_TAG,  (const void**)&tt_os2, &os2_context, NULL, NULL);
    opentype_get_font_table(stream_desc, MS_NAME_TAG, &name_table, &name_context, NULL, NULL);

    *names = NULL;

    /* if Preferred Family doesn't conform to WWS model try WWS name */
    if (tt_os2 && !(GET_BE_WORD(tt_os2->fsSelection) & OS2_FSSELECTION_WWS))
        hr = opentype_get_font_strings_from_id(name_table, OPENTYPE_STRING_WWS_FAMILY_NAME, names);
    else
        hr = E_FAIL;

    if (FAILED(hr))
        hr = opentype_get_font_strings_from_id(name_table, OPENTYPE_STRING_PREFERRED_FAMILY_NAME, names);
    if (FAILED(hr))
        hr = opentype_get_font_strings_from_id(name_table, OPENTYPE_STRING_FAMILY_NAME, names);

    if (tt_os2)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, os2_context);
    if (name_context)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, name_context);

    return hr;
}

/* FaceName locating order is WWS Face Name -> Preferred Face Name -> Face Name. If font claims to
   have 'Preferred Face Name' in WWS format, then WWS name is not used. */
HRESULT opentype_get_font_facename(struct file_stream_desc *stream_desc, WCHAR *lfname, IDWriteLocalizedStrings **names)
{
    IDWriteLocalizedStrings *lfnames;
    void *os2_context, *name_context;
    const TT_OS2_V2 *tt_os2;
    const void *name_table;
    HRESULT hr;

    opentype_get_font_table(stream_desc, MS_OS2_TAG,  (const void**)&tt_os2, &os2_context, NULL, NULL);
    opentype_get_font_table(stream_desc, MS_NAME_TAG, &name_table, &name_context, NULL, NULL);

    *names = NULL;

    /* if Preferred Family doesn't conform to WWS model try WWS name */
    if (tt_os2 && !(GET_BE_WORD(tt_os2->fsSelection) & OS2_FSSELECTION_WWS))
        hr = opentype_get_font_strings_from_id(name_table, OPENTYPE_STRING_WWS_SUBFAMILY_NAME, names);
    else
        hr = E_FAIL;

    if (FAILED(hr))
        hr = opentype_get_font_strings_from_id(name_table, OPENTYPE_STRING_PREFERRED_SUBFAMILY_NAME, names);
    if (FAILED(hr))
        hr = opentype_get_font_strings_from_id(name_table, OPENTYPE_STRING_SUBFAMILY_NAME, names);

    /* User locale is preferred, with fallback to en-us. */
    *lfname = 0;
    if (SUCCEEDED(opentype_get_font_strings_from_id(name_table, OPENTYPE_STRING_FAMILY_NAME, &lfnames))) {
        static const WCHAR enusW[] = {'e','n','-','u','s',0};
        WCHAR localeW[LOCALE_NAME_MAX_LENGTH];
        UINT32 index;
        BOOL exists;

        exists = FALSE;
        if (GetSystemDefaultLocaleName(localeW, ARRAY_SIZE(localeW)))
            IDWriteLocalizedStrings_FindLocaleName(lfnames, localeW, &index, &exists);

        if (!exists)
            IDWriteLocalizedStrings_FindLocaleName(lfnames, enusW, &index, &exists);

        if (exists) {
            UINT32 length = 0;
            WCHAR *nameW;

            IDWriteLocalizedStrings_GetStringLength(lfnames, index, &length);
            nameW = heap_alloc((length + 1) * sizeof(WCHAR));
            if (nameW) {
                *nameW = 0;
                IDWriteLocalizedStrings_GetString(lfnames, index, nameW, length + 1);
                lstrcpynW(lfname, nameW, LF_FACESIZE);
                heap_free(nameW);
            }
        }

        IDWriteLocalizedStrings_Release(lfnames);
    }

    if (tt_os2)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, os2_context);
    if (name_context)
        IDWriteFontFileStream_ReleaseFileFragment(stream_desc->stream, name_context);

    return hr;
}

static inline const struct ot_script *opentype_get_script(const struct ot_script_list *scriptlist, UINT32 scripttag)
{
    UINT16 j;

    for (j = 0; j < GET_BE_WORD(scriptlist->script_count); j++) {
        const char *tag = scriptlist->scripts[j].tag;
        if (scripttag == DWRITE_MAKE_OPENTYPE_TAG(tag[0], tag[1], tag[2], tag[3]))
            return (struct ot_script*)((BYTE*)scriptlist + GET_BE_WORD(scriptlist->scripts[j].script));
    }

    return NULL;
}

static inline const struct ot_langsys *opentype_get_langsys(const struct ot_script *script, UINT32 languagetag)
{
    UINT16 j;

    for (j = 0; j < GET_BE_WORD(script->langsys_count); j++) {
        const char *tag = script->langsys[j].tag;
        if (languagetag == DWRITE_MAKE_OPENTYPE_TAG(tag[0], tag[1], tag[2], tag[3]))
            return (struct ot_langsys *)((BYTE*)script + GET_BE_WORD(script->langsys[j].langsys));
    }

    return NULL;
}

static void opentype_add_font_features(const struct gpos_gsub_header *header, const struct ot_langsys *langsys,
    UINT32 max_tagcount, UINT32 *count, DWRITE_FONT_FEATURE_TAG *tags)
{
    const struct ot_feature_list *features = (const struct ot_feature_list *)((const BYTE*)header + GET_BE_WORD(header->feature_list));
    UINT16 j;

    for (j = 0; j < GET_BE_WORD(langsys->feature_count); j++) {
        const struct ot_feature_record *feature = &features->features[langsys->feature_index[j]];

        if (*count < max_tagcount)
            tags[*count] = GET_BE_DWORD(feature->tag);

        (*count)++;
    }
}

HRESULT opentype_get_typographic_features(IDWriteFontFace *fontface, UINT32 scripttag, UINT32 languagetag, UINT32 max_tagcount,
    UINT32 *count, DWRITE_FONT_FEATURE_TAG *tags)
{
    UINT32 tables[2] = { MS_GSUB_TAG, MS_GPOS_TAG };
    HRESULT hr;
    UINT8 i;

    *count = 0;
    for (i = 0; i < ARRAY_SIZE(tables); i++) {
        const struct ot_script_list *scriptlist;
        const struct gpos_gsub_header *header;
        const struct ot_script *script;
        const void *ptr;
        void *context;
        UINT32 size;
        BOOL exists;

        exists = FALSE;
        hr = IDWriteFontFace_TryGetFontTable(fontface, tables[i], &ptr, &size, &context, &exists);
        if (FAILED(hr))
            return hr;

        if (!exists)
            continue;

        header = (const struct gpos_gsub_header *)ptr;
        scriptlist = (const struct ot_script_list *)((const BYTE*)header + GET_BE_WORD(header->script_list));

        script = opentype_get_script(scriptlist, scripttag);
        if (script) {
            const struct ot_langsys *langsys = opentype_get_langsys(script, languagetag);
            if (langsys)
                opentype_add_font_features(header, langsys, max_tagcount, count, tags);
        }

        IDWriteFontFace_ReleaseFontTable(fontface, context);
    }

    return *count > max_tagcount ? E_NOT_SUFFICIENT_BUFFER : S_OK;
}

static unsigned int find_vdmx_group(const struct vdmx_header *hdr)
{
    WORD num_ratios, i;
    const struct vdmx_ratio *ratios = (struct vdmx_ratio *)(hdr + 1);
    BYTE dev_x_ratio = 1, dev_y_ratio = 1;
    unsigned int group_offset = 0;

    num_ratios = GET_BE_WORD(hdr->num_ratios);

    for (i = 0; i < num_ratios; i++) {

        if (!ratios[i].bCharSet) continue;

        if ((ratios[i].xRatio == 0 && ratios[i].yStartRatio == 0 &&
             ratios[i].yEndRatio == 0) ||
	   (ratios[i].xRatio == dev_x_ratio && ratios[i].yStartRatio <= dev_y_ratio &&
            ratios[i].yEndRatio >= dev_y_ratio))
        {
            group_offset = GET_BE_WORD(*((WORD *)(ratios + num_ratios) + i));
            break;
        }
    }

    return group_offset;
}

BOOL opentype_get_vdmx_size(const struct dwrite_fonttable *vdmx, INT emsize, UINT16 *ascent, UINT16 *descent)
{
    unsigned int num_ratios, num_recs, group_offset, i;
    const struct vdmx_header *header;
    const struct vdmx_group *group;

    if (!vdmx->exists)
        return FALSE;

    num_ratios = table_read_be_word(vdmx, FIELD_OFFSET(struct vdmx_header, num_ratios));
    num_recs = table_read_be_word(vdmx, FIELD_OFFSET(struct vdmx_header, num_recs));

    header = table_read_ensure(vdmx, 0, sizeof(*header) + num_ratios * sizeof(struct vdmx_ratio) +
            num_recs * sizeof(*group));

    if (!header)
        return FALSE;

    group_offset = find_vdmx_group(header);
    if (!group_offset)
        return FALSE;

    num_recs = table_read_be_word(vdmx, group_offset);
    group = table_read_ensure(vdmx, group_offset, FIELD_OFFSET(struct vdmx_group, entries[num_recs]));

    if (!group)
        return FALSE;

    if (emsize < group->startsz || emsize >= group->endsz)
        return FALSE;

    for (i = 0; i < num_recs; ++i)
    {
        WORD ppem = GET_BE_WORD(group->entries[i].yPelHeight);
        if (ppem > emsize) {
            FIXME("interpolate %d\n", emsize);
            return FALSE;
        }

        if (ppem == emsize) {
            *ascent = (SHORT)GET_BE_WORD(group->entries[i].yMax);
            *descent = -(SHORT)GET_BE_WORD(group->entries[i].yMin);
            return TRUE;
        }
    }

    return FALSE;
}

unsigned int opentype_get_gasp_flags(const struct dwrite_fonttable *gasp, float emsize)
{
    unsigned int version, num_ranges, i;
    const struct gasp_header *table;
    WORD flags = 0;

    if (!gasp->exists)
        return 0;

    num_ranges = table_read_be_word(gasp, FIELD_OFFSET(struct gasp_header, num_ranges));

    table = table_read_ensure(gasp, 0, FIELD_OFFSET(struct gasp_header, ranges[num_ranges]));
    if (!table)
        return 0;

    version = GET_BE_WORD(table->version);
    if (version > 1)
    {
        ERR("Unsupported gasp table format version %u.\n", version);
        goto done;
    }

    for (i = 0; i < num_ranges; ++i)
    {
        flags = GET_BE_WORD(table->ranges[i].flags);
        if (emsize <= GET_BE_WORD(table->ranges[i].max_ppem)) break;
    }

done:
    return flags;
}

unsigned int opentype_get_cpal_palettecount(const struct dwrite_fonttable *cpal)
{
    return table_read_be_word(cpal, FIELD_OFFSET(struct cpal_header_0, num_palettes));
}

unsigned int opentype_get_cpal_paletteentrycount(const struct dwrite_fonttable *cpal)
{
    return table_read_be_word(cpal, FIELD_OFFSET(struct cpal_header_0, num_palette_entries));
}

HRESULT opentype_get_cpal_entries(const struct dwrite_fonttable *cpal, unsigned int palette,
        unsigned int first_entry_index, unsigned int entry_count, DWRITE_COLOR_F *entries)
{
    unsigned int num_palettes, num_palette_entries, i;
    const struct cpal_color_record *records;
    const struct cpal_header_0 *header;

    header = table_read_ensure(cpal, 0, sizeof(*header));

    if (!cpal->exists || !header)
        return DWRITE_E_NOCOLOR;

    num_palettes = GET_BE_WORD(header->num_palettes);
    if (palette >= num_palettes)
        return DWRITE_E_NOCOLOR;

    header = table_read_ensure(cpal, 0, FIELD_OFFSET(struct cpal_header_0, color_record_indices[palette]));
    if (!header)
        return DWRITE_E_NOCOLOR;

    num_palette_entries = GET_BE_WORD(header->num_palette_entries);
    if (first_entry_index + entry_count > num_palette_entries)
        return E_INVALIDARG;

    records = table_read_ensure(cpal, GET_BE_DWORD(header->offset_first_color_record),
            sizeof(*records) * GET_BE_WORD(header->num_color_records));
    if (!records)
        return DWRITE_E_NOCOLOR;

    first_entry_index += GET_BE_WORD(header->color_record_indices[palette]);

    for (i = 0; i < entry_count; i++) {
        entries[i].u1.r = records[first_entry_index + i].red   / 255.0f;
        entries[i].u2.g = records[first_entry_index + i].green / 255.0f;
        entries[i].u3.b = records[first_entry_index + i].blue  / 255.0f;
        entries[i].u4.a = records[first_entry_index + i].alpha / 255.0f;
    }

    return S_OK;
}

static int colr_compare_gid(const void *g, const void *r)
{
    const struct colr_baseglyph_record *record = r;
    UINT16 glyph = *(UINT16*)g, GID = GET_BE_WORD(record->glyph);
    int ret = 0;

    if (glyph > GID)
        ret = 1;
    else if (glyph < GID)
        ret = -1;

    return ret;
}

HRESULT opentype_get_colr_glyph(const struct dwrite_fonttable *colr, UINT16 glyph, struct dwrite_colorglyph *ret)
{
    unsigned int num_baseglyph_records, offset_baseglyph_records;
    const struct colr_baseglyph_record *record;
    const struct colr_layer_record *layer;
    const struct colr_header *header;

    memset(ret, 0, sizeof(*ret));
    ret->glyph = glyph;
    ret->palette_index = 0xffff;

    header = table_read_ensure(colr, 0, sizeof(*header));
    if (!header)
        return S_FALSE;

    num_baseglyph_records = GET_BE_WORD(header->num_baseglyph_records);
    offset_baseglyph_records = GET_BE_DWORD(header->offset_baseglyph_records);
    if (!table_read_ensure(colr, offset_baseglyph_records, num_baseglyph_records * sizeof(*record)))
    {
        return S_FALSE;
    }

    record = bsearch(&glyph, colr->data + offset_baseglyph_records, num_baseglyph_records,
            sizeof(*record), colr_compare_gid);
    if (!record)
        return S_FALSE;

    ret->first_layer = GET_BE_WORD(record->first_layer_index);
    ret->num_layers = GET_BE_WORD(record->num_layers);

    if ((layer = table_read_ensure(colr, GET_BE_DWORD(header->offset_layer_records),
            (ret->first_layer + ret->layer) * sizeof(*layer))))
    {
        layer += ret->first_layer + ret->layer;
        ret->glyph = GET_BE_WORD(layer->glyph);
        ret->palette_index = GET_BE_WORD(layer->palette_index);
    }

    return S_OK;
}

void opentype_colr_next_glyph(const struct dwrite_fonttable *colr, struct dwrite_colorglyph *glyph)
{
    const struct colr_layer_record *layer;
    const struct colr_header *header;

    /* iterated all the way through */
    if (glyph->layer == glyph->num_layers)
        return;

    if (!(header = table_read_ensure(colr, 0, sizeof(*header))))
        return;

    glyph->layer++;

    if ((layer = table_read_ensure(colr, GET_BE_DWORD(header->offset_layer_records),
            (glyph->first_layer + glyph->layer) * sizeof(*layer))))
    {
        layer += glyph->first_layer + glyph->layer;
        glyph->glyph = GET_BE_WORD(layer->glyph);
        glyph->palette_index = GET_BE_WORD(layer->palette_index);
    }
}

BOOL opentype_has_vertical_variants(IDWriteFontFace4 *fontface)
{
    const struct gpos_gsub_header *header;
    const struct ot_feature_list *featurelist;
    const struct ot_lookup_list *lookup_list;
    BOOL exists = FALSE, ret = FALSE;
    unsigned int i, j;
    const void *data;
    void *context;
    UINT32 size;
    HRESULT hr;

    hr = IDWriteFontFace4_TryGetFontTable(fontface, MS_GSUB_TAG, &data, &size, &context, &exists);
    if (FAILED(hr) || !exists)
        return FALSE;

    header = data;
    featurelist = (struct ot_feature_list *)((BYTE*)header + GET_BE_WORD(header->feature_list));
    lookup_list = (const struct ot_lookup_list *)((BYTE*)header + GET_BE_WORD(header->lookup_list));

    for (i = 0; i < GET_BE_WORD(featurelist->feature_count); i++) {
        if (featurelist->features[i].tag == DWRITE_FONT_FEATURE_TAG_VERTICAL_WRITING) {
            const struct ot_feature *feature = (const struct ot_feature*)((BYTE*)featurelist + GET_BE_WORD(featurelist->features[i].offset));
            UINT16 lookup_count = GET_BE_WORD(feature->lookup_count), index, count, type;
            const GSUB_SingleSubstFormat2 *subst2;
            const struct ot_lookup_table *lookup_table;
            UINT32 offset;

            if (lookup_count == 0)
                continue;

            for (j = 0; j < lookup_count; ++j) {
                /* check if lookup is empty */
                index = GET_BE_WORD(feature->lookuplist_index[j]);
                lookup_table = (const struct ot_lookup_table *)((BYTE*)lookup_list + GET_BE_WORD(lookup_list->lookup[index]));

                type = GET_BE_WORD(lookup_table->lookup_type);
                if (type != GSUB_LOOKUP_SINGLE_SUBST && type != GSUB_LOOKUP_EXTENSION_SUBST)
                    continue;

                count = GET_BE_WORD(lookup_table->subtable_count);
                if (count == 0)
                    continue;

                offset = GET_BE_WORD(lookup_table->subtable[0]);
                if (type == GSUB_LOOKUP_EXTENSION_SUBST) {
                    const GSUB_ExtensionPosFormat1 *ext = (const GSUB_ExtensionPosFormat1 *)((const BYTE *)lookup_table + offset);
                    if (GET_BE_WORD(ext->SubstFormat) == 1)
                        offset += GET_BE_DWORD(ext->ExtensionOffset);
                    else
                        FIXME("Unhandled Extension Substitution Format %u\n", GET_BE_WORD(ext->SubstFormat));
                }

                subst2 = (const GSUB_SingleSubstFormat2*)((BYTE*)lookup_table + offset);
                index = GET_BE_WORD(subst2->SubstFormat);
                if (index == 1)
                    FIXME("Validate Single Substitution Format 1\n");
                else if (index == 2) {
                    /* SimSun-ExtB has 0 glyph count for this substitution */
                    if (GET_BE_WORD(subst2->GlyphCount) > 0) {
                        ret = TRUE;
                        break;
                    }
                }
                else
                    WARN("Unknown Single Substitution Format, %u\n", index);
            }
        }
    }

    IDWriteFontFace4_ReleaseFontTable(fontface, context);

    return ret;
}

static BOOL opentype_has_font_table(IDWriteFontFace4 *fontface, UINT32 tag)
{
    BOOL exists = FALSE;
    const void *data;
    void *context;
    UINT32 size;
    HRESULT hr;

    hr = IDWriteFontFace4_TryGetFontTable(fontface, tag, &data, &size, &context, &exists);
    if (FAILED(hr))
        return FALSE;

    if (exists)
        IDWriteFontFace4_ReleaseFontTable(fontface, context);

    return exists;
}

static unsigned int opentype_get_sbix_formats(IDWriteFontFace4 *fontface)
{
    unsigned int num_strikes, num_glyphs, i, j, ret = 0;
    const struct sbix_header *sbix_header;
    struct dwrite_fonttable table;

    memset(&table, 0, sizeof(table));
    table.exists = TRUE;

    if (!get_fontface_table(fontface, MS_MAXP_TAG, &table))
        return 0;

    num_glyphs = table_read_be_word(&table, FIELD_OFFSET(struct maxp, num_glyphs));

    IDWriteFontFace4_ReleaseFontTable(fontface, table.context);

    memset(&table, 0, sizeof(table));
    table.exists = TRUE;

    if (!get_fontface_table(fontface, MS_SBIX_TAG, &table))
        return 0;

    num_strikes = table_read_be_dword(&table, FIELD_OFFSET(struct sbix_header, num_strikes));
    sbix_header = table_read_ensure(&table, 0, FIELD_OFFSET(struct sbix_header, strike_offset[num_strikes]));

    if (sbix_header)
    {
        for (i = 0; i < num_strikes; ++i)
        {
            unsigned int strike_offset = GET_BE_DWORD(sbix_header->strike_offset[i]);
            const struct sbix_strike *strike = table_read_ensure(&table, strike_offset,
                    FIELD_OFFSET(struct sbix_strike, glyphdata_offsets[num_glyphs + 1]));

            if (!strike)
                continue;

            for (j = 0; j < num_glyphs; j++)
            {
                unsigned int offset = GET_BE_DWORD(strike->glyphdata_offsets[j]);
                unsigned int next_offset = GET_BE_DWORD(strike->glyphdata_offsets[j + 1]);
                const struct sbix_glyph_data *glyph_data;

                if (offset == next_offset)
                    continue;

                glyph_data = table_read_ensure(&table, strike_offset + offset, sizeof(*glyph_data));
                if (!glyph_data)
                    continue;

                switch (glyph_data->graphic_type)
                {
                    case MS_PNG__TAG:
                        ret |= DWRITE_GLYPH_IMAGE_FORMATS_PNG;
                        break;
                    case MS_JPG__TAG:
                        ret |= DWRITE_GLYPH_IMAGE_FORMATS_JPEG;
                        break;
                    case MS_TIFF_TAG:
                        ret |= DWRITE_GLYPH_IMAGE_FORMATS_TIFF;
                        break;
                    default:
                        FIXME("unexpected bitmap format %s\n", debugstr_tag(GET_BE_DWORD(glyph_data->graphic_type)));
                }
            }
        }
    }

    IDWriteFontFace4_ReleaseFontTable(fontface, table.context);

    return ret;
}

static unsigned int opentype_get_cblc_formats(IDWriteFontFace4 *fontface)
{
    const unsigned int format_mask = DWRITE_GLYPH_IMAGE_FORMATS_PNG |
            DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8;
    const struct cblc_bitmapsize_table *sizes;
    struct dwrite_fonttable cblc = { 0 };
    unsigned int num_sizes, i, ret = 0;
    const struct cblc_header *header;

    cblc.exists = TRUE;
    if (!get_fontface_table(fontface, MS_CBLC_TAG, &cblc))
        return 0;

    num_sizes = table_read_be_dword(&cblc, FIELD_OFFSET(struct cblc_header, num_sizes));
    sizes = table_read_ensure(&cblc, sizeof(*header), num_sizes * sizeof(*sizes));

    if (sizes)
    {
        for (i = 0; i < num_sizes; ++i)
        {
            BYTE bpp = sizes[i].bit_depth;

            if ((ret & format_mask) == format_mask)
                break;

            if (bpp == 1 || bpp == 2 || bpp == 4 || bpp == 8)
                ret |= DWRITE_GLYPH_IMAGE_FORMATS_PNG;
            else if (bpp == 32)
                ret |= DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8;
        }
    }

    IDWriteFontFace4_ReleaseFontTable(fontface, cblc.context);

    return ret;
}

UINT32 opentype_get_glyph_image_formats(IDWriteFontFace4 *fontface)
{
    UINT32 ret = DWRITE_GLYPH_IMAGE_FORMATS_NONE;

    if (opentype_has_font_table(fontface, MS_GLYF_TAG))
        ret |= DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE;

    if (opentype_has_font_table(fontface, MS_CFF__TAG) ||
            opentype_has_font_table(fontface, MS_CFF2_TAG))
        ret |= DWRITE_GLYPH_IMAGE_FORMATS_CFF;

    if (opentype_has_font_table(fontface, MS_COLR_TAG))
        ret |= DWRITE_GLYPH_IMAGE_FORMATS_COLR;

    if (opentype_has_font_table(fontface, MS_SVG__TAG))
        ret |= DWRITE_GLYPH_IMAGE_FORMATS_SVG;

    if (opentype_has_font_table(fontface, MS_SBIX_TAG))
        ret |= opentype_get_sbix_formats(fontface);

    if (opentype_has_font_table(fontface, MS_CBLC_TAG))
        ret |= opentype_get_cblc_formats(fontface);

    return ret;
}

DWRITE_CONTAINER_TYPE opentype_analyze_container_type(void const *data, UINT32 data_size)
{
    DWORD signature;

    if (data_size < sizeof(DWORD))
        return DWRITE_CONTAINER_TYPE_UNKNOWN;

    /* Both WOFF and WOFF2 start with 4 bytes signature. */
    signature = *(DWORD *)data;

    switch (signature)
    {
    case MS_WOFF_TAG:
        return DWRITE_CONTAINER_TYPE_WOFF;
    case MS_WOF2_TAG:
        return DWRITE_CONTAINER_TYPE_WOFF2;
    default:
        return DWRITE_CONTAINER_TYPE_UNKNOWN;
    }
}

void opentype_layout_scriptshaping_cache_init(struct scriptshaping_cache *cache)
{
    cache->font->grab_font_table(cache->context, MS_GPOS_TAG, &cache->gpos.table.data, &cache->gpos.table.size,
            &cache->gpos.table.context);

    if (cache->gpos.table.data)
    {
        cache->gpos.script_list = table_read_be_word(&cache->gpos.table,
                FIELD_OFFSET(struct gpos_gsub_header, script_list));
        cache->gpos.feature_list = table_read_be_word(&cache->gpos.table,
                FIELD_OFFSET(struct gpos_gsub_header, feature_list));
        cache->gpos.lookup_list = table_read_be_word(&cache->gpos.table,
                FIELD_OFFSET(struct gpos_gsub_header, lookup_list));
    }

    cache->font->grab_font_table(cache->context, MS_GDEF_TAG, &cache->gdef.table.data, &cache->gdef.table.size,
            &cache->gdef.table.context);

    if (cache->gdef.table.data)
        cache->gdef.classdef = table_read_be_word(&cache->gdef.table, FIELD_OFFSET(struct gdef_header, classdef));
}

DWORD opentype_layout_find_script(const struct scriptshaping_cache *cache, DWORD kind, DWORD script,
        unsigned int *script_index)
{
    WORD script_count;
    unsigned int i;

    *script_index = ~0u;

    if (kind != MS_GPOS_TAG)
        return 0;

    script_count = table_read_be_word(&cache->gpos.table, cache->gpos.script_list);
    if (!script_count)
        return 0;

    for (i = 0; i < script_count; i++)
    {
        DWORD tag = table_read_dword(&cache->gpos.table, cache->gpos.script_list +
                FIELD_OFFSET(struct ot_script_list, scripts) + i * sizeof(struct ot_script_record));
        if (!tag)
            continue;

        if (tag == script)
        {
            *script_index = i;
            return script;
        }
    }

    return 0;
}

DWORD opentype_layout_find_language(const struct scriptshaping_cache *cache, DWORD kind, DWORD language,
        unsigned int script_index, unsigned int *language_index)
{
    WORD table_offset, lang_count;
    unsigned int i;

    *language_index = ~0u;

    if (kind != MS_GPOS_TAG)
        return 0;

    table_offset = table_read_be_word(&cache->gpos.table, cache->gpos.script_list +
            FIELD_OFFSET(struct ot_script_list, scripts) + script_index * sizeof(struct ot_script_record) +
            FIELD_OFFSET(struct ot_script_record, script));
    if (!table_offset)
        return 0;

    lang_count = table_read_be_word(&cache->gpos.table, cache->gpos.script_list + table_offset +
            FIELD_OFFSET(struct ot_script, langsys_count));
    for (i = 0; i < lang_count; i++)
    {
        DWORD tag = table_read_dword(&cache->gpos.table, cache->gpos.script_list + table_offset +
                FIELD_OFFSET(struct ot_script, langsys) + i * sizeof(struct ot_langsys_record));

        if (tag == language)
        {
            *language_index = i;
            return language;
        }
    }

    /* Try 'defaultLangSys' if it's set. */
    if (table_read_be_word(&cache->gpos.table, cache->gpos.script_list + table_offset))
        return ~0u;

    return 0;
}

static int gdef_class_compare_format2(const void *g, const void *r)
{
    const struct ot_gdef_class_range *range = r;
    UINT16 glyph = *(UINT16 *)g;

    if (glyph < GET_BE_WORD(range->start_glyph))
        return -1;
    else if (glyph > GET_BE_WORD(range->end_glyph))
        return 1;
    else
        return 0;
}

static unsigned int opentype_layout_get_glyph_class(const struct dwrite_fonttable *table,
        unsigned int offset, UINT16 glyph)
{
    WORD format = table_read_be_word(table, offset), count;
    unsigned int glyph_class = GDEF_CLASS_UNCLASSIFIED;

    if (format == 1)
    {
        const struct ot_gdef_classdef_format1 *format1;

        count = table_read_be_word(table, offset + FIELD_OFFSET(struct ot_gdef_classdef_format1, glyph_count));
        format1 = table_read_ensure(table, offset, FIELD_OFFSET(struct ot_gdef_classdef_format1, classes[count]));
        if (format1)
        {
            WORD start_glyph = GET_BE_WORD(format1->start_glyph);
            if (glyph >= start_glyph && (glyph - start_glyph) < count)
            {
                glyph_class = GET_BE_WORD(format1->classes[glyph - start_glyph]);
                if (glyph_class > GDEF_CLASS_MAX)
                     glyph_class = GDEF_CLASS_UNCLASSIFIED;
            }
        }
    }
    else if (format == 2)
    {
        const struct ot_gdef_classdef_format2 *format2;

        count = table_read_be_word(table, offset + FIELD_OFFSET(struct ot_gdef_classdef_format2, range_count));
        format2 = table_read_ensure(table, offset, FIELD_OFFSET(struct ot_gdef_classdef_format2, ranges[count]));
        if (format2)
        {
            const struct ot_gdef_class_range *range = bsearch(&glyph, format2->ranges, count,
                    sizeof(struct ot_gdef_class_range), gdef_class_compare_format2);
            glyph_class = range && glyph <= GET_BE_WORD(range->end_glyph) ?
                    GET_BE_WORD(range->glyph_class) : GDEF_CLASS_UNCLASSIFIED;
            if (glyph_class > GDEF_CLASS_MAX)
                 glyph_class = GDEF_CLASS_UNCLASSIFIED;
        }
    }
    else
        WARN("Unknown GDEF format %u.\n", format);

    return glyph_class;
}

struct coverage_compare_format1_context
{
    UINT16 glyph;
    const UINT16 *table_base;
    unsigned int *coverage_index;
};

static int coverage_compare_format1(const void *left, const void *right)
{
    const struct coverage_compare_format1_context *context = left;
    UINT16 glyph = GET_BE_WORD(*(UINT16 *)right);
    int ret;

    ret = context->glyph - glyph;
    if (!ret)
        *context->coverage_index = (UINT16 *)right - context->table_base;

    return ret;
}

static int coverage_compare_format2(const void *g, const void *r)
{
    const struct ot_coverage_range *range = r;
    UINT16 glyph = *(UINT16 *)g;

    if (glyph < GET_BE_WORD(range->start_glyph))
        return -1;
    else if (glyph > GET_BE_WORD(range->end_glyph))
        return 1;
    else
        return 0;
}

static unsigned int opentype_layout_is_glyph_covered(const struct dwrite_fonttable *table, DWORD coverage,
        UINT16 glyph)
{
    WORD format = table_read_be_word(table, coverage), count;

    count = table_read_be_word(table, coverage + 2);

    if (format == 1)
    {
        const struct ot_coverage_format1 *format1 = table_read_ensure(table, coverage,
                FIELD_OFFSET(struct ot_coverage_format1, glyphs[count]));
        struct coverage_compare_format1_context context;
        unsigned int coverage_index = GLYPH_NOT_COVERED;

        if (format1)
        {
            context.glyph = glyph;
            context.table_base = format1->glyphs;
            context.coverage_index = &coverage_index;

            bsearch(&context, format1->glyphs, count, sizeof(glyph), coverage_compare_format1);
        }

        return coverage_index;
    }
    else if (format == 2)
    {
        const struct ot_coverage_format2 *format2 = table_read_ensure(table, coverage,
                FIELD_OFFSET(struct ot_coverage_format2, ranges[count]));
        if (format2)
        {
            const struct ot_coverage_range *range = bsearch(&glyph, format2->ranges, count,
                    sizeof(struct ot_coverage_range), coverage_compare_format2);
            return range && glyph <= GET_BE_WORD(range->end_glyph) ?
                    GET_BE_WORD(range->startcoverage_index) + glyph - GET_BE_WORD(range->start_glyph) :
                    GLYPH_NOT_COVERED;
        }
    }
    else
        WARN("Unknown coverage format %u.\n", format);

    return -1;
}

static inline unsigned int dwrite_popcount(unsigned int x)
{
#ifdef HAVE___BUILTIN_POPCOUNT
    return __builtin_popcount(x);
#else
    x -= x >> 1 & 0x55555555;
    x = (x & 0x33333333) + (x >> 2 & 0x33333333);
    return ((x + (x >> 4)) & 0x0f0f0f0f) * 0x01010101 >> 24;
#endif
}

static float opentype_scale_gpos_be_value(WORD value, float emsize, UINT16 upem)
{
    return (short)GET_BE_WORD(value) * emsize / upem;
}

static int opentype_layout_gpos_get_dev_value(const struct scriptshaping_context *context, unsigned int offset)
{
    const struct scriptshaping_cache *cache = context->cache;
    unsigned int start_size, end_size, format, value_word;
    unsigned int index, ppem, mask;
    int value;

    if (!offset)
        return 0;

    start_size = table_read_be_word(&cache->gpos.table, offset);
    end_size = table_read_be_word(&cache->gpos.table, offset + FIELD_OFFSET(struct ot_gpos_device_table, end_size));

    ppem = context->emsize;
    if (ppem < start_size || ppem > end_size)
        return 0;

    format = table_read_be_word(&cache->gpos.table, offset + FIELD_OFFSET(struct ot_gpos_device_table, format));

    if (format < 1 || format > 3)
        return 0;

    index = ppem - start_size;

    value_word = table_read_be_word(&cache->gpos.table, offset +
            FIELD_OFFSET(struct ot_gpos_device_table, values[index >> (4 - format)]));
    mask = 0xffff >> (16 - (1 << format));

    value = (value_word >> ((index % (4 - format)) * (1 << format))) & mask;

    if ((unsigned int)value >= ((mask + 1) >> 1))
        value -= mask + 1;

    return value;
}

static void opentype_layout_apply_gpos_value(struct scriptshaping_context *context, unsigned int table_offset,
        WORD value_format, const WORD *values, unsigned int glyph)
{
    const struct scriptshaping_cache *cache = context->cache;
    DWRITE_GLYPH_OFFSET *offset = &context->offsets[glyph];
    float *advance = &context->advances[glyph];

    if (!value_format)
        return;

    if (value_format & GPOS_VALUE_X_PLACEMENT)
    {
        offset->advanceOffset += opentype_scale_gpos_be_value(*values, context->emsize, cache->upem);
        values++;
    }
    if (value_format & GPOS_VALUE_Y_PLACEMENT)
    {
        offset->ascenderOffset += opentype_scale_gpos_be_value(*values, context->emsize, cache->upem);
        values++;
    }
    if (value_format & GPOS_VALUE_X_ADVANCE)
    {
        *advance += opentype_scale_gpos_be_value(*values, context->emsize, cache->upem);
        values++;
    }
    if (value_format & GPOS_VALUE_Y_ADVANCE)
    {
        values++;
    }
    if (value_format & GPOS_VALUE_X_PLACEMENT_DEVICE)
    {
        offset->advanceOffset += opentype_layout_gpos_get_dev_value(context, table_offset + GET_BE_WORD(*values));
        values++;
    }
    if (value_format & GPOS_VALUE_Y_PLACEMENT_DEVICE)
    {
        offset->ascenderOffset += opentype_layout_gpos_get_dev_value(context, table_offset + GET_BE_WORD(*values));
        values++;
    }
    if (value_format & GPOS_VALUE_X_ADVANCE_DEVICE)
    {
        *advance += opentype_layout_gpos_get_dev_value(context, table_offset + GET_BE_WORD(*values));
        values++;
    }
    if (value_format & GPOS_VALUE_Y_ADVANCE_DEVICE)
    {
        values++;
    }
}

static unsigned int opentype_layout_get_gpos_subtable(const struct scriptshaping_cache *cache,
        unsigned int lookup_offset, unsigned int subtable)
{
    WORD lookup_type = table_read_be_word(&cache->gpos.table, lookup_offset);
    unsigned int subtable_offset = table_read_be_word(&cache->gpos.table, lookup_offset +
            FIELD_OFFSET(struct ot_lookup_table, subtable[subtable]));
    if (lookup_type == GPOS_LOOKUP_EXTENSION_POSITION)
    {
        const struct ot_gpos_extensionpos_format1 *format1 = table_read_ensure(&cache->gpos.table,
                lookup_offset + subtable_offset, sizeof(*format1));
        subtable_offset += GET_BE_DWORD(format1->extension_offset);
    }

    return lookup_offset + subtable_offset;
}

struct lookup
{
    unsigned int offset;
    unsigned int subtable_count;
    unsigned int flags;
};

struct glyph_iterator
{
    const struct scriptshaping_context *context;
    unsigned int flags;
    unsigned int pos;
    unsigned int len;
};

static void glyph_iterator_init(const struct scriptshaping_context *context, unsigned int flags, unsigned int pos,
        unsigned int len, struct glyph_iterator *iter)
{
    iter->context = context;
    iter->flags = flags;
    iter->pos = pos;
    iter->len = len;
}

static BOOL glyph_iterator_match(const struct glyph_iterator *iter)
{
    struct scriptshaping_cache *cache = iter->context->cache;

    if (cache->gdef.classdef)
    {
        unsigned int glyph_class = opentype_layout_get_glyph_class(&cache->gdef.table, cache->gdef.classdef,
                iter->context->u.pos.glyphs[iter->pos]);
        if ((1 << glyph_class) & iter->flags & LOOKUP_FLAG_IGNORE_MASK)
            return FALSE;
    }

    return TRUE;
}

static BOOL glyph_iterator_next(struct glyph_iterator *iter)
{
    while (iter->pos + iter->len < iter->context->glyph_count)
    {
        ++iter->pos;
        if (glyph_iterator_match(iter))
        {
            --iter->len;
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL glyph_iterator_prev(struct glyph_iterator *iter)
{
    if (!iter->pos)
        return FALSE;

    while (iter->pos > iter->len - 1)
    {
        --iter->pos;
        if (glyph_iterator_match(iter))
        {
            --iter->len;
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL opentype_layout_apply_gpos_single_adjustment(struct scriptshaping_context *context,
        struct glyph_iterator *iter, const struct lookup *lookup)
{
    struct scriptshaping_cache *cache = context->cache;
    WORD format, value_format, value_len, coverage;
    unsigned int i;

    for (i = 0; i < lookup->subtable_count; ++i)
    {
        unsigned int subtable_offset = opentype_layout_get_gpos_subtable(cache, lookup->offset, i);
        unsigned int coverage_index;

        format = table_read_be_word(&cache->gpos.table, subtable_offset);

        coverage = table_read_be_word(&cache->gpos.table, subtable_offset +
                FIELD_OFFSET(struct ot_gpos_singlepos_format1, coverage));
        value_format = table_read_be_word(&cache->gpos.table, subtable_offset +
                FIELD_OFFSET(struct ot_gpos_singlepos_format1, value_format));
        value_len = dwrite_popcount(value_format);

        if (format == 1)
        {
            const struct ot_gpos_singlepos_format1 *format1 = table_read_ensure(&cache->gpos.table, subtable_offset,
                    FIELD_OFFSET(struct ot_gpos_singlepos_format1, value[value_len]));

            coverage_index = opentype_layout_is_glyph_covered(&cache->gpos.table, subtable_offset + coverage,
                    context->u.pos.glyphs[iter->pos]);
            if (coverage_index == GLYPH_NOT_COVERED)
                continue;

            opentype_layout_apply_gpos_value(context, subtable_offset, value_format, format1->value, iter->pos);
            break;
        }
        else if (format == 2)
        {
            WORD value_count = table_read_be_word(&cache->gpos.table, subtable_offset +
                    FIELD_OFFSET(struct ot_gpos_singlepos_format2, value_count));
            const struct ot_gpos_singlepos_format2 *format2 = table_read_ensure(&cache->gpos.table, subtable_offset,
                    FIELD_OFFSET(struct ot_gpos_singlepos_format2, values) + value_count * value_len * sizeof(WORD));

            coverage_index = opentype_layout_is_glyph_covered(&cache->gpos.table, subtable_offset + coverage,
                    context->u.pos.glyphs[iter->pos]);
            if (coverage_index == GLYPH_NOT_COVERED || coverage_index >= value_count)
                continue;

            opentype_layout_apply_gpos_value(context, subtable_offset, value_format,
                    &format2->values[coverage_index * value_len], iter->pos);
            break;
        }
        else
            WARN("Unknown single adjustment format %u.\n", format);
    }

    return FALSE;
}

static int gpos_pair_adjustment_compare_format1(const void *g, const void *r)
{
    const struct ot_gpos_pairvalue *pairvalue = r;
    UINT16 second_glyph = GET_BE_WORD(pairvalue->second_glyph);
    return *(UINT16 *)g - second_glyph;
}

static BOOL opentype_layout_apply_gpos_pair_adjustment(struct scriptshaping_context *context,
        struct glyph_iterator *iter, const struct lookup *lookup)
{
    struct scriptshaping_cache *cache = context->cache;
    unsigned int i, first_glyph, second_glyph;
    struct glyph_iterator iter_pair;
    WORD format, coverage;

    glyph_iterator_init(context, iter->flags, iter->pos, 1, &iter_pair);
    if (!glyph_iterator_next(&iter_pair))
        return FALSE;

    if (context->is_rtl)
    {
        first_glyph = iter_pair.pos;
        second_glyph = iter->pos;
    }
    else
    {
        first_glyph = iter->pos;
        second_glyph = iter_pair.pos;
    }

    for (i = 0; i < lookup->subtable_count; ++i)
    {
        unsigned int subtable_offset = opentype_layout_get_gpos_subtable(cache, lookup->offset, i);
        WORD value_format1, value_format2, value_len1, value_len2;
        unsigned int coverage_index;

        format = table_read_be_word(&cache->gpos.table, subtable_offset);

        coverage = table_read_be_word(&cache->gpos.table, subtable_offset +
                FIELD_OFFSET(struct ot_gpos_pairpos_format1, coverage));
        if (!coverage)
            continue;

        coverage_index = opentype_layout_is_glyph_covered(&cache->gpos.table, subtable_offset +
                    coverage, context->u.pos.glyphs[first_glyph]);
        if (coverage_index == GLYPH_NOT_COVERED)
            continue;

        if (format == 1)
        {
            const struct ot_gpos_pairpos_format1 *format1;
            WORD pairset_count = table_read_be_word(&cache->gpos.table, subtable_offset +
                    FIELD_OFFSET(struct ot_gpos_pairpos_format1, pairset_count));
            unsigned int pairvalue_len, pairset_offset;
            const struct ot_gpos_pairset *pairset;
            const WORD *pairvalue;
            WORD pairvalue_count;

            if (!pairset_count || coverage_index >= pairset_count)
                continue;

            format1 = table_read_ensure(&cache->gpos.table, subtable_offset,
                    FIELD_OFFSET(struct ot_gpos_pairpos_format1, pairsets[pairset_count]));
            if (!format1)
                continue;

            /* Ordered paired values. */
            pairvalue_count = table_read_be_word(&cache->gpos.table, subtable_offset +
                    GET_BE_WORD(format1->pairsets[coverage_index]));
            if (!pairvalue_count)
                continue;

            /* Structure length is variable, but does not change across the subtable. */
            value_format1 = GET_BE_WORD(format1->value_format1) & 0xff;
            value_format2 = GET_BE_WORD(format1->value_format2) & 0xff;

            value_len1 = dwrite_popcount(value_format1);
            value_len2 = dwrite_popcount(value_format2);
            pairvalue_len = FIELD_OFFSET(struct ot_gpos_pairvalue, data) + value_len1 * sizeof(WORD) +
                    value_len2 * sizeof(WORD);

            pairset_offset = subtable_offset + GET_BE_WORD(format1->pairsets[coverage_index]);
            pairset = table_read_ensure(&cache->gpos.table, subtable_offset + pairset_offset,
                    pairvalue_len * pairvalue_count);
            if (!pairset)
                continue;

            pairvalue = bsearch(&context->u.pos.glyphs[second_glyph], pairset->pairvalues, pairvalue_count,
                    pairvalue_len, gpos_pair_adjustment_compare_format1);
            if (!pairvalue)
                continue;

            pairvalue += 1; /* Skip SecondGlyph. */
            opentype_layout_apply_gpos_value(context, pairset_offset, value_format1, pairvalue, first_glyph);
            opentype_layout_apply_gpos_value(context, pairset_offset, value_format2, pairvalue + value_len1,
                    second_glyph);

            iter->pos = iter_pair.pos;
            if (value_len2)
                iter->pos++;

            return TRUE;
        }
        else if (format == 2)
        {
            const struct ot_gpos_pairpos_format2 *format2;
            WORD class1_count, class2_count;
            unsigned int class1, class2;

            value_format1 = table_read_be_word(&cache->gpos.table, subtable_offset +
                    FIELD_OFFSET(struct ot_gpos_pairpos_format2, value_format1)) & 0xff;
            value_format2 = table_read_be_word(&cache->gpos.table, subtable_offset +
                    FIELD_OFFSET(struct ot_gpos_pairpos_format2, value_format2)) & 0xff;

            class1_count = table_read_be_word(&cache->gpos.table, subtable_offset +
                    FIELD_OFFSET(struct ot_gpos_pairpos_format2, class1_count));
            class2_count = table_read_be_word(&cache->gpos.table, subtable_offset +
                    FIELD_OFFSET(struct ot_gpos_pairpos_format2, class2_count));

            value_len1 = dwrite_popcount(value_format1);
            value_len2 = dwrite_popcount(value_format2);

            format2 = table_read_ensure(&cache->gpos.table, subtable_offset,
                    FIELD_OFFSET(struct ot_gpos_pairpos_format2,
                    values[class1_count * class2_count * (value_len1 + value_len2)]));
            if (!format2)
                continue;

            class1 = opentype_layout_get_glyph_class(&cache->gpos.table, subtable_offset + GET_BE_WORD(format2->class_def1),
                    context->u.pos.glyphs[first_glyph]);
            class2 = opentype_layout_get_glyph_class(&cache->gpos.table, subtable_offset + GET_BE_WORD(format2->class_def2),
                    context->u.pos.glyphs[second_glyph]);

            if (class1 < class1_count && class2 < class2_count)
            {
                const WCHAR *values = &format2->values[(class1 * class2_count + class2) * (value_len1 + value_len2)];
                opentype_layout_apply_gpos_value(context, subtable_offset, value_format1, values, first_glyph);
                opentype_layout_apply_gpos_value(context, subtable_offset, value_format2, values + value_len1,
                        second_glyph);

                iter->pos = iter_pair.pos;
                if (value_len2)
                    iter->pos++;

                return TRUE;
            }
        }
        else
        {
            WARN("Unknown pair adjustment format %u.\n", format);
            continue;
        }
    }

    return FALSE;
}

static void opentype_layout_gpos_get_anchor(const struct scriptshaping_context *context, unsigned int anchor_offset,
        unsigned int glyph_index, float *x, float *y)
{
    const struct scriptshaping_cache *cache = context->cache;

    WORD format = table_read_be_word(&cache->gpos.table, anchor_offset);

    *x = *y = 0.0f;

    if (format == 1)
    {
        const struct ot_gpos_anchor_format1 *format1 = table_read_ensure(&cache->gpos.table, anchor_offset,
                sizeof(*format1));

        if (format1)
        {
            *x = opentype_scale_gpos_be_value(format1->x_coord, context->emsize, cache->upem);
            *y = opentype_scale_gpos_be_value(format1->y_coord, context->emsize, cache->upem);
        }
    }
    else if (format == 2)
    {
        const struct ot_gpos_anchor_format2 *format2 = table_read_ensure(&cache->gpos.table, anchor_offset,
                sizeof(*format2));

        if (format2)
        {
            if (context->measuring_mode != DWRITE_MEASURING_MODE_NATURAL)
                FIXME("Use outline anchor point for glyph %u.\n", context->u.pos.glyphs[glyph_index]);

            *x = opentype_scale_gpos_be_value(format2->x_coord, context->emsize, cache->upem);
            *y = opentype_scale_gpos_be_value(format2->y_coord, context->emsize, cache->upem);
        }
    }
    else if (format == 3)
    {
        const struct ot_gpos_anchor_format3 *format3 = table_read_ensure(&cache->gpos.table, anchor_offset,
                sizeof(*format3));

        if (format3)
        {
            *x = opentype_scale_gpos_be_value(format3->x_coord, context->emsize, cache->upem);
            *y = opentype_scale_gpos_be_value(format3->y_coord, context->emsize, cache->upem);

            if (context->measuring_mode != DWRITE_MEASURING_MODE_NATURAL)
            {
                if (format3->x_dev_offset)
                    *x += opentype_layout_gpos_get_dev_value(context, anchor_offset + GET_BE_WORD(format3->x_dev_offset));
                if (format3->y_dev_offset)
                    *y += opentype_layout_gpos_get_dev_value(context, anchor_offset + GET_BE_WORD(format3->y_dev_offset));
            }
        }
    }
    else
        WARN("Unknown anchor format %u.\n", format);
}

static BOOL opentype_layout_apply_gpos_cursive_attachment(struct scriptshaping_context *context,
        struct glyph_iterator *iter, const struct lookup *lookup)
{
    struct scriptshaping_cache *cache = context->cache;
    unsigned int i;

    for (i = 0; i < lookup->subtable_count; ++i)
    {
        unsigned int subtable_offset = opentype_layout_get_gpos_subtable(cache, lookup->offset, i);
        WORD format;

        format = table_read_be_word(&cache->gpos.table, subtable_offset);

        if (format == 1)
        {
            WORD coverage_offset = table_read_be_word(&cache->gpos.table, subtable_offset +
                    FIELD_OFFSET(struct ot_gpos_cursive_format1, coverage));
            unsigned int glyph_index, entry_count, entry_anchor, exit_anchor;
            float entry_x, entry_y, exit_x, exit_y, delta;
            struct glyph_iterator prev_iter;

            if (!coverage_offset)
                continue;

            entry_count = table_read_be_word(&cache->gpos.table, subtable_offset +
                    FIELD_OFFSET(struct ot_gpos_cursive_format1, count));

            glyph_index = opentype_layout_is_glyph_covered(&cache->gpos.table, subtable_offset +
                    coverage_offset, context->u.pos.glyphs[iter->pos]);
            if (glyph_index == GLYPH_NOT_COVERED || glyph_index >= entry_count)
                continue;

            entry_anchor = table_read_be_word(&cache->gpos.table, subtable_offset +
                    FIELD_OFFSET(struct ot_gpos_cursive_format1, anchors[glyph_index * 2]));
            if (!entry_anchor)
                continue;

            glyph_iterator_init(context, iter->flags, iter->pos, 1, &prev_iter);
            if (!glyph_iterator_prev(&prev_iter))
                continue;

            glyph_index = opentype_layout_is_glyph_covered(&cache->gpos.table, subtable_offset +
                    coverage_offset, context->u.pos.glyphs[prev_iter.pos]);
            if (glyph_index == GLYPH_NOT_COVERED || glyph_index >= entry_count)
                continue;

            exit_anchor = table_read_be_word(&cache->gpos.table, subtable_offset +
                    FIELD_OFFSET(struct ot_gpos_cursive_format1, anchors[glyph_index * 2 + 1]));
            if (!exit_anchor)
                continue;

            opentype_layout_gpos_get_anchor(context, subtable_offset + exit_anchor, prev_iter.pos, &exit_x, &exit_y);
            opentype_layout_gpos_get_anchor(context, subtable_offset + entry_anchor, iter->pos, &entry_x, &entry_y);

            if (context->is_rtl)
            {
                delta = exit_x + context->offsets[prev_iter.pos].advanceOffset;
                context->advances[prev_iter.pos] -= delta;
                context->advances[iter->pos] = entry_x + context->offsets[iter->pos].advanceOffset;
                context->offsets[prev_iter.pos].advanceOffset -= delta;
            }
            else
            {
                delta = entry_x + context->offsets[iter->pos].advanceOffset;
                context->advances[prev_iter.pos] = exit_x + context->offsets[prev_iter.pos].advanceOffset;
                context->advances[iter->pos] -= delta;
                context->offsets[iter->pos].advanceOffset -= delta;
            }

            if (lookup->flags & LOOKUP_FLAG_RTL)
                context->offsets[prev_iter.pos].ascenderOffset = entry_y - exit_y;
            else
                context->offsets[iter->pos].ascenderOffset = exit_y - entry_y;

            break;
        }
        else
            WARN("Unknown cursive attachment format %u.\n", format);

    }

    return FALSE;
}

static BOOL opentype_layout_apply_gpos_mark_to_base_attachment(const struct scriptshaping_context *context,
        struct glyph_iterator *iter, const struct lookup *lookup)
{
    struct scriptshaping_cache *cache = context->cache;
    unsigned int i;
    WORD format;

    for (i = 0; i < lookup->subtable_count; ++i)
    {
        unsigned int subtable_offset = opentype_layout_get_gpos_subtable(cache, lookup->offset, i);

        format = table_read_be_word(&cache->gpos.table, subtable_offset);

        if (format == 1)
        {
            const struct ot_gpos_mark_to_base_format1 *format1 = table_read_ensure(&cache->gpos.table, subtable_offset,
                    sizeof(*format1));
            unsigned int mark_class_count, count, mark_array_offset, base_array_offset;
            const struct ot_gpos_mark_array *mark_array;
            const struct ot_gpos_base_array *base_array;
            float mark_x, mark_y, base_x, base_y;
            unsigned int base_index, mark_index;
            struct glyph_iterator base_iter;
            unsigned int base_anchor;

            if (!format1)
                continue;

            mark_array_offset = subtable_offset + GET_BE_WORD(format1->mark_array);
            if (!(count = table_read_be_word(&cache->gpos.table, mark_array_offset)))
                continue;

            mark_array = table_read_ensure(&cache->gpos.table, mark_array_offset,
                    FIELD_OFFSET(struct ot_gpos_mark_array, records[count]));
            if (!mark_array)
                continue;

            base_array_offset = subtable_offset + GET_BE_WORD(format1->base_array);
            if (!(count = table_read_be_word(&cache->gpos.table, base_array_offset)))
                continue;

            base_array = table_read_ensure(&cache->gpos.table, base_array_offset,
                    FIELD_OFFSET(struct ot_gpos_base_array, offsets[count * GET_BE_WORD(format1->mark_class_count)]));
            if (!base_array)
                continue;

            mark_class_count = GET_BE_WORD(format1->mark_class_count);

            mark_index = opentype_layout_is_glyph_covered(&cache->gpos.table, subtable_offset +
                    GET_BE_WORD(format1->mark_coverage), context->u.pos.glyphs[iter->pos]);

            if (mark_index == GLYPH_NOT_COVERED || mark_index >= GET_BE_WORD(mark_array->count))
                continue;

            /* Look back for first base glyph. */
            glyph_iterator_init(context, LOOKUP_FLAG_IGNORE_MARKS, iter->pos, 1, &base_iter);
            if (!glyph_iterator_prev(&base_iter))
                continue;

            base_index = opentype_layout_is_glyph_covered(&cache->gpos.table, subtable_offset +
                    GET_BE_WORD(format1->base_coverage), context->u.pos.glyphs[base_iter.pos]);
            if (base_index == GLYPH_NOT_COVERED || base_index >= GET_BE_WORD(base_array->count))
                continue;

            base_anchor = GET_BE_WORD(base_array->offsets[base_index * mark_class_count +
                    GET_BE_WORD(mark_array->records[mark_index].mark_class)]);

            opentype_layout_gpos_get_anchor(context, mark_array_offset +
                    GET_BE_WORD(mark_array->records[mark_index].mark_anchor), iter->pos, &mark_x, &mark_y);
            opentype_layout_gpos_get_anchor(context, base_array_offset + base_anchor, base_iter.pos, &base_x, &base_y);

            context->offsets[iter->pos].advanceOffset = (context->is_rtl ? -1.0f : 1.0f) * (base_x - mark_x);
            context->offsets[iter->pos].ascenderOffset = base_y - mark_y;

            break;
        }
        else
            WARN("Unknown mark-to-base format %u.\n", format);
    }

    return FALSE;
}

static BOOL opentype_layout_apply_gpos_mark_to_lig_attachment(const struct scriptshaping_context *context,
        struct glyph_iterator *iter, const struct lookup *lookup)
{
    struct scriptshaping_cache *cache = context->cache;
    unsigned int i;
    WORD format;

    for (i = 0; i < lookup->subtable_count; ++i)
    {
        unsigned int subtable_offset = opentype_layout_get_gpos_subtable(cache, lookup->offset, i);

        format = table_read_be_word(&cache->gpos.table, subtable_offset);

        if (format == 1)
        {
            const struct ot_gpos_mark_to_lig_format1 *format1 = table_read_ensure(&cache->gpos.table,
                    subtable_offset, sizeof(*format1));
            unsigned int mark_index, lig_index;
            struct glyph_iterator lig_iter;

            if (!format1)
                continue;

            mark_index = opentype_layout_is_glyph_covered(&cache->gpos.table, subtable_offset +
                    GET_BE_WORD(format1->mark_coverage), context->u.pos.glyphs[iter->pos]);
            if (mark_index == GLYPH_NOT_COVERED)
                continue;

            glyph_iterator_init(context, LOOKUP_FLAG_IGNORE_MARKS, iter->pos, 1, &lig_iter);
            if (!glyph_iterator_prev(&lig_iter))
                continue;

            lig_index = opentype_layout_is_glyph_covered(&cache->gpos.table, subtable_offset +
                    GET_BE_WORD(format1->lig_coverage), context->u.pos.glyphs[lig_iter.pos]);
            if (lig_index == GLYPH_NOT_COVERED)
                continue;

            FIXME("Unimplemented.\n");
        }
        else
            WARN("Unknown mark-to-ligature format %u.\n", format);
    }

    return FALSE;
}

static BOOL opentype_layout_apply_gpos_mark_to_mark_attachment(const struct scriptshaping_context *context,
        struct glyph_iterator *iter, const struct lookup *lookup)
{
    struct scriptshaping_cache *cache = context->cache;
    unsigned int i;
    WORD format;

    for (i = 0; i < lookup->subtable_count; ++i)
    {
        unsigned int subtable_offset = opentype_layout_get_gpos_subtable(cache, lookup->offset, i);

        format = table_read_be_word(&cache->gpos.table, subtable_offset);

        if (format == 1)
        {
            const struct ot_gpos_mark_to_mark_format1 *format1 = table_read_ensure(&cache->gpos.table,
                    subtable_offset, sizeof(*format1));
            unsigned int count, mark1_array_offset, mark2_array_offset, mark_class_count;
            unsigned int mark1_index, mark2_index, mark2_anchor;
            const struct ot_gpos_mark_array *mark1_array;
            const struct ot_gpos_base_array *mark2_array;
            float mark1_x, mark1_y, mark2_x, mark2_y;
            struct glyph_iterator mark_iter;

            if (!format1)
                continue;

            mark1_index = opentype_layout_is_glyph_covered(&cache->gpos.table, subtable_offset +
                    GET_BE_WORD(format1->mark1_coverage), context->u.pos.glyphs[iter->pos]);

            mark1_array_offset = subtable_offset + GET_BE_WORD(format1->mark1_array);
            if (!(count = table_read_be_word(&cache->gpos.table, mark1_array_offset)))
                continue;

            mark1_array = table_read_ensure(&cache->gpos.table, mark1_array_offset,
                    FIELD_OFFSET(struct ot_gpos_mark_array, records[count]));
            if (!mark1_array)
                continue;

            if (mark1_index == GLYPH_NOT_COVERED || mark1_index >= count)
                continue;

            glyph_iterator_init(context, lookup->flags & ~LOOKUP_FLAG_IGNORE_MASK, iter->pos, 1, &mark_iter);
            if (!glyph_iterator_prev(&mark_iter))
                continue;

            if (!context->u.pos.glyph_props[mark_iter.pos].isDiacritic)
                continue;

            mark2_array_offset = subtable_offset + GET_BE_WORD(format1->mark2_array);
            if (!(count = table_read_be_word(&cache->gpos.table, mark2_array_offset)))
                continue;

            mark_class_count = GET_BE_WORD(format1->mark_class_count);

            mark2_array = table_read_ensure(&cache->gpos.table, mark2_array_offset,
                    FIELD_OFFSET(struct ot_gpos_base_array, offsets[count * mark_class_count]));
            if (!mark2_array)
                continue;

            mark2_index = opentype_layout_is_glyph_covered(&cache->gpos.table, subtable_offset +
                    GET_BE_WORD(format1->mark2_coverage), context->u.pos.glyphs[mark_iter.pos]);

            if (mark2_index == GLYPH_NOT_COVERED || mark2_index >= count)
                continue;

            mark2_anchor = GET_BE_WORD(mark2_array->offsets[mark2_index * mark_class_count +
                    GET_BE_WORD(mark1_array->records[mark1_index].mark_class)]);
            opentype_layout_gpos_get_anchor(context, mark1_array_offset +
                    GET_BE_WORD(mark1_array->records[mark1_index].mark_anchor), iter->pos, &mark1_x, &mark1_y);
            opentype_layout_gpos_get_anchor(context, mark2_array_offset + mark2_anchor, mark_iter.pos,
                    &mark2_x, &mark2_y);

            context->offsets[iter->pos].advanceOffset = mark2_x - mark1_x;
            context->offsets[iter->pos].ascenderOffset = mark2_y - mark1_y;

            break;
        }
        else
            WARN("Unknown mark-to-mark format %u.\n", format);
    }

    return FALSE;
}

static BOOL opentype_layout_apply_gpos_contextual_positioning(const struct scriptshaping_context *context,
        struct glyph_iterator *iter, const struct lookup *lookup)
{
    return FALSE;
}

static BOOL opentype_layout_apply_gpos_chaining_contextual_positioning(const struct scriptshaping_context *context,
        struct glyph_iterator *iter, const struct lookup *lookup)
{
    return FALSE;
}

static void opentype_layout_apply_gpos_lookup(struct scriptshaping_context *context, int lookup_index)
{
    struct scriptshaping_cache *cache = context->cache;
    const struct ot_lookup_table *lookup_table;
    struct glyph_iterator iter;
    struct lookup lookup;
    WORD lookup_type;

    lookup.offset = table_read_be_word(&cache->gpos.table, cache->gpos.lookup_list +
            FIELD_OFFSET(struct ot_lookup_list, lookup[lookup_index]));
    if (!lookup.offset)
        return;

    lookup.offset += cache->gpos.lookup_list;

    if (!(lookup_table = table_read_ensure(&cache->gpos.table, lookup.offset, sizeof(*lookup_table))))
        return;

    lookup.subtable_count = GET_BE_WORD(lookup_table->subtable_count);
    if (!lookup.subtable_count)
        return;

    lookup_type = GET_BE_WORD(lookup_table->lookup_type);
    if (lookup_type == GPOS_LOOKUP_EXTENSION_POSITION)
    {
        const struct ot_gpos_extensionpos_format1 *extension = table_read_ensure(&cache->gpos.table,
                lookup.offset + GET_BE_WORD(lookup_table->subtable[0]), sizeof(*extension));
        WORD format;

        if (!extension)
            return;

        format = GET_BE_WORD(extension->format);
        if (format != 1)
        {
            WARN("Unexpected extension table format %u.\n", format);
            return;
        }

        lookup_type = GET_BE_WORD(extension->lookup_type);
    }
    lookup.flags = GET_BE_WORD(lookup_table->flags);

    glyph_iterator_init(context, lookup.flags, 0, context->glyph_count, &iter);

    while (iter.pos < context->glyph_count)
    {
        BOOL ret;

        if (!glyph_iterator_match(&iter))
        {
            ++iter.pos;
            continue;
        }

        switch (lookup_type)
        {
            case GPOS_LOOKUP_SINGLE_ADJUSTMENT:
                ret = opentype_layout_apply_gpos_single_adjustment(context, &iter, &lookup);
                break;
            case GPOS_LOOKUP_PAIR_ADJUSTMENT:
                ret = opentype_layout_apply_gpos_pair_adjustment(context, &iter, &lookup);
                break;
            case GPOS_LOOKUP_CURSIVE_ATTACHMENT:
                ret = opentype_layout_apply_gpos_cursive_attachment(context, &iter, &lookup);
                break;
            case GPOS_LOOKUP_MARK_TO_BASE_ATTACHMENT:
                ret = opentype_layout_apply_gpos_mark_to_base_attachment(context, &iter, &lookup);
                break;
            case GPOS_LOOKUP_MARK_TO_LIGATURE_ATTACHMENT:
                ret = opentype_layout_apply_gpos_mark_to_lig_attachment(context, &iter, &lookup);
                break;
            case GPOS_LOOKUP_MARK_TO_MARK_ATTACHMENT:
                ret = opentype_layout_apply_gpos_mark_to_mark_attachment(context, &iter, &lookup);
                break;
            case GPOS_LOOKUP_CONTEXTUAL_POSITION:
                ret = opentype_layout_apply_gpos_contextual_positioning(context, &iter, &lookup);
                break;
            case GPOS_LOOKUP_CONTEXTUAL_CHAINING_POSITION:
                ret = opentype_layout_apply_gpos_chaining_contextual_positioning(context, &iter, &lookup);
                break;
            case GPOS_LOOKUP_EXTENSION_POSITION:
                WARN("Recursive extension lookup.\n");
                ret = FALSE;
                break;
            default:
                WARN("Unknown lookup type %u.\n", lookup_type);
                return;
        }

        /* Some lookups update position after making changes. */
        if (!ret)
            ++iter.pos;
    }
}

struct lookups
{
    int *indexes;
    size_t capacity;
    size_t count;
};

static int lookups_sorting_compare(const void *left, const void *right)
{
    return *(int *)left - *(int *)right;
};

void opentype_layout_apply_gpos_features(struct scriptshaping_context *context,
        unsigned int script_index, unsigned int language_index, const struct shaping_features *features)
{
    WORD table_offset, langsys_offset, script_feature_count, total_feature_count, total_lookup_count;
    struct scriptshaping_cache *cache = context->cache;
    const struct ot_feature_list *feature_list;
    struct lookups lookups = { 0 };
    unsigned int i, j, l;

    /* ScriptTable offset. */
    table_offset = table_read_be_word(&cache->gpos.table, cache->gpos.script_list +
            FIELD_OFFSET(struct ot_script_list, scripts) + script_index * sizeof(struct ot_script_record) +
            FIELD_OFFSET(struct ot_script_record, script));
    if (!table_offset)
        return;

    if (language_index == ~0u)
        langsys_offset = table_read_be_word(&cache->gpos.table, cache->gpos.script_list + table_offset);
    else
        langsys_offset = table_read_be_word(&cache->gpos.table, cache->gpos.script_list + table_offset +
                FIELD_OFFSET(struct ot_script, langsys) + language_index * sizeof(struct ot_langsys_record) +
                FIELD_OFFSET(struct ot_langsys_record, langsys));

    script_feature_count = table_read_be_word(&cache->gpos.table, cache->gpos.script_list + table_offset +
            langsys_offset + FIELD_OFFSET(struct ot_langsys, feature_count));
    if (!script_feature_count)
        return;

    total_feature_count = table_read_be_word(&cache->gpos.table, cache->gpos.feature_list);
    if (!total_feature_count)
        return;

    total_lookup_count = table_read_be_word(&cache->gpos.table, cache->gpos.lookup_list);
    if (!total_lookup_count)
        return;

    feature_list = table_read_ensure(&cache->gpos.table, cache->gpos.feature_list,
            FIELD_OFFSET(struct ot_feature_list, features[total_feature_count]));
    if (!feature_list)
        return;

    /* Collect lookups for all given features. */
    for (i = 0; i < features->count; ++i)
    {
        for (j = 0; j < script_feature_count; ++j)
        {
            WORD feature_index = table_read_be_word(&cache->gpos.table, cache->gpos.script_list + table_offset +
                    langsys_offset + FIELD_OFFSET(struct ot_langsys, feature_index[j]));
            if (feature_index >= total_feature_count)
                continue;

            if (feature_list->features[feature_index].tag == features->tags[i])
            {
                WORD feature_offset = GET_BE_WORD(feature_list->features[feature_index].offset);
                WORD lookup_count;

                lookup_count = table_read_be_word(&cache->gpos.table, cache->gpos.feature_list + feature_offset +
                        FIELD_OFFSET(struct ot_feature, lookup_count));
                if (!lookup_count)
                    continue;

                if (!dwrite_array_reserve((void **)&lookups.indexes, &lookups.capacity, lookups.count + lookup_count,
                        sizeof(*lookups.indexes)))
                {
                    heap_free(lookups.indexes);
                    return;
                }

                for (l = 0; l < lookup_count; ++l)
                {
                    WORD lookup_index = table_read_be_word(&cache->gpos.table, cache->gpos.feature_list +
                            feature_offset + FIELD_OFFSET(struct ot_feature, lookuplist_index[l]));

                    if (lookup_index >= total_lookup_count)
                        continue;

                    lookups.indexes[lookups.count++] = lookup_index;
                }
            }
        }
    }

    /* Sort lookups. */
    qsort(lookups.indexes, lookups.count, sizeof(*lookups.indexes), lookups_sorting_compare);

    for (l = 0; l < lookups.count; ++l)
    {
        /* Skip duplicates. */
        if (l && lookups.indexes[l] == lookups.indexes[l - 1])
            continue;

        opentype_layout_apply_gpos_lookup(context, lookups.indexes[l]);
    }

    heap_free(lookups.indexes);
}
