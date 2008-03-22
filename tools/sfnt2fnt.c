/*
 * sfnttofnt.  Bitmap only ttf to Window fnt file converter
 *
 * Copyright 2004 Huw Davies
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_FREETYPE

#ifdef HAVE_FT2BUILD_H
#include <ft2build.h>
#endif
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H
#ifdef HAVE_FREETYPE_INTERNAL_SFNT_H
#include <freetype/internal/sfnt.h>
#endif

#include "wine/unicode.h"
#include "wine/wingdi16.h"
#include "wingdi.h"

#include "pshpack1.h"

typedef struct
{
    WORD dfVersion;
    DWORD dfSize;
    char dfCopyright[60];
    FONTINFO16 fi;
} FNT_HEADER;

typedef struct {
    WORD width;
    DWORD offset;
} CHAR_TABLE_ENTRY;

typedef struct {
    DWORD version;
    ULONG numSizes;
} eblcHeader_t;

typedef struct {
    CHAR ascender;
    CHAR descender;
    BYTE widthMax;
    CHAR caretSlopeNumerator;
    CHAR caretSlopeDenominator;
    CHAR caretOffset;
    CHAR minOriginSB;
    CHAR minAdvanceSB;
    CHAR maxBeforeBL;
    CHAR maxAfterBL;
    CHAR pad1;
    CHAR pad2;
} sbitLineMetrics_t;

typedef struct {
    ULONG indexSubTableArrayOffset;
    ULONG indexTableSize;
    ULONG numberOfIndexSubTables;
    ULONG colorRef;
    sbitLineMetrics_t hori;
    sbitLineMetrics_t vert;
    USHORT startGlyphIndex;
    USHORT endGlyphIndex;
    BYTE ppemX;
    BYTE ppemY;
    BYTE bitDepth;
    CHAR flags;
} bitmapSizeTable_t;

typedef struct
{
    FT_Int major;
    FT_Int minor;
    FT_Int patch;
} FT_Version_t;
static FT_Version_t FT_Version;

#define GET_BE_WORD(ptr)  MAKEWORD( ((BYTE *)(ptr))[1], ((BYTE *)(ptr))[0] )
#define GET_BE_DWORD(ptr) ((DWORD)MAKELONG( GET_BE_WORD(&((WORD *)(ptr))[1]), \
                                            GET_BE_WORD(&((WORD *)(ptr))[0]) ))

#include "poppack.h"

struct fontinfo
{
    FNT_HEADER hdr;
    CHAR_TABLE_ENTRY dfCharTable[258];
    BYTE *data;
};

static const BYTE MZ_hdr[] =
{
    'M',  'Z',  0x0d, 0x01, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
    0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
    0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd, 0x21, 0xb8, 0x01, 0x4c, 0xcd, 0x21, 'T',  'h',
    'i',  's',  ' ',  'P',  'r',  'o',  'g',  'r',  'a',  'm',  ' ',  'c',  'a',  'n',  'n',  'o',
    't',  ' ',  'b',  'e',  ' ',  'r',  'u',  'n',  ' ',  'i',  'n',  ' ',  'D',  'O',  'S',  ' ',
    'm',  'o',  'd',  'e',  0x0d, 0x0a, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const char *output_name;

static FT_Library ft_library;

static void usage(char **argv)
{
    fprintf(stderr, "%s foo.ttf ppem enc dpi def_char avg_width\n", argv[0]);
    return;
}

#ifndef __GNUC__
#define __attribute__(X)
#endif

/* atexit handler to cleanup files */
static void cleanup(void)
{
    if (output_name) unlink( output_name );
}

static void exit_on_signal( int sig )
{
    exit(1);  /* this will call the atexit functions */
}

static void error(const char *s, ...) __attribute__((format (printf, 1, 2)));

static void error(const char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, s, ap);
    va_end(ap);
    exit(1);
}

static const char *get_face_name( const struct fontinfo *info )
{
    return (const char *)info->data + info->hdr.fi.dfFace - info->hdr.fi.dfBitsOffset;
}

