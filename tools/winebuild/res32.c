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

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "build.h"

typedef unsigned short WCHAR;

/* Unicode string or integer id */
struct string_id
{
    WCHAR *str;  /* ptr to Unicode string */
    unsigned int len;    /* len in characters */
    unsigned short id;   /* integer id if str is NULL */
};

/* descriptor for a resource */
struct resource
{
    struct string_id type;
    struct string_id name;
    const char      *input_name;
    unsigned int     input_offset;
    const void      *data;
    unsigned int     data_size;
    unsigned int     data_offset;
    unsigned short   mem_options;
    unsigned short   lang;
    unsigned int     version;
};

/* name level of the resource tree */
struct res_name
{
    const struct string_id  *name;         /* name */
    struct resource         *res;          /* resource */
    int                      nb_languages; /* number of languages */
    unsigned int             dir_offset;   /* offset of directory in resource dir */
    unsigned int             name_offset;  /* offset of name in resource dir */
};

/* type level of the resource tree */
struct res_type
{
    const struct string_id  *type;         /* type name */
    struct res_name         *names;        /* names array */
    unsigned int             nb_names;     /* total number of names */
    unsigned int             nb_id_names;  /* number of names that have a numeric id */
    unsigned int             dir_offset;   /* offset of directory in resource dir */
    unsigned int             name_offset;  /* offset of type name in resource dir */
};

/* top level of the resource tree */
struct res_tree
{
    struct res_type *types;                /* types array */
    unsigned int     nb_types;             /* total number of types */
};

/* size of a resource directory with n entries */
#define RESOURCE_DIR_SIZE        (4 * sizeof(unsigned int))
#define RESOURCE_DIR_ENTRY_SIZE  (2 * sizeof(unsigned int))
#define RESOURCE_DATA_ENTRY_SIZE (4 * sizeof(unsigned int))
#define RESDIR_SIZE(n)  (RESOURCE_DIR_SIZE + (n) * RESOURCE_DIR_ENTRY_SIZE)


static inline struct resource *add_resource( DLLSPEC *spec )
{
    spec->resources = xrealloc( spec->resources, (spec->nb_resources + 1) * sizeof(spec->resources[0]) );
    return &spec->resources[spec->nb_resources++];
}

static struct res_name *add_name( struct res_type *type, struct resource *res )
{
    struct res_name *name;
    type->names = xrealloc( type->names, (type->nb_names + 1) * sizeof(*type->names) );
    name = &type->names[type->nb_names++];
    name->name         = &res->name;
    name->res          = res;
    name->nb_languages = 1;
    if (!name->name->str) type->nb_id_names++;
    return name;
}

static struct res_type *add_type( struct res_tree *tree, struct resource *res )
{
    struct res_type *type;
    tree->types = xrealloc( tree->types, (tree->nb_types + 1) * sizeof(*tree->types) );
    type = &tree->types[tree->nb_types++];
    type->type        = &res->type;
    type->names       = NULL;
    type->nb_names    = 0;
    type->nb_id_names = 0;
    return type;
}

/* get a string from the current resource file */
static void get_string( struct string_id *str )
{
    unsigned int i = 0;
    size_t start_pos = input_buffer_pos;
    WCHAR wc = get_word();

    str->len = 0;
    if (wc == 0xffff)
    {
        str->str = NULL;
        str->id = get_word();
    }
    else
    {
        input_buffer_pos = start_pos;
        while (get_word()) str->len++;
        str->str = xmalloc( str->len * sizeof(WCHAR) );
        input_buffer_pos = start_pos;
        while ((wc = get_word())) str->str[i++] = wc;
    }
}

/* put a string into the resource file */
static void put_string( const struct string_id *str )
{
    if (str->str)
    {
        unsigned int i;
        for (i = 0; i < str->len; i++) put_word( str->str[i] );
        put_word( 0 );
    }
    else
    {
        put_word( 0xffff );
        put_word( str->id );
    }
}

