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
#include <ctype.h>

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
__NEW_STRUCT_FUNC(ani_any)

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
		fnt->memopt = WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE;
	return fnt;
}

fontdir_t *new_fontdir(raw_data_t *rd, int *memopt)
{
	fontdir_t *fnd = (fontdir_t *)xmalloc(sizeof(fontdir_t));
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
#ifdef WORDS_BIGENDIAN
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
#ifdef WORDS_BIGENDIAN
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
#ifndef WORDS_BIGENDIAN
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
		ico->lvc = icog->lvc;
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
#ifdef WORDS_BIGENDIAN
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
#ifdef WORDS_BIGENDIAN
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
		cur->lvc = curg->lvc;
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
#ifdef WORDS_BIGENDIAN
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
	icog->lvc = rd->lvc;
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
	curg->lvc = rd->lvc;
	split_cursors(rd, curg, &(curg->ncursor));
	free(rd->data);
	free(rd);
	return curg;
}

/*
 * Animated cursors and icons
 *
 * The format of animated cursors and icons is yet another example
 * of bad design by "The Company". The entire RIFF structure is
 * flawed by design because it is inconsistent and single minded:
 * - some tags have lengths attached, others don't. The use of these
 *   non-length tags is absolutely unclear;
 * - the content of "icon" tags can be both icons and cursors;
 * - tags lack proper alignment constraints. It seems that everything
 *   is 16bit aligned, but I could not find that in any docu. Just be
 *   prepared to eat anything;
 * - there are no strict constraints on tag-nesting and the organization
 *   is highly illogical;
 *
 * Anyhow, here is the basic structure:
 * "RIFF" { dword taglength }
 * 	"ACON"					// What does it do?
 *	"LIST" { dword taglength }
 *		"INFO"				// And what does this do?
 *		"INAM" { dword taglength }	// Icon/cursor name
 *			{inam data}
 *		"IART" { dword taglength }	// The artist
 *			{iart data}
 *		"fram"				// Is followed by "icon"s
 *		"icon" { dword taglength }	// First frame
 *			{ icon/cursor data }
 *		"icon" { dword taglength }	// Second frame
 *			{ icon/cursor data }
 *			...			// ...
 *	"anih" { dword taglength }		// Header structure
 *		{ aniheader_t structure }
 *	"rate" { dword taglength }		// The rate for each frame
 *		{ `steps' dwords }
 *	"seq " { dword taglength }		// The frame blit-order
 *		{ `steps' dwords }
 *
 * Tag length are bytelength without the header and length field (i.e. -8).
 * The "LIST" tag may occur several times and may encapsulate different
 * tags. The `steps' is the number of "icon" tags found (actually the
 * number of steps specified in the aniheader_t structure). The "seq "uence
 * tag can be ommitted, in which case the sequence is equal to the sequence
 * of "icon"s found in the file. Also "rate" may be ommitted, in which case
 * the default from the aniheader_t structure is used.
 *
 * An animated cursor puts `.cur' formatted files into each "icon" tag,
 * whereas animated icons contain `.ico' formatted files.
 *
 * Note about the code: Yes, it can be shorter/compressed. Some tags can be
 * dealt with in the same code. However, this version shows what is going on
 * and is better debug-able.
 */
static const char riff[4] = "RIFF";
static const char acon[4] = "ACON";
static const char list[4] = "LIST";
static const char info[4] = "INFO";
static const char inam[4] = "INAM";
static const char iart[4] = "IART";
static const char fram[4] = "fram";
static const char icon[4] = "icon";
static const char anih[4] = "anih";
static const char rate[4] = "rate";
static const char seq[4]  = "seq ";

#define NEXT_TAG(p)	((riff_tag_t *)(((char *)p) + (isswapped ? BYTESWAP_DWORD(p->size) : p->size) + sizeof(*p)))

