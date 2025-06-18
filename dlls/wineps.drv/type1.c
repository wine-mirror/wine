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

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winternl.h"

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


#define MS_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (DWORD)_x4 << 24 ) |     \
            ( (DWORD)_x3 << 16 ) |     \
            ( (DWORD)_x2 <<  8 ) |     \
              (DWORD)_x1         )

#ifdef WORDS_BIGENDIAN
static inline WORD  get_be_word(const void *p)  { return *(const WORD*)p; }
static inline DWORD get_be_dword(const void *p) { return *(const DWORD*)p; }
#else
static inline WORD  get_be_word(const void *p)  { return RtlUshortByteSwap(*(const WORD*)p); }
static inline DWORD get_be_dword(const void *p) { return RtlUlongByteSwap(*(const DWORD*)p); }
#endif

TYPE1 *T1_download_header(print_ctx *ctx, char *ps_name, RECT *bbox, UINT emsize)
{
    char *buf;
    TYPE1 *t1;

    static const char dict[] = /* name, emsquare, fontbbox */
      "25 dict begin\n"
      " /FontName /%s def\n"
      " /Encoding 256 array 0 1 255{1 index exch /.notdef put} for def\n"
      " /PaintType 0 def\n"
      " /FontMatrix [1 %ld div 0 0 1 %ld div 0 0] def\n"
      " /FontBBox [%ld %ld %ld %ld] def\n"
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

    PSDRV_WriteSpool(ctx, buf, strlen(buf));

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

static void str_add_point(STR *str, POINT pt, POINT *curpos)
{
    str_add_num(str, pt.x - curpos->x);
    str_add_num(str, pt.y - curpos->y);
    *curpos = pt;
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

static BOOL get_hmetrics(HDC hdc, DWORD index, short *lsb, WORD *advance)
{
    BYTE hhea[36];
    WORD num_of_long;
    BYTE buf[4];

    *lsb = 0;
    *advance = 0;

    GetFontData(hdc, MS_MAKE_TAG('h','h','e','a'), 0, hhea, sizeof(hhea));
    num_of_long = get_be_word(hhea + 34);

    if(index < num_of_long)
    {
        if(GetFontData(hdc, MS_MAKE_TAG('h','m','t','x'), index * 4, buf, 4) != 4) return FALSE;
        *advance = get_be_word(buf);
        *lsb = (signed short)get_be_word(buf + 2);
    }
    else
    {
        if(GetFontData(hdc, MS_MAKE_TAG('h','m','t','x'), (num_of_long - 1) * 4, buf, 2) != 2) return FALSE;
        *advance = get_be_word(buf);
        if(GetFontData(hdc, MS_MAKE_TAG('h','m','t','x'), num_of_long * 4 + (index - num_of_long) * 2, buf, 2) != 2) return FALSE;
        *lsb = (signed short)get_be_word(buf);
    }

    return TRUE;
}

static BOOL get_glyf_pos(HDC hdc, DWORD index, DWORD *start, DWORD *end)
{
    DWORD len;
    BYTE *head, *loca;
    WORD loca_format;
    BOOL ret = FALSE;

    *start = *end = 0;

    len = GetFontData(hdc, MS_MAKE_TAG('h','e','a','d'), 0, NULL, 0);
    if (len == GDI_ERROR) return FALSE;
    head = HeapAlloc(GetProcessHeap(), 0, len);
    GetFontData(hdc, MS_MAKE_TAG('h','e','a','d'), 0, head, len);
    loca_format = get_be_word(head + 50);

    len = GetFontData(hdc, MS_MAKE_TAG('l','o','c','a'), 0, NULL, 0);
    if (len == GDI_ERROR)
    {
        len = GetFontData(hdc, MS_MAKE_TAG('C','F','F',' '), 0, NULL, 0);
        if (len != GDI_ERROR) FIXME( "CFF tables not supported yet\n" );
        else ERR( "loca table not found\n" );
        HeapFree(GetProcessHeap(), 0, head);
        return FALSE;
    }
    loca = HeapAlloc(GetProcessHeap(), 0, len);
    GetFontData(hdc, MS_MAKE_TAG('l','o','c','a'), 0, loca, len);

    switch(loca_format) {
    case 0:
        *start = get_be_word(((WORD*)loca) + index);
        *start <<= 1;
        *end = get_be_word(((WORD*)loca) + index + 1);
        *end <<= 1;
        ret = TRUE;
        break;
    case 1:
        *start = get_be_dword(((DWORD*)loca) + index);
        *end = get_be_dword(((DWORD*)loca) + index + 1);
        ret = TRUE;
        break;
    default:
        ERR("Unknown loca_format %d\n", loca_format);
    }

    HeapFree(GetProcessHeap(), 0, loca);
    HeapFree(GetProcessHeap(), 0, head);
    return ret;
}


static BYTE *get_glyph_data(HDC hdc, DWORD index)
{
    DWORD start, end, len;
    BYTE *data;

    if(!get_glyf_pos(hdc, index, &start, &end))
        return NULL;

    len = end - start;
    if(!len) return NULL;

    data = HeapAlloc(GetProcessHeap(), 0, len);
    if(!data) return NULL;

    if(GetFontData(hdc, MS_MAKE_TAG('g','l','y','f'), start, data, len) != len)
    {
        HeapFree(GetProcessHeap(), 0, data);
        return NULL;
    }
    return data;
}

typedef struct
{
    WORD num_conts;
    WORD *end_pts; /* size_is(num_conts) */
    BYTE *flags;   /* size_is(end_pts[num_conts - 1] + 1) */
    POINT *pts;    /* size_is(end_pts[num_conts - 1] + 1) */
    short lsb;
    WORD advance;
} glyph_outline;

static inline WORD pts_in_outline(glyph_outline *outline)
{
    WORD num_pts = 0;

    if(outline->num_conts)
        num_pts = outline->end_pts[outline->num_conts - 1] + 1;

    return num_pts;
}

static BOOL append_glyph_outline(HDC hdc, DWORD index, glyph_outline *outline);

static BOOL append_simple_glyph(BYTE *data, glyph_outline *outline)
{
    USHORT num_pts, start_pt = 0, ins_len;
    WORD *end_pts;
    int num_conts, start_cont, i;
    BYTE *ptr;
    int x, y;

    start_cont = outline->num_conts;
    start_pt = pts_in_outline(outline);

    num_conts = get_be_word(data);

    end_pts = (WORD*)(data + 10);
    num_pts = get_be_word(end_pts + num_conts - 1) + 1;

    ins_len = get_be_word(end_pts + num_conts);

    ptr = (BYTE*)(end_pts + num_conts) + 2 + ins_len;

    if(outline->num_conts)
    {
        outline->end_pts = HeapReAlloc(GetProcessHeap(), 0, outline->end_pts, (start_cont + num_conts) * sizeof(*outline->end_pts));
        outline->flags   = HeapReAlloc(GetProcessHeap(), 0, outline->flags, start_pt + num_pts);
        outline->pts     = HeapReAlloc(GetProcessHeap(), 0, outline->pts,  (start_pt + num_pts) * sizeof(*outline->pts));
    }
    else
    {
        outline->end_pts = HeapAlloc(GetProcessHeap(), 0, num_conts * sizeof(*outline->end_pts));
        outline->flags   = HeapAlloc(GetProcessHeap(), 0, num_pts);
        outline->pts     = HeapAlloc(GetProcessHeap(), 0, num_pts * sizeof(*outline->pts));
    }

    outline->num_conts += num_conts;

    for(i = 0; i < num_conts; i++)
        outline->end_pts[start_cont + i] = start_pt + get_be_word(end_pts + i);

    for(i = 0; i < num_pts; i++)
    {
        outline->flags[start_pt + i] = *ptr;
        if(*ptr & 8)
        {
            BYTE count = *++ptr;
            while(count)
            {
                i++;
                outline->flags[start_pt + i] = *(ptr - 1);
                count--;
            }
        }
        ptr++;
    }

    x = 0;

    for(i = 0; i < num_pts; i++)
    {
        INT delta;

        delta = 0;
        if(outline->flags[start_pt + i] & 2)
        {
            delta = *ptr++;
            if((outline->flags[start_pt + i] & 0x10) == 0)
                delta = -delta;
        }
        else if((outline->flags[start_pt + i] & 0x10) == 0)
        {
            delta = (signed short)get_be_word(ptr);
            ptr += 2;
        }
        x += delta;
        outline->pts[start_pt + i].x = x;
    }

    y = 0;
    for(i = 0; i < num_pts; i++)
    {
        INT delta;

        delta = 0;
        if(outline->flags[start_pt + i] & 4)
        {
            delta = *ptr++;
            if((outline->flags[start_pt + i] & 0x20) == 0)
                delta = -delta;
        }
        else if((outline->flags[start_pt + i] & 0x20) == 0)
        {
            delta = (signed short)get_be_word(ptr);
            ptr += 2;
        }
        y += delta;
        outline->pts[start_pt + i].y = y;
    }
    return TRUE;
}

/* Some flags for composite glyphs.  See glyf table in OT spec */
#define ARG_1_AND_2_ARE_WORDS    (1L << 0)
#define ARGS_ARE_XY_VALUES       (1L << 1)
#define WE_HAVE_A_SCALE          (1L << 3)
#define MORE_COMPONENTS          (1L << 5)
#define WE_HAVE_AN_X_AND_Y_SCALE (1L << 6)
#define WE_HAVE_A_TWO_BY_TWO     (1L << 7)
#define USE_MY_METRICS           (1L << 9)

static BOOL append_complex_glyph(HDC hdc, const BYTE *data, glyph_outline *outline)
{
    const BYTE *ptr = data;
    WORD flags, index;
    short arg1, arg2;
    FLOAT scale_xx = 1, scale_xy = 0, scale_yx = 0, scale_yy = 1;
    WORD start_pt, end_pt;

    ptr += 10;
    do
    {
        flags = get_be_word(ptr);
        ptr += 2;
        index = get_be_word(ptr);
        ptr += 2;
        if(flags & ARG_1_AND_2_ARE_WORDS)
        {
            arg1 = (short)get_be_word(ptr);
            ptr += 2;
            arg2 = (short)get_be_word(ptr);
            ptr += 2;
        }
        else
        {
            arg1 = *(const char*)ptr++;
            arg2 = *(const char*)ptr++;
        }
        if(flags & WE_HAVE_A_SCALE)
        {
            scale_xx = scale_yy = (FLOAT)(short)get_be_word(ptr) / 0x4000;
            ptr += 2;
        }
        else if(flags & WE_HAVE_AN_X_AND_Y_SCALE)
        {
            scale_xx = (FLOAT)(short)get_be_word(ptr) / 0x4000;
            ptr += 2;
            scale_yy = (FLOAT)(short)get_be_word(ptr) / 0x4000;
            ptr += 2;
        }
        else if(flags & WE_HAVE_A_TWO_BY_TWO)
        {
            scale_xx = (FLOAT)(short)get_be_word(ptr) / 0x4000;
            ptr += 2;
            scale_xy = (FLOAT)(short)get_be_word(ptr) / 0x4000;
            ptr += 2;
            scale_yx = (FLOAT)(short)get_be_word(ptr) / 0x4000;
            ptr += 2;
            scale_yy = (FLOAT)(short)get_be_word(ptr) / 0x4000;
            ptr += 2;
        }

        start_pt = pts_in_outline(outline);
        append_glyph_outline(hdc, index, outline);
        end_pt = pts_in_outline(outline);

        if (flags & (WE_HAVE_A_SCALE | WE_HAVE_AN_X_AND_Y_SCALE | WE_HAVE_A_TWO_BY_TWO))
        {
            WORD i;
            TRACE("transform %f,%f,%f,%f of glyph %x\n", scale_xx, scale_xy, scale_yx, scale_yy, index);
            for (i = start_pt; i < end_pt; i++)
            {
                LONG x = outline->pts[i].x, y = outline->pts[i].y;
                outline->pts[i].x = x * scale_xx + y * scale_yx;
                outline->pts[i].y = x * scale_xy + y * scale_yy;
            }
        }

        if((flags & ARGS_ARE_XY_VALUES) == 0)
        {
            WORD orig_pt = arg1, new_pt = arg2;
            new_pt += start_pt;

            if(orig_pt >= start_pt || new_pt >= end_pt) return FALSE;

            arg1 = outline->pts[orig_pt].x - outline->pts[new_pt].x;
            arg2 = outline->pts[orig_pt].y - outline->pts[new_pt].y;
        }
        while(start_pt < end_pt)
        {
            outline->pts[start_pt].x += arg1;
            outline->pts[start_pt].y += arg2;
            start_pt++;
        }

        if(flags & USE_MY_METRICS)
            get_hmetrics(hdc, index, &outline->lsb, &outline->advance);

    } while(flags & MORE_COMPONENTS);
    return TRUE;
}

static BOOL append_glyph_outline(HDC hdc, DWORD index, glyph_outline *outline)
{
    BYTE *glyph_data;
    SHORT num_conts;

    glyph_data = get_glyph_data(hdc, index);
    if(!glyph_data) return TRUE;

    num_conts = get_be_word(glyph_data);

    if(num_conts == -1)
        append_complex_glyph(hdc, glyph_data, outline);
    else if(num_conts > 0)
        append_simple_glyph(glyph_data, outline);

    HeapFree(GetProcessHeap(), 0, glyph_data);
    return TRUE;
}

static inline BOOL on_point(const glyph_outline *outline, WORD pt)
{
    return outline->flags[pt] & 1;
}

BOOL T1_download_glyph(print_ctx *ctx, DOWNLOAD *pdl, DWORD index, char *glyph_name)
{
    DWORD len;
    WORD cur_pt, cont;
    char *buf;
    TYPE1 *t1;
    STR *charstring;
    BYTE *bytes;
    POINT curpos;
    glyph_outline outline;

    static const char glyph_def_begin[] =
      "/%s findfont dup\n"
      "/Private get begin\n"
      "/CharStrings get begin\n"
      "/%s %ld RD\n";
    static const char glyph_def_end[] =
      "ND\n"
      "end end\n";

    TRACE("%ld %s\n", index, glyph_name);
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

    outline.num_conts = 0;
    outline.flags = NULL;
    outline.end_pts = NULL;
    outline.pts = NULL;
    get_hmetrics(ctx->hdc, index, &outline.lsb, &outline.advance);

    if(!append_glyph_outline(ctx->hdc, index, &outline)) return FALSE;

    charstring = str_init(100);
    curpos.x = outline.lsb;
    curpos.y = 0;

    str_add_num(charstring, curpos.x);
    str_add_num(charstring, outline.advance);
    str_add_cmd(charstring, hsbw);

    for(cur_pt = 0, cont = 0; cont < outline.num_conts; cont++)
    {
        POINT start_pos = outline.pts[cur_pt++];
        WORD end_pt = outline.end_pts[cont];
        POINT curve_pts[3] = {{0,0},{0,0},{0,0}};

	str_add_point(charstring, start_pos, &curpos);
	str_add_cmd(charstring, rmoveto);

        for(; cur_pt <= end_pt; cur_pt++)
        {
            if(on_point(&outline, cur_pt))
            {
                str_add_point(charstring, outline.pts[cur_pt], &curpos);
                str_add_cmd(charstring, rlineto);
            }
            else
            {
                BOOL added_next = FALSE;

                /* temporarily store the start pt in curve_pts[0] */
                if(on_point(&outline, cur_pt - 1))
                    curve_pts[0] = outline.pts[cur_pt - 1];
                else /* last pt was off curve too, so the previous curve's
                        end pt was constructed */
                    curve_pts[0] = curve_pts[2];

                if(cur_pt != end_pt)
                {
                    if(on_point(&outline, cur_pt + 1))
                    {
                        curve_pts[2] = outline.pts[cur_pt + 1];
                        added_next = TRUE;
                    }
                    else /* next pt is off curve too, so construct the end pt from the
                            average of the two */
                    {
                        curve_pts[2].x = (outline.pts[cur_pt].x + outline.pts[cur_pt + 1].x + 1) / 2;
                        curve_pts[2].y = (outline.pts[cur_pt].y + outline.pts[cur_pt + 1].y + 1) / 2;
                    }
                }
                else /* last pt of the contour is off curve, so the end pt is the
                        start pt of the contour */
                    curve_pts[2] = start_pos;

                curve_pts[0].x = (curve_pts[0].x + 2 * outline.pts[cur_pt].x + 1) / 3;
                curve_pts[0].y = (curve_pts[0].y + 2 * outline.pts[cur_pt].y + 1) / 3;

                curve_pts[1].x = (curve_pts[2].x + 2 * outline.pts[cur_pt].x + 1) / 3;
                curve_pts[1].y = (curve_pts[2].y + 2 * outline.pts[cur_pt].y + 1) / 3;

                str_add_point(charstring, curve_pts[0], &curpos);
                str_add_point(charstring, curve_pts[1], &curpos);
                str_add_point(charstring, curve_pts[2], &curpos);
                str_add_cmd(charstring, rrcurveto);
                if(added_next) cur_pt++;
            }
        }
        str_add_cmd(charstring, closepath);
    }
    str_add_cmd(charstring, endchar);

    HeapFree(GetProcessHeap(), 0, outline.pts);
    HeapFree(GetProcessHeap(), 0, outline.end_pts);
    HeapFree(GetProcessHeap(), 0, outline.flags);

    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(glyph_def_begin) +
		    strlen(pdl->ps_name) + strlen(glyph_name) + 100);

    sprintf(buf, "%%%%glyph %04lx\n", index);
    PSDRV_WriteSpool(ctx, buf, strlen(buf));

    len = str_get_bytes(charstring, &bytes);
    sprintf(buf, glyph_def_begin, pdl->ps_name, glyph_name, len);
    PSDRV_WriteSpool(ctx, buf, strlen(buf));
    PSDRV_WriteBytes(ctx, bytes, len);
    sprintf(buf, glyph_def_end);
    PSDRV_WriteSpool(ctx, buf, strlen(buf));
    str_free(charstring);

    t1->glyph_sent[index] = TRUE;
    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}

void T1_free(TYPE1 *t1)
{
    HeapFree(GetProcessHeap(), 0, t1->glyph_sent);
    HeapFree(GetProcessHeap(), 0, t1);
    return;
}
