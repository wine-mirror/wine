/*
 * General type definitions
 *
 * Copyright 1998 Bertho A. Stultiens (BS)
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

#ifndef __WRC_WRCTYPES_H
#define __WRC_WRCTYPES_H

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#ifndef VS_FFI_SIGNATURE
#include "winver.h"
#endif

/* Memory/load flags */
#define WRC_MO_MOVEABLE		0x0010
#define WRC_MO_PURE		0x0020
#define WRC_MO_PRELOAD		0x0040
#define WRC_MO_DISCARDABLE	0x1000

/* Resource type IDs */
#define WRC_RT_CURSOR		(1)
#define WRC_RT_BITMAP		(2)
#define WRC_RT_ICON		(3)
#define WRC_RT_MENU		(4)
#define WRC_RT_DIALOG		(5)
#define WRC_RT_STRING		(6)
#define WRC_RT_FONTDIR		(7)
#define WRC_RT_FONT		(8)
#define WRC_RT_ACCELERATOR	(9)
#define WRC_RT_RCDATA		(10)
#define WRC_RT_MESSAGETABLE	(11)
#define WRC_RT_GROUP_CURSOR	(12)
#define WRC_RT_GROUP_ICON	(14)
#define WRC_RT_VERSION		(16)
#define WRC_RT_DLGINCLUDE	(17)
#define WRC_RT_PLUGPLAY		(19)
#define WRC_RT_VXD		(20)
#define WRC_RT_ANICURSOR	(21)
#define WRC_RT_ANIICON		(22)
#define WRC_RT_HTML		(23)
#define WRC_RT_DLGINIT          (240)
#define WRC_RT_TOOLBAR		(241)

/* Default class type IDs */
#define CT_BUTTON	0x80
#define CT_EDIT		0x81
#define CT_STATIC 	0x82
#define CT_LISTBOX	0x83
#define CT_SCROLLBAR	0x84
#define CT_COMBOBOX	0x85

#define GET_WORD(ptr)  (((unsigned char *)(ptr))[0] | (((unsigned char *)(ptr))[1] << 8))
#define GET_DWORD(ptr) (((unsigned char *)(ptr))[0] | (((unsigned char *)(ptr))[1] << 8) | (((unsigned char *)(ptr))[2] << 16)  | (((unsigned char *)(ptr))[3] << 24))

typedef struct
{
    const char *file;
    int         line;
    int         col;
} location_t;

/* Resource strings are slightly more complex because they include '\0' */
enum str_e {str_char, str_unicode};

typedef struct string {
	int 		size;
	enum str_e	type;
	union {
		char *cstr;
		WCHAR *wstr;
	} str;
	location_t loc;
} string_t;

/* Resources are identified either by name or by number */
enum name_e {name_str, name_ord};

typedef struct name_id {
	union {
		string_t *s_name;
		int	i_name;
	} name;
	enum name_e type;
} name_id_t;

typedef unsigned int language_t;
typedef unsigned int characts_t;
typedef unsigned int version_t;

typedef struct lvc {
	language_t	language;
	version_t	version;
	characts_t	characts;
} lvc_t;

typedef struct font_id {
	string_t	*name;
	int		size;
	int		weight;
	int		italic;
} font_id_t;

/* control styles */
typedef struct style {
	unsigned int	or_mask;
	unsigned int	and_mask;
} style_t;

/* resource types */
/* These are in the same order (and ordinal) as the RT_xxx
 * defines. This is _required_.
 * I rolled my own numbers for the win32 extension that are
 * documented, but generate either old RT_xxx numbers, or
 * don't have an ordinal associated (user type).
 * I don't know any specs for those noted such, for that matter,
 * I don't even know whether they can be generated other than by
 * using a user-type resource.
 */
