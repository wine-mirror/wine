/*
 * Support for po files
 *
 * Copyright 2010 Alexandre Julliard
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
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#ifdef HAVE_LIBGETTEXTPO
#include <gettext-po.h>
#endif

#include "../tools.h"
#include "wrc.h"
#include "newstruc.h"
#include "utils.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/list.h"

static resource_t *new_top, *new_tail;

struct mo_file
{
    unsigned int magic;
    unsigned int revision;
    unsigned int count;
    unsigned int msgid_off;
    unsigned int msgstr_off;
    /* ... rest of file data here */
};

static int is_english( language_t lan )
{
    return lan == MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT );
}

static int is_rtl_language( language_t lan )
{
    return PRIMARYLANGID(lan) == LANG_ARABIC ||
           PRIMARYLANGID(lan) == LANG_HEBREW ||
           PRIMARYLANGID(lan) == LANG_PERSIAN;
}

static int uses_larger_font( language_t lan )
{
    return PRIMARYLANGID(lan) == LANG_CHINESE ||
           PRIMARYLANGID(lan) == LANG_JAPANESE ||
           PRIMARYLANGID(lan) == LANG_KOREAN;
}

static language_t get_default_sublang( language_t lan )
{
    if (SUBLANGID(lan) != SUBLANG_NEUTRAL) return lan;

    switch (PRIMARYLANGID(lan))
    {
    case LANG_SPANISH:
        return MAKELANGID( LANG_SPANISH, SUBLANG_SPANISH_MODERN );
    case LANG_CHINESE:
        return MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED );
    default:
        return MAKELANGID( PRIMARYLANGID(lan), SUBLANG_DEFAULT );
    }
}

static version_t get_dup_version( language_t lang )
{
    /* English "translations" take precedence over the original rc contents */
    return is_english( lang ) ? 1 : -1;
}

static name_id_t *dup_name_id( name_id_t *id )
{
    name_id_t *new;

    if (!id || id->type != name_str) return id;
    new = new_name_id();
    *new = *id;
    new->name.s_name = convert_string_unicode( id->name.s_name, 1252 );
    return new;
}

static char *convert_msgid_ascii( const string_t *str, int error_on_invalid_char )
{
    int i;
    char *buffer = xmalloc( str->size + 1 );

    for (i = 0; i < str->size; i++)
    {
        WCHAR ch = (str->type == str_unicode ? str->str.wstr[i] : (unsigned char)str->str.cstr[i]);
        buffer[i] = ch;
        if (ch >= 32 && ch <= 127) continue;
        if (ch == '\t' || ch == '\n') continue;
        if (error_on_invalid_char)
        {
            print_location( &str->loc );
            error( "Invalid character %04x in source string\n", ch );
        }
        free( buffer);
        return NULL;
    }
    buffer[i] = 0;
    return buffer;
}

static char *get_message_context( char **msgid )
{
    static const char magic[] = "#msgctxt#";
    char *id, *context;

    if (strncmp( *msgid, magic, sizeof(magic) - 1 )) return NULL;
    context = *msgid + sizeof(magic) - 1;
    if (!(id = strchr( context, '#' ))) return NULL;
    *id = 0;
    *msgid = id + 1;
    return context;
}

static int control_has_title( const control_t *ctrl )
{
    if (!ctrl->title) return FALSE;
    if (ctrl->title->type != name_str) return FALSE;
    /* check for text static control */
    if (ctrl->ctlclass && ctrl->ctlclass->type == name_ord && ctrl->ctlclass->name.i_name == CT_STATIC)
    {
        switch (ctrl->style->or_mask & SS_TYPEMASK)
        {
        case SS_LEFT:
        case SS_CENTER:
        case SS_RIGHT:
            return TRUE;
        default:
            return FALSE;
        }
    }
    return TRUE;
}

static resource_t *dup_resource( resource_t *res, language_t lang )
{
    resource_t *new = xmalloc( sizeof(*new) );

    *new = *res;
    new->lan = lang;
    new->next = new->prev = NULL;
    new->name = dup_name_id( res->name );

    switch (res->type)
    {
    case res_acc:
        new->res.acc = xmalloc( sizeof(*(new)->res.acc) );
        *new->res.acc = *res->res.acc;
        new->res.acc->lvc.language = lang;
        new->res.acc->lvc.version = get_dup_version( lang );
        break;
    case res_dlg:
        new->res.dlg = xmalloc( sizeof(*(new)->res.dlg) );
        *new->res.dlg = *res->res.dlg;
        new->res.dlg->lvc.language = lang;
        new->res.dlg->lvc.version = get_dup_version( lang );
        break;
    case res_men:
        new->res.men = xmalloc( sizeof(*(new)->res.men) );
        *new->res.men = *res->res.men;
        new->res.men->lvc.language = lang;
        new->res.men->lvc.version = get_dup_version( lang );
        break;
    case res_stt:
        new->res.stt = xmalloc( sizeof(*(new)->res.stt) );
        *new->res.stt = *res->res.stt;
        new->res.stt->lvc.language = lang;
        new->res.stt->lvc.version = get_dup_version( lang );
        break;
    case res_ver:
        new->res.ver = xmalloc( sizeof(*(new)->res.ver) );
        *new->res.ver = *res->res.ver;
        new->res.ver->lvc.language = lang;
        new->res.ver->lvc.version = get_dup_version( lang );
        break;
    default:
        assert(0);
    }
    return new;
}

