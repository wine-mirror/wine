/*
 *	PostScript driver Type42 font functions
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
#include <locale.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "psdrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);


#define GET_BE_WORD(ptr)  MAKEWORD( ((BYTE *)(ptr))[1], ((BYTE *)(ptr))[0] )
#define GET_BE_DWORD(ptr) ((DWORD)MAKELONG( GET_BE_WORD(&((WORD *)(ptr))[1]), \
                                            GET_BE_WORD(&((WORD *)(ptr))[0]) ))

#define MS_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (DWORD)_x4 << 24 ) |     \
            ( (DWORD)_x3 << 16 ) |     \
            ( (DWORD)_x2 <<  8 ) |     \
              (DWORD)_x1         )

typedef struct {
  DWORD MS_tag;
  DWORD len, check;
  BYTE *data;
  BOOL write;
} OTTable;

static const OTTable tables_templ[] = {
      { MS_MAKE_TAG('c','v','t',' '), 0, 0, NULL, TRUE },
      { MS_MAKE_TAG('f','p','g','m'), 0, 0, NULL, TRUE },
      { MS_MAKE_TAG('g','d','i','r'), 0, 0, NULL, TRUE },
      { MS_MAKE_TAG('g','l','y','f'), 0, 0, NULL, FALSE },
      { MS_MAKE_TAG('h','e','a','d'), 0, 0, NULL, TRUE },
      { MS_MAKE_TAG('h','h','e','a'), 0, 0, NULL, TRUE },
      { MS_MAKE_TAG('h','m','t','x'), 0, 0, NULL, TRUE },
      { MS_MAKE_TAG('l','o','c','a'), 0, 0, NULL, TRUE },
      { MS_MAKE_TAG('m','a','x','p'), 0, 0, NULL, TRUE },
      { MS_MAKE_TAG('p','r','e','p'), 0, 0, NULL, TRUE },
      { 0, 0, 0, NULL, 0 }
};

struct tagTYPE42 {
    OTTable tables[ARRAY_SIZE(tables_templ)];
    int glyf_tab, loca_tab, head_tab; /* indices of glyf, loca and head tables */
    int maxp_tab;
    int num_of_written_tables;
    DWORD glyph_sent_size;
    BOOL *glyph_sent;
    DWORD emsize;
    DWORD *glyf_blocks;
};

#define GLYPH_SENT_INC 128

#define FLIP_ORDER(x) \
 ( ( ((x) & 0xff) << 24) | \
   ( ((x) & 0xff00) << 8) | \
   ( ((x) & 0xff0000) >> 8) | \
   ( ((x) & 0xff000000) >> 24) )


/* Some flags for composite glyphs.  See glyf table in OT spec */
#define ARG_1_AND_2_ARE_WORDS    (1L << 0)
#define WE_HAVE_A_SCALE          (1L << 3)
#define MORE_COMPONENTS          (1L << 5)
#define WE_HAVE_AN_X_AND_Y_SCALE (1L << 6)
#define WE_HAVE_A_TWO_BY_TWO     (1L << 7)


static BOOL LoadTable(HDC hdc, OTTable *table)
{
    unsigned int i;
    DWORD len;

    if(table->MS_tag == MS_MAKE_TAG('g','d','i','r')) return TRUE;
    table->len = 0;
    len = GetFontData(hdc, table->MS_tag, 0, NULL, 0);
    if(len == GDI_ERROR) return FALSE;
    table->data = HeapAlloc(GetProcessHeap(), 0, (len + 3) & ~3);
    if(!table->data) return FALSE;
    table->len = len;
    memset(table->data + ((table->len - 1) & ~3), 0, sizeof(DWORD));
    GetFontData(hdc, table->MS_tag, 0, table->data, table->len);
    table->check = 0;
    for(i = 0; i < (table->len + 3) / 4; i++)
        table->check += FLIP_ORDER(*((DWORD*)(table->data) + i));
    return TRUE;
}

