/*
 * Spec file parser
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
 */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "winbase.h"
#include "build.h"

int current_line = 0;

static SPEC_TYPE SpecType = SPEC_INVALID;

static char ParseBuffer[512];
static char *ParseNext = ParseBuffer;
static char ParseSaveChar;
static FILE *input_file;

static const char * const TypeNames[TYPE_NBTYPES] =
{
    "variable",     /* TYPE_VARIABLE */
    "pascal16",     /* TYPE_PASCAL_16 */
    "pascal",       /* TYPE_PASCAL */
    "equate",       /* TYPE_ABS */
    "register",     /* TYPE_REGISTER */
    "interrupt",    /* TYPE_INTERRUPT */
    "stub",         /* TYPE_STUB */
    "stdcall",      /* TYPE_STDCALL */
    "cdecl",        /* TYPE_CDECL */
    "varargs",      /* TYPE_VARARGS */
    "extern",       /* TYPE_EXTERN */
    "forward"       /* TYPE_FORWARD */
};

static const char * const FlagNames[] =
{
    "noimport",    /* FLAG_NOIMPORT */
    "norelay",     /* FLAG_NORELAY */
    "ret64",       /* FLAG_RET64 */
    "i386",        /* FLAG_I386 */
    NULL
};

static int IsNumberString(char *s)
{
    while (*s) if (!isdigit(*s++)) return 0;
    return 1;
}

inline static int is_token_separator( char ch )
{
    return (ch == '(' || ch == ')' || ch == '-');
}

static char * GetTokenInLine(void)
{
    char *p;
    char *token;

    if (ParseNext != ParseBuffer)
    {
	if (ParseSaveChar == '\0')
	    return NULL;
	*ParseNext = ParseSaveChar;
    }
    
    /*
     * Remove initial white space.
     */
    for (p = ParseNext; isspace(*p); p++)
	;
    
    if ((*p == '\0') || (*p == '#'))
	return NULL;
    
    /*
     * Find end of token.
     */
    token = p++;
    if (!is_token_separator(*token))
        while (*p != '\0' && !is_token_separator(*p) && !isspace(*p))
	    p++;
    
    ParseSaveChar = *p;
    ParseNext = p;
    *p = '\0';

    return token;
}

static char * GetToken( int allow_eof )
{
    char *token;

    while ((token = GetTokenInLine()) == NULL)
    {
	ParseNext = ParseBuffer;
	while (1)
	{
            current_line++;
	    if (fgets(ParseBuffer, sizeof(ParseBuffer), input_file) == NULL)
            {
                if (!allow_eof) fatal_error( "Unexpected end of file\n" );
                return NULL;
            }
	    if (ParseBuffer[0] != '#')
		break;
	}
    }
    return token;
}


/*******************************************************************
 *         ParseDebug
 *
 * Parse a debug channel definition.
 */
static void ParseDebug(void)
{
    char *token = GetToken(0);
    if (*token != '(') fatal_error( "Expected '(' got '%s'\n", token );
    for (;;)
    {
        token = GetToken(0);
        if (*token == ')') break;
        debug_channels = xrealloc( debug_channels,
                                   (nb_debug_channels + 1) * sizeof(*debug_channels));
        debug_channels[nb_debug_channels++] = xstrdup(token);
    }
}


/*******************************************************************
 *         ParseIgnore
 *
 * Parse an 'ignore' definition.
 */
static void ParseIgnore(void)
{
    char *token = GetToken(0);
    if (*token != '(') fatal_error( "Expected '(' got '%s'\n", token );
    for (;;)
    {
        token = GetToken(0);
        if (*token == ')') break;
        add_ignore_symbol( token );
    }
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
    
    char *token = GetToken(0);
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
    char *token;
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
    if ((odp->type == TYPE_STDCALL) && !i)
        odp->type = TYPE_CDECL; /* stdcall is the same as cdecl for 0 args */
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
    
    char *token = GetToken(0);
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
 *         ParseInterrupt
 *
 * Parse an 'interrupt' definition.
 */
static void ParseInterrupt( ORDDEF *odp )
{
    char *token;

    if (SpecType == SPEC_WIN32)
        fatal_error( "'interrupt' not supported for Win32\n" );

    token = GetToken(0);
    if (*token != '(') fatal_error( "Expected '(' got '%s'\n", token );

    token = GetToken(0);
    if (*token != ')') fatal_error( "Expected ')' got '%s'\n", token );

    odp->u.func.arg_types[0] = '\0';
    odp->link_name = xstrdup( GetToken(0) );
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
    /* 'extern' definitions are not available for implicit import */
    odp->flags |= FLAG_NOIMPORT;
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
static char *ParseFlags( ORDDEF *odp )
{
    unsigned int i;
    char *token;

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
    char *token;

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
    case TYPE_REGISTER:
    case TYPE_PASCAL_16:
    case TYPE_PASCAL:
    case TYPE_STDCALL:
    case TYPE_VARARGS:
    case TYPE_CDECL:
        ParseExportFunction( odp );
        break;
    case TYPE_INTERRUPT:
        ParseInterrupt( odp );
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
        if (ordinal >= MAX_ORDINALS) fatal_error( "Ordinal number %d too large\n", ordinal );
        if (ordinal > Limit) Limit = ordinal;
        if (ordinal < Base) Base = ordinal;
        odp->ordinal = ordinal;
        Ordinals[ordinal] = odp;
    }

    if (!strcmp( odp->name, "@" ))
    {
        if (ordinal == -1)
            fatal_error( "Nameless function needs an explicit ordinal number\n" );
        if (SpecType != SPEC_WIN32)
            fatal_error( "Nameless functions not supported for Win16\n" );
        odp->name[0] = 0;
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
            fatal_error( "'%s' redefined (previous definition at line %d)\n",
                         Names[i]->name, min( Names[i]->lineno, Names[i+1]->lineno ) );
        }
    }
}


