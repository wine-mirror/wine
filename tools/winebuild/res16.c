/*
 * Builtin dlls resource support
 *
 * Copyright 2000 Alexandre Julliard
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
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <fcntl.h>

#include "build.h"

/* Unicode string or integer id */
struct string_id
{
    char  *str;  /* ptr to string */
    unsigned short id;   /* integer id if str is NULL */
};

/* descriptor for a resource */
struct resource
{
    struct string_id type;
    struct string_id name;
    const void      *data;
    unsigned int     data_size;
    unsigned int     memopt;
};

/* type level of the resource tree */
struct res_type
{
    const struct string_id  *type;         /* type name */
    const struct resource   *res;          /* first resource of this type */
    unsigned int             nb_names;     /* total number of names */
};

/* top level of the resource tree */
struct res_tree
{
    struct res_type *types;                /* types array */
    unsigned int     nb_types;             /* total number of types */
};


static inline struct resource *add_resource( DLLSPEC *spec )
{
    spec->resources = xrealloc( spec->resources, (spec->nb_resources + 1) * sizeof(*spec->resources) );
    return &spec->resources[spec->nb_resources++];
}

static struct res_type *add_type( struct res_tree *tree, const struct resource *res )
{
    struct res_type *type;
    tree->types = xrealloc( tree->types, (tree->nb_types + 1) * sizeof(*tree->types) );
    type = &tree->types[tree->nb_types++];
    type->type        = &res->type;
    type->res         = res;
    type->nb_names    = 0;
    return type;
}

/* get a string from the current resource file */
static void get_string( struct string_id *str )
{
    unsigned char c = get_byte();

    if (c == 0xff)
    {
        str->str = NULL;
        str->id = get_word();
    }
    else
    {
        str->str = (char *)input_buffer + input_buffer_pos - 1;
        str->id = 0;
        while (get_byte()) /* nothing */;
    }
}

/* load the next resource from the current file */
static void load_next_resource( DLLSPEC *spec )
{
    struct resource *res = add_resource( spec );

    get_string( &res->type );
    get_string( &res->name );
    res->memopt    = get_word();
    res->data_size = get_dword();
    res->data      = input_buffer + input_buffer_pos;
    input_buffer_pos += res->data_size;
    if (input_buffer_pos > input_buffer_size)
        fatal_error( "%s is a truncated/corrupted file\n", input_buffer_filename );
}

/* load a Win16 .res file */
void load_res16_file( const char *name, DLLSPEC *spec )
{
    init_input_buffer( name );
    while (input_buffer_pos < input_buffer_size) load_next_resource( spec );
}

/* compare two strings/ids */
static int cmp_string( const struct string_id *str1, const struct string_id *str2 )
{
    if (!str1->str)
    {
        if (!str2->str) return str1->id - str2->id;
        return 1;  /* an id compares larger than a string */
    }
    if (!str2->str) return -1;
    return strcasecmp( str1->str, str2->str );
}

/* compare two resources for sorting the resource directory */
/* resources are stored first by type, then by name */
static int cmp_res( const void *ptr1, const void *ptr2 )
{
    const struct resource *res1 = ptr1;
    const struct resource *res2 = ptr2;
    int ret;

    if ((ret = cmp_string( &res1->type, &res2->type ))) return ret;
    return cmp_string( &res1->name, &res2->name );
}

/* build the 2-level (type,name) resource tree */
static struct res_tree *build_resource_tree( DLLSPEC *spec )
{
    unsigned int i;
    struct res_tree *tree;
    struct res_type *type = NULL;

    qsort( spec->resources, spec->nb_resources, sizeof(*spec->resources), cmp_res );

    tree = xmalloc( sizeof(*tree) );
    tree->types = NULL;
    tree->nb_types = 0;

    for (i = 0; i < spec->nb_resources; i++)
    {
        if (!i || cmp_string( &spec->resources[i].type, &spec->resources[i-1].type ))  /* new type */
            type = add_type( tree, &spec->resources[i] );
        type->nb_names++;
    }
    return tree;
}

/* free the resource tree */
static void free_resource_tree( struct res_tree *tree )
{
    free( tree->types );
    free( tree );
}

/* output a string preceded by its length */
static void output_string( const char *str )
{
    unsigned int i, len = strlen(str);
    output( "\t.byte 0x%02x", len );
    for (i = 0; i < len; i++) output( ",0x%02x", (unsigned char)str[i] );
    output( " /* %s */\n", str );
}

/* output the resource data */
void output_res16_data( DLLSPEC *spec )
{
    const struct resource *res;
    unsigned int i;

    if (!spec->nb_resources) return;

    for (i = 0, res = spec->resources; i < spec->nb_resources; i++, res++)
    {
        output( ".L__wine_spec_resource_%u:\n", i );
        dump_bytes( res->data, res->data_size );
        output( ".L__wine_spec_resource_%u_end:\n", i );
    }
}

/* output the resource definitions */
void output_res16_directory( DLLSPEC *spec )
{
    unsigned int i, j;
    struct res_tree *tree;
    const struct res_type *type;
    const struct resource *res;

    tree = build_resource_tree( spec );

    output( "\n.L__wine_spec_ne_rsrctab:\n" );
    output( "\t%s 0\n", get_asm_short_keyword() );  /* alignment */

    /* type and name structures */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        if (type->type->str)
            output( "\t%s .L__wine_spec_restype_%u-.L__wine_spec_ne_rsrctab\n",
                     get_asm_short_keyword(), i );
        else
            output( "\t%s 0x%04x\n", get_asm_short_keyword(), type->type->id | 0x8000 );

        output( "\t%s %u,0,0\n", get_asm_short_keyword(), type->nb_names );

        for (j = 0, res = type->res; j < type->nb_names; j++, res++)
        {
            output( "\t%s .L__wine_spec_resource_%lu-.L__wine_spec_dos_header\n",
                     get_asm_short_keyword(), (unsigned long)(res - spec->resources) );
            output( "\t%s .L__wine_spec_resource_%lu_end-.L__wine_spec_resource_%lu\n",
                     get_asm_short_keyword(), (unsigned long)(res - spec->resources),
                     (unsigned long)(res - spec->resources) );
            output( "\t%s 0x%04x\n", get_asm_short_keyword(), res->memopt );
            if (res->name.str)
                output( "\t%s .L__wine_spec_resname_%u_%u-.L__wine_spec_ne_rsrctab\n",
                         get_asm_short_keyword(), i, j );
            else
                output( "\t%s 0x%04x\n", get_asm_short_keyword(), res->name.id | 0x8000 );

            output( "\t%s 0,0\n", get_asm_short_keyword() );
        }
    }
    output( "\t%s 0\n", get_asm_short_keyword() );  /* terminator */

    /* name strings */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        if (type->type->str)
        {
            output( ".L__wine_spec_restype_%u:\n", i );
            output_string( type->type->str );
        }
        for (j = 0, res = type->res; j < type->nb_names; j++, res++)
        {
            if (res->name.str)
            {
                output( ".L__wine_spec_resname_%u_%u:\n", i, j );
                output_string( res->name.str );
            }
        }
    }
    output( "\t.byte 0\n" );  /* names terminator */

    free_resource_tree( tree );
}