#ifndef HAVE_LIBGETTEXTPO

void write_pot_file( const char *outname )
{
    error( "PO files not supported in this wrc build\n" );
}

void write_po_files( const char *outname )
{
    error( "PO files not supported in this wrc build\n" );
}

#else  /* HAVE_LIBGETTEXTPO */

static void po_xerror( int severity, po_message_t message,
                       const char *filename, size_t lineno, size_t column,
                       int multiline_p, const char *message_text )
{
    fprintf( stderr, "%s:%u:%u: %s\n",
             filename, (unsigned int)lineno, (unsigned int)column, message_text );
    if (severity) exit(1);
}

static void po_xerror2( int severity, po_message_t message1,
                        const char *filename1, size_t lineno1, size_t column1,
                        int multiline_p1, const char *message_text1,
                        po_message_t message2,
                        const char *filename2, size_t lineno2, size_t column2,
                        int multiline_p2, const char *message_text2 )
{
    fprintf( stderr, "%s:%u:%u: %s\n",
             filename1, (unsigned int)lineno1, (unsigned int)column1, message_text1 );
    fprintf( stderr, "%s:%u:%u: %s\n",
             filename2, (unsigned int)lineno2, (unsigned int)column2, message_text2 );
    if (severity) exit(1);
}

static const struct po_xerror_handler po_xerror_handler = { po_xerror, po_xerror2 };

static po_message_t find_message( po_file_t po, const char *msgid, const char *msgctxt,
                                  po_message_iterator_t *iterator )
{
    po_message_t msg;
    const char *context;

    *iterator = po_message_iterator( po, NULL );
    while ((msg = po_next_message( *iterator )))
    {
        if (strcmp( po_message_msgid( msg ), msgid )) continue;
        if (!msgctxt) break;
        if (!(context = po_message_msgctxt( msg ))) continue;
        if (!strcmp( context, msgctxt )) break;
    }
    return msg;
}

static void add_po_string( po_file_t po, const string_t *msgid, const string_t *msgstr, language_t lang )
{
    static const char dnt[] = "do not translate";
    po_message_t msg;
    po_message_iterator_t iterator;
    int codepage;
    char *id, *id_buffer, *context, *str = NULL, *str_buffer = NULL;

    if (!msgid->size) return;

    id_buffer = id = convert_msgid_ascii( msgid, 1 );
    context = get_message_context( &id );
    if (context && strcmp(context, dnt) == 0)
    {
        /* This string should not be translated */
        free( id_buffer );
        return;
    }

    if (msgstr)
    {
        codepage = get_language_codepage( lang );
        str = str_buffer = convert_string_utf8( msgstr, codepage );
        if (is_english( lang )) get_message_context( &str );
    }
    if (!(msg = find_message( po, id, context, &iterator )))
    {
        msg = po_message_create();
        po_message_set_msgid( msg, id );
        po_message_set_msgstr( msg, str ? str : "" );
        if (context) po_message_set_msgctxt( msg, context );
        po_message_insert( iterator, msg );
    }
    if (msgid->loc.file) po_message_add_filepos( msg, msgid->loc.file, msgid->loc.line );
    po_message_iterator_free( iterator );
    free( id_buffer );
    free( str_buffer );
}

struct po_file_lang
{
    struct list entry;
    language_t  lang;
    po_file_t   po;
};

static struct list po_file_langs = LIST_INIT( po_file_langs );

static po_file_t create_po_file(void)
{
    po_file_t po;
    po_message_t msg;
    po_message_iterator_t iterator;

    po = po_file_create();
    iterator = po_message_iterator( po, NULL );
    msg = po_message_create();
    po_message_set_msgid( msg, "" );
    po_message_set_msgstr( msg,
                           "Project-Id-Version: Wine\n"
                           "Report-Msgid-Bugs-To: https://bugs.winehq.org\n"
                           "POT-Creation-Date: N/A\n"
                           "PO-Revision-Date: N/A\n"
                           "Last-Translator: Automatically generated\n"
                           "Language-Team: none\n"
                           "MIME-Version: 1.0\n"
                           "Content-Type: text/plain; charset=UTF-8\n"
                           "Content-Transfer-Encoding: 8bit\n" );
    po_message_insert( iterator, msg );
    po_message_iterator_free( iterator );
    return po;
}

static po_file_t get_po_file( language_t lang )
{
    struct po_file_lang *po_file;

    LIST_FOR_EACH_ENTRY( po_file, &po_file_langs, struct po_file_lang, entry )
        if (po_file->lang == lang) return po_file->po;

    /* create a new one */
    po_file = xmalloc( sizeof(*po_file) );
    po_file->lang = lang;
    po_file->po = create_po_file();
    list_add_tail( &po_file_langs, &po_file->entry );
    return po_file->po;
}