/* check the file header */
/* all values must be zero except header size */
static int check_header(void)
{
    unsigned int size;

    if (get_dword()) return 0;        /* data size */
    size = get_dword();               /* header size */
    if (size == 0x20000000) byte_swapped = 1;
    else if (size != 0x20) return 0;
    if (get_word() != 0xffff || get_word()) return 0;  /* type, must be id 0 */
    if (get_word() != 0xffff || get_word()) return 0;  /* name, must be id 0 */
    if (get_dword()) return 0;        /* data version */
    if (get_word()) return 0;         /* mem options */
    if (get_word()) return 0;         /* language */
    if (get_dword()) return 0;        /* version */
    if (get_dword()) return 0;        /* characteristics */
    return 1;
}

/* load the next resource from the current file */
static void load_next_resource( DLLSPEC *spec, const char *name )
{
    unsigned int hdr_size;
    struct resource *res = add_resource( spec );

    res->data_size = get_dword();
    hdr_size = get_dword();
    if (hdr_size & 3) fatal_error( "%s header size not aligned\n", input_buffer_filename );
    if (hdr_size < 32) fatal_error( "%s invalid header size %u\n", input_buffer_filename, hdr_size );

    res->input_name = xstrdup( name );
    res->input_offset = input_buffer_pos - 2*sizeof(unsigned int) + hdr_size;

    res->data = input_buffer + input_buffer_pos - 2*sizeof(unsigned int) + hdr_size;
    if ((const unsigned char *)res->data < input_buffer ||
        (const unsigned char *)res->data >= input_buffer + input_buffer_size)
        fatal_error( "%s invalid header size %u\n", input_buffer_filename, hdr_size );
    get_string( &res->type );
    get_string( &res->name );
    if (input_buffer_pos & 2) get_word();  /* align to dword boundary */
    get_dword();                        /* skip data version */
    res->mem_options = get_word();
    res->lang = get_word();
    res->version = get_dword();
    get_dword();                        /* skip characteristics */

    input_buffer_pos = ((const unsigned char *)res->data - input_buffer) + ((res->data_size + 3) & ~3);
    input_buffer_pos = (input_buffer_pos + 3) & ~3;
    if (input_buffer_pos > input_buffer_size)
        fatal_error( "%s is a truncated file\n", input_buffer_filename );
}

/* load a Win32 .res file */
int load_res32_file( const char *name, DLLSPEC *spec )
{
    int ret;

    init_input_buffer( name );

    if ((ret = check_header()))
    {
        while (input_buffer_pos < input_buffer_size) load_next_resource( spec, name );
    }
    return ret;
}

/* compare two unicode strings/ids */
static int cmp_string( const struct string_id *str1, const struct string_id *str2 )
{
    unsigned int i;

    if (!str1->str)
    {
        if (!str2->str) return str1->id - str2->id;
        return 1;  /* an id compares larger than a string */
    }
    if (!str2->str) return -1;

    for (i = 0; i < str1->len && i < str2->len; i++)
        if (str1->str[i] != str2->str[i]) return str1->str[i] - str2->str[i];
    return str1->len - str2->len;
}

/* compare two resources for sorting the resource directory */
/* resources are stored first by type, then by name, then by language */
static int cmp_res( const void *ptr1, const void *ptr2 )
{
    const struct resource *res1 = ptr1;
    const struct resource *res2 = ptr2;
    int ret;

    if ((ret = cmp_string( &res1->type, &res2->type ))) return ret;
    if ((ret = cmp_string( &res1->name, &res2->name ))) return ret;
    if ((ret = res1->lang - res2->lang)) return ret;
    return res1->version - res2->version;
}

static char *format_res_string( const struct string_id *str )
{
    unsigned int i;
    char *ret;

    if (!str->str) return strmake( "#%04x", str->id );
    ret = xmalloc( str->len + 1 );
    for (i = 0; i < str->len; i++) ret[i] = str->str[i];  /* dumb W->A conversion */
    ret[i] = 0;
    return ret;
}

