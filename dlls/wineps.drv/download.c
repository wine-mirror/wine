/*
 *	PostScript driver downloadable font functions
 *
 *	Copyright 2002  Huw D M Davies for CodeWeavers
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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"

#include "psdrv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

#define MS_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (DWORD)_x4 << 24 ) |     \
            ( (DWORD)_x3 << 16 ) |     \
            ( (DWORD)_x2 <<  8 ) |     \
              (DWORD)_x1         )

#define GET_BE_WORD(ptr)  MAKEWORD( ((BYTE *)(ptr))[1], ((BYTE *)(ptr))[0] )
#define GET_BE_DWORD(ptr) ((DWORD)MAKELONG( GET_BE_WORD(&((WORD *)(ptr))[1]), \
                                            GET_BE_WORD(&((WORD *)(ptr))[0]) ))

/****************************************************************************
 *  get_download_name
 */
static void get_download_name(print_ctx *ctx, LPOUTLINETEXTMETRICA potm, char **str, BOOL vertical)
{
    static const char reserved_chars[] = " %/(){}[]<>\n\r\t\b\f";
    static const char vertical_suffix[] = "_vert";
    int len;
    char *p;
    DWORD size;

    size = GetFontData(ctx->hdc, MS_MAKE_TAG('n','a','m','e'), 0, NULL, 0);
    if(size != 0 && size != GDI_ERROR)
    {
        BYTE *name = HeapAlloc(GetProcessHeap(), 0, size);
        if(name)
        {
            USHORT count, i;
            BYTE *strings;
            struct name_record
            {
                USHORT platform_id;
                USHORT encoding_id;
                USHORT language_id;
                USHORT name_id;
                USHORT length;
                USHORT offset;
            } *name_record;

            GetFontData(ctx->hdc, MS_MAKE_TAG('n','a','m','e'), 0, name, size);
            count = GET_BE_WORD(name + 2);
            strings = name + GET_BE_WORD(name + 4);
            name_record = (struct name_record *)(name + 6);
            for(i = 0; i < count; i++, name_record++)
            {
                name_record->platform_id = GET_BE_WORD(&name_record->platform_id);
                name_record->encoding_id = GET_BE_WORD(&name_record->encoding_id);
                name_record->language_id = GET_BE_WORD(&name_record->language_id);
                name_record->name_id     = GET_BE_WORD(&name_record->name_id);
                name_record->length      = GET_BE_WORD(&name_record->length);
                name_record->offset      = GET_BE_WORD(&name_record->offset);

                if(name_record->platform_id == 1 && name_record->encoding_id == 0 &&
                   name_record->language_id == 0 && name_record->name_id == 6)
                {
                    TRACE("Got Mac PS name %s\n", debugstr_an((char*)strings + name_record->offset, name_record->length));
                    *str = HeapAlloc(GetProcessHeap(), 0, name_record->length + sizeof(vertical_suffix));
                    memcpy(*str, strings + name_record->offset, name_record->length);
                    *(*str + name_record->length) = '\0';
                    HeapFree(GetProcessHeap(), 0, name);
                    goto done;
                }
                if(name_record->platform_id == 3 && name_record->encoding_id == 1 &&
                   name_record->language_id == 0x409 && name_record->name_id == 6)
                {
                    WCHAR *unicode = HeapAlloc(GetProcessHeap(), 0, name_record->length + 2);
                    DWORD len;
                    int c;

                    for(c = 0; c < name_record->length / 2; c++)
                        unicode[c] = GET_BE_WORD(strings + name_record->offset + c * 2);
                    unicode[c] = 0;
                    TRACE("Got Windows PS name %s\n", debugstr_w(unicode));
                    len = WideCharToMultiByte(1252, 0, unicode, -1, NULL, 0, NULL, NULL);
                    *str = HeapAlloc(GetProcessHeap(), 0, len + sizeof(vertical_suffix) - 1);
                    WideCharToMultiByte(1252, 0, unicode, -1, *str, len, NULL, NULL);
                    HeapFree(GetProcessHeap(), 0, unicode);
                    HeapFree(GetProcessHeap(), 0, name);
                    goto done;
                }
            }
            TRACE("Unable to find PostScript name\n");
            HeapFree(GetProcessHeap(), 0, name);
        }
    }

    len = strlen((char*)potm + (ptrdiff_t)potm->otmpFaceName) + 1;
    *str = HeapAlloc(GetProcessHeap(),0,len + sizeof(vertical_suffix) - 1);
    strcpy(*str, (char*)potm + (ptrdiff_t)potm->otmpFaceName);

done:
    for (p = *str; *p; p++) if (strchr( reserved_chars, *p )) *p = '_';
    if (vertical)
        strcat(*str,vertical_suffix);
}

