/*
 *	PostScript driver Type1 font functions
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

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "psdrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

struct tagTYPE1 {
    DWORD glyph_sent_size;
    BOOL *glyph_sent;
    DWORD emsize;
};

#define GLYPH_SENT_INC 128

/* Type 1 font commands */
enum t1_cmds {
    rlineto = 5,
    rrcurveto = 8,
    closepath = 9,
    hsbw = 13,
    endchar = 14,
    rmoveto = 21
};


TYPE1 *T1_download_header(PSDRV_PDEVICE *physDev, char *ps_name, RECT *bbox, UINT emsize)
{
    char *buf;
    TYPE1 *t1;

    char dict[] = /* name, emsquare, fontbbox */
      "25 dict begin\n"
      " /FontName /%s def\n"
      " /Encoding 256 array 0 1 255{1 index exch /.notdef put} for def\n"
      " /PaintType 0 def\n"
      " /FontMatrix [1 %d div 0 0 1 %d div 0 0] def\n"
      " /FontBBox [%d %d %d %d] def\n"
      " /FontType 1 def\n"
      " /Private 7 dict begin\n"
      "  /RD {string currentfile exch readhexstring pop} def\n"
      "  /ND {def} def\n"
      "  /NP {put} def\n"
      "  /MinFeature {16 16} def\n"
      "  /BlueValues [] def\n"
      "  /password 5839 def\n"
      "  /lenIV -1 def\n"
      " currentdict end def\n"
      " currentdict dup /Private get begin\n"
      "  /CharStrings 256 dict begin\n"
      "   /.notdef 4 RD 8b8b0d0e ND\n"
      "  currentdict end put\n"
      " end\n"
      "currentdict end dup /FontName get exch definefont pop\n";

    t1 = HeapAlloc(GetProcessHeap(), 0, sizeof(*t1));
    t1->emsize = emsize;

    t1->glyph_sent_size = GLYPH_SENT_INC;
    t1->glyph_sent = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			       t1->glyph_sent_size *
			       sizeof(*(t1->glyph_sent)));

    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(dict) + strlen(ps_name) +
		    100);

    sprintf(buf, dict, ps_name, t1->emsize, t1->emsize,
	    bbox->left, bbox->bottom, bbox->right, bbox->top);

    PSDRV_WriteSpool(physDev, buf, strlen(buf));

    HeapFree(GetProcessHeap(), 0, buf);
    return t1;
}


typedef struct {
  BYTE *str;
  int len, max_len;
} STR;

static STR *str_init(int sz)
{
  STR *str = HeapAlloc(GetProcessHeap(), 0, sizeof(*str));
  str->max_len = sz;
  str->str = HeapAlloc(GetProcessHeap(), 0, str->max_len);
  str->len = 0;
  return str;
}

static void str_free(STR *str)
{
  HeapFree(GetProcessHeap(), 0, str->str);
  HeapFree(GetProcessHeap(), 0, str);
}

static void str_add_byte(STR *str, BYTE b)
{
    if(str->len == str->max_len) {
        str->max_len *= 2;
	str->str = HeapReAlloc(GetProcessHeap(), 0, str->str, str->max_len);
    }
    str->str[str->len++] = b;
}

static void str_add_num(STR *str, int num)
{
    if(num <= 107 && num >= -107)
        str_add_byte(str, num + 139);
    else if(num >= 108 && num <= 1131) {
        str_add_byte(str, ((num - 108) >> 8) + 247);
	str_add_byte(str, (num - 108) & 0xff);
    } else if(num <= -108 && num >= -1131) {
        num = -num;
	str_add_byte(str, ((num - 108) >> 8) + 251);
	str_add_byte(str, (num - 108) & 0xff);
    } else {
        str_add_byte(str, 0xff);
	str_add_byte(str, (num >> 24) & 0xff);
	str_add_byte(str, (num >> 16) & 0xff);
	str_add_byte(str, (num >> 8) & 0xff);
	str_add_byte(str, (num & 0xff));
    }
}

static void str_add_point(STR *str, POINTFX *pt, POINT *curpos)
{
    POINT newpos;
    newpos.x = pt->x.value + ((pt->x.fract >> 15) & 0x1);
    newpos.y = pt->y.value + ((pt->y.fract >> 15) & 0x1);

    str_add_num(str, newpos.x - curpos->x);
    str_add_num(str, newpos.y - curpos->y);
    *curpos = newpos;
}

static void str_add_cmd(STR *str, enum t1_cmds cmd)
{
    str_add_byte(str, (BYTE)cmd);
}

static int str_get_bytes(STR *str, BYTE **b)
{
  *b = str->str;
  return str->len;
}