/* get rid of duplicate resources with different versions */
static void remove_duplicate_resources( DLLSPEC *spec )
{
    unsigned int i, n;

    for (i = n = 0; i < spec->nb_resources; i++, n++)
    {
        if (i && !cmp_string( &spec->resources[i].type, &spec->resources[i-1].type ) &&
            !cmp_string( &spec->resources[i].name, &spec->resources[i-1].name ) &&
            spec->resources[i].lang == spec->resources[i-1].lang)
        {
            if (spec->resources[i].version == spec->resources[i-1].version)
            {
                char *type_str = format_res_string( &spec->resources[i].type );
                char *name_str = format_res_string( &spec->resources[i].name );
                error( "winebuild: duplicate resource type %s name %s language %04x version %08x\n",
                       type_str, name_str, spec->resources[i].lang, spec->resources[i].version );
            }
            else n--;  /* replace the previous one */
        }
        if (n < i) spec->resources[n] = spec->resources[i];
    }
    spec->nb_resources = n;
}

/* build the 3-level (type,name,language) resource tree */
static struct res_tree *build_resource_tree( DLLSPEC *spec, unsigned int *dir_size )
{
    unsigned int i, k, n, offset, data_offset;
    struct res_tree *tree;
    struct res_type *type = NULL;
    struct res_name *name = NULL;
    struct resource *res;

    qsort( spec->resources, spec->nb_resources, sizeof(*spec->resources), cmp_res );
    remove_duplicate_resources( spec );

    tree = xmalloc( sizeof(*tree) );
    tree->types = NULL;
    tree->nb_types = 0;

    for (i = 0; i < spec->nb_resources; i++)
    {
        if (!i || cmp_string( &spec->resources[i].type, &spec->resources[i-1].type ))  /* new type */
        {
            type = add_type( tree, &spec->resources[i] );
            name = add_name( type, &spec->resources[i] );
        }
        else if (cmp_string( &spec->resources[i].name, &spec->resources[i-1].name )) /* new name */
        {
            name = add_name( type, &spec->resources[i] );
        }
        else
        {
            name->nb_languages++;
        }
    }

    /* compute the offsets */

    offset = RESDIR_SIZE( tree->nb_types );
    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        type->dir_offset = offset;
        offset += RESDIR_SIZE( type->nb_names );
        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
        {
            name->dir_offset = offset;
            offset += RESDIR_SIZE( name->nb_languages );
        }
    }
    data_offset = offset;
    offset += spec->nb_resources * RESOURCE_DATA_ENTRY_SIZE;

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        if (type->type->str)
        {
            type->name_offset = offset | 0x80000000;
            offset += (type->type->len + 1) * sizeof(WCHAR);
        }
        else type->name_offset = type->type->id;

        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
        {
            if (name->name->str)
            {
                name->name_offset = offset | 0x80000000;
                offset += (name->name->len + 1) * sizeof(WCHAR);
            }
            else name->name_offset = name->name->id;
            for (k = 0, res = name->res; k < name->nb_languages; k++, res++)
            {
                unsigned int entry_offset = (res - spec->resources) * RESOURCE_DATA_ENTRY_SIZE;
                res->data_offset = data_offset + entry_offset;
            }
        }
    }
    if (dir_size) *dir_size = (offset + 3) & ~3;
    return tree;
}

/* free the resource tree */
static void free_resource_tree( struct res_tree *tree )
{
    unsigned int i;

    for (i = 0; i < tree->nb_types; i++) free( tree->types[i].names );
    free( tree->types );
    free( tree );
}

/* output a Unicode string */
static void output_string( const struct string_id *str )
{
    unsigned int i;
    output( "\t.short 0x%04x", str->len );
    for (i = 0; i < str->len; i++) output( ",0x%04x", str->str[i] );
    output( " /* " );
    for (i = 0; i < str->len; i++) output( "%c", isprint((char)str->str[i]) ? (char)str->str[i] : '?' );
    output( " */\n" );
}

/* output a resource directory */
static inline void output_res_dir( unsigned int nb_names, unsigned int nb_ids )
{
    output( "\t.long 0\n" );  /* Characteristics */
    output( "\t.long 0\n" );  /* TimeDateStamp */
    output( "\t.short 0,0\n" );   /* Major/MinorVersion */
    output( "\t.short %u,%u\n",   /* NumberOfNamed/IdEntries */
             nb_names, nb_ids );
}

