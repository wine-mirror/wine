/*
 * Spec file parser
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "winbase.h"
#include "build.h"

int current_line = 0;

static char ParseBuffer[512];
static char TokenBuffer[512];
static char *ParseNext = ParseBuffer;
static FILE *input_file;

static const char * const TypeNames[TYPE_NBTYPES] =
{
    "variable",     /* TYPE_VARIABLE */
    "pascal16",     /* TYPE_PASCAL_16 */
    "pascal",       /* TYPE_PASCAL */
    "equate",       /* TYPE_ABS */
    "stub",         /* TYPE_STUB */
    "stdcall",      /* TYPE_STDCALL */
    "cdecl",        /* TYPE_CDECL */
    "varargs",      /* TYPE_VARARGS */
    "extern",       /* TYPE_EXTERN */
    "forward"       /* TYPE_FORWARD */
};

static const char * const FlagNames[] =
{
    "norelay",     /* FLAG_NORELAY */
    "noname",      /* FLAG_NONAME */
    "ret64",       /* FLAG_RET64 */
    "i386",        /* FLAG_I386 */
    "register",    /* FLAG_REGISTER */
    "interrupt",   /* FLAG_INTERRUPT */
    NULL
};

static int IsNumberString(const char *s)
{
    while (*s) if (!isdigit(*s++)) return 0;
    return 1;
}

inline static int is_token_separator( char ch )
{
    return (ch == '(' || ch == ')' || ch == '-');
}