enum res_e {
	res_0 = 0,
	res_cur,
	res_bmp,
	res_ico,
	res_men,
	res_dlg,
	res_stt,
	res_fntdir,
	res_fnt,
	res_acc,
	res_rdt,
	res_msg,
	res_curg,
	res_13,         /* Hm, wonder why it's not used... */
	res_icog,
	res_15,
	res_ver,
	res_dlginc,	/* Not implemented, no layout available */
	res_18,
	res_pnp,	/* Not implemented, no layout available */
	res_vxd,	/* Not implemented, no layout available */
	res_anicur,
	res_aniico,
	res_html,	/* Not implemented, no layout available */

	res_dlginit = WRC_RT_DLGINIT,	/* 240 */
	res_toolbar = WRC_RT_TOOLBAR,	/* 241 */

	res_usr = 256 + 6
};

/* Raw bytes in a row... */
typedef struct raw_data {
	unsigned int	size;
	char		*data;
	lvc_t		lvc;		/* Localized data */
} raw_data_t;

/* Dialog structures */
typedef struct control {
	struct control	*next;		/* List of controls */
	struct control	*prev;
	name_id_t	*ctlclass;	/* ControlClass */
	name_id_t	*title;		/* Title of control */
	int		id;
	int		x;		/* Position */
	int		y;
	int		width;		/* Size */
	int		height;
	style_t		*style;		/* Style */
	style_t		*exstyle;
	unsigned int	helpid;		/* EX: */
	int		gotstyle;	/* Used to determine whether the default */
	int		gotexstyle;	/* styles must be set */
	int		gothelpid;
	raw_data_t	*extra;		/* EX: number of extra bytes in resource */
} control_t;

typedef struct dialog {
	unsigned int	memopt;
	int		x;		/* Position */
	int		y;
	int		width;		/* Size */
	int		height;
	style_t		*style;		/* Style */
	style_t		*exstyle;
	unsigned int	helpid;		/* EX: */
	int		gotstyle;	/* Used to determine whether the default */
	int		gotexstyle;	/* styles must be set */
	int		gothelpid;
	int		is_ex;
	name_id_t	*menu;
	name_id_t	*dlgclass;
	string_t	*title;
	font_id_t	*font;
	lvc_t		lvc;
	control_t	*controls;
} dialog_t;

/* Menu structures */
typedef struct menu_item {
	struct menu_item *next;
	struct menu_item *prev;
	struct menu_item *popup;
	int		id;
	unsigned int	type;
	unsigned int	state;
	int		helpid;
	string_t	*name;
	int		gotid;
	int		gottype;
	int		gotstate;
	int		gothelpid;
} menu_item_t;

typedef struct menu {
	unsigned int	memopt;
	lvc_t		lvc;
	int		is_ex;
	menu_item_t	*items;
} menu_t;

typedef struct itemex_opt
{
	int	id;
	unsigned int type;
	unsigned int state;
	int	helpid;
	int	gotid;
	int	gottype;
	int	gotstate;
	int	gothelpid;
} itemex_opt_t;

/*
 * Font resources
 */
typedef struct font {
	unsigned int	memopt;
	raw_data_t	*data;
} font_t;

typedef struct fontdir {
	unsigned int	memopt;
	raw_data_t	*data;
} fontdir_t;

/*
 * Icon resources
 */
typedef struct icon_header {
	unsigned short	reserved;	/* Don't know, should be 0 I guess */
	unsigned short	type;		/* Always 1 for icons */
	unsigned short	count;		/* Number of packed icons in resource */
} icon_header_t;

typedef struct icon_dir_entry {
	unsigned char	width;		/* From the SDK doc. */
	unsigned char	height;
	unsigned char	nclr;
	unsigned char	reserved;
	unsigned short	planes;
	unsigned short	bits;
	unsigned int	ressize;
	unsigned int	offset;
} icon_dir_entry_t;

typedef struct icon {
	struct icon	*next;
	struct icon	*prev;
	lvc_t		lvc;
	int		id;	/* Unique icon id within resource file */
	int		width;	/* Field from the IconDirEntry */
	int		height;
	int		nclr;
	int		planes;
	int		bits;
	raw_data_t	*data;
} icon_t;