/****************************************************************************
 *  is_font_downloaded
 */
static DOWNLOAD *is_font_downloaded(print_ctx *ctx, char *ps_name)
{
    DOWNLOAD *pdl;

    for(pdl = ctx->downloaded_fonts; pdl; pdl = pdl->next)
        if(!strcmp(pdl->ps_name, ps_name))
	    break;
    return pdl;
}

/****************************************************************************
 *  is_room_for_font
 */
static BOOL is_room_for_font(print_ctx *ctx)
{
    DOWNLOAD *pdl;
    int count = 0;

    /* FIXME: should consider vm usage of each font and available printer memory.
       For now we allow up to two fonts to be downloaded at a time */
    for(pdl = ctx->downloaded_fonts; pdl; pdl = pdl->next)
        count++;

    if(count > 1)
        return FALSE;
    return TRUE;
}

/****************************************************************************
 *  get_bbox
 *
 * This retrieves the bounding box of the font in font units as well as
 * the size of the emsquare.  To avoid having to worry about mapping mode and
 * the font size we'll get the data directly from the TrueType HEAD table rather
 * than using GetOutlineTextMetrics.
 */
static UINT get_bbox(HDC hdc, RECT *rc)
{
    BYTE head[54]; /* the head table is 54 bytes long */

    if(GetFontData(hdc, MS_MAKE_TAG('h','e','a','d'), 0, head, sizeof(head)) == GDI_ERROR)
    {
        ERR("Can't retrieve head table\n");
        return 0;
    }
    if(rc)
    {
        rc->left   = (signed short)GET_BE_WORD(head + 36); /* xMin */
        rc->bottom = (signed short)GET_BE_WORD(head + 38); /* yMin */
        rc->right  = (signed short)GET_BE_WORD(head + 40); /* xMax */
        rc->top    = (signed short)GET_BE_WORD(head + 42); /* yMax */
    }
    return GET_BE_WORD(head + 18); /* unitsPerEm */
}

static UINT calc_ppem_for_height(HDC hdc, LONG height)
{
    BYTE os2[78]; /* size of version 0 table */
    BYTE hhea[8]; /* just enough to get the ascender and descender */
    LONG ascent = 0, descent = 0;
    UINT emsize;

    if(height < 0) return -height;

    if(GetFontData(hdc, MS_MAKE_TAG('O','S','/','2'), 0, os2, sizeof(os2)) == sizeof(os2))
    {
        ascent  = GET_BE_WORD(os2 + 74); /* usWinAscent */
        descent = GET_BE_WORD(os2 + 76); /* usWinDescent */
    }

    if(ascent + descent == 0)
    {
        if(GetFontData(hdc, MS_MAKE_TAG('h','h','e','a'), 0, hhea, sizeof(hhea)) == sizeof(hhea))
        {
            ascent  =  (signed short)GET_BE_WORD(hhea + 4); /* Ascender */
            descent = -(signed short)GET_BE_WORD(hhea + 6); /* Descender */
        }
    }

    if(ascent + descent == 0) return height;

    emsize = get_bbox(hdc, NULL);

    return MulDiv(emsize, height, ascent + descent);
}

static inline float ps_round(float f)
{
    return (f > 0) ? (f + 0.5) : (f - 0.5);
}

