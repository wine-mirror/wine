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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winspool.h"

#include "psdrv.h"
#include "wine/debug.h"
#include "config.h"
#include "wine/port.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);


#define GET_BE_WORD(ptr)  MAKEWORD( ((BYTE *)(ptr))[1], ((BYTE *)(ptr))[0] )
#define GET_BE_DWORD(ptr) ((DWORD)MAKELONG( GET_BE_WORD(&((WORD *)(ptr))[1]), \
                                            GET_BE_WORD(&((WORD *)(ptr))[0]) ))

#define MS_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (DWORD)_x4 << 24 ) |     \
            ( (DWORD)_x3 << 16 ) |     \
            ( (DWORD)_x2 <<  8 ) |     \
              (DWORD)_x1         )

/* undef this to download the metrics in one go in the hmtx table.
   Most printers seem unable to use incremental metrics unfortunately */
#define USE_SEPARATE_METRICS
#undef USE_SEPARATE_METRICS

typedef struct {
  DWORD MS_tag;
  DWORD len, check;
  BYTE *data;
  BOOL write;
} OTTable;

const OTTable tables_templ[] = {
      { MS_MAKE_TAG('c','v','t',' '), 0, 0, NULL, TRUE },
      { MS_MAKE_TAG('f','p','g','m'), 0, 0, NULL, TRUE },
      { MS_MAKE_TAG('g','d','i','r'), 0, 0, NULL, TRUE },
      { MS_MAKE_TAG('g','l','y','f'), 0, 0, NULL, FALSE },
      { MS_MAKE_TAG('h','e','a','d'), 0, 0, NULL, TRUE },
      { MS_MAKE_TAG('h','h','e','a'), 0, 0, NULL, TRUE },
#ifdef USE_SEPARATE_METRICS
      { MS_MAKE_TAG('h','m','t','x'), 0, 0, NULL, FALSE },
#else
      { MS_MAKE_TAG('h','m','t','x'), 0, 0, NULL, TRUE },
#endif
      { MS_MAKE_TAG('l','o','c','a'), 0, 0, NULL, FALSE },
      { MS_MAKE_TAG('m','a','x','p'), 0, 0, NULL, TRUE },
      { MS_MAKE_TAG('p','r','e','p'), 0, 0, NULL, TRUE },
      { 0, 0, 0, NULL, 0 }
};

struct tagTYPE42 {
    OTTable tables[sizeof(tables_templ)/sizeof(tables_templ[0])];
    int glyf_tab, loca_tab, head_tab; /* indices of glyf, loca and head tables */
    int hmtx_tab;
    DWORD glyph_sent_size;
    BOOL *glyph_sent;
    DWORD emsize;
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
    int i;

    if(table->MS_tag == MS_MAKE_TAG('g','d','i','r')) return TRUE;
    table->len = GetFontData(hdc, table->MS_tag, 0, NULL, 0);
    table->data = HeapAlloc(GetProcessHeap(), 0, (table->len + 3) & ~3 );
    memset(table->data + ((table->len - 1) & ~3), 0, sizeof(DWORD));
    GetFontData(hdc, table->MS_tag, 0, table->data, table->len);
    table->check = 0;
    for(i = 0; i < (table->len + 3) / 4; i++)
        table->check += FLIP_ORDER(*((DWORD*)(table->data) + i));
    return TRUE;
}