static void handle_ani_icon(riff_tag_t *rtp, enum res_e type, int isswapped)
{
	cursor_dir_entry_t *cdp;
	cursor_header_t *chp;
	int count;
	int ctype;
	int i;
	static int once = 0;	/* This will trigger only once per file! */
	const char *anistr = type == res_aniico ? "icon" : "cursor";
	/* Notes:
	 * Both cursor and icon directories are similar
	 * Both cursor and icon headers are similar
	 */

	chp = (cursor_header_t *)(rtp+1);
	cdp = (cursor_dir_entry_t *)(chp+1);
	count = isswapped ? BYTESWAP_WORD(chp->count) : chp->count;
	ctype = isswapped ? BYTESWAP_WORD(chp->type) : chp->type;
	chp->reserved	= BYTESWAP_WORD(chp->reserved);
	chp->type	= BYTESWAP_WORD(chp->type);
	chp->count	= BYTESWAP_WORD(chp->count);

	if(type == res_anicur && ctype != 2 && !once)
	{
		yywarning("Animated cursor contains invalid \"icon\" tag cursor-file (%d->%s)",
				ctype,
				ctype == 1 ? "icontype" : "?");
		once++;
	}
	else if(type == res_aniico && ctype != 1 && !once)
	{
		yywarning("Animated icon contains invalid \"icon\" tag icon-file (%d->%s)",
				ctype,
				ctype == 2 ? "cursortype" : "?");
		once++;
	}
	else if(ctype != 1 && ctype != 2 && !once)
	{
		yywarning("Animated %s contains invalid \"icon\" tag file-type (%d; neither icon nor cursor)", anistr, ctype);
		once++;
	}

	for(i = 0; i < count; i++)
	{
		DWORD ofs = isswapped ? BYTESWAP_DWORD(cdp[i].offset) : cdp[i].offset;
		DWORD sze = isswapped ? BYTESWAP_DWORD(cdp[i].ressize) : cdp[i].ressize;
		if(ofs > rtp->size || ofs+sze > rtp->size)
			yyerror("Animated %s's data corrupt", anistr);
		convert_bitmap((char *)chp + ofs, 0);
		cdp[i].xhot	= BYTESWAP_WORD(cdp->xhot);
		cdp[i].yhot	= BYTESWAP_WORD(cdp->yhot);
		cdp[i].ressize	= BYTESWAP_DWORD(cdp->ressize);
		cdp[i].offset	= BYTESWAP_DWORD(cdp->offset);
	}
}

static void handle_ani_list(riff_tag_t *lst, enum res_e type, int isswapped)
{
	riff_tag_t *rtp = lst+1;	/* Skip the "LIST" tag */
	
	while((char *)rtp < (char *)lst + lst->size + sizeof(*lst))
	{
		if(!memcmp(rtp->tag, info, sizeof(info)))
		{
			rtp = (riff_tag_t *)(((char *)rtp) + 4);
		}
		else if(!memcmp(rtp->tag, inam, sizeof(inam)))
		{
			/* Ignore the icon/cursor name; its a string */
			rtp = NEXT_TAG(rtp);
		}
		else if(!memcmp(rtp->tag, iart, sizeof(iart)))
		{
			/* Ignore the author's name; its a string */
			rtp = NEXT_TAG(rtp);
		}
		else if(!memcmp(rtp->tag, fram, sizeof(fram)))
		{
			/* This should be followed by "icon"s, but we
			 * simply ignore this because it is pure
			 * non-information.
			 */
			rtp = (riff_tag_t *)(((char *)rtp) + 4);
		}
		else if(!memcmp(rtp->tag, icon, sizeof(icon)))
		{
			handle_ani_icon(rtp, type, isswapped);
			rtp = NEXT_TAG(rtp);
		}
		else
			internal_error(__FILE__, __LINE__, "Unknown tag \"%c%c%c%c\" in RIFF file",
				       isprint(rtp->tag[0]) ? rtp->tag[0] : '.',
				       isprint(rtp->tag[1]) ? rtp->tag[1] : '.',
				       isprint(rtp->tag[2]) ? rtp->tag[2] : '.',
				       isprint(rtp->tag[3]) ? rtp->tag[3] : '.');

		/* FIXME: This relies in sizeof(DWORD) == sizeof(pointer_type) */
		if((DWORD)rtp & 1)
			((char *)rtp)++;
	}
}

