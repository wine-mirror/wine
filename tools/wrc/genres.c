/*
 * Generate .res format from a resource-tree
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
 *
 * History:
 * 05-May-2000 BS	- Added code to support endian conversions. The
 * 			  extra functions also aid unaligned access, but
 * 			  this is not yet implemented.
 * 25-May-1998 BS	- Added simple unicode -> char conversion for resource
 *			  names in .s and .h files.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "../tools.h"
#include "wrc.h"
#include "utils.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

unsigned char *output_buffer = NULL;
size_t output_buffer_pos = 0;
size_t output_buffer_size = 0;

static void set_word(int ofs, unsigned int w)
{
    output_buffer[ofs+0] = w;
    output_buffer[ofs+1] = w >> 8;
}

static void set_dword(int ofs, unsigned int d)
{
    output_buffer[ofs+0] = d;
    output_buffer[ofs+1] = d >> 8;
    output_buffer[ofs+2] = d >> 16;
    output_buffer[ofs+3] = d >> 24;
}

static unsigned int get_dword(int ofs)
{
    return (output_buffer[ofs+3] << 24)
         | (output_buffer[ofs+2] << 16)
         | (output_buffer[ofs+1] <<  8)
         |  output_buffer[ofs+0];
}

static void set_res_size(int tag)
{
    set_dword( tag, output_buffer_pos - tag - get_dword(tag) );
}

/*
 *****************************************************************************
 * Function	: string_to_upper
 * Syntax	: void string_to_upper(string_t *str)
 *****************************************************************************
*/
static void string_to_upper(string_t *str)
{
    int i;

    switch (str->type)
    {
    case str_char:
        for (i = 0; i < str->size; i++)
            if (str->str.cstr[i] >= 'a' && str->str.cstr[i] <= 'z') str->str.cstr[i] -= 32;
        break;
    case str_unicode:
        for (i = 0; i < str->size; i++)
            if (str->str.wstr[i] >= 'a' && str->str.wstr[i] <= 'z') str->str.wstr[i] -= 32;
        break;
    }
}

static int parse_accel_string( const string_t *key, int flags )
{
    int keycode = 0;

    switch (key->type)
    {
    case str_char:
        if (key->str.cstr[0] == '#') return 0;  /* ignore message contexts */
	if((flags & WRC_AF_VIRTKEY) &&
           !((key->str.cstr[0] >= 'A' && key->str.cstr[0] <= 'Z') ||
             (key->str.cstr[0] >= '0' && key->str.cstr[0] <= '9')))
        {
            print_location( &key->loc );
            error("VIRTKEY code is not equal to ascii value\n");
        }

	if(key->str.cstr[0] == '^' && (flags & WRC_AF_CONTROL) != 0)
	{
            print_location( &key->loc );
            error("Cannot use both '^' and CONTROL modifier\n");
	}
	else if(key->str.cstr[0] == '^')
	{
            if (key->str.cstr[1] >= 'a' && key->str.cstr[1] <= 'z')
                keycode = key->str.cstr[1] - 'a' + 1;
            else if (key->str.cstr[1] >= 'A' && key->str.cstr[1] <= 'Z')
                keycode = key->str.cstr[1] - 'A' + 1;
            else
            {
                print_location( &key->loc );
                error("Control-code out of range\n");
            }
	}
	else
            keycode = key->str.cstr[0];
        break;

    case str_unicode:
        if (key->str.wstr[0] == '#') return 0;  /* ignore message contexts */
	if((flags & WRC_AF_VIRTKEY) &&
           !((key->str.wstr[0] >= 'A' && key->str.wstr[0] <= 'Z') ||
             (key->str.wstr[0] >= '0' && key->str.wstr[0] <= '9')))
        {
            print_location( &key->loc );
            error("VIRTKEY code is not equal to ascii value\n");
        }
	if(key->str.wstr[0] == '^' && (flags & WRC_AF_CONTROL) != 0)
	{
            print_location( &key->loc );
            error("Cannot use both '^' and CONTROL modifier\n");
	}
	else if(key->str.wstr[0] == '^')
	{
            if (key->str.wstr[1] >= 'a' && key->str.wstr[1] <= 'z')
                keycode = key->str.wstr[1] - 'a' + 1;
            else if (key->str.wstr[1] >= 'A' && key->str.wstr[1] <= 'Z')
                keycode = key->str.wstr[1] - 'A' + 1;
            else
            {
                print_location( &key->loc );
                error("Control-code out of range\n");
            }
	}
	else
            keycode = key->str.wstr[0];
        break;
    }
    return keycode;
}