/*******************************************************************
 *         ParseTopLevel
 *
 * Parse a spec file.
 */
SPEC_TYPE ParseTopLevel( FILE *file )
{
    char *token;

    input_file = file;
    current_line = 1;
    while ((token = GetToken(1)) != NULL)
    {
	if (strcmp(token, "name") == 0)
	{
	    strcpy(DLLName, GetToken(0));
	}
	else if (strcmp(token, "file") == 0)
	{
	    strcpy(DLLFileName, GetToken(0));
	    strupper(DLLFileName);
	}
        else if (strcmp(token, "type") == 0)
        {
            token = GetToken(0);
            if (!strcmp(token, "win16" )) SpecType = SPEC_WIN16;
            else if (!strcmp(token, "win32" )) SpecType = SPEC_WIN32;
            else fatal_error( "Type must be 'win16' or 'win32'\n" );
        }
        else if (strcmp(token, "mode") == 0)
        {
            token = GetToken(0);
            if (!strcmp(token, "dll" )) SpecMode = SPEC_MODE_DLL;
            else if (!strcmp(token, "guiexe" )) SpecMode = SPEC_MODE_GUIEXE;
            else if (!strcmp(token, "cuiexe" )) SpecMode = SPEC_MODE_CUIEXE;
            else if (!strcmp(token, "guiexe_unicode" )) SpecMode = SPEC_MODE_GUIEXE_UNICODE;
            else if (!strcmp(token, "cuiexe_unicode" )) SpecMode = SPEC_MODE_CUIEXE_UNICODE;
            else fatal_error( "Mode must be 'dll', 'guiexe', 'cuiexe', 'guiexe_unicode' or 'cuiexe_unicode'\n" );
        }
	else if (strcmp(token, "heap") == 0)
	{
            token = GetToken(0);
            if (!IsNumberString(token)) fatal_error( "Expected number after heap\n" );
            DLLHeapSize = atoi(token);
	}
        else if (strcmp(token, "init") == 0)
        {
            if (SpecType == SPEC_WIN16)
                fatal_error( "init cannot be used for Win16 spec files\n" );
            init_func = xstrdup( GetToken(0) );
        }
        else if (strcmp(token, "import") == 0)
        {
            const char* name;
            int delay = 0;

            if (SpecType != SPEC_WIN32)
                fatal_error( "Imports not supported for Win16\n" );
            name = GetToken(0);
            if (*name == '-')
            {
                name = GetToken(0);
                if (!strcmp(name, "delay"))
                {
                    name = GetToken(0);
                    delay = 1;
                }
                else fatal_error( "Unknown option '%s' for import directive\n", name );
            }
            add_import_dll( name, delay );
        }
        else if (strcmp(token, "rsrc") == 0)
        {
            if (SpecType != SPEC_WIN16) load_res32_file( GetToken(0) );
            else load_res16_file( GetToken(0) );
        }
        else if (strcmp(token, "owner") == 0)
        {
            if (SpecType != SPEC_WIN16)
                fatal_error( "Owner only supported for Win16 spec files\n" );
            strcpy( owner_name, GetToken(0) );
        }
        else if (strcmp(token, "debug_channels") == 0)
        {
            if (SpecType != SPEC_WIN32)
                fatal_error( "debug channels only supported for Win32 spec files\n" );
            ParseDebug();
        }
        else if (strcmp(token, "ignore") == 0)
        {
            if (SpecType != SPEC_WIN32)
                fatal_error( "'ignore' only supported for Win32 spec files\n" );
            ParseIgnore();
        }
        else if (strcmp(token, "@") == 0)
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
            fatal_error( "Expected name, id, length or ordinal\n" );
    }

    if (!DLLFileName[0])
    {
        if (SpecMode == SPEC_MODE_DLL)
            sprintf( DLLFileName, "%s.dll", DLLName );
        else
            sprintf( DLLFileName, "%s.exe", DLLName );
    }

    if (SpecType == SPEC_INVALID) fatal_error( "Missing 'type' declaration\n" );
    if (SpecType == SPEC_WIN16 && !owner_name[0])
        fatal_error( "'owner' not specified for Win16 dll\n" );

    current_line = 0;  /* no longer parsing the input file */
    sort_names();
    return SpecType;
}
