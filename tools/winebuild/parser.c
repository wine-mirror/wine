/*
 * Spec file parser
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997, 2004 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build.h"

int current_line = 0;

static char ParseBuffer[512];
static char TokenBuffer[512];
static char *ParseNext = ParseBuffer;
static FILE *input_file;

static const char *separator_chars;
static const char *comment_chars;

/* valid characters in ordinal names */
static const char valid_ordname_chars[] = "/$:-_@?<>abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static const char * const TypeNames[TYPE_NBTYPES] =
{
    "variable",     /* TYPE_VARIABLE */
    "pascal",       /* TYPE_PASCAL */
    "equate",       /* TYPE_ABS */
    "stub",         /* TYPE_STUB */
    "stdcall",      /* TYPE_STDCALL */
    "cdecl",        /* TYPE_CDECL */
    "varargs",      /* TYPE_VARARGS */
    "extern"        /* TYPE_EXTERN */
};

static const char * const FlagNames[] =
{
    "norelay",     /* FLAG_NORELAY */
    "noname",      /* FLAG_NONAME */
    "ret16",       /* FLAG_RET16 */
    "ret64",       /* FLAG_RET64 */
    "register",    /* FLAG_REGISTER */
    "private",     /* FLAG_PRIVATE */
    "ordinal",     /* FLAG_ORDINAL */
    "thiscall",    /* FLAG_THISCALL */
    "fastcall",    /* FLAG_FASTCALL */
    "syscall",     /* FLAG_SYSCALL */
    "import",      /* FLAG_IMPORT */
    NULL
};

static const char * const ArgNames[ARG_MAXARG + 1] =
{
    "word",    /* ARG_WORD */
    "s_word",  /* ARG_SWORD */
    "segptr",  /* ARG_SEGPTR */
    "segstr",  /* ARG_SEGSTR */
    "long",    /* ARG_LONG */
    "ptr",     /* ARG_PTR */
    "str",     /* ARG_STR */
    "wstr",    /* ARG_WSTR */
    "int64",   /* ARG_INT64 */
    "int128",  /* ARG_INT128 */
    "float",   /* ARG_FLOAT */
    "double"   /* ARG_DOUBLE */
};

static int IsNumberString(const char *s)
{
    while (*s) if (!isdigit(*s++)) return 0;
    return 1;
}

static inline int is_token_separator( char ch )
{
    return strchr( separator_chars, ch ) != NULL;
}

static inline int is_token_comment( char ch )
{
    return strchr( comment_chars, ch ) != NULL;
}

/* get the next line from the input file, or return 0 if at eof */
static int get_next_line(void)
{
    ParseNext = ParseBuffer;
    current_line++;
    return (fgets(ParseBuffer, sizeof(ParseBuffer), input_file) != NULL);
}

static const char * GetToken( int allow_eol )
{
    char *p;
    char *token = TokenBuffer;

    for (;;)
    {
        /* remove initial white space */
        p = ParseNext;
        while (isspace(*p)) p++;

        if (*p == '\\' && p[1] == '\n')  /* line continuation */
        {
            if (!get_next_line())
            {
                if (!allow_eol) error( "Unexpected end of file\n" );
                return NULL;
            }
        }
        else break;
    }

    if ((*p == '\0') || is_token_comment(*p))
    {
        if (!allow_eol) error( "Declaration not terminated properly\n" );
        return NULL;
    }

    /*
     * Find end of token.
     */
    if (is_token_separator(*p))
    {
        /* a separator is always a complete token */
        *token++ = *p++;
    }
    else while (*p != '\0' && !is_token_separator(*p) && !isspace(*p))
    {
        if (*p == '\\') p++;
        if (*p) *token++ = *p++;
    }
    *token = '\0';
    ParseNext = p;
    return TokenBuffer;
}


static ORDDEF *add_entry_point( DLLSPEC *spec )
{
    ORDDEF *ret;

    if (spec->nb_entry_points == spec->alloc_entry_points)
    {
        spec->alloc_entry_points += 128;
        spec->entry_points = xrealloc( spec->entry_points,
                                       spec->alloc_entry_points * sizeof(*spec->entry_points) );
    }
    ret = &spec->entry_points[spec->nb_entry_points++];
    memset( ret, 0, sizeof(*ret) );
    return ret;
}

/*******************************************************************
 *         parse_spec_variable
 *
 * Parse a variable definition in a .spec file.
 */