static char *get_po_file_name( language_t lang )
{
    return strmake( "%04x.po", lang );
}

static unsigned int flush_po_files( const char *output_name )
{
    struct po_file_lang *po_file, *next;
    unsigned int count = 0;

    LIST_FOR_EACH_ENTRY_SAFE( po_file, next, &po_file_langs, struct po_file_lang, entry )
    {
        char *name = get_po_file_name( po_file->lang );
        if (output_name)
        {
            if (!strcmp( get_basename(output_name), name ))
            {
                po_file_write( po_file->po, name, &po_xerror_handler );
                count++;
            }
        }
        else  /* no specified output name, output a file for every language found */
        {
            po_file_write( po_file->po, name, &po_xerror_handler );
            count++;
            fprintf( stderr, "created %s\n", name );
        }
        po_file_free( po_file->po );
        list_remove( &po_file->entry );
        free( po_file );
        free( name );
    }
    return count;
}

static void add_pot_stringtable( po_file_t po, const resource_t *res )
{
    const stringtable_t *stt = res->res.stt;
    int i;

    while (stt)
    {
        for (i = 0; i < stt->nentries; i++)
            if (stt->entries[i].str) add_po_string( po, stt->entries[i].str, NULL, 0 );
        stt = stt->next;
    }
}

static void add_po_stringtable( const resource_t *english, const resource_t *res )
{
    const stringtable_t *english_stt = english->res.stt;
    const stringtable_t *stt = res->res.stt;
    po_file_t po = get_po_file( stt->lvc.language );
    int i;

    while (english_stt && stt)
    {
        for (i = 0; i < stt->nentries; i++)
            if (english_stt->entries[i].str && stt->entries[i].str)
                add_po_string( po, english_stt->entries[i].str, stt->entries[i].str, stt->lvc.language );
        stt = stt->next;
        english_stt = english_stt->next;
    }
}

static void add_pot_dialog_controls( po_file_t po, const control_t *ctrl )
{
    while (ctrl)
    {
        if (control_has_title( ctrl )) add_po_string( po, ctrl->title->name.s_name, NULL, 0 );
        ctrl = ctrl->next;
    }
}

static void add_pot_dialog( po_file_t po, const resource_t *res )
{
    const dialog_t *dlg = res->res.dlg;

    if (dlg->title) add_po_string( po, dlg->title, NULL, 0 );
    add_pot_dialog_controls( po, dlg->controls );
}

static void compare_dialogs( const dialog_t *english_dlg, const dialog_t *dlg )
{
    const control_t *english_ctrl, *ctrl;
    unsigned int style = 0, exstyle = 0, english_style = 0, english_exstyle = 0;
    char *name;
    char *title = english_dlg->title ? convert_msgid_ascii( english_dlg->title, 0 ) : xstrdup("??");

    if (english_dlg->width != dlg->width || english_dlg->height != dlg->height)
        warning( "%04x: dialog %s doesn't have the same size (%d,%d vs %d,%d)\n",
                 dlg->lvc.language, title, dlg->width, dlg->height,
                 english_dlg->width, english_dlg->height );

    if (dlg->gotstyle) style = dlg->style->or_mask;
    if (dlg->gotexstyle) exstyle = dlg->exstyle->or_mask;
    if (english_dlg->gotstyle) english_style = english_dlg->style->or_mask;
    if (english_dlg->gotexstyle) english_exstyle = english_dlg->exstyle->or_mask;
    if (is_rtl_language( dlg->lvc.language )) english_exstyle |= WS_EX_LAYOUTRTL;

    if (english_style != style)
        warning( "%04x: dialog %s doesn't have the same style (%08x vs %08x)\n",
                 dlg->lvc.language, title, style, english_style );
    if (english_exstyle != exstyle)
        warning( "%04x: dialog %s doesn't have the same exstyle (%08x vs %08x)\n",
                 dlg->lvc.language, title, exstyle, english_exstyle );

    if (english_dlg->font || dlg->font)
    {
        int size = 0, english_size = 0;
        char *font = NULL, *english_font = NULL;

        if (english_dlg->font)
        {
            english_font = convert_msgid_ascii( english_dlg->font->name, 0 );
            english_size = english_dlg->font->size;
        }
        if (dlg->font)
        {
            font = convert_msgid_ascii( dlg->font->name, 0 );
            size = dlg->font->size;
        }
        if (uses_larger_font( dlg->lvc.language )) english_size++;

        if (!english_font || !font || strcasecmp( english_font, font ) || english_size != size)
            warning( "%04x: dialog %s doesn't have the same font (%s %u vs %s %u)\n",
                     dlg->lvc.language, title,
                     english_font ? english_font : "default", english_size,
                     font ? font : "default", size );
        free( font );
        free( english_font );
    }
    english_ctrl = english_dlg->controls;
    ctrl = dlg->controls;
    for ( ; english_ctrl && ctrl; ctrl = ctrl->next, english_ctrl = english_ctrl->next )
    {
        if (control_has_title( english_ctrl ))
            name = convert_msgid_ascii( english_ctrl->title->name.s_name, 0 );
        else
            name = strmake( "%d", ctrl->id );

        if (english_ctrl->width != ctrl->width || english_ctrl->height != ctrl->height)
            warning( "%04x: dialog %s control %s doesn't have the same size (%d,%d vs %d,%d)\n",
                     dlg->lvc.language, title, name,
                     ctrl->width, ctrl->height, english_ctrl->width, english_ctrl->height );
        if (english_ctrl->x != ctrl->x || english_ctrl->y != ctrl->y)
            warning( "%04x: dialog %s control %s doesn't have the same position (%d,%d vs %d,%d)\n",
                     dlg->lvc.language, title, name, ctrl->x, ctrl->y, english_ctrl->x, english_ctrl->y );
        free( name );
    }
    free( title );
}

