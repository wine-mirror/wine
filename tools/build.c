/*
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
 */

#include <assert.h>
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
    TYPE_INVALID,
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
    NULL,
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
    int arg_size;
    int ret_value;
} ORD_RETURN;

typedef struct
{
    int value;
} ORD_ABS;

typedef struct
{
    char link_name[80];
} ORD_VARARGS;

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
    int         offset;
    int		lineno;
    char        name[80];
    union
    {
        ORD_VARIABLE   var;
        ORD_FUNCTION   func;
        ORD_RETURN     ret;
        ORD_ABS        abs;
        ORD_VARARGS    vargs;
        ORD_EXTERN     ext;
        ORD_FORWARD    fwd;
    } u;
} ORDDEF;

static ORDDEF OrdinalDefinitions[MAX_ORDINALS];

static SPEC_TYPE SpecType = SPEC_INVALID;
static char DLLName[80];
static char DLLFileName[80];
static int Limit = 0;
static int Base = MAX_ORDINALS;
static int DLLHeapSize = 0;
static char *SpecName;
static FILE *SpecFp;
static WORD Code_Selector, Data_Selector;
static char DLLInitFunc[80];
static char *DLLImports[MAX_IMPORTS];
static int nb_imports = 0;

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


static void *xmalloc (size_t size)
{
    void *res;

    res = malloc (size ? size : 1);
    if (res == NULL)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
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
        exit (1);
    }
    return res;
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


/*******************************************************************
 *         ParseVariable
 *
 * Parse a variable definition.
 */
static int ParseVariable( ORDDEF *odp )
{
    char *endptr;
    int *value_array;
    int n_values;
    int value_array_size;
    
    char *token = GetToken();
    if (*token != '(')
    {
	fprintf(stderr, "%s:%d: Expected '(' got '%s'\n",
                SpecName, Line, token);
	return -1;
    }

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
	{
	    fprintf(stderr, "%s:%d: Expected number value, got '%s'\n",
                    SpecName, Line, token);
	    return -1;
	}
    }
    
    if (token == NULL)
    {
	fprintf(stderr, "%s:%d: End of file in variable declaration\n",
                SpecName, Line);
	return -1;
    }

    odp->u.var.n_values = n_values;
    odp->u.var.values = xrealloc(value_array, sizeof(*value_array) * n_values);

    return 0;
}


/*******************************************************************
 *         ParseExportFunction
 *
 * Parse a function definition.
 */
static int ParseExportFunction( ORDDEF *odp )
{
    char *token;
    int i;

    switch(SpecType)
    {
    case SPEC_WIN16:
        if (odp->type == TYPE_STDCALL)
        {
            fprintf( stderr, "%s:%d: 'stdcall' not supported for Win16\n",
                     SpecName, Line );
            return -1;
        }
        break;
    case SPEC_WIN32:
        if ((odp->type == TYPE_PASCAL) || (odp->type == TYPE_PASCAL_16))
        {
            fprintf( stderr, "%s:%d: 'pascal' not supported for Win32\n",
                     SpecName, Line );
            return -1;
        }
        break;
    default:
        break;
    }

    token = GetToken();
    if (*token != '(')
    {
	fprintf(stderr, "%s:%d: Expected '(' got '%s'\n",
                SpecName, Line, token);
	return -1;
    }

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
        else
        {
            fprintf(stderr, "%s:%d: Unknown variable type '%s'\n",
                    SpecName, Line, token);
            return -1;
        }
        if (SpecType == SPEC_WIN32)
        {
            if (strcmp(token, "long") &&
                strcmp(token, "ptr") &&
                strcmp(token, "str") &&
                strcmp(token, "wstr") &&
                strcmp(token, "double"))
            {
                fprintf( stderr, "%s:%d: Type '%s' not supported for Win32\n",
                         SpecName, Line, token );
                return -1;
            }
        }
    }
    if ((*token != ')') || (i >= sizeof(odp->u.func.arg_types)))
    {
        fprintf( stderr, "%s:%d: Too many arguments\n", SpecName, Line );
        return -1;
    }
    odp->u.func.arg_types[i] = '\0';
    if ((odp->type == TYPE_STDCALL) && !i)
        odp->type = TYPE_CDECL; /* stdcall is the same as cdecl for 0 args */
    strcpy(odp->u.func.link_name, GetToken());
    return 0;
}


/*******************************************************************
 *         ParseEquate
 *
 * Parse an 'equate' definition.
 */
static int ParseEquate( ORDDEF *odp )
{
    char *endptr;
    
    char *token = GetToken();
    int value = strtol(token, &endptr, 0);
    if (endptr == NULL || *endptr != '\0')
    {
	fprintf(stderr, "%s:%d: Expected number value, got '%s'\n",
                SpecName, Line, token);
	return -1;
    }

    if (SpecType == SPEC_WIN32)
    {
        fprintf( stderr, "%s:%d: 'equate' not supported for Win32\n",
                 SpecName, Line );
        return -1;
    }

    odp->u.abs.value = value;
    return 0;
}


/*******************************************************************
 *         ParseStub
 *
 * Parse a 'stub' definition.
 */
static int ParseStub( ORDDEF *odp )
{
    odp->u.func.arg_types[0] = '\0';
    strcpy( odp->u.func.link_name, STUB_CALLBACK );
    return 0;
}


/*******************************************************************
 *         ParseVarargs
 *
 * Parse an 'varargs' definition.
 */
static int ParseVarargs( ORDDEF *odp )
{
    char *token;

    if (SpecType == SPEC_WIN16)
    {
        fprintf( stderr, "%s:%d: 'varargs' not supported for Win16\n",
                 SpecName, Line );
        return -1;
    }

    token = GetToken();
    if (*token != '(')
    {
	fprintf(stderr, "%s:%d: Expected '(' got '%s'\n",
                SpecName, Line, token);
	return -1;
    }
    token = GetToken();
    if (*token != ')')
    {
	fprintf(stderr, "%s:%d: Expected ')' got '%s'\n",
                SpecName, Line, token);
	return -1;
    }

    strcpy( odp->u.vargs.link_name, GetToken() );
    return 0;
}

/*******************************************************************
 *         ParseInterrupt
 *
 * Parse an 'interrupt' definition.
 */