static int lookup_charset(int enc)
{
    /* FIXME: make winelib app and use TranslateCharsetInfo */
    switch(enc) {
    case 1250:
        return EE_CHARSET;
    case 1251:
        return RUSSIAN_CHARSET;
    case 1252:
        return ANSI_CHARSET;
    case 1253:
        return GREEK_CHARSET;
    case 1254:
        return TURKISH_CHARSET;
    case 1255:
        return HEBREW_CHARSET;
    case 1256:
        return ARABIC_CHARSET;
    case 1257:
        return BALTIC_CHARSET;
    case 1258:
        return VIETNAMESE_CHARSET;
    case 437:
    case 737:
    case 775:
    case 850:
    case 852:
    case 855:
    case 857:
    case 860:
    case 861:
    case 862:
    case 863:
    case 864:
    case 865:
    case 866:
    case 869:
        return OEM_CHARSET;
    case 874:
        return THAI_CHARSET;
    case 932:
        return SHIFTJIS_CHARSET;
    case 936:
        return GB2312_CHARSET;
    case 949:
        return HANGUL_CHARSET;
    case 950:
        return CHINESEBIG5_CHARSET;
    }
    fprintf(stderr, "Unknown encoding %d - using OEM_CHARSET\n", enc);

    return OEM_CHARSET;
}

static int get_char(const union cptable *cptable, int enc, int index)
{
    /* Korean has the Won sign in place of '\\' */
    if(enc == 949 && index == '\\')
        return 0x20a9;

    return cptable->sbcs.cp2uni[index];
}

/* from gdi32/freetype.c */
static FT_Error load_sfnt_table(FT_Face ft_face, FT_ULong table, FT_Long offset, FT_Byte *buf, FT_ULong *len)
{

    FT_Error err;

    /* If the FT_Load_Sfnt_Table function is there we'll use it */
#ifdef HAVE_FT_LOAD_SFNT_TABLE
    err = FT_Load_Sfnt_Table(ft_face, table, offset, buf, len);
#elif defined(HAVE_FREETYPE_INTERNAL_SFNT_H)
    TT_Face tt_face = (TT_Face) ft_face;
    SFNT_Interface *sfnt;
    if (FT_Version.major==2 && FT_Version.minor==0)
    {
        /* 2.0.x */
        sfnt = *(SFNT_Interface**)((char*)tt_face + 528);
    }
    else
    {
        /* A field was added in the middle of the structure in 2.1.x */
        sfnt = *(SFNT_Interface**)((char*)tt_face + 532);
    }
    err = sfnt->load_any(tt_face, table, offset, buf, len);
#else
    err = FT_Err_Unimplemented_Feature;
#endif
    return err;
}