static int parse_spec_variable( ORDDEF *odp, DLLSPEC *spec )
{
    char *endptr;
    unsigned int *value_array;
    int n_values;
    int value_array_size;
    const char *token;

    if (spec->type == SPEC_WIN32)
    {
        error( "'variable' not supported in Win32, use 'extern' instead\n" );
        return 0;
    }

    if (!(token = GetToken(0))) return 0;
    if (*token != '(')
    {
        error( "Expected '(' got '%s'\n", token );
        return 0;
    }

    n_values = 0;
    value_array_size = 25;
    value_array = xmalloc(sizeof(*value_array) * value_array_size);

    for (;;)
    {
        if (!(token = GetToken(0)))
        {
            free( value_array );
            return 0;
        }
	if (*token == ')')
	    break;

	value_array[n_values++] = strtoul(token, &endptr, 0);
	if (n_values == value_array_size)
	{
	    value_array_size += 25;
	    value_array = xrealloc(value_array,
				   sizeof(*value_array) * value_array_size);
	}

	if (endptr == NULL || *endptr != '\0')
        {
            error( "Expected number value, got '%s'\n", token );
            free( value_array );
            return 0;
        }
    }

    odp->u.var.n_values = n_values;
    odp->u.var.values = xrealloc(value_array, sizeof(*value_array) * n_values);
    return 1;
}


/*******************************************************************
 *         parse_spec_arguments
 *
 * Parse the arguments of an entry point.
 */
static int parse_spec_arguments( ORDDEF *odp, DLLSPEC *spec, int optional )
{
    const char *token;
    unsigned int i, arg;
    int is_win32 = (spec->type == SPEC_WIN32) || (odp->flags & FLAG_EXPORT32);

    odp->u.func.nb_args = 0;

    if (!(token = GetToken( optional ))) return optional;
    if (*token != '(')
    {
        error( "Expected '(' got '%s'\n", token );
        return 0;
    }

    for (i = 0; i < MAX_ARGUMENTS; i++)
    {
        if (!(token = GetToken(0))) return 0;
	if (*token == ')')
	    break;

        for (arg = 0; arg <= ARG_MAXARG; arg++)
            if (!strcmp( ArgNames[arg], token )) break;

        if (arg > ARG_MAXARG)
        {
            error( "Unknown argument type '%s'\n", token );
            return 0;
        }
        switch (arg)
        {
        case ARG_WORD:
        case ARG_SWORD:
        case ARG_SEGPTR:
        case ARG_SEGSTR:
            if (!is_win32) break;
            error( "Argument type '%s' only allowed for Win16\n", token );
            return 0;
        case ARG_FLOAT:
        case ARG_DOUBLE:
            if (!(odp->flags & FLAG_SYSCALL)) break;
            error( "Argument type '%s' not allowed for syscall function\n", token );
            return 0;
        }
        odp->u.func.args[i] = arg;
    }
    if (*token != ')')
    {
        error( "Too many arguments\n" );
        return 0;
    }

    odp->u.func.nb_args = i;
    if (odp->flags & FLAG_THISCALL)
    {
        if (odp->type != TYPE_STDCALL)
        {
            error( "A thiscall function must use the stdcall convention\n" );
            return 0;
        }
        if (!i || odp->u.func.args[0] != ARG_PTR)
        {
            error( "First argument of a thiscall function must be a pointer\n" );
            return 0;
        }
    }
    if (odp->flags & FLAG_FASTCALL)
    {
        if (odp->type != TYPE_STDCALL)
        {
            error( "A fastcall function must use the stdcall convention\n" );
            return 0;
        }
        if ((i && odp->u.func.args[0] != ARG_PTR && odp->u.func.args[0] != ARG_LONG) ||
            (i > 1 && odp->u.func.args[1] != ARG_PTR && odp->u.func.args[1] != ARG_LONG))
            odp->flags |= FLAG_NORELAY;  /* no relay debug possible for non-standard fastcall args */
    }
    if (odp->flags & FLAG_SYSCALL)
    {
        if (odp->type != TYPE_STDCALL && odp->type != TYPE_STUB)
        {
            error( "A syscall function must use the stdcall convention\n" );
            return 0;
        }
    }
    return 1;
}


/*******************************************************************
 *         parse_spec_export
 *
 * Parse an exported function definition in a .spec file.
 */
static int parse_spec_export( ORDDEF *odp, DLLSPEC *spec )
{
    const char *token;
    int is_win32 = (spec->type == SPEC_WIN32) || (odp->flags & FLAG_EXPORT32);

    if (!is_win32 && odp->type == TYPE_STDCALL)
    {
        error( "'stdcall' not supported for Win16\n" );
        return 0;
    }
    if (is_win32 && odp->type == TYPE_PASCAL)
    {
        error( "'pascal' not supported for Win32\n" );
        return 0;
    }

    if (!parse_spec_arguments( odp, spec, 0 )) return 0;

    if (odp->type == TYPE_VARARGS)
        odp->flags |= FLAG_NORELAY;  /* no relay debug possible for varags entry point */

    if (target.cpu != CPU_i386)
        odp->flags &= ~(FLAG_THISCALL | FLAG_FASTCALL);

    if (!(token = GetToken(1)))
    {
        if (!strcmp( odp->name, "@" ))
        {
            error( "Missing handler name for anonymous function\n" );
            return 0;
        }
        odp->link_name = xstrdup( odp->name );
    }
    else
    {
        odp->link_name = xstrdup( token );
        if (strchr( odp->link_name, '.' ))
        {
            if (!is_win32)
            {
                error( "Forwarded functions not supported for Win16\n" );
                return 0;
            }
            odp->flags |= FLAG_FORWARD;
        }
    }
    return 1;
}