static void add_po_dialog_controls( po_file_t po, const control_t *english_ctrl,
                                    const control_t *ctrl, language_t lang )
{
    while (english_ctrl && ctrl)
    {
        if (control_has_title( english_ctrl ) && control_has_title( ctrl ))
            add_po_string( po, english_ctrl->title->name.s_name, ctrl->title->name.s_name, lang );

        ctrl = ctrl->next;
        english_ctrl = english_ctrl->next;
    }
}

static void add_po_dialog( const resource_t *english, const resource_t *res )
{
    const dialog_t *english_dlg = english->res.dlg;
    const dialog_t *dlg = res->res.dlg;
    po_file_t po = get_po_file( dlg->lvc.language );

    compare_dialogs( english_dlg, dlg );

    if (english_dlg->title && dlg->title)
        add_po_string( po, english_dlg->title, dlg->title, dlg->lvc.language );
    add_po_dialog_controls( po, english_dlg->controls, dlg->controls, dlg->lvc.language );
}

static void add_pot_menu_items( po_file_t po, const menu_item_t *item )
{
    while (item)
    {
        if (item->name) add_po_string( po, item->name, NULL, 0 );
        if (item->popup) add_pot_menu_items( po, item->popup );
        item = item->next;
    }
}

static void add_pot_menu( po_file_t po, const resource_t *res )
{
    add_pot_menu_items( po, res->res.men->items );
}

static void add_po_menu_items( po_file_t po, const menu_item_t *english_item,
                               const menu_item_t *item, language_t lang )
{
    while (english_item && item)
    {
        if (english_item->name && item->name)
            add_po_string( po, english_item->name, item->name, lang );
        if (english_item->popup && item->popup)
            add_po_menu_items( po, english_item->popup, item->popup, lang );
        item = item->next;
        english_item = english_item->next;
    }
}

static void add_po_menu( const resource_t *english, const resource_t *res )
{
    const menu_item_t *english_items = english->res.men->items;
    const menu_item_t *items = res->res.men->items;
    po_file_t po = get_po_file( res->res.men->lvc.language );

    add_po_menu_items( po, english_items, items, res->res.men->lvc.language );
}

static int string_has_context( const string_t *str )
{
    char *id, *id_buffer, *context;

    id_buffer = id = convert_msgid_ascii( str, 1 );
    context = get_message_context( &id );
    free( id_buffer );
    return context != NULL;
}

static void add_pot_accel( po_file_t po, const resource_t *res )
{
    event_t *event = res->res.acc->events;

    while (event)
    {
        /* accelerators without a context don't make sense in po files */
        if (event->str && string_has_context( event->str ))
            add_po_string( po, event->str, NULL, 0 );
        event = event->next;
    }
}

static void add_po_accel( const resource_t *english, const resource_t *res )
{
    event_t *english_event = english->res.acc->events;
    event_t *event = res->res.acc->events;
    po_file_t po = get_po_file( res->res.acc->lvc.language );

    while (english_event && event)
    {
        if (english_event->str && event->str && string_has_context( english_event->str ))
            add_po_string( po, english_event->str, event->str, res->res.acc->lvc.language );
        event = event->next;
        english_event = english_event->next;
    }
}

static ver_block_t *get_version_langcharset_block( ver_block_t *block )
{
    ver_block_t *stringfileinfo = NULL;
    char *translation = NULL;
    ver_value_t *val;

    for (; block; block = block->next)
    {
        char *name;
        name = convert_msgid_ascii( block->name, 0 );
        if (!strcasecmp( name, "stringfileinfo" ))
            stringfileinfo = block;
        else if (!strcasecmp( name, "varfileinfo" ))
        {
            for (val = block->values; val; val = val->next)
            {
                char *key = convert_msgid_ascii( val->key, 0 );
                if (val->type == val_words &&
                    !strcasecmp( key, "Translation" ) &&
                    val->value.words->nwords >= 2)
                    translation = strmake( "%04x%04x",
                                           val->value.words->words[0],
                                           val->value.words->words[1] );
                free( key );
            }
        }
        free( name );
    }

    if (!stringfileinfo || !translation) return NULL;

    for (val = stringfileinfo->values; val; val = val->next)
    {
        char *block_name;
        if (val->type != val_block) continue;
        block_name = convert_msgid_ascii( val->value.block->name, 0 );
        if (!strcasecmp( block_name, translation ))
        {
            free( block_name );
            free( translation );
            return val->value.block;
        }
        free( block_name );
    }
    free( translation );
    return NULL;
}