/* output the resource definitions */
void output_resources( DLLSPEC *spec )
{
    int k, nb_id_types;
    unsigned int i, n;
    struct res_tree *tree;
    struct res_type *type;
    struct res_name *name;
    const struct resource *res;

    if (!spec->nb_resources) return;

    tree = build_resource_tree( spec, NULL );

    /* output the resource directories */

    output( "\n/* resources */\n\n" );
    output( "\t%s\n", get_asm_rsrc_section() );
    output( "\t.balign %u\n", get_ptr_size() );
    output( ".L__wine_spec_resources:\n" );

    for (i = nb_id_types = 0, type = tree->types; i < tree->nb_types; i++, type++)
        if (!type->type->str) nb_id_types++;

    output_res_dir( tree->nb_types - nb_id_types, nb_id_types );

    /* dump the type directory */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
        output( "\t.long 0x%08x,0x%08x\n",
                 type->name_offset, type->dir_offset | 0x80000000 );

    /* dump the names and languages directories */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        output_res_dir( type->nb_names - type->nb_id_names, type->nb_id_names );
        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
            output( "\t.long 0x%08x,0x%08x\n",
                     name->name_offset, name->dir_offset | 0x80000000 );

        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
        {
            output_res_dir( 0, name->nb_languages );
            for (k = 0, res = name->res; k < name->nb_languages; k++, res++)
                output( "\t.long 0x%08x,0x%08x\n", res->lang, res->data_offset );
        }
    }

    /* dump the resource data entries */

    for (i = 0, res = spec->resources; i < spec->nb_resources; i++, res++)
    {
        output_rva( ".L__wine_spec_res_%d", i );
        output( "\t.long %u,0,0\n", res->data_size );
    }

    /* dump the name strings */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        if (type->type->str) output_string( type->type );
        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
            if (name->name->str) output_string( name->name );
    }

    /* resource data */

    for (i = 0, res = spec->resources; i < spec->nb_resources; i++, res++)
    {
        output( "\n\t.balign 4\n" );
        output( ".L__wine_spec_res_%d:\n", i );
        output( "\t.incbin \"%s\",%d,%d\n", res->input_name, res->input_offset, res->data_size );
    }

    if (!is_pe())
    {
        output( ".L__wine_spec_resources_end:\n" );
        output( "\t.byte 0\n" );
    }
    free_resource_tree( tree );
}

/* output a Unicode string in binary format */
static void output_bin_string( const struct string_id *str )
{
    unsigned int i;

    put_word( str->len );
    for (i = 0; i < str->len; i++) put_word( str->str[i] );
}

/* output a resource directory in binary format */
static inline void output_bin_res_dir( unsigned int nb_names, unsigned int nb_ids )
{
    put_dword( 0 );        /* Characteristics */
    put_dword( 0 );        /* TimeDateStamp */
    put_word( 0 );         /* MajorVersion */
    put_word( 0 );         /* MinorVersion */
    put_word( nb_names );  /* NumberOfNamedEntries */
    put_word( nb_ids );    /* NumberOfIdEntries */
}

