/*
 * Create dynamic new structures of various types
 * and some utils in that trend.
 *
 * Copyright 1998 Bertho A. Stultiens
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wrc.h"
#include "newstruc.h"
#include "utils.h"
#include "parser.h"

#include "wingdi.h"	/* for BITMAPINFOHEADER */

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


/*
 * Convert bitmaps to proper endian
 */
static void convert_bitmap_swap_v3(BITMAPINFOHEADER *bih)
{
	bih->biSize		= BYTESWAP_DWORD(bih->biSize);
	bih->biWidth		= BYTESWAP_DWORD(bih->biWidth);
	bih->biHeight		= BYTESWAP_DWORD(bih->biHeight);
	bih->biPlanes		= BYTESWAP_WORD(bih->biPlanes);
	bih->biBitCount		= BYTESWAP_WORD(bih->biBitCount);
	bih->biCompression	= BYTESWAP_DWORD(bih->biCompression);
	bih->biSizeImage	= BYTESWAP_DWORD(bih->biSizeImage);
	bih->biXPelsPerMeter	= BYTESWAP_DWORD(bih->biXPelsPerMeter);
	bih->biYPelsPerMeter	= BYTESWAP_DWORD(bih->biYPelsPerMeter);
	bih->biClrUsed		= BYTESWAP_DWORD(bih->biClrUsed);
	bih->biClrImportant	= BYTESWAP_DWORD(bih->biClrImportant);
}

static void convert_bitmap_swap_v4(BITMAPV4HEADER *b4h)
{
	convert_bitmap_swap_v3((BITMAPINFOHEADER *)b4h);
	b4h->bV4RedMask		= BYTESWAP_DWORD(b4h->bV4RedMask);
	b4h->bV4GreenMask	= BYTESWAP_DWORD(b4h->bV4GreenMask);
	b4h->bV4BlueMask	= BYTESWAP_DWORD(b4h->bV4BlueMask);
	b4h->bV4AlphaMask	= BYTESWAP_DWORD(b4h->bV4AlphaMask);
	b4h->bV4CSType		= BYTESWAP_DWORD(b4h->bV4CSType);
	b4h->bV4EndPoints.ciexyzRed.ciexyzX	= BYTESWAP_DWORD(b4h->bV4EndPoints.ciexyzRed.ciexyzX);
	b4h->bV4EndPoints.ciexyzRed.ciexyzY	= BYTESWAP_DWORD(b4h->bV4EndPoints.ciexyzRed.ciexyzY);
	b4h->bV4EndPoints.ciexyzRed.ciexyzZ	= BYTESWAP_DWORD(b4h->bV4EndPoints.ciexyzRed.ciexyzZ);
	b4h->bV4EndPoints.ciexyzGreen.ciexyzX	= BYTESWAP_DWORD(b4h->bV4EndPoints.ciexyzGreen.ciexyzX);
	b4h->bV4EndPoints.ciexyzGreen.ciexyzY	= BYTESWAP_DWORD(b4h->bV4EndPoints.ciexyzGreen.ciexyzY);
	b4h->bV4EndPoints.ciexyzGreen.ciexyzZ	= BYTESWAP_DWORD(b4h->bV4EndPoints.ciexyzGreen.ciexyzZ);
	b4h->bV4EndPoints.ciexyzBlue.ciexyzX	= BYTESWAP_DWORD(b4h->bV4EndPoints.ciexyzBlue.ciexyzX);
	b4h->bV4EndPoints.ciexyzBlue.ciexyzY	= BYTESWAP_DWORD(b4h->bV4EndPoints.ciexyzBlue.ciexyzY);
	b4h->bV4EndPoints.ciexyzBlue.ciexyzZ	= BYTESWAP_DWORD(b4h->bV4EndPoints.ciexyzBlue.ciexyzZ);
	b4h->bV4GammaRed	= BYTESWAP_DWORD(b4h->bV4GammaRed);
	b4h->bV4GammaGreen	= BYTESWAP_DWORD(b4h->bV4GammaGreen);
	b4h->bV4GammaBlue	= BYTESWAP_DWORD(b4h->bV4GammaBlue);
}