/*******************************************************************
 *         parse_spec_equate
 *
 * Parse an 'equate' definition in a .spec file.
 */
static int parse_spec_equate( ORDDEF *odp, DLLSPEC *spec )
{
    char *endptr;
    int value;
    const char *token;

    if (spec->type == SPEC_WIN32)
    {
        error( "'equate' not supported for Win32\n" );
        return 0;
    }
    if (!(token = GetToken(0))) return 0;
    value = strtol(token, &endptr, 0);
    if (endptr == NULL || *endptr != '\0')
    {
        error( "Expected number value, got '%s'\n", token );
        return 0;
    }
    if (value < -0x8000 || value > 0xffff)
    {
        error( "Value %d for absolute symbol doesn't fit in 16 bits\n", value );
        value = 0;
    }
    odp->u.abs.value = value;
    return 1;
}


/*******************************************************************
 *         parse_spec_stub
 *
 * Parse a 'stub' definition in a .spec file
 */
static int parse_spec_stub( ORDDEF *odp, DLLSPEC *spec )
{
    const char *token;

    if (!parse_spec_arguments( odp, spec, 1 )) return 0;

    if (!(token = GetToken(1)))
        odp->link_name = xstrdup( odp->name );
    else
        odp->link_name = xstrdup( token );

    return 1;
}


/*******************************************************************
 *         parse_spec_extern
 *
 * Parse an 'extern' definition in a .spec file.
 */
static int parse_spec_extern( ORDDEF *odp, DLLSPEC *spec )
{
    const char *token;

    if (spec->type == SPEC_WIN16)
    {
        error( "'extern' not supported for Win16, use 'variable' instead\n" );
        return 0;
    }
    if (!(token = GetToken(1)))
    {
        if (!strcmp( odp->name, "@" ))
        {
            error( "Missing handler name for anonymous extern\n" );
            return 0;
        }
        odp->link_name = xstrdup( odp->name );
    }
    else
    {
        odp->link_name = xstrdup( token );
        if (strchr( odp->link_name, '.' )) odp->flags |= FLAG_FORWARD;
    }
    return 1;
}


/*******************************************************************
 *         parse_spec_flags
 *
 * Parse the optional flags for an entry point in a .spec file.
 */
static const char *parse_spec_flags( DLLSPEC *spec, ORDDEF *odp, const char *token )
{
    unsigned int i, cpu_mask = 0;

    do
    {
        token++;
        if (!strncmp( token, "arch=", 5))
        {
            char *args = xstrdup( token + 5 );
            char *cpu_name = strtok( args, "," );
            while (cpu_name)
            {
                if (!strcmp( cpu_name, "win32" ))
                {
                    if (spec->type == SPEC_WIN32)
                        odp->flags |= FLAG_CPU_WIN32;
                    else
                        odp->flags |= FLAG_EXPORT32;
                }
                else if (!strcmp( cpu_name, "win64" ))
                    odp->flags |= FLAG_CPU_WIN64;
                else
                {
                    int cpu = get_cpu_from_name( cpu_name + (cpu_name[0] == '!') );
                    if (cpu == -1)
                    {
                        error( "Unknown architecture '%s'\n", cpu_name );
                        return NULL;
                    }
                    if (cpu_name[0] == '!') cpu_mask |= FLAG_CPU( cpu );
                    else odp->flags |= FLAG_CPU( cpu );
                }
                cpu_name = strtok( NULL, "," );
            }
            free( args );
        }
        else if (!strncmp( token, "syscall=", 8 ))
        {
            char *end;
            unsigned int id = strtoul( token + 8, &end, 0 );

            if (*end || id >= 0x4000)
                error( "Invalid syscall number '%s', should be in range 0-0x3fff\n", token + 8 );
            odp->flags |= FLAG_SYSCALL;
        }
        else if (!strcmp( token, "i386" ))  /* backwards compatibility */
        {
            odp->flags |= FLAG_CPU(CPU_i386);
        }
        else
        {
            for (i = 0; FlagNames[i]; i++)
                if (!strcmp( FlagNames[i], token )) break;
            if (!FlagNames[i])
            {
                error( "Unknown flag '%s'\n", token );
                return NULL;
            }
            switch (1 << i)
            {
            case FLAG_RET16:
            case FLAG_REGISTER:
                if (spec->type == SPEC_WIN32)
                    error( "Flag '%s' is not supported in Win32\n", FlagNames[i] );
                break;
            case FLAG_RET64:
            case FLAG_THISCALL:
            case FLAG_FASTCALL:
                if (spec->type == SPEC_WIN16)
                    error( "Flag '%s' is not supported in Win16\n", FlagNames[i] );
                break;
            }
            odp->flags |= 1 << i;
        }
        token = GetToken(0);
    } while (token && *token == '-');

    /* x86-64 implies arm64ec */
    if (odp->flags & FLAG_CPU(CPU_x86_64)) odp->flags |= FLAG_CPU(CPU_ARM64EC);
    if (cpu_mask & FLAG_CPU(CPU_x86_64)) cpu_mask |= FLAG_CPU(CPU_ARM64EC);

    if (cpu_mask) odp->flags |= FLAG_CPU_MASK & ~cpu_mask;
    return token;
}