static int version_value_needs_translation( const ver_value_t *val )
{
    int ret;
    char *key;

    if (val->type != val_str) return 0;
    if (!(key = convert_msgid_ascii( val->key, 0 ))) return 0;

    /* most values contain version numbers or file names, only translate a few specific ones */
    ret = (!strcasecmp( key, "FileDescription" ) || !strcasecmp( key, "ProductName" ));

    free( key );
    return ret;
}

static void add_pot_versioninfo( po_file_t po, const resource_t *res )
{
    ver_value_t *val;
    ver_block_t *langcharset = get_version_langcharset_block( res->res.ver->blocks );

    if (!langcharset) return;
    for (val = langcharset->values; val; val = val->next)
        if (version_value_needs_translation( val ))
            add_po_string( po, val->value.str, NULL, 0 );
}

static void add_po_versioninfo( const resource_t *english, const resource_t *res )
{
    const ver_block_t *langcharset = get_version_langcharset_block( res->res.ver->blocks );
    const ver_block_t *english_langcharset = get_version_langcharset_block( english->res.ver->blocks );
    ver_value_t *val, *english_val;
    po_file_t po = get_po_file( res->res.ver->lvc.language );

    if (!langcharset && !english_langcharset) return;
    val = langcharset->values;
    english_val = english_langcharset->values;
    while (english_val && val)
    {
        if (val->type == val_str)
            add_po_string( po, english_val->value.str, val->value.str, res->res.ver->lvc.language );
        val = val->next;
        english_val = english_val->next;
    }
}

static resource_t *find_english_resource( resource_t *res )
{
    resource_t *ptr;

    for (ptr = resource_top; ptr; ptr = ptr->next)
    {
        if (ptr->type != res->type) continue;
        if (!ptr->lan) continue;
        if (!is_english( ptr->lan )) continue;
        if (compare_name_id( ptr->name, res->name )) continue;
        return ptr;
    }
    return NULL;
}

void write_pot_file( const char *outname )
{
    resource_t *res;
    po_file_t po = create_po_file();

    for (res = resource_top; res; res = res->next)
    {
        if (!is_english( res->lan )) continue;

        switch (res->type)
        {
        case res_acc: add_pot_accel( po, res ); break;
        case res_dlg: add_pot_dialog( po, res ); break;
        case res_men: add_pot_menu( po, res ); break;
        case res_stt: add_pot_stringtable( po, res ); break;
        case res_ver: add_pot_versioninfo( po, res ); break;
        case res_msg: break;  /* FIXME */
        default: break;
        }
    }
    po_file_write( po, outname, &po_xerror_handler );
    po_file_free( po );
}

void write_po_files( const char *outname )
{
    resource_t *res, *english;

    for (res = resource_top; res; res = res->next)
    {
        if (!(english = find_english_resource( res ))) continue;
        switch (res->type)
        {
        case res_acc: add_po_accel( english, res ); break;
        case res_dlg: add_po_dialog( english, res ); break;
        case res_men: add_po_menu( english, res ); break;
        case res_stt: add_po_stringtable( english, res ); break;
        case res_ver: add_po_versioninfo( english, res ); break;
        case res_msg: break;  /* FIXME */
        default: break;
        }
    }
    if (!flush_po_files( outname ))
    {
        if (outname) error( "No translations found for %s\n", outname );
        else error( "No translations found\n" );
    }
}

#endif  /* HAVE_LIBGETTEXTPO */

static struct mo_file *mo_file;

static void byteswap( unsigned int *data, unsigned int count )
{
    unsigned int i;

    for (i = 0; i < count; i++)
        data[i] = data[i] >> 24 | (data[i] >> 8 & 0xff00) | (data[i] << 8 & 0xff0000) | data[i] << 24;
}

static void load_mo_file( const char *name )
{
    size_t size;

    if (!(mo_file = read_file( name, &size ))) fatal_perror( "Failed to read %s", name );

    /* sanity checks */

    if (size < sizeof(*mo_file))
        error( "%s is not a valid .mo file\n", name );
    if (mo_file->magic == 0xde120495)
        byteswap( &mo_file->revision, 4 );
    else if (mo_file->magic != 0x950412de)
        error( "%s is not a valid .mo file\n", name );
    if ((mo_file->revision >> 16) > 1)
        error( "%s: unsupported file version %x\n", name, mo_file->revision );
    if (mo_file->msgid_off >= size ||
        mo_file->msgstr_off >= size ||
        size < sizeof(*mo_file) + 2 * 8 * mo_file->count)
        error( "%s: corrupted file\n", name );

    if (mo_file->magic == 0xde120495)
    {
        byteswap( (unsigned int *)((char *)mo_file + mo_file->msgid_off), 2 * mo_file->count );
        byteswap( (unsigned int *)((char *)mo_file + mo_file->msgstr_off), 2 * mo_file->count );
    }
}

static void free_mo_file(void)
{
    free( mo_file );
    mo_file = NULL;
}