/* output the resource definitions in binary format */
void output_bin_resources( DLLSPEC *spec, unsigned int start_rva )
{
    int k, nb_id_types;
    unsigned int i, n, data_offset;
    struct res_tree *tree;
    struct res_type *type;
    struct res_name *name;
    const struct resource *res;

    if (!spec->nb_resources) return;

    tree = build_resource_tree( spec, &data_offset );
    init_output_buffer();

    /* output the resource directories */

    for (i = nb_id_types = 0, type = tree->types; i < tree->nb_types; i++, type++)
        if (!type->type->str) nb_id_types++;

    output_bin_res_dir( tree->nb_types - nb_id_types, nb_id_types );

    /* dump the type directory */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        put_dword( type->name_offset );
        put_dword( type->dir_offset | 0x80000000 );
    }

    /* dump the names and languages directories */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        output_bin_res_dir( type->nb_names - type->nb_id_names, type->nb_id_names );
        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
        {
            put_dword( name->name_offset );
            put_dword( name->dir_offset | 0x80000000 );
        }

        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
        {
            output_bin_res_dir( 0, name->nb_languages );
            for (k = 0, res = name->res; k < name->nb_languages; k++, res++)
            {
                put_dword( res->lang );
                put_dword( res->data_offset );
            }
        }
    }

    /* dump the resource data entries */

    for (i = 0, res = spec->resources; i < spec->nb_resources; i++, res++)
    {
        put_dword( data_offset + start_rva );
        put_dword( res->data_size );
        put_dword( 0 );
        put_dword( 0 );
        data_offset += (res->data_size + 3) & ~3;
    }

    /* dump the name strings */

    for (i = 0, type = tree->types; i < tree->nb_types; i++, type++)
    {
        if (type->type->str) output_bin_string( type->type );
        for (n = 0, name = type->names; n < type->nb_names; n++, name++)
            if (name->name->str) output_bin_string( name->name );
    }

    /* resource data */

    align_output( 4 );
    for (i = 0, res = spec->resources; i < spec->nb_resources; i++, res++)
    {
        put_data( res->data, res->data_size );
        align_output( 4 );
    }

    free_resource_tree( tree );
}

static unsigned int get_resource_header_size( const struct resource *res )
{
    unsigned int size  = 5 * sizeof(unsigned int) + 2 * sizeof(unsigned short);

    if (!res->type.str) size += 2 * sizeof(unsigned short);
    else size += (res->type.len + 1) * sizeof(WCHAR);

    if (!res->name.str) size += 2 * sizeof(unsigned short);
    else size += (res->name.len + 1) * sizeof(WCHAR);

    return size;
}

/* output the resources into a .o file */
void output_res_o_file( DLLSPEC *spec )
{
    unsigned int i;
    char *res_file = NULL;
    struct strarray args;

    if (!spec->nb_resources) fatal_error( "--resources mode needs at least one resource file as input\n" );
    if (!output_file_name) fatal_error( "No output file name specified\n" );

    qsort( spec->resources, spec->nb_resources, sizeof(*spec->resources), cmp_res );
    remove_duplicate_resources( spec );

    byte_swapped = 0;
    init_output_buffer();

    put_dword( 0 );      /* ResSize */
    put_dword( 32 );     /* HeaderSize */
    put_word( 0xffff );  /* ResType */
    put_word( 0x0000 );
    put_word( 0xffff );  /* ResName */
    put_word( 0x0000 );
    put_dword( 0 );      /* DataVersion */
    put_word( 0 );       /* Memory options */
    put_word( 0 );       /* Language */
    put_dword( 0 );      /* Version */
    put_dword( 0 );      /* Characteristics */

    for (i = 0; i < spec->nb_resources; i++)
    {
        unsigned int header_size = get_resource_header_size( &spec->resources[i] );

        put_dword( spec->resources[i].data_size );
        put_dword( (header_size + 3) & ~3 );
        put_string( &spec->resources[i].type );
        put_string( &spec->resources[i].name );
        align_output( 4 );
        put_dword( 0 );
        put_word( spec->resources[i].mem_options );
        put_word( spec->resources[i].lang );
        put_dword( spec->resources[i].version );
        put_dword( 0 );
        put_data( spec->resources[i].data, spec->resources[i].data_size );
        align_output( 4 );
    }

    /* if the output file name is a .res too, don't run the results through windres */
    if (strendswith( output_file_name, ".res"))
    {
        flush_output_buffer( output_file_name );
        return;
    }

    res_file = make_temp_file( output_file_name, ".res" );
    flush_output_buffer( res_file );

    args = find_tool( "windres", NULL );
    strarray_add( &args, "-i" );
    strarray_add( &args, res_file );
    strarray_add( &args, "-o" );
    strarray_add( &args, output_file_name );
    switch (target.cpu)
    {
        case CPU_i386:
            strarray_add( &args, "-F" );
            strarray_add( &args, "pe-i386" );
            break;
        case CPU_x86_64:
            strarray_add( &args, "-F" );
            strarray_add( &args, "pe-x86-64" );
            break;
        default:
            break;
    }
    spawn( args );
}