/*******************************************************************
 *         parse_spec_ordinal
 *
 * Parse an ordinal definition in a .spec file.
 */
static int parse_spec_ordinal( int ordinal, DLLSPEC *spec )
{
    const char *token;
    size_t len;
    ORDDEF *odp = add_entry_point( spec );

    if (!(token = GetToken(0))) goto error;

    for (odp->type = 0; odp->type < TYPE_NBTYPES; odp->type++)
        if (TypeNames[odp->type] && !strcmp( token, TypeNames[odp->type] ))
            break;

    if (odp->type >= TYPE_NBTYPES)
    {
        if (!strcmp( token, "thiscall" )) /* for backwards compatibility */
        {
            odp->type = TYPE_STDCALL;
            odp->flags |= FLAG_THISCALL;
        }
        else
        {
            error( "Expected type after ordinal, found '%s' instead\n", token );
            goto error;
        }
    }

    if (!(token = GetToken(0))) goto error;
    if (*token == '-' && !(token = parse_spec_flags( spec, odp, token ))) goto error;

    if (ordinal == -1 && spec->type != SPEC_WIN32 && !(odp->flags & FLAG_EXPORT32))
    {
        error( "'@' ordinals not supported for Win16\n" );
        goto error;
    }

    odp->name = xstrdup( token );
    odp->lineno = current_line;
    odp->ordinal = ordinal;

    len = strspn( odp->name, valid_ordname_chars );
    if (len < strlen( odp->name ))
    {
        error( "Character '%c' is not allowed in exported name '%s'\n", odp->name[len], odp->name );
        goto error;
    }

    switch(odp->type)
    {
    case TYPE_VARIABLE:
        if (!parse_spec_variable( odp, spec )) goto error;
        break;
    case TYPE_PASCAL:
    case TYPE_STDCALL:
    case TYPE_VARARGS:
    case TYPE_CDECL:
        if (!parse_spec_export( odp, spec )) goto error;
        break;
    case TYPE_ABS:
        if (!parse_spec_equate( odp, spec )) goto error;
        break;
    case TYPE_STUB:
        if (!parse_spec_stub( odp, spec )) goto error;
        break;
    case TYPE_EXTERN:
        if (!parse_spec_extern( odp, spec )) goto error;
        break;
    default:
        assert( 0 );
    }

    if (data_only && !(odp->flags & FLAG_FORWARD))
    {
        error( "Only forwarded entry points are allowed in data-only mode\n" );
        goto error;
    }

    if (ordinal != -1)
    {
        if (!ordinal)
        {
            error( "Ordinal 0 is not valid\n" );
            goto error;
        }
        if (ordinal >= MAX_ORDINALS)
        {
            error( "Ordinal number %d too large\n", ordinal );
            goto error;
        }
        odp->ordinal = ordinal;
    }

    if (odp->type == TYPE_STDCALL && !(odp->flags & FLAG_PRIVATE))
    {
        if (!strcmp( odp->name, "DllRegisterServer" ) ||
            !strcmp( odp->name, "DllUnregisterServer" ) ||
            !strcmp( odp->name, "DllMain" ) ||
            !strcmp( odp->name, "DllGetClassObject" ) ||
            !strcmp( odp->name, "DllGetVersion" ) ||
            !strcmp( odp->name, "DllInstall" ) ||
            !strcmp( odp->name, "DllCanUnloadNow" ))
        {
            warning( "Function %s should be marked private\n", odp->name );
            if (strcmp( odp->name, odp->link_name ))
                warning( "Function %s should not use a different internal name (%s)\n",
                         odp->name, odp->link_name );
        }
    }

    if (!strcmp( odp->name, "@" ) || odp->flags & (FLAG_NONAME | FLAG_ORDINAL))
    {
        if (ordinal == -1)
        {
            if (!strcmp( odp->name, "@" ))
                error( "Nameless function needs an explicit ordinal number\n" );
            else
                error( "Function imported by ordinal needs an explicit ordinal number\n" );
            goto error;
        }
        if (spec->type != SPEC_WIN32)
        {
            error( "Nameless functions not supported for Win16\n" );
            goto error;
        }
        if (!strcmp( odp->name, "@" ))
        {
            free( odp->name );
            odp->name = NULL;
        }
        else if (!(odp->flags & FLAG_ORDINAL))  /* -ordinal only affects the import library */
        {
            odp->export_name = odp->name;
            odp->name = NULL;
        }
    }
    return 1;

error:
    spec->nb_entry_points--;
    free( odp->name );
    return 0;
}


