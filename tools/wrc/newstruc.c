/*
 * Create dynamic new structures of various types
 * and some utils in that trend.
 *
 * Copyright 1998 Bertho A. Stultiens
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <config.h>
#include "wrc.h"
#include "newstruc.h"
#include "utils.h"
#include "parser.h"

/* Generate new_* functions that have no parameters (NOTE: no ';') */
__NEW_STRUCT_FUNC(dialog)
__NEW_STRUCT_FUNC(dialogex)
__NEW_STRUCT_FUNC(name_id)
__NEW_STRUCT_FUNC(menu)
__NEW_STRUCT_FUNC(menuex)
__NEW_STRUCT_FUNC(menu_item)
__NEW_STRUCT_FUNC(menuex_item)
__NEW_STRUCT_FUNC(control)
__NEW_STRUCT_FUNC(icon)
__NEW_STRUCT_FUNC(cursor)
__NEW_STRUCT_FUNC(versioninfo)
__NEW_STRUCT_FUNC(ver_value)
__NEW_STRUCT_FUNC(ver_block)
__NEW_STRUCT_FUNC(stt_entry)
__NEW_STRUCT_FUNC(accelerator)
__NEW_STRUCT_FUNC(event)
__NEW_STRUCT_FUNC(raw_data)
__NEW_STRUCT_FUNC(lvc)
__NEW_STRUCT_FUNC(res_count)
__NEW_STRUCT_FUNC(string)
__NEW_STRUCT_FUNC(toolbar_item)

/* New instances for all types of structures */
/* Very inefficient (in size), but very functional :-]
 * Especially for type-checking.
 */
resource_t *new_resource(enum res_e t, void *res, int memopt, language_t *lan)
{
	resource_t *r = (resource_t *)xmalloc(sizeof(resource_t));
	r->type = t;
	r->res.overlay = res;
	r->memopt = memopt;
	r->lan = lan;
	return r;
}

version_t *new_version(DWORD v)
{
	version_t *vp = (version_t *)xmalloc(sizeof(version_t));
	*vp = v;
	return vp;
}

characts_t *new_characts(DWORD c)
{
	characts_t *cp = (characts_t *)xmalloc(sizeof(characts_t));
	*cp = c;
	return cp;
}

language_t *new_language(int id, int sub)
{
	language_t *lan = (language_t *)xmalloc(sizeof(language_t));
	lan->id = id;
	lan->sub = sub;
	return lan;
}

language_t *dup_language(language_t *l)
{
	if(!l) return NULL;
	return new_language(l->id, l->sub);
}

version_t *dup_version(version_t *v)
{
	if(!v) return NULL;
	return new_version(*v);
}

characts_t *dup_characts(characts_t *c)
{
	if(!c) return NULL;
	return new_characts(*c);
}

rcdata_t *new_rcdata(raw_data_t *rd, int *memopt)
{
	rcdata_t *rc = (rcdata_t *)xmalloc(sizeof(rcdata_t));
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
	font_id_t *fid = (font_id_t *)xmalloc(sizeof(font_id_t));
	fid->name = face;
	fid->size = size;
	fid->weight = weight;
	fid->italic = italic;
	return fid;
}

user_t *new_user(name_id_t *type, raw_data_t *rd, int *memopt)
{
	user_t *usr = (user_t *)xmalloc(sizeof(user_t));
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
	font_t *fnt = (font_t *)xmalloc(sizeof(font_t));
	fnt->data = rd;
	if(memopt)
	{
		fnt->memopt = *memopt;
		free(memopt);
	}
	else
		fnt->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
	return fnt;
}

icon_group_t *new_icon_group(raw_data_t *rd, int *memopt)
{
	icon_group_t *icog = (icon_group_t *)xmalloc(sizeof(icon_group_t));
	if(memopt)
	{
		icog->memopt = *memopt;
		free(memopt);
	}
	else
		icog->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
	icog->lvc.language = dup_language(currentlanguage);
	split_icons(rd, icog, &(icog->nicon));
	free(rd->data);
	free(rd);
	return icog;
}

cursor_group_t *new_cursor_group(raw_data_t *rd, int *memopt)
{
	cursor_group_t *curg = (cursor_group_t *)xmalloc(sizeof(cursor_group_t));
	if(memopt)
	{
		curg->memopt = *memopt;
		free(memopt);
	}
	else
		curg->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
	curg->lvc.language = dup_language(currentlanguage);
	split_cursors(rd, curg, &(curg->ncursor));
	free(rd->data);
	free(rd);
	return curg;
}

bitmap_t *new_bitmap(raw_data_t *rd, int *memopt)
{
	bitmap_t *bmp = (bitmap_t *)xmalloc(sizeof(bitmap_t));
	bmp->data = rd;
	if(memopt)
	{
		bmp->memopt = *memopt;
		free(memopt);
	}
	else
		bmp->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
	return bmp;
}

ver_words_t *new_ver_words(int i)
{
	ver_words_t *w = (ver_words_t *)xmalloc(sizeof(ver_words_t));
	w->words = (WORD *)xmalloc(sizeof(WORD));
	w->words[0] = (WORD)i;
	w->nwords = 1;
	return w;
}

ver_words_t *add_ver_words(ver_words_t *w, int i)
{
	w->words = (WORD *)xrealloc(w->words, (w->nwords+1) * sizeof(WORD));
	w->words[w->nwords] = (WORD)i;
	w->nwords++;
	return w;
}

messagetable_t *new_messagetable(raw_data_t *rd)
{
	messagetable_t *msg = (messagetable_t *)xmalloc(sizeof(messagetable_t));
	msg->data = rd;
	return msg;
}

void copy_raw_data(raw_data_t *dst, raw_data_t *src, int offs, int len)
{
	assert(offs <= src->size);
	assert(offs + len <= src->size);
	if(!dst->data)
	{
		dst->data = (char *)xmalloc(len);
		dst->size = 0;
	}
	else
		dst->data = (char *)xrealloc(dst->data, dst->size + len);
	/* dst->size holds the offset to copy to */
	memcpy(dst->data + dst->size, src->data + offs, len);
	dst->size += len;
}

int *new_int(int i)
{
	int *ip = (int *)xmalloc(sizeof(int));
	*ip = i;
	return ip;
}

stringtable_t *new_stringtable(lvc_t *lvc)
{
	stringtable_t *stt = (stringtable_t *)xmalloc(sizeof(stringtable_t));

	if(lvc)
		stt->lvc = *lvc;

	return stt;
}

toolbar_t *new_toolbar(int button_width, int button_height, toolbar_item_t *items, int nitems)
{
	toolbar_t *tb = (toolbar_t *)xmalloc(sizeof(toolbar_t));
	tb->button_width = button_width;
	tb->button_height = button_height;
	tb->nitems = nitems;
	tb->items = items;
	return tb;
}