static BOOL is_fake_italic( HDC hdc )
{
    TEXTMETRICW tm;
    BYTE head[54]; /* the head table is 54 bytes long */
    WORD mac_style;

    GetTextMetricsW( hdc, &tm );
    if (!tm.tmItalic) return FALSE;

    if (GetFontData( hdc, MS_MAKE_TAG('h','e','a','d'), 0, head, sizeof(head) ) == GDI_ERROR)
        return FALSE;

    mac_style = GET_BE_WORD( head + 44 );
    TRACE( "mac style %04x\n", mac_style );
    return !(mac_style & 2);
}

/****************************************************************************
 *  PSDRV_WriteSetDownloadFont
 *
 *  Write setfont for download font.
 *
 */
BOOL PSDRV_WriteSetDownloadFont(print_ctx *ctx, BOOL vertical)
{
    char *ps_name;
    LPOUTLINETEXTMETRICA potm;
    DWORD len = GetOutlineTextMetricsA(ctx->hdc, 0, NULL);
    DOWNLOAD *pdl;
    LOGFONTW lf;
    UINT ppem;
    XFORM xform;

    assert(ctx->font.fontloc == Download);

    if (!GetObjectW( GetCurrentObject(ctx->hdc, OBJ_FONT), sizeof(lf), &lf ))
        return FALSE;

    potm = HeapAlloc(GetProcessHeap(), 0, len);
    if (!potm)
        return FALSE;

    GetOutlineTextMetricsA(ctx->hdc, len, potm);

    get_download_name(ctx, potm, &ps_name, vertical);
    ctx->font.fontinfo.Download = is_font_downloaded(ctx, ps_name);

    ppem = calc_ppem_for_height(ctx->hdc, lf.lfHeight);

    /* Retrieve the world -> device transform */
    GetTransform(ctx->hdc, 0x204, &xform);

    if(GetGraphicsMode(ctx->hdc) == GM_COMPATIBLE)
    {
        if (xform.eM22 < 0) lf.lfEscapement = -lf.lfEscapement;
        xform.eM11 = xform.eM22 = fabs(xform.eM22);
        xform.eM21 = xform.eM12 = 0;
    }

    ctx->font.size.xx = ps_round(ppem * xform.eM11);
    ctx->font.size.xy = ps_round(ppem * xform.eM12);
    ctx->font.size.yx = -ps_round(ppem * xform.eM21);
    ctx->font.size.yy = -ps_round(ppem * xform.eM22);

    if(ctx->font.fontinfo.Download == NULL) {
        RECT bbox;
        UINT emsize = get_bbox(ctx->hdc, &bbox);

        if (!emsize) {
            HeapFree(GetProcessHeap(), 0, ps_name);
            HeapFree(GetProcessHeap(), 0, potm);
            return FALSE;
        }
        if(!is_room_for_font(ctx))
            PSDRV_EmptyDownloadList(ctx, TRUE);

        pdl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pdl));
	pdl->ps_name = HeapAlloc(GetProcessHeap(), 0, strlen(ps_name)+1);
	strcpy(pdl->ps_name, ps_name);
	pdl->next = NULL;

        if(ctx->pi->ppd->TTRasterizer == RO_Type42) {
	    pdl->typeinfo.Type42 = T42_download_header(ctx, ps_name, &bbox, emsize);
	    pdl->type = Type42;
	}
	if(pdl->typeinfo.Type42 == NULL) {
	    pdl->typeinfo.Type1 = T1_download_header(ctx, ps_name, &bbox, emsize);
	    pdl->type = Type1;
	}
	pdl->next = ctx->downloaded_fonts;
	ctx->downloaded_fonts = pdl;
	ctx->font.fontinfo.Download = pdl;

        if(pdl->type == Type42) {
            char g_name[MAX_G_NAME + 1];
            get_glyph_name(ctx->hdc, 0, g_name);
            T42_download_glyph(ctx, pdl, 0, g_name);
        }
    }

    if (vertical)
        lf.lfEscapement += 900;

    PSDRV_WriteSetFont(ctx, ps_name, ctx->font.size, lf.lfEscapement,
                        is_fake_italic( ctx->hdc ));

    HeapFree(GetProcessHeap(), 0, ps_name);
    HeapFree(GetProcessHeap(), 0, potm);
    return TRUE;
}

