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
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

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
static void get_download_name(PSDRV_PDEVICE *physDev, LPOUTLINETEXTMETRICA
			      potm, char **str)
{
    int len;
    char *p;
    DWORD size;

    size = GetFontData(physDev->hdc, MS_MAKE_TAG('n','a','m','e'), 0, NULL, 0);
    if(size != 0 && size != GDI_ERROR)
    {
        BYTE *name = HeapAlloc(GetProcessHeap(), 0, size);
        if(name)
        {
            USHORT count, i;
            BYTE *strings;
            struct
            {
                USHORT platform_id;
                USHORT encoding_id;
                USHORT language_id;
                USHORT name_id;
                USHORT length;
                USHORT offset;
            } *name_record;

            GetFontData(physDev->hdc, MS_MAKE_TAG('n','a','m','e'), 0, name, size);
            count = GET_BE_WORD(name + 2);
            strings = name + GET_BE_WORD(name + 4);
            name_record = (typeof(name_record))(name + 6);
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
                    *str = HeapAlloc(GetProcessHeap(), 0, name_record->length + 1);
                    memcpy(*str, strings + name_record->offset, name_record->length);
                    *(*str + name_record->length) = '\0';
                    HeapFree(GetProcessHeap(), 0, name);
                    return;
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
                    *str = HeapAlloc(GetProcessHeap(), 0, len);
                    WideCharToMultiByte(1252, 0, unicode, -1, *str, len, NULL, NULL);
                    HeapFree(GetProcessHeap(), 0, unicode);
                    HeapFree(GetProcessHeap(), 0, name);
                    return;
                }
            }
            TRACE("Unable to find PostScript name\n");
            HeapFree(GetProcessHeap(), 0, name);
        }
    }

    len = strlen((char*)potm + (ptrdiff_t)potm->otmpFullName) + 1;
    *str = HeapAlloc(GetProcessHeap(),0,len);
    strcpy(*str, (char*)potm + (ptrdiff_t)potm->otmpFullName);

    p = *str;
    while((p = strchr(p, ' ')))
        *p = '_';

    return;
}

/****************************************************************************
 *  is_font_downloaded
 */
static DOWNLOAD *is_font_downloaded(PSDRV_PDEVICE *physDev, char *ps_name)
{
    DOWNLOAD *pdl;

    for(pdl = physDev->downloaded_fonts; pdl; pdl = pdl->next)
        if(!strcmp(pdl->ps_name, ps_name))
	    break;
    return pdl;
}

/****************************************************************************
 *  is_room_for_font
 */