/*
 *****************************************************************************
 * Function	: put_string
 * Input	:
 *	str	- String to put
 *	isterm	- The string is '\0' terminated (disregard the string's
 *		  size member)
 *****************************************************************************
*/
static void put_string(const string_t *str, int isterm, language_t lang)
{
    int cnt;

    if (win32)
    {

        if (str->type == str_char)
        {
            int codepage = get_language_codepage( lang );
            string_t *newstr = convert_string_unicode( str, codepage );

            if (check_valid_utf8( str, codepage ))
            {
                print_location( &str->loc );
                warning( "string \"%s\" seems to be UTF-8 but codepage %u is in use, maybe use --utf8?\n",
                         str->str.cstr, codepage );
            }
            str = newstr;
        }
        if (!isterm) put_word(str->size);
        for(cnt = 0; cnt < str->size; cnt++)
        {
            WCHAR c = str->str.wstr[cnt];
            if (isterm && !c) break;
            put_word(c);
        }
        if (isterm) put_word(0);
    }
    else
    {
        assert( str->type == str_char );
        if (!isterm) put_byte(str->size);
        for(cnt = 0; cnt < str->size; cnt++)
        {
            char c = str->str.cstr[cnt];
            if (isterm && !c) break;
            put_byte(c);
        }
        if (isterm) put_byte(0);
    }
}

/*
 *****************************************************************************
 * Function	: put_name_id
 *****************************************************************************
*/
static void put_name_id(name_id_t *nid, int upcase, language_t lang)
{
	switch (nid->type)
	{
	case name_ord:
		if(win32)
			put_word(0xffff);
		else
			put_byte(0xff);
		put_word(nid->name.i_name);
		break;
	case name_str:
		if(upcase)
			string_to_upper(nid->name.s_name);
		put_string(nid->name.s_name, TRUE, lang);
		break;
	}
}

/*
 *****************************************************************************
 * Function	: put_lvc
 *****************************************************************************
*/
static void put_lvc(lvc_t *lvc)
{
	if(lvc)
	{
		put_word(lvc->language);
		put_dword(lvc->version);
		put_dword(lvc->characts);
	}
	else
	{
		put_word(0);	/* Neutral */
		put_dword(0);
		put_dword(0);
	}
}

/*
 *****************************************************************************
 * Function	: put_res_header
 * Input	:
 *	type	- Resource identifier
 *	name	- Resource's name
 *	memopt	- Resource's memory options to write
 *	lvc	- Language, version and characteristics (win32 only)
 * Output	: An index to the resource size field. The resource size field
 *		  contains the header size upon exit.
 *****************************************************************************
*/
static int put_res_header(int type, name_id_t *name, unsigned int memopt, lvc_t *lvc)
{
	int tag = output_buffer_pos;
	if(win32)
	{
		put_dword(0);		/* We will overwrite these later */
		put_dword(0);
		put_word(0xffff);		/* ResType */
		put_word(type);
		put_name_id(name, TRUE, lvc->language); /* ResName */
		align_output(4);
		put_dword(0);		/* DataVersion */
		put_word(memopt);		/* Memory options */
		put_lvc(lvc);		/* Language, version and characts */
		set_dword(tag + 0, output_buffer_pos - tag);
		set_dword(tag + 4, output_buffer_pos - tag);	/* Set HeaderSize */
	}
	else /* win16 */
	{
		put_byte(0xff);		/* ResType */
		put_word(type);
		put_name_id(name, TRUE, 0); /* ResName */
		put_word(memopt);		/* Memory options */
		tag = output_buffer_pos;
		put_dword(4);		/* ResSize overwritten later*/
	}
	return tag;
}

