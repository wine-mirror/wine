/*
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
 */

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "winbase.h"
#include "winnt.h"
#include "module.h"
#include "neexe.h"
#include "selectors.h"
#include "stackframe.h"
#include "builtin16.h"
#include "thread.h"

#ifdef NEED_UNDERSCORE_PREFIX
# define PREFIX "_"
#else
# define PREFIX
#endif

#ifdef HAVE_ASM_STRING
# define STRING ".string"
#else
# define STRING ".ascii"
#endif

#if defined(__GNUC__) && !defined(__svr4__)
# define USE_STABS
#else
# undef USE_STABS
#endif

typedef enum
{
    TYPE_BYTE,         /* byte variable (Win16) */
    TYPE_WORD,         /* word variable (Win16) */
    TYPE_LONG,         /* long variable (Win16) */
    TYPE_PASCAL_16,    /* pascal function with 16-bit return (Win16) */
    TYPE_PASCAL,       /* pascal function with 32-bit return (Win16) */
    TYPE_ABS,          /* absolute value (Win16) */
    TYPE_REGISTER,     /* register function */
    TYPE_INTERRUPT,    /* interrupt handler function (Win16) */
    TYPE_STUB,         /* unimplemented stub */
    TYPE_STDCALL,      /* stdcall function (Win32) */
    TYPE_CDECL,        /* cdecl function (Win32) */
    TYPE_VARARGS,      /* varargs function (Win32) */
    TYPE_EXTERN,       /* external symbol (Win32) */
    TYPE_FORWARD,      /* forwarded function (Win32) */
    TYPE_NBTYPES
} ORD_TYPE;

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

#define MAX_ORDINALS	2048
#define MAX_IMPORTS       16

  /* Callback function used for stub functions */
#define STUB_CALLBACK \
  ((SpecType == SPEC_WIN16) ? "RELAY_Unimplemented16": "RELAY_Unimplemented32")

typedef enum
{
    SPEC_INVALID,
    SPEC_WIN16,
    SPEC_WIN32
} SPEC_TYPE;

typedef struct
{
    int n_values;
    int *values;
} ORD_VARIABLE;

typedef struct
{
    int  n_args;
    char arg_types[32];
    char link_name[80];
} ORD_FUNCTION;

typedef struct
{
    int value;
} ORD_ABS;

typedef struct
{
    char link_name[80];
} ORD_EXTERN;

typedef struct
{
    char link_name[80];
} ORD_FORWARD;

typedef struct
{
    ORD_TYPE    type;
    int         ordinal;
    int         offset;
    int		lineno;
    char        name[80];
    union
    {
        ORD_VARIABLE   var;
        ORD_FUNCTION   func;
        ORD_ABS        abs;
        ORD_EXTERN     ext;
        ORD_FORWARD    fwd;
    } u;
} ORDDEF;

static ORDDEF EntryPoints[MAX_ORDINALS];
static ORDDEF *Ordinals[MAX_ORDINALS];
static ORDDEF *Names[MAX_ORDINALS];

static SPEC_TYPE SpecType = SPEC_INVALID;
static char DLLName[80];
static char DLLFileName[80];
static int Limit = 0;
static int Base = MAX_ORDINALS;
static int DLLHeapSize = 0;
static FILE *SpecFp;
static WORD Code_Selector, Data_Selector;
static char DLLInitFunc[80];
static char *DLLImports[MAX_IMPORTS];
static char rsrc_name[80];
static int nb_imports;
static int nb_entry_points;
static int nb_names;
static const char *input_file_name;
static const char *output_file_name;

char *ParseBuffer = NULL;
char *ParseNext;
char ParseSaveChar;
int Line;

static int UsePIC = 0;

static int debugging = 1;

  /* Offset of a structure field relative to the start of the struct */
#define STRUCTOFFSET(type,field) ((int)&((type *)0)->field)

  /* Offset of register relative to the start of the CONTEXT struct */
#define CONTEXTOFFSET(reg)  STRUCTOFFSET(CONTEXT86,reg)

  /* Offset of register relative to the start of the STACK16FRAME struct */
#define STACK16OFFSET(reg)  STRUCTOFFSET(STACK16FRAME,reg)

  /* Offset of register relative to the start of the STACK32FRAME struct */
#define STACK32OFFSET(reg)  STRUCTOFFSET(STACK32FRAME,reg)

  /* Offset of the stack pointer relative to %fs:(0) */
#define STACKOFFSET (STRUCTOFFSET(TEB,cur_stack))

static void BuildCallFrom16Func( FILE *outfile, char *profile, char *prefix, int local );

static void *xmalloc (size_t size)
{
    void *res;

    res = malloc (size ? size : 1);
    if (res == NULL)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
        if (output_file_name) unlink( output_file_name );
        exit (1);
    }
    return res;
}


static void *xrealloc (void *ptr, size_t size)
{
    void *res = realloc (ptr, size);
    if (res == NULL)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
        if (output_file_name) unlink( output_file_name );
        exit (1);
    }
    return res;
}

static char *xstrdup( const char *str )
{
    char *res = strdup( str );
    if (!res)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
        if (output_file_name) unlink( output_file_name );
        exit (1);
    }
    return res;
}

static void fatal_error( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    fprintf( stderr, "%s:%d: ", input_file_name, Line );
    vfprintf( stderr, msg, valist );
    va_end( valist );
    if (output_file_name) unlink( output_file_name );
    exit(1);
}

static int IsNumberString(char *s)
{
    while (*s != '\0')
	if (!isdigit(*s++))
	    return 0;

    return 1;
}

static char *strupper(char *s)
{
    char *p;
    
    for(p = s; *p != '\0'; p++)
	*p = toupper(*p);

    return s;
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

    if (ParseBuffer == NULL)
    {
	ParseBuffer = xmalloc(512);
	ParseNext = ParseBuffer;
	while (1)
	{
            Line++;
	    if (fgets(ParseBuffer, 511, SpecFp) == NULL)
		return NULL;
	    if (ParseBuffer[0] != '#')
		break;
	}
    }

    while ((token = GetTokenInLine()) == NULL)
    {
	ParseNext = ParseBuffer;
	while (1)
	{
            Line++;
	    if (fgets(ParseBuffer, 511, SpecFp) == NULL)
		return NULL;
	    if (ParseBuffer[0] != '#')
		break;
	}
    }

    return token;
}


static int name_compare( const void *name1, const void *name2 )
{
    ORDDEF *odp1 = *(ORDDEF **)name1;
    ORDDEF *odp2 = *(ORDDEF **)name2;
    return strcmp( odp1->name, odp2->name );
}

/*******************************************************************
 *         AssignOrdinals
 *
 * Assign ordinals to all entry points.
 */