BOOL T1_download_glyph(PSDRV_PDEVICE *physDev, DOWNLOAD *pdl, DWORD index,
		       char *glyph_name)
{
    DWORD len, i;
    char *buf;
    TYPE1 *t1;
    STR *charstring;
    BYTE *bytes;
    HFONT old_font, unscaled_font;
    GLYPHMETRICS gm;
    char *glyph_buf;
    POINT curpos;
    TTPOLYGONHEADER *pph;
    TTPOLYCURVE *ppc;
    LOGFONTW lf;
    RECT rc;

    char glyph_def_begin[] =
      "/%s findfont dup\n"
      "/Private get begin\n"
      "/CharStrings get begin\n"
      "/%s %d RD\n";
    char glyph_def_end[] =
      "ND\n"
      "end end\n";

    TRACE("%d %s\n", index, glyph_name);
    assert(pdl->type == Type1);
    t1 = pdl->typeinfo.Type1;

    if(index < t1->glyph_sent_size) {
        if(t1->glyph_sent[index])
	    return TRUE;
    } else {
        t1->glyph_sent_size = (index / GLYPH_SENT_INC + 1) * GLYPH_SENT_INC;
	t1->glyph_sent = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				      t1->glyph_sent,
				      t1->glyph_sent_size * sizeof(*(t1->glyph_sent)));
    }

    GetObjectW(GetCurrentObject(physDev->hdc, OBJ_FONT), sizeof(lf), &lf);
    rc.left = rc.right = rc.bottom = 0;
    rc.top = t1->emsize;
    DPtoLP(physDev->hdc, (POINT*)&rc, 2);
    lf.lfHeight = -abs(rc.top - rc.bottom);
    lf.lfOrientation = lf.lfEscapement = 0;
    unscaled_font = CreateFontIndirectW(&lf);
    old_font = SelectObject(physDev->hdc, unscaled_font);
    len = GetGlyphOutlineW(physDev->hdc, index, GGO_GLYPH_INDEX | GGO_BEZIER,
			   &gm, 0, NULL, NULL);
    if(len == GDI_ERROR) return FALSE;
    glyph_buf = HeapAlloc(GetProcessHeap(), 0, len);
    GetGlyphOutlineW(physDev->hdc, index, GGO_GLYPH_INDEX | GGO_BEZIER,
		     &gm, len, glyph_buf, NULL);

    SelectObject(physDev->hdc, old_font);
    DeleteObject(unscaled_font);

    charstring = str_init(100);

    curpos.x = gm.gmptGlyphOrigin.x;
    curpos.y = 0;

    str_add_num(charstring, curpos.x);
    str_add_num(charstring, gm.gmCellIncX);
    str_add_cmd(charstring, hsbw);

    pph = (TTPOLYGONHEADER*)glyph_buf;
    while((char*)pph < glyph_buf + len) {
        TRACE("contour len %d\n", pph->cb);
	ppc = (TTPOLYCURVE*)((char*)pph + sizeof(*pph));

	str_add_point(charstring, &pph->pfxStart, &curpos);
	str_add_cmd(charstring, rmoveto);

	while((char*)ppc < (char*)pph + pph->cb) {
	    TRACE("line type %d cpfx = %d\n", ppc->wType, ppc->cpfx);
	    switch(ppc->wType) {
	    case TT_PRIM_LINE:
	        for(i = 0; i < ppc->cpfx; i++) {
		    str_add_point(charstring, ppc->apfx + i, &curpos);
		    str_add_cmd(charstring, rlineto);
		}
		break;
	    case TT_PRIM_CSPLINE:
	        for(i = 0; i < ppc->cpfx/3; i++) {
		    str_add_point(charstring, ppc->apfx + 3 * i, &curpos);
		    str_add_point(charstring, ppc->apfx + 3 * i + 1, &curpos);
		    str_add_point(charstring, ppc->apfx + 3 * i + 2, &curpos);
		    str_add_cmd(charstring, rrcurveto);
		}
		break;
	    default:
	        ERR("curve type = %d\n", ppc->wType);
		return FALSE;
	    }
	    ppc = (TTPOLYCURVE*)((char*)ppc + sizeof(*ppc) +
				 (ppc->cpfx - 1) * sizeof(POINTFX));
	}
	str_add_cmd(charstring, closepath);
	pph = (TTPOLYGONHEADER*)((char*)pph + pph->cb);
    }
    str_add_cmd(charstring, endchar);

    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(glyph_def_begin) +
		    strlen(pdl->ps_name) + strlen(glyph_name) + 100);

    sprintf(buf, "%%%%glyph %04x\n", index);
    PSDRV_WriteSpool(physDev, buf, strlen(buf));

    len = str_get_bytes(charstring, &bytes);
    sprintf(buf, glyph_def_begin, pdl->ps_name, glyph_name, len);
    PSDRV_WriteSpool(physDev, buf, strlen(buf));
    PSDRV_WriteBytes(physDev, bytes, len);
    sprintf(buf, glyph_def_end);
    PSDRV_WriteSpool(physDev, buf, strlen(buf));
    str_free(charstring);

    t1->glyph_sent[index] = TRUE;
    HeapFree(GetProcessHeap(), 0, glyph_buf);
    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}

void T1_free(TYPE1 *t1)
{
    HeapFree(GetProcessHeap(), 0, t1->glyph_sent);
    HeapFree(GetProcessHeap(), 0, t1);
    return;
}
