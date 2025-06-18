/*
 * Create dynamic new structures of various types
 * and some utils in that trend.
 *
 * Copyright 1998 Bertho A. Stultiens
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "../tools.h"
#include "wrc.h"
#include "newstruc.h"
#include "utils.h"
#include "parser.h"

#include "wingdi.h"	/* for BITMAPINFOHEADER */

#pragma pack(push,2)
typedef struct
{
    unsigned int   biSize;
    unsigned short biWidth;
    unsigned short biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
} BITMAPOS2HEADER;
#pragma pack(pop)

/* New instances for all types of structures */
/* Very inefficient (in size), but very functional :-]
 * Especially for type-checking.
 */

dialog_t *new_dialog(void)
{
    dialog_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

name_id_t *new_name_id(void)
{
    name_id_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

menu_t *new_menu(void)
{
    menu_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

menu_item_t *new_menu_item(void)
{
    menu_item_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

control_t *new_control(void)
{
    control_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

icon_t *new_icon(void)
{
    icon_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

cursor_t *new_cursor(void)
{
    cursor_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

versioninfo_t *new_versioninfo(void)
{
    versioninfo_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

ver_value_t *new_ver_value(void)
{
    ver_value_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

ver_block_t *new_ver_block(void)
{
    ver_block_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

stt_entry_t *new_stt_entry(void)
{
    stt_entry_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

accelerator_t *new_accelerator(void)
{
    accelerator_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

event_t *new_event(void)
{
    event_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

raw_data_t *new_raw_data(void)
{
    raw_data_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

lvc_t *new_lvc(void)
{
    lvc_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

string_t *new_string(void)
{
    string_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    set_location( &ret->loc );
    return ret;
}

toolbar_item_t *new_toolbar_item(void)
{
    toolbar_item_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

ani_any_t *new_ani_any(void)
{
    ani_any_t *ret = xmalloc( sizeof(*ret) );
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

resource_t *new_resource(enum res_e t, void *res, int memopt, language_t lan)
{
	resource_t *r = xmalloc(sizeof(resource_t));
	memset( r, 0, sizeof(*r) );
	r->type = t;
	r->res.overlay = res;
	r->memopt = memopt;
	r->lan = lan;
	return r;
}

html_t *new_html(raw_data_t *rd, int *memopt)
{
	html_t *html = xmalloc(sizeof(html_t));
	html->data = rd;
	if(memopt)
	{
		html->memopt = *memopt;
		free(memopt);
	}
	else
		html->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
	return html;
}

rcdata_t *new_rcdata(raw_data_t *rd, int *memopt)
{
	rcdata_t *rc = xmalloc(sizeof(rcdata_t));
	rc->data = rd;
	if(memopt)
	{
		rc->memopt = *memopt;
		free(memopt);
	}
	else
		rc->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
	return rc;
}

font_id_t *new_font_id(int size, string_t *face, int weight, int italic)
{
	font_id_t *fid = xmalloc(sizeof(font_id_t));
	fid->name = face;
	fid->size = size;
	fid->weight = weight;
	fid->italic = italic;
	return fid;
}

user_t *new_user(name_id_t *type, raw_data_t *rd, int *memopt)
{
	user_t *usr = xmalloc(sizeof(user_t));
	usr->data = rd;
	if(memopt)
	{
		usr->memopt = *memopt;
		free(memopt);
	}
	else
		usr->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
	usr->type = type;
	return usr;
}

font_t *new_font(raw_data_t *rd, int *memopt)
{
	font_t *fnt = xmalloc(sizeof(font_t));
	fnt->data = rd;
	if(memopt)
	{
		fnt->memopt = *memopt;
		free(memopt);
	}
	else
		fnt->memopt = WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE;
	return fnt;
}

fontdir_t *new_fontdir(raw_data_t *rd, int *memopt)
{
	fontdir_t *fnd = xmalloc(sizeof(fontdir_t));
	fnd->data = rd;
	if(memopt)
	{
		fnd->memopt = *memopt;
		free(memopt);
	}
	else
		fnd->memopt = WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE;
	return fnd;
}


static int convert_bitmap(char *data, int size)
{
    if (size > sizeof(BITMAPFILEHEADER) && data[0] == 'B' && data[1] == 'M')
    {
         memmove(data, data+sizeof(BITMAPFILEHEADER), size - sizeof(BITMAPFILEHEADER));
         return sizeof(BITMAPFILEHEADER);
    }
    return 0;
}

/*
 * Cursor and icon splitter functions used when allocating
 * cursor- and icon-groups.
 */
typedef struct {
	language_t	lan;
	int		id;
} id_alloc_t;

static int get_new_id(id_alloc_t **list, int *n, language_t lan)
{
	int i;
	assert(list != NULL);
	assert(n != NULL);

	if(!*list)
	{
		*list = xmalloc(sizeof(id_alloc_t));
		*n = 1;
		(*list)[0].lan = lan;
		(*list)[0].id = 1;
		return 1;
	}

	for(i = 0; i < *n; i++)
	{
		if((*list)[i].lan == lan)
			return ++((*list)[i].id);
	}

	*list = xrealloc(*list, sizeof(id_alloc_t) * (*n+1));
	(*list)[*n].lan = lan;
	(*list)[*n].id = 1;
	*n += 1;
	return 1;
}

static int alloc_icon_id(language_t lan)
{
	static id_alloc_t *idlist = NULL;
	static int nid = 0;

	return get_new_id(&idlist, &nid, lan);
}

static int alloc_cursor_id(language_t lan)
{
	static id_alloc_t *idlist = NULL;
	static int nid = 0;

	return get_new_id(&idlist, &nid, lan);
}

static void split_icons(raw_data_t *rd, icon_group_t *icog, int *nico)
{
	int cnt;
	int i;
	icon_t *ico;
	icon_t *list = NULL;
	icon_header_t *ih = (icon_header_t *)rd->data;

	if(GET_WORD(&ih->type) != 1)
		parser_error("Icon resource data has invalid type id %d", ih->type);

	cnt = GET_WORD(&ih->count);
	for(i = 0; i < cnt; i++)
	{
		icon_dir_entry_t ide;
                int offset, size;
		BITMAPINFOHEADER info;
		memcpy(&ide, rd->data + sizeof(icon_header_t)
                                      + i*sizeof(icon_dir_entry_t), sizeof(ide));

		ico = new_icon();
		ico->id = alloc_icon_id(icog->lvc.language);
		ico->lvc = icog->lvc;
                offset = GET_DWORD(&ide.offset);
                size = GET_DWORD(&ide.ressize);
		if(offset > rd->size || offset + size > rd->size)
			parser_error("Icon resource data corrupt");
		ico->width = ide.width;
		ico->height = ide.height;
		ico->nclr = ide.nclr;
		ico->planes = GET_WORD(&ide.planes);
		ico->bits = GET_WORD(&ide.bits);
		memcpy(&info, rd->data + offset, sizeof(info));
                convert_bitmap((char *) &info, 0);
                memcpy(rd->data + offset, &info, sizeof(info));
		if(!ico->planes) ico->planes = GET_WORD(&info.biPlanes);
		if(!ico->bits) ico->bits = GET_WORD(&info.biBitCount);
		ico->data = new_raw_data();
		copy_raw_data(ico->data, rd, offset, size);
		if(!list)
		{
			list = ico;
		}
		else
		{
			ico->next = list;
			list->prev = ico;
			list = ico;
		}
	}
	icog->iconlist = list;
	*nico = cnt;
}

static void split_cursors(raw_data_t *rd, cursor_group_t *curg, int *ncur)
{
	int cnt;
	int i;
	cursor_t *cur;
	cursor_t *list = NULL;
	cursor_header_t *ch = (cursor_header_t *)rd->data;

	if(GET_WORD(&ch->type) != 2) parser_error("Cursor resource data has invalid type id %d", ch->type);
	cnt = GET_WORD(&ch->count);
	for(i = 0; i < cnt; i++)
	{
		cursor_dir_entry_t cde;
                int offset, size;
		BITMAPINFOHEADER info;
		memcpy(&cde, rd->data + sizeof(cursor_header_t)
                                      + i*sizeof(cursor_dir_entry_t), sizeof(cde));

		cur = new_cursor();
		cur->id = alloc_cursor_id(curg->lvc.language);
		cur->lvc = curg->lvc;
                offset = GET_DWORD(&cde.offset);
                size = GET_DWORD(&cde.ressize);
		if(offset > rd->size || offset + size > rd->size)
			parser_error("Cursor resource data corrupt");
		cur->width = cde.width;
		cur->height = cde.height;
		cur->nclr = cde.nclr;
		memcpy(&info, rd->data + offset, sizeof(info));
		convert_bitmap((char *)&info, 0);
		memcpy(rd->data + offset, &info, sizeof(info));
		cur->planes = GET_WORD(&info.biPlanes);
		cur->bits = GET_WORD(&info.biBitCount);
		if(!win32 && (cur->planes != 1 || cur->bits != 1))
			parser_warning("Win16 cursor contains colors\n");
		cur->xhot = GET_WORD(&cde.xhot);
		cur->yhot = GET_WORD(&cde.yhot);
		cur->data = new_raw_data();
		copy_raw_data(cur->data, rd, offset, size);
		if(!list)
		{
			list = cur;
		}
		else
		{
			cur->next = list;
			list->prev = cur;
			list = cur;
		}
	}
	curg->cursorlist = list;
	*ncur = cnt;
}


icon_group_t *new_icon_group(raw_data_t *rd, int *memopt)
{
	icon_group_t *icog = xmalloc(sizeof(icon_group_t));
	if(memopt)
	{
		icog->memopt = *memopt;
		free(memopt);
	}
	else
		icog->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
	icog->lvc = rd->lvc;
	split_icons(rd, icog, &(icog->nicon));
	free(rd->data);
	free(rd);
	return icog;
}

cursor_group_t *new_cursor_group(raw_data_t *rd, int *memopt)
{
	cursor_group_t *curg = xmalloc(sizeof(cursor_group_t));
	if(memopt)
	{
		curg->memopt = *memopt;
		free(memopt);
	}
	else
		curg->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
	curg->lvc = rd->lvc;
	split_cursors(rd, curg, &(curg->ncursor));
	free(rd->data);
	free(rd);
	return curg;
}

ani_curico_t *new_ani_curico(enum res_e type, raw_data_t *rd, int *memopt)
{
	ani_curico_t *ani = xmalloc(sizeof(ani_curico_t));

	assert(type == res_anicur || type == res_aniico);

	ani->data = rd;
	if(memopt)
	{
		ani->memopt = *memopt;
		free(memopt);
	}
	else
		ani->memopt = WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE;
	return ani;
}

/* Bitmaps */
bitmap_t *new_bitmap(raw_data_t *rd, int *memopt)
{
	bitmap_t *bmp = xmalloc(sizeof(bitmap_t));

	bmp->data = rd;
	if(memopt)
	{
		bmp->memopt = *memopt;
		free(memopt);
	}
	else
		bmp->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
	rd->size -= convert_bitmap(rd->data, rd->size);
	return bmp;
}

ver_words_t *new_ver_words(int i)
{
	ver_words_t *w = xmalloc(sizeof(ver_words_t));
	w->words = xmalloc(sizeof(unsigned short));
	w->words[0] = i;
	w->nwords = 1;
	return w;
}

ver_words_t *add_ver_words(ver_words_t *w, int i)
{
	w->words = xrealloc(w->words, (w->nwords+1) * sizeof(unsigned short));
	w->words[w->nwords] = i;
	w->nwords++;
	return w;
}

#define MSGTAB_BAD_PTR(p, b, l, r)	(((l) - ((char *)(p) - (char *)(b))) > (r))
messagetable_t *new_messagetable(raw_data_t *rd, int *memopt)
{
	messagetable_t *msg = xmalloc(sizeof(messagetable_t));

	msg->data = rd;
	if(memopt)
	{
		msg->memopt = *memopt;
		free(memopt);
	}
	else
		msg->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;

	if(rd->size < sizeof(unsigned int))
		parser_error("Invalid messagetable, size too small");

	return msg;
}
#undef MSGTAB_BAD_PTR

void copy_raw_data(raw_data_t *dst, raw_data_t *src, unsigned int offs, int len)
{
	assert(offs <= src->size);
	assert(offs + len <= src->size);
	if(!dst->data)
	{
		dst->data = xmalloc(len);
		dst->size = 0;
	}
	else
		dst->data = xrealloc(dst->data, dst->size + len);
	/* dst->size holds the offset to copy to */
	memcpy(dst->data + dst->size, src->data + offs, len);
	dst->size += len;
}

int *new_int(int i)
{
	int *ip = xmalloc(sizeof(int));
	*ip = i;
	return ip;
}

stringtable_t *new_stringtable(lvc_t *lvc)
{
	stringtable_t *stt = xmalloc(sizeof(stringtable_t));

	memset( stt, 0, sizeof(*stt) );
	if(lvc)
		stt->lvc = *lvc;

	return stt;
}

toolbar_t *new_toolbar(int button_width, int button_height, toolbar_item_t *items, int nitems)
{
	toolbar_t *tb = xmalloc(sizeof(toolbar_t));
	memset( tb, 0, sizeof(*tb) );
	tb->button_width = button_width;
	tb->button_height = button_height;
	tb->nitems = nitems;
	tb->items = items;
	return tb;
}

dlginit_t *new_dlginit(raw_data_t *rd, int *memopt)
{
	dlginit_t *di = xmalloc(sizeof(dlginit_t));
	di->data = rd;
	if(memopt)
	{
		di->memopt = *memopt;
		free(memopt);
	}
	else
		di->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;

	return di;
}

style_pair_t *new_style_pair(style_t *style, style_t *exstyle)
{
	style_pair_t *sp = xmalloc(sizeof(style_pair_t));
	sp->style = style;
	sp->exstyle = exstyle;
	return sp;
}

style_t *new_style(unsigned int or_mask, unsigned int and_mask)
{
	style_t *st = xmalloc(sizeof(style_t));
	st->or_mask = or_mask;
	st->and_mask = and_mask;
	return st;
}