/*
 *****************************************************************************
 * Function	: accelerator2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	acc	- The accelerator descriptor
 *****************************************************************************
*/
static void accelerator2res(name_id_t *name, accelerator_t *acc)
{
	int restag;
	event_t *ev;

	restag = put_res_header(WRC_RT_ACCELERATOR, name, acc->memopt, &acc->lvc);
	if(win32)
	{
		for (ev = acc->events; ev; ev = ev->next)
		{
			int key = ev->key;
			if (ev->str) key = parse_accel_string( ev->str, ev->flags );
			put_word(ev->flags | (ev->next ? 0 : 0x80));
			put_word(key);
			put_word(ev->id);
                        align_output(4);
		}
	}
	else /* win16 */
	{
		for (ev = acc->events; ev; ev = ev->next)
		{
			int key = ev->key;
			if (ev->str) key = parse_accel_string( ev->str, ev->flags );
			put_byte(ev->flags | (ev->next ? 0 : 0x80));
			put_word(key);
			put_word(ev->id);
		}
	}
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: dialog2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	dlg	- The dialog descriptor
 *****************************************************************************
*/
static void dialog2res(name_id_t *name, dialog_t *dlg)
{
	int restag;
	control_t *ctrl;
	int nctrl;

        restag = put_res_header(WRC_RT_DIALOG, name, dlg->memopt, &dlg->lvc);
	for (ctrl = dlg->controls, nctrl = 0; ctrl; ctrl = ctrl->next) nctrl++;
	if(win32)
	{
		if (dlg->is_ex)
		{
			/* FIXME: MS doc says that the first word must contain 0xffff
			 * and the second 0x0001 to signal a DLGTEMPLATEEX. Borland's
			 * compiler reverses the two words.
			 * I don't know which one to choose, but I write it as Mr. B
			 * writes it.
			 */
			put_word(1);		/* Signature */
			put_word(0xffff);		/* DlgVer */
			put_dword(dlg->gothelpid ? dlg->helpid : 0);
			put_dword(dlg->gotexstyle ? dlg->exstyle->or_mask : 0);
			put_dword(dlg->gotstyle ? dlg->style->or_mask : WS_POPUPWINDOW);
		}
		else
		{
			put_dword(dlg->style->or_mask);
			put_dword(dlg->gotexstyle ? dlg->exstyle->or_mask : 0);
		}
		put_word(nctrl);
		put_word(dlg->x);
		put_word(dlg->y);
		put_word(dlg->width);
		put_word(dlg->height);
		if(dlg->menu)
			put_name_id(dlg->menu, FALSE, dlg->lvc.language);
		else
			put_word(0);
		if(dlg->dlgclass)
			put_name_id(dlg->dlgclass, FALSE, dlg->lvc.language);
		else
			put_word(0);
		if(dlg->title)
			put_string(dlg->title, TRUE, dlg->lvc.language);
		else
			put_word(0);
		if(dlg->font)
		{
			put_word(dlg->font->size);
			if (dlg->is_ex)
			{
				put_word(dlg->font->weight);
				/* FIXME: ? TRUE should be sufficient to say that it's
				 * italic, but Borland's compiler says it's 0x0101.
				 * I just copy it here, and hope for the best.
				 */
				put_word(dlg->font->italic ? 0x0101 : 0);
			}
			put_string(dlg->font->name, TRUE, dlg->lvc.language);
		}
                else if (dlg->style->or_mask & DS_SETFONT) put_word( 0x7fff );

		align_output(4);
		for (ctrl = dlg->controls; ctrl; ctrl = ctrl->next)
		{
			if (dlg->is_ex)
			{
				put_dword(ctrl->gothelpid ? ctrl->helpid : 0);
				put_dword(ctrl->gotexstyle ? ctrl->exstyle->or_mask : 0);
				/* FIXME: what is default control style? */
				put_dword(ctrl->gotstyle ? ctrl->style->or_mask : WS_CHILD | WS_VISIBLE);
			}
			else
			{
				/* FIXME: what is default control style? */
				put_dword(ctrl->gotstyle ? ctrl->style->or_mask: WS_CHILD);
				put_dword(ctrl->gotexstyle ? ctrl->exstyle->or_mask : 0);
			}
			put_word(ctrl->x);
			put_word(ctrl->y);
			put_word(ctrl->width);
			put_word(ctrl->height);
			if (dlg->is_ex)
				put_dword(ctrl->id);
			else
				put_word(ctrl->id);
			put_name_id(ctrl->ctlclass, FALSE, dlg->lvc.language);
			if(ctrl->title)
				put_name_id(ctrl->title, FALSE, dlg->lvc.language);
			else
				put_word(0);
			if(ctrl->extra)
			{
				put_word(ctrl->extra->size);
				put_data(ctrl->extra->data, ctrl->extra->size);
			}
			else
				put_word(0);

			if(ctrl->next)
				align_output(4);
		}
	}
	else /* win16 */
	{
		restag = put_res_header(WRC_RT_DIALOG, name, dlg->memopt, NULL);

		put_dword(dlg->gotstyle ? dlg->style->or_mask : WS_POPUPWINDOW);
		put_byte(nctrl);
		put_word(dlg->x);
		put_word(dlg->y);
		put_word(dlg->width);
		put_word(dlg->height);
		if(dlg->menu)
			put_name_id(dlg->menu, TRUE, 0);
		else
			put_byte(0);
		if(dlg->dlgclass)
			put_name_id(dlg->dlgclass, TRUE, 0);
		else
			put_byte(0);
		if(dlg->title)
			put_string(dlg->title, TRUE, 0);
		else
			put_byte(0);
		if(dlg->font)
		{
			put_word(dlg->font->size);
			put_string(dlg->font->name, TRUE, 0);
		}

		for (ctrl = dlg->controls; ctrl; ctrl = ctrl->next)
		{
			put_word(ctrl->x);
			put_word(ctrl->y);
			put_word(ctrl->width);
			put_word(ctrl->height);
			put_word(ctrl->id);
			put_dword(ctrl->gotstyle ? ctrl->style->or_mask: WS_CHILD);
			if(ctrl->ctlclass->type == name_ord
			&& ctrl->ctlclass->name.i_name >= 0x80
			&& ctrl->ctlclass->name.i_name <= 0x85)
				put_byte(ctrl->ctlclass->name.i_name);
			else if(ctrl->ctlclass->type == name_str)
				put_name_id(ctrl->ctlclass, FALSE, 0);
			else
				error("Unknown control-class %04x\n", ctrl->ctlclass->name.i_name);
			if(ctrl->title)
				put_name_id(ctrl->title, FALSE, 0);
			else
				put_byte(0);

			/* FIXME: What is this extra byte doing here? */
			put_byte(0);
		}
	}
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: menuitem2res
 * Remarks	: Self recursive
 *****************************************************************************
*/
static void menuitem2res(menu_item_t *menitem, language_t lang)
{
	menu_item_t *itm = menitem;

	while(itm)
	{
		put_word(itm->state | (itm->popup ? MF_POPUP : 0) | (!itm->next ? MF_END : 0));
		if(!itm->popup)
			put_word(itm->id);
		if(itm->name)
			put_string(itm->name, TRUE, lang);
		else
                {
			if (win32) put_word(0);
			else put_byte(0);
		}
		if(itm->popup)
			menuitem2res(itm->popup, lang);
		itm = itm->next;
	}
}

/*
 *****************************************************************************
 * Function	: menuexitem2res
 * Remarks	: Self recursive
 *****************************************************************************
*/
static void menuexitem2res(menu_item_t *menitem, language_t lang)
{
	menu_item_t *itm = menitem;
	assert(win32 != 0);
	while(itm)
	{
		put_dword(itm->gottype ? itm->type : 0);
		put_dword(itm->gotstate ? itm->state : 0);
		put_dword(itm->gotid ? itm->id : 0);
		put_word((itm->popup ? 0x01 : 0) | (!itm->next ? MF_END : 0));
		if(itm->name)
			put_string(itm->name, TRUE, lang);
		else
			put_word(0);
		align_output(4);
		if(itm->popup)
		{
			put_dword(itm->gothelpid ? itm->helpid : 0);
			menuexitem2res(itm->popup, lang);
		}
		itm = itm->next;
	}

}

/*
 *****************************************************************************
 * Function	: menu2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	menex	- The menuex descriptor
 *****************************************************************************
*/
static void menu2res(name_id_t *name, menu_t *men)
{
	int restag;
	assert(name != NULL);
	assert(men != NULL);

	restag = put_res_header(WRC_RT_MENU, name, men->memopt, &men->lvc);
	if(win32)
	{
		if (men->is_ex)
		{
			put_word(1);		/* Menuheader: Version */
			put_word(4);		/* Offset */
			put_dword(0);		/* HelpId */
			align_output(4);
			menuexitem2res(men->items, men->lvc.language);
		}
		else
		{
			put_dword(0);		/* Menuheader: Version and HeaderSize */
			menuitem2res(men->items, men->lvc.language);
		}
	}
	else /* win16 */
	{
		put_dword(0);		/* Menuheader: Version and HeaderSize */
		menuitem2res(men->items, 0);
	}
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: cursorgroup2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	curg	- The cursor descriptor
 *****************************************************************************
*/
static void cursorgroup2res(name_id_t *name, cursor_group_t *curg)
{
    int restag;
    cursor_t *cur;

    restag = put_res_header(WRC_RT_GROUP_CURSOR, name, curg->memopt, &curg->lvc);

    put_word(0);	/* Reserved */
    /* FIXME: The ResType in the NEWHEADER structure should
     * contain 14 according to the MS win32 doc. This is
     * not the case with the BRC compiler and I really doubt
     * the latter. Putting one here is compliant to win16 spec,
     * but who knows the true value?
     */
    put_word(2);	/* ResType */
    put_word(curg->ncursor);
#if 0
    for(cur = curg->cursorlist; cur; cur = cur->next)
#else
    cur = curg->cursorlist;
    while(cur->next)
        cur = cur->next;
    for(; cur; cur = cur->prev)
#endif
    {
        put_word(cur->width);
        /* FIXME: The height of a cursor is half the size of
         * the bitmap's height. BRC puts the height from the
         * BITMAPINFOHEADER here instead of the cursorfile's
         * height. MS doesn't seem to care...
         */
        put_word(cur->height);
        /* FIXME: The next two are reversed in BRC and I don't
         * know why. Probably a bug. But, we can safely ignore
         * it because win16 does not support color cursors.
         * A warning should have been generated by the parser.
         */
        put_word(cur->planes);
        put_word(cur->bits);
        /* FIXME: The +4 is the hotspot in the cursor resource.
         * However, I could not find this in the documentation.
         * The hotspot bytes must either be included or MS
         * doesn't care.
         */
        put_dword(cur->data->size +4);
        put_word(cur->id);
    }

    set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: cursor2res
 * Input	:
 *	cur	- The cursor descriptor
 *****************************************************************************
*/
static void cursor2res(cursor_t *cur)
{
	int restag;
	name_id_t name;

	name.type = name_ord;
	name.name.i_name = cur->id;
	restag = put_res_header(WRC_RT_CURSOR, &name, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, &cur->lvc);
	put_word(cur->xhot);
	put_word(cur->yhot);
	put_data(cur->data->data, cur->data->size);

	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: icongroup2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	icog	- The icon group descriptor
 *****************************************************************************
*/
static void icongroup2res(name_id_t *name, icon_group_t *icog)
{
    int restag;
    icon_t *ico;

    restag = put_res_header(WRC_RT_GROUP_ICON, name, icog->memopt, &icog->lvc);

    put_word(0);	/* Reserved */
    /* FIXME: The ResType in the NEWHEADER structure should
     * contain 14 according to the MS win32 doc. This is
     * not the case with the BRC compiler and I really doubt
     * the latter. Putting one here is compliant to win16 spec,
     * but who knows the true value?
     */
    put_word(1);	/* ResType */
    put_word(icog->nicon);
    for(ico = icog->iconlist; ico; ico = ico->next)
    {
        put_byte(ico->width);
        put_byte(ico->height);
        put_byte(ico->nclr);
        put_byte(0);	/* Reserved */
        put_word(ico->planes);
        put_word(ico->bits);
        put_dword(ico->data->size);
        put_word(ico->id);
    }

    set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: icon2res
 * Input	:
 *	ico	- The icon descriptor
 *****************************************************************************
*/
static void icon2res(icon_t *ico)
{
	int restag;
	name_id_t name;

	name.type = name_ord;
	name.name.i_name = ico->id;
	restag = put_res_header(WRC_RT_ICON, &name, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, &ico->lvc);
	put_data(ico->data->data, ico->data->size);
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: anicurico2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	ani	- The animated object descriptor
 * Remarks	: There are rumors that win311 could handle animated stuff.
 * 		  That is why they are generated for both win16 and win32
 * 		  compile.
 *****************************************************************************
*/
static void anicur2res(name_id_t *name, ani_curico_t *ani)
{
	int restag = put_res_header(WRC_RT_ANICURSOR, name, ani->memopt, NULL);

	put_data(ani->data->data, ani->data->size);
	set_res_size(restag);
}

static void aniico2res(name_id_t *name, ani_curico_t *ani)
{
	int restag = put_res_header(WRC_RT_ANIICON, name, ani->memopt, NULL);

	put_data(ani->data->data, ani->data->size);
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: bitmap2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	bmp	- The bitmap descriptor
 *****************************************************************************
*/
static void bitmap2res(name_id_t *name, bitmap_t *bmp)
{
	int restag = put_res_header(WRC_RT_BITMAP, name, bmp->memopt, &bmp->data->lvc);

	if(bmp->data->data[0] == 'B'
	&& bmp->data->data[1] == 'M'
	&& ((BITMAPFILEHEADER *)bmp->data->data)->bfSize == bmp->data->size
	&& bmp->data->size >= sizeof(BITMAPFILEHEADER))
	{
		/* The File header is still attached, don't write it */
		put_data((BITMAPFILEHEADER *)bmp->data->data + 1, bmp->data->size - sizeof(BITMAPFILEHEADER));
	}
	else
	{
		put_data(bmp->data->data, bmp->data->size);
	}
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: font2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	fnt	- The font descriptor
 * Remarks	: The data has been prepared just after parsing.
 *****************************************************************************
*/
static void font2res(name_id_t *name, font_t *fnt)
{
	int restag = put_res_header(WRC_RT_FONT, name, fnt->memopt, &fnt->data->lvc);

	put_data(fnt->data->data, fnt->data->size);
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: fontdir2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	fntdir	- The fontdir descriptor
 * Remarks	: The data has been prepared just after parsing.
 *****************************************************************************
*/
static void fontdir2res(name_id_t *name, fontdir_t *fnd)
{
	int restag = put_res_header(WRC_RT_FONTDIR, name, fnd->memopt, &fnd->data->lvc);

	put_data(fnd->data->data, fnd->data->size);
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: html2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	rdt	- The html descriptor
 *****************************************************************************
*/
static void html2res(name_id_t *name, html_t *html)
{
	int restag = put_res_header(WRC_RT_HTML, name, html->memopt, &html->data->lvc);

	put_data(html->data->data, html->data->size);
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: rcdata2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	rdt	- The rcdata descriptor
 *****************************************************************************
*/
static void rcdata2res(name_id_t *name, rcdata_t *rdt)
{
	int restag = put_res_header(WRC_RT_RCDATA, name, rdt->memopt, &rdt->data->lvc);

	put_data(rdt->data->data, rdt->data->size);
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: messagetable2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	msg	- The messagetable descriptor
 *****************************************************************************
*/
static void messagetable2res(name_id_t *name, messagetable_t *msg)
{
	int restag = put_res_header(WRC_RT_MESSAGETABLE, name, msg->memopt, &msg->data->lvc);

	put_data(msg->data->data, msg->data->size);
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: stringtable2res
 * Input	:
 *	stt	- The stringtable descriptor
 *****************************************************************************
*/
static void stringtable2res(stringtable_t *stt)
{
	name_id_t name;
	int i;
	int restag;

	for(; stt; stt = stt->next)
	{
		if(!stt->nentries)
		{
			warning("Empty internal stringtable\n");
			continue;
		}
		name.type = name_ord;
		name.name.i_name = (stt->idbase >> 4) + 1;
		restag = put_res_header(WRC_RT_STRING, &name, stt->memopt, &stt->lvc);
		for(i = 0; i < stt->nentries; i++)
		{
			if(stt->entries[i].str && stt->entries[i].str->size)
			{
				put_string(stt->entries[i].str, FALSE, stt->lvc.language);
			}
			else
			{
				if (win32)
					put_word(0);
				else
					put_byte(0);
			}
		}
                set_res_size(restag);
	}
}

/*
 *****************************************************************************
 * Function	: user2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	usr	- The userresource descriptor
 *****************************************************************************
*/
static void user2res(name_id_t *name, user_t *usr)
{
	int tag = output_buffer_pos;

	if(win32)
	{
		put_dword(0);		/* We will overwrite these later */
		put_dword(0);
		put_name_id(usr->type, TRUE, usr->data->lvc.language);
		put_name_id(name, TRUE, usr->data->lvc.language); /* ResName */
		align_output(4);
		put_dword(0);		/* DataVersion */
		put_word(usr->memopt);		/* Memory options */
		put_lvc(&usr->data->lvc);	/* Language, version and characts */
		set_dword(tag + 0, output_buffer_pos - tag);
		set_dword(tag + 4, output_buffer_pos - tag);	/* Set HeaderSize */
	}
	else /* win16 */
	{
		put_name_id(usr->type, TRUE, 0);
		put_name_id(name, TRUE, 0); /* ResName */
		put_word(usr->memopt);		/* Memory options */
		tag = output_buffer_pos;
		put_dword(4);		/* ResSize overwritten later*/
	}

	put_data(usr->data->data, usr->data->size);
	set_res_size(tag);
}

/*
 *****************************************************************************
 * Function	: versionblock2res
 * Input	:
 *	blk	- The version block to be written
 * Remarks	: Self recursive
 *****************************************************************************
*/
static void versionblock2res(ver_block_t *blk, int level, language_t lang)
{
	ver_value_t *val;
	int blksizetag;
	int valblksizetag;
	int valvalsizetag;
	int tag;
	int i;

	blksizetag = output_buffer_pos;
	put_word(0);	/* Will be overwritten later */
	put_word(0);
	if(win32)
		put_word(0);	/* level ? */
	put_string(blk->name, TRUE, 0);
	align_output(4);
	for(val = blk->values; val; val = val->next)
	{
		switch(val->type)
		{
		case val_str:
			valblksizetag = output_buffer_pos;
			put_word(0);	/* Will be overwritten later */
			valvalsizetag = output_buffer_pos;
			put_word(0);	/* Will be overwritten later */
			if(win32)
			{
				put_word(level);
			}
			put_string(val->key, TRUE, 0);
			align_output(4);
			tag = output_buffer_pos;
			put_string(val->value.str, TRUE, lang);
			if(win32)
				set_word(valvalsizetag, (output_buffer_pos - tag) >> 1);
			else
				set_word(valvalsizetag, output_buffer_pos - tag);
			set_word(valblksizetag, output_buffer_pos - valblksizetag);
			align_output(4);
                        break;
		case val_words:
			valblksizetag = output_buffer_pos;
			put_word(0);	/* Will be overwritten later */
			valvalsizetag = output_buffer_pos;
			put_word(0);	/* Will be overwritten later */
			if(win32)
			{
				put_word(level);
			}
			put_string(val->key, TRUE, 0);
			align_output(4);
			tag = output_buffer_pos;
			for(i = 0; i < val->value.words->nwords; i++)
			{
				put_word(val->value.words->words[i]);
			}
			set_word(valvalsizetag, output_buffer_pos - tag);
			set_word(valblksizetag, output_buffer_pos - valblksizetag);
			align_output(4);
                        break;
		case val_block:
			versionblock2res(val->value.block, level+1, lang);
                        break;
		}
	}

	/* Set blocksize */
	set_word(blksizetag, output_buffer_pos - blksizetag);
}

/*
 *****************************************************************************
 * Function	: versioninfo2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	ver	- The versioninfo descriptor
 *****************************************************************************
*/
static void versioninfo2res(name_id_t *name, versioninfo_t *ver)
{
	static const char info[] = "VS_VERSION_INFO";
	unsigned int i;
	int restag, rootblocksizetag, valsizetag, tag;
	ver_block_t *blk;

	restag = put_res_header(WRC_RT_VERSION, name, ver->memopt, &ver->lvc);
	rootblocksizetag = output_buffer_pos;
	put_word(0);	/* BlockSize filled in later */
	valsizetag = output_buffer_pos;
	put_word(0);	/* ValueSize filled in later*/
	if(win32)
	{
		put_word(0);	/* Tree-level ? */
		for (i = 0; i < sizeof(info); i++) put_word(info[i]);
		align_output(4);
	}
	else
	{
		for (i = 0; i < sizeof(info); i++) put_byte(info[i]);
	}
	tag = output_buffer_pos;
	put_dword(VS_FFI_SIGNATURE);
	put_dword(VS_FFI_STRUCVERSION);
	put_dword((ver->filever_maj1 << 16) + (ver->filever_maj2 & 0xffff));
	put_dword((ver->filever_min1 << 16) + (ver->filever_min2 & 0xffff));
	put_dword((ver->prodver_maj1 << 16) + (ver->prodver_maj2 & 0xffff));
	put_dword((ver->prodver_min1 << 16) + (ver->prodver_min2 & 0xffff));
	put_dword(ver->fileflagsmask);
	put_dword(ver->fileflags);
	put_dword(ver->fileos);
	put_dword(ver->filetype);
	put_dword(ver->filesubtype);
	put_dword(0);		/* FileDateMS */
	put_dword(0);		/* FileDateLS */
	/* Set ValueSize */
	set_word(valsizetag, output_buffer_pos - tag);
	/* Descend into the blocks */
	for(blk = ver->blocks; blk; blk = blk->next)
		versionblock2res(blk, 0, win32 ? ver->lvc.language : 0);
	/* Set root block's size */
	set_word(rootblocksizetag, output_buffer_pos - rootblocksizetag);
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: toolbar2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	toolbar	- The toolbar descriptor
 *****************************************************************************
*/
static void toolbar2res(name_id_t *name, toolbar_t *toolbar)
{
	toolbar_item_t *itm;
	int restag;

	if (!win32) return;

	restag = put_res_header(WRC_RT_TOOLBAR, name, toolbar->memopt, &toolbar->lvc);
	put_word(1);		/* Menuheader: Version */
	put_word(toolbar->button_width); /* (in pixels?) */
	put_word(toolbar->button_height); /* (in pixels?) */
	put_word(toolbar->nitems);
	align_output(4);
	for (itm = toolbar->items; itm; itm = itm->next) put_word(itm->id);
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: dlginit2res
 * Input	:
 *	name	- Name/ordinal of the resource
 *	rdt	- The dlginit descriptor
 *****************************************************************************
*/
static void dlginit2res(name_id_t *name, dlginit_t *dit)
{
	int restag = put_res_header(WRC_RT_DLGINIT, name, dit->memopt, &dit->data->lvc);

	put_data(dit->data->data, dit->data->size);
	set_res_size(restag);
}

/*
 *****************************************************************************
 * Function	: write_resfile
 * Syntax	: void write_resfile(char *outname, resource_t *top)
 * Input	:
 *	outname	- Filename to write to
 *	top	- The resource-tree to convert
 *****************************************************************************
*/
void write_resfile(char *outname, resource_t *top)
{
        init_output_buffer();

	if(win32)
	{
		/* Put an empty resource first to signal win32 format */
		put_dword(0);		/* ResSize */
		put_dword(0x00000020);	/* HeaderSize */
		put_word(0xffff);		/* ResType */
		put_word(0);
		put_word(0xffff);		/* ResName */
		put_word(0);
		put_dword(0);		/* DataVersion */
		put_word(0);		/* Memory options */
		put_word(0);		/* Language */
		put_dword(0);		/* Version */
		put_dword(0);		/* Characteristics */
	}

	for ( ; top; top = top->next)
	{
		switch(top->type)
		{
		case res_acc:     accelerator2res(top->name, top->res.acc); break;
		case res_bmp:     bitmap2res(top->name, top->res.bmp); break;
		case res_cur:     cursor2res(top->res.cur); break;
		case res_curg:    cursorgroup2res(top->name, top->res.curg); break;
		case res_dlg:     dialog2res(top->name, top->res.dlg); break;
		case res_fnt:     font2res(top->name, top->res.fnt); break;
		case res_fntdir:  fontdir2res(top->name, top->res.fnd); break;
		case res_ico:     icon2res(top->res.ico); break;
		case res_icog:    icongroup2res(top->name, top->res.icog); break;
		case res_men:     menu2res(top->name, top->res.men); break;
		case res_html:    html2res(top->name, top->res.html); break;
		case res_rdt:     rcdata2res(top->name, top->res.rdt); break;
		case res_stt:     stringtable2res(top->res.stt); break;
		case res_usr:     user2res(top->name, top->res.usr); break;
		case res_msg:     messagetable2res(top->name, top->res.msg); break;
		case res_ver:     versioninfo2res(top->name, top->res.ver); break;
		case res_toolbar: toolbar2res(top->name, top->res.tbt); break;
		case res_dlginit: dlginit2res(top->name, top->res.dlgi); break;
		case res_anicur:  anicur2res(top->name, top->res.ani); break;
		case res_aniico:  aniico2res(top->name, top->res.ani); break;
		default: assert(0);
		}
		if (win32) align_output( 4 );
	}

	flush_output_buffer( outname );
}