#define FL_SIGBE	0x01
#define FL_SIZEBE	0x02
#define FL_V4		0x04
static int convert_bitmap(char *data, int size)
{
	BITMAPINFOHEADER *bih = (BITMAPINFOHEADER *)data;
	BITMAPV4HEADER *b4h = (BITMAPV4HEADER *)data;
	int type = 0;
#if BYTE_ORDER == BIG_ENDIAN
	DWORD bisizel = BYTESWAP_DWORD(sizeof(BITMAPINFOHEADER));
	DWORD b4sizel = BYTESWAP_DWORD(sizeof(BITMAPV4HEADER));
	DWORD bisizeb = sizeof(BITMAPINFOHEADER);
	DWORD b4sizeb = sizeof(BITMAPV4HEADER);
#else
	DWORD bisizel = sizeof(BITMAPINFOHEADER);
	DWORD b4sizel = sizeof(BITMAPV4HEADER);
	DWORD bisizeb = BYTESWAP_DWORD(sizeof(BITMAPINFOHEADER));
	DWORD b4sizeb = BYTESWAP_DWORD(sizeof(BITMAPV4HEADER));
#endif

	if(data[0] == 'B' && data[1] == 'M')
	{
		/* Little endian signature */
		bih = (BITMAPINFOHEADER *)(data + sizeof(BITMAPFILEHEADER));
		b4h = (BITMAPV4HEADER *)(data + sizeof(BITMAPFILEHEADER));
	}
	else if(data[0] == 'M' && data[1] == 'B')
	{
		type |= FL_SIGBE;	/* Big endian signature */
		bih = (BITMAPINFOHEADER *)(data + sizeof(BITMAPFILEHEADER));
		b4h = (BITMAPV4HEADER *)(data + sizeof(BITMAPFILEHEADER));
	}

	if(bih->biSize == bisizel)
	{
		/* Little endian */
	}
	else if(bih->biSize == b4sizel)
	{
		type |= FL_V4;
	}
	else if(bih->biSize == bisizeb)
	{
		type |= FL_SIZEBE;
	}
	else if(bih->biSize == b4sizeb)
	{
		type |= FL_SIZEBE | FL_V4;
	}
	else
		type = -1;

	switch(type)
	{
	default:
		break;
	case FL_SIZEBE:
	case FL_SIZEBE | FL_V4:
		yywarning("Bitmap v%c signature little-endian, but size big-endian", type & FL_V4 ? '4' : '3');
		break;
	case FL_SIGBE:
	case FL_SIGBE | FL_V4:
		yywarning("Bitmap v%c signature big-endian, but size little-endian", type & FL_V4 ? '4' : '3');
		break;
	case -1:
		yyerror("Invalid bitmap format");
		break;
	}

	switch(byteorder)
	{
#if BYTE_ORDER == BIG_ENDIAN
	default:
#endif
	case WRC_BO_BIG:
		if(!(type & FL_SIZEBE))
		{
			if(type & FL_V4)
				convert_bitmap_swap_v4(b4h);
			else
				convert_bitmap_swap_v3(bih);
		}
		break;
#if BYTE_ORDER == LITTLE_ENDIAN
	default:
#endif
	case WRC_BO_LITTLE:
		if(type & FL_SIZEBE)
		{
			if(type & FL_V4)
				convert_bitmap_swap_v4(b4h);
			else
				convert_bitmap_swap_v3(bih);
		}
		break;
	}

	if(size && (void *)data != (void *)bih)
	{
		/* We have the fileheader still attached, remove it */
		memmove(data, data+sizeof(BITMAPFILEHEADER), size - sizeof(BITMAPFILEHEADER));
		return sizeof(BITMAPFILEHEADER);
	}
	return 0;
}
#undef FL_SIGBE
#undef FL_SIZEBE
#undef FL_V4