static inline const char *get_mo_msgid( int index )
{
    const char *base = (const char *)mo_file;
    const unsigned int *offsets = (const unsigned int *)(base + mo_file->msgid_off);
    return base + offsets[2 * index + 1];
}

static inline const char *get_mo_msgstr( int index )
{
    const char *base = (const char *)mo_file;
    const unsigned int *offsets = (const unsigned int *)(base + mo_file->msgstr_off);
    return base + offsets[2 * index + 1];
}

static const char *get_msgstr( const char *msgid, const char *context, int *found )
{
    int pos, res, min, max;
    const char *ret = msgid;
    char *id = NULL;

    if (!mo_file)  /* strings containing a context still need to be transformed */
    {
        if (context) (*found)++;
        return ret;
    }

    if (context) id = strmake( "%s%c%s", context, 4, msgid );
    min = 0;
    max = mo_file->count - 1;
    while (min <= max)
    {
        pos = (min + max) / 2;
        res = strcmp( get_mo_msgid(pos), id ? id : msgid );
        if (!res)
        {
            const char *str = get_mo_msgstr( pos );
            if (str[0])  /* ignore empty strings */
            {
                ret = str;
                (*found)++;
            }
            break;
        }
        if (res > 0) max = pos - 1;
        else min = pos + 1;
    }
    free( id );
    return ret;
}

static string_t *translate_string( string_t *str, int *found )
{
    string_t ustr, *new;
    const char *transl;
    char *buffer, *msgid, *context;

    if (!str->size || !(buffer = convert_msgid_ascii( str, 0 )))
        return convert_string_unicode( str, 1252 );

    msgid = buffer;
    context = get_message_context( &msgid );
    transl = get_msgstr( msgid, context, found );

    ustr.type = str_char;
    ustr.size = strlen( transl );
    ustr.str.cstr = (char *)transl;
    ustr.loc = str->loc;

    new = convert_string_unicode( &ustr, CP_UTF8 );
    free( buffer );
    return new;
}

static control_t *translate_controls( control_t *ctrl, int *found )
{
    control_t *new, *head = NULL, *tail = NULL;

    while (ctrl)
    {
        new = xmalloc( sizeof(*new) );
        *new = *ctrl;
        if (control_has_title( ctrl ))
        {
            new->title = new_name_id();
            *new->title = *ctrl->title;
            new->title->name.s_name = translate_string( ctrl->title->name.s_name, found );
        }
        else new->title = dup_name_id( ctrl->title );
        new->ctlclass = dup_name_id( ctrl->ctlclass );
        if (tail) tail->next = new;
        else head = new;
        new->next = NULL;
        new->prev = tail;
        tail = new;
        ctrl = ctrl->next;
    }
    return head;
}

static menu_item_t *translate_items( menu_item_t *item, int *found )
{
    menu_item_t *new, *head = NULL, *tail = NULL;

    while (item)
    {
        new = xmalloc( sizeof(*new) );
        *new = *item;
        if (item->name) new->name = translate_string( item->name, found );
        if (item->popup) new->popup = translate_items( item->popup, found );
        if (tail) tail->next = new;
        else head = new;
        new->next = NULL;
        new->prev = tail;
        tail = new;
        item = item->next;
    }
    return head;
}

static stringtable_t *translate_stringtable( stringtable_t *stt, language_t lang, int *found )
{
    stringtable_t *new, *head = NULL, *tail = NULL;
    int i;

    while (stt)
    {
        new = xmalloc( sizeof(*new) );
        *new = *stt;
        new->lvc.language = lang;
        new->lvc.version = get_dup_version( lang );
        new->entries = xmalloc( new->nentries * sizeof(*new->entries) );
        memcpy( new->entries, stt->entries, new->nentries * sizeof(*new->entries) );
        for (i = 0; i < stt->nentries; i++)
            if (stt->entries[i].str)
                new->entries[i].str = translate_string( stt->entries[i].str, found );

        if (tail) tail->next = new;
        else head = new;
        new->next = NULL;
        new->prev = tail;
        tail = new;
        stt = stt->next;
    }
    return head;
}

static void translate_dialog( dialog_t *dlg, dialog_t *new, int *found )
{
    if (dlg->title) new->title = translate_string( dlg->title, found );
    if (is_rtl_language( new->lvc.language ))
    {
        new->gotexstyle = TRUE;
        if (dlg->gotexstyle)
            new->exstyle = new_style( dlg->exstyle->or_mask | WS_EX_LAYOUTRTL, dlg->exstyle->and_mask );
        else
            new->exstyle = new_style( WS_EX_LAYOUTRTL, 0 );
    }
    if (dlg->font)
    {
        new->font = xmalloc( sizeof(*dlg->font) );
        *new->font = *dlg->font;
        if (uses_larger_font( new->lvc.language )) new->font->size++;
        new->font->name = convert_string_unicode( dlg->font->name, 1252 );
    }
    new->controls = translate_controls( dlg->controls, found );
}