static void get_standard_glyph_name(WORD index, char *name)
{
    /* These PostScript Format 1 glyph names are stored by glyph index, do not reorder them. */
    static const char * const glyph_table[] = {
        ".notdef",
        ".null",
        "nonmarkingreturn",
        "space",
        "exclam",
        "quotedbl",
        "numbersign",
        "dollar",
        "percent",
        "ampersand",
        "quotesingle",
        "parenleft",
        "parenright",
        "asterisk",
        "plus",
        "comma",
        "hyphen",
        "period",
        "slash",
        "zero",
        "one",
        "two",
        "three",
        "four",
        "five",
        "six",
        "seven",
        "eight",
        "nine",
        "colon",
        "semicolon",
        "less",
        "equal",
        "greater",
        "question",
        "at",
        "A",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G",
        "H",
        "I",
        "J",
        "K",
        "L",
        "M",
        "N",
        "O",
        "P",
        "Q",
        "R",
        "S",
        "T",
        "U",
        "V",
        "W",
        "X",
        "Y",
        "Z",
        "bracketleft",
        "backslash",
        "bracketright",
        "asciicircum",
        "underscore",
        "grave",
        "a",
        "b",
        "c",
        "d",
        "e",
        "f",
        "g",
        "h",
        "i",
        "j",
        "k",
        "l",
        "m",
        "n",
        "o",
        "p",
        "q",
        "r",
        "s",
        "t",
        "u",
        "v",
        "w",
        "x",
        "y",
        "z",
        "braceleft",
        "bar",
        "braceright",
        "asciitilde",
        "Adieresis",
        "Aring",
        "Ccedilla",
        "Eacute",
        "Ntilde",
        "Odieresis",
        "Udieresis",
        "aacute",
        "agrave",
        "acircumflex",
        "adieresis",
        "atilde",
        "aring",
        "ccedilla",
        "eacute",
        "egrave",
        "ecircumflex",
        "edieresis",
        "iacute",
        "igrave",
        "icircumflex",
        "idieresis",
        "ntilde",
        "oacute",
        "ograve",
        "ocircumflex",
        "odieresis",
        "otilde",
        "uacute",
        "ugrave",
        "ucircumflex",
        "udieresis",
        "dagger",
        "degree",
        "cent",
        "sterling",
        "section",
        "bullet",
        "paragraph",
        "germandbls",
        "registered",
        "copyright",
        "trademark",
        "acute",
        "dieresis",
        "notequal",
        "AE",
        "Oslash",
        "infinity",
        "plusminus",
        "lessequal",
        "greaterequal",
        "yen",
        "mu",
        "partialdiff",
        "summation",
        "product",
        "pi",
        "integral",
        "ordfeminine",
        "ordmasculine",
        "Omega",
        "ae",
        "oslash",
        "questiondown",
        "exclamdown",
        "logicalnot",
        "radical",
        "florin",
        "approxequal",
        "Delta",
        "guillemotleft",
        "guillemotright",
        "ellipsis",
        "nonbreakingspace",
        "Agrave",
        "Atilde",
        "Otilde",
        "OE",
        "oe",
        "endash",
        "emdash",
        "quotedblleft",
        "quotedblright",
        "quoteleft",
        "quoteright",
        "divide",
        "lozenge",
        "ydieresis",
        "Ydieresis",
        "fraction",
        "currency",
        "guilsinglleft",
        "guilsinglright",
        "fi",
        "fl",
        "daggerdbl",
        "periodcentered",
        "quotesinglbase",
        "quotedblbase",
        "perthousand",
        "Acircumflex",
        "Ecircumflex",
        "Aacute",
        "Edieresis",
        "Egrave",
        "Iacute",
        "Icircumflex",
        "Idieresis",
        "Igrave",
        "Oacute",
        "Ocircumflex",
        "apple",
        "Ograve",
        "Uacute",
        "Ucircumflex",
        "Ugrave",
        "dotlessi",
        "circumflex",
        "tilde",
        "macron",
        "breve",
        "dotaccent",
        "ring",
        "cedilla",
        "hungarumlaut",
        "ogonek",
        "caron",
        "Lslash",
        "lslash",
        "Scaron",
        "scaron",
        "Zcaron",
        "zcaron",
        "brokenbar",
        "Eth",
        "eth",
        "Yacute",
        "yacute",
        "Thorn",
        "thorn",
        "minus",
        "multiply",
        "onesuperior",
        "twosuperior",
        "threesuperior",
        "onehalf",
        "onequarter",
        "threequarters",
        "franc",
        "Gbreve",
        "gbreve",
        "Idotaccent",
        "Scedilla",
        "scedilla",
        "Cacute",
        "cacute",
        "Ccaron",
        "ccaron",
        "dcroat",
    };
    snprintf(name, MAX_G_NAME + 1, "%s", glyph_table[index]);
}