/*
 * Cursor and icon splitter functions used when allocating
 * cursor- and icon-groups.
 */
typedef struct {
	language_t	lan;
	int		id;
} id_alloc_t;

static int get_new_id(id_alloc_t **list, int *n, language_t *lan)
{
	int i;
	assert(lan != NULL);
	assert(list != NULL);
	assert(n != NULL);

	if(!*list)
	{
		*list = (id_alloc_t *)xmalloc(sizeof(id_alloc_t));
		*n = 1;
		(*list)[0].lan = *lan;
		(*list)[0].id = 1;
		return 1;
	}

	for(i = 0; i < *n; i++)
	{
		if((*list)[i].lan.id == lan->id && (*list)[i].lan.sub == lan->sub)
			return ++((*list)[i].id);
	}

	*list = (id_alloc_t *)xrealloc(*list, sizeof(id_alloc_t) * (*n+1));
	(*list)[*n].lan = *lan;
	(*list)[*n].id = 1;
	*n += 1;
	return 1;
}

static int alloc_icon_id(language_t *lan)
{
	static id_alloc_t *idlist = NULL;
	static int nid = 0;

	return get_new_id(&idlist, &nid, lan);
}

static int alloc_cursor_id(language_t *lan)
{
	static id_alloc_t *idlist = NULL;
	static int nid = 0;

	return get_new_id(&idlist, &nid, lan);
}