static unsigned int apiset_hash_len( const char *str )
{
    return strrchr( str, '-' ) - str;
}

static unsigned int apiset_hash( const char *str )
{
    unsigned int ret = 0, len = apiset_hash_len( str );
    while (len--) ret = ret * apiset_hash_factor + *str++;
    return ret;
}

static unsigned int apiset_add_str( struct apiset *apiset, const char *str, unsigned int len )
{
    char *ret;

    if (!apiset->strings || !(ret = strstr( apiset->strings, str )))
    {
        if (apiset->str_pos + len >= apiset->str_size)
        {
            apiset->str_size = max( apiset->str_size * 2, 1024 );
            apiset->strings = xrealloc( apiset->strings, apiset->str_size );
        }
        ret = apiset->strings + apiset->str_pos;
        memcpy( ret, str, len );
        ret[len] = 0;
        apiset->str_pos += len;
    }
    return ret - apiset->strings;
}

static void add_apiset( struct apiset *apiset, const char *api )
{
    struct apiset_entry *entry;

    if (apiset->count == apiset->size)
    {
        apiset->size = max( apiset->size * 2, 64 );
        apiset->entries = xrealloc( apiset->entries, apiset->size * sizeof(*apiset->entries) );
    }
    entry = &apiset->entries[apiset->count++];
    entry->name_len = strlen( api );
    entry->name_off = apiset_add_str( apiset, api, entry->name_len );
    entry->hash = apiset_hash( api );
    entry->hash_len = apiset_hash_len( api );
    entry->val_count = 0;
}

static void add_apiset_value( struct apiset *apiset, const char *value )
{
    struct apiset_entry *entry = &apiset->entries[apiset->count - 1];

    if (entry->val_count < ARRAY_SIZE(entry->values) - 1)
    {
        struct apiset_value *val = &entry->values[entry->val_count++];
        char *sep = strchr( value, ':' );

        if (sep)
        {
            val->name_len = sep - value;
            val->name_off = apiset_add_str( apiset, value, val->name_len );
            val->val_len = strlen( sep + 1 );
            val->val_off = apiset_add_str( apiset, sep + 1, val->val_len );
        }
        else
        {
            val->name_len = val->name_off = 0;
            val->val_len = strlen( value );
            val->val_off = apiset_add_str( apiset, value, val->val_len );
        }
    }
    else error( "Too many values for api '%.*s'\n", entry->name_len, apiset->strings + entry->name_off );
}

/*******************************************************************
 *         parse_spec_apiset
 */
static int parse_spec_apiset( DLLSPEC *spec )
{
    struct apiset_entry *entry;
    const char *token;
    unsigned int i, hash;

    if (!data_only)
    {
        error( "Apiset definitions are only allowed in data-only mode\n" );
        return 0;
    }

    if (!(token = GetToken(0))) return 0;

    if (!strncmp( token, "api-", 4 ) && !strncmp( token, "ext-", 4 ))
    {
        error( "Unrecognized API set name '%s'\n", token );
        return 0;
    }

    hash = apiset_hash( token );
    for (i = 0, entry = spec->apiset.entries; i < spec->apiset.count; i++, entry++)
    {
        if (entry->name_len == strlen( token ) &&
            !strncmp( spec->apiset.strings + entry->name_off, token, entry->name_len ))
        {
            error( "Duplicate API set '%s'\n", token );
            return 0;
        }
        if (entry->hash == hash)
        {
            error( "Duplicate hash code '%.*s' and '%s'\n",
                   entry->name_len, spec->apiset.strings + entry->name_off, token );
            return 0;
        }
    }
    add_apiset( &spec->apiset, token );

    if (!(token = GetToken(0)) || strcmp( token, "=" ))
    {
        error( "Syntax error near '%s'\n", token );
        return 0;
    }

    while ((token = GetToken(1))) add_apiset_value( &spec->apiset, token );
    return 1;
}


static int name_compare( const void *ptr1, const void *ptr2 )
{
    const ORDDEF *odp1 = *(const ORDDEF * const *)ptr1;
    const ORDDEF *odp2 = *(const ORDDEF * const *)ptr2;
    const char *name1 = odp1->name ? odp1->name : odp1->export_name;
    const char *name2 = odp2->name ? odp2->name : odp2->export_name;
    return strcmp( name1, name2 );
}

