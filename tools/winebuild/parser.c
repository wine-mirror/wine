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

#include "build.h"

int current_line = 0;

static SPEC_TYPE SpecType = SPEC_INVALID;

static char ParseBuffer[512];
static char *ParseNext = ParseBuffer;
static char ParseSaveChar;
static FILE *input_file;

static const char * const TypeNames[TYPE_NBTYPES] =
{
    "byte",         /* TYPE_BYTE */
    "word",         /* TYPE_WORD */
    "long",         /* TYPE_LONG */
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

/* callback function used for stub functions */
#define STUB_CALLBACK \
  ((SpecType == SPEC_WIN16) ? "RELAY_Unimplemented16": "RELAY_Unimplemented32")

static int IsNumberString(char *s)
{
    while (*s) if (!isdigit(*s++)) return 0;
    return 1;
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
    if (*token != '(' && *token != ')')
	while (*p != '\0' && *p != '(' && *p != ')' && !isspace(*p))
	    p++;
    
    ParseSaveChar = *p;
    ParseNext = p;
    *p = '\0';

    return token;
}

static char * GetToken(void)
{
    char *token;

    while ((token = GetTokenInLine()) == NULL)
    {
	ParseNext = ParseBuffer;
	while (1)
	{
            current_line++;
	    if (fgets(ParseBuffer, sizeof(ParseBuffer), input_file) == NULL)
		return NULL;
	    if (ParseBuffer[0] != '#')
		break;
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
    
    char *token = GetToken();
    if (*token != '(') fatal_error( "Expected '(' got '%s'\n", token );

    n_values = 0;
    value_array_size = 25;
    value_array = xmalloc(sizeof(*value_array) * value_array_size);
    
    while ((token = GetToken()) != NULL)
    {
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
    
    if (token == NULL)
	fatal_error( "End of file in variable declaration\n" );

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
    int i;

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

    token = GetToken();
    if (*token != '(') fatal_error( "Expected '(' got '%s'\n", token );

    for (i = 0; i < sizeof(odp->u.func.arg_types)-1; i++)
    {
	token = GetToken();
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
            odp->u.func.arg_types[i] = 'l';
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
    strcpy(odp->u.func.link_name, GetToken());
}


/*******************************************************************
 *         ParseEquate
 *
 * Parse an 'equate' definition.
 */
static void ParseEquate( ORDDEF *odp )
{
    char *endptr;
    
    char *token = GetToken();
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
    strcpy( odp->u.func.link_name, STUB_CALLBACK );
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

    token = GetToken();
    if (*token != '(') fatal_error( "Expected '(' got '%s'\n", token );

    token = GetToken();
    if (*token != ')') fatal_error( "Expected ')' got '%s'\n", token );

    odp->u.func.arg_types[0] = '\0';
    strcpy( odp->u.func.link_name, GetToken() );
}


/*******************************************************************
 *         ParseExtern
 *
 * Parse an 'extern' definition.
 */
static void ParseExtern( ORDDEF *odp )
{
    if (SpecType == SPEC_WIN16) fatal_error( "'extern' not supported for Win16\n" );
    strcpy( odp->u.ext.link_name, GetToken() );
}


/*******************************************************************
 *         ParseForward
 *
 * Parse a 'forward' definition.
 */
static void ParseForward( ORDDEF *odp )
{
    if (SpecType == SPEC_WIN16) fatal_error( "'forward' not supported for Win16\n" );
    strcpy( odp->u.fwd.link_name, GetToken() );
}


/*******************************************************************
 *         ParseOrdinal
 *
 * Parse an ordinal definition.
 */
static void ParseOrdinal(int ordinal)
{
    char *token;

    ORDDEF *odp = &EntryPoints[nb_entry_points++];

    if (!(token = GetToken())) fatal_error( "Expected type after ordinal\n" );

    for (odp->type = 0; odp->type < TYPE_NBTYPES; odp->type++)
        if (TypeNames[odp->type] && !strcmp( token, TypeNames[odp->type] ))
            break;

    if (odp->type >= TYPE_NBTYPES)
        fatal_error( "Expected type after ordinal, found '%s' instead\n", token );

    if (!(token = GetToken())) fatal_error( "Expected name after type\n" );

    strcpy( odp->name, token );
    odp->lineno = current_line;
    odp->ordinal = ordinal;

    switch(odp->type)
    {
    case TYPE_BYTE:
    case TYPE_WORD:
    case TYPE_LONG:
        ParseVariable( odp );
        break;
    case TYPE_REGISTER:
	ParseExportFunction( odp );
#ifndef __i386__
        /* ignore Win32 'register' routines on non-Intel archs */
        if (SpecType == SPEC_WIN32)
        {
            nb_entry_points--;
            return;
        }
#endif
        break;
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
    while ((token = GetToken()) != NULL)
    {
	if (strcmp(token, "name") == 0)
	{
	    strcpy(DLLName, GetToken());
	}
	else if (strcmp(token, "file") == 0)
	{
	    strcpy(DLLFileName, GetToken());
	    strupper(DLLFileName);
	}
        else if (strcmp(token, "type") == 0)
        {
            token = GetToken();
            if (!strcmp(token, "win16" )) SpecType = SPEC_WIN16;
            else if (!strcmp(token, "win32" )) SpecType = SPEC_WIN32;
            else fatal_error( "Type must be 'win16' or 'win32'\n" );
        }
        else if (strcmp(token, "mode") == 0)
        {
            token = GetToken();
            if (!strcmp(token, "dll" )) SpecMode = SPEC_MODE_DLL;
            else if (!strcmp(token, "guiexe" )) SpecMode = SPEC_MODE_GUIEXE;
            else if (!strcmp(token, "cuiexe" )) SpecMode = SPEC_MODE_CUIEXE;
            else fatal_error( "Mode must be 'dll', 'guiexe' or 'cuiexe'\n" );
        }
	else if (strcmp(token, "heap") == 0)
	{
            token = GetToken();
            if (!IsNumberString(token)) fatal_error( "Expected number after heap\n" );
            DLLHeapSize = atoi(token);
	}
        else if (strcmp(token, "init") == 0)
        {
            strcpy(DLLInitFunc, GetToken());
	    if (SpecType == SPEC_WIN16)
                fatal_error( "init cannot be used for Win16 spec files\n" );
            if (!DLLInitFunc[0])
                fatal_error( "Expected function name after init\n" );
            if (!strcmp(DLLInitFunc, "main"))
                fatal_error( "The init function cannot be named 'main'\n" );
        }
        else if (strcmp(token, "import") == 0)
        {
            if (nb_imports >= MAX_IMPORTS)
                fatal_error( "Too many imports (limit %d)\n", MAX_IMPORTS );
            if (SpecType != SPEC_WIN32)
                fatal_error( "Imports not supported for Win16\n" );
            DLLImports[nb_imports++] = xstrdup(GetToken());
        }
        else if (strcmp(token, "rsrc") == 0)
        {
            strcpy( rsrc_name, GetToken() );
            strcat( rsrc_name, "_ResourceDescriptor" );
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
    current_line = 0;  /* no longer parsing the input file */
    return SpecType;
}