static int ParseInterrupt( ORDDEF *odp )
{
    char *token;

    if (SpecType == SPEC_WIN32)
    {
        fprintf( stderr, "%s:%d: 'interrupt' not supported for Win32\n",
                 SpecName, Line );
        return -1;
    }

    token = GetToken();
    if (*token != '(')
    {
	fprintf(stderr, "%s:%d: Expected '(' got '%s'\n",
                SpecName, Line, token);
	return -1;
    }
    token = GetToken();
    if (*token != ')')
    {
	fprintf(stderr, "%s:%d: Expected ')' got '%s'\n",
                SpecName, Line, token);
	return -1;
    }

    odp->u.func.arg_types[0] = '\0';
    strcpy( odp->u.func.link_name, GetToken() );
    return 0;
}


/*******************************************************************
 *         ParseExtern
 *
 * Parse an 'extern' definition.
 */
static int ParseExtern( ORDDEF *odp )
{
    if (SpecType == SPEC_WIN16)
    {
        fprintf( stderr, "%s:%d: 'extern' not supported for Win16\n",
                 SpecName, Line );
        return -1;
    }
    strcpy( odp->u.ext.link_name, GetToken() );
    return 0;
}


/*******************************************************************
 *         ParseForward
 *
 * Parse a 'forward' definition.
 */
static int ParseForward( ORDDEF *odp )
{
    if (SpecType == SPEC_WIN16)
    {
        fprintf( stderr, "%s:%d: 'forward' not supported for Win16\n",
                 SpecName, Line );
        return -1;
    }
    strcpy( odp->u.fwd.link_name, GetToken() );
    return 0;
}


/*******************************************************************
 *         ParseOrdinal
 *
 * Parse an ordinal definition.
 */
static int ParseOrdinal(int ordinal)
{
    ORDDEF *odp;
    char *token;

    if (ordinal >= MAX_ORDINALS)
    {
	fprintf(stderr, "%s:%d: Ordinal number too large\n", SpecName, Line );
	return -1;
    }
    if (ordinal > Limit) Limit = ordinal;
    if (ordinal < Base) Base = ordinal;

    odp = &OrdinalDefinitions[ordinal];
    if (!(token = GetToken()))
    {
	fprintf(stderr, "%s:%d: Expected type after ordinal\n", SpecName, Line);
	return -1;
    }

    for (odp->type = 0; odp->type < TYPE_NBTYPES; odp->type++)
        if (TypeNames[odp->type] && !strcmp( token, TypeNames[odp->type] ))
            break;

    if (odp->type >= TYPE_NBTYPES)
    {
        fprintf( stderr,
                 "%s:%d: Expected type after ordinal, found '%s' instead\n",
                 SpecName, Line, token );
        return -1;
    }

    if (!(token = GetToken()))
    {
        fprintf( stderr, "%s:%d: Expected name after type\n", SpecName, Line );
        return -1;
    }
    strcpy( odp->name, token );
    odp->lineno = Line;

    switch(odp->type)
    {
    case TYPE_BYTE:
    case TYPE_WORD:
    case TYPE_LONG:
	return ParseVariable( odp );
    case TYPE_PASCAL_16:
    case TYPE_PASCAL:
    case TYPE_REGISTER:
    case TYPE_STDCALL:
    case TYPE_CDECL:
	return ParseExportFunction( odp );
    case TYPE_INTERRUPT:
	return ParseInterrupt( odp );
    case TYPE_ABS:
	return ParseEquate( odp );
    case TYPE_STUB:
	return ParseStub( odp );
    case TYPE_VARARGS:
	return ParseVarargs( odp );
    case TYPE_EXTERN:
	return ParseExtern( odp );
    case TYPE_FORWARD:
	return ParseForward( odp );
    default:
        fprintf( stderr, "Should not happen\n" );
        return -1;
    }
}


/*******************************************************************
 *         ParseTopLevel
 *
 * Parse a spec file.
 */