/*******************************************************************
 *         assign_names
 *
 * Build the name array and catch duplicates.
 */
static void assign_names( struct exports *exports )
{
    int i, j, nb_exp_names = 0;
    ORDDEF **all_names;

    exports->nb_names = 0;
    for (i = 0; i < exports->nb_entry_points; i++)
        if (exports->entry_points[i]->name) exports->nb_names++;
        else if (exports->entry_points[i]->export_name) nb_exp_names++;

    if (!exports->nb_names && !nb_exp_names) return;

    /* check for duplicates */

    all_names = xmalloc( (exports->nb_names + nb_exp_names) * sizeof(all_names[0]) );
    for (i = j = 0; i < exports->nb_entry_points; i++)
        if (exports->entry_points[i]->name || exports->entry_points[i]->export_name)
            all_names[j++] = exports->entry_points[i];

    qsort( all_names, j, sizeof(all_names[0]), name_compare );

    for (i = 0; i < j - 1; i++)
    {
        const char *name1 = all_names[i]->name ? all_names[i]->name : all_names[i]->export_name;
        const char *name2 = all_names[i+1]->name ? all_names[i+1]->name : all_names[i+1]->export_name;
        if (!strcmp( name1, name2 ) &&
            !((all_names[i]->flags ^ all_names[i+1]->flags) & FLAG_EXPORT32))
        {
            current_line = max( all_names[i]->lineno, all_names[i+1]->lineno );
            error( "'%s' redefined\n%s:%d: First defined here\n",
                   name1, input_file_name,
                   min( all_names[i]->lineno, all_names[i+1]->lineno ) );
        }
    }
    free( all_names );

    if (exports->nb_names)
    {
        exports->names = xmalloc( exports->nb_names * sizeof(exports->names[0]) );
        for (i = j = 0; i < exports->nb_entry_points; i++)
            if (exports->entry_points[i]->name) exports->names[j++] = exports->entry_points[i];

        /* sort the list of names */
        qsort( exports->names, exports->nb_names, sizeof(exports->names[0]), name_compare );
        for (i = 0; i < exports->nb_names; i++) exports->names[i]->hint = i;
    }
}

/*******************************************************************
 *         assign_ordinals
 *
 * Build the ordinal array.
 */
static void assign_ordinals( struct exports *exports )
{
    int i, count, ordinal;

    /* start assigning from base, or from 1 if no ordinal defined yet */

    exports->base = MAX_ORDINALS;
    exports->limit = 0;
    for (i = 0; i < exports->nb_entry_points; i++)
    {
        ordinal = exports->entry_points[i]->ordinal;
        if (ordinal == -1) continue;
        if (ordinal > exports->limit) exports->limit = ordinal;
        if (ordinal < exports->base) exports->base = ordinal;
    }
    if (exports->base == MAX_ORDINALS) exports->base = 1;
    if (exports->limit < exports->base) exports->limit = exports->base;

    count = max( exports->limit + 1, exports->base + exports->nb_entry_points );
    exports->ordinals = xmalloc( count * sizeof(exports->ordinals[0]) );
    memset( exports->ordinals, 0, count * sizeof(exports->ordinals[0]) );

    /* fill in all explicitly specified ordinals */
    for (i = 0; i < exports->nb_entry_points; i++)
    {
        ordinal = exports->entry_points[i]->ordinal;
        if (ordinal == -1) continue;
        if (exports->ordinals[ordinal])
        {
            current_line = max( exports->entry_points[i]->lineno, exports->ordinals[ordinal]->lineno );
            error( "ordinal %d redefined\n%s:%d: First defined here\n",
                   ordinal, input_file_name,
                   min( exports->entry_points[i]->lineno, exports->ordinals[ordinal]->lineno ) );
        }
        else exports->ordinals[ordinal] = exports->entry_points[i];
    }

    /* now assign ordinals to the rest */
    for (i = 0, ordinal = exports->base; i < exports->nb_entry_points; i++)
    {
        if (exports->entry_points[i]->ordinal != -1) continue;
        while (exports->ordinals[ordinal]) ordinal++;
        if (ordinal >= MAX_ORDINALS)
        {
            current_line = exports->entry_points[i]->lineno;
            fatal_error( "Too many functions defined (max %d)\n", MAX_ORDINALS );
        }
        exports->entry_points[i]->ordinal = ordinal;
        exports->ordinals[ordinal] = exports->entry_points[i];
    }
    if (ordinal > exports->limit) exports->limit = ordinal;
}


static void assign_exports( DLLSPEC *spec, unsigned int cpu, struct exports *exports )
{
    unsigned int i;

    exports->entry_points = xmalloc( spec->nb_entry_points * sizeof(*exports->entry_points) );
    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *entry = &spec->entry_points[i];
        if ((entry->flags & FLAG_CPU_MASK) && !(entry->flags & FLAG_CPU(cpu)))
            continue;
        exports->entry_points[exports->nb_entry_points++] = entry;
    }

    assign_names( exports );
    assign_ordinals( exports );
}