static BOOL is_room_for_font(PSDRV_PDEVICE *physDev)
{
    DOWNLOAD *pdl;
    int count = 0;

    /* FIXME: should consider vm usage of each font and available printer memory.
       For now we allow up to two fonts to be downloaded at a time */
    for(pdl = physDev->downloaded_fonts; pdl; pdl = pdl->next)
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
static BOOL get_bbox(PSDRV_PDEVICE *physDev, RECT *rc, UINT *emsize)
{
    BYTE head[54]; /* the head table is 54 bytes long */

    if(GetFontData(physDev->hdc, MS_MAKE_TAG('h','e','a','d'), 0, head,
                   sizeof(head)) == GDI_ERROR) {
        ERR("Can't retrieve head table\n");
        return FALSE;
    }
    *emsize = GET_BE_WORD(head + 18); /* unitsPerEm */
    rc->left = (signed short)GET_BE_WORD(head + 36); /* xMin */
    rc->bottom = (signed short)GET_BE_WORD(head + 38); /* yMin */
    rc->right = (signed short)GET_BE_WORD(head + 40); /* xMax */
    rc->top = (signed short)GET_BE_WORD(head + 42); /* yMax */
    return TRUE;
}

/****************************************************************************
 *  PSDRV_SelectDownloadFont
 *
 *  Set up physDev->font for a downloadable font
 *
 */
BOOL PSDRV_SelectDownloadFont(PSDRV_PDEVICE *physDev)
{
    char *ps_name;
    LPOUTLINETEXTMETRICA potm;
    DWORD len = GetOutlineTextMetricsA(physDev->hdc, 0, NULL);

    if(!len) return FALSE;
    potm = HeapAlloc(GetProcessHeap(), 0, len);
    GetOutlineTextMetricsA(physDev->hdc, len, potm);
    get_download_name(physDev, potm, &ps_name);

    physDev->font.fontloc = Download;
    physDev->font.fontinfo.Download = is_font_downloaded(physDev, ps_name);

    physDev->font.size = abs(PSDRV_YWStoDS(physDev, /* ppem */
                                       potm->otmTextMetrics.tmAscent +
                                       potm->otmTextMetrics.tmDescent -
                                       potm->otmTextMetrics.tmInternalLeading));
    physDev->font.underlineThickness = potm->otmsUnderscoreSize;
    physDev->font.underlinePosition = potm->otmsUnderscorePosition;
    physDev->font.strikeoutThickness = potm->otmsStrikeoutSize;
    physDev->font.strikeoutPosition = potm->otmsStrikeoutPosition;

    HeapFree(GetProcessHeap(), 0, ps_name);
    HeapFree(GetProcessHeap(), 0, potm);
    return TRUE;
}

/****************************************************************************
 *  PSDRV_WriteSetDownloadFont
 *
 *  Write setfont for download font.
 *
 */
BOOL PSDRV_WriteSetDownloadFont(PSDRV_PDEVICE *physDev)
{
    char *ps_name;
    LPOUTLINETEXTMETRICA potm;
    DWORD len = GetOutlineTextMetricsA(physDev->hdc, 0, NULL);
    DOWNLOAD *pdl;

    assert(physDev->font.fontloc == Download);

    potm = HeapAlloc(GetProcessHeap(), 0, len);
    GetOutlineTextMetricsA(physDev->hdc, len, potm);

    get_download_name(physDev, potm, &ps_name);

    if(physDev->font.fontinfo.Download == NULL) {
        RECT bbox;
        UINT emsize;

        pdl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pdl));
	pdl->ps_name = HeapAlloc(GetProcessHeap(), 0, strlen(ps_name)+1);
	strcpy(pdl->ps_name, ps_name);
	pdl->next = NULL;

        if (!get_bbox(physDev, &bbox, &emsize))
		return FALSE;
        if(!is_room_for_font(physDev))
            PSDRV_EmptyDownloadList(physDev, TRUE);

        if(physDev->pi->ppd->TTRasterizer == RO_Type42) {
	    pdl->typeinfo.Type42 = T42_download_header(physDev, ps_name, &bbox, emsize);
	    pdl->type = Type42;
	}
	if(pdl->typeinfo.Type42 == NULL) {
	    pdl->typeinfo.Type1 = T1_download_header(physDev, ps_name, &bbox, emsize);
	    pdl->type = Type1;
	}
	pdl->next = physDev->downloaded_fonts;
	physDev->downloaded_fonts = pdl;
	physDev->font.fontinfo.Download = pdl;

        if(pdl->type == Type42) {
            char g_name[MAX_G_NAME + 1];
            get_glyph_name(physDev->hdc, 0, g_name);
            T42_download_glyph(physDev, pdl, 0, g_name);
        }
    }


    PSDRV_WriteSetFont(physDev, ps_name, physDev->font.size,
		       physDev->font.escapement);

    HeapFree(GetProcessHeap(), 0, ps_name);
    HeapFree(GetProcessHeap(), 0, potm);
    return TRUE;
}

void get_glyph_name(HDC hdc, WORD index, char *name)
{
  /* FIXME */
    sprintf(name, "g%04x", index);
    return;
}

/****************************************************************************
 *  PSDRV_WriteDownloadGlyphShow
 *
 *  Download and write out a number of glyphs
 *
 */
BOOL PSDRV_WriteDownloadGlyphShow(PSDRV_PDEVICE *physDev, WORD *glyphs,
				  UINT count)
{
    UINT i;
    char g_name[MAX_G_NAME + 1];
    assert(physDev->font.fontloc == Download);

    switch(physDev->font.fontinfo.Download->type) {
    case Type42:
    for(i = 0; i < count; i++) {
        get_glyph_name(physDev->hdc, glyphs[i], g_name);
	T42_download_glyph(physDev, physDev->font.fontinfo.Download,
			   glyphs[i], g_name);
	PSDRV_WriteGlyphShow(physDev, g_name);
    }
    break;

    case Type1:
    for(i = 0; i < count; i++) {
        get_glyph_name(physDev->hdc, glyphs[i], g_name);
	T1_download_glyph(physDev, physDev->font.fontinfo.Download,
			  glyphs[i], g_name);
	PSDRV_WriteGlyphShow(physDev, g_name);
    }
    break;

    default:
        ERR("Type = %d\n", physDev->font.fontinfo.Download->type);
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
BOOL PSDRV_EmptyDownloadList(PSDRV_PDEVICE *physDev, BOOL write_undef)
{
    DOWNLOAD *pdl, *old;
    static const char undef[] = "/%s findfont 40 scalefont setfont /%s undefinefont\n";
    char buf[sizeof(undef) + 200];
    const char *default_font = physDev->pi->ppd->DefaultFont ?
        physDev->pi->ppd->DefaultFont : "Courier";

    if(physDev->font.fontloc == Download) {
        physDev->font.set = FALSE;
	physDev->font.fontinfo.Download = NULL;
    }

    pdl = physDev->downloaded_fonts;
    physDev->downloaded_fonts = NULL;
    while(pdl) {
        if(write_undef) {
            sprintf(buf, undef, default_font, pdl->ps_name);
            PSDRV_WriteSpool(physDev, buf, strlen(buf));
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
