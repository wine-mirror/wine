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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "winspool.h"
#include "gdi.h"
#include "psdrv.h"
#include "wine/debug.h"
#include "winerror.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);


/****************************************************************************
 *  get_download_name
 */
static void get_download_name(PSDRV_PDEVICE *physDev, LPOUTLINETEXTMETRICA
			      potm, char **str)
{
    int len;
    char *p;
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

    potm = HeapAlloc(GetProcessHeap(), 0, len);
    GetOutlineTextMetricsA(physDev->hdc, len, potm);
    get_download_name(physDev, potm, &ps_name);

    physDev->font.fontloc = Download;
    physDev->font.fontinfo.Download = is_font_downloaded(physDev, ps_name);

    physDev->font.size = INTERNAL_YWSTODS(physDev->dc, /* ppem */
					  potm->otmTextMetrics.tmAscent +
					  potm->otmTextMetrics.tmDescent -
					  potm->otmTextMetrics.tmInternalLeading);
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
        pdl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pdl));
	pdl->ps_name = HeapAlloc(GetProcessHeap(), 0, strlen(ps_name)+1);
	strcpy(pdl->ps_name, ps_name);
	pdl->next = NULL;

        if(physDev->pi->ppd->TTRasterizer == RO_Type42) {
	    pdl->typeinfo.Type42 = T42_download_header(physDev, potm,
						       ps_name);
	    pdl->type = Type42;
	}
	if(pdl->typeinfo.Type42 == NULL) {
	    pdl->typeinfo.Type1 = T1_download_header(physDev, potm, ps_name);
	    pdl->type = Type1;
	}
	pdl->next = physDev->downloaded_fonts;
	physDev->downloaded_fonts = pdl;
	physDev->font.fontinfo.Download = pdl;
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
BOOL PSDRV_EmptyDownloadList(PSDRV_PDEVICE *physDev)
{
    DOWNLOAD *pdl, *old;
    if(physDev->font.fontloc == Download) {
        physDev->font.set = FALSE;
	physDev->font.fontinfo.Download = NULL;
    }

    pdl = physDev->downloaded_fonts;
    physDev->downloaded_fonts = NULL;
    while(pdl) {
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
