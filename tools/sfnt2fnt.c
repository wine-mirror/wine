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

static const char *output_name;

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

static void fill_fontinfo(FT_Face face, int enc, FILE *fp, int dpi, unsigned char def_char, int avg_width)
{
    int ascent = 0, il, el, ppem, descent = 0, width_bytes = 0, space_size, max_width = 0;
    FNT_HEADER hdr;
    FONTINFO16 fi;
    BYTE left_byte, right_byte, byte;
    DWORD start;
    CHAR_TABLE_ENTRY *dfCharTable;
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

    cptable = wine_cp_get_table(enc);
    if(!cptable)
        error("Can't find codepage %d\n", enc);

    if(cptable->info.char_size != 1) {
        /* for double byte charsets we actually want to use cp1252 */
        cptable = wine_cp_get_table(1252);
        if(!cptable)
            error("Can't find codepage 1252\n");
    }

    ppem = face->size->metrics.y_ppem;

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

    start = sizeof(FNT_HEADER) + sizeof(FONTINFO16);

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

    dfCharTable = malloc((255 + 3) * sizeof(*dfCharTable));
    memset(dfCharTable, 0, (255 + 3) * sizeof(*dfCharTable));

    memset(&fi, 0, sizeof(fi));
    fi.dfFirstChar = first_char;
    fi.dfLastChar = 0xff;
    start += ((unsigned char)fi.dfLastChar - (unsigned char)fi.dfFirstChar + 3 ) * sizeof(*dfCharTable);

    num_names = FT_Get_Sfnt_Name_Count(face);
    for(i = 0; i <num_names; i++) {
        FT_Get_Sfnt_Name(face, i, &sfntname);
        if(sfntname.platform_id == 1 && sfntname.encoding_id == 0 &&
           sfntname.language_id == 0 && sfntname.name_id == 0) {
            size_t len = min( sfntname.string_len, sizeof(hdr.dfCopyright)-1 );
            memcpy(hdr.dfCopyright, sfntname.string, len);
            hdr.dfCopyright[len] = 0;
        }
    }

    os2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);
    for(i = first_char; i < 0x100; i++) {
        int c = get_char(cptable, enc, i);
        gi = FT_Get_Char_Index(face, c);
        if(gi == 0)
            fprintf(stderr, "Missing glyph for char %04x\n", cptable->sbcs.cp2uni[i]);
        if(FT_Load_Char(face, c, FT_LOAD_DEFAULT)) {
            fprintf(stderr, "error loading char %d - bad news!\n", i);
            continue;
        }
        dfCharTable[i].width = face->glyph->metrics.horiAdvance >> 6;
        dfCharTable[i].offset = start + (width_bytes * ppem);
        width_bytes += ((face->glyph->metrics.horiAdvance >> 6) + 7) >> 3;
        if(max_width < (face->glyph->metrics.horiAdvance >> 6))
            max_width = face->glyph->metrics.horiAdvance >> 6;
    }
    /* space */
    space_size = (ppem + 3) / 4;
    dfCharTable[i].width = space_size;
    dfCharTable[i].offset = start + (width_bytes * ppem);
    width_bytes += (space_size + 7) >> 3;
    /* sentinel */
    dfCharTable[++i].width = 0;
    dfCharTable[i].offset = start + (width_bytes * ppem);

    fi.dfType = 0;
    fi.dfPoints = ((ppem - il) * 72 + dpi/2) / dpi;
    fi.dfVertRes = dpi;
    fi.dfHorizRes = dpi;
    fi.dfAscent = ascent;
    fi.dfInternalLeading = il;
    fi.dfExternalLeading = el;
    fi.dfItalic = (face->style_flags & FT_STYLE_FLAG_ITALIC) ? 1 : 0;
    fi.dfUnderline = 0;
    fi.dfStrikeOut = 0;
    fi.dfWeight = os2->usWeightClass;
    fi.dfCharSet = lookup_charset(enc);
    fi.dfPixWidth = (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) ?
        avg_width : 0;
    fi.dfPixHeight = ppem;
    fi.dfPitchAndFamily = FT_IS_FIXED_WIDTH(face) ? 0 : TMPF_FIXED_PITCH;
    switch(os2->panose[PAN_FAMILYTYPE_INDEX]) {
    case PAN_FAMILY_SCRIPT:
        fi.dfPitchAndFamily |= FF_SCRIPT;
	break;
    case PAN_FAMILY_DECORATIVE:
    case PAN_FAMILY_PICTORIAL:
        fi.dfPitchAndFamily |= FF_DECORATIVE;
	break;
    case PAN_FAMILY_TEXT_DISPLAY:
        if(fi.dfPitchAndFamily == 0) /* fixed */
	    fi.dfPitchAndFamily = FF_MODERN;
	else {
	    switch(os2->panose[PAN_SERIFSTYLE_INDEX]) {
	    case PAN_SERIF_NORMAL_SANS:
	    case PAN_SERIF_OBTUSE_SANS:
	    case PAN_SERIF_PERP_SANS:
	        fi.dfPitchAndFamily |= FF_SWISS;
		break;
	    default:
	        fi.dfPitchAndFamily |= FF_ROMAN;
	    }
	}
	break;
    default:
        fi.dfPitchAndFamily |= FF_DONTCARE;
    }

    fi.dfAvgWidth = avg_width;
    fi.dfMaxWidth = max_width;
    fi.dfDefaultChar = def_char - fi.dfFirstChar;
    fi.dfBreakChar = ' ' - fi.dfFirstChar;
    fi.dfWidthBytes = (width_bytes + 1) & ~1;

    fi.dfFace = start + fi.dfWidthBytes * ppem;
    fi.dfBitsOffset = start;
    fi.dfFlags = 0x10; /* DFF_1COLOR */
    fi.dfFlags |= FT_IS_FIXED_WIDTH(face) ? 1 : 2; /* DFF_FIXED : DFF_PROPORTIONAL */

    hdr.dfVersion = 0x300;
    hdr.dfSize = start + fi.dfWidthBytes * ppem + strlen(face->family_name) + 1;
    fwrite(&hdr, sizeof(hdr), 1, fp);
    fwrite(&fi, sizeof(fi), 1, fp);
    fwrite(dfCharTable + fi.dfFirstChar, sizeof(*dfCharTable), ((unsigned char)fi.dfLastChar - (unsigned char)fi.dfFirstChar) + 3, fp);

    for(i = first_char; i < 0x100; i++) {
        int c = get_char(cptable, enc, i);
        if(FT_Load_Char(face, c, FT_LOAD_DEFAULT)) {
            continue;
        }
        assert(dfCharTable[i].width == face->glyph->metrics.horiAdvance >> 6);

        for(x = 0; x < ((dfCharTable[i].width + 7) / 8); x++) {
            for(y = 0; y < ppem; y++) {
                if(y < ascent - face->glyph->bitmap_top ||
                   y >=  face->glyph->bitmap.rows + ascent - face->glyph->bitmap_top) {
                    fputc('\0', fp);
                    continue;
                }
                x_off = face->glyph->bitmap_left / 8;
                x_end = (face->glyph->bitmap_left + face->glyph->bitmap.width - 1) / 8;
                if(x < x_off || x > x_end) {
                    fputc('\0', fp);
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
                fputc(byte, fp);
            }
        }
    }
    for(x = 0; x < (space_size + 7) / 8; x++) {
        for(y = 0; y < ppem; y++)
            fputc('\0', fp);
    }

    if(width_bytes & 1) {
        for(y = 0; y < ppem; y++)
            fputc('\0', fp);
    }
    fprintf(fp, "%s", face->family_name);
    fputc('\0', fp);

}