static struct fontinfo *fill_fontinfo( const char *face_name, int ppem, int enc, int dpi,
                                       unsigned char def_char, int avg_width )
{
    FT_Face face;
    int ascent = 0, il, el, descent = 0, width_bytes = 0, space_size, max_width = 0;
    BYTE left_byte, right_byte, byte;
    DWORD start;
    int i, x, y, x_off, x_end, first_char;
    FT_UInt gi;
    int num_names;
    const union cptable *cptable;
    FT_SfntName sfntname;
    TT_OS2 *os2;
    FT_ULong needed;
    eblcHeader_t *eblc;
    bitmapSizeTable_t *size_table;
    int num_sizes;
    struct fontinfo *info;
    size_t data_pos;

    if (FT_New_Face(ft_library, face_name, 0, &face)) error( "Cannot open face %s\n", face_name );
    if (FT_Set_Pixel_Sizes(face, ppem, ppem)) error( "cannot set face size to %u\n", ppem );

    cptable = wine_cp_get_table(enc);
    if(!cptable)
        error("Can't find codepage %d\n", enc);

    if(cptable->info.char_size != 1) {
        /* for double byte charsets we actually want to use cp1252 */
        cptable = wine_cp_get_table(1252);
        if(!cptable)
            error("Can't find codepage 1252\n");
    }

    assert( face->size->metrics.y_ppem == ppem );

    needed = 0;
    if (load_sfnt_table(face, TTAG_EBLC, 0, NULL, &needed))
        fprintf(stderr,"Can't find EBLC table\n");
    else
    {
        eblc = malloc(needed);
        load_sfnt_table(face, TTAG_EBLC, 0, (FT_Byte *)eblc, &needed);

        num_sizes = GET_BE_DWORD(&eblc->numSizes);

        size_table = (bitmapSizeTable_t *)(eblc + 1);
        for(i = 0; i < num_sizes; i++)
        {
            if(size_table->hori.ascender - size_table->hori.descender == ppem)
            {
                ascent = size_table->hori.ascender;
                descent = -size_table->hori.descender;
                break;
            }
            size_table++;
        }

        free(eblc);
    }

    /* Versions of fontforge prior to early 2006 have incorrect
       ascender values in the eblc table, so we won't find the 
       correct bitmapSizeTable.  In this case use the height of
       the Aring glyph instead. */
    if(ascent == 0) 
    {
        if(FT_Load_Char(face, 0xc5, FT_LOAD_DEFAULT))
            error("Can't find Aring\n");
        ascent = face->glyph->metrics.horiBearingY >> 6;
        descent = ppem - ascent;
    }

    start = sizeof(FNT_HEADER);

    if(FT_Load_Char(face, 'M', FT_LOAD_DEFAULT))
        error("Can't find M\n");
    il = ascent - (face->glyph->metrics.height >> 6);

    /* Hack: Courier has no internal leading, nor do any Chinese or Japanese fonts */
    if(!strcmp(face->family_name, "Courier") || enc == 936 || enc == 950 || enc == 932)
        il = 0;
    /* Japanese system fonts have an external leading (not small font) */
    if (enc == 932 && ppem > 11)
        el = 2;
    else
        el = 0;

    first_char = FT_Get_First_Char(face, &gi);
    if(first_char == 0xd) /* fontforge's first glyph is 0xd, we'll catch this and skip it */
        first_char = 32; /* FT_Get_Next_Char for some reason returns too high
                            number in this case */

    info = calloc( 1, sizeof(*info) );

    info->hdr.fi.dfFirstChar = first_char;
    info->hdr.fi.dfLastChar = 0xff;
    start += ((unsigned char)info->hdr.fi.dfLastChar - (unsigned char)info->hdr.fi.dfFirstChar + 3 ) * sizeof(*info->dfCharTable);

    num_names = FT_Get_Sfnt_Name_Count(face);
    for(i = 0; i <num_names; i++) {
        FT_Get_Sfnt_Name(face, i, &sfntname);
        if(sfntname.platform_id == 1 && sfntname.encoding_id == 0 &&
           sfntname.language_id == 0 && sfntname.name_id == 0) {
            size_t len = min( sfntname.string_len, sizeof(info->hdr.dfCopyright)-1 );
            memcpy(info->hdr.dfCopyright, sfntname.string, len);
            info->hdr.dfCopyright[len] = 0;
        }
    }

    os2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);
    for(i = first_char; i < 0x100; i++) {
        int c = get_char(cptable, enc, i);
        gi = FT_Get_Char_Index(face, c);
        if(gi == 0)
            fprintf(stderr, "warning: %s %u: missing glyph for char %04x\n",
                    face->family_name, ppem, cptable->sbcs.cp2uni[i]);
        if(FT_Load_Char(face, c, FT_LOAD_DEFAULT)) {
            fprintf(stderr, "error loading char %d - bad news!\n", i);
            continue;
        }
        info->dfCharTable[i].width = face->glyph->metrics.horiAdvance >> 6;
        info->dfCharTable[i].offset = start + (width_bytes * ppem);
        width_bytes += ((face->glyph->metrics.horiAdvance >> 6) + 7) >> 3;
        if(max_width < (face->glyph->metrics.horiAdvance >> 6))
            max_width = face->glyph->metrics.horiAdvance >> 6;
    }
    /* space */
    space_size = (ppem + 3) / 4;
    info->dfCharTable[i].width = space_size;
    info->dfCharTable[i].offset = start + (width_bytes * ppem);
    width_bytes += (space_size + 7) >> 3;
    /* sentinel */
    info->dfCharTable[++i].width = 0;
    info->dfCharTable[i].offset = start + (width_bytes * ppem);

    info->hdr.fi.dfType = 0;
    info->hdr.fi.dfPoints = ((ppem - il) * 72 + dpi/2) / dpi;
    info->hdr.fi.dfVertRes = dpi;
    info->hdr.fi.dfHorizRes = dpi;
    info->hdr.fi.dfAscent = ascent;
    info->hdr.fi.dfInternalLeading = il;
    info->hdr.fi.dfExternalLeading = el;
    info->hdr.fi.dfItalic = (face->style_flags & FT_STYLE_FLAG_ITALIC) ? 1 : 0;
    info->hdr.fi.dfUnderline = 0;
    info->hdr.fi.dfStrikeOut = 0;
    info->hdr.fi.dfWeight = os2->usWeightClass;
    info->hdr.fi.dfCharSet = lookup_charset(enc);
    info->hdr.fi.dfPixWidth = (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) ? avg_width : 0;
    info->hdr.fi.dfPixHeight = ppem;
    info->hdr.fi.dfPitchAndFamily = FT_IS_FIXED_WIDTH(face) ? 0 : TMPF_FIXED_PITCH;
    switch(os2->panose[PAN_FAMILYTYPE_INDEX]) {
    case PAN_FAMILY_SCRIPT:
        info->hdr.fi.dfPitchAndFamily |= FF_SCRIPT;
	break;
    case PAN_FAMILY_DECORATIVE:
    case PAN_FAMILY_PICTORIAL:
        info->hdr.fi.dfPitchAndFamily |= FF_DECORATIVE;
	break;
    case PAN_FAMILY_TEXT_DISPLAY:
        if(info->hdr.fi.dfPitchAndFamily == 0) /* fixed */
	    info->hdr.fi.dfPitchAndFamily = FF_MODERN;
	else {
	    switch(os2->panose[PAN_SERIFSTYLE_INDEX]) {
	    case PAN_SERIF_NORMAL_SANS:
	    case PAN_SERIF_OBTUSE_SANS:
	    case PAN_SERIF_PERP_SANS:
	        info->hdr.fi.dfPitchAndFamily |= FF_SWISS;
		break;
	    default:
	        info->hdr.fi.dfPitchAndFamily |= FF_ROMAN;
	    }
	}
	break;
    default:
        info->hdr.fi.dfPitchAndFamily |= FF_DONTCARE;
    }

    info->hdr.fi.dfAvgWidth = avg_width;
    info->hdr.fi.dfMaxWidth = max_width;
    info->hdr.fi.dfDefaultChar = def_char - info->hdr.fi.dfFirstChar;
    info->hdr.fi.dfBreakChar = ' ' - info->hdr.fi.dfFirstChar;
    info->hdr.fi.dfWidthBytes = (width_bytes + 1) & ~1;

    info->hdr.fi.dfFace = start + info->hdr.fi.dfWidthBytes * ppem;
    info->hdr.fi.dfBitsOffset = start;
    info->hdr.fi.dfFlags = 0x10; /* DFF_1COLOR */
    info->hdr.fi.dfFlags |= FT_IS_FIXED_WIDTH(face) ? 1 : 2; /* DFF_FIXED : DFF_PROPORTIONAL */

    info->hdr.dfVersion = 0x300;
    info->hdr.dfSize = start + info->hdr.fi.dfWidthBytes * ppem + strlen(face->family_name) + 1;

    info->data = calloc( info->hdr.dfSize - start, 1 );
    data_pos = 0;

    for(i = first_char; i < 0x100; i++) {
        int c = get_char(cptable, enc, i);
        if(FT_Load_Char(face, c, FT_LOAD_DEFAULT)) {
            continue;
        }
        assert(info->dfCharTable[i].width == face->glyph->metrics.horiAdvance >> 6);

        for(x = 0; x < ((info->dfCharTable[i].width + 7) / 8); x++) {
            for(y = 0; y < ppem; y++) {
                if(y < ascent - face->glyph->bitmap_top ||
                   y >=  face->glyph->bitmap.rows + ascent - face->glyph->bitmap_top) {
                    info->data[data_pos++] = 0;
                    continue;
                }
                x_off = face->glyph->bitmap_left / 8;
                x_end = (face->glyph->bitmap_left + face->glyph->bitmap.width - 1) / 8;
                if(x < x_off || x > x_end) {
                    info->data[data_pos++] = 0;
                    continue;
                }
                if(x == x_off)
                    left_byte = 0;
                else
                    left_byte = face->glyph->bitmap.buffer[(y - (ascent - face->glyph->bitmap_top)) * face->glyph->bitmap.pitch + x - x_off - 1];

                /* On the last non-trival output byte (x == x_end) have we got one or two input bytes */
                if(x == x_end && (face->glyph->bitmap_left % 8 != 0) && ((face->glyph->bitmap.width % 8 == 0) || (x != (((face->glyph->bitmap.width) & ~0x7) + face->glyph->bitmap_left) / 8)))
                    right_byte = 0;
                else
                    right_byte = face->glyph->bitmap.buffer[(y - (ascent - face->glyph->bitmap_top)) * face->glyph->bitmap.pitch + x - x_off];

                byte = (left_byte << (8 - (face->glyph->bitmap_left & 7))) & 0xff;
                byte |= ((right_byte >> (face->glyph->bitmap_left & 7)) & 0xff);
                info->data[data_pos++] = byte;
            }
        }
    }
    data_pos += ((space_size + 7) / 8) * ppem;
    if (width_bytes & 1) data_pos += ppem;

    memcpy( info->data + data_pos, face->family_name, strlen( face->family_name ));
    data_pos += strlen( face->family_name ) + 1;
    assert( start + data_pos == info->hdr.dfSize );

    FT_Done_Face( face );
    return info;
}