/*******************************************************************
 *         add_16bit_exports
 *
 * Add the necessary exports to the 32-bit counterpart of a 16-bit module.
 */
void add_16bit_exports( DLLSPEC *spec32, DLLSPEC *spec16 )
{
    int i;
    ORDDEF *odp;

    spec32->file_name = xstrdup( spec16->file_name );
    spec32->characteristics = IMAGE_FILE_DLL;
    spec32->init_func = xstrdup( "DllMain" );

    /* add an export for the NE module */

    odp = add_entry_point( spec32 );
    odp->type = TYPE_EXTERN;
    odp->flags = FLAG_PRIVATE;
    odp->name = xstrdup( "__wine_spec_dos_header" );
    odp->lineno = 0;
    odp->ordinal = 1;
    odp->link_name = xstrdup( ".L__wine_spec_dos_header" );

    if (spec16->main_module)
    {
        odp = add_entry_point( spec32 );
        odp->type = TYPE_EXTERN;
        odp->flags = FLAG_PRIVATE;
        odp->name = xstrdup( "__wine_spec_main_module" );
        odp->lineno = 0;
        odp->ordinal = 2;
        odp->link_name = xstrdup( ".L__wine_spec_main_module" );
    }

    /* add the explicit win32 exports */

    for (i = 1; i <= spec16->exports.limit; i++)
    {
        ORDDEF *odp16 = spec16->exports.ordinals[i];

        if (!odp16 || !odp16->name) continue;
        if (!(odp16->flags & FLAG_EXPORT32)) continue;

        odp = add_entry_point( spec32 );
        odp->flags = odp16->flags & ~FLAG_EXPORT32;
        odp->type = odp16->type;
        odp->name = xstrdup( odp16->name );
        odp->lineno = odp16->lineno;
        odp->ordinal = -1;
        odp->link_name = xstrdup( odp16->link_name );
        odp->u.func.nb_args = odp16->u.func.nb_args;
        if (odp->u.func.nb_args > 0) memcpy( odp->u.func.args, odp16->u.func.args,
                                             odp->u.func.nb_args * sizeof(odp->u.func.args[0]) );
    }

    assign_exports( spec32, target.cpu, &spec32->exports );
}


/*******************************************************************
 *         parse_spec_file
 *
 * Parse a .spec file.
 */
int parse_spec_file( FILE *file, DLLSPEC *spec )
{
    const char *token;

    input_file = file;
    current_line = 0;

    comment_chars = "#;";
    separator_chars = "()";

    while (get_next_line())
    {
        if (!(token = GetToken(1))) continue;
        if (strcmp(token, "@") == 0)
        {
            if (!parse_spec_ordinal( -1, spec )) continue;
        }
        else if (IsNumberString(token))
        {
            if (!parse_spec_ordinal( atoi(token), spec )) continue;
        }
        else if (strcmp(token, "apiset") == 0)
        {
            if (!parse_spec_apiset( spec )) continue;
        }
        else
        {
            error( "Expected ordinal declaration, got '%s'\n", token );
            continue;
        }
        if ((token = GetToken(1))) error( "Syntax error near '%s'\n", token );
    }

    current_line = 0;  /* no longer parsing the input file */
    assign_exports( spec, target.cpu, &spec->exports );
    if (native_arch != -1) assign_exports( spec, native_arch, &spec->native_exports );
    return !nb_errors;
}


/*******************************************************************
 *         parse_def_library
 *
 * Parse a LIBRARY declaration in a .def file.
 */
static int parse_def_library( DLLSPEC *spec )
{
    const char *token = GetToken(1);

    if (!token) return 1;
    if (strcmp( token, "BASE" ))
    {
        free( spec->file_name );
        spec->file_name = xstrdup( token );
        if (!(token = GetToken(1))) return 1;
    }
    if (strcmp( token, "BASE" ))
    {
        error( "Expected library name or BASE= declaration, got '%s'\n", token );
        return 0;
    }
    if (!(token = GetToken(0))) return 0;
    if (strcmp( token, "=" ))
    {
        error( "Expected '=' after BASE, got '%s'\n", token );
        return 0;
    }
    if (!(token = GetToken(0))) return 0;
    /* FIXME: do something with base address */

    return 1;
}


/*******************************************************************
 *         parse_def_stack_heap_size
 *
 * Parse a STACKSIZE or HEAPSIZE declaration in a .def file.
 */
