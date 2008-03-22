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
    int ppem, enc;
    int dpi, avg_width;
    unsigned int def_char;
    FILE *fp;
    char output[256];
    char name[256];
    char *cp;
    struct fontinfo *info;

    if(argc != 7) {
        usage(argv);
        exit(0);
    }

    ppem = atoi(argv[2]);
    enc = atoi(argv[3]);
    dpi = atoi(argv[4]);
    def_char = atoi(argv[5]);
    avg_width = atoi(argv[6]);

    if(FT_Init_FreeType(&ft_library))
        error("ft init failure\n");

    FT_Version.major=FT_Version.minor=FT_Version.patch=-1;
    FT_Library_Version(ft_library,&FT_Version.major,&FT_Version.minor,&FT_Version.patch);

    if (!(info = fill_fontinfo( argv[1], ppem, enc, dpi, def_char, avg_width ))) exit(1);

    strcpy(name, get_face_name(info));
    /* FIXME: should add a -o option instead */
    for(cp = name; *cp; cp++)
    {
        if(*cp == ' ') *cp = '_';
        else if (*cp >= 'A' && *cp <= 'Z') *cp += 'a' - 'A';
    }

    sprintf(output, "%s-%d-%d-%d.fnt", name, enc, dpi, ppem);

    atexit( cleanup );
    signal( SIGTERM, exit_on_signal );
    signal( SIGINT, exit_on_signal );
#ifdef SIGHUP
    signal( SIGHUP, exit_on_signal );
#endif

    if (!(fp = fopen(output, "w")))
    {
        perror( output );
        exit(1);
    }
    output_name = output;
    write_fontinfo( info, fp );
    fclose(fp);
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