static event_t *translate_accel( accelerator_t *acc, accelerator_t *new, int *found )
{
    event_t *event, *new_ev, *head = NULL, *tail = NULL;

    event = acc->events;
    while (event)
    {
        new_ev = new_event();
        *new_ev = *event;
        if (event->str) new_ev->str = translate_string( event->str, found );
        if (tail) tail->next = new_ev;
        else head = new_ev;
        new_ev->next = NULL;
        new_ev->prev = tail;
        tail = new_ev;
        event = event->next;
    }
    return head;
}

static ver_value_t *translate_langcharset_values( ver_value_t *val, language_t lang, int *found )
{
    ver_value_t *new_val, *head = NULL, *tail = NULL;
    while (val)
    {
        new_val = new_ver_value();
        *new_val = *val;
        if (val->type == val_str)
            new_val->value.str = translate_string( val->value.str, found );
        if (tail) tail->next = new_val;
        else head = new_val;
        new_val->next = NULL;
        new_val->prev = tail;
        tail = new_val;
        val = val->next;
    }
    return head;
}

static ver_value_t *translate_stringfileinfo( ver_value_t *val, language_t lang, int *found )
{
    int i;
    ver_value_t *new_val, *head = NULL, *tail = NULL;
    const char *english_block_name[2] = { "040904b0", "040904e4" };
    char *block_name[2];
    language_t langid = get_default_sublang( lang );

    block_name[0] = strmake( "%04x%04x", langid, 1200 );
    block_name[1] = strmake( "%04x%04x", langid, get_language_codepage( lang ));

    while (val)
    {
        new_val = new_ver_value();
        *new_val = *val;
        if (val->type == val_block)
        {
            ver_block_t *blk, *blk_head = NULL, *blk_tail = NULL;
            for (blk = val->value.block; blk; blk = blk->next)
            {
                ver_block_t *new_blk;
                char *name;
                new_blk = new_ver_block();
                *new_blk = *blk;
                name = convert_msgid_ascii( blk->name, 0 );
                for (i = 0; i < ARRAY_SIZE(block_name); i++)
                {
                    if (!strcasecmp( name, english_block_name[i] ))
                    {
                        string_t str;
                        str.type     = str_char;
                        str.size     = strlen( block_name[i] ) + 1;
                        str.str.cstr = block_name[i];
                        str.loc      = blk->name->loc;
                        new_blk->name   = convert_string_unicode( &str, CP_UTF8 );
                        new_blk->values = translate_langcharset_values( blk->values, lang, found );
                    }
                }
                free( name );
                if (blk_tail) blk_tail->next = new_blk;
                else blk_head = new_blk;
                new_blk->next = NULL;
                new_blk->prev = blk_tail;
                blk_tail = new_blk;
            }
            new_val->value.block = blk_head;
        }
        if (tail) tail->next = new_val;
        else head = new_val;
        new_val->next = NULL;
        new_val->prev = tail;
        tail = new_val;
        val = val->next;
    }

    for (i = 0; i < ARRAY_SIZE(block_name); i++)
        free( block_name[i] );
    return head;
}

static ver_value_t *translate_varfileinfo( ver_value_t *val, language_t lang )
{
    ver_value_t *new_val, *head = NULL, *tail = NULL;

    while (val)
    {
        new_val = new_ver_value();
        *new_val = *val;
        if (val->type == val_words)
        {
            char *key = convert_msgid_ascii( val->key, 0 );
            if (!strcasecmp( key, "Translation" ) &&
                val->value.words->nwords == 2 &&
                val->value.words->words[0] == MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ))
            {
                ver_words_t *new_words;
                int codepage;
                language_t langid = get_default_sublang( lang );
                new_words = new_ver_words( langid );
                if (val->value.words->words[1] == 1200)
                    codepage = 1200;
                else
                    codepage = get_language_codepage( lang );
                new_val->value.words = add_ver_words( new_words, codepage );
            }
            free( key );
        }
        if (tail) tail->next = new_val;
        else head = new_val;
        new_val->next = NULL;
        new_val->prev = tail;
        tail = new_val;
        val = val->next;
    }
    return head;
}

static ver_block_t *translate_versioninfo( ver_block_t *blk, language_t lang, int *found )
{
    ver_block_t *new_blk, *head = NULL, *tail = NULL;
    char *name;

    while (blk)
    {
        new_blk = new_ver_block();
        *new_blk = *blk;
        name = convert_msgid_ascii( blk->name, 0 );
        if (!strcasecmp( name, "stringfileinfo" ))
            new_blk->values = translate_stringfileinfo( blk->values, lang, found );
        else if (!strcasecmp( name, "varfileinfo" ))
            new_blk->values = translate_varfileinfo( blk->values, lang );
        free(name);
        if (tail) tail->next = new_blk;
        else head = new_blk;
        new_blk->next = NULL;
        new_blk->prev = tail;
        tail = new_blk;
        blk = blk->next;
    }
    return head;
}