ani_curico_t *new_ani_curico(enum res_e type, raw_data_t *rd, int *memopt)
{
	ani_curico_t *ani = (ani_curico_t *)xmalloc(sizeof(ani_curico_t));
	riff_tag_t *rtp;
	int isswapped = 0;
	int doswap;
	const char *anistr = type == res_aniico ? "icon" : "cursor";

	assert(!memcmp(rd->data, riff, sizeof(riff)));
	assert(type == res_anicur || type == res_aniico);

	rtp = (riff_tag_t *)rd->data;

	if(BYTESWAP_DWORD(rtp->size) + 2*sizeof(DWORD) == rd->size)
		isswapped = 1;
	else if(rtp->size + 2*sizeof(DWORD) == rd->size)
		isswapped = 0;
	else
		yyerror("Animated %s has an invalid RIFF length", anistr);

	switch(byteorder)
	{
#ifdef WORDS_BIGENDIAN
	case WRC_BO_LITTLE:
#else
	case WRC_BO_BIG:
#endif
		doswap = !isswapped;
		break;
	default:
		doswap = isswapped;
	}

	/*
	 * When to swap what:
	 * isswapped | doswap |
	 * ----------+--------+---------------------------------
	 *     0     |    0   | read native; don't convert
	 *     1     |    0   | read swapped size; don't convert
	 *     0     |    1   | read native; convert
	 *     1     |    1   | read swapped size; convert
	 * Reading swapped size if necessary to calculate in native
	 * format. E.g. a little-endian source on a big-endian
	 * processor.
	 */
	if(doswap)
	{
		/* We only go through the RIFF file if we need to swap
		 * bytes in words/dwords. Else we couldn't care less
		 * what the file contains. This is consistent with
		 * MS' rc.exe, which doesn't complain at all, eventhough
		 * the fileformat might not be entirely correct.
		 */
		rtp++;	/* Skip the "RIFF" tag */

		while((char *)rtp < (char *)rd->data + rd->size)
		{
			if(!memcmp(rtp->tag, acon, sizeof(acon)))
			{
				rtp = (riff_tag_t *)(((char *)rtp) + 4);
			}
			else if(!memcmp(rtp->tag, list, sizeof(list)))
			{
				handle_ani_list(rtp, type, isswapped);
				rtp = NEXT_TAG(rtp);
			}
			else if(!memcmp(rtp->tag, anih, sizeof(anih)))
			{
				aniheader_t *ahp = (aniheader_t *)((char *)(rtp+1));
				ahp->structsize	= BYTESWAP_DWORD(ahp->structsize);
				ahp->frames	= BYTESWAP_DWORD(ahp->frames);
				ahp->steps	= BYTESWAP_DWORD(ahp->steps);
				ahp->cx		= BYTESWAP_DWORD(ahp->cx);
				ahp->cy		= BYTESWAP_DWORD(ahp->cy);
				ahp->bitcount	= BYTESWAP_DWORD(ahp->bitcount);
				ahp->planes	= BYTESWAP_DWORD(ahp->planes);
				ahp->rate	= BYTESWAP_DWORD(ahp->rate);
				ahp->flags	= BYTESWAP_DWORD(ahp->flags);
				rtp = NEXT_TAG(rtp);
			}
			else if(!memcmp(rtp->tag, rate, sizeof(rate)))
			{
				int cnt = rtp->size / sizeof(DWORD);
				DWORD *dwp = (DWORD *)(rtp+1);
				int i;
				for(i = 0; i < cnt; i++)
					dwp[i] = BYTESWAP_DWORD(dwp[i]);
				rtp = NEXT_TAG(rtp);
			}
			else if(!memcmp(rtp->tag, seq, sizeof(seq)))
			{
				int cnt = rtp->size / sizeof(DWORD);
				DWORD *dwp = (DWORD *)(rtp+1);
				int i;
				for(i = 0; i < cnt; i++)
					dwp[i] = BYTESWAP_DWORD(dwp[i]);
				rtp = NEXT_TAG(rtp);
			}
			else
				internal_error(__FILE__, __LINE__, "Unknown tag \"%c%c%c%c\" in RIFF file",
				       isprint(rtp->tag[0]) ? rtp->tag[0] : '.',
				       isprint(rtp->tag[1]) ? rtp->tag[1] : '.',
				       isprint(rtp->tag[2]) ? rtp->tag[2] : '.',
				       isprint(rtp->tag[3]) ? rtp->tag[3] : '.');

			/* FIXME: This relies in sizeof(DWORD) == sizeof(pointer_type) */
			if((DWORD)rtp & 1)
				((char *)rtp)++;
		}

		/* We must end correctly here */
		if((char *)rtp != (char *)rd->data + rd->size)
			yyerror("Animated %s contains invalid field size(s)", anistr);
	}

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
#undef NEXT_TAG

/* Bitmaps */
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

#define MSGTAB_BAD_PTR(p, b, l, r)	(((l) - ((char *)(p) - (char *)(b))) > (r))
messagetable_t *new_messagetable(raw_data_t *rd, int *memopt)
{
	messagetable_t *msg = (messagetable_t *)xmalloc(sizeof(messagetable_t));
	DWORD nblk;
	DWORD i;
	WORD lo;
	WORD hi;

	msg->data = rd;
	if(memopt)
	{
		msg->memopt = *memopt;
		free(memopt);
	}
	else
		msg->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;

	if(rd->size < sizeof(DWORD))
		yyerror("Invalid messagetable, size too small");

	nblk = *(DWORD *)rd->data;
	lo = WRC_LOWORD(nblk);
	hi = WRC_HIWORD(nblk);

	/* FIXME:
	 * This test will fail for all n*2^16 blocks in the messagetable.
	 * However, no sane person would want to have so many blocks
	 * and have a table of megabytes attached.
	 * So, I will assume that we have less than 2^16 blocks in the table
	 * and all will just work out fine. Otherwise, we would need to test
	 * the ID, offset and length (and flag) fields to be very sure.
	 */
	if(hi && lo)
		internal_error(__FILE__, __LINE__, "Messagetable contains more than 65535 blocks; cannot determine endian");
	if(!hi && !lo)
		yyerror("Invalid messagetable block count 0");

#ifdef WORDS_BIGENDIAN
	if(!hi && lo && byteorder != WRC_BO_LITTLE)
#else
	if(hi && !lo && byteorder != WRC_BO_BIG)
#endif
	{
		msgtab_block_t *mbp = (msgtab_block_t *)&(((DWORD *)rd->data)[1]);
		nblk = BYTESWAP_DWORD(nblk);
		if(MSGTAB_BAD_PTR(mbp, rd->data, rd->size, nblk * sizeof(*mbp)))
			yyerror("Messagetable's blocks are outside of defined data");
		for(i = 0; i < nblk; i++)
		{
			msgtab_entry_t *mep;
			DWORD id;

			mbp[i].idlo   = BYTESWAP_DWORD(mbp[i].idlo);
			mbp[i].idhi   = BYTESWAP_DWORD(mbp[i].idhi);
			mbp[i].offset = BYTESWAP_DWORD(mbp[i].offset);
			mep = (msgtab_entry_t *)(((char *)rd->data) + mbp[i].offset);
			for(id = mbp[i].idlo; id <= mbp[i].idhi; id++)
			{
				mep->length = BYTESWAP_WORD(mep->length);
				mep->flags  = BYTESWAP_WORD(mep->flags);
				if(MSGTAB_BAD_PTR(mep, rd->data, rd->size, mep->length))
					yyerror("Messagetable's data for block %d, ID 0x%08lx is outside of defined data", (int)i, id);
				if(mep->flags == 1)	/* Docu says 'flags == 0x0001' for unicode */
				{
					WORD *wp = (WORD *)&mep[1];
					int l = mep->length/2 - 2; /* Length included header */
					int n;

					if(mep->length & 1)
						yyerror("Message 0x%08lx is unicode (block %d), but has odd length (%d)", id, (int)i, mep->length);
					for(n = 0; n < l; n++)
						wp[n] = BYTESWAP_WORD(wp[n]);
						
				}
				mep = (msgtab_entry_t *)(((char *)mep) + mep->length);
			}
		}
	}

	return msg;
}
#undef MSGTAB_BAD_PTR

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