typedef struct icon_group {
	unsigned int	memopt;
	lvc_t		lvc;
	icon_t		*iconlist;
	int		nicon;
} icon_group_t;

/*
 * Cursor resources
 */
typedef struct cursor_header {
	unsigned short	reserved;	/* Don't know, should be 0 I guess */
	unsigned short	type;		/* Always 2 for cursors */
	unsigned short	count;		/* Number of packed cursors in resource */
} cursor_header_t;

typedef struct cursor_dir_entry {
	unsigned char	width;		/* From the SDK doc. */
	unsigned char	height;
	unsigned char	nclr;
	unsigned char	reserved;
	unsigned short	xhot;
	unsigned short	yhot;
	unsigned int	ressize;
	unsigned int	offset;
} cursor_dir_entry_t;

typedef struct cursor {
	struct cursor	*next;
	struct cursor	*prev;
	lvc_t		lvc;
	int		id;	/* Unique icon id within resource file */
	int		width;	/* Field from the CursorDirEntry */
	int		height;
	int		nclr;
	int		planes;
	int		bits;
	int		xhot;
	int		yhot;
	raw_data_t	*data;
} cursor_t;

typedef struct cursor_group {
	unsigned int	memopt;
	lvc_t		lvc;
	cursor_t	*cursorlist;
	int		ncursor;
} cursor_group_t;

/*
 * Animated cursors and icons
 */
typedef struct aniheader {
	unsigned int	structsize;	/* Header size (36 bytes) */
	unsigned int	frames;		/* Number of unique icons in this cursor */
	unsigned int	steps;		/* Number of blits before the animation cycles */
	unsigned int	cx;		/* reserved, must be 0? */
	unsigned int	cy;		/* reserved, must be 0? */
	unsigned int	bitcount;	/* reserved, must be 0? */
	unsigned int	planes;		/* reserved, must be 0? */
	unsigned int	rate;		/* Default rate (1/60th of a second) if "rate" not present */
	unsigned int	flags;		/* Animation flag (1==AF_ICON, although both icons and cursors set this) */
} aniheader_t;

typedef struct riff_tag {
	unsigned char	tag[4];
	unsigned int	size;
} riff_tag_t;

typedef struct ani_curico {
	unsigned int	memopt;
	raw_data_t	*data;
} ani_curico_t;

typedef struct ani_any {
	enum res_e	type;
	union {
		ani_curico_t	*ani;
		cursor_group_t	*curg;
		icon_group_t	*icog;
	} u;
} ani_any_t;

/*
 * Bitmaps
 */
typedef struct bitmap {
	unsigned int	memopt;
	raw_data_t	*data;
} bitmap_t;

typedef struct html {
	unsigned int	memopt;
	raw_data_t	*data;
} html_t;

typedef struct rcdata {
	unsigned int	memopt;
	raw_data_t	*data;
} rcdata_t;

typedef struct {
	unsigned int	memopt;
	name_id_t	*type;
	raw_data_t	*data;
} user_t;

/*
 * Messagetables
 */
typedef struct msgtab_block {
	unsigned int	idlo;		/* Lowest id in the set */
	unsigned int	idhi;		/* Highest is in the set */
	unsigned int	offset;		/* Offset from resource start to first entry */
} msgtab_block_t;

typedef struct msgtab_entry {
	unsigned short	length;		/* Length of the data in bytes */
	unsigned short	flags;		/* 0 for char, 1 for WCHAR */
/*	{char}|{WCHAR} data[...]; */
} msgtab_entry_t;

typedef struct messagetable {
	unsigned int	memopt;
	raw_data_t	*data;
} messagetable_t;

/* StringTable structures */
typedef struct stt_entry {
	string_t		*str;
	int			id;
	unsigned int		memopt;
	characts_t		characts;
	version_t		version;
} stt_entry_t;