static void AssignOrdinals(void)
{
    int i, ordinal;

    /* sort the list of names */
    qsort( Names, nb_names, sizeof(Names[0]), name_compare );

    /* check for duplicate names */
    for (i = 0; i < nb_names - 1; i++)
    {
        if (!strcmp( Names[i]->name, Names[i+1]->name ))
        {
            Line = MAX( Names[i]->lineno, Names[i+1]->lineno );
            fatal_error( "'%s' redefined (previous definition at line %d)\n",
                         Names[i]->name, MIN( Names[i]->lineno, Names[i+1]->lineno ) );
        }
    }

    /* start assigning from Base, or from 1 if no ordinal defined yet */
    if (Base == MAX_ORDINALS) Base = 1;
    for (i = 0, ordinal = Base; i < nb_names; i++)
    {
        if (Names[i]->ordinal != -1) continue;  /* already has an ordinal */
        while (Ordinals[ordinal]) ordinal++;
        if (ordinal >= MAX_ORDINALS)
        {
            Line = Names[i]->lineno;
            fatal_error( "Too many functions defined (max %d)\n", MAX_ORDINALS );
        }
        Names[i]->ordinal = ordinal;
        Ordinals[ordinal] = Names[i];
    }
    if (ordinal > Limit) Limit = ordinal;
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
    odp->lineno = Line;
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
static void ParseTopLevel(void)
{
    char *token;
    
    while ((token = GetToken()) != NULL)
    {
	if (strcmp(token, "name") == 0)
	{
	    strcpy(DLLName, GetToken());
	    strupper(DLLName);
            if (!DLLFileName[0]) sprintf( DLLFileName, "%s.DLL", DLLName );
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
}


/*******************************************************************
 *         StoreVariableCode
 *
 * Store a list of ints into a byte array.
 */
static int StoreVariableCode( unsigned char *buffer, int size, ORDDEF *odp )
{
    int i;

    switch(size)
    {
    case 1:
        for (i = 0; i < odp->u.var.n_values; i++)
            buffer[i] = odp->u.var.values[i];
        break;
    case 2:
        for (i = 0; i < odp->u.var.n_values; i++)
            ((unsigned short *)buffer)[i] = odp->u.var.values[i];
        break;
    case 4:
        for (i = 0; i < odp->u.var.n_values; i++)
            ((unsigned int *)buffer)[i] = odp->u.var.values[i];
        break;
    }
    return odp->u.var.n_values * size;
}


/*******************************************************************
 *         DumpBytes
 *
 * Dump a byte stream into the assembly code.
 */
static void DumpBytes( FILE *outfile, const unsigned char *data, int len,
                       const char *label )
{
    int i;

    fprintf( outfile, "\nstatic BYTE %s[] = \n{", label );

    for (i = 0; i < len; i++)
    {
        if (!(i & 0x0f)) fprintf( outfile, "\n    " );
        fprintf( outfile, "%d", *data++ );
        if (i < len - 1) fprintf( outfile, ", " );
    }
    fprintf( outfile, "\n};\n" );
}


/*******************************************************************
 *         BuildModule16
 *
 * Build the in-memory representation of a 16-bit NE module, and dump it
 * as a byte stream into the assembly code.
 */
static int BuildModule16( FILE *outfile, int max_code_offset,
                          int max_data_offset )
{
    int i;
    char *buffer;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegment;
    OFSTRUCT *pFileInfo;
    BYTE *pstr;
    WORD *pword;
    ET_BUNDLE *bundle = 0;
    ET_ENTRY *entry = 0;

    /*   Module layout:
     * NE_MODULE       Module
     * OFSTRUCT        File information
     * SEGTABLEENTRY   Segment 1 (code)
     * SEGTABLEENTRY   Segment 2 (data)
     * WORD[2]         Resource table (empty)
     * BYTE[2]         Imported names (empty)
     * BYTE[n]         Resident names table
     * BYTE[n]         Entry table
     */

    buffer = xmalloc( 0x10000 );

    pModule = (NE_MODULE *)buffer;
    memset( pModule, 0, sizeof(*pModule) );
    pModule->magic = IMAGE_OS2_SIGNATURE;
    pModule->count = 1;
    pModule->next = 0;
    pModule->flags = NE_FFLAGS_SINGLEDATA | NE_FFLAGS_BUILTIN | NE_FFLAGS_LIBMODULE;
    pModule->dgroup = 2;
    pModule->heap_size = DLLHeapSize;
    pModule->stack_size = 0;
    pModule->ip = 0;
    pModule->cs = 0;
    pModule->sp = 0;
    pModule->ss = 0;
    pModule->seg_count = 2;
    pModule->modref_count = 0;
    pModule->nrname_size = 0;
    pModule->modref_table = 0;
    pModule->nrname_fpos = 0;
    pModule->moveable_entries = 0;
    pModule->alignment = 0;
    pModule->truetype = 0;
    pModule->os_flags = NE_OSFLAGS_WINDOWS;
    pModule->misc_flags = 0;
    pModule->dlls_to_init  = 0;
    pModule->nrname_handle = 0;
    pModule->min_swap_area = 0;
    pModule->expected_version = 0;
    pModule->module32 = 0;
    pModule->self = 0;
    pModule->self_loading_sel = 0;

      /* File information */

    pFileInfo = (OFSTRUCT *)(pModule + 1);
    pModule->fileinfo = (int)pFileInfo - (int)pModule;
    memset( pFileInfo, 0, sizeof(*pFileInfo) - sizeof(pFileInfo->szPathName) );
    pFileInfo->cBytes = sizeof(*pFileInfo) - sizeof(pFileInfo->szPathName)
                        + strlen(DLLFileName);
    strcpy( pFileInfo->szPathName, DLLFileName );
    pstr = (char *)pFileInfo + pFileInfo->cBytes + 1;
        
#ifdef __i386__  /* FIXME: Alignment problems! */

      /* Segment table */

    pSegment = (SEGTABLEENTRY *)pstr;
    pModule->seg_table = (int)pSegment - (int)pModule;
    pSegment->filepos = 0;
    pSegment->size = max_code_offset;
    pSegment->flags = 0;
    pSegment->minsize = max_code_offset;
    pSegment->hSeg = 0;
    pSegment++;

    pModule->dgroup_entry = (int)pSegment - (int)pModule;
    pSegment->filepos = 0;
    pSegment->size = max_data_offset;
    pSegment->flags = NE_SEGFLAGS_DATA;
    pSegment->minsize = max_data_offset;
    pSegment->hSeg = 0;
    pSegment++;

      /* Resource table */

    pword = (WORD *)pSegment;
    pModule->res_table = (int)pword - (int)pModule;
    *pword++ = 0;
    *pword++ = 0;

      /* Imported names table */

    pstr = (char *)pword;
    pModule->import_table = (int)pstr - (int)pModule;
    *pstr++ = 0;
    *pstr++ = 0;

      /* Resident names table */

    pModule->name_table = (int)pstr - (int)pModule;
    /* First entry is module name */
    *pstr = strlen(DLLName );
    strcpy( pstr + 1, DLLName );
    pstr += *pstr + 1;
    *(WORD *)pstr = 0;
    pstr += sizeof(WORD);
    /* Store all ordinals */
    for (i = 1; i <= Limit; i++)
    {
        ORDDEF *odp = Ordinals[i];
        if (!odp || !odp->name[0]) continue;
        *pstr = strlen( odp->name );
        strcpy( pstr + 1, odp->name );
        strupper( pstr + 1 );
        pstr += *pstr + 1;
        *(WORD *)pstr = i;
        pstr += sizeof(WORD);
    }
    *pstr++ = 0;

      /* Entry table */

    pModule->entry_table = (int)pstr - (int)pModule;
    for (i = 1; i <= Limit; i++)
    {
        int selector = 0;
        ORDDEF *odp = Ordinals[i];
        if (!odp) continue;

	switch (odp->type)
	{
        case TYPE_CDECL:
        case TYPE_PASCAL:
        case TYPE_PASCAL_16:
        case TYPE_REGISTER:
        case TYPE_INTERRUPT:
        case TYPE_STUB:
            selector = 1;  /* Code selector */
            break;

        case TYPE_BYTE:
        case TYPE_WORD:
        case TYPE_LONG:
            selector = 2;  /* Data selector */
            break;

        case TYPE_ABS:
            selector = 0xfe;  /* Constant selector */
            break;

        default:
            selector = 0;  /* Invalid selector */
            break;
        }

        if ( !selector )
           continue;

        if ( bundle && bundle->last+1 == i )
            bundle->last++;
        else
        {
            if ( bundle )
                bundle->next = (char *)pstr - (char *)pModule;

            bundle = (ET_BUNDLE *)pstr;
            bundle->first = i-1;
            bundle->last = i;
            bundle->next = 0;
            pstr += sizeof(ET_BUNDLE);
        }

	/* FIXME: is this really correct ?? */
	entry = (ET_ENTRY *)pstr;
	entry->type = 0xff;  /* movable */
	entry->flags = 3; /* exported & public data */
	entry->segnum = selector;
	entry->offs = odp->offset;
	pstr += sizeof(ET_ENTRY);
    }
    *pstr++ = 0;
#endif

      /* Dump the module content */

    DumpBytes( outfile, (char *)pModule, (int)pstr - (int)pModule,
               "Module" );
    return (int)pstr - (int)pModule;
}


/*******************************************************************
 *         BuildSpec32File
 *
 * Build a Win32 C file from a spec file.
 */
static int BuildSpec32File( FILE *outfile )
{
    ORDDEF *odp;
    int i, fwd_size = 0, have_regs = FALSE;

    AssignOrdinals();

    fprintf( outfile, "/* File generated automatically from %s; do not edit! */\n\n",
             input_file_name );
    fprintf( outfile, "#include \"builtin32.h\"\n\n" );
    fprintf( outfile, "extern const BUILTIN32_DESCRIPTOR %s_Descriptor;\n",
             DLLName );

    /* Output the DLL functions prototypes */

    for (i = 0, odp = EntryPoints; i < nb_entry_points; i++, odp++)
    {
        switch(odp->type)
        {
        case TYPE_EXTERN:
            fprintf( outfile, "extern void %s();\n", odp->u.ext.link_name );
            break;
        case TYPE_STDCALL:
        case TYPE_VARARGS:
        case TYPE_CDECL:
            fprintf( outfile, "extern void %s();\n", odp->u.func.link_name );
            break;
        case TYPE_FORWARD:
            fwd_size += strlen(odp->u.fwd.link_name) + 1;
            break;
        case TYPE_REGISTER:
            fprintf( outfile, "extern void __regs_%d();\n", odp->ordinal );
            have_regs = TRUE;
            break;
        case TYPE_STUB:
            fprintf( outfile, "static void __stub_%d() { BUILTIN32_Unimplemented(&%s_Descriptor,%d); }\n",
                     odp->ordinal, DLLName, odp->ordinal );
            break;
        default:
            fprintf(stderr,"build: function type %d not available for Win32\n",
                    odp->type);
            return -1;
        }
    }

    /* Output LibMain function */
    if (DLLInitFunc[0]) fprintf( outfile, "extern void %s();\n", DLLInitFunc );


    /* Output code for all register functions */

    if ( have_regs )
    { 
        fprintf( outfile, "#ifndef __GNUC__\n" );
        fprintf( outfile, "static void __asm__dummy() {\n" );
        fprintf( outfile, "#endif /* !defined(__GNUC__) */\n" );
        for (i = 0, odp = EntryPoints; i < nb_entry_points; i++, odp++)
        {
            if (odp->type != TYPE_REGISTER) continue;
            fprintf( outfile,
                     "__asm__(\".align 4\\n\\t\"\n"
                     "        \".type " PREFIX "__regs_%d,@function\\n\\t\"\n"
                     "        \"" PREFIX "__regs_%d:\\n\\t\"\n"
                     "        \"call " PREFIX "CALL32_Regs\\n\\t\"\n"
                     "        \".long " PREFIX "%s\\n\\t\"\n"
                     "        \".byte %d,%d\");\n",
                     odp->ordinal, odp->ordinal, odp->u.func.link_name,
                     4 * strlen(odp->u.func.arg_types),
                     4 * strlen(odp->u.func.arg_types) );
        }
        fprintf( outfile, "#ifndef __GNUC__\n" );
        fprintf( outfile, "}\n" );
        fprintf( outfile, "#endif /* !defined(__GNUC__) */\n" );
    }

    /* Output the DLL functions table */

    fprintf( outfile, "\nstatic const ENTRYPOINT32 Functions[%d] =\n{\n",
             Limit - Base + 1 );
    for (i = Base; i <= Limit; i++)
    {
        ORDDEF *odp = Ordinals[i];
        if (!odp) fprintf( outfile, "    0" );
        else switch(odp->type)
        {
        case TYPE_EXTERN:
            fprintf( outfile, "    %s", odp->u.ext.link_name );
            break;
        case TYPE_STDCALL:
        case TYPE_VARARGS:
        case TYPE_CDECL:
            fprintf( outfile, "    %s", odp->u.func.link_name);
            break;
        case TYPE_STUB:
            fprintf( outfile, "    __stub_%d", i );
            break;
        case TYPE_REGISTER:
            fprintf( outfile, "    __regs_%d", i );
            break;
        case TYPE_FORWARD:
            fprintf( outfile, "    (ENTRYPOINT32)\"%s\"", odp->u.fwd.link_name );
            break;
        default:
            return -1;
        }
        if (i < Limit) fprintf( outfile, ",\n" );
    }
    fprintf( outfile, "\n};\n\n" );

    /* Output the DLL names table */

    fprintf( outfile, "static const char * const FuncNames[%d] =\n{\n", nb_names );
    for (i = 0; i < nb_names; i++)
    {
        if (i) fprintf( outfile, ",\n" );
        fprintf( outfile, "    \"%s\"", Names[i]->name );
    }
    fprintf( outfile, "\n};\n\n" );

    /* Output the DLL ordinals table */

    fprintf( outfile, "static const unsigned short FuncOrdinals[%d] =\n{\n", nb_names );
    for (i = 0; i < nb_names; i++)
    {
        if (i) fprintf( outfile, ",\n" );
        fprintf( outfile, "    %d", Names[i]->ordinal - Base );
    }
    fprintf( outfile, "\n};\n\n" );

    /* Output the DLL argument types */

    fprintf( outfile, "static const unsigned int ArgTypes[%d] =\n{\n",
             Limit - Base + 1 );
    for (i = Base; i <= Limit; i++)
    {
        ORDDEF *odp = Ordinals[i];
    	unsigned int j, mask = 0;
	if (odp &&
            ((odp->type == TYPE_STDCALL) || (odp->type == TYPE_CDECL) ||
            (odp->type == TYPE_REGISTER)))
	    for (j = 0; odp->u.func.arg_types[j]; j++)
            {
                if (odp->u.func.arg_types[j] == 't') mask |= 1<< (j*2);
                if (odp->u.func.arg_types[j] == 'W') mask |= 2<< (j*2);
	    }
	fprintf( outfile, "    %d", mask );
        if (i < Limit) fprintf( outfile, ",\n" );
    }
    fprintf( outfile, "\n};\n\n" );

    /* Output the DLL functions arguments */

    fprintf( outfile, "static const unsigned char FuncArgs[%d] =\n{\n",
             Limit - Base + 1 );
    for (i = Base; i <= Limit; i++)
    {
        unsigned char args = 0xff;
        ORDDEF *odp = Ordinals[i];
        if (odp) switch(odp->type)
        {
        case TYPE_STDCALL:
            args = (unsigned char)strlen(odp->u.func.arg_types);
            break;
        case TYPE_CDECL:
            args = 0x80 | (unsigned char)strlen(odp->u.func.arg_types);
            break;
        case TYPE_REGISTER:
            args = 0x40 | (unsigned char)strlen(odp->u.func.arg_types);
            break;
        case TYPE_FORWARD:
            args = 0xfd;
            break;
        default:
            args = 0xff;
            break;
        }
        fprintf( outfile, "    0x%02x", args );
        if (i < Limit) fprintf( outfile, ",\n" );
    }
    fprintf( outfile, "\n};\n\n" );

    /* Output the DLL imports */

    if (nb_imports)
    {
        fprintf( outfile, "static const char * const Imports[%d] =\n{\n", nb_imports );
        for (i = 0; i < nb_imports; i++)
        {
            fprintf( outfile, "    \"%s\"", DLLImports[i] );
            if (i < nb_imports-1) fprintf( outfile, ",\n" );
        }
        fprintf( outfile, "\n};\n\n" );
    }

    /* Output the DLL descriptor */

    if (rsrc_name[0]) fprintf( outfile, "extern const char %s[];\n\n", rsrc_name );

    fprintf( outfile, "const BUILTIN32_DESCRIPTOR %s_Descriptor =\n{\n",
             DLLName );
    fprintf( outfile, "    \"%s\",\n", DLLName );
    fprintf( outfile, "    \"%s\",\n", DLLFileName );
    fprintf( outfile, "    %d,\n", Base );
    fprintf( outfile, "    %d,\n", Limit - Base + 1 );
    fprintf( outfile, "    %d,\n", nb_names );
    fprintf( outfile, "    %d,\n", nb_imports );
    fprintf( outfile, "    %d,\n", (fwd_size + 3) & ~3 );
    fprintf( outfile,
             "    Functions,\n"
             "    FuncNames,\n"
             "    FuncOrdinals,\n"
             "    FuncArgs,\n"
             "    ArgTypes,\n");
    fprintf( outfile, "    %s,\n", nb_imports ? "Imports" : "0" );
    fprintf( outfile, "    %s,\n", DLLInitFunc[0] ? DLLInitFunc : "0" );
    fprintf( outfile, "    %s\n", rsrc_name[0] ? rsrc_name : "0" );
    fprintf( outfile, "};\n" );
    return 0;
}

/*******************************************************************
 *         Spec16TypeCompare
 */
static int Spec16TypeCompare( const void *e1, const void *e2 )
{
    const ORDDEF *odp1 = *(const ORDDEF **)e1;
    const ORDDEF *odp2 = *(const ORDDEF **)e2;

    int type1 = (odp1->type == TYPE_CDECL) ? 0
              : (odp1->type == TYPE_REGISTER) ? 3
              : (odp1->type == TYPE_INTERRUPT) ? 4
              : (odp1->type == TYPE_PASCAL_16) ? 1 : 2;

    int type2 = (odp2->type == TYPE_CDECL) ? 0
              : (odp2->type == TYPE_REGISTER) ? 3
              : (odp2->type == TYPE_INTERRUPT) ? 4
              : (odp2->type == TYPE_PASCAL_16) ? 1 : 2;

    int retval = type1 - type2;
    if ( !retval )
        retval = strcmp( odp1->u.func.arg_types, odp2->u.func.arg_types );

    return retval;
}

/*******************************************************************
 *         BuildSpec16File
 *
 * Build a Win16 assembly file from a spec file.
 */
static int BuildSpec16File( FILE *outfile )
{
    ORDDEF **type, **typelist;
    int i, nFuncs, nTypes;
    int code_offset, data_offset, module_size;
    unsigned char *data;

    /* File header */

    fprintf( outfile, "/* File generated automatically from %s; do not edit! */\n\n",
             input_file_name );
    fprintf( outfile, "#define __FLATCS__ 0x%04x\n", Code_Selector );
    fprintf( outfile, "#include \"builtin16.h\"\n\n" );

    data = (unsigned char *)xmalloc( 0x10000 );
    memset( data, 0, 16 );
    data_offset = 16;


    /* Build sorted list of all argument types, without duplicates */

    typelist = (ORDDEF **)calloc( Limit+1, sizeof(ORDDEF *) );

    for (i = nFuncs = 0; i <= Limit; i++)
    {
        ORDDEF *odp = Ordinals[i];
        if (!odp) continue;
        switch (odp->type)
        {
          case TYPE_REGISTER:
          case TYPE_INTERRUPT:
          case TYPE_CDECL:
          case TYPE_PASCAL:
          case TYPE_PASCAL_16:
          case TYPE_STUB:
            typelist[nFuncs++] = odp;

          default:
            break;
        }
    }

    qsort( typelist, nFuncs, sizeof(ORDDEF *), Spec16TypeCompare );

    i = nTypes = 0;
    while ( i < nFuncs )
    {
        typelist[nTypes++] = typelist[i++];
        while ( i < nFuncs && Spec16TypeCompare( typelist + i, typelist + nTypes-1 ) == 0 )
            i++;
    }

    /* Output CallFrom16 routines needed by this .spec file */

    for ( i = 0; i < nTypes; i++ )
    {
        char profile[101];

        sprintf( profile, "%s_%s_%s",
                 (typelist[i]->type == TYPE_CDECL) ? "c" : "p",
                 (typelist[i]->type == TYPE_REGISTER) ? "regs" :
                 (typelist[i]->type == TYPE_INTERRUPT) ? "intr" :
                 (typelist[i]->type == TYPE_PASCAL_16) ? "word" : "long",
                 typelist[i]->u.func.arg_types );

        BuildCallFrom16Func( outfile, profile, DLLName, TRUE );
    }

    /* Output the DLL functions prototypes */

    for (i = 0; i <= Limit; i++)
    {
        ORDDEF *odp = Ordinals[i];
        if (!odp) continue;
        switch(odp->type)
        {
        case TYPE_REGISTER:
        case TYPE_INTERRUPT:
        case TYPE_CDECL:
        case TYPE_PASCAL:
        case TYPE_PASCAL_16:
            fprintf( outfile, "extern void %s();\n", odp->u.func.link_name );
            break;
        default:
            break;
        }
    }

    /* Output code segment */

    fprintf( outfile, "\nstatic struct\n{\n    CALLFROM16   call[%d];\n"
                      "    ENTRYPOINT16 entry[%d];\n} Code_Segment = \n{\n    {\n",
                      nTypes, nFuncs );
    code_offset = 0;

    for ( i = 0; i < nTypes; i++ )
    {
        char profile[101], *arg;
        int argsize = 0;

        sprintf( profile, "%s_%s_%s", 
                          (typelist[i]->type == TYPE_CDECL) ? "c" : "p",
                          (typelist[i]->type == TYPE_REGISTER) ? "regs" :
                          (typelist[i]->type == TYPE_INTERRUPT) ? "intr" :
                          (typelist[i]->type == TYPE_PASCAL_16) ? "word" : "long",
                          typelist[i]->u.func.arg_types );

        if ( typelist[i]->type != TYPE_CDECL )
            for ( arg = typelist[i]->u.func.arg_types; *arg; arg++ )
                switch ( *arg )
                {
                case 'w':  /* word */
                case 's':  /* s_word */
                    argsize += 2;
                    break;
        
                case 'l':  /* long or segmented pointer */
                case 'T':  /* segmented pointer to null-terminated string */
                case 'p':  /* linear pointer */
                case 't':  /* linear pointer to null-terminated string */
                    argsize += 4;
                    break;
                }

        if ( typelist[i]->type == TYPE_INTERRUPT )
            argsize += 2;

        fprintf( outfile, "        CF16_%s( %s_CallFrom16_%s, %d, \"%s\" ),\n",
                 (   typelist[i]->type == TYPE_REGISTER 
                  || typelist[i]->type == TYPE_INTERRUPT)? "REGS":
                 typelist[i]->type == TYPE_PASCAL_16? "WORD" : "LONG",
                 DLLName, profile, argsize, profile );

        code_offset += sizeof(CALLFROM16);
    }
    fprintf( outfile, "    },\n    {\n" );

    for (i = 0; i <= Limit; i++)
    {
        ORDDEF *odp = Ordinals[i];
        if (!odp) continue;
        switch (odp->type)
        {
          case TYPE_ABS:
            odp->offset = LOWORD(odp->u.abs.value);
            break;

          case TYPE_BYTE:
            odp->offset = data_offset;
            data_offset += StoreVariableCode( data + data_offset, 1, odp);
            break;

          case TYPE_WORD:
            odp->offset = data_offset;
            data_offset += StoreVariableCode( data + data_offset, 2, odp);
            break;

          case TYPE_LONG:
            odp->offset = data_offset;
            data_offset += StoreVariableCode( data + data_offset, 4, odp);
            break;

          case TYPE_REGISTER:
          case TYPE_INTERRUPT:
          case TYPE_CDECL:
          case TYPE_PASCAL:
          case TYPE_PASCAL_16:
          case TYPE_STUB:
            type = bsearch( &odp, typelist, nTypes, sizeof(ORDDEF *), Spec16TypeCompare );
            assert( type );

            fprintf( outfile, "        /* %s.%d */ ", DLLName, i );
            fprintf( outfile, "EP( %s, %d /* %s_%s_%s */ ),\n",
                              odp->u.func.link_name,
                              (type-typelist)*sizeof(CALLFROM16) - 
                              (code_offset + sizeof(ENTRYPOINT16)),
                              (odp->type == TYPE_CDECL) ? "c" : "p",
                              (odp->type == TYPE_REGISTER) ? "regs" :
                              (odp->type == TYPE_INTERRUPT) ? "intr" :
                              (odp->type == TYPE_PASCAL_16) ? "word" : "long",
                              odp->u.func.arg_types );
                                 
            odp->offset = code_offset;
            code_offset += sizeof(ENTRYPOINT16);
            break;
		
          default:
            fprintf(stderr,"build: function type %d not available for Win16\n",
                    odp->type);
            return -1;
	}
    }

    fprintf( outfile, "    }\n};\n" );

    /* Output data segment */

    DumpBytes( outfile, data, data_offset, "Data_Segment" );

    /* Build the module */

    module_size = BuildModule16( outfile, code_offset, data_offset );

    /* Output the DLL descriptor */

    if (rsrc_name[0]) fprintf( outfile, "extern const char %s[];\n\n", rsrc_name );

    fprintf( outfile, "\nconst WIN16_DESCRIPTOR %s_Descriptor = \n{\n", DLLName );
    fprintf( outfile, "    \"%s\",\n", DLLName );
    fprintf( outfile, "    Module,\n" );
    fprintf( outfile, "    sizeof(Module),\n" );
    fprintf( outfile, "    (BYTE *)&Code_Segment,\n" );
    fprintf( outfile, "    (BYTE *)Data_Segment,\n" );
    fprintf( outfile, "    %s\n", rsrc_name[0] ? rsrc_name : "0" );
    fprintf( outfile, "};\n" );
    
    return 0;
}


/*******************************************************************
 *         BuildSpecFile
 *
 * Build an assembly file from a spec file.
 */
static void BuildSpecFile( FILE *outfile, FILE *infile )
{
    SpecFp = infile;
    ParseTopLevel();

    switch(SpecType)
    {
    case SPEC_WIN16:
        BuildSpec16File( outfile );
        break;
    case SPEC_WIN32:
        BuildSpec32File( outfile );
        break;
    default:
        fatal_error( "Missing 'type' declaration\n" );
    }
}


/*******************************************************************
 *         BuildCallFrom16Func
 *
 * Build a 16-bit-to-Wine callback glue function. 
 *
 * The generated routines are intended to be used as argument conversion 
 * routines to be called by the CallFrom16... core. Thus, the prototypes of
 * the generated routines are (see also CallFrom16):
 *
 *  extern WORD WINAPI PREFIX_CallFrom16_C_word_xxx( FARPROC func, LPBYTE args );
 *  extern LONG WINAPI PREFIX_CallFrom16_C_long_xxx( FARPROC func, LPBYTE args );
 *  extern void WINAPI PREFIX_CallFrom16_C_regs_xxx( FARPROC func, LPBYTE args, 
 *                                                   CONTEXT86 *context );
 *  extern void WINAPI PREFIX_CallFrom16_C_intr_xxx( FARPROC func, LPBYTE args, 
 *                                                   CONTEXT86 *context );
 *
 * where 'C' is the calling convention ('p' for pascal or 'c' for cdecl), 
 * and each 'x' is an argument  ('w'=word, 's'=signed word, 'l'=long, 
 * 'p'=linear pointer, 't'=linear pointer to null-terminated string,
 * 'T'=segmented pointer to null-terminated string).
 *
 * The generated routines fetch the arguments from the 16-bit stack (pointed
 * to by 'args'); the offsets of the single argument values are computed 
 * according to the calling convention and the argument types.  Then, the
 * 32-bit entry point is called with these arguments.
 * 
 * For register functions, the arguments (if present) are converted just
 * the same as for normal functions, but in addition the CONTEXT86 pointer 
 * filled with the current register values is passed to the 32-bit routine.
 * (An 'intr' interrupt handler routine is treated exactly like a register 
 * routine, except that upon return, the flags word pushed onto the stack 
 * by the interrupt is removed by the 16-bit call stub.)
 *
 */
static void BuildCallFrom16Func( FILE *outfile, char *profile, char *prefix, int local )
{
    int i, pos, argsize = 0;
    int short_ret = 0;
    int reg_func = 0;
    int usecdecl = 0;
    char *args = profile + 7;
    char *ret_type;

    /* Parse function type */

    if (!strncmp( "c_", profile, 2 )) usecdecl = 1;
    else if (strncmp( "p_", profile, 2 ))
    {
        fprintf( stderr, "Invalid function name '%s', ignored\n", profile );
        return;
    }

    if (!strncmp( "word_", profile + 2, 5 )) short_ret = 1;
    else if (!strncmp( "regs_", profile + 2, 5 )) reg_func = 1;
    else if (!strncmp( "intr_", profile + 2, 5 )) reg_func = 2;
    else if (strncmp( "long_", profile + 2, 5 ))
    {
        fprintf( stderr, "Invalid function name '%s', ignored\n", profile );
        return;
    }

    for ( i = 0; args[i]; i++ )
        switch ( args[i] )
        {
        case 'w':  /* word */
        case 's':  /* s_word */
            argsize += 2;
            break;
        
        case 'l':  /* long or segmented pointer */
        case 'T':  /* segmented pointer to null-terminated string */
        case 'p':  /* linear pointer */
        case 't':  /* linear pointer to null-terminated string */
            argsize += 4;
            break;
        }

    ret_type = reg_func? "void" : short_ret? "WORD" : "LONG";

    fprintf( outfile, "typedef %s WINAPI (*proc_%s_t)( ", 
                      ret_type, profile );
    args = profile + 7;
    for ( i = 0; args[i]; i++ )
    {
        if ( i ) fprintf( outfile, ", " );
        switch (args[i])
        {
        case 'w':           fprintf( outfile, "WORD" ); break;
        case 's':           fprintf( outfile, "INT16" ); break;
        case 'l': case 'T': fprintf( outfile, "LONG" ); break;
        case 'p': case 't': fprintf( outfile, "LPVOID" ); break;
        }
    }
    if ( reg_func )
        fprintf( outfile, "%sstruct _CONTEXT86 *", i? ", " : "" );
    else if ( !i )
        fprintf( outfile, "void" );
    fprintf( outfile, " );\n" );
    
    fprintf( outfile, "%s%s WINAPI %s_CallFrom16_%s( FARPROC proc, LPBYTE args%s )\n{\n",
             local? "static " : "", ret_type, prefix, profile,
             reg_func? ", struct _CONTEXT86 *context" : "" );

    fprintf( outfile, "    %s((proc_%s_t) proc) (\n",
             reg_func? "" : "return ", profile );
    args = profile + 7;
    pos = !usecdecl? argsize : 0;
    for ( i = 0; args[i]; i++ )
    {
        if ( i ) fprintf( outfile, ",\n" );
        fprintf( outfile, "        " );
        switch (args[i])
        {
        case 'w':  /* word */
            if ( !usecdecl ) pos -= 2;
            fprintf( outfile, "*(WORD *)(args+%d)", pos );
            if (  usecdecl ) pos += 2;
            break;

        case 's':  /* s_word */
            if ( !usecdecl ) pos -= 2;
            fprintf( outfile, "*(INT16 *)(args+%d)", pos );
            if (  usecdecl ) pos += 2;
            break;

        case 'l':  /* long or segmented pointer */
        case 'T':  /* segmented pointer to null-terminated string */
            if ( !usecdecl ) pos -= 4;
            fprintf( outfile, "*(LONG *)(args+%d)", pos );
            if (  usecdecl ) pos += 4;
            break;

        case 'p':  /* linear pointer */
        case 't':  /* linear pointer to null-terminated string */
            if ( !usecdecl ) pos -= 4;
            fprintf( outfile, "PTR_SEG_TO_LIN( *(SEGPTR *)(args+%d) )", pos );
            if (  usecdecl ) pos += 4;
            break;

        default:
            fprintf( stderr, "Unknown arg type '%c'\n", args[i] );
        }
    }
    if ( reg_func )
        fprintf( outfile, "%s        context", i? ",\n" : "" );
    fprintf( outfile, " );\n}\n\n" );
}

/*******************************************************************
 *         BuildCallTo16Func
 *
 * Build a Wine-to-16-bit callback glue function. 
 *
 * Prototypes for the CallTo16 functions:
 *   extern WORD CALLBACK PREFIX_CallTo16_word_xxx( FARPROC16 func, args... );
 *   extern LONG CALLBACK PREFIX_CallTo16_long_xxx( FARPROC16 func, args... );
 * 
 * These routines are provided solely for convenience; they simply
 * write the arguments onto the 16-bit stack, and call the appropriate
 * CallTo16... core routine.
 *
 * If you have more sophisticated argument conversion requirements than
 * are provided by these routines, you might as well call the core 
 * routines by yourself.
 *
 */
static void BuildCallTo16Func( FILE *outfile, char *profile, char *prefix )
{
    char *args = profile + 5;
    int i, argsize = 0, short_ret = 0;

    if (!strncmp( "word_", profile, 5 )) short_ret = 1;
    else if (strncmp( "long_", profile, 5 ))
    {
        fprintf( stderr, "Invalid function name '%s'.\n", profile );
        exit(1);
    }

    fprintf( outfile, "%s %s_CallTo16_%s( FARPROC16 proc",
             short_ret? "WORD" : "LONG", prefix, profile );
    args = profile + 5;
    for ( i = 0; args[i]; i++ )
    {
        fprintf( outfile, ", " );
        switch (args[i])
        {
        case 'w': fprintf( outfile, "WORD" ); argsize += 2; break;
        case 'l': fprintf( outfile, "LONG" ); argsize += 4; break;
        }
        fprintf( outfile, " arg%d", i+1 );
    }
    fprintf( outfile, " )\n{\n" );

    if ( argsize > 0 )
        fprintf( outfile, "    LPBYTE args = (LPBYTE)CURRENT_STACK16;\n" );

    args = profile + 5;
    for ( i = 0; args[i]; i++ )
    {
        switch (args[i])
        {
        case 'w': fprintf( outfile, "    args -= sizeof(WORD); *(WORD" ); break;
        case 'l': fprintf( outfile, "    args -= sizeof(LONG); *(LONG" ); break;
        default:  fprintf( stderr, "Unexpected case '%c' in BuildCallTo16Func\n",
                                   args[i] );
        }
        fprintf( outfile, " *)args = arg%d;\n", i+1 );
    }

    fprintf( outfile, "    return CallTo16%s( proc, %d );\n}\n\n",
             short_ret? "Word" : "Long", argsize );
}



/*******************************************************************
 *         BuildCallFrom16Core
 *
 * This routine builds the core routines used in 16->32 thunks:
 * CallFrom16Word, CallFrom16Long, CallFrom16Register, and CallFrom16Thunk.
 *
 * These routines are intended to be called via a far call (with 32-bit
 * operand size) from 16-bit code.  The 16-bit code stub must push %bp,
 * the 32-bit entry point to be called, and the argument conversion 
 * routine to be used (see stack layout below).  
 *
 * The core routine completes the STACK16FRAME on the 16-bit stack and
 * switches to the 32-bit stack.  Then, the argument conversion routine 
 * is called; it gets passed the 32-bit entry point and a pointer to the 
 * 16-bit arguments (on the 16-bit stack) as parameters. (You can either 
 * use conversion routines automatically generated by BuildCallFrom16, 
 * or write your own for special purposes.)
 * 
 * The conversion routine must call the 32-bit entry point, passing it
 * the converted arguments, and return its return value to the core.  
 * After the conversion routine has returned, the core switches back
 * to the 16-bit stack, converts the return value to the DX:AX format
 * (CallFrom16Long), and returns to the 16-bit call stub.  All parameters,
 * including %bp, are popped off the stack.
 *
 * The 16-bit call stub now returns to the caller, popping the 16-bit
 * arguments if necessary (pascal calling convention).
 *
 * In the case of a 'register' function, CallFrom16Register fills a
 * CONTEXT86 structure with the values all registers had at the point
 * the first instruction of the 16-bit call stub was about to be 
 * executed.  A pointer to this CONTEXT86 is passed as third parameter 
 * to the argument conversion routine, which typically passes it on
 * to the called 32-bit entry point.
 *
 * CallFrom16Thunk is a special variant used by the implementation of 
 * the Win95 16->32 thunk functions C16ThkSL and C16ThkSL01 and is 
 * implemented as follows:
 * On entry, the EBX register is set up to contain a flat pointer to the
 * 16-bit stack such that EBX+22 points to the first argument.
 * Then, the entry point is called, while EBP is set up to point
 * to the return address (on the 32-bit stack).
 * The called function returns with CX set to the number of bytes
 * to be popped of the caller's stack.
 *
 * Stack layout upon entry to the core routine (STACK16FRAME):
 *  ...           ...
 * (sp+24) word   first 16-bit arg
 * (sp+22) word   cs
 * (sp+20) word   ip
 * (sp+18) word   bp
 * (sp+14) long   32-bit entry point (reused for Win16 mutex recursion count)
 * (sp+12) word   ip of actual entry point (necessary for relay debugging)
 * (sp+8)  long   relay (argument conversion) function entry point
 * (sp+4)  long   cs of 16-bit entry point
 * (sp)    long   ip of 16-bit entry point
 *
 * Added on the stack:
 * (sp-2)  word   saved gs
 * (sp-4)  word   saved fs
 * (sp-6)  word   saved es
 * (sp-8)  word   saved ds
 * (sp-12) long   saved ebp
 * (sp-16) long   saved ecx
 * (sp-20) long   saved edx
 * (sp-24) long   saved previous stack
 */
static void BuildCallFrom16Core( FILE *outfile, int reg_func, int thunk, int short_ret )
{
    char *name = thunk? "Thunk" : reg_func? "Register" : short_ret? "Word" : "Long";

    /* Function header */
    fprintf( outfile, "\n\t.align 4\n" );
#ifdef USE_STABS
    fprintf( outfile, ".stabs \"CallFrom16%s:F1\",36,0,0," PREFIX "CallFrom16%s\n", 
	     name, name);
#endif
    fprintf( outfile, "\t.type " PREFIX "CallFrom16%s,@function\n", name );
    fprintf( outfile, "\t.globl " PREFIX "CallFrom16%s\n", name );
    fprintf( outfile, PREFIX "CallFrom16%s:\n", name );

    /* Create STACK16FRAME (except STACK32FRAME link) */
    fprintf( outfile, "\tpushw %%gs\n" );
    fprintf( outfile, "\tpushw %%fs\n" );
    fprintf( outfile, "\tpushw %%es\n" );
    fprintf( outfile, "\tpushw %%ds\n" );
    fprintf( outfile, "\tpushl %%ebp\n" );
    fprintf( outfile, "\tpushl %%ecx\n" );
    fprintf( outfile, "\tpushl %%edx\n" );

    /* Save original EFlags register */
    fprintf( outfile, "\tpushfl\n" );

    if ( UsePIC )
    {
        /* Get Global Offset Table into %ecx */
        fprintf( outfile, "\tcall .LCallFrom16%s.getgot1\n", name );
        fprintf( outfile, ".LCallFrom16%s.getgot1:\n", name );
        fprintf( outfile, "\tpopl %%ecx\n" );
        fprintf( outfile, "\taddl $_GLOBAL_OFFSET_TABLE_+[.-.LCallFrom16%s.getgot1], %%ecx\n", name );
    }

    /* Load 32-bit segment registers */
    fprintf( outfile, "\tmovw $0x%04x, %%dx\n", Data_Selector );
#ifdef __svr4__
    fprintf( outfile, "\tdata16\n");
#endif
    fprintf( outfile, "\tmovw %%dx, %%ds\n" );
#ifdef __svr4__
    fprintf( outfile, "\tdata16\n");
#endif
    fprintf( outfile, "\tmovw %%dx, %%es\n" );

    if ( UsePIC )
    {
        fprintf( outfile, "\tmovl " PREFIX "SYSLEVEL_Win16CurrentTeb@GOT(%%ecx), %%edx\n" );
        fprintf( outfile, "\tmovw (%%edx), %%fs\n" );
    }
    else
        fprintf( outfile, "\tmovw " PREFIX "SYSLEVEL_Win16CurrentTeb, %%fs\n" );

    /* Get address of ldt_copy array into %ecx */
    if ( UsePIC )
        fprintf( outfile, "\tmovl " PREFIX "ldt_copy@GOT(%%ecx), %%ecx\n" );
    else
        fprintf( outfile, "\tmovl $" PREFIX "ldt_copy, %%ecx\n" );

    /* Translate STACK16FRAME base to flat offset in %edx */
    fprintf( outfile, "\tmovw %%ss, %%dx\n" );
    fprintf( outfile, "\tandl $0xfff8, %%edx\n" );
    fprintf( outfile, "\tmovl (%%ecx,%%edx), %%edx\n" );
    fprintf( outfile, "\tmovzwl %%sp, %%ebp\n" );
    fprintf( outfile, "\tleal (%%ebp,%%edx), %%edx\n" );  

    /* Get saved flags into %ecx */
    fprintf( outfile, "\tpopl %%ecx\n" );

    /* Get the 32-bit stack pointer from the TEB and complete STACK16FRAME */
    fprintf( outfile, "\t.byte 0x64\n\tmovl (%d), %%ebp\n", STACKOFFSET );
    fprintf( outfile, "\tpushl %%ebp\n" );

    /* Switch stacks */
#ifdef __svr4__
    fprintf( outfile,"\tdata16\n");
#endif
    fprintf( outfile, "\t.byte 0x64\n\tmovw %%ss, (%d)\n", STACKOFFSET + 2 );
    fprintf( outfile, "\t.byte 0x64\n\tmovw %%sp, (%d)\n", STACKOFFSET );
    fprintf( outfile, "\tpushl %%ds\n" );
    fprintf( outfile, "\tpopl %%ss\n" );
    fprintf( outfile, "\tmovl %%ebp, %%esp\n" );
    fprintf( outfile, "\taddl $%d, %%ebp\n", STRUCTOFFSET(STACK32FRAME, ebp) );


    /* At this point:
       STACK16FRAME is completely set up
       DS, ES, SS: flat data segment
       FS: current TEB
       ESP: points to last STACK32FRAME
       EBP: points to ebp member of last STACK32FRAME
       EDX: points to current STACK16FRAME
       ECX: contains saved flags
       all other registers: unchanged */

    /* Special case: C16ThkSL stub */
    if ( thunk )
    {
        /* Set up registers as expected and call thunk */
        fprintf( outfile, "\tleal %d(%%edx), %%ebx\n", sizeof(STACK16FRAME)-22 );
        fprintf( outfile, "\tleal -4(%%esp), %%ebp\n" );

        fprintf( outfile, "\tcall *%d(%%edx)\n", STACK16OFFSET(entry_point) );

        /* Switch stack back */
        /* fprintf( outfile, "\t.byte 0x64\n\tlssw (%d), %%sp\n", STACKOFFSET ); */
        fprintf( outfile, "\t.byte 0x64,0x66,0x0f,0xb2,0x25\n\t.long %d\n", STACKOFFSET );
        fprintf( outfile, "\t.byte 0x64\n\tpopl (%d)\n", STACKOFFSET );

        /* Restore registers and return directly to caller */
        fprintf( outfile, "\taddl $8, %%esp\n" );
        fprintf( outfile, "\tpopl %%ebp\n" );
        fprintf( outfile, "\tpopw %%ds\n" );
        fprintf( outfile, "\tpopw %%es\n" );
        fprintf( outfile, "\tpopw %%fs\n" );
        fprintf( outfile, "\tpopw %%gs\n" );
        fprintf( outfile, "\taddl $20, %%esp\n" );

        fprintf( outfile, "\txorb %%ch, %%ch\n" );
        fprintf( outfile, "\tpopl %%ebx\n" );
        fprintf( outfile, "\taddw %%cx, %%sp\n" );
        fprintf( outfile, "\tpush %%ebx\n" );

        fprintf( outfile, "\t.byte 0x66\n" );
        fprintf( outfile, "\tlret\n" );

        return;
    }


    /* Build register CONTEXT */
    if ( reg_func )
    {
        fprintf( outfile, "\tsubl $%d, %%esp\n", sizeof(CONTEXT86) );

        fprintf( outfile, "\tmovl %%ecx, %d(%%esp)\n", CONTEXTOFFSET(EFlags) );  

        fprintf( outfile, "\tmovl %%eax, %d(%%esp)\n", CONTEXTOFFSET(Eax) );
        fprintf( outfile, "\tmovl %%ebx, %d(%%esp)\n", CONTEXTOFFSET(Ebx) );
        fprintf( outfile, "\tmovl %%esi, %d(%%esp)\n", CONTEXTOFFSET(Esi) );
        fprintf( outfile, "\tmovl %%edi, %d(%%esp)\n", CONTEXTOFFSET(Edi) );

        fprintf( outfile, "\tmovl %d(%%edx), %%eax\n", STACK16OFFSET(ebp) );
        fprintf( outfile, "\tmovl %%eax, %d(%%esp)\n", CONTEXTOFFSET(Ebp) );
        fprintf( outfile, "\tmovl %d(%%edx), %%eax\n", STACK16OFFSET(ecx) );
        fprintf( outfile, "\tmovl %%eax, %d(%%esp)\n", CONTEXTOFFSET(Ecx) );
        fprintf( outfile, "\tmovl %d(%%edx), %%eax\n", STACK16OFFSET(edx) );
        fprintf( outfile, "\tmovl %%eax, %d(%%esp)\n", CONTEXTOFFSET(Edx) );

        fprintf( outfile, "\tmovzwl %d(%%edx), %%eax\n", STACK16OFFSET(ds) );
        fprintf( outfile, "\tmovl %%eax, %d(%%esp)\n", CONTEXTOFFSET(SegDs) );
        fprintf( outfile, "\tmovzwl %d(%%edx), %%eax\n", STACK16OFFSET(es) );
        fprintf( outfile, "\tmovl %%eax, %d(%%esp)\n", CONTEXTOFFSET(SegEs) );
        fprintf( outfile, "\tmovzwl %d(%%edx), %%eax\n", STACK16OFFSET(fs) );
        fprintf( outfile, "\tmovl %%eax, %d(%%esp)\n", CONTEXTOFFSET(SegFs) );
        fprintf( outfile, "\tmovzwl %d(%%edx), %%eax\n", STACK16OFFSET(gs) );
        fprintf( outfile, "\tmovl %%eax, %d(%%esp)\n", CONTEXTOFFSET(SegGs) );

        fprintf( outfile, "\tmovzwl %d(%%edx), %%eax\n", STACK16OFFSET(cs) );
        fprintf( outfile, "\tmovl %%eax, %d(%%esp)\n", CONTEXTOFFSET(SegCs) );
        fprintf( outfile, "\tmovzwl %d(%%edx), %%eax\n", STACK16OFFSET(ip) );
        fprintf( outfile, "\tmovl %%eax, %d(%%esp)\n", CONTEXTOFFSET(Eip) );

        fprintf( outfile, "\t.byte 0x64\n\tmovzwl (%d), %%eax\n", STACKOFFSET+2 );
        fprintf( outfile, "\tmovl %%eax, %d(%%esp)\n", CONTEXTOFFSET(SegSs) );
        fprintf( outfile, "\t.byte 0x64\n\tmovzwl (%d), %%eax\n", STACKOFFSET );
        fprintf( outfile, "\taddl $%d, %%eax\n", STACK16OFFSET(ip) );
        fprintf( outfile, "\tmovl %%eax, %d(%%esp)\n", CONTEXTOFFSET(Esp) );
#if 0
        fprintf( outfile, "\tfsave %d(%%esp)\n", CONTEXTOFFSET(FloatSave) );
#endif

        /* Push address of CONTEXT86 structure -- popped by the relay routine */
        fprintf( outfile, "\tpushl %%esp\n" );
    }


    /* Print debug info before call */
    if ( debugging )
    {
        if ( UsePIC )
        {
            fprintf( outfile, "\tpushl %%ebx\n" );

            /* Get Global Offset Table into %ebx (for PLT call) */
            fprintf( outfile, "\tcall .LCallFrom16%s.getgot2\n", name );
            fprintf( outfile, ".LCallFrom16%s.getgot2:\n", name );
            fprintf( outfile, "\tpopl %%ebx\n" );
            fprintf( outfile, "\taddl $_GLOBAL_OFFSET_TABLE_+[.-.LCallFrom16%s.getgot2], %%ebx\n", name );
        }

        fprintf( outfile, "\tpushl %%edx\n" );
        if ( reg_func )
            fprintf( outfile, "\tleal -%d(%%ebp), %%eax\n\tpushl %%eax\n",
                              sizeof(CONTEXT) + STRUCTOFFSET(STACK32FRAME, ebp) );
        else
            fprintf( outfile, "\tpushl $0\n" );

        if ( UsePIC )
            fprintf( outfile, "\tcall " PREFIX "RELAY_DebugCallFrom16@PLT\n ");
        else
            fprintf( outfile, "\tcall " PREFIX "RELAY_DebugCallFrom16\n ");

        fprintf( outfile, "\tpopl %%edx\n" );
        fprintf( outfile, "\tpopl %%edx\n" );

        if ( UsePIC )
            fprintf( outfile, "\tpopl %%ebx\n" );
    }

    /* Call relay routine (which will call the API entry point) */
    fprintf( outfile, "\tleal %d(%%edx), %%eax\n", sizeof(STACK16FRAME) );
    fprintf( outfile, "\tpushl %%eax\n" );
    fprintf( outfile, "\tpushl %d(%%edx)\n", STACK16OFFSET(entry_point) );
    fprintf( outfile, "\tcall *%d(%%edx)\n", STACK16OFFSET(relay) );

    /* Print debug info after call */
    if ( debugging )
    {
        if ( UsePIC )
        {
            fprintf( outfile, "\tpushl %%ebx\n" );

            /* Get Global Offset Table into %ebx (for PLT call) */
            fprintf( outfile, "\tcall .LCallFrom16%s.getgot3\n", name );
            fprintf( outfile, ".LCallFrom16%s.getgot3:\n", name );
            fprintf( outfile, "\tpopl %%ebx\n" );
            fprintf( outfile, "\taddl $_GLOBAL_OFFSET_TABLE_+[.-.LCallFrom16%s.getgot3], %%ebx\n", name );
        }

        fprintf( outfile, "\tpushl %%eax\n" );
        if ( reg_func )
            fprintf( outfile, "\tleal -%d(%%ebp), %%eax\n\tpushl %%eax\n",
                              sizeof(CONTEXT) + STRUCTOFFSET(STACK32FRAME, ebp) );
        else
            fprintf( outfile, "\tpushl $0\n" );

        if ( UsePIC )
            fprintf( outfile, "\tcall " PREFIX "RELAY_DebugCallFrom16Ret@PLT\n ");
        else
            fprintf( outfile, "\tcall " PREFIX "RELAY_DebugCallFrom16Ret\n ");

        fprintf( outfile, "\tpopl %%eax\n" );
        fprintf( outfile, "\tpopl %%eax\n" );

        if ( UsePIC )
            fprintf( outfile, "\tpopl %%ebx\n" );
    }


    if ( reg_func )
    {
        fprintf( outfile, "\tmovl %%esp, %%ebx\n" );

        /* Switch stack back */
        /* fprintf( outfile, "\t.byte 0x64\n\tlssw (%d), %%sp\n", STACKOFFSET ); */
        fprintf( outfile, "\t.byte 0x64,0x66,0x0f,0xb2,0x25\n\t.long %d\n", STACKOFFSET );
        fprintf( outfile, "\t.byte 0x64\n\tpopl (%d)\n", STACKOFFSET );

        /* Get return address to CallFrom16 stub */
        fprintf( outfile, "\taddw $%d, %%sp\n", STACK16OFFSET(callfrom_ip)-4 );
        fprintf( outfile, "\tpopl %%eax\n" );
        fprintf( outfile, "\tpopl %%edx\n" );

        /* Restore all registers from CONTEXT */
        fprintf( outfile, "\tmovw %d(%%ebx), %%ss\n", CONTEXTOFFSET(SegSs) );
        fprintf( outfile, "\tmovl %d(%%ebx), %%esp\n", CONTEXTOFFSET(Esp) );
        fprintf( outfile, "\taddl $4, %%esp\n" );  /* room for final return address */

        fprintf( outfile, "\tpushw %d(%%ebx)\n", CONTEXTOFFSET(SegCs) );
        fprintf( outfile, "\tpushw %d(%%ebx)\n", CONTEXTOFFSET(Eip) );
        fprintf( outfile, "\tpushl %%edx\n" );
        fprintf( outfile, "\tpushl %%eax\n" );
        fprintf( outfile, "\tpushl %d(%%ebx)\n", CONTEXTOFFSET(EFlags) );
        fprintf( outfile, "\tpushl %d(%%ebx)\n", CONTEXTOFFSET(SegDs) );

        fprintf( outfile, "\tmovw %d(%%ebx), %%es\n", CONTEXTOFFSET(SegEs) );
        fprintf( outfile, "\tmovw %d(%%ebx), %%fs\n", CONTEXTOFFSET(SegFs) );
        fprintf( outfile, "\tmovw %d(%%ebx), %%gs\n", CONTEXTOFFSET(SegGs) );

        fprintf( outfile, "\tmovl %d(%%ebx), %%ebp\n", CONTEXTOFFSET(Ebp) );
        fprintf( outfile, "\tmovl %d(%%ebx), %%esi\n", CONTEXTOFFSET(Esi) );
        fprintf( outfile, "\tmovl %d(%%ebx), %%edi\n", CONTEXTOFFSET(Edi) );
        fprintf( outfile, "\tmovl %d(%%ebx), %%eax\n", CONTEXTOFFSET(Eax) );
        fprintf( outfile, "\tmovl %d(%%ebx), %%edx\n", CONTEXTOFFSET(Edx) );
        fprintf( outfile, "\tmovl %d(%%ebx), %%ecx\n", CONTEXTOFFSET(Ecx) );
        fprintf( outfile, "\tmovl %d(%%ebx), %%ebx\n", CONTEXTOFFSET(Ebx) );
  
        fprintf( outfile, "\tpopl %%ds\n" );
        fprintf( outfile, "\tpopfl\n" );
        fprintf( outfile, "\tlret\n" );
    }
    else
    {
        /* Switch stack back */
        /* fprintf( outfile, "\t.byte 0x64\n\tlssw (%d), %%sp\n", STACKOFFSET ); */
        fprintf( outfile, "\t.byte 0x64,0x66,0x0f,0xb2,0x25\n\t.long %d\n", STACKOFFSET );
        fprintf( outfile, "\t.byte 0x64\n\tpopl (%d)\n", STACKOFFSET );

        /* Restore registers */
        fprintf( outfile, "\tpopl %%edx\n" );
        fprintf( outfile, "\tpopl %%ecx\n" );
        fprintf( outfile, "\tpopl %%ebp\n" );
        fprintf( outfile, "\tpopw %%ds\n" );
        fprintf( outfile, "\tpopw %%es\n" );
        fprintf( outfile, "\tpopw %%fs\n" );
        fprintf( outfile, "\tpopw %%gs\n" );

        /* Prepare return value and set flags accordingly */
        if ( !short_ret )
            fprintf( outfile, "\tshldl $16, %%eax, %%edx\n" );
        fprintf( outfile, "\torl %%eax, %%eax\n" );

        /* Return to return stub which will return to caller */
        fprintf( outfile, "\tlret $12\n" );
    }
}
  

/*******************************************************************
 *         BuildCallTo16Core
 *
 * This routine builds the core routines used in 32->16 thunks:
 *
 *   extern void CALLBACK CallTo16Word( SEGPTR target, int nb_args );
 *   extern void CALLBACK CallTo16Long( SEGPTR target, int nb_args );
 *   extern void CALLBACK CallTo16RegisterShort( const CONTEXT86 *context, int nb_args );
 *   extern void CALLBACK CallTo16RegisterLong ( const CONTEXT86 *context, int nb_args );
 *
 * These routines can be called directly from 32-bit code. 
 *
 * All routines expect that the 16-bit stack contents (arguments) were 
 * already set up by the caller; nb_args must contain the number of bytes 
 * to be conserved.  The 16-bit SS:SP will be set accordinly.
 *
 * All other registers are either taken from the CONTEXT86 structure 
 * or else set to default values.  The target routine address is either
 * given directly or taken from the CONTEXT86.
 *
 * If you want to call a 16-bit routine taking only standard argument types 
 * (WORD and LONG), you can also have an appropriate argument conversion 
 * stub automatically generated (see BuildCallTo16); you'd then call this
 * stub, which in turn would prepare the 16-bit stack and call the appropiate
 * core routine.
 *
 */

static void BuildCallTo16Core( FILE *outfile, int short_ret, int reg_func )
{
    char *name = reg_func == 2 ? "RegisterLong" : 
                 reg_func == 1 ? "RegisterShort" :
                 short_ret? "Word" : "Long";

    /* Function header */
    fprintf( outfile, "\n\t.align 4\n" );
#ifdef USE_STABS
    fprintf( outfile, ".stabs \"CallTo16%s:F1\",36,0,0," PREFIX "CallTo16%s\n", 
	     name, name);
#endif
    fprintf( outfile, "\t.globl " PREFIX "CallTo16%s\n", name );
    fprintf( outfile, PREFIX "CallTo16%s:\n", name );

    /* Function entry sequence */
    fprintf( outfile, "\tpushl %%ebp\n" );
    fprintf( outfile, "\tmovl %%esp, %%ebp\n" );

    /* Save the 32-bit registers */
    fprintf( outfile, "\tpushl %%ebx\n" );
    fprintf( outfile, "\tpushl %%ecx\n" );
    fprintf( outfile, "\tpushl %%edx\n" );
    fprintf( outfile, "\tpushl %%esi\n" );
    fprintf( outfile, "\tpushl %%edi\n" );

    if ( UsePIC )
    {
        /* Get Global Offset Table into %ebx */
        fprintf( outfile, "\tcall .LCallTo16%s.getgot1\n", name );
        fprintf( outfile, ".LCallTo16%s.getgot1:\n", name );
        fprintf( outfile, "\tpopl %%ebx\n" );
        fprintf( outfile, "\taddl $_GLOBAL_OFFSET_TABLE_+[.-.LCallTo16%s.getgot1], %%ebx\n", name );
    }

    /* Enter Win16 Mutex */
    if ( UsePIC )
        fprintf( outfile, "\tcall " PREFIX "SYSLEVEL_EnterWin16Lock@PLT\n" );
    else
        fprintf( outfile, "\tcall " PREFIX "SYSLEVEL_EnterWin16Lock\n" );

    /* Print debugging info */
    if (debugging)
    {
        /* Push flags, number of arguments, and target */
        fprintf( outfile, "\tpushl $%d\n", reg_func );
        fprintf( outfile, "\tpushl 12(%%ebp)\n" );
        fprintf( outfile, "\tpushl  8(%%ebp)\n" );

        if ( UsePIC )
            fprintf( outfile, "\tcall " PREFIX "RELAY_DebugCallTo16@PLT\n" );
        else
            fprintf( outfile, "\tcall " PREFIX "RELAY_DebugCallTo16\n" );

        fprintf( outfile, "\taddl $12, %%esp\n" );
    }

    /* Get return address */
    if ( UsePIC )
        fprintf( outfile, "\tmovl " PREFIX "CallTo16_RetAddr@GOTOFF(%%ebx), %%ecx\n" );
    else
        fprintf( outfile, "\tmovl " PREFIX "CallTo16_RetAddr, %%ecx\n" );

    /* Call the actual CallTo16 routine (simulate a lcall) */
    fprintf( outfile, "\tpushl %%cs\n" );
    fprintf( outfile, "\tcall .LCallTo16%s\n", name );

    /* Convert and push return value */
    if ( short_ret )
    {
        fprintf( outfile, "\tmovzwl %%ax, %%eax\n" );
        fprintf( outfile, "\tpushl %%eax\n" );
    }
    else if ( reg_func != 2 )
    {
        fprintf( outfile, "\tshll $16,%%edx\n" );
        fprintf( outfile, "\tmovw %%ax,%%dx\n" );
        fprintf( outfile, "\tpushl %%edx\n" );
    }
    else
        fprintf( outfile, "\tpushl %%eax\n" );

    if ( UsePIC )
    {
        /* Get Global Offset Table into %ebx (might have been overwritten) */
        fprintf( outfile, "\tcall .LCallTo16%s.getgot2\n", name );
        fprintf( outfile, ".LCallTo16%s.getgot2:\n", name );
        fprintf( outfile, "\tpopl %%ebx\n" );
        fprintf( outfile, "\taddl $_GLOBAL_OFFSET_TABLE_+[.-.LCallTo16%s.getgot2], %%ebx\n", name );
    }

    /* Print debugging info */
    if (debugging)
    {
        if ( UsePIC )
            fprintf( outfile, "\tcall " PREFIX "RELAY_DebugCallTo16Ret@PLT\n" );
        else
            fprintf( outfile, "\tcall " PREFIX "RELAY_DebugCallTo16Ret\n" );
    }

    /* Leave Win16 Mutex */
    if ( UsePIC )
        fprintf( outfile, "\tcall " PREFIX "SYSLEVEL_LeaveWin16Lock@PLT\n" );
    else
        fprintf( outfile, "\tcall " PREFIX "SYSLEVEL_LeaveWin16Lock\n" );

    /* Get return value */
    fprintf( outfile, "\tpopl %%eax\n" );

    /* Restore the 32-bit registers */
    fprintf( outfile, "\tpopl %%edi\n" );
    fprintf( outfile, "\tpopl %%esi\n" );
    fprintf( outfile, "\tpopl %%edx\n" );
    fprintf( outfile, "\tpopl %%ecx\n" );
    fprintf( outfile, "\tpopl %%ebx\n" );

    /* Function exit sequence */
    fprintf( outfile, "\tpopl %%ebp\n" );
    fprintf( outfile, "\tret $8\n" );


    /* Start of the actual CallTo16 routine */

    fprintf( outfile, ".LCallTo16%s:\n", name );

    /* Complete STACK32FRAME */
    fprintf( outfile, "\t.byte 0x64\n\tpushl (%d)\n", STACKOFFSET );
    fprintf( outfile, "\tmovl %%esp,%%edx\n" );

    /* Switch to the 16-bit stack */
#ifdef __svr4__
    fprintf( outfile,"\tdata16\n");
#endif
    fprintf( outfile, "\t.byte 0x64\n\tmovw (%d),%%ss\n", STACKOFFSET + 2);
    fprintf( outfile, "\t.byte 0x64\n\tmovw (%d),%%sp\n", STACKOFFSET );
    fprintf( outfile, "\t.byte 0x64\n\tmovl %%edx,(%d)\n", STACKOFFSET );

    /* Make %bp point to the previous stackframe (built by CallFrom16) */
    fprintf( outfile, "\tmovzwl %%sp,%%ebp\n" );
    fprintf( outfile, "\tleal %d(%%ebp),%%ebp\n", STACK16OFFSET(bp) );

    /* Add the specified offset to the new sp */
    fprintf( outfile, "\tsubw %d(%%edx), %%sp\n", STACK32OFFSET(nb_args) );

    /* Push the return address 
     * With sreg suffix, we push 16:16 address (normal lret)
     * With lreg suffix, we push 16:32 address (0x66 lret, for KERNEL32_45)
     */
    if (reg_func != 2)
        fprintf( outfile, "\tpushl %%ecx\n" );
    else 
    {
        fprintf( outfile, "\tshldl $16, %%ecx, %%eax\n" );
        fprintf( outfile, "\tpushw $0\n" );
        fprintf( outfile, "\tpushw %%ax\n" );
        fprintf( outfile, "\tpushw $0\n" );
        fprintf( outfile, "\tpushw %%cx\n" );
    }

    if (reg_func)
    {
        /* Push the called routine address */
        fprintf( outfile, "\tmovl %d(%%edx),%%edx\n", STACK32OFFSET(target) );
        fprintf( outfile, "\tpushw %d(%%edx)\n", CONTEXTOFFSET(SegCs) );
        fprintf( outfile, "\tpushw %d(%%edx)\n", CONTEXTOFFSET(Eip) );

        /* Get the registers */
        fprintf( outfile, "\tpushw %d(%%edx)\n", CONTEXTOFFSET(SegDs) );
        fprintf( outfile, "\tmovl %d(%%edx),%%eax\n", CONTEXTOFFSET(SegEs) );
        fprintf( outfile, "\tmovw %%ax,%%es\n" );
        fprintf( outfile, "\tmovl %d(%%edx),%%eax\n", CONTEXTOFFSET(SegFs) );
        fprintf( outfile, "\tmovw %%ax,%%fs\n" );
        fprintf( outfile, "\tmovl %d(%%edx),%%ebp\n", CONTEXTOFFSET(Ebp) );
        fprintf( outfile, "\tmovl %d(%%edx),%%esi\n", CONTEXTOFFSET(Esi) );
        fprintf( outfile, "\tmovl %d(%%edx),%%edi\n", CONTEXTOFFSET(Edi) );
        fprintf( outfile, "\tmovl %d(%%edx),%%eax\n", CONTEXTOFFSET(Eax) );
        fprintf( outfile, "\tmovl %d(%%edx),%%ebx\n", CONTEXTOFFSET(Ebx) );
        fprintf( outfile, "\tmovl %d(%%edx),%%ecx\n", CONTEXTOFFSET(Ecx) );
        fprintf( outfile, "\tmovl %d(%%edx),%%edx\n", CONTEXTOFFSET(Edx) );

        /* Get the 16-bit ds */
        fprintf( outfile, "\tpopw %%ds\n" );
    }
    else  /* not a register function */
    {
        /* Push the called routine address */
        fprintf( outfile, "\tpushl %d(%%edx)\n", STACK32OFFSET(target) );

        /* Set %fs to the value saved by the last CallFrom16 */
        fprintf( outfile, "\tmovw %d(%%ebp),%%ax\n", STACK16OFFSET(fs)-STACK16OFFSET(bp) );
        fprintf( outfile, "\tmovw %%ax,%%fs\n" );

        /* Set %ds and %es (and %ax just in case) equal to %ss */
        fprintf( outfile, "\tmovw %%ss,%%ax\n" );
        fprintf( outfile, "\tmovw %%ax,%%ds\n" );
        fprintf( outfile, "\tmovw %%ax,%%es\n" );
    }

    /* Jump to the called routine */
    fprintf( outfile, "\t.byte 0x66\n" );
    fprintf( outfile, "\tlret\n" );
}

/*******************************************************************
 *         BuildRet16Func
 *
 * Build the return code for 16-bit callbacks
 */
static void BuildRet16Func( FILE *outfile )
{
    /* 
     *  Note: This must reside in the .data section to allow
     *        run-time relocation of the SYSLEVEL_Win16CurrentTeb symbol
     */

    fprintf( outfile, "\n\t.globl " PREFIX "CallTo16_Ret\n" );
    fprintf( outfile, PREFIX "CallTo16_Ret:\n" );

    /* Restore 32-bit segment registers */

    fprintf( outfile, "\tmovw $0x%04x,%%bx\n", Data_Selector );
#ifdef __svr4__
    fprintf( outfile, "\tdata16\n");
#endif
    fprintf( outfile, "\tmovw %%bx,%%ds\n" );
#ifdef __svr4__
    fprintf( outfile, "\tdata16\n");
#endif
    fprintf( outfile, "\tmovw %%bx,%%es\n" );

    fprintf( outfile, "\tmovw " PREFIX "SYSLEVEL_Win16CurrentTeb,%%fs\n" );

    /* Restore the 32-bit stack */

#ifdef __svr4__
    fprintf( outfile, "\tdata16\n");
#endif
    fprintf( outfile, "\tmovw %%bx,%%ss\n" );
    fprintf( outfile, "\t.byte 0x64\n\tmovl (%d),%%esp\n", STACKOFFSET );
    fprintf( outfile, "\t.byte 0x64\n\tpopl (%d)\n", STACKOFFSET );

    /* Return to caller */

    fprintf( outfile, "\tlret\n" );

    /* Declare the return address variable */

    fprintf( outfile, "\n\t.globl " PREFIX "CallTo16_RetAddr\n" );
    fprintf( outfile, PREFIX "CallTo16_RetAddr:\t.long 0\n" );
}

/*******************************************************************
 *         BuildCallTo32CBClient
 *
 * Call a CBClient relay stub from 32-bit code (KERNEL.620).
 *
 * Since the relay stub is itself 32-bit, this should not be a problem;
 * unfortunately, the relay stubs are expected to switch back to a 
 * 16-bit stack (and 16-bit code) after completion :-(
 *
 * This would conflict with our 16- vs. 32-bit stack handling, so
 * we simply switch *back* to our 32-bit stack before returning to
 * the caller ...
 *
 * The CBClient relay stub expects to be called with the following
 * 16-bit stack layout, and with ebp and ebx pointing into the 16-bit
 * stack at the designated places:
 *
 *    ...
 *  (ebp+14) original arguments to the callback routine
 *  (ebp+10) far return address to original caller
 *  (ebp+6)  Thunklet target address
 *  (ebp+2)  Thunklet relay ID code
 *  (ebp)    BP (saved by CBClientGlueSL)
 *  (ebp-2)  SI (saved by CBClientGlueSL)
 *  (ebp-4)  DI (saved by CBClientGlueSL)
 *  (ebp-6)  DS (saved by CBClientGlueSL)
 *
 *   ...     buffer space used by the 16-bit side glue for temp copies
 *
 *  (ebx+4)  far return address to 16-bit side glue code
 *  (ebx)    saved 16-bit ss:sp (pointing to ebx+4)
 *
 * The 32-bit side glue code accesses both the original arguments (via ebp)
 * and the temporary copies prepared by the 16-bit side glue (via ebx).
 * After completion, the stub will load ss:sp from the buffer at ebx
 * and perform a far return to 16-bit code.  
 *
 * To trick the relay stub into returning to us, we replace the 16-bit
 * return address to the glue code by a cs:ip pair pointing to our
 * return entry point (the original return address is saved first).
 * Our return stub thus called will then reload the 32-bit ss:esp and
 * return to 32-bit code (by using and ss:esp value that we have also
 * pushed onto the 16-bit stack before and a cs:eip values found at
 * that position on the 32-bit stack).  The ss:esp to be restored is
 * found relative to the 16-bit stack pointer at:
 *
 *  (ebx-4)   ss  (flat)
 *  (ebx-8)   sp  (32-bit stack pointer)
 *
 * The second variant of this routine, CALL32_CBClientEx, which is used
 * to implement KERNEL.621, has to cope with yet another problem: Here,
 * the 32-bit side directly returns to the caller of the CBClient thunklet,
 * restoring registers saved by CBClientGlueSL and cleaning up the stack.
 * As we have to return to our 32-bit code first, we have to adapt the
 * layout of our temporary area so as to include values for the registers
 * that are to be restored, and later (in the implementation of KERNEL.621)
 * we *really* restore them. The return stub restores DS, DI, SI, and BP
 * from the stack, skips the next 8 bytes (CBClient relay code / target),
 * and then performs a lret NN, where NN is the number of arguments to be
 * removed. Thus, we prepare our temporary area as follows:
 *
 *     (ebx+22) 16-bit cs  (this segment)
 *     (ebx+20) 16-bit ip  ('16-bit' return entry point)
 *     (ebx+16) 32-bit ss  (flat)
 *     (ebx+12) 32-bit sp  (32-bit stack pointer)
 *     (ebx+10) 16-bit bp  (points to ebx+24)
 *     (ebx+8)  16-bit si  (ignored)
 *     (ebx+6)  16-bit di  (ignored)
 *     (ebx+4)  16-bit ds  (we actually use the flat DS here)
 *     (ebx+2)  16-bit ss  (16-bit stack segment)
 *     (ebx+0)  16-bit sp  (points to ebx+4)
 *
 * Note that we ensure that DS is not changed and remains the flat segment,
 * and the 32-bit stack pointer our own return stub needs fits just 
 * perfectly into the 8 bytes that are skipped by the Windows stub.
 * One problem is that we have to determine the number of removed arguments,
 * as these have to be really removed in KERNEL.621. Thus, the BP value 
 * that we place in the temporary area to be restored, contains the value 
 * that SP would have if no arguments were removed. By comparing the actual
 * value of SP with this value in our return stub we can compute the number
 * of removed arguments. This is then returned to KERNEL.621.
 *
 * The stack layout of this function:
 * (ebp+20)  nArgs     pointer to variable receiving nr. of args (Ex only)
 * (ebp+16)  esi       pointer to caller's esi value
 * (ebp+12)  arg       ebp value to be set for relay stub
 * (ebp+8)   func      CBClient relay stub address
 * (ebp+4)   ret addr
 * (ebp)     ebp
 */
static void BuildCallTo32CBClient( FILE *outfile, BOOL isEx )
{
    char *name = isEx? "CBClientEx" : "CBClient";
    int size = isEx? 24 : 12;

    /* Function header */

    fprintf( outfile, "\n\t.align 4\n" );
#ifdef USE_STABS
    fprintf( outfile, ".stabs \"CALL32_%s:F1\",36,0,0," PREFIX "CALL32_%s\n",
                      name, name );
#endif
    fprintf( outfile, "\t.globl " PREFIX "CALL32_%s\n", name );
    fprintf( outfile, PREFIX "CALL32_%s:\n", name );

    /* Entry code */

    fprintf( outfile, "\tpushl %%ebp\n" );
    fprintf( outfile, "\tmovl %%esp,%%ebp\n" );
    fprintf( outfile, "\tpushl %%edi\n" );
    fprintf( outfile, "\tpushl %%esi\n" );
    fprintf( outfile, "\tpushl %%ebx\n" );

    /* Get the 16-bit stack */

    fprintf( outfile, "\t.byte 0x64\n\tmovl (%d),%%ebx\n", STACKOFFSET);
    
    /* Convert it to a flat address */

    fprintf( outfile, "\tshldl $16,%%ebx,%%eax\n" );
    fprintf( outfile, "\tandl $0xfff8,%%eax\n" );
    fprintf( outfile, "\tmovl " PREFIX "ldt_copy(%%eax),%%esi\n" );
    fprintf( outfile, "\tmovw %%bx,%%ax\n" );
    fprintf( outfile, "\taddl %%eax,%%esi\n" );

    /* Allocate temporary area (simulate STACK16_PUSH) */

    fprintf( outfile, "\tpushf\n" );
    fprintf( outfile, "\tcld\n" );
    fprintf( outfile, "\tleal -%d(%%esi), %%edi\n", size );
    fprintf( outfile, "\tmovl $%d, %%ecx\n", sizeof(STACK16FRAME) );
    fprintf( outfile, "\trep\n\tmovsb\n" );
    fprintf( outfile, "\tpopf\n" );

    fprintf( outfile, "\t.byte 0x64\n\tsubw $%d,(%d)\n", size, STACKOFFSET );

    fprintf( outfile, "\tpushl %%edi\n" );  /* remember address */

    /* Set up temporary area */

    if ( !isEx )
    {
        fprintf( outfile, "\tleal 4(%%edi), %%edi\n" );

        fprintf( outfile, "\tleal -8(%%esp), %%eax\n" );
        fprintf( outfile, "\tmovl %%eax, -8(%%edi)\n" );    /* 32-bit sp */

        fprintf( outfile, "\tmovl %%ss, %%ax\n" );
        fprintf( outfile, "\tandl $0x0000ffff, %%eax\n" );
        fprintf( outfile, "\tmovl %%eax, -4(%%edi)\n" );    /* 32-bit ss */

        fprintf( outfile, "\taddl $%d, %%ebx\n", sizeof(STACK16FRAME)-size+4 + 4 );
        fprintf( outfile, "\tmovl %%ebx, 0(%%edi)\n" );    /* 16-bit ss:sp */

        fprintf( outfile, "\tmovl " PREFIX "CALL32_%s_RetAddr, %%eax\n", name );
        fprintf( outfile, "\tmovl %%eax, 4(%%edi)\n" );   /* overwrite return address */
    }
    else
    {
        fprintf( outfile, "\taddl $%d, %%ebx\n", sizeof(STACK16FRAME)-size+4 );
        fprintf( outfile, "\tmovl %%ebx, 0(%%edi)\n" );

        fprintf( outfile, "\tmovl %%ds, %%ax\n" );
        fprintf( outfile, "\tmovw %%ax, 4(%%edi)\n" );

        fprintf( outfile, "\taddl $20, %%ebx\n" );
        fprintf( outfile, "\tmovw %%bx, 10(%%edi)\n" );

        fprintf( outfile, "\tleal -8(%%esp), %%eax\n" );
        fprintf( outfile, "\tmovl %%eax, 12(%%edi)\n" );

        fprintf( outfile, "\tmovl %%ss, %%ax\n" );
        fprintf( outfile, "\tandl $0x0000ffff, %%eax\n" );
        fprintf( outfile, "\tmovl %%eax, 16(%%edi)\n" );

        fprintf( outfile, "\tmovl " PREFIX "CALL32_%s_RetAddr, %%eax\n", name );
        fprintf( outfile, "\tmovl %%eax, 20(%%edi)\n" );
    }

    /* Set up registers and call CBClient relay stub (simulating a far call) */

    fprintf( outfile, "\tmovl 16(%%ebp), %%esi\n" );
    fprintf( outfile, "\tmovl (%%esi), %%esi\n" );

    fprintf( outfile, "\tmovl %%edi, %%ebx\n" );
    fprintf( outfile, "\tmovl 8(%%ebp), %%eax\n" );
    fprintf( outfile, "\tmovl 12(%%ebp), %%ebp\n" );

    fprintf( outfile, "\tpushl %%cs\n" );
    fprintf( outfile, "\tcall *%%eax\n" );

    /* Return new esi value to caller */

    fprintf( outfile, "\tmovl 32(%%esp), %%edi\n" );
    fprintf( outfile, "\tmovl %%esi, (%%edi)\n" );

    /* Cleanup temporary area (simulate STACK16_POP) */

    fprintf( outfile, "\tpop %%esi\n" );

    fprintf( outfile, "\tpushf\n" );
    fprintf( outfile, "\tstd\n" );
    fprintf( outfile, "\tdec %%esi\n" );
    fprintf( outfile, "\tleal %d(%%esi), %%edi\n", size );
    fprintf( outfile, "\tmovl $%d, %%ecx\n", sizeof(STACK16FRAME) );
    fprintf( outfile, "\trep\n\tmovsb\n" );
    fprintf( outfile, "\tpopf\n" );

    fprintf( outfile, "\t.byte 0x64\n\taddw $%d,(%d)\n", size, STACKOFFSET );

    /* Return argument size to caller */
    if ( isEx )
    {
        fprintf( outfile, "\tmovl 32(%%esp), %%ebx\n" );
        fprintf( outfile, "\tmovl %%ebp, (%%ebx)\n" );
    }

    /* Restore registers and return */

    fprintf( outfile, "\tpopl %%ebx\n" );
    fprintf( outfile, "\tpopl %%esi\n" );
    fprintf( outfile, "\tpopl %%edi\n" );
    fprintf( outfile, "\tpopl %%ebp\n" );
    fprintf( outfile, "\tret\n" );
}

static void BuildCallTo32CBClientRet( FILE *outfile, BOOL isEx )
{
    char *name = isEx? "CBClientEx" : "CBClient";

    /* '16-bit' return stub */

    fprintf( outfile, "\n\t.globl " PREFIX "CALL32_%s_Ret\n", name );
    fprintf( outfile, PREFIX "CALL32_%s_Ret:\n", name );

    if ( !isEx )
    {
        fprintf( outfile, "\tmovzwl %%sp, %%ebx\n" );
        fprintf( outfile, "\tlssl %%ss:-16(%%ebx), %%esp\n" );
    }
    else
    {
        fprintf( outfile, "\tmovzwl %%bp, %%ebx\n" );
        fprintf( outfile, "\tsubw %%bp, %%sp\n" );
        fprintf( outfile, "\tmovzwl %%sp, %%ebp\n" );
        fprintf( outfile, "\tlssl %%ss:-12(%%ebx), %%esp\n" );
    }
    fprintf( outfile, "\tlret\n" );

    /* Declare the return address variable */

    fprintf( outfile, "\n\t.globl " PREFIX "CALL32_%s_RetAddr\n", name );
    fprintf( outfile, PREFIX "CALL32_%s_RetAddr:\t.long 0\n", name );
}


/*******************************************************************
 *         BuildCallTo32LargeStack
 *
 * Build the function used to switch to the original 32-bit stack
 * before calling a 32-bit function from 32-bit code. This is used for
 * functions that need a large stack, like X bitmaps functions.
 *
 * The generated function has the following prototype:
 *   int xxx( int (*func)(), void *arg );
 *
 * The pointer to the function can be retrieved by calling CALL32_Init,
 * which also takes care of saving the current 32-bit stack pointer.
 * Furthermore, CALL32_Init switches to a new stack and jumps to the
 * specified target address.
 *
 * NOTE: The CALL32_LargeStack routine may be recursively entered by the 
 *       same thread, but not concurrently entered by several threads.
 *
 * Stack layout of CALL32_Init:
 *
 * (esp+12)  new stack address
 * (esp+8)   target address
 * (esp+4)   pointer to variable to receive CALL32_LargeStack address
 * (esp)     ret addr
 *
 * Stack layout of CALL32_LargeStack:
 *   ...     ...
 * (ebp+12)  arg
 * (ebp+8)   func
 * (ebp+4)   ret addr
 * (ebp)     ebp
 */
static void BuildCallTo32LargeStack( FILE *outfile )
{
    /* Initialization function */

    fprintf( outfile, "\n\t.align 4\n" );
#ifdef USE_STABS
    fprintf( outfile, ".stabs \"CALL32_Init:F1\",36,0,0," PREFIX "CALL32_Init\n");
#endif
    fprintf( outfile, "\t.globl " PREFIX "CALL32_Init\n" );
    fprintf( outfile, "\t.type " PREFIX "CALL32_Init,@function\n" );
    fprintf( outfile, PREFIX "CALL32_Init:\n" );
    fprintf( outfile, "\tmovl %%esp,CALL32_Original32_esp\n" );
    fprintf( outfile, "\tpopl %%eax\n" );
    fprintf( outfile, "\tpopl %%eax\n" );
    fprintf( outfile, "\tmovl $CALL32_LargeStack,(%%eax)\n" );
    fprintf( outfile, "\tpopl %%eax\n" );
    fprintf( outfile, "\tpopl %%esp\n" );
    fprintf( outfile, "\tpushl %%eax\n" );
    fprintf( outfile, "\tret\n" );

    /* Function header */

    fprintf( outfile, "\n\t.align 4\n" );
#ifdef USE_STABS
    fprintf( outfile, ".stabs \"CALL32_LargeStack:F1\",36,0,0,CALL32_LargeStack\n");
#endif
    fprintf( outfile, "CALL32_LargeStack:\n" );
    
    /* Entry code */

    fprintf( outfile, "\tpushl %%ebp\n" );
    fprintf( outfile, "\tmovl %%esp,%%ebp\n" );

    /* Switch to the original 32-bit stack pointer */

    fprintf( outfile, "\tcmpl $0, CALL32_RecursionCount\n" );
    fprintf( outfile, "\tjne  CALL32_skip\n" );
    fprintf( outfile, "\tmovl CALL32_Original32_esp, %%esp\n" );
    fprintf( outfile, "CALL32_skip:\n" );

    fprintf( outfile, "\tincl CALL32_RecursionCount\n" );

    /* Transfer the argument and call the function */

    fprintf( outfile, "\tpushl 12(%%ebp)\n" );
    fprintf( outfile, "\tcall *8(%%ebp)\n" );

    /* Restore registers and return */

    fprintf( outfile, "\tdecl CALL32_RecursionCount\n" );

    fprintf( outfile, "\tmovl %%ebp,%%esp\n" );
    fprintf( outfile, "\tpopl %%ebp\n" );
    fprintf( outfile, "\tret\n" );

    /* Data */

    fprintf( outfile, "\t.data\n" );
    fprintf( outfile, "CALL32_Original32_esp:\t.long 0\n" );
    fprintf( outfile, "CALL32_RecursionCount:\t.long 0\n" );
    fprintf( outfile, "\t.text\n" );
}


/*******************************************************************
 *         BuildCallFrom32Regs
 *
 * Build a 32-bit-to-Wine call-back function for a 'register' function.
 * 'args' is the number of dword arguments.
 *
 * Stack layout:
 *   ...
 * (ebp+12)  first arg
 * (ebp+8)   ret addr to user code
 * (ebp+4)   ret addr to relay code
 * (ebp+0)   saved ebp
 * (ebp-128) buffer area to allow stack frame manipulation
 * (ebp-332) CONTEXT86 struct
 * (ebp-336) CONTEXT86 *argument
 *  ....     other arguments copied from (ebp+12)
 *
 * The entry point routine is called with a CONTEXT* extra argument,
 * following the normal args. In this context structure, EIP_reg
 * contains the return address to user code, and ESP_reg the stack
 * pointer on return (with the return address and arguments already
 * removed).
 */
static void BuildCallFrom32Regs( FILE *outfile )
{
    static const int STACK_SPACE = 128 + sizeof(CONTEXT86);

    /* Function header */

    fprintf( outfile, "\n\t.align 4\n" );
#ifdef USE_STABS
    fprintf( outfile, ".stabs \"CALL32_Regs:F1\",36,0,0," PREFIX "CALL32_Regs\n" );
#endif
    fprintf( outfile, "\t.globl " PREFIX "CALL32_Regs\n" );
    fprintf( outfile, PREFIX "CALL32_Regs:\n" );

    /* Allocate some buffer space on the stack */

    fprintf( outfile, "\tpushl %%ebp\n" );
    fprintf( outfile, "\tmovl %%esp,%%ebp\n ");
    fprintf( outfile, "\tleal -%d(%%esp), %%esp\n", STACK_SPACE );
    
    /* Build the context structure */

    fprintf( outfile, "\tmovl %%eax,%d(%%ebp)\n", CONTEXTOFFSET(Eax) - STACK_SPACE );
    fprintf( outfile, "\tpushfl\n" );
    fprintf( outfile, "\tpopl %%eax\n" );
    fprintf( outfile, "\tmovl %%eax,%d(%%ebp)\n", CONTEXTOFFSET(EFlags) - STACK_SPACE );
    fprintf( outfile, "\tmovl 0(%%ebp),%%eax\n" );
    fprintf( outfile, "\tmovl %%eax,%d(%%ebp)\n", CONTEXTOFFSET(Ebp) - STACK_SPACE );
    fprintf( outfile, "\tmovl %%ebx,%d(%%ebp)\n", CONTEXTOFFSET(Ebx) - STACK_SPACE );
    fprintf( outfile, "\tmovl %%ecx,%d(%%ebp)\n", CONTEXTOFFSET(Ecx) - STACK_SPACE );
    fprintf( outfile, "\tmovl %%edx,%d(%%ebp)\n", CONTEXTOFFSET(Edx) - STACK_SPACE );
    fprintf( outfile, "\tmovl %%esi,%d(%%ebp)\n", CONTEXTOFFSET(Esi) - STACK_SPACE );
    fprintf( outfile, "\tmovl %%edi,%d(%%ebp)\n", CONTEXTOFFSET(Edi) - STACK_SPACE );

    fprintf( outfile, "\txorl %%eax,%%eax\n" );
    fprintf( outfile, "\tmovw %%cs,%%ax\n" );
    fprintf( outfile, "\tmovl %%eax,%d(%%ebp)\n", CONTEXTOFFSET(SegCs) - STACK_SPACE );
    fprintf( outfile, "\tmovw %%es,%%ax\n" );
    fprintf( outfile, "\tmovl %%eax,%d(%%ebp)\n", CONTEXTOFFSET(SegEs) - STACK_SPACE );
    fprintf( outfile, "\tmovw %%fs,%%ax\n" );
    fprintf( outfile, "\tmovl %%eax,%d(%%ebp)\n", CONTEXTOFFSET(SegFs) - STACK_SPACE );
    fprintf( outfile, "\tmovw %%gs,%%ax\n" );
    fprintf( outfile, "\tmovl %%eax,%d(%%ebp)\n", CONTEXTOFFSET(SegGs) - STACK_SPACE );
    fprintf( outfile, "\tmovw %%ss,%%ax\n" );
    fprintf( outfile, "\tmovl %%eax,%d(%%ebp)\n", CONTEXTOFFSET(SegSs) - STACK_SPACE );
    fprintf( outfile, "\tmovw %%ds,%%ax\n" );
    fprintf( outfile, "\tmovl %%eax,%d(%%ebp)\n", CONTEXTOFFSET(SegDs) - STACK_SPACE );
    fprintf( outfile, "\tmovw %%ax,%%es\n" );  /* set %es equal to %ds just in case */

    fprintf( outfile, "\tmovl $0x%x,%%eax\n", CONTEXT86_FULL );
    fprintf( outfile, "\tmovl %%eax,%d(%%ebp)\n", CONTEXTOFFSET(ContextFlags) - STACK_SPACE );

    fprintf( outfile, "\tmovl 8(%%ebp),%%eax\n" ); /* Get %eip at time of call */
    fprintf( outfile, "\tmovl %%eax,%d(%%ebp)\n", CONTEXTOFFSET(Eip) - STACK_SPACE );

    /* Transfer the arguments */

    fprintf( outfile, "\tmovl 4(%%ebp),%%ebx\n" );   /* get relay code addr */
    fprintf( outfile, "\tpushl %%esp\n" );           /* push ptr to context struct */
    fprintf( outfile, "\tmovzbl 4(%%ebx),%%ecx\n" ); /* fetch number of args to copy */
    fprintf( outfile, "\tjecxz 1f\n" );
    fprintf( outfile, "\tsubl %%ecx,%%esp\n" );
    fprintf( outfile, "\tleal 12(%%ebp),%%esi\n" );  /* get %esp at time of call */
    fprintf( outfile, "\tmovl %%esp,%%edi\n" );
    fprintf( outfile, "\tshrl $2,%%ecx\n" );
    fprintf( outfile, "\tcld\n" );
    fprintf( outfile, "\trep\n\tmovsl\n" );  /* copy args */

    fprintf( outfile, "1:\tmovzbl 5(%%ebx),%%eax\n" ); /* fetch number of args to remove */
    fprintf( outfile, "\tleal 12(%%ebp,%%eax),%%eax\n" );
    fprintf( outfile, "\tmovl %%eax,%d(%%ebp)\n", CONTEXTOFFSET(Esp) - STACK_SPACE );

    /* Call the entry point */

    fprintf( outfile, "\tcall *0(%%ebx)\n" );

    /* Store %eip and %ebp onto the new stack */

    fprintf( outfile, "\tmovl %d(%%ebp),%%edx\n", CONTEXTOFFSET(Esp) - STACK_SPACE );
    fprintf( outfile, "\tmovl %d(%%ebp),%%eax\n", CONTEXTOFFSET(Eip) - STACK_SPACE );
    fprintf( outfile, "\tmovl %%eax,-4(%%edx)\n" );
    fprintf( outfile, "\tmovl %d(%%ebp),%%eax\n", CONTEXTOFFSET(Ebp) - STACK_SPACE );
    fprintf( outfile, "\tmovl %%eax,-8(%%edx)\n" );

    /* Restore the context structure */

    /* Note: we don't bother to restore %cs, %ds and %ss
     *       changing them in 32-bit code is a recipe for disaster anyway
     */
    fprintf( outfile, "\tmovl %d(%%ebp),%%eax\n", CONTEXTOFFSET(SegEs) - STACK_SPACE );
    fprintf( outfile, "\tmovw %%ax,%%es\n" );
    fprintf( outfile, "\tmovl %d(%%ebp),%%eax\n", CONTEXTOFFSET(SegFs) - STACK_SPACE );
    fprintf( outfile, "\tmovw %%ax,%%fs\n" );
    fprintf( outfile, "\tmovl %d(%%ebp),%%eax\n", CONTEXTOFFSET(SegGs) - STACK_SPACE );
    fprintf( outfile, "\tmovw %%ax,%%gs\n" );

    fprintf( outfile, "\tmovl %d(%%ebp),%%edi\n", CONTEXTOFFSET(Edi) - STACK_SPACE );
    fprintf( outfile, "\tmovl %d(%%ebp),%%esi\n", CONTEXTOFFSET(Esi) - STACK_SPACE );
    fprintf( outfile, "\tmovl %d(%%ebp),%%edx\n", CONTEXTOFFSET(Edx) - STACK_SPACE );
    fprintf( outfile, "\tmovl %d(%%ebp),%%ecx\n", CONTEXTOFFSET(Ecx) - STACK_SPACE );
    fprintf( outfile, "\tmovl %d(%%ebp),%%ebx\n", CONTEXTOFFSET(Ebx) - STACK_SPACE );

    fprintf( outfile, "\tmovl %d(%%ebp),%%eax\n", CONTEXTOFFSET(EFlags) - STACK_SPACE );
    fprintf( outfile, "\tpushl %%eax\n" );
    fprintf( outfile, "\tpopfl\n" );
    fprintf( outfile, "\tmovl %d(%%ebp),%%eax\n", CONTEXTOFFSET(Eax) - STACK_SPACE );

    fprintf( outfile, "\tmovl %d(%%ebp),%%ebp\n", CONTEXTOFFSET(Esp) - STACK_SPACE );
    fprintf( outfile, "\tleal -8(%%ebp),%%esp\n" );
    fprintf( outfile, "\tpopl %%ebp\n" );
    fprintf( outfile, "\tret\n" );
}


/*******************************************************************
 *         BuildGlue
 *
 * Build the 16-bit-to-Wine/Wine-to-16-bit callback glue code
 */
static void BuildGlue( FILE *outfile, FILE *infile )
{
    char buffer[1024];

    /* File header */

    fprintf( outfile, "/* File generated automatically from %s; do not edit! */\n\n",
             input_file_name );
    fprintf( outfile, "#include \"builtin16.h\"\n" );
    fprintf( outfile, "#include \"stackframe.h\"\n\n" );

    /* Build the callback glue functions */

    while (fgets( buffer, sizeof(buffer), infile ))
    {
        if (strstr( buffer, "### start build ###" )) break;
    }
    while (fgets( buffer, sizeof(buffer), infile ))
    {
        char *p; 
        if ( (p = strstr( buffer, "CallFrom16_" )) != NULL )
        {
            char *q, *profile = p + strlen( "CallFrom16_" );
            for (q = profile; (*q == '_') || isalpha(*q); q++ )
                ;
            *q = '\0';
            for (q = p-1; q > buffer && ((*q == '_') || isalnum(*q)); q-- )
                ;
            if ( ++q < p ) p[-1] = '\0'; else q = "";
            BuildCallFrom16Func( outfile, profile, q, FALSE );
        }
        if ( (p = strstr( buffer, "CallTo16_" )) != NULL )
        {
            char *q, *profile = p + strlen( "CallTo16_" );
            for (q = profile; (*q == '_') || isalpha(*q); q++ )
                ;
            *q = '\0';
            for (q = p-1; q > buffer && ((*q == '_') || isalnum(*q)); q-- )
                ;
            if ( ++q < p ) p[-1] = '\0'; else q = "";
            BuildCallTo16Func( outfile, profile, q );
        }
        if (strstr( buffer, "### stop build ###" )) break;
    }

    fclose( infile );
}

/*******************************************************************
 *         BuildCall16
 *
 * Build the 16-bit callbacks
 */
static void BuildCall16( FILE *outfile )
{
    /* File header */

    fprintf( outfile, "/* File generated automatically. Do not edit! */\n\n" );
    fprintf( outfile, "\t.text\n" );

#ifdef __i386__

#ifdef USE_STABS
    if (output_file_name)
    {
        char buffer[1024];
        getcwd(buffer, sizeof(buffer));
        fprintf( outfile, "\t.file\t\"%s\"\n", output_file_name );

        /*
         * The stabs help the internal debugger as they are an indication that it
         * is sensible to step into a thunk/trampoline.
         */
        fprintf( outfile, ".stabs \"%s/\",100,0,0,Code_Start\n", buffer);
        fprintf( outfile, ".stabs \"%s\",100,0,0,Code_Start\n", output_file_name );
        fprintf( outfile, "Code_Start:\n\n" );
    }
#endif
    fprintf( outfile, PREFIX"Call16_Start:\n" );
    fprintf( outfile, "\t.globl "PREFIX"Call16_Start\n" );
    fprintf( outfile, "\t.byte 0\n\n" );


    /* Standard CallFrom16 routine (WORD return) */
    BuildCallFrom16Core( outfile, FALSE, FALSE, TRUE );

    /* Standard CallFrom16 routine (DWORD return) */
    BuildCallFrom16Core( outfile, FALSE, FALSE, FALSE );

    /* Register CallFrom16 routine */
    BuildCallFrom16Core( outfile, TRUE, FALSE, FALSE );

    /* C16ThkSL CallFrom16 routine */
    BuildCallFrom16Core( outfile, FALSE, TRUE, FALSE );

    /* Standard CallTo16 routine (WORD return) */
    BuildCallTo16Core( outfile, TRUE, FALSE );

    /* Standard CallTo16 routine (DWORD return) */
    BuildCallTo16Core( outfile, FALSE, FALSE );

    /* Register CallTo16 routine (16:16 retf) */
    BuildCallTo16Core( outfile, FALSE, 1 );

    /* Register CallTo16 routine (16:32 retf) */
    BuildCallTo16Core( outfile, FALSE, 2 );

    /* CBClientThunkSL routine */
    BuildCallTo32CBClient( outfile, FALSE );

    /* CBClientThunkSLEx routine */
    BuildCallTo32CBClient( outfile, TRUE  );

    fprintf( outfile, PREFIX"Call16_End:\n" );
    fprintf( outfile, "\t.globl "PREFIX"Call16_End\n" );

#ifdef USE_STABS
    fprintf( outfile, "\t.stabs \"\",100,0,0,.Letext\n");
    fprintf( outfile, ".Letext:\n");
#endif

    /* The whole Call16_Ret segment must lie within the .data section */
    fprintf( outfile, "\n\t.data\n" );
    fprintf( outfile, "\t.globl " PREFIX "Call16_Ret_Start\n" );
    fprintf( outfile, PREFIX "Call16_Ret_Start:\n" );

    /* Standard CallTo16 return stub */
    BuildRet16Func( outfile );

    /* CBClientThunkSL return stub */
    BuildCallTo32CBClientRet( outfile, FALSE );

    /* CBClientThunkSLEx return stub */
    BuildCallTo32CBClientRet( outfile, TRUE  );

    /* End of Call16_Ret segment */
    fprintf( outfile, "\n\t.globl " PREFIX "Call16_Ret_End\n" );
    fprintf( outfile, PREFIX "Call16_Ret_End:\n" );

#else  /* __i386__ */

    fprintf( outfile, PREFIX"Call16_Start:\n" );
    fprintf( outfile, "\t.globl "PREFIX"Call16_Start\n" );
    fprintf( outfile, "\t.byte 0\n\n" );
    fprintf( outfile, PREFIX"Call16_End:\n" );
    fprintf( outfile, "\t.globl "PREFIX"Call16_End\n" );

    fprintf( outfile, "\t.globl " PREFIX "Call16_Ret_Start\n" );
    fprintf( outfile, PREFIX "Call16_Ret_Start:\n" );
    fprintf( outfile, "\t.byte 0\n\n" );
    fprintf( outfile, "\n\t.globl " PREFIX "Call16_Ret_End\n" );
    fprintf( outfile, PREFIX "Call16_Ret_End:\n" );

#endif  /* __i386__ */
}

/*******************************************************************
 *         BuildCall32
 *
 * Build the 32-bit callbacks
 */
static void BuildCall32( FILE *outfile )
{
    /* File header */

    fprintf( outfile, "/* File generated automatically. Do not edit! */\n\n" );
    fprintf( outfile, "\t.text\n" );

#ifdef __i386__

#ifdef USE_STABS
    if (output_file_name)
    {
        char buffer[1024];
        getcwd(buffer, sizeof(buffer));
        fprintf( outfile, "\t.file\t\"%s\"\n", output_file_name );

        /*
         * The stabs help the internal debugger as they are an indication that it
         * is sensible to step into a thunk/trampoline.
         */
        fprintf( outfile, ".stabs \"%s/\",100,0,0,Code_Start\n", buffer);
        fprintf( outfile, ".stabs \"%s\",100,0,0,Code_Start\n", output_file_name );
        fprintf( outfile, "Code_Start:\n" );
    }
#endif

    /* Build the 32-bit large stack callback */

    BuildCallTo32LargeStack( outfile );

    /* Build the register callback function */

    BuildCallFrom32Regs( outfile );

#ifdef USE_STABS
    fprintf( outfile, "\t.text\n");
    fprintf( outfile, "\t.stabs \"\",100,0,0,.Letext\n");
    fprintf( outfile, ".Letext:\n");
#endif

#else  /* __i386__ */

    /* Just to avoid an empty file */
    fprintf( outfile, "\t.long 0\n" );

#endif  /* __i386__ */
}


/*******************************************************************
 *         usage
 */
static void usage(void)
{
    fprintf( stderr,
             "usage: build [-pic] [-o outfile] -spec SPEC_FILE\n"
             "       build [-pic] [-o outfile] -glue SOURCE_FILE\n"
             "       build [-pic] [-o outfile] -call16\n"
             "       build [-pic] [-o outfile] -call32\n" );
    if (output_file_name) unlink( output_file_name );
    exit(1);
}


/*******************************************************************
 *         open_input
 */
static FILE *open_input( const char *name )
{
    FILE *f;

    if (!name)
    {
        input_file_name = "<stdin>";
        return stdin;
    }
    input_file_name = name;
    if (!(f = fopen( name, "r" )))
    {
        fprintf( stderr, "Cannot open input file '%s'\n", name );
        if (output_file_name) unlink( output_file_name );
        exit(1);
    }
    return f;
}

/*******************************************************************
 *         main
 */
int main(int argc, char **argv)
{
    FILE *outfile = stdout;

    if (argc < 2) usage();

    if (!strcmp( argv[1], "-pic" ))
    {
        UsePIC = 1;
        argv += 1;
        argc -= 1;
        if (argc < 2) usage();
    }

    if (!strcmp( argv[1], "-o" ))
    {
        output_file_name = argv[2];
        argv += 2;
        argc -= 2;
        if (argc < 2) usage();
        if (!(outfile = fopen( output_file_name, "w" )))
        {
            fprintf( stderr, "Unable to create output file '%s'\n", output_file_name );
            exit(1);
        }
    }

    /* Retrieve the selector values; this assumes that we are building
     * the asm files on the platform that will also run them. Probably
     * a safe assumption to make.
     */
    GET_CS( Code_Selector );
    GET_DS( Data_Selector );

    if (!strcmp( argv[1], "-spec" )) BuildSpecFile( outfile, open_input( argv[2] ) );
    else if (!strcmp( argv[1], "-glue" )) BuildGlue( outfile, open_input( argv[2] ) );
    else if (!strcmp( argv[1], "-call16" )) BuildCall16( outfile );
    else if (!strcmp( argv[1], "-call32" )) BuildCall32( outfile );
    else
    {
        fclose( outfile );
        usage();
    }

    fclose( outfile );
    return 0;
}