static void translate_resources( language_t lang )
{
    resource_t *res;

    for (res = resource_top; res; res = res->next)
    {
        resource_t *new = NULL;
        int found = 0;

        if (!is_english( res->lan )) continue;

        switch (res->type)
        {
        case res_acc:
            new = dup_resource( res, lang );
            new->res.acc->events = translate_accel( res->res.acc, new->res.acc, &found );
            break;
        case res_dlg:
            new = dup_resource( res, lang );
            translate_dialog( res->res.dlg, new->res.dlg, &found );
            break;
        case res_men:
            new = dup_resource( res, lang );
            new->res.men->items = translate_items( res->res.men->items, &found );
            break;
        case res_stt:
            new = dup_resource( res, lang );
            new->res.stt = translate_stringtable( res->res.stt, lang, &found );
            break;
        case res_ver:
            new = dup_resource( res, lang );
            new->res.ver->blocks = translate_versioninfo( res->res.ver->blocks, lang, &found );
            break;
        case res_msg:
            /* FIXME */
            break;
        default:
            break;
        }

        if (new && found)
        {
            if (new_tail) new_tail->next = new;
            else new_top = new;
            new->prev = new_tail;
            new_tail = new;
        }
    }
}

/* Unix format is: lang[_country][.charset][@modifier]
 * Windows format is: lang[-script][-country][_modifier] */
static int unix_to_win_locale( const char *unix_name, char *win_name )
{
    static const char sep[] = "_.@";
    const char *extra = NULL;
    char buffer[LOCALE_NAME_MAX_LENGTH];
    char *p, *country = NULL, *modifier = NULL;

    if (strlen( unix_name ) >= LOCALE_NAME_MAX_LENGTH) return FALSE;
    strcpy( buffer, unix_name );
    if (!(p = strpbrk( buffer, sep )))
    {
        strcpy( win_name, buffer );
        return TRUE;
    }

    if (*p == '_')
    {
        *p++ = 0;
        country = p;
        p = strpbrk( p, sep + 1 );
    }
    if (p && *p == '.')
    {
        *p++ = 0;
        /* charset, ignore */
        p = strchr( p, '@' );
    }
    if (p)
    {
        *p++ = 0;
        modifier = p;
    }

    /* rebuild a Windows name */

    strcpy( win_name, buffer );
    if (modifier)
    {
        if (!strcmp( modifier, "arabic" )) strcat( win_name, "-Arab" );
        else if (!strcmp( modifier, "chakma" )) strcat( win_name, "-Cakm" );
        else if (!strcmp( modifier, "cherokee" )) strcat( win_name, "-Cher" );
        else if (!strcmp( modifier, "cyrillic" )) strcat( win_name, "-Cyrl" );
        else if (!strcmp( modifier, "devanagari" )) strcat( win_name, "-Deva" );
        else if (!strcmp( modifier, "gurmukhi" )) strcat( win_name, "-Guru" );
        else if (!strcmp( modifier, "javanese" )) strcat( win_name, "-Java" );
        else if (!strcmp( modifier, "latin" )) strcat( win_name, "-Latn" );
        else if (!strcmp( modifier, "mongolian" )) strcat( win_name, "-Mong" );
        else if (!strcmp( modifier, "syriac" )) strcat( win_name, "-Syrc" );
        else if (!strcmp( modifier, "tifinagh" )) strcat( win_name, "-Tfng" );
        else if (!strcmp( modifier, "tibetan" )) strcat( win_name, "-Tibt" );
        else if (!strcmp( modifier, "vai" )) strcat( win_name, "-Vaii" );
        else if (!strcmp( modifier, "yi" )) strcat( win_name, "-Yiii" );
        else if (!strcmp( modifier, "saaho" )) strcpy( win_name, "ssy" );
        else if (!strcmp( modifier, "valencia" )) extra = "-valencia";
        /* ignore unknown modifiers */
    }
    if (country)
    {
        p = win_name + strlen(win_name);
        *p++ = '-';
        strcpy( p, country );
    }
    if (extra) strcat( win_name, extra );
    return TRUE;
}


void add_translations( const char *po_dir )
{
    resource_t *res;
    char buffer[256];
    char *p, *tok, *name;
    FILE *f;

    /* first check if we have English resources to translate */
    for (res = resource_top; res; res = res->next) if (is_english( res->lan )) break;
    if (!res) return;

    if (!po_dir)  /* run through the translation process to remove msg contexts */
    {
        translate_resources( MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ));
        goto done;
    }

    new_top = new_tail = NULL;

    name = strmake( "%s/LINGUAS", po_dir );
    if (!(f = fopen( name, "r" )))
    {
        free( name );
        return;
    }
    free( name );
    while (fgets( buffer, sizeof(buffer), f ))
    {
        if ((p = strchr( buffer, '#' ))) *p = 0;
        for (tok = strtok( buffer, " \t\r\n" ); tok; tok = strtok( NULL, " \t\r\n" ))
        {
            char locale[LOCALE_NAME_MAX_LENGTH];
            language_t lang;

            if (!unix_to_win_locale( tok, locale ) || !(lang = get_language_from_name( locale )))
                error( "unknown language '%s'\n", tok );

            name = strmake( "%s/%s.mo", po_dir, tok );
            load_mo_file( name );
            translate_resources( lang );
            free_mo_file();
            free( name );
        }
    }
    fclose( f );

done:
    /* prepend the translated resources to the global list */
    if (new_tail)
    {
        new_tail->next = resource_top;
        resource_top->prev = new_tail;
        resource_top = new_top;
    }
}