typedef struct stringtable {
	struct stringtable	*next;
	struct stringtable	*prev;
	unsigned int		memopt;
	lvc_t			lvc;
	int			idbase;
	int			nentries;
	stt_entry_t		*entries;
} stringtable_t;

/* VersionInfo structures */
enum ver_val_e {val_str, val_words, val_block};

struct ver_block;	/* Forward ref */

typedef struct ver_words {
	unsigned short	*words;
	int		nwords;
} ver_words_t;

typedef struct ver_value {
	struct ver_value	*next;
	struct ver_value	*prev;
	string_t		*key;
	union {
		string_t		*str;
		ver_words_t		*words;
		struct ver_block	*block;
	} value;
	enum ver_val_e		type;
} ver_value_t;

typedef struct ver_block {
	struct ver_block	*next;
	struct ver_block	*prev;
	string_t		*name;
	ver_value_t		*values;
} ver_block_t;

typedef struct versioninfo {
	int	filever_maj1;
	int	filever_maj2;
	int	filever_min1;
	int	filever_min2;
	int	prodver_maj1;
	int	prodver_maj2;
	int	prodver_min1;
	int	prodver_min2;
	int	fileos;
	int	fileflags;
	int	fileflagsmask;
	int	filetype;
	int	filesubtype;
	struct {
		unsigned fv:1;
		unsigned pv:1;
		unsigned fo:1;
		unsigned ff:1;
		unsigned ffm:1;
		unsigned ft:1;
		unsigned fst:1;
	} gotit;
	ver_block_t	*blocks;
	lvc_t		lvc;
	unsigned int	memopt;
} versioninfo_t;

/* Accelerator structures */
#define WRC_AF_VIRTKEY	0x0001
#define WRC_AF_NOINVERT	0x0002
#define WRC_AF_SHIFT	0x0004
#define WRC_AF_CONTROL	0x0008
#define WRC_AF_ALT	0x0010
#define WRC_AF_ASCII	0x4000

typedef struct event {
	struct event	*next;
	struct event	*prev;
	string_t	*str;
	int		flags;
	int		key;
	int		id;
} event_t;

typedef struct accelerator {
	unsigned int	memopt;
	lvc_t		lvc;
	event_t		*events;
} accelerator_t;

/* Toolbar structures */
typedef struct toolbar_item {
	struct toolbar_item	*next;
	struct toolbar_item	*prev;
	int			id;
} toolbar_item_t;

typedef struct toolbar {
	unsigned int	memopt;
	lvc_t		lvc;
	int		button_width;
	int		button_height;
	int		nitems;
	toolbar_item_t	*items;
} toolbar_t;

typedef struct dlginit {
	unsigned int	memopt;
	raw_data_t	*data;
} dlginit_t;


/* A top-level resource node */
typedef struct resource {
	struct resource	*next;
	struct resource *prev;
	enum res_e	type;
	name_id_t	*name;	/* resource's name */
	language_t	lan;	/* Only used as a sorting key and c-name creation*/
	union {
		accelerator_t	*acc;
		ani_curico_t	*ani;
		bitmap_t	*bmp;
		cursor_t	*cur;
		cursor_group_t	*curg;
		dialog_t	*dlg;
		dlginit_t       *dlgi;
		font_t		*fnt;
		fontdir_t	*fnd;
		icon_t		*ico;
		icon_group_t	*icog;
		menu_t		*men;
		messagetable_t	*msg;
		html_t		*html;
		rcdata_t	*rdt;
		stringtable_t	*stt;
		toolbar_t	*tbt;
		user_t		*usr;
		versioninfo_t	*ver;
		void		*overlay; /* To catch all types at once... */
	} res;
	unsigned int	memopt;
} resource_t;

typedef struct style_pair {
    	style_t			*style;
	style_t			*exstyle;
} style_pair_t;

#endif