static int parse_def_stack_heap_size( int is_stack, DLLSPEC *spec )
{
    const char *token = GetToken(0);
    char *end;
    unsigned long size;

    if (!token) return 0;
    size = strtoul( token, &end, 0 );
    if (*end)
    {
        error( "Invalid number '%s'\n", token );
        return 0;
    }
    if (is_stack) spec->stack_size = size / 1024;
    else spec->heap_size = size / 1024;
    if (!(token = GetToken(1))) return 1;
    if (strcmp( token, "," ))
    {
        error( "Expected ',' after size, got '%s'\n", token );
        return 0;
    }
    if (!(token = GetToken(0))) return 0;
    /* FIXME: do something with reserve size */
    return 1;
}


/*******************************************************************
 *         parse_def_export
 *
 * Parse an export declaration in a .def file.
 */
static int parse_def_export( char *name, DLLSPEC *spec )
{
    int i, args;
    const char *token = GetToken(1);
    ORDDEF *odp = add_entry_point( spec );

    odp->lineno = current_line;
    odp->ordinal = -1;
    odp->name = name;
    args = remove_stdcall_decoration( odp->name );
    if (args == -1)
    {
        odp->type = TYPE_CDECL;
        args = 0;
    }
    else
    {
        odp->type = TYPE_STDCALL;
        args /= get_ptr_size();
        if (args >= MAX_ARGUMENTS)
        {
            error( "Too many arguments in stdcall function '%s'\n", odp->name );
            return 0;
        }
        for (i = 0; i < args; i++) odp->u.func.args[i] = ARG_LONG;
    }
    odp->u.func.nb_args = args;

    /* check for optional internal name */

    if (token && !strcmp( token, "=" ))
    {
        if (!(token = GetToken(0))) goto error;
        odp->link_name = xstrdup( token );
        remove_stdcall_decoration( odp->link_name );
        token = GetToken(1);
    }
    else
    {
      odp->link_name = xstrdup( name );
    }

    /* check for optional ordinal */

    if (token && token[0] == '@')
    {
        int ordinal;

        if (!IsNumberString( token+1 ))
        {
            error( "Expected number after '@', got '%s'\n", token+1 );
            goto error;
        }
        ordinal = atoi( token+1 );
        if (!ordinal)
        {
            error( "Ordinal 0 is not valid\n" );
            goto error;
        }
        if (ordinal >= MAX_ORDINALS)
        {
            error( "Ordinal number %d too large\n", ordinal );
            goto error;
        }
        odp->ordinal = ordinal;
        token = GetToken(1);
    }

    /* check for other optional keywords */

    while (token)
    {
        if (!strcmp( token, "NONAME" ))
        {
            if (odp->ordinal == -1)
            {
                error( "NONAME requires an ordinal\n" );
                goto error;
            }
            odp->export_name = odp->name;
            odp->name = NULL;
            odp->flags |= FLAG_NONAME;
        }
        else if (!strcmp( token, "PRIVATE" ))
        {
            odp->flags |= FLAG_PRIVATE;
        }
        else if (!strcmp( token, "DATA" ))
        {
            odp->type = TYPE_EXTERN;
        }
        else
        {
            error( "Garbage text '%s' found at end of export declaration\n", token );
            goto error;
        }
        token = GetToken(1);
    }
    return 1;

error:
    spec->nb_entry_points--;
    free( odp->name );
    return 0;
}


/*******************************************************************
 *         parse_def_file
 *
 * Parse a .def file.
 */
int parse_def_file( FILE *file, DLLSPEC *spec )
{
    const char *token;
    int in_exports = 0;

    input_file = file;
    current_line = 0;

    comment_chars = ";";
    separator_chars = ",=";

    while (get_next_line())
    {
        if (!(token = GetToken(1))) continue;

        if (!strcmp( token, "LIBRARY" ) || !strcmp( token, "NAME" ))
        {
            if (!parse_def_library( spec )) continue;
            goto end_of_line;
        }
        else if (!strcmp( token, "STACKSIZE" ))
        {
            if (!parse_def_stack_heap_size( 1, spec )) continue;
            goto end_of_line;
        }
        else if (!strcmp( token, "HEAPSIZE" ))
        {
            if (!parse_def_stack_heap_size( 0, spec )) continue;
            goto end_of_line;
        }
        else if (!strcmp( token, "EXPORTS" ))
        {
            in_exports = 1;
            if (!(token = GetToken(1))) continue;
        }
        else if (!strcmp( token, "IMPORTS" ))
        {
            in_exports = 0;
            if (!(token = GetToken(1))) continue;
        }
        else if (!strcmp( token, "SECTIONS" ))
        {
            in_exports = 0;
            if (!(token = GetToken(1))) continue;
        }

        if (!in_exports) continue;  /* ignore this line */
        if (!parse_def_export( xstrdup(token), spec )) continue;

    end_of_line:
        if ((token = GetToken(1))) error( "Syntax error near '%s'\n", token );
    }

    current_line = 0;  /* no longer parsing the input file */
    assign_exports( spec, target.cpu, &spec->exports );
    return !nb_errors;
}