static void write_fontinfo( const struct fontinfo *info, FILE *fp )
{
    fwrite( &info->hdr, sizeof(info->hdr), 1, fp );
    fwrite( info->dfCharTable + info->hdr.fi.dfFirstChar, sizeof(*info->dfCharTable),
            ((unsigned char)info->hdr.fi.dfLastChar - (unsigned char)info->hdr.fi.dfFirstChar) + 3, fp );
    fwrite( info->data, info->hdr.dfSize - info->hdr.fi.dfBitsOffset, 1, fp );
}

int main(int argc, char **argv)
{
    int i, j;
    FILE *ofp;
    short align, num_files;
    int resource_table_len, non_resident_name_len, resident_name_len;
    unsigned short resource_table_off, resident_name_off, module_ref_off, non_resident_name_off, fontdir_off, font_off;
    char resident_name[200];
    int fontdir_len = 2;
    char non_resident_name[200];
    unsigned short first_res = 0x0050, pad, res;
    IMAGE_OS2_HEADER NE_hdr;
    NE_TYPEINFO rc_type;
    NE_NAMEINFO rc_name;
    struct fontinfo **info;
    char *p;

    if(argc <= 3) {
        usage(argv);
        exit(1);
    }

    if(FT_Init_FreeType(&ft_library))
        error("ft init failure\n");

    FT_Version.major=FT_Version.minor=FT_Version.patch=-1;
    FT_Library_Version(ft_library,&FT_Version.major,&FT_Version.minor,&FT_Version.patch);

    num_files = argc - 3;
    info = malloc( num_files * sizeof(*info) );
    for (i = 0; i < num_files; i++)
    {
        int ppem, enc, dpi, def_char, avg_width;
        const char *name;

        if (sscanf( argv[i+3], "%d,%d,%d,%d,%d", &ppem, &enc, &dpi, &def_char, &avg_width ) != 5)
        {
            usage(argv);
            exit(1);
        }
        if (!(info[i] = fill_fontinfo( argv[1], ppem, enc, dpi, def_char, avg_width ))) exit(1);

        name = get_face_name( info[i] );
        fontdir_len += 0x74 + strlen(name) + 1;
        if(i == 0) {
            sprintf(non_resident_name, "FONTRES 100,%d,%d : %s %d",
                    info[i]->hdr.fi.dfVertRes, info[i]->hdr.fi.dfHorizRes,
                    name, info[i]->hdr.fi.dfPoints );
            strcpy(resident_name, name);
        } else {
            sprintf(non_resident_name + strlen(non_resident_name), ",%d", info[i]->hdr.fi.dfPoints );
        }
    }

    if(info[0]->hdr.fi.dfVertRes <= 108)
        strcat(non_resident_name, " (VGA res)");
    else
        strcat(non_resident_name, " (8514 res)");
    non_resident_name_len = strlen(non_resident_name) + 4;

    /* shift count + fontdir entry + num_files of font + nul type + \007FONTDIR */
    resource_table_len = sizeof(align) + sizeof("FONTDIR") +
                         sizeof(NE_TYPEINFO) + sizeof(NE_NAMEINFO) +
                         sizeof(NE_TYPEINFO) + sizeof(NE_NAMEINFO) * num_files +
                         sizeof(NE_TYPEINFO);
    resource_table_off = sizeof(NE_hdr);
    resident_name_off = resource_table_off + resource_table_len;
    resident_name_len = strlen(resident_name) + 4;
    module_ref_off = resident_name_off + resident_name_len;
    non_resident_name_off = sizeof(MZ_hdr) + module_ref_off + sizeof(align);

    memset(&NE_hdr, 0, sizeof(NE_hdr));
    NE_hdr.ne_magic = 0x454e;
    NE_hdr.ne_ver = 5;
    NE_hdr.ne_rev = 1;
    NE_hdr.ne_flags = NE_FFLAGS_LIBMODULE | NE_FFLAGS_GUI;
    NE_hdr.ne_cbnrestab = non_resident_name_len;
    NE_hdr.ne_segtab = sizeof(NE_hdr);
    NE_hdr.ne_rsrctab = sizeof(NE_hdr);
    NE_hdr.ne_restab = resident_name_off;
    NE_hdr.ne_modtab = module_ref_off;
    NE_hdr.ne_imptab = module_ref_off;
    NE_hdr.ne_enttab = NE_hdr.ne_modtab;
    NE_hdr.ne_nrestab = non_resident_name_off;
    NE_hdr.ne_align = 4;
    NE_hdr.ne_exetyp = NE_OSFLAGS_WINDOWS;
    NE_hdr.ne_expver = 0x400;

    fontdir_off = (non_resident_name_off + non_resident_name_len + 15) & ~0xf;
    font_off = (fontdir_off + fontdir_len + 15) & ~0x0f;

    atexit( cleanup );
    signal( SIGTERM, exit_on_signal );
    signal( SIGINT, exit_on_signal );
#ifdef SIGHUP
    signal( SIGHUP, exit_on_signal );
#endif

    /* check for .fnt extension on output file (FIXME: should be a cmdline option instead) */
    if ((p = strrchr( argv[2], '.' )) && !strcmp( p, ".fnt" ))
    {
        if (num_files > 1) error( ".fnt generation mode can only contain one font\n" );
        output_name = argv[2];
        if (!(ofp = fopen(output_name, "wb")))
        {
            perror( output_name );
            exit(1);
        }
        write_fontinfo( info[0], ofp );
        fclose( ofp );
        output_name = NULL;
        exit(0);
    }

    output_name = argv[2];
    if (!(ofp = fopen(output_name, "wb")))
    {
        perror( output_name );
        exit(1);
    }

    fwrite(MZ_hdr, sizeof(MZ_hdr), 1, ofp);
    fwrite(&NE_hdr, sizeof(NE_hdr), 1, ofp);

    align = 4;
    fwrite(&align, sizeof(align), 1, ofp);

    rc_type.type_id = NE_RSCTYPE_FONTDIR;
    rc_type.count = 1;
    rc_type.resloader = 0;
    fwrite(&rc_type, sizeof(rc_type), 1, ofp);

    rc_name.offset = fontdir_off >> 4;
    rc_name.length = (fontdir_len + 15) >> 4;
    rc_name.flags = NE_SEGFLAGS_MOVEABLE | NE_SEGFLAGS_PRELOAD;
    rc_name.id = resident_name_off - sizeof("FONTDIR") - NE_hdr.ne_rsrctab;
    rc_name.handle = 0;
    rc_name.usage = 0;
    fwrite(&rc_name, sizeof(rc_name), 1, ofp);

    rc_type.type_id = NE_RSCTYPE_FONT;
    rc_type.count = num_files;
    rc_type.resloader = 0;
    fwrite(&rc_type, sizeof(rc_type), 1, ofp);

    for(res = first_res | 0x8000, i = 0; i < num_files; i++, res++) {
        int len = (info[i]->hdr.dfSize + 15) & ~0xf;

        rc_name.offset = font_off >> 4;
        rc_name.length = len >> 4;
        rc_name.flags = NE_SEGFLAGS_MOVEABLE | NE_SEGFLAGS_SHAREABLE | NE_SEGFLAGS_DISCARDABLE;
        rc_name.id = res;
        rc_name.handle = 0;
        rc_name.usage = 0;
        fwrite(&rc_name, sizeof(rc_name), 1, ofp);

        font_off += len;
    }

    /* empty type info */
    memset(&rc_type, 0, sizeof(rc_type));
    fwrite(&rc_type, sizeof(rc_type), 1, ofp);

    fputc(strlen("FONTDIR"), ofp);
    fwrite("FONTDIR", strlen("FONTDIR"), 1, ofp);
    fputc(strlen(resident_name), ofp);
    fwrite(resident_name, strlen(resident_name), 1, ofp);

    fputc(0x00, ofp);    fputc(0x00, ofp);
    fputc(0x00, ofp);
    fputc(0x00, ofp);    fputc(0x00, ofp);

    fputc(strlen(non_resident_name), ofp);
    fwrite(non_resident_name, strlen(non_resident_name), 1, ofp);
    fputc(0x00, ofp); /* terminator */

    /* empty ne_modtab and ne_imptab */
    fputc(0x00, ofp);
    fputc(0x00, ofp);

    pad = ftell(ofp) & 0xf;
    if(pad != 0)
        pad = 0x10 - pad;
    for(i = 0; i < pad; i++)
        fputc(0x00, ofp);

    /* FONTDIR resource */
    fwrite(&num_files, sizeof(num_files), 1, ofp);

    for(res = first_res, i = 0; i < num_files; i++, res++) {
        const char *name = get_face_name( info[i] );
        fwrite(&res, sizeof(res), 1, ofp);
        fwrite(&info[i]->hdr, FIELD_OFFSET(FNT_HEADER,fi.dfBitsOffset), 1, ofp);
        fputc(0x00, ofp);
        fwrite(name, strlen(name) + 1, 1, ofp);
    }

    pad = ftell(ofp) & 0xf;
    if(pad != 0)
        pad = 0x10 - pad;
    for(i = 0; i < pad; i++)
        fputc(0x00, ofp);

    for(res = first_res, i = 0; i < num_files; i++, res++) {
        write_fontinfo( info[i], ofp );
        pad = info[i]->hdr.dfSize & 0xf;
        if(pad != 0)
            pad = 0x10 - pad;
        for(j = 0; j < pad; j++)
            fputc(0x00, ofp);
    }
    fclose(ofp);
    output_name = NULL;
    exit(0);
}

#else /* HAVE_FREETYPE */

int main(int argc, char **argv)
{
    fprintf( stderr, "%s needs to be built with FreeType support\n", argv[0] );
    exit(1);
}

#endif /* HAVE_FREETYPE */