static int get_post2_name_index(BYTE *post2header, DWORD size, WORD index)
{
    USHORT numberOfGlyphs = GET_BE_WORD(post2header);
    DWORD offset = (1 + index) * sizeof(USHORT);

    if(offset + sizeof(USHORT) > size || index >= numberOfGlyphs)
    {
        FIXME("Index '%d' exceeds PostScript Format 2 table size (%d)\n", index, numberOfGlyphs);
        return -1;
    }
    return GET_BE_WORD(post2header + offset);
}

static void get_post2_custom_glyph_name(BYTE *post2header, DWORD size, WORD index, char *name)
{
    USHORT numberOfGlyphs = GET_BE_WORD(post2header);
    int i, name_offset = (1 + numberOfGlyphs) * sizeof(USHORT);
    BYTE name_length = 0;

    for(i = 0; i <= index; i++)
    {
        name_offset += name_length;
        if(name_offset + sizeof(BYTE) > size)
        {
            FIXME("Pascal name offset '%d' exceeds PostScript Format 2 table size (%ld)\n",
                  name_offset + 1, size);
            return;
        }
        name_length = (post2header + name_offset)[0];
        if(name_offset + name_length > size)
        {
            FIXME("Pascal name offset '%d' exceeds PostScript Format 2 table size (%ld)\n",
                  name_offset + name_length, size);
            return;
        }
        name_offset += sizeof(BYTE);
    }
    name_length = min(name_length, MAX_G_NAME);
    memcpy(name, post2header + name_offset, name_length);
    name[name_length] = 0;
}