static const char * GetTokenInLine(void)
{
    char *p = ParseNext;
    char *token = TokenBuffer;

    /*
     * Remove initial white space.
     */
    while (isspace(*p)) p++;

    if ((*p == '\0') || (*p == '#')) return NULL;

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

static const char * GetToken( int allow_eof )
{
    const char *token;

    while ((token = GetTokenInLine()) == NULL)
    {
	ParseNext = ParseBuffer;
        current_line++;
        if (fgets(ParseBuffer, sizeof(ParseBuffer), input_file) == NULL)
        {
            if (!allow_eof) fatal_error( "Unexpected end of file\n" );
            return NULL;
        }
    }
    return token;
}


/*******************************************************************
 *         ParseVariable
 *
 * Parse a variable definition.
 */
static void ParseVariable( ORDDEF *odp )
{
    char *endptr;
    int *value_array;
    int n_values;
    int value_array_size;

    const char *token = GetToken(0);
    if (*token != '(') fatal_error( "Expected '(' got '%s'\n", token );

    n_values = 0;
    value_array_size = 25;
    value_array = xmalloc(sizeof(*value_array) * value_array_size);

    for (;;)
    {
        token = GetToken(0);
	if (*token == ')')
	    break;

	value_array[n_values++] = strtol(token, &endptr, 0);
	if (n_values == value_array_size)
	{
	    value_array_size += 25;
	    value_array = xrealloc(value_array,
				   sizeof(*value_array) * value_array_size);
	}

	if (endptr == NULL || *endptr != '\0')
	    fatal_error( "Expected number value, got '%s'\n", token );
    }

    odp->u.var.n_values = n_values;
    odp->u.var.values = xrealloc(value_array, sizeof(*value_array) * n_values);
}


/*******************************************************************
 *         ParseExportFunction
 *
 * Parse a function definition.
 */
static void ParseExportFunction( ORDDEF *odp )
{
    const char *token;
    unsigned int i;

    switch(SpecType)
    {
    case SPEC_WIN16:
        if (odp->type == TYPE_STDCALL)
            fatal_error( "'stdcall' not supported for Win16\n" );
        if (odp->type == TYPE_VARARGS)
	    fatal_error( "'varargs' not supported for Win16\n" );
        break;
    case SPEC_WIN32:
        if ((odp->type == TYPE_PASCAL) || (odp->type == TYPE_PASCAL_16))
            fatal_error( "'pascal' not supported for Win32\n" );
        if (odp->flags & FLAG_INTERRUPT)
            fatal_error( "'interrupt' not supported for Win32\n" );
        break;
    default:
        break;
    }

    token = GetToken(0);
    if (*token != '(') fatal_error( "Expected '(' got '%s'\n", token );

    for (i = 0; i < sizeof(odp->u.func.arg_types); i++)
    {
	token = GetToken(0);
	if (*token == ')')
	    break;

        if (!strcmp(token, "word"))
            odp->u.func.arg_types[i] = 'w';
        else if (!strcmp(token, "s_word"))
            odp->u.func.arg_types[i] = 's';
        else if (!strcmp(token, "long") || !strcmp(token, "segptr"))
            odp->u.func.arg_types[i] = 'l';
        else if (!strcmp(token, "ptr"))
            odp->u.func.arg_types[i] = 'p';
	else if (!strcmp(token, "str"))
	    odp->u.func.arg_types[i] = 't';
	else if (!strcmp(token, "wstr"))
	    odp->u.func.arg_types[i] = 'W';
	else if (!strcmp(token, "segstr"))
	    odp->u.func.arg_types[i] = 'T';
        else if (!strcmp(token, "double"))
        {
            odp->u.func.arg_types[i++] = 'l';
            if (i < sizeof(odp->u.func.arg_types)) odp->u.func.arg_types[i] = 'l';
        }
        else fatal_error( "Unknown variable type '%s'\n", token );

        if (SpecType == SPEC_WIN32)
        {
            if (strcmp(token, "long") &&
                strcmp(token, "ptr") &&
                strcmp(token, "str") &&
                strcmp(token, "wstr") &&
                strcmp(token, "double"))
            {
                fatal_error( "Type '%s' not supported for Win32\n", token );
            }
        }
    }
    if ((*token != ')') || (i >= sizeof(odp->u.func.arg_types)))
        fatal_error( "Too many arguments\n" );

    odp->u.func.arg_types[i] = '\0';
    if (odp->type == TYPE_VARARGS)
        odp->flags |= FLAG_NORELAY;  /* no relay debug possible for varags entry point */
    odp->link_name = xstrdup( GetToken(0) );
}


/*******************************************************************
 *         ParseEquate
 *
 * Parse an 'equate' definition.
 */
static void ParseEquate( ORDDEF *odp )
{
    char *endptr;

    const char *token = GetToken(0);
    int value = strtol(token, &endptr, 0);
    if (endptr == NULL || *endptr != '\0')
	fatal_error( "Expected number value, got '%s'\n", token );
    if (SpecType == SPEC_WIN32)
        fatal_error( "'equate' not supported for Win32\n" );
    odp->u.abs.value = value;
}


/*******************************************************************
 *         ParseStub
 *
 * Parse a 'stub' definition.
 */
static void ParseStub( ORDDEF *odp )
{
    odp->u.func.arg_types[0] = '\0';
    odp->link_name = xstrdup("");
}


/*******************************************************************
 *         ParseExtern
 *
 * Parse an 'extern' definition.
 */
static void ParseExtern( ORDDEF *odp )
{
    if (SpecType == SPEC_WIN16) fatal_error( "'extern' not supported for Win16\n" );
    odp->link_name = xstrdup( GetToken(0) );
}


/*******************************************************************
 *         ParseForward
 *
 * Parse a 'forward' definition.
 */
static void ParseForward( ORDDEF *odp )
{
    if (SpecType == SPEC_WIN16) fatal_error( "'forward' not supported for Win16\n" );
    odp->link_name = xstrdup( GetToken(0) );
}


/*******************************************************************
 *         ParseFlags
 *
 * Parse the optional flags for an entry point
 */
static const char *ParseFlags( ORDDEF *odp )
{
    unsigned int i;
    const char *token;

    do
    {
        token = GetToken(0);
        for (i = 0; FlagNames[i]; i++)
            if (!strcmp( FlagNames[i], token )) break;
        if (!FlagNames[i]) fatal_error( "Unknown flag '%s'\n", token );
        odp->flags |= 1 << i;
        token = GetToken(0);
    } while (*token == '-');

    return token;
}

/*******************************************************************
 *         fix_export_name
 *
 * Fix an exported function name by removing a possible @xx suffix
 */
static void fix_export_name( char *name )
{
    char *p, *end = strrchr( name, '@' );
    if (!end || !end[1] || end == name) return;
    /* make sure all the rest is digits */
    for (p = end + 1; *p; p++) if (!isdigit(*p)) return;
    *end = 0;
}

/*******************************************************************
 *         ParseOrdinal
 *
 * Parse an ordinal definition.
 */
static void ParseOrdinal(int ordinal)
{
    const char *token;

    ORDDEF *odp = xmalloc( sizeof(*odp) );
    memset( odp, 0, sizeof(*odp) );
    EntryPoints[nb_entry_points++] = odp;

    token = GetToken(0);

    for (odp->type = 0; odp->type < TYPE_NBTYPES; odp->type++)
        if (TypeNames[odp->type] && !strcmp( token, TypeNames[odp->type] ))
            break;

    if (odp->type >= TYPE_NBTYPES)
        fatal_error( "Expected type after ordinal, found '%s' instead\n", token );

    token = GetToken(0);
    if (*token == '-') token = ParseFlags( odp );

    odp->name = xstrdup( token );
    fix_export_name( odp->name );
    odp->lineno = current_line;
    odp->ordinal = ordinal;

    switch(odp->type)
    {
    case TYPE_VARIABLE:
        ParseVariable( odp );
        break;
    case TYPE_PASCAL_16:
    case TYPE_PASCAL:
    case TYPE_STDCALL:
    case TYPE_VARARGS:
    case TYPE_CDECL:
        ParseExportFunction( odp );
        break;
    case TYPE_ABS:
        ParseEquate( odp );
        break;
    case TYPE_STUB:
        ParseStub( odp );
        break;
    case TYPE_EXTERN:
        ParseExtern( odp );
        break;
    case TYPE_FORWARD:
        ParseForward( odp );
        break;
    default:
        assert( 0 );
    }

#ifndef __i386__
    if (odp->flags & FLAG_I386)
    {
        /* ignore this entry point on non-Intel archs */
        EntryPoints[--nb_entry_points] = NULL;
        free( odp );
        return;
    }
#endif

    if (ordinal != -1)
    {
        if (!ordinal) fatal_error( "Ordinal 0 is not valid\n" );
        if (ordinal >= MAX_ORDINALS) fatal_error( "Ordinal number %d too large\n", ordinal );
        if (ordinal > Limit) Limit = ordinal;
        if (ordinal < Base) Base = ordinal;
        odp->ordinal = ordinal;
        Ordinals[ordinal] = odp;
    }

    if (!strcmp( odp->name, "@" ) || odp->flags & FLAG_NONAME)
    {
        if (ordinal == -1)
            fatal_error( "Nameless function needs an explicit ordinal number\n" );
        if (SpecType != SPEC_WIN32)
            fatal_error( "Nameless functions not supported for Win16\n" );
        if (!strcmp( odp->name, "@" )) free( odp->name );
        else odp->export_name = odp->name;
        odp->name = NULL;
    }
    else Names[nb_names++] = odp;
}


static int name_compare( const void *name1, const void *name2 )
{
    ORDDEF *odp1 = *(ORDDEF **)name1;
    ORDDEF *odp2 = *(ORDDEF **)name2;
    return strcmp( odp1->name, odp2->name );
}

/*******************************************************************
 *         sort_names
 *
 * Sort the name array and catch duplicates.
 */
static void sort_names(void)
{
    int i;

    if (!nb_names) return;

    /* sort the list of names */
    qsort( Names, nb_names, sizeof(Names[0]), name_compare );

    /* check for duplicate names */
    for (i = 0; i < nb_names - 1; i++)
    {
        if (!strcmp( Names[i]->name, Names[i+1]->name ))
        {
            current_line = max( Names[i]->lineno, Names[i+1]->lineno );
            fatal_error( "'%s' redefined\n%s:%d: First defined here\n",
                         Names[i]->name, input_file_name,
                         min( Names[i]->lineno, Names[i+1]->lineno ) );
        }
    }
}


/*******************************************************************
 *         ParseTopLevel
 *
 * Parse a spec file.
 */
void ParseTopLevel( FILE *file )
{
    const char *token;

    input_file = file;
    current_line = 0;

    while ((token = GetToken(1)) != NULL)
    {
        if (strcmp(token, "@") == 0)
	{
            if (SpecType != SPEC_WIN32)
                fatal_error( "'@' ordinals not supported for Win16\n" );
	    ParseOrdinal( -1 );
	}
	else if (IsNumberString(token))
	{
	    ParseOrdinal( atoi(token) );
	}
	else
            fatal_error( "Expected ordinal declaration\n" );
    }

    current_line = 0;  /* no longer parsing the input file */
    sort_names();
}


/*******************************************************************
 *         add_debug_channel
 */
static void add_debug_channel( const char *name )
{
    int i;

    for (i = 0; i < nb_debug_channels; i++)
        if (!strcmp( debug_channels[i], name )) return;

    debug_channels = xrealloc( debug_channels, (nb_debug_channels + 1) * sizeof(*debug_channels));
    debug_channels[nb_debug_channels++] = xstrdup(name);
}


/*******************************************************************
 *         parse_debug_channels
 *
 * Parse a source file and extract the debug channel definitions.
 */
void parse_debug_channels( const char *srcdir, const char *filename )
{
    FILE *file;
    int eol_seen = 1;

    file = open_input_file( srcdir, filename );
    while (fgets( ParseBuffer, sizeof(ParseBuffer), file ))
    {
        char *channel, *end, *p = ParseBuffer;

        p = ParseBuffer + strlen(ParseBuffer) - 1;
        if (!eol_seen)  /* continuation line */
        {
            eol_seen = (*p == '\n');
            continue;
        }
        if ((eol_seen = (*p == '\n'))) *p = 0;

        p = ParseBuffer;
        while (isspace(*p)) p++;
        if (!memcmp( p, "WINE_DECLARE_DEBUG_CHANNEL", 26 ) ||
            !memcmp( p, "WINE_DEFAULT_DEBUG_CHANNEL", 26 ))
        {
            p += 26;
            while (isspace(*p)) p++;
            if (*p != '(')
                fatal_error( "invalid debug channel specification '%s'\n", ParseBuffer );
            p++;
            while (isspace(*p)) p++;
            if (!isalpha(*p))
                fatal_error( "invalid debug channel specification '%s'\n", ParseBuffer );
            channel = p;
            while (isalnum(*p) || *p == '_') p++;
            end = p;
            while (isspace(*p)) p++;
            if (*p != ')')
                fatal_error( "invalid debug channel specification '%s'\n", ParseBuffer );
            *end = 0;
            add_debug_channel( channel );
        }
        current_line++;
    }
    close_input_file( file );
}