static BOOL get_glyf_pos(TYPE42 *t42, DWORD index, DWORD *start, DWORD *end)
{
    WORD loca_format = GET_BE_WORD(t42->tables[t42->head_tab].data + 50);
    TRACE("loca_format = %d\n", loca_format);
    switch(loca_format) {
    case 0:
        *start = GET_BE_WORD(((WORD*)t42->tables[t42->loca_tab].data) + index);
        *start <<= 1;
        *end = GET_BE_WORD(((WORD*)t42->tables[t42->loca_tab].data) + index + 1);
        *end <<= 1;
        break;
    case 1:
        *start = GET_BE_DWORD(((DWORD*)t42->tables[t42->loca_tab].data) + index);
        *end = GET_BE_DWORD(((DWORD*)t42->tables[t42->loca_tab].data) + index + 1);
        break;
    default:
        ERR("Unknown loca_format %d\n", loca_format);
        return FALSE;
    }
    return TRUE;
}

TYPE42 *T42_download_header(print_ctx *ctx, char *ps_name,
                            RECT *bbox, UINT emsize)
{
    DWORD i, j, tablepos, nb_blocks, glyf_off = 0, loca_off = 0, cur_off;
    WORD num_of_tables = ARRAY_SIZE(tables_templ) - 1;
    char *buf;
    TYPE42 *t42;
    static const char start[] = /* name, fontbbox */
            "25 dict begin\n"
	    " /FontName /%s def\n"
	    " /Encoding 256 array 0 1 255{1 index exch /.notdef put} for\n"
	    " def\n"
	    " /PaintType 0 def\n"
	    " /FontMatrix [1 0 0 1 0 0] def\n"
	    " /FontBBox [%f %f %f %f] def\n"
	    " /FontType 42 def\n"
	    " /CharStrings 256 dict begin\n"
	    "  /.notdef 0 def\n"
            " currentdict end def\n"
	    " /sfnts [\n";
    static const char TT_offset_table[] = "<00010000%04x%04x%04x%04x\n";
    static const char TT_table_dir_entry[] = "%08lx%08lx%08lx%08lx\n";
    static const char storage[] ="]\nhavetype42gdir{pop}{{string} forall}ifelse\n";
    static const char end[] = "] def\n"
      "havetype42gdir{/GlyphDirectory 256 dict def\n"
      " sfnts 0 get dup\n"
      "  %ld <6c6f6378000000000000000000000000> putinterval\n" /* replace loca entry with dummy locx */
      "  %ld <676c6678000000000000000000000000> putinterval\n" /* replace glyf entry with dummy glfx */
      " }if\n"
      "currentdict end dup /FontName get exch definefont pop\n";


    t42 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*t42));
    memcpy(t42->tables, tables_templ, sizeof(tables_templ));
    t42->emsize = emsize;
    t42->num_of_written_tables = 0;

    for(i = 0; i < num_of_tables; i++) {
        LoadTable(ctx->hdc, t42->tables + i);
	if(t42->tables[i].len > 0xffff && t42->tables[i].write) break;
	if(t42->tables[i].write) t42->num_of_written_tables++;
	if(t42->tables[i].MS_tag == MS_MAKE_TAG('l','o','c','a'))
	    t42->loca_tab = i;
	else if(t42->tables[i].MS_tag == MS_MAKE_TAG('g','l','y','f'))
	    t42->glyf_tab = i;
	else if(t42->tables[i].MS_tag == MS_MAKE_TAG('h','e','a','d'))
	    t42->head_tab = i;
	else if(t42->tables[i].MS_tag == MS_MAKE_TAG('m','a','x','p'))
	    t42->maxp_tab = i;
        else
            continue;
        if(!t42->tables[i].len) break;
    }
    if(i < num_of_tables) {
        TRACE("Table %ld has length %ld.  Will use Type 1 font instead.\n", i, t42->tables[i].len);
        T42_free(t42);
	return NULL;
    }

    t42->glyph_sent_size = GLYPH_SENT_INC;
    t42->glyph_sent = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				t42->glyph_sent_size *
				sizeof(*(t42->glyph_sent)));

    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(start) + strlen(ps_name) +
		    100);

    _sprintf_l(buf, start, c_locale, ps_name,
               (float)bbox->left / emsize, (float)bbox->bottom / emsize,
               (float)bbox->right / emsize, (float)bbox->top / emsize);

    PSDRV_WriteSpool(ctx, buf, strlen(buf));

    t42->num_of_written_tables++; /* explicitly add glyf */
    sprintf(buf, TT_offset_table, t42->num_of_written_tables,
	    t42->num_of_written_tables, t42->num_of_written_tables, t42->num_of_written_tables);

    PSDRV_WriteSpool(ctx, buf, strlen(buf));

    tablepos = 12 + t42->num_of_written_tables * 16;
    cur_off = 12;
    for(i = 0; i < num_of_tables; i++) {
        if(!t42->tables[i].write) continue;
        sprintf(buf, TT_table_dir_entry, FLIP_ORDER(t42->tables[i].MS_tag),
		t42->tables[i].check, t42->tables[i].len ? tablepos : 0,
		t42->tables[i].len);
	PSDRV_WriteSpool(ctx, buf, strlen(buf));
	tablepos += ((t42->tables[i].len + 3) & ~3);
        if(t42->tables[i].MS_tag == MS_MAKE_TAG('l','o','c','a'))
            loca_off = cur_off;
        cur_off += 16;
    }
    sprintf(buf, TT_table_dir_entry, FLIP_ORDER(t42->tables[t42->glyf_tab].MS_tag),
            t42->tables[t42->glyf_tab].check, tablepos, t42->tables[t42->glyf_tab].len);
    PSDRV_WriteSpool(ctx, buf, strlen(buf));
    PSDRV_WriteSpool(ctx, "00>\n", 4); /* add an extra byte for old PostScript rips */
    glyf_off = cur_off;

    for(i = 0; i < num_of_tables; i++) {
        if(t42->tables[i].len == 0 || !t42->tables[i].write) continue;
	PSDRV_WriteSpool(ctx, "<", 1);
	for(j = 0; j < ((t42->tables[i].len + 3) & ~3); j++) {
	    sprintf(buf, "%02x", t42->tables[i].data[j]);
	    PSDRV_WriteSpool(ctx, buf, strlen(buf));
	    if(j % 16 == 15) PSDRV_WriteSpool(ctx, "\n", 1);
	}
	PSDRV_WriteSpool(ctx, "00>\n", 4); /* add an extra byte for old PostScript rips */
    }
    
    /* glyf_blocks is a 0 terminated list, holding the start offset of each block.  For simplicity
       glyf_blocks[0] is 0 */
    nb_blocks = 2;
    t42->glyf_blocks = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nb_blocks + 1) * sizeof(DWORD));
    for(i = 0; i < GET_BE_WORD(t42->tables[t42->maxp_tab].data + 4); i++) {
        DWORD start, end, size;
        get_glyf_pos(t42, i, &start, &end);
        size = end - t42->glyf_blocks[nb_blocks-2];
        if(size > 0x2000 && t42->glyf_blocks[nb_blocks-1] % 4 == 0) {
            nb_blocks++;
            t42->glyf_blocks = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                           t42->glyf_blocks, (nb_blocks + 1) * sizeof(DWORD));
        }
        t42->glyf_blocks[nb_blocks-1] = end;
    }

    PSDRV_WriteSpool(ctx, "[ ", 2);
    for(i = 1; t42->glyf_blocks[i]; i++) {
        sprintf(buf,"%ld ", t42->glyf_blocks[i] - t42->glyf_blocks[i-1] + 1);
        /* again add one byte for old PostScript rips */
        PSDRV_WriteSpool(ctx, buf, strlen(buf));
        if(i % 8 == 0)
            PSDRV_WriteSpool(ctx, "\n", 1);
    }
    PSDRV_WriteSpool(ctx, storage, sizeof(storage) - 1);
    sprintf(buf, end, loca_off, glyf_off);
    PSDRV_WriteSpool(ctx, buf, strlen(buf));
    HeapFree(GetProcessHeap(), 0, buf);
    return t42;
}