void get_glyph_name(HDC hdc, WORD index, char *name)
{
    struct
    {
        DWORD format;
        DWORD italicAngle;
        SHORT underlinePosition;
        SHORT underlineThickness;
        DWORD isFixedPitch;
        DWORD minMemType42;
        DWORD maxMemType42;
        DWORD minMemType1;
        DWORD maxMemType1;
    } *post_header;
    DWORD size;

    /* set a fallback name that is just 'g<index>' */
    snprintf(name, MAX_G_NAME + 1, "g%04x", index);

    /* attempt to obtain the glyph name from the 'post' table */
    size = GetFontData(hdc, MS_MAKE_TAG('p','o','s','t'), 0, NULL, 0);
    if(size < sizeof(*post_header) || size == GDI_ERROR)
        return;
    post_header = HeapAlloc(GetProcessHeap(), 0, size);
    if(!post_header) return;
    size = GetFontData(hdc, MS_MAKE_TAG('p','o','s','t'), 0, post_header, size);
    if(size < sizeof(*post_header) || size == GDI_ERROR)
        goto cleanup;
    /* note: only interested in the format for obtaining glyph names */
    post_header->format = GET_BE_DWORD(&post_header->format);

    /* now that we know the format of the 'post' table we can get the glyph name */
    if(post_header->format == MAKELONG(0, 1))
    {
        if(index < 258)
            get_standard_glyph_name(index, name);
        else
            WARN("Font uses PostScript Format 1, but non-standard glyph (%d) requested.\n", index);
    }
    else if(post_header->format == MAKELONG(0, 2))
    {
        void *post2header = post_header + 1;
        int glyphNameIndex;

        size -= sizeof(*post_header);
        if(size < sizeof(USHORT))
        {
            FIXME("PostScript Format 2 table is invalid (cannot fit header)\n");
            goto cleanup;
        }
        glyphNameIndex = get_post2_name_index(post2header, size, index);
        if(glyphNameIndex == -1)
            goto cleanup; /* invalid index, use fallback name */
        else if(glyphNameIndex < 258)
            get_standard_glyph_name(glyphNameIndex, name);
        else
            get_post2_custom_glyph_name(post2header, size, glyphNameIndex - 258, name);
    }
    else
        FIXME("PostScript Format %d.%d glyph names are currently unsupported.\n",
              HIWORD(post_header->format), LOWORD(post_header->format));

cleanup:
    HeapFree(GetProcessHeap(), 0, post_header);
}

/****************************************************************************
 *  PSDRV_WriteDownloadGlyphShow
 *
 *  Download and write out a number of glyphs
 *
 */
BOOL PSDRV_WriteDownloadGlyphShow(print_ctx *ctx, const WORD *glyphs,
				  UINT count)
{
    UINT i;
    char g_name[MAX_G_NAME + 1];
    assert(ctx->font.fontloc == Download);

    switch(ctx->font.fontinfo.Download->type) {
    case Type42:
    for(i = 0; i < count; i++) {
        get_glyph_name(ctx->hdc, glyphs[i], g_name);
	T42_download_glyph(ctx, ctx->font.fontinfo.Download, glyphs[i], g_name);
	PSDRV_WriteGlyphShow(ctx, g_name);
    }
    break;

    case Type1:
    for(i = 0; i < count; i++) {
        get_glyph_name(ctx->hdc, glyphs[i], g_name);
	T1_download_glyph(ctx, ctx->font.fontinfo.Download, glyphs[i], g_name);
	PSDRV_WriteGlyphShow(ctx, g_name);
    }
    break;

    default:
        ERR("Type = %d\n", ctx->font.fontinfo.Download->type);
	assert(0);
    }
    return TRUE;
}

/****************************************************************************
 *  PSDRV_EmptyDownloadList
 *
 *  Clear the list of downloaded fonts
 *
 */
BOOL PSDRV_EmptyDownloadList(print_ctx *ctx, BOOL write_undef)
{
    DOWNLOAD *pdl, *old;
    static const char undef[] = "/%s findfont 40 scalefont setfont /%s undefinefont\n";
    char buf[sizeof(undef) + 200];
    const char *default_font = ctx->pi->ppd->DefaultFont ?
        ctx->pi->ppd->DefaultFont : "Courier";

    if(ctx->font.fontloc == Download) {
        ctx->font.set = FALSE;
	ctx->font.fontinfo.Download = NULL;
    }

    pdl = ctx->downloaded_fonts;
    ctx->downloaded_fonts = NULL;
    while(pdl) {
        if(write_undef) {
            sprintf(buf, undef, default_font, pdl->ps_name);
            PSDRV_WriteSpool(ctx, buf, strlen(buf));
        }

        switch(pdl->type) {
	case Type42:
	    T42_free(pdl->typeinfo.Type42);
	    break;

	case Type1:
	    T1_free(pdl->typeinfo.Type1);
	    break;

	default:
	    ERR("Type = %d\n", pdl->type);
	    assert(0);
	}

	HeapFree(GetProcessHeap(), 0, pdl->ps_name);
	old = pdl;
	pdl = pdl->next;
	HeapFree(GetProcessHeap(), 0, old);
    }
    return TRUE;
}
