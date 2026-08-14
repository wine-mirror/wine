/*
 * Support for po files
 *
 * Copyright 2010, 2011 Alexandre Julliard
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
#ifdef HAVE_GETTEXT_PO_H
#include <gettext-po.h>
#endif

#include "wmc.h"
#include "utils.h"
#include "lang.h"
#include "write.h"
#include "windef.h"

struct mo_file
{
    unsigned int magic;
    unsigned int revision;
    unsigned int count;
    unsigned int msgid_off;
    unsigned int msgstr_off;
    /* ... rest of file data here */
};

static struct lan_blk *new_top, *new_tail;

static BOOL is_english( int lan )
{
    return lan == MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT );
}

static char *convert_msgid_ascii( const struct lanmsg *msg, int error_on_invalid_char )
{
    int i;
    char *buffer = xmalloc( msg->len * 4 + 1 );

    for (i = 0; i < msg->len; i++)
    {
        buffer[i] = msg->msg[i];
        if (!msg->msg[i]) break;
        if (msg->msg[i] >= 32 && msg->msg[i] <= 127) continue;
        if (msg->msg[i] == '\t' || msg->msg[i] == '\n') continue;
        if (error_on_invalid_char)
        {
            fprintf( stderr, "%s:%d: ", msg->file, msg->line );
            error( "Invalid character %04x in source string\n", msg->msg[i] );
        }
        free( buffer );
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

#ifdef HAVE_LIBGETTEXTPO

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

static void add_po_string( po_file_t po, const struct lanmsg *msgid, const struct lanmsg *msgstr )
{
    po_message_t msg;
    po_message_iterator_t iterator;
    char *id, *id_buffer, *context, *str = NULL, *str_buffer = NULL;

    if (msgid->len <= 1) return;

    id_buffer = id = convert_msgid_ascii( msgid, 1 );
    context = get_message_context( &id );

    if (msgstr)
    {
        int len;
        str_buffer = str = unicode_to_utf8( msgstr->msg, msgstr->len, &len );
        if (is_english( msgstr->lan )) get_message_context( &str );
    }
    if (!(msg = find_message( po, id, context, &iterator )))
    {
        msg = po_message_create();
        po_message_set_msgid( msg, id );
        po_message_set_msgstr( msg, str ? str : "" );
        if (context) po_message_set_msgctxt( msg, context );
        po_message_insert( iterator, msg );
    }
    if (msgid->file) po_message_add_filepos( msg, msgid->file, msgid->line );
    po_message_iterator_free( iterator );
    free( id_buffer );
    free( str_buffer );
}

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

void write_pot_file( const char *outname )
{
    int i, j;
    struct lan_blk *lbp;
    po_file_t po = create_po_file();

    for (lbp = lanblockhead; lbp; lbp = lbp->next)
    {
        if (!is_english( lbp->lan )) continue;
        for (i = 0; i < lbp->nblk; i++)
        {
            struct block *blk = &lbp->blks[i];
            for (j = 0; j < blk->nmsg; j++) add_po_string( po, blk->msgs[j], NULL );
        }
    }
    po_file_write( po, outname, &po_xerror_handler );
    po_file_free( po );
}


#else  /* HAVE_LIBGETTEXTPO */

void write_pot_file( const char *outname )
{
    error( "PO files not supported in this wmc build\n" );
}

#endif

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

static struct lanmsg *translate_string( struct lanmsg *str, int lang, int *found )
{
    struct lanmsg *new;
    const char *transl;
    char *buffer, *msgid, *context;

    if (str->len <= 1 || !(buffer = convert_msgid_ascii( str, 0 ))) return str;

    msgid = buffer;
    context = get_message_context( &msgid );
    transl = get_msgstr( msgid, context, found );

    new = xmalloc( sizeof(*new) );
    new->lan  = lang;
    new->cp   = 0;  /* FIXME */
    new->file = str->file;
    new->line = str->line;
    new->msg  = utf8_to_unicode( transl, strlen(transl) + 1, &new->len );
    free( buffer );
    return new;
}

static void translate_block( struct block *blk, struct block *new, int lang, int *found )
{
    int i;

    new->idlo = blk->idlo;
    new->idhi = blk->idhi;
    new->size = 0;
    new->msgs = xmalloc( blk->nmsg * sizeof(*new->msgs) );
    new->nmsg = blk->nmsg;
    for (i = 0; i < blk->nmsg; i++)
    {
        new->msgs[i] = translate_string( blk->msgs[i], lang, found );
        new->size += ((2 * new->msgs[i]->len + 3) & ~3) + 4;
    }
}

static void translate_messages( int lang )
{
    int i, found;
    struct lan_blk *lbp, *new;

    for (lbp = lanblockhead; lbp; lbp = lbp->next)
    {
        if (!is_english( lbp->lan )) continue;
        found = 0;
        new = xmalloc( sizeof(*new) );
        /* English "translations" take precedence over the original contents */
        new->version = is_english( lang ) ? 1 : -1;
        new->lan = lang;
        new->blks = xmalloc( lbp->nblk * sizeof(*new->blks) );
        new->nblk = lbp->nblk;

        for (i = 0; i < lbp->nblk; i++)
            translate_block( &lbp->blks[i], &new->blks[i], lang, &found );
        if (found)
        {
            if (new_tail) new_tail->next = new;
            else new_top = new;
            new->prev = new_tail;
            new_tail = new;
        }
        else
        {
            free( new->blks );
            free( new );
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
    struct lan_blk *lbp;
    char buffer[256];
    char *p, *tok, *name;
    FILE *f;

    /* first check if we have English resources to translate */
    for (lbp = lanblockhead; lbp; lbp = lbp->next) if (is_english( lbp->lan )) break;
    if (!lbp) return;

    if (!po_dir)  /* run through the translation process to remove msg contexts */
    {
        translate_messages( MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ));
        goto done;
    }

    new_top = new_tail = NULL;

    name = strmake( "%s/LINGUAS", po_dir );
    if (!(f = fopen( name, "r" ))) return;
    free( name );
    while (fgets( buffer, sizeof(buffer), f ))
    {
        if ((p = strchr( buffer, '#' ))) *p = 0;
        for (tok = strtok( buffer, " \t\r\n" ); tok; tok = strtok( NULL, " \t\r\n" ))
        {
            char locale[LOCALE_NAME_MAX_LENGTH];
            unsigned int lang;

            if (!unix_to_win_locale( tok, locale ) || !(lang = get_language_from_name( locale )))
            {
                error( "unknown language '%s'\n", tok );
                continue;
            }
            name = strmake( "%s/%s.mo", po_dir, tok );
            load_mo_file( name );
            translate_messages( lang );
            free_mo_file();
            free( name );
        }
    }
    fclose( f );

done:
    /* prepend the translated messages to the global list */
    if (new_tail)
    {
        new_tail->next = lanblockhead;
        lanblockhead->prev = new_tail;
        lanblockhead = new_top;
    }
}