TYPE42 *T42_download_header(PSDRV_PDEVICE *physDev, LPOUTLINETEXTMETRICA potm,
			    char *ps_name)
{
    DWORD i, j, tablepos;
    WORD num_of_tables = sizeof(tables_templ) / sizeof(tables_templ[0]) - 1;
    WORD num_of_write_tables = 0;
    char *buf;
    TYPE42 *t42;
    char start[] = /* name, fontbbox */
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
	    " /GlyphDirectory 256 dict def\n"
#ifdef USE_SEPARATE_METRICS
            " /Metrics 256 dict def\n"
#endif
	    " /sfnts [\n";
    char TT_offset_table[] = "<00010000%04x%04x%04x%04x\n";
    char TT_table_dir_entry[] = "%08lx%08lx%08lx%08lx\n";
    char end[] = "] def\n"
      "currentdict end dup /FontName get exch definefont pop\n";


    t42 = HeapAlloc(GetProcessHeap(), 0, sizeof(*t42));
    memcpy(t42->tables, tables_templ, sizeof(tables_templ));
    t42->loca_tab = t42->glyf_tab = t42->head_tab = t42->hmtx_tab = -1;
    t42->emsize = potm->otmEMSquare;

    for(i = 0; i < num_of_tables; i++) {
        LoadTable(physDev->hdc, t42->tables + i);
	if(t42->tables[i].len > 0xffff && t42->tables[i].write) break;
	if(t42->tables[i].write) num_of_write_tables++;
	if(t42->tables[i].MS_tag == MS_MAKE_TAG('l','o','c','a'))
	    t42->loca_tab = i;
	else if(t42->tables[i].MS_tag == MS_MAKE_TAG('g','l','y','f'))
	    t42->glyf_tab = i;
	else if(t42->tables[i].MS_tag == MS_MAKE_TAG('h','e','a','d'))
	    t42->head_tab = i;
	else if(t42->tables[i].MS_tag == MS_MAKE_TAG('h','m','t','x'))
	    t42->hmtx_tab = i;
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

    sprintf(buf, start, ps_name,
	    (float)potm->otmrcFontBox.left / potm->otmEMSquare,
	    (float)potm->otmrcFontBox.bottom / potm->otmEMSquare,
	    (float)potm->otmrcFontBox.right / potm->otmEMSquare,
	    (float)potm->otmrcFontBox.top / potm->otmEMSquare);

    PSDRV_WriteSpool(physDev, buf, strlen(buf));

    sprintf(buf, TT_offset_table, num_of_write_tables,
	    num_of_write_tables, num_of_write_tables, num_of_write_tables);

    PSDRV_WriteSpool(physDev, buf, strlen(buf));

    tablepos = 12 + num_of_write_tables * 16;
    for(i = 0; i < num_of_tables; i++) {
        if(!t42->tables[i].write) continue;
        sprintf(buf, TT_table_dir_entry, FLIP_ORDER(t42->tables[i].MS_tag),
		t42->tables[i].check, t42->tables[i].len ? tablepos : 0,
		t42->tables[i].len);
	PSDRV_WriteSpool(physDev, buf, strlen(buf));
	tablepos += ((t42->tables[i].len + 3) & ~3);
    }
    PSDRV_WriteSpool(physDev, ">\n", 2);

    for(i = 0; i < num_of_tables; i++) {
        if(t42->tables[i].len == 0 || !t42->tables[i].write) continue;
	PSDRV_WriteSpool(physDev, "<", 1);
	for(j = 0; j < ((t42->tables[i].len + 3) & ~3); j++) {
	    sprintf(buf, "%02x", t42->tables[i].data[j]);
	    PSDRV_WriteSpool(physDev, buf, strlen(buf));
	    if(j % 16 == 15) PSDRV_WriteSpool(physDev, "\n", 1);
	}
	PSDRV_WriteSpool(physDev, ">\n", 2);
    }

    PSDRV_WriteSpool(physDev, end, sizeof(end) - 1);
    HeapFree(GetProcessHeap(), 0, buf);
    return t42;
}




BOOL T42_download_glyph(PSDRV_PDEVICE *physDev, DOWNLOAD *pdl, DWORD index,
			char *glyph_name)
{
    DWORD start, end, i;
    char *buf;
    TYPE42 *t42;
    WORD loca_format;
    WORD awidth;
    short lsb;

#ifdef USE_SEPARATE_METRICS
    char glyph_with_Metrics_def[] = 
      "/%s findfont exch 1 index /GlyphDirectory get\n"
      "begin\n"
      " %d exch def\n"
      "end\n"
      "dup /CharStrings get\n"
      "begin\n"
      " /%s %d def\n"
      "end\n"
      "/Metrics get\n"
      "begin\n"
      " /%s [%f %f] def\n"
      "end\n";
#else
    char glyph_def[] = 
      "/%s findfont exch 1 index /GlyphDirectory get\n"
      "begin\n"
      " %d exch def\n"
      "end\n"
      "/CharStrings get\n"
      "begin\n"
      " /%s %d def\n"
      "end\n";
#endif

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

    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(glyph_def) +
		    strlen(pdl->ps_name) + 100);

    loca_format = GET_BE_WORD(t42->tables[t42->head_tab].data + 50);
    TRACE("loca_format = %d\n", loca_format);
    switch(loca_format) {
    case 0:
        start = GET_BE_WORD(((WORD*)t42->tables[t42->loca_tab].data) + index);
	start <<= 1;
	end = GET_BE_WORD(((WORD*)t42->tables[t42->loca_tab].data) + index + 1);
	end <<= 1;
	break;
    case 1:
        start = GET_BE_DWORD(((DWORD*)t42->tables[t42->loca_tab].data) + index);
	end = GET_BE_DWORD(((DWORD*)t42->tables[t42->loca_tab].data) + index + 1);
	break;
    default:
        ERR("Unknown loca_format %d\n", loca_format);
	return FALSE;
    }
    TRACE("start = %lx end = %lx\n", start, end);

    awidth = GET_BE_WORD(t42->tables[t42->hmtx_tab].data + index * 4);
    lsb = GET_BE_WORD(t42->tables[t42->hmtx_tab].data + index * 4 + 2);

    if(GET_BE_WORD(t42->tables[t42->glyf_tab].data + start) == 0xffff) {
      /* Composite glyph */
        char *sg_start = t42->tables[t42->glyf_tab].data + start + 10;
	DWORD sg_flags, sg_index;
	char sg_name[MAX_G_NAME + 1];

	do {
	    sg_flags = GET_BE_WORD(sg_start);
	    sg_index = GET_BE_WORD(sg_start + 2);

	    TRACE("Sending subglyph %04lx for glyph %04lx\n", sg_index, index);
	    get_glyph_name(physDev->hdc, sg_index, sg_name);
	    T42_download_glyph(physDev, pdl, sg_index, sg_name);
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

    sprintf(buf, "%%%%glyph %04lx\n", index);
    PSDRV_WriteSpool(physDev, buf, strlen(buf));
    PSDRV_WriteSpool(physDev, "<", 1);
    for(i = start; i < end; i++) {
        sprintf(buf, "%02x", *(t42->tables[t42->glyf_tab].data + i));
	PSDRV_WriteSpool(physDev, buf, strlen(buf));
	if((i - start) % 16 == 15)
	    PSDRV_WriteSpool(physDev, "\n", 1);
    }
    PSDRV_WriteSpool(physDev, ">\n", 2);
#if USE_SEPARATE_METRICS
    sprintf(buf, glyph_with_Metrics_def, pdl->ps_name, index, glyph_name, index,
	    glyph_name, (float)lsb / t42->emsize, (float)awidth / t42->emsize);
#else
    sprintf(buf, glyph_def, pdl->ps_name, index, glyph_name, index);
#endif
    PSDRV_WriteSpool(physDev, buf, strlen(buf));

    t42->glyph_sent[index] = TRUE;
    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}

void T42_free(TYPE42 *t42)
{
    OTTable *table;
    for(table = t42->tables; table->MS_tag; table++)
        if(table->data) HeapFree(GetProcessHeap(), 0, table->data);
    if(t42->glyph_sent) HeapFree(GetProcessHeap(), 0, t42->glyph_sent);
    HeapFree(GetProcessHeap(), 0, t42);
    return;
}