BOOL T42_download_glyph(print_ctx *ctx, DOWNLOAD *pdl, DWORD index,
			char *glyph_name)
{
    DWORD start, end, i;
    char *buf;
    TYPE42 *t42;

    static const char glyph_def[] =
      "/%s findfont exch 1 index\n"
      "havetype42gdir\n"
      "{/GlyphDirectory get begin %ld exch def end}\n"
      "{/sfnts get 4 index get 3 index 2 index putinterval pop}\n"
      "ifelse\n"
      "/CharStrings get\n"
      "begin\n"
      " /%s %ld def\n"
      "end\n"
      "pop pop\n";

    TRACE("%ld %s\n", index, glyph_name);
    assert(pdl->type == Type42);
    t42 = pdl->typeinfo.Type42;

    if(index < t42->glyph_sent_size) {
        if(t42->glyph_sent[index])
	    return TRUE;
    } else {
        t42->glyph_sent_size = (index / GLYPH_SENT_INC + 1) * GLYPH_SENT_INC;
	t42->glyph_sent = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				      t42->glyph_sent,
				      t42->glyph_sent_size * sizeof(*(t42->glyph_sent)));
    }

    if(!get_glyf_pos(t42, index, &start, &end)) return FALSE;
    TRACE("start = %lx end = %lx\n", start, end);

    if(GET_BE_WORD(t42->tables[t42->glyf_tab].data + start) == 0xffff) {
      /* Composite glyph */
        BYTE *sg_start = t42->tables[t42->glyf_tab].data + start + 10;
	DWORD sg_flags, sg_index;
	char sg_name[MAX_G_NAME + 1];

	do {
	    sg_flags = GET_BE_WORD(sg_start);
	    sg_index = GET_BE_WORD(sg_start + 2);

	    TRACE("Sending subglyph %04lx for glyph %04lx\n", sg_index, index);
	    get_glyph_name(ctx->hdc, sg_index, sg_name);
	    T42_download_glyph(ctx, pdl, sg_index, sg_name);
	    sg_start += 4;
	    if(sg_flags & ARG_1_AND_2_ARE_WORDS)
	        sg_start += 4;
	    else
	        sg_start += 2;
	    if(sg_flags & WE_HAVE_A_SCALE)
	        sg_start += 2;
	    else if(sg_flags & WE_HAVE_AN_X_AND_Y_SCALE)
	        sg_start += 4;
	    else if(sg_flags & WE_HAVE_A_TWO_BY_TWO)
	        sg_start += 8;
	} while(sg_flags & MORE_COMPONENTS);
    }

    for(i = 1; t42->glyf_blocks[i]; i++)
        if(start < t42->glyf_blocks[i]) break;

    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(glyph_def) +
		    strlen(pdl->ps_name) + 100);

    /* we don't have a string for the gdir and glyf tables, but we do have a 
       string for the TT header.  So the offset we need is tables - 2 */
    sprintf(buf, "%ld %ld\n", t42->num_of_written_tables - 2 + i, start - t42->glyf_blocks[i-1]);
    PSDRV_WriteSpool(ctx, buf, strlen(buf));

    PSDRV_WriteSpool(ctx, "<", 1);
    for(i = start; i < end; i++) {
        sprintf(buf, "%02x", *(t42->tables[t42->glyf_tab].data + i));
	PSDRV_WriteSpool(ctx, buf, strlen(buf));
	if((i - start) % 16 == 15)
	    PSDRV_WriteSpool(ctx, "\n", 1);
    }
    PSDRV_WriteSpool(ctx, ">\n", 2);
    sprintf(buf, glyph_def, pdl->ps_name, index, glyph_name, index);
    PSDRV_WriteSpool(ctx, buf, strlen(buf));

    t42->glyph_sent[index] = TRUE;
    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}

void T42_free(TYPE42 *t42)
{
    OTTable *table;
    for(table = t42->tables; table->MS_tag; table++)
        HeapFree(GetProcessHeap(), 0, table->data);
    HeapFree(GetProcessHeap(), 0, t42->glyph_sent);
    HeapFree(GetProcessHeap(), 0, t42->glyf_blocks);
    HeapFree(GetProcessHeap(), 0, t42);
    return;
}