static int ParseTopLevel(void)
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
            else
            {
                fprintf(stderr, "%s:%d: Type must be 'win16' or 'win32'\n",
                        SpecName, Line);
                return -1;
            }
        }
	else if (strcmp(token, "heap") == 0)
	{
            token = GetToken();
            if (!IsNumberString(token))
            {
		fprintf(stderr, "%s:%d: Expected number after heap\n",
                        SpecName, Line);
		return -1;
            }
            DLLHeapSize = atoi(token);
	}
        else if (strcmp(token, "init") == 0)
        {
            strcpy(DLLInitFunc, GetToken());
            if (!DLLInitFunc[0])
                fprintf(stderr, "%s:%d: Expected function name after init\n", SpecName, Line);
        }
        else if (strcmp(token, "import") == 0)
        {
            if (nb_imports >= MAX_IMPORTS)
            {
                fprintf( stderr, "%s:%d: Too many imports (limit %d)\n",
                         SpecName, Line, MAX_IMPORTS );
                return -1;
            }
            if (SpecType != SPEC_WIN32)
            {
                fprintf( stderr, "%s:%d: Imports not supported for Win16\n", SpecName, Line );
                return -1;
            }
            DLLImports[nb_imports++] = xstrdup(GetToken());
        }
	else if (IsNumberString(token))
	{
	    int ordinal;
	    int rv;
	    
	    ordinal = atoi(token);
	    if ((rv = ParseOrdinal(ordinal)) < 0)
		return rv;
	}
	else
	{
	    fprintf(stderr, 
		    "%s:%d: Expected name, id, length or ordinal\n",
                    SpecName, Line);
	    return -1;
	}
    }

    return 0;
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
    ORDDEF *odp;
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
    pModule->expected_version = 0x030a;
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
    odp = OrdinalDefinitions + 1;
    for (i = 1; i <= Limit; i++, odp++)
    {
        if (!odp->name[0]) continue;
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
    odp = OrdinalDefinitions + 1;
    for (i = 1; i <= Limit; i++, odp++)
    {
        int selector = 0;

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
static int BuildSpec32File( char * specfile, FILE *outfile )
{
    ORDDEF *odp;
    int i, nb_names, fwd_size = 0;

    fprintf( outfile, "/* File generated automatically from %s; do not edit! */\n\n",
             specfile );
    fprintf( outfile, "#include \"builtin32.h\"\n\n" );

    /* Output code for all stubs functions */

    fprintf( outfile, "extern const BUILTIN32_DESCRIPTOR %s_Descriptor;\n",
             DLLName );
    for (i = Base, odp = OrdinalDefinitions + Base; i <= Limit; i++, odp++)
    {
        if (odp->type != TYPE_STUB) continue;
        fprintf( outfile, "static void __stub_%d() { BUILTIN32_Unimplemented(&%s_Descriptor,%d); }\n",
                 i, DLLName, i );
    }

    /* Output code for all register functions */

    fprintf( outfile, "#ifdef __i386__\n" );
    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "static void __asm__dummy() {\n" );
    fprintf( outfile, "#endif /* !defined(__GNUC__) */\n" );
    for (i = Base, odp = OrdinalDefinitions + Base; i <= Limit; i++, odp++)
    {
        if (odp->type != TYPE_REGISTER) continue;
        fprintf( outfile,
                 "__asm__(\".align 4\\n\\t\"\n"
                 "        \".globl " PREFIX "%s\\n\\t\"\n"
                 "        \".type " PREFIX "%s,@function\\n\\t\"\n"
                 "        \"" PREFIX "%s:\\n\\t\"\n"
                 "        \"call " PREFIX "CALL32_Regs\\n\\t\"\n"
                 "        \".long " PREFIX "__regs_%s\\n\\t\"\n"
                 "        \".byte %d,%d\");\n",
                 odp->u.func.link_name, odp->u.func.link_name,
                 odp->u.func.link_name, odp->u.func.link_name,
                 4 * strlen(odp->u.func.arg_types),
                 4 * strlen(odp->u.func.arg_types) );
    }
    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "}\n" );
    fprintf( outfile, "#endif /* !defined(__GNUC__) */\n" );
    fprintf( outfile, "#endif /* defined(__i386__) */\n" );

    /* Output the DLL functions prototypes */

    for (i = Base, odp = OrdinalDefinitions + Base; i <= Limit; i++, odp++)
    {
        switch(odp->type)
        {
        case TYPE_VARARGS:
            fprintf( outfile, "extern void %s();\n", odp->u.vargs.link_name );
            break;
        case TYPE_EXTERN:
            fprintf( outfile, "extern void %s();\n", odp->u.ext.link_name );
            break;
        case TYPE_REGISTER:
        case TYPE_STDCALL:
        case TYPE_CDECL:
            fprintf( outfile, "extern void %s();\n", odp->u.func.link_name );
            break;
        case TYPE_FORWARD:
            fwd_size += strlen(odp->u.fwd.link_name) + 1;
            break;
        case TYPE_INVALID:
        case TYPE_STUB:
            break;
        default:
            fprintf(stderr,"build: function type %d not available for Win32\n",
                    odp->type);
            return -1;
        }
    }

    /* Output LibMain function */
    if (DLLInitFunc[0]) fprintf( outfile, "extern void %s();\n", DLLInitFunc );


    /* Output the DLL functions table */

    fprintf( outfile, "\nstatic const ENTRYPOINT32 Functions[%d] =\n{\n",
             Limit - Base + 1 );
    for (i = Base, odp = OrdinalDefinitions + Base; i <= Limit; i++, odp++)
    {
        switch(odp->type)
        {
        case TYPE_INVALID:
            fprintf( outfile, "    0" );
            break;
        case TYPE_VARARGS:
            fprintf( outfile, "    %s", odp->u.vargs.link_name );
            break;
        case TYPE_EXTERN:
            fprintf( outfile, "    %s", odp->u.ext.link_name );
            break;
        case TYPE_REGISTER:
        case TYPE_STDCALL:
        case TYPE_CDECL:
            fprintf( outfile, "    %s", odp->u.func.link_name);
            break;
        case TYPE_STUB:
            fprintf( outfile, "    __stub_%d", i );
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

    nb_names = 0;
    fprintf( outfile, "static const char * const FuncNames[] =\n{\n" );
    for (i = Base, odp = OrdinalDefinitions + Base; i <= Limit; i++, odp++)
    {
        if (odp->type == TYPE_INVALID) continue;
        if (nb_names++) fprintf( outfile, ",\n" );
        fprintf( outfile, "    \"%s\"", odp->name );
    }
    fprintf( outfile, "\n};\n\n" );

    /* Output the DLL argument types */

    fprintf( outfile, "static const unsigned int ArgTypes[%d] =\n{\n",
             Limit - Base + 1 );
    for (i = Base, odp = OrdinalDefinitions + Base; i <= Limit; i++, odp++)
    {
    	unsigned int j, mask = 0;
	if ((odp->type == TYPE_STDCALL) || (odp->type == TYPE_CDECL) ||
            (odp->type == TYPE_REGISTER))
	    for (j = 0; odp->u.func.arg_types[j]; j++)
            {
                if (odp->u.func.arg_types[j] == 't') mask |= 1<< (j*2);
                if (odp->u.func.arg_types[j] == 'W') mask |= 2<< (j*2);
	    }
	fprintf( outfile, "    %d", mask );
        if (i < Limit) fprintf( outfile, ",\n" );
    }
    fprintf( outfile, "\n};\n\n" );

    /* Output the DLL ordinals table */

    fprintf( outfile, "static const unsigned short FuncOrdinals[] =\n{\n" );
    nb_names = 0;
    for (i = Base, odp = OrdinalDefinitions + Base; i <= Limit; i++, odp++)
    {
        if (odp->type == TYPE_INVALID) continue;
        if (nb_names++) fprintf( outfile, ",\n" );
        fprintf( outfile, "    %d", i - Base );
    }
    fprintf( outfile, "\n};\n\n" );

    /* Output the DLL functions arguments */

    fprintf( outfile, "static const unsigned char FuncArgs[%d] =\n{\n",
             Limit - Base + 1 );
    for (i = Base, odp = OrdinalDefinitions + Base; i <= Limit; i++, odp++)
    {
        unsigned char args;
        switch(odp->type)
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

    fprintf( outfile, "const BUILTIN32_DESCRIPTOR %s_Descriptor =\n{\n",
             DLLName );
    fprintf( outfile, "    \"%s\",\n", DLLName );
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
    fprintf( outfile, "    %s\n", DLLInitFunc[0] ? DLLInitFunc : "0" );
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
static int BuildSpec16File( char * specfile, FILE *outfile )
{
    ORDDEF *odp, **typelist;
    int i, j, k;
    int code_offset, data_offset, module_size;
    unsigned char *data;

    /* File header */

    fprintf( outfile, "/* File generated automatically from %s; do not edit! */\n\n",
             specfile );
    fprintf( outfile, "#define __FLATCS__ 0x%04x\n", Code_Selector );
    fprintf( outfile, "#include \"builtin16.h\"\n\n" );

    data = (unsigned char *)xmalloc( 0x10000 );
    memset( data, 0, 16 );
    data_offset = 16;


    /* Build sorted list of all argument types, without duplicates */

    typelist = (ORDDEF **)calloc( Limit+1, sizeof(ORDDEF *) );

    odp = OrdinalDefinitions;
    for (i = j = 0; i <= Limit; i++, odp++)
    {
        switch (odp->type)
        {
          case TYPE_REGISTER:
          case TYPE_INTERRUPT:
          case TYPE_CDECL:
          case TYPE_PASCAL:
          case TYPE_PASCAL_16:
          case TYPE_STUB:
            typelist[j++] = odp;

          default:
            break;
        }
    }

    qsort( typelist, j, sizeof(ORDDEF *), Spec16TypeCompare );

    i = k = 0;
    while ( i < j )
    {
        typelist[k++] = typelist[i++];
        while ( i < j && Spec16TypeCompare( typelist + i, typelist + k-1 ) == 0 )
            i++;
    }

    /* Output CallFrom16 profiles needed by this .spec file */

    fprintf( outfile, "\n/* ### start build ### */\n" );

    for ( i = 0; i < k; i++ )
        fprintf( outfile, "extern void %s_CallFrom16_%s_%s_%s();\n",
                 DLLName,
                 (typelist[i]->type == TYPE_CDECL) ? "c" : "p",
                 (typelist[i]->type == TYPE_REGISTER) ? "regs" :
                 (typelist[i]->type == TYPE_INTERRUPT) ? "intr" :
                 (typelist[i]->type == TYPE_PASCAL_16) ? "word" : "long",
                 typelist[i]->u.func.arg_types );

    fprintf( outfile, "/* ### stop build ### */\n\n" );

    /* Output the DLL functions prototypes */

    odp = OrdinalDefinitions;
    for (i = 0; i <= Limit; i++, odp++)
    {
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

    fprintf( outfile, "\nstatic ENTRYPOINT16 Code_Segment[] = \n{\n" );
    code_offset = 0;

    odp = OrdinalDefinitions;
    for (i = 0; i <= Limit; i++, odp++)
    {
        switch (odp->type)
        {
          case TYPE_INVALID:
            odp->offset = 0xffff;
            break;

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
            fprintf( outfile, "    /* %s.%d */ ", DLLName, i );
            fprintf( outfile, "EP( %s, %s_CallFrom16_%s_%s_%s ),\n",
                              odp->u.func.link_name,
                              DLLName,
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

    if (!code_offset)  /* Make sure the code segment is not empty */
    {
        fprintf( outfile, "    { },\n" );
        code_offset += sizeof(ENTRYPOINT16);
    }

    fprintf( outfile, "};\n" );

    /* Output data segment */

    DumpBytes( outfile, data, data_offset, "Data_Segment" );

    /* Build the module */

    module_size = BuildModule16( outfile, code_offset, data_offset );

    /* Output the DLL descriptor */

    fprintf( outfile, "\nWIN16_DESCRIPTOR %s_Descriptor = \n{\n", DLLName );
    fprintf( outfile, "    \"%s\",\n", DLLName );
    fprintf( outfile, "    Module,\n" );
    fprintf( outfile, "    sizeof(Module),\n" );
    fprintf( outfile, "    (BYTE *)Code_Segment,\n" );
    fprintf( outfile, "    (BYTE *)Data_Segment\n" );
    fprintf( outfile, "};\n" );
    
    return 0;
}


/*******************************************************************
 *         BuildSpecFile
 *
 * Build an assembly file from a spec file.
 */
static int BuildSpecFile( FILE *outfile, char *specname )
{
    SpecName = specname;
    SpecFp = fopen( specname, "r");
    if (SpecFp == NULL)
    {
	fprintf(stderr, "Could not open specification file, '%s'\n", specname);
        return -1;
    }

    if (ParseTopLevel() < 0) return -1;

    switch(SpecType)
    {
    case SPEC_WIN16:
        return BuildSpec16File( specname, outfile );
    case SPEC_WIN32:
        return BuildSpec32File( specname, outfile );
    default:
        fprintf( stderr, "%s: Missing 'type' declaration\n", specname );
        return -1;
    }
}


/*******************************************************************
 *         BuildCallFrom16Func
 *
 * Build a 16-bit-to-Wine callback glue function. The syntax of the function
 * profile is: call_type_xxxxx, where 'call' is the letter 'c' or 'p' for C or
 * Pascal calling convention, 'type' is one of 'regs', 'intr', 'word' or
 * 'long' and each 'x' is an argument ('w'=word, 's'=signed word,
 * 'l'=long, 'p'=linear pointer, 't'=linear pointer to null-terminated string,
 * 'T'=segmented pointer to null-terminated string).
 * For register functions, the arguments (if present) are converted just
 * the same as for normal functions, but in addition a CONTEXT pointer 
 * filled with the current register values is passed to the 32-bit routine.
 * A 'intr' interrupt handler routine is treated like a register routine
 * without arguments, except that upon return, the flags word pushed onto
 * the stack by the interrupt is removed.
 *
 * This glue function contains only that part of the 16->32 thunk that is
 * variable (depending on the type and number of arguments); the glue routine
 * is part of a particular DLL and uses a routine provided by the core to
 * perform those actions that do not depend on the argument type.
 *
 * The 16->32 glue routine consists of a main entry point (which must be part
 * of the ELF .data section) and two auxillary routines (in the .text section).
 * The main entry point pushes address of the 'Thunk' auxillary routine onto
 * the stack and then jumps to the appropriate core CallFrom16... routine.
 * Furthermore, at a fixed position relative to the main entry point, the 
 * function profile string is stored if relay debugging is active; this string
 * will be consulted by the debugging routines in the core to correctly 
 * display the arguments.
 *
 * The core will perform the switch to 32-bit, and then call back to the 
 * 'Thunk' auxillary routine.  This routine is expected to perform the
 * following tasks:
 *   - modify the auxillary routine address in the STACK16FRAME to now
 *     point to the 'ThunkRet' routine
 *   - convert arguemnts and push them onto the 32-bit stack
 *   - call the 32-bit target routine
 *
 * After the target routine (and then the 'Thunk' routine) have returned,
 * the core part will switch back to 16-bit, and finally jump to the 
 * 'ThunkRet' auxillary routine.  This routine is expected to convert the
 * return value if necessary (%eax -> %dx:%ax), and perform the appropriate
 * return to the 16-bit caller (lret or lret $argsize).
 *
 * In the case of a 'register' routine, there is no 'ThunkRet' auxillary
 * routine, as the reloading of all registers and return to 16-bit code
 * is done by the core routine.  The number of arguments to be popped off
 * the caller's stack must be returned (in %eax) by the 'Thunk' routine.
 *
 * NOTE: The generated routines contain only proper position-independent 
 *       code and may thus be used within shared objects (e.g. libwine.so 
 *       or elfdlls).  The exception is the main entry point, which must
 *       contain absolute relocations but cannot yet use the GOT; thus 
 *       this entry point is made part of the ELF .data section.
 */
static void BuildCallFrom16Func( FILE *outfile, char *profile, char *prefix )
{
    int i, pos, argsize = 0;
    int short_ret = 0;
    int reg_func = 0;
    int usecdecl = 0;
    char *args = profile + 7;

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

    /* 
     * Build main entry point (in .data section) 
     *
     * NOTE: If you change this, you must also change the retrieval of
     *       the profile string relative to the main entry point address
     *       (see BUILTIN_GetEntryPoint16 in if1632/builtin.c).
     */
    fprintf( outfile, "\t.data\n" );
    fprintf( outfile, "\t.globl " PREFIX "%s_CallFrom16_%s\n", prefix, profile );
    fprintf( outfile, PREFIX "%s_CallFrom16_%s:\n", prefix, profile );
    fprintf( outfile, "\tpushl $.L%s_Thunk_%s\n", prefix, profile );
    fprintf( outfile, "\tjmp " PREFIX "%s\n", 
             !reg_func? "CallFrom16" : "CallFrom16Register" );
    if ( debugging )
        fprintf( outfile, "\t" STRING " \"%s\\0\"\n", profile );

    /* 
     * Build *Thunk* routine 
     *
     *    This routine gets called by the main CallFrom16 routine with 
     *    registers set up as follows:
     *
     *        STACK16FRAME is completely set up on the 16-bit stack
     *        DS, ES, SS: flat data segment
     *        FS: current TEB
     *        ESP: points to 32-bit stack
     *        EBP: points to ebp member of last STACK32FRAME
     *        EDX: points to current STACK16FRAME
     *        ECX: points to ldt_copy
     */

    fprintf( outfile, "\t.text\n" );
    fprintf( outfile, ".L%s_Thunk_%s:\n", prefix, profile );

    if ( reg_func )
        fprintf( outfile, "\tleal 4(%%esp), %%eax\n"
                          "\tpushl %%eax\n" );
    else
        fprintf( outfile, "\taddl $[.L%s_ThunkRet_%s - .L%s_Thunk_%s], %d(%%edx)\n",
                          prefix, profile, prefix, profile, 
                          STACK16OFFSET(relay));

    /* Copy the arguments */
    pos = (usecdecl? argsize : 0) + sizeof( STACK16FRAME );
    args = profile + 7;
    for ( i = strlen(args)-1; i >= 0; i-- )
        switch (args[i])
        {
        case 'w':  /* word */
            if (  usecdecl ) pos -= 2;
            fprintf( outfile, "\tmovzwl %d(%%edx),%%eax\n", pos );
            fprintf( outfile, "\tpushl %%eax\n" );
            if ( !usecdecl ) pos += 2;
            break;

        case 's':  /* s_word */
            if (  usecdecl ) pos -= 2;
            fprintf( outfile, "\tmovswl %d(%%edx),%%eax\n", pos );
            fprintf( outfile, "\tpushl %%eax\n" );
            if ( !usecdecl ) pos += 2;
            break;

        case 'l':  /* long or segmented pointer */
        case 'T':  /* segmented pointer to null-terminated string */
            if (  usecdecl ) pos -= 4;
            fprintf( outfile, "\tpushl %d(%%edx)\n", pos );
            if ( !usecdecl ) pos += 4;
            break;

        case 'p':  /* linear pointer */
        case 't':  /* linear pointer to null-terminated string */
            if (  usecdecl ) pos -= 4;
            /* Get the selector */
            fprintf( outfile, "\tmovzwl %d(%%edx),%%eax\n", pos + 2 );
            /* Get the selector base */
            fprintf( outfile, "\tandl $0xfff8,%%eax\n" );
            fprintf( outfile, "\tpushl (%%ecx,%%eax)\n" );
            /* Add the offset */
            fprintf( outfile, "\tmovzwl %d(%%edx),%%eax\n", pos );
            fprintf( outfile, "\taddl %%eax,(%%esp)\n" );
            if ( !usecdecl ) pos += 4;
            break;

        default:
            fprintf( stderr, "Unknown arg type '%c'\n", args[i] );
        }

    /* Call entry point */
    fprintf( outfile, "\tcall *%d(%%edx)\n", STACK16OFFSET(entry_point) );

    if ( reg_func )
        fprintf( outfile, "\tmovl $%d, %%eax\n", reg_func == 2 ? 2 : argsize );

    fprintf( outfile, "\tret\n" );


    /* 
     * Build *ThunkRet* routine 
     * 
     *    At this point, all registers are set up for return to 16-bit code.
     *    EAX contains the function return value.
     *    SS:SP point to the return address to the caller (on 16-bit stack).
     */
    if ( !reg_func )
    {
        fprintf( outfile,  ".L%s_ThunkRet_%s:\n", prefix, profile );

        if ( !short_ret )
            fprintf( outfile, "\tshldl $16, %%eax, %%edx\n" );

        if ( !usecdecl && argsize )
        {
            fprintf( outfile, "\t.byte 0x66\n" );
            fprintf( outfile, "\tlret $%d\n", argsize );
        }
        else
        {
            fprintf( outfile, "\t.byte 0x66\n" );
            fprintf( outfile, "\tlret\n" );
        }
    }
    fprintf( outfile, "\n" );
}

/*******************************************************************
 *         BuildCallTo16Func
 *
 * Build a Wine-to-16-bit callback glue function.  As above, this glue
 * routine will only contain the argument-type dependent part of the
 * thunk; the rest will be done by a routine provided by the core.
 *
 * Prototypes for the CallTo16 functions:
 *   extern WORD PREFIX_CallTo16_word_xxx( FARPROC16 func, args... );
 *   extern LONG PREFIX_CallTo16_long_xxx( FARPROC16 func, args... );
 * 
 * The main entry point of the glue routine simply performs a call to
 * the proper core routine depending on the return type (WORD/LONG).
 * Note that the core will never perform a return from this call; however,
 * it will use the return address on the stack to access the other 
 * argument-type dependent parts of the thunk.  (If relay debugging is
 * active, the core routine will access the number of arguments stored
 * in the code section immediately precending the main entry point).
 * 
 * After the core routine has performed the switch to 16-bit code, it
 * will call the argument-transfer routine provided by the glue code
 * immediately after the main entry point.  This routine is expected
 * to transfer the arguments to the 16-bit stack, finish loading the
 * register for 16-bit code (%ds and %es must be loaded from %ss),
 * and call the 16-bit target.  
 *
 * The target will return to a 16:16 return address provided by the core.
 * The core will finalize the switch back to 32-bit and the return to
 * the caller without additional support by the glue code.  Note that
 * the 32-bit arguments will not be popped off the stack (hence the 
 * CallTo... routine must *not* be declared WINAPI/CALLBACK).
 *
 */
static void BuildCallTo16Func( FILE *outfile, char *profile, char *prefix )
{
    char *args = profile + 5;
    int pos, short_ret = 0;

    if (!strncmp( "word_", profile, 5 )) short_ret = 1;
    else if (strncmp( "long_", profile, 5 ))
    {
        fprintf( stderr, "Invalid function name '%s'.\n", profile );
        exit(1);
    }

    if ( debugging )
    {
        /* Number of arguments (for relay debugging) */
        fprintf( outfile, "\n\t.align 4\n" );
        fprintf( outfile, "\t.long %d\n", strlen(args) );
    }

    /* Main entry point */

#ifdef USE_STABS
    fprintf( outfile, ".stabs \"%s_CallTo16_%s:F1\",36,0,0," PREFIX "%s_CallTo16_%s\n", 
	     prefix, profile, prefix, profile);
#endif
    fprintf( outfile, "\t.globl " PREFIX "%s_CallTo16_%s\n", prefix, profile );
    fprintf( outfile, PREFIX "%s_CallTo16_%s:\n", prefix, profile );

    if ( short_ret )
        fprintf( outfile, "\tcall " PREFIX "CallTo16Word\n" );
    else
        fprintf( outfile, "\tcall " PREFIX "CallTo16Long\n" );

    /* Return to caller (using STDCALL calling convention) */
    if ( strlen( args ) > 0 )
        fprintf( outfile, "\tret $%d\n", strlen( args ) * 4 );
    else
    {
        fprintf( outfile, "\tret\n" );

        /* Note: the arg transfer routine must start exactly three bytes
                 after the return stub, hence the nop's */
        fprintf( outfile, "\tnop\n" );
        fprintf( outfile, "\tnop\n" );
    }

    /* 
     * The core routine will call here with registers set up as follows:
     *
     *     SS:SP points to the 16-bit stack
     *     SS:BP points to the bp member of last STACK16FRAME
     *     EDX   points to the current STACK32FRAME
     *     ECX   contains the 16:16 return address
     *     FS    contains the last 16-bit %fs value
     */

    /* Transfer the arguments */

    pos = STACK32OFFSET(args) + 4;  /* first arg is target address */
    while (*args)
    {
        switch (*args++)
        {
        case 'w': /* word */
            fprintf( outfile, "\tpushw %d(%%edx)\n", pos );
            break;
        case 'l': /* long */
            fprintf( outfile, "\tpushl %d(%%edx)\n", pos );
            break;
        default:
            fprintf( stderr, "Unexpected case '%c' in BuildCallTo16Func\n",
                             args[-1] );
        }
        pos += 4;
    }

    /* Push the return address */

    fprintf( outfile, "\tpushl %%ecx\n" );

    /* Push the called routine address */

    fprintf( outfile, "\tpushl %d(%%edx)\n", STACK32OFFSET(args) );

    /* Set %ds and %es (and %ax just in case) equal to %ss */

    fprintf( outfile, "\tmovw %%ss,%%ax\n" );
    fprintf( outfile, "\tmovw %%ax,%%ds\n" );
    fprintf( outfile, "\tmovw %%ax,%%es\n" );

    /* Jump to the called routine */

    fprintf( outfile, "\t.byte 0x66\n" );
    fprintf( outfile, "\tlret\n" );
}



/*******************************************************************
 *         BuildCallFrom16Core
 *
 * This routine builds the core routines used in 16->32 thunks:
 * CallFrom16, CallFrom16Register, and CallFrom16Thunk.
 *
 * CallFrom16 and CallFrom16Register are used by the 16->32 glue code
 * as described above.  CallFrom16Thunk is a special variant used by
 * the implementation of the Win95 16->32 thunk functions C16ThkSL and 
 * C16ThkSL01 and is implemented as follows:
 * On entry, the EBX register is set up to contain a flat pointer to the
 * 16-bit stack such that EBX+22 points to the first argument.
 * Then, the entry point is called, while EBP is set up to point
 * to the return address (on the 32-bit stack).
 * The called function returns with CX set to the number of bytes
 * to be popped of the caller's stack.
 *
 * Stack layout upon entry to the core routine (STACK16FRAME):
 *  ...           ...
 * (sp+22) word   first 16-bit arg
 * (sp+20) word   cs
 * (sp+18) word   ip
 * (sp+16) word   bp
 * (sp+12) long   32-bit entry point (reused for Win16 mutex recursion count)
 * (sp+8)  long   cs of 16-bit entry point
 * (sp+4)  long   ip of 16-bit entry point
 * (sp)    long   auxillary relay function address
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
static void BuildCallFrom16Core( FILE *outfile, int reg_func, int thunk )
{
    char *name = thunk? "Thunk" : reg_func? "Register" : "";

    /* Function header */
    fprintf( outfile, "\n\t.align 4\n" );
#ifdef USE_STABS
    fprintf( outfile, ".stabs \"CallFrom16%s:F1\",36,0,0," PREFIX "CallFrom16%s\n", 
	     name, name);
#endif
    fprintf( outfile, "\t.globl " PREFIX "CallFrom16%s\n", name );
    fprintf( outfile, PREFIX "CallFrom16%s:\n", name );

    /* No relay function for 'thunk' */
    if ( thunk )
        fprintf( outfile, "\tpushl $0\n" );

    /* Create STACK16FRAME (except STACK32FRAME link) */
    fprintf( outfile, "\tpushw %%gs\n" );
    fprintf( outfile, "\tpushw %%fs\n" );
    fprintf( outfile, "\tpushw %%es\n" );
    fprintf( outfile, "\tpushw %%ds\n" );
    fprintf( outfile, "\tpushl %%ebp\n" );
    fprintf( outfile, "\tpushl %%ecx\n" );
    fprintf( outfile, "\tpushl %%edx\n" );

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
    fprintf( outfile, "\tleal -4(%%ebp,%%edx), %%edx\n" );  
                           /* -4 since STACK16FRAME not yet complete! */

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
       ECX: points to ldt_copy
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
        fprintf( outfile, "\taddl $18, %%esp\n" );

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

        fprintf( outfile, "\tpushfl\n" );
        fprintf( outfile, "\tpopl %d(%%esp)\n", CONTEXTOFFSET(EFlags) );  

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

        fprintf( outfile, "\tpushl %%ecx\n" );
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
        fprintf( outfile, "\tpopl %%ecx\n" );

        if ( UsePIC )
            fprintf( outfile, "\tpopl %%ebx\n" );
    }

    /* Call *Thunk* relay routine (which will call the API entry point) */
    fprintf( outfile, "\tcall *%d(%%edx)\n",  STACK16OFFSET(relay) );

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

        /* Restore all registers from CONTEXT */
        fprintf( outfile, "\tmovw %d(%%ebx), %%ss\n", CONTEXTOFFSET(SegSs) );
        fprintf( outfile, "\tmovl %d(%%ebx), %%esp\n", CONTEXTOFFSET(Esp) );
        fprintf( outfile, "\tleal 4(%%esp, %%eax), %%esp\n" );

        fprintf( outfile, "\tpushw %d(%%ebx)\n", CONTEXTOFFSET(SegCs) );
        fprintf( outfile, "\tpushw %d(%%ebx)\n", CONTEXTOFFSET(Eip) );
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
        fprintf( outfile, "\t.byte 0x66\n" );
        fprintf( outfile, "\tlret\n" );
    }
    else
    {
        /* Switch stack back */
        /* fprintf( outfile, "\t.byte 0x64\n\tlssw (%d), %%sp\n", STACKOFFSET ); */
        fprintf( outfile, "\t.byte 0x64,0x66,0x0f,0xb2,0x25\n\t.long %d\n", STACKOFFSET );
        fprintf( outfile, "\t.byte 0x64\n\tpopl (%d)\n", STACKOFFSET );

        /* Set flags according to return value */
        fprintf( outfile, "\torl %%eax, %%eax\n" );

        /* Restore registers and return to *ThunkRet* routine */
        fprintf( outfile, "\tpopl %%edx\n" );
        fprintf( outfile, "\tpopl %%ecx\n" );
        fprintf( outfile, "\tpopl %%ebp\n" );
        fprintf( outfile, "\tpopw %%ds\n" );
        fprintf( outfile, "\tpopw %%es\n" );
        fprintf( outfile, "\tpopw %%fs\n" );
        fprintf( outfile, "\tpopw %%gs\n" );
        fprintf( outfile, "\tret $14\n" );
    }
}
  

/*******************************************************************
 *         BuildCallTo16Core
 *
 * This routine builds the core routines used in 32->16 thunks:
 * CallTo16Word, CallTo16Long, CallTo16RegisterShort, and 
 * CallTo16RegisterLong.
 *
 * CallTo16Word and CallTo16Long are used by the 32->16 glue code
 * as described above.  The register functions can be called directly:
 *
 *   extern void CALLBACK CallTo16RegisterShort( const CONTEXT86 *context, int nb_args );
 *   extern void CALLBACK CallTo16RegisterLong ( const CONTEXT86 *context, int nb_args );
 *
 * They call to 16-bit code with all registers except SS:SP set up as specified 
 * by the 'context' structure, and SS:SP set to point to the current 16-bit
 * stack, decremented by the value specified in the 'nb_args' argument.
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

    /* No relay stub for 'register' functions */
    if ( reg_func )
        fprintf( outfile, "\tpushl $0\n" );

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

    /* Move relay target address to %edi */
    if ( !reg_func )
    {
        fprintf( outfile, "\tmovl 4(%%ebp), %%edi\n" );
        fprintf( outfile, "\taddl $3, %%edi\n" );
    }

    /* Enter Win16 Mutex */
    if ( UsePIC )
        fprintf( outfile, "\tcall " PREFIX "SYSLEVEL_EnterWin16Lock@PLT\n" );
    else
        fprintf( outfile, "\tcall " PREFIX "SYSLEVEL_EnterWin16Lock\n" );

    /* Print debugging info */
    if (debugging)
    {
        /* Push number of arguments (from relay stub) */
        if ( reg_func )
            fprintf( outfile, "\tpushl $-1\n" );
        else
            fprintf( outfile, "\tpushl -12(%%edi)\n" );

        /* Push the address of the first argument */
        fprintf( outfile, "\tleal 12(%%ebp),%%eax\n" );
        fprintf( outfile, "\tpushl %%eax\n" );

        if ( UsePIC )
            fprintf( outfile, "\tcall " PREFIX "RELAY_DebugCallTo16@PLT\n" );
        else
            fprintf( outfile, "\tcall " PREFIX "RELAY_DebugCallTo16\n" );

        fprintf( outfile, "\tpopl %%eax\n" );
        fprintf( outfile, "\tpopl %%eax\n" );
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

    if ( !reg_func )
        fprintf( outfile, "\tret\n" );   /* return to relay return stub */
    else
    {
        fprintf( outfile, "\taddl $4, %%esp\n" );
        fprintf( outfile, "\tret $8\n" );
    }


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

    if (reg_func)
    {
        /* Add the specified offset to the new sp */
        fprintf( outfile, "\tsubw %d(%%edx), %%sp\n", STACK32OFFSET(args)+4 );

        /* Push the return address 
	 * With sreg suffix, we push 16:16 address (normal lret)
	 * With lreg suffix, we push 16:32 address (0x66 lret, for KERNEL32_45)
	 */
	if (reg_func == 1)
	    fprintf( outfile, "\tpushl %%ecx\n" );
	else 
	{
            fprintf( outfile, "\tshldl $16, %%ecx, %%eax\n" );
	    fprintf( outfile, "\tpushw $0\n" );
	    fprintf( outfile, "\tpushw %%ax\n" );
	    fprintf( outfile, "\tpushw $0\n" );
	    fprintf( outfile, "\tpushw %%cx\n" );
	}

        /* Push the called routine address */
        fprintf( outfile, "\tmovl %d(%%edx),%%edx\n", STACK32OFFSET(args) );
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

        /* Jump to the called routine */
        fprintf( outfile, "\t.byte 0x66\n" );
        fprintf( outfile, "\tlret\n" );
    }
    else  /* not a register function */
    {
        /* Make %bp point to the previous stackframe (built by CallFrom16) */
        fprintf( outfile, "\tmovzwl %%sp,%%ebp\n" );
        fprintf( outfile, "\tleal %d(%%ebp),%%ebp\n", STACK16OFFSET(bp) );

        /* Set %fs to the value saved by the last CallFrom16 */
        fprintf( outfile, "\tmovw %d(%%ebp),%%ax\n", STACK16OFFSET(fs)-STACK16OFFSET(bp) );
        fprintf( outfile, "\tmovw %%ax,%%fs\n" );

        /* Jump to the relay code */
        fprintf( outfile, "\tjmp *%%edi\n" );
    }
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
 *         BuildSpec
 *
 * Build the spec files
 */
static int BuildSpec( FILE *outfile, int argc, char *argv[] )
{
    int i;
    for (i = 2; i < argc; i++)
        if (BuildSpecFile( outfile, argv[i] ) < 0) return -1;
    return 0;
}

/*******************************************************************
 *         BuildGlue
 *
 * Build the 16-bit-to-Wine/Wine-to-16-bit callback glue code
 */
static int BuildGlue( FILE *outfile, char * outname, int argc, char *argv[] )
{
    char buffer[1024];
    FILE *infile;

    if (argc > 2)
    {
        infile = fopen( argv[2], "r" );
        if (!infile)
        {
            perror( argv[2] );
            exit( 1 );
        }
    }
    else infile = stdin;

    /* File header */

    fprintf( outfile, "/* File generated automatically. Do not edit! */\n\n" );
    fprintf( outfile, "\t.text\n" );

#ifdef __i386__

#ifdef USE_STABS
    fprintf( outfile, "\t.file\t\"%s\"\n", outname );
    getcwd(buffer, sizeof(buffer));

    /*
     * The stabs help the internal debugger as they are an indication that it
     * is sensible to step into a thunk/trampoline.
     */
    fprintf( outfile, ".stabs \"%s/\",100,0,0,Code_Start\n", buffer);
    fprintf( outfile, ".stabs \"%s\",100,0,0,Code_Start\n", outname);
    fprintf( outfile, "\t.text\n" );
    fprintf( outfile, "\t.align 4\n" );
    fprintf( outfile, "Code_Start:\n\n" );
#endif

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
            BuildCallFrom16Func( outfile, profile, q );
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


#ifdef USE_STABS
    fprintf( outfile, "\t.text\n");
    fprintf( outfile, "\t.stabs \"\",100,0,0,.Letext\n");
    fprintf( outfile, ".Letext:\n");
#endif

#else  /* __i386__ */

    /* Just to avoid an empty file */
    fprintf( outfile, "\t.long 0\n" );

#endif  /* __i386__ */

    fclose( infile );
    return 0;
}

/*******************************************************************
 *         BuildCall16
 *
 * Build the 16-bit callbacks
 */
static int BuildCall16( FILE *outfile, char * outname )
{
#ifdef USE_STABS
    char buffer[1024];
#endif

    /* File header */

    fprintf( outfile, "/* File generated automatically. Do not edit! */\n\n" );
    fprintf( outfile, "\t.text\n" );

#ifdef __i386__

#ifdef USE_STABS
    fprintf( outfile, "\t.file\t\"%s\"\n", outname );
    getcwd(buffer, sizeof(buffer));

    /*
     * The stabs help the internal debugger as they are an indication that it
     * is sensible to step into a thunk/trampoline.
     */
    fprintf( outfile, ".stabs \"%s/\",100,0,0,Code_Start\n", buffer);
    fprintf( outfile, ".stabs \"%s\",100,0,0,Code_Start\n", outname);
    fprintf( outfile, "\t.text\n" );
    fprintf( outfile, "\t.align 4\n" );
    fprintf( outfile, "Code_Start:\n\n" );
#endif
    fprintf( outfile, PREFIX"Call16_Start:\n" );
    fprintf( outfile, "\t.globl "PREFIX"Call16_Start\n" );
    fprintf( outfile, "\t.byte 0\n\n" );


    /* Standard CallFrom16 routine */
    BuildCallFrom16Core( outfile, FALSE, FALSE );

    /* Register CallFrom16 routine */
    BuildCallFrom16Core( outfile, TRUE, FALSE );

    /* C16ThkSL CallFrom16 routine */
    BuildCallFrom16Core( outfile, FALSE, TRUE );

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

    return 0;
}

/*******************************************************************
 *         BuildCall32
 *
 * Build the 32-bit callbacks
 */
static int BuildCall32( FILE *outfile, char * outname )
{
#ifdef USE_STABS
    char buffer[1024];
#endif

    /* File header */

    fprintf( outfile, "/* File generated automatically. Do not edit! */\n\n" );
    fprintf( outfile, "\t.text\n" );

#ifdef __i386__

#ifdef USE_STABS
    fprintf( outfile, "\t.file\t\"%s\"\n", outname );
    getcwd(buffer, sizeof(buffer));

    /*
     * The stabs help the internal debugger as they are an indication that it
     * is sensible to step into a thunk/trampoline.
     */
    fprintf( outfile, ".stabs \"%s/\",100,0,0,Code_Start\n", buffer);
    fprintf( outfile, ".stabs \"%s\",100,0,0,Code_Start\n", outname);
    fprintf( outfile, "\t.text\n" );
    fprintf( outfile, "\t.align 4\n" );
    fprintf( outfile, "Code_Start:\n" );
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
    return 0;
}


/*******************************************************************
 *         usage
 */
static void usage(void)
{
    fprintf( stderr,
             "usage: build [-pic] [-o outfile] -spec SPECNAMES\n"
             "       build [-pic] [-o outfile] -glue SOURCE_FILE\n"
             "       build [-pic] [-o outfile] -call16\n"
             "       build [-pic] [-o outfile] -call32\n" );
    exit(1);
}


/*******************************************************************
 *         main
 */
int main(int argc, char **argv)
{
    char *outname = NULL;
    FILE *outfile = stdout;
    int res = -1;

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
        outname = argv[2];
        argv += 2;
        argc -= 2;
        if (argc < 2) usage();
        if (!(outfile = fopen( outname, "w" )))
        {
            fprintf( stderr, "Unable to create output file '%s'\n", outname );
            exit(1);
        }
    }

    /* Retrieve the selector values; this assumes that we are building
     * the asm files on the platform that will also run them. Probably
     * a safe assumption to make.
     */
    GET_CS( Code_Selector );
    GET_DS( Data_Selector );

    if (!strcmp( argv[1], "-spec" ))
        res = BuildSpec( outfile, argc, argv );
    else if (!strcmp( argv[1], "-glue" ))
        res = BuildGlue( outfile, outname, argc, argv );
    else if (!strcmp( argv[1], "-call16" ))
        res = BuildCall16( outfile, outname );
    else if (!strcmp( argv[1], "-call32" ))
        res = BuildCall32( outfile, outname );
    else
    {
        fclose( outfile );
        unlink( outname );
        usage();
    }

    fclose( outfile );
    if (res < 0)
    {
        unlink( outname );
        return 1;
    }
    return 0;
}