int main(int argc, char **argv)
{
    int ppem, enc;
    FT_Face face;
    FT_Library lib;
    int dpi, avg_width;
    unsigned int def_char;
    FILE *fp;
    char output[256];
    char name[256];
    char *cp;
    if(argc != 7) {
        usage(argv);
        exit(0);
    }

    ppem = atoi(argv[2]);
    enc = atoi(argv[3]);
    dpi = atoi(argv[4]);
    def_char = atoi(argv[5]);
    avg_width = atoi(argv[6]);

    if(FT_Init_FreeType(&lib))
        error("ft init failure\n");

    FT_Version.major=FT_Version.minor=FT_Version.patch=-1;
    FT_Library_Version(lib,&FT_Version.major,&FT_Version.minor,&FT_Version.patch);

    if(FT_New_Face(lib, argv[1], 0, &face)) {
        fprintf(stderr, "Can't open face\n");
        usage(argv);
        exit(1);
    }

    if(FT_Set_Pixel_Sizes(face, ppem, ppem)) {
        fprintf(stderr, "Can't set size\n");
        usage(argv);
        exit(1);
    }

    strcpy(name, face->family_name);
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

    fp = fopen(output, "w");
    output_name = output;

    fill_fontinfo(face, enc, fp, dpi, def_char, avg_width);
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