static void split_icons(raw_data_t *rd, icon_group_t *icog, int *nico)
{
	int cnt;
	int i;
	icon_dir_entry_t *ide;
	icon_t *ico;
	icon_t *list = NULL;
	icon_header_t *ih = (icon_header_t *)rd->data;
	int swap = 0;

	/* FIXME: Distinguish between normal and animated icons (RIFF format) */
	if(ih->type == 1)
		swap = 0;
	else if(BYTESWAP_WORD(ih->type) == 1)
		swap = 1;
	else
		yyerror("Icon resource data has invalid type id %d", ih->type);

	cnt = swap ? BYTESWAP_WORD(ih->count) : ih->count;
	ide = (icon_dir_entry_t *)((char *)rd->data + sizeof(icon_header_t));
	for(i = 0; i < cnt; i++)
	{
		ico = new_icon();
		ico->id = alloc_icon_id(icog->lvc.language);
		ico->lvc.language = dup_language(icog->lvc.language);
		if(swap)
		{
			ide[i].offset = BYTESWAP_DWORD(ide[i].offset);
			ide[i].ressize= BYTESWAP_DWORD(ide[i].ressize);
		}
		if(ide[i].offset > rd->size
		|| ide[i].offset + ide[i].ressize > rd->size)
			yyerror("Icon resource data corrupt");
		ico->width = ide[i].width;
		ico->height = ide[i].height;
		ico->nclr = ide[i].nclr;
		ico->planes = swap ? BYTESWAP_WORD(ide[i].planes) : ide[i].planes;
		ico->bits = swap ? BYTESWAP_WORD(ide[i].bits) : ide[i].bits;
		convert_bitmap((char *)rd->data + ide[i].offset, 0);
		if(!ico->planes)
		{
			WORD planes;
			/* Argh! They did not fill out the resdir structure */
			planes = ((BITMAPINFOHEADER *)((char *)rd->data + ide[i].offset))->biPlanes;
			/* The bitmap is in destination byteorder. We want native for our structures */
			switch(byteorder)
			{
#if BYTE_ORDER == BIG_ENDIAN
			case WRC_BO_LITTLE:
#else
			case WRC_BO_BIG:
#endif
				ico->planes = BYTESWAP_WORD(planes);
				break;
			default:
				ico->planes = planes;
			}
		}
		if(!ico->bits)
		{
			WORD bits;
			/* Argh! They did not fill out the resdir structure */
			bits = ((BITMAPINFOHEADER *)((char *)rd->data + ide[i].offset))->biBitCount;
			/* The bitmap is in destination byteorder. We want native for our structures */
			switch(byteorder)
			{
#if BYTE_ORDER == BIG_ENDIAN
			case WRC_BO_LITTLE:
#else
			case WRC_BO_BIG:
#endif
				ico->bits = BYTESWAP_WORD(bits);
				break;
			default:
				ico->bits = bits;
			}
		}
		ico->data = new_raw_data();
		copy_raw_data(ico->data, rd, ide[i].offset, ide[i].ressize);
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
	cursor_dir_entry_t *cde;
	cursor_t *cur;
	cursor_t *list = NULL;
	cursor_header_t *ch = (cursor_header_t *)rd->data;
	int swap = 0;

	/* FIXME: Distinguish between normal and animated cursors (RIFF format)*/
	if(ch->type == 2)
		swap = 0;
	else if(BYTESWAP_WORD(ch->type) == 2)
		swap = 1;
	else
		yyerror("Cursor resource data has invalid type id %d", ch->type);
	cnt = swap ? BYTESWAP_WORD(ch->count) : ch->count;
	cde = (cursor_dir_entry_t *)((char *)rd->data + sizeof(cursor_header_t));
	for(i = 0; i < cnt; i++)
	{
		WORD planes;
		WORD bits;
		cur = new_cursor();
		cur->id = alloc_cursor_id(curg->lvc.language);
		cur->lvc.language = dup_language(curg->lvc.language);
		if(swap)
		{
			cde[i].offset = BYTESWAP_DWORD(cde[i].offset);
			cde[i].ressize= BYTESWAP_DWORD(cde[i].ressize);
		}
		if(cde[i].offset > rd->size
		|| cde[i].offset + cde[i].ressize > rd->size)
			yyerror("Cursor resource data corrupt");
		cur->width = cde[i].width;
		cur->height = cde[i].height;
		cur->nclr = cde[i].nclr;
		convert_bitmap((char *)rd->data + cde[i].offset, 0);
		/* The next two are to support color cursors */
		planes = ((BITMAPINFOHEADER *)((char *)rd->data + cde[i].offset))->biPlanes;
		bits = ((BITMAPINFOHEADER *)((char *)rd->data + cde[i].offset))->biBitCount;
		/* The bitmap is in destination byteorder. We want native for our structures */
		switch(byteorder)
		{
#if BYTE_ORDER == BIG_ENDIAN
		case WRC_BO_LITTLE:
#else
		case WRC_BO_BIG:
#endif
			cur->planes = BYTESWAP_WORD(planes);
			cur->bits = BYTESWAP_WORD(bits);
			break;
		default:
			cur->planes = planes;
			cur->bits = bits;
		}
		if(!win32 && (cur->planes != 1 || cur->bits != 1))
			yywarning("Win16 cursor contains colors");
		cur->xhot = swap ? BYTESWAP_WORD(cde[i].xhot) : cde[i].xhot;
		cur->yhot = swap ? BYTESWAP_WORD(cde[i].yhot) : cde[i].yhot;
		cur->data = new_raw_data();
		copy_raw_data(cur->data, rd, cde[i].offset, cde[i].ressize);
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
	rd->size -= convert_bitmap(rd->data, rd->size);
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

dlginit_t *new_dlginit(raw_data_t *rd, int *memopt)
{
	dlginit_t *di = (dlginit_t *)xmalloc(sizeof(dlginit_t));
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
	style_pair_t *sp = (style_pair_t *)xmalloc(sizeof(style_pair_t));
	sp->style = style;
	sp->exstyle = exstyle;
	return sp;
}

style_t *new_style(DWORD or_mask, DWORD and_mask)
{
	style_t *st = (style_t *)xmalloc(sizeof(style_t));
	st->or_mask = or_mask;
	st->and_mask = and_mask;
	return st;
}

