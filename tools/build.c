/*
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 * Copyright 1995 Martin von Loewis
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "wine.h"
#include "module.h"
#include "neexe.h"

/* ELF symbols do not have an underscore in front */
#ifdef __ELF__
#define PREFIX
#else
#define PREFIX "_"
#endif

#define TYPE_INVALID     0
#define TYPE_BYTE        1
#define TYPE_WORD        2
#define TYPE_LONG        3
#define TYPE_PASCAL_16   4
#define TYPE_PASCAL      5
#define TYPE_REGISTER    6
#define TYPE_ABS         7
#define TYPE_RETURN      8
#define TYPE_STUB        9
#define TYPE_STDCALL    10

#define MAX_ORDINALS	1299

  /* Callback function used for stub functions */
#define STUB_CALLBACK  "RELAY_Unimplemented"

typedef struct ordinal_definition_s
{
    int type;
    int offset;
    char export_name[80];
    void *additional_data;
} ORDDEF;

typedef struct ordinal_variable_definition_s
{
    int n_values;
    int *values;
} ORDVARDEF;

typedef struct ordinal_function_definition_s
{
    int  n_args;
    char arg_types[32];
    char internal_name[80];
} ORDFUNCDEF;

typedef struct ordinal_return_definition_s
{
    int arg_size;
    int ret_value;
} ORDRETDEF;

static ORDDEF OrdinalDefinitions[MAX_ORDINALS];

char LowerDLLName[80];
char UpperDLLName[80];
int Limit = 0;
int DLLId;
int Base = 0;
FILE *SpecFp;

char *ParseBuffer = NULL;
char *ParseNext;
char ParseSaveChar;
int Line;

static int debugging = 1;

  /* Offset of register relative to the end of the context struct */
#define CONTEXTOFFSET(reg) \
   ((int)&(((struct sigcontext_struct *)1)->reg) - 1 \
    - sizeof(struct sigcontext_struct))

static int IsNumberString(char *s)
{
    while (*s != '\0')
	if (!isdigit(*s++))
	    return 0;

    return 1;
}

static char *strlower(char *s)
{
    char *p;
    
    for(p = s; *p != '\0'; p++)
	*p = tolower(*p);

    return s;
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
	ParseBuffer = malloc(512);
	ParseNext = ParseBuffer;
	Line++;
	while (1)
	{
	    if (fgets(ParseBuffer, 511, SpecFp) == NULL)
		return NULL;
	    if (ParseBuffer[0] != '#')
		break;
	}
    }

    while ((token = GetTokenInLine()) == NULL)
    {
	ParseNext = ParseBuffer;
	Line++;
	while (1)
	{
	    if (fgets(ParseBuffer, 511, SpecFp) == NULL)
		return NULL;
	    if (ParseBuffer[0] != '#')
		break;
	}
    }

    return token;
}

static int ParseVariable(int ordinal, int type)
{
    ORDDEF *odp;
    ORDVARDEF *vdp;
    char export_name[80];
    char *token;
    char *endptr;
    int *value_array;
    int n_values;
    int value_array_size;
    
    strcpy(export_name, GetToken());

    token = GetToken();
    if (*token != '(')
    {
	fprintf(stderr, "%d: Expected '(' got '%s'\n", Line, token);
	exit(1);
    }

    n_values = 0;
    value_array_size = 25;
    value_array = malloc(sizeof(*value_array) * value_array_size);
    
    while ((token = GetToken()) != NULL)
    {
	if (*token == ')')
	    break;

	value_array[n_values++] = strtol(token, &endptr, 0);
	if (n_values == value_array_size)
	{
	    value_array_size += 25;
	    value_array = realloc(value_array, 
				  sizeof(*value_array) * value_array_size);
	}
	
	if (endptr == NULL || *endptr != '\0')
	{
	    fprintf(stderr, "%d: Expected number value, got '%s'\n", Line,
		    token);
	    exit(1);
	}
    }
    
    if (token == NULL)
    {
	fprintf(stderr, "%d: End of file in variable declaration\n", Line);
	exit(1);
    }

    if (ordinal >= MAX_ORDINALS)
    {
	fprintf(stderr, "%d: Ordinal number too large\n", Line);
	exit(1);
    }
    
    odp = &OrdinalDefinitions[ordinal];
    odp->type = type;
    strcpy(odp->export_name, export_name);

    vdp = malloc(sizeof(*vdp));
    odp->additional_data = vdp;
    
    vdp->n_values = n_values;
    vdp->values = realloc(value_array, sizeof(*value_array) * n_values);

    return 0;
}

static int ParseExportFunction(int ordinal, int type)
{
    char *token;
    ORDDEF *odp;
    ORDFUNCDEF *fdp;
    int i;
    
    odp = &OrdinalDefinitions[ordinal];
    strcpy(odp->export_name, GetToken());
    odp->type = type;
    fdp = malloc(sizeof(*fdp));
    odp->additional_data = fdp;

    token = GetToken();
    if (*token != '(')
    {
	fprintf(stderr, "%d: Expected '(' got '%s'\n", Line, token);
	exit(1);
    }

    for (i = 0; i < 16; i++)
    {
	token = GetToken();
	if (*token == ')')
	    break;

	if (!strcmp(token, "byte") || !strcmp(token, "word"))
            fdp->arg_types[i] = 'w';
	else if (!strcmp(token, "s_byte") || !strcmp(token, "s_word"))
            fdp->arg_types[i] = 's';
	else if (!strcmp(token, "long") || !strcmp(token, "segptr"))
            fdp->arg_types[i] = 'l';
	else if (!strcmp(token, "ptr"))
            fdp->arg_types[i] = 'p';
	else
	{
	    fprintf(stderr, "%d: Unknown variable type '%s'\n", Line, token);
	    exit(1);
	}
    }
    fdp->arg_types[i] = '\0';

    if ((type == TYPE_REGISTER) && (i > 0))
    {
        fprintf( stderr, "%d: Register function can't have arguments\n", Line);
        exit(1);
    }

    strcpy(fdp->internal_name, GetToken());
    return 0;
}

static int ParseEquate(int ordinal)
{
    ORDDEF *odp;
    char *token;
    char *endptr;
    int value;
    
    odp = &OrdinalDefinitions[ordinal];
    strcpy(odp->export_name, GetToken());

    token = GetToken();
    value = strtol(token, &endptr, 0);
    if (endptr == NULL || *endptr != '\0')
    {
	fprintf(stderr, "%d: Expected number value, got '%s'\n", Line,
		token);
	exit(1);
    }

    odp->type = TYPE_ABS;
    odp->additional_data = (void *) value;

    return 0;
}

static int ParseReturn(int ordinal)
{
    ORDDEF *odp;
    ORDRETDEF *rdp;
    char *token;
    char *endptr;
    
    rdp = malloc(sizeof(*rdp));
    
    odp = &OrdinalDefinitions[ordinal];
    strcpy(odp->export_name, GetToken());
    odp->type = TYPE_RETURN;
    odp->additional_data = rdp;

    token = GetToken();
    rdp->arg_size = strtol(token, &endptr, 0);
    if (endptr == NULL || *endptr != '\0')
    {
	fprintf(stderr, "%d: Expected number value, got '%s'\n", Line,
		token);
	exit(1);
    }

    token = GetToken();
    rdp->ret_value = strtol(token, &endptr, 0);
    if (endptr == NULL || *endptr != '\0')
    {
	fprintf(stderr, "%d: Expected number value, got '%s'\n", Line,
		token);
	exit(1);
    }

    return 0;
}


static int ParseStub( int ordinal )
{
    ORDDEF *odp;
    ORDFUNCDEF *fdp;
    
    odp = &OrdinalDefinitions[ordinal];
    strcpy( odp->export_name, GetToken() );
    odp->type = TYPE_STUB;
    fdp = malloc(sizeof(*fdp));
    odp->additional_data = fdp;
    fdp->arg_types[0] = '\0';
    strcpy( fdp->internal_name, STUB_CALLBACK );
    return 0;
}


static int ParseOrdinal(int ordinal)
{
    char *token;
    
    if (ordinal >= MAX_ORDINALS)
    {
	fprintf(stderr, "%d: Ordinal number too large\n", Line);
	exit(1);
    }
    if (ordinal > Limit) Limit = ordinal;

    token = GetToken();
    if (token == NULL)
    {
	fprintf(stderr, "%d: Expected type after ordinal\n", Line);
	exit(1);
    }

    if (strcmp(token, "byte") == 0)
	return ParseVariable(ordinal, TYPE_BYTE);
    else if (strcmp(token, "word") == 0)
	return ParseVariable(ordinal, TYPE_WORD);
    else if (strcmp(token, "long") == 0)
	return ParseVariable(ordinal, TYPE_LONG);
    else if (strcmp(token, "p") == 0)
	return ParseExportFunction(ordinal, TYPE_PASCAL);
    else if (strcmp(token, "pascal") == 0)
	return ParseExportFunction(ordinal, TYPE_PASCAL);
    else if (strcmp(token, "pascal16") == 0)
	return ParseExportFunction(ordinal, TYPE_PASCAL_16);
    else if (strcmp(token, "register") == 0)
        return ParseExportFunction(ordinal, TYPE_REGISTER);
    else if (strcmp(token, "stdcall") == 0)
        return ParseExportFunction(ordinal, TYPE_STDCALL);
    else if (strcmp(token, "equate") == 0)
	return ParseEquate(ordinal);
    else if (strcmp(token, "return") == 0)
	return ParseReturn(ordinal);
    else if (strcmp(token, "stub") == 0)
	return ParseStub(ordinal);
    else
    {
	fprintf(stderr, 
		"%d: Expected type after ordinal, found '%s' instead\n",
		Line, token);
	exit(1);
    }
}

static int ParseTopLevel(void)
{
    char *token;
    
    while ((token = GetToken()) != NULL)
    {
	if (strcmp(token, "name") == 0)
	{
	    strcpy(LowerDLLName, GetToken());
	    strlower(LowerDLLName);

	    strcpy(UpperDLLName, LowerDLLName);
	    strupper(UpperDLLName);
	}
	else if (strcmp(token, "id") == 0)
	{
	    token = GetToken();
	    if (!IsNumberString(token))
	    {
		fprintf(stderr, "%d: Expected number after id\n", Line);
		exit(1);
	    }
	    
	    DLLId = atoi(token);
	}
	else if (strcmp(token, "base") == 0)
	{
		token = GetToken();
		if (!IsNumberString(token))
		{
		fprintf(stderr, "%d: Expected number after base\n", Line);
		exit(1);
		}

		Base = atoi(token);
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
		    "%d: Expected name, id, length or ordinal\n", Line);
	    exit(1);
	}
    }

    return 0;
}


static int OutputVariableCode( char *storage, ORDDEF *odp )
{
    ORDVARDEF *vdp;
    int i;

    vdp = odp->additional_data;
    printf( "\t.data\n" );
    for (i = 0; i < vdp->n_values; i++)
    {
	if ((i & 7) == 0)
	    printf( "\t%s\t", storage);
	    
	printf( "%d", vdp->values[i]);
	
	if ((i & 7) == 7 || i == vdp->n_values - 1) printf( "\n");
	else printf( ", ");
    }
    printf( "\n");
    printf( "\t.text\n" );
    return vdp->n_values;
}


/*******************************************************************
 *         BuildModule
 *
 * Build the in-memory representation of the module, and dump it
 * as a byte stream into the assembly code.
 */
static void BuildModule( int max_code_offset, int max_data_offset )
{
    ORDDEF *odp;
    int i, size;
    char *buffer;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegment;
    LOADEDFILEINFO *pFileInfo;
    BYTE *pstr, *bundle;
    WORD *pword;

    /*   Module layout:
     * NE_MODULE       Module
     * LOADEDFILEINFO  File information
     * SEGTABLEENTRY   Segment 1 (code)
     * SEGTABLEENTRY   Segment 2 (data)
     * WORD[2]         Resource table (empty)
     * BYTE[2]         Imported names (empty)
     * BYTE[n]         Resident names table
     * BYTE[n]         Entry table
     */

    buffer = malloc( 0x10000 );

    pModule = (NE_MODULE *)buffer;
    pModule->magic = NE_SIGNATURE;
    pModule->count = 1;
    pModule->next = 0;
    pModule->flags = NE_FFLAGS_SINGLEDATA | NE_FFLAGS_LIBMODULE;
    pModule->dgroup = 2;
    pModule->heap_size = 0xffff;
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

      /* File information */

    pFileInfo = (LOADEDFILEINFO *)(pModule + 1);
    pModule->fileinfo = (int)pFileInfo - (int)pModule;
    pFileInfo->length = sizeof(LOADEDFILEINFO) + strlen(UpperDLLName) + 3;
    pFileInfo->fixed_media = 0;
    pFileInfo->error = 0;
    pFileInfo->date = 0;
    pFileInfo->time = 0;
    sprintf( pFileInfo->filename, "%s.DLL", UpperDLLName );
    pstr = (char *)pFileInfo + pFileInfo->length + 1;
        
      /* Segment table */

    pSegment = (SEGTABLEENTRY *)pstr;
    pModule->seg_table = (int)pSegment - (int)pModule;
    pSegment->filepos = 0;
    pSegment->size = max_code_offset;
    pSegment->flags = 0;
    pSegment->minsize = max_code_offset;
    pSegment->selector = 0;
    pSegment++;

    pModule->dgroup_entry = (int)pSegment - (int)pModule;
    pSegment->filepos = 0;
    pSegment->size = max_data_offset;
    pSegment->flags = NE_SEGFLAGS_DATA;
    pSegment->minsize = max_data_offset;
    pSegment->selector = 0;
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
    *pstr = strlen(UpperDLLName );
    strcpy( pstr + 1, UpperDLLName );
    pstr += *pstr + 1;
    *(WORD *)pstr = 0;
    pstr += sizeof(WORD);
    /* Store all ordinals */
    odp = OrdinalDefinitions + 1;
    for (i = 1; i <= Limit; i++, odp++)
    {
        if (!odp->export_name[0]) continue;
        *pstr = strlen( odp->export_name );
        strcpy( pstr + 1, odp->export_name );
        strupper( pstr + 1 );
        pstr += *pstr + 1;
        *(WORD *)pstr = i;
        pstr += sizeof(WORD);
    }
    *pstr++ = 0;

      /* Entry table */

    pModule->entry_table = (int)pstr - (int)pModule;
    bundle = NULL;
    odp = OrdinalDefinitions + 1;
    for (i = 1; i <= Limit; i++, odp++)
    {
        int selector = 0;

	switch (odp->type)
	{
        case TYPE_INVALID:
            selector = 0;  /* Invalid selector */
            break;

        case TYPE_PASCAL:
        case TYPE_PASCAL_16:
        case TYPE_REGISTER:
        case TYPE_RETURN:
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
        }

          /* create a new bundle if necessary */
        if (!bundle || (bundle[0] >= 254) || (bundle[1] != selector))
        {
            bundle = pstr;
            bundle[0] = 0;
            bundle[1] = selector;
            pstr += 2;
        }

        (*bundle)++;
        if (selector != 0)
        {
            *pstr++ = 1;
            *(WORD *)pstr = odp->offset;
            pstr += sizeof(WORD);
        }
    }
    *pstr++ = 0;

      /* Dump the module content */

    printf( "\t.data\n" );
    printf( "\t.globl " PREFIX "%s_Module_Start\n", UpperDLLName );
    printf( PREFIX "%s_Module_Start:\n", UpperDLLName );
    size = (int)pstr - (int)pModule;
    for (i = 0, pstr = buffer; i < size; i++, pstr++)
    {
        if (!(i & 7)) printf( "\t.byte " );
        printf( "%d%c", *pstr, ((i & 7) != 7) ? ',' : '\n' );
    }
    if (i & 7) printf( "0\n" );
    printf( "\t.globl " PREFIX "%s_Module_End\n", UpperDLLName );
    printf( PREFIX "%s_Module_End:\n", UpperDLLName );
}


static void BuildSpec32Files( char *specname )
{
    ORDDEF *odp;
    ORDFUNCDEF *fdp;
    ORDRETDEF *rdp;
    int i;

    SpecFp = fopen( specname, "r");
    if (SpecFp == NULL)
    {
	fprintf(stderr, "Could not open specification file, '%s'\n", specname);
	exit(1);
    }

    ParseTopLevel();

    printf( "/* File generated automatically, do not edit! */\n" );
    printf( "#include <sys/types.h>\n");
    printf( "#include \"dlls.h\"\n");
    printf( "#include \"pe_image.h\"\n");
    printf( "#include \"winerror.h\"\n");
    printf( "#include \"stddebug.h\"\n");
    printf( "#include \"debug.h\"\n");
    printf( "\nextern int RELAY32_Unimplemented();\n\n" );

    odp = OrdinalDefinitions;
    for (i = 0; i <= Limit; i++, odp++)
    {
        int argno,argc;
        fdp = odp->additional_data;
        rdp = odp->additional_data;

        switch (odp->type)
        {
        case TYPE_INVALID:
        case TYPE_STUB:
            printf( "int %s_%d()\n{\n\t", UpperDLLName, i);
            printf( "RELAY32_Unimplemented(\"%s\",%d);\n", UpperDLLName, i);
            printf( "\t/*NOTREACHED*/\n\treturn 0;\n}\n\n");
            break;
        case TYPE_STDCALL:
            argc=strlen(fdp->arg_types);
            printf( "void %s_%d(", UpperDLLName, i);
            for(argno=0;argno<argc;argno++)
            {
                switch(fdp->arg_types[argno])
                {
                case 'p': printf( "void *");break;
                case 'l': printf( "int ");break;
                default:
                    fprintf(stderr, "Not supported argument type %c\n",
                            fdp->arg_types[argno]);
                    exit(1);
                }
                putchar( 'a'+argno );
                if (argno!=argc-1) putchar( ',' );
            }
            printf( ")\n{\n" );
            printf( "\textern int %s();\n", fdp->internal_name );
            printf( "\tdprintf_relay(stddeb,\"Entering %%s.%%s(");
            for (argno=0;argno<argc;argno++) printf( "%%x ");
            printf( ")\\n\", \"%s\", \"%s\"", UpperDLLName, odp->export_name);
            for(argno=0;argno<argc;argno++) printf( ",%c", 'a'+argno);
            printf( ");\n\t%s(", fdp->internal_name );
            for(argno=0;argno<argc;argno++)
            {
                putchar('a'+argno);
                if (argno!=argc-1) putchar(',');
            }
            printf( ");\n\t__asm__ __volatile__(\"movl %%ebp,%%esp;"
                    "popl %%ebp;ret $%d\");\n}\n\n",
                    4*argc);
            break;
        case TYPE_RETURN:
            printf( "void %s_%d()\n{\n\t", UpperDLLName, i);
            printf( "RELAY32_DebugEnter(\"%s\",\"%s\");\n\t",
                   UpperDLLName, odp->export_name);
            printf( "WIN32_LastError=ERROR_CALL_NOT_IMPLEMENTED;\n");
            printf( "\t__asm__ __volatile__ (\"movl %d,%%eax\");\n", 
                   rdp->ret_value);
            printf( "\t__asm__ __volatile__ (\"movl %%ebp,%%esp;popl %%ebp;"
                    "ret $%d\");\n}\n\n", rdp->arg_size);
            break;
        default:
            fprintf(stderr,"build: function type %d not available for Win32\n",
                    odp->type);
            break;
        }
    }

    printf( "static WIN32_function functions[%d+1]={\n", Limit);

    odp = OrdinalDefinitions;
    for (i = 0; i <= Limit; i++, odp++)
    {
        fdp = odp->additional_data;
        rdp = odp->additional_data;

        switch (odp->type)
        {
        case TYPE_INVALID:
            printf( "{0,%s_%d},\n",UpperDLLName, i);
            break;
        case TYPE_RETURN:
        case TYPE_STDCALL:
        case TYPE_STUB:
            printf( "{\"%s\",%s_%d},\n", odp->export_name, UpperDLLName, i);
            break;
        default:
            fprintf(stderr, "build: implementation error: missing %d\n",
                    odp->type);
            exit(1);
        }
    }
    printf("};\n\n");

    printf( "static WIN32_builtin dll={\"%s\",functions,%d,0};\n",
            UpperDLLName, Limit);

    printf( "void %s_Init(void)\n{\n",UpperDLLName);
    printf( "\tdll.next=WIN32_builtin_list;\n");
    printf( "\tWIN32_builtin_list=&dll;\n}");
}


static void BuildSpec16Files( char *specname )
{
    ORDDEF *odp;
    ORDFUNCDEF *fdp;
    ORDRETDEF *rdp;
    int i;
    int code_offset, data_offset;

    SpecFp = fopen( specname, "r");
    if (SpecFp == NULL)
    {
	fprintf(stderr, "Could not open specification file, '%s'\n", specname);
	exit(1);
    }

    ParseTopLevel();

    printf( "/* File generated automatically; do not edit! */\n" );
    printf( "\t.data\n" );
    printf( "\t.globl " PREFIX "%s_Data_Start\n", UpperDLLName );
    printf( PREFIX "%s_Data_Start:\n", UpperDLLName );
    printf( "\t.word 0,0,0,0,0,0,0,0\n" );
    data_offset = 16;
    printf( "\t.text\n" );
    printf( "\t.globl " PREFIX "%s_Code_Start\n", UpperDLLName );
    printf( PREFIX "%s_Code_Start:\n", UpperDLLName );
    code_offset = 0;

    odp = OrdinalDefinitions;
    for (i = 0; i <= Limit; i++, odp++)
    {
        fdp = odp->additional_data;
        rdp = odp->additional_data;
	    
        switch (odp->type)
        {
          case TYPE_INVALID:
            odp->offset = 0xffff;
            break;

          case TYPE_ABS:
            odp->offset = (int)odp->additional_data & 0xffff;
            break;

          case TYPE_BYTE:
            printf( "/* %s.%d */\n", UpperDLLName, i);
            odp->offset = data_offset;
            data_offset += OutputVariableCode( ".byte", odp);
            break;

          case TYPE_WORD:
            printf( "/* %s.%d */\n", UpperDLLName, i);
            odp->offset = data_offset;
            data_offset += 2 * OutputVariableCode( ".word", odp);
            break;

          case TYPE_LONG:
            printf( "/* %s.%d */\n", UpperDLLName, i);
            odp->offset = data_offset;
            data_offset += 4 * OutputVariableCode( ".long", odp);
            break;

          case TYPE_RETURN:
            printf( "/* %s.%d */\n", UpperDLLName, i);
            printf( "\tmovw $%d,%%ax\n", rdp->ret_value & 0xffff );
            printf( "\tmovw $%d,%%dx\n", (rdp->ret_value >> 16) & 0xffff);
            printf( "\t.byte 0x66\n");
            if (rdp->arg_size != 0)
                printf( "\tlret $%d\n", rdp->arg_size);
            else
                printf( "\tlret\n");
            odp->offset = code_offset;
            code_offset += 10;  /* Assembly code is 10 bytes long */
            if (rdp->arg_size != 0) code_offset += 2;
            break;

          case TYPE_REGISTER:
          case TYPE_PASCAL:
          case TYPE_PASCAL_16:
          case TYPE_STUB:
            printf( "/* %s.%d */\n", UpperDLLName, i);
            printf( "\tpushw %%bp\n" );
            printf( "\tpushl $0x%08x\n", (DLLId << 16) | i);
            printf( "\tpushl $" PREFIX "%s\n", fdp->internal_name );
            printf( "\tljmp $0x%04x, $" PREFIX "CallTo32_%s_%s\n\n",
                    WINE_CODE_SELECTOR,
                    (odp->type == TYPE_REGISTER) ? "regs" :
                    (odp->type == TYPE_PASCAL) ? "long" : "word",
                    fdp->arg_types );
            printf( "\tnop\n" );
            printf( "\tnop\n" );
            printf( "\tnop\n" );
            printf( "\tnop\n" );
            printf( "\tnop\n" );
            odp->offset = code_offset;
            code_offset += 24;  /* Assembly code is 24 bytes long */
            break;
		
          default:
            fprintf( stderr, "build: Unknown function type; please report.\n");
            break;
	}
    }

    if (!code_offset)  /* Make sure the code segment is not empty */
    {
        printf( "\t.byte 0\n" );
        code_offset++;
    }

    BuildModule( code_offset, data_offset );
}


/*******************************************************************
 *         TransferArgs16To32
 *
 * Get the arguments from the 16-bit stack and push them on the 32-bit stack.
 * The 16-bit stack layout is:
 *   ...     ...
 *  (bp+8)    arg2
 *  (bp+6)    arg1
 *  (bp+4)    cs
 *  (bp+2)    ip
 *  (bp)      bp
 */
static int TransferArgs16To32( char *args )
{
    int i, pos16, pos32;

    /* Save ebx first */

    printf( "\tpushl %%ebx\n" );

    /* Get the 32-bit stack pointer */

    printf( "\tmovl " PREFIX "IF1632_Saved32_esp,%%ebx\n" );

    /* Copy the arguments */

    pos16 = 6;  /* skip bp and return address */
    pos32 = 0;

    for (i = strlen(args); i > 0; i--)
    {
        pos32 -= 4;
        switch(args[i-1])
        {
        case 'w':  /* word */
            printf( "\tmovzwl %d(%%ebp),%%eax\n", pos16 );
            printf( "\tmovl %%eax,%d(%%ebx)\n", pos32 );
            pos16 += 2;
            break;

        case 's':  /* s_word */
            printf( "\tmovswl %d(%%ebp),%%eax\n", pos16 );
            printf( "\tmovl %%eax,%d(%%ebx)\n", pos32 );
            pos16 += 2;
            break;

        case 'l':  /* long */
            printf( "\tmovl %d(%%ebp),%%eax\n", pos16 );
            printf( "\tmovl %%eax,%d(%%ebx)\n", pos32 );
            pos16 += 4;
            break;

        case 'p':  /* ptr */
            /* Get the selector */
            printf( "\tmovw %d(%%ebp),%%ax\n", pos16 + 2 );
            /* Get the selector base */
            printf( "\tandl $0xfff8,%%eax\n" );
            printf( "\tmovl " PREFIX "ldt_copy(%%eax),%%eax\n" );
            printf( "\tmovl %%eax,%d(%%ebx)\n", pos32 );
            /* Add the offset */
            printf( "\tmovzwl %d(%%ebp),%%eax\n", pos16 );
            printf( "\taddl %%eax,%d(%%ebx)\n", pos32 );
            pos16 += 4;
            break;

        default:
            fprintf( stderr, "Unknown arg type '%c'\n", args[i-1] );
        }
    }

    /* Restore ebx */
    
    printf( "\tpopl %%ebx\n" );

    return pos16 - 6;  /* Return the size of the 16-bit args */
}


/*******************************************************************
 *         BuildContext
 *
 * Build the context structure on the 32-bit stack.
 * The only valid registers in the context structure are:
 *   eax, ebx, ecx, edx, esi, edi, ds, es, (some of the) flags
 */
static void BuildContext(void)
{
    /* Save ebx first */

    printf( "\tpushl %%ebx\n" );

    /* Get the 32-bit stack pointer */

    printf( "\tmovl " PREFIX "IF1632_Saved32_esp,%%ebx\n" );

    /* Store the registers */

    printf( "\tpopl %d(%%ebx)\n", CONTEXTOFFSET(sc_ebx) ); /* Get ebx from stack */
    printf( "\tmovl %%eax,%d(%%ebx)\n", CONTEXTOFFSET(sc_eax) );
    printf( "\tmovl %%ecx,%d(%%ebx)\n", CONTEXTOFFSET(sc_ecx) );
    printf( "\tmovl %%edx,%d(%%ebx)\n", CONTEXTOFFSET(sc_edx) );
    printf( "\tmovl %%esi,%d(%%ebx)\n", CONTEXTOFFSET(sc_esi) );
    printf( "\tmovl %%edi,%d(%%ebx)\n", CONTEXTOFFSET(sc_edi) );
    printf( "\tpushw %%es\n" );
    printf( "\tpopw %d(%%ebx)\n", CONTEXTOFFSET(sc_es) );
    printf( "\tmovw -10(%%ebp),%%ax\n" );  /* Get saved ds from stack */
    printf( "\tmovw %%ax,%d(%%ebx)\n", CONTEXTOFFSET(sc_ds) );
    printf( "\tpushfl\n" );
#ifndef __FreeBSD__
    printf( "\tpopl %d(%%ebx)\n", CONTEXTOFFSET(sc_eflags) );
#else    
    printf( "\tpopl %d(%%ebx)\n", CONTEXTOFFSET(sc_efl) );
#endif
}


/*******************************************************************
 *         RestoreContext
 *
 * Restore the registers from the context structure
 */
static void RestoreContext(void)
{
    /* Get the 32-bit stack pointer */

    printf( "\tmovl " PREFIX "IF1632_Saved32_esp,%%ebx\n" );

    /* Restore the registers */

    printf( "\tmovl %d(%%ebx),%%ecx\n", CONTEXTOFFSET(sc_ecx) );
    printf( "\tmovl %d(%%ebx),%%edx\n", CONTEXTOFFSET(sc_edx) );
    printf( "\tmovl %d(%%ebx),%%esi\n", CONTEXTOFFSET(sc_esi) );
    printf( "\tmovl %d(%%ebx),%%edi\n", CONTEXTOFFSET(sc_edi) );
    printf( "\tpushw %d(%%ebx)\n", CONTEXTOFFSET(sc_es) );
    printf( "\tpopw %%es\n" );
    printf( "\tpopw %%ax\n" );  /* Remove old ds from the stack */
    printf( "\tpushw %d(%%ebx)\n", CONTEXTOFFSET(sc_ds) ); /* Push new ds */
#ifndef __FreeBSD__
    printf( "\tpushl %d(%%ebx)\n", CONTEXTOFFSET(sc_eflags) );
#else    
    printf( "\tpushl %d(%%ebx)\n", CONTEXTOFFSET(sc_efl) );
#endif
    printf( "\tpopfl\n" );
    printf( "\tmovl %d(%%ebx),%%eax\n", CONTEXTOFFSET(sc_eax) );
    printf( "\tmovl %d(%%ebx),%%ebx\n", CONTEXTOFFSET(sc_ebx) );
}


/*******************************************************************
 *         BuildCall32Func
 *
 * Build a 32-bit callback function. The syntax of the function
 * profile is: type_xxxxx, where 'type' is one of 'regs', 'word' or
 * 'long' and each 'x' is an argument ('w'=word, 's'=signed word,
 * 'l'=long, 'p'=pointer).
 *
 * Stack layout upon entry to the callback function:
 *  ...      ...
 * (sp+14)  first 16-bit arg
 * (sp+12)  cs (word)
 * (sp+10)  ip (word)
 * (sp+8)   bp (word)
 * (sp+4)   dll_id+ordinal (long)
 * (sp)     entrypoint (long)
 *
 */
static void BuildCall32Func( char *profile )
{
    int argsize = 0;
    int short_ret = 0;
    int reg_func = 0;
    char *args = profile + 5;

    /* Parse function type */

    if (!strncmp( "word_", profile, 5 )) short_ret = 1;
    else if (!strncmp( "regs_", profile, 5 )) reg_func = 1;
    else if (strncmp( "long_", profile, 5 ))
    {
        fprintf( stderr, "Invalid function name '%s', ignored\n", profile );
        return;
    }

    /* Function header */

    printf( "/**********\n" );
    printf( " * " PREFIX "CallTo32_%s\n", profile );
    printf( " **********/\n" );
    printf( "\t.align 4\n" );
    printf( "\t.globl " PREFIX "CallTo32_%s\n\n", profile );
    printf( PREFIX "CallTo32_%s:\n", profile );

    /* Setup bp to point to its copy on the stack */

    printf( "\tmovzwl %%sp,%%ebp\n" );
    printf( "\taddw $8,%%bp\n" );

    /* Save 16-bit ds */

    printf( "\tpushw %%ds\n" );

    /* Restore 32-bit ds */

    printf( "\tpushw $0x%04x\n", WINE_DATA_SELECTOR );
    printf( "\tpopw %%ds\n" );

    /* Save the 16-bit stack */

    printf( "\tpushw " PREFIX "IF1632_Saved16_sp\n" );
    printf( "\tpushw " PREFIX "IF1632_Saved16_ss\n" );
    printf( "\tmovw %%ss," PREFIX "IF1632_Saved16_ss\n" );
    printf( "\tmovw %%sp," PREFIX "IF1632_Saved16_sp\n" );

    /* Transfer the arguments */

    if (reg_func) BuildContext();
    else if (*args) argsize = TransferArgs16To32( args );

    /* Get the address of the API function */

    printf( "\tmovl -8(%%ebp),%%eax\n" );

    /* Setup es */

    printf( "\tpushw %%ds\n" );
    printf( "\tpopw %%es\n" );

    /* Switch to the 32-bit stack */

    printf( "\tpushw %%ds\n" );
    printf( "\tpopw %%ss\n" );
    printf( "\tmovl " PREFIX "IF1632_Saved32_esp,%%esp\n" );

    /* Setup %ebp to point to the previous stack frame (built by CallTo16) */

    printf( "\tmovl %%esp,%%ebp\n" );
    printf( "\taddl $24,%%ebp\n" );

    if (reg_func)
        printf( "\tsubl $%d,%%esp\n", sizeof(struct sigcontext_struct) );
    else if (*args)
        printf( "\tsubl $%d,%%esp\n", 4 * strlen(args) );

    /* Call the entry point */

    if (debugging)
    {
        printf( "\tpushl %%eax\n" );
        printf( "\tpushl $CALL32_Str_%s\n", profile );
        printf( "\tcall " PREFIX "RELAY_DebugCall32\n" );
        printf( "\tpopl %%eax\n" );
        printf( "\tpopl %%eax\n" );
    }

    printf( "\tcall %%eax\n" );

    if (debugging)
    {
        printf( "\tpushl %%eax\n" );
        printf( "\tpushl $%d\n", short_ret );
        printf( "\tcall " PREFIX "RELAY_DebugReturn\n" );
        printf( "\tpopl %%eax\n" );
        printf( "\tpopl %%eax\n" );
    }

    if (reg_func)
        printf( "\taddl $%d,%%esp\n", sizeof(struct sigcontext_struct) );
    else if (*args)
        printf( "\taddl $%d,%%esp\n", 4 * strlen(args) );

    /* Restore the 16-bit stack */

    printf( "\tmovw " PREFIX "IF1632_Saved16_ss,%%ss\n" );
    printf( "\tmovw " PREFIX "IF1632_Saved16_sp,%%sp\n" );
    printf( "\tpopw " PREFIX "IF1632_Saved16_ss\n" );
    printf( "\tpopw " PREFIX "IF1632_Saved16_sp\n" );

    if (reg_func)
    {
        /* Restore registers from the context structure */
        RestoreContext();
    }
    else  /* Store the return value in dx:ax if needed */
    {
        if (!short_ret)
        {
            printf( "\tpushl %%eax\n" );
            printf( "\tpopw %%dx\n" );
            printf( "\tpopw %%dx\n" );
        }
    }

    /* Restore ds and bp */

    printf( "\tpopw %%ds\n" );
    printf( "\tpopl %%ebp\n" );  /* Remove entry point address */
    printf( "\tpopl %%ebp\n" );  /* Remove DLL id and ordinal */
    printf( "\tpopw %%bp\n" );

    /* Remove the arguments and return */

    if (argsize)
    {
        printf( "\t.byte 0x66\n" );
        printf( "\tlret $%d\n", argsize );
    }
    else
    {
        printf( "\t.byte 0x66\n" );
        printf( "\tlret\n" );
    }
}


/*******************************************************************
 *         BuildCall16Func
 *
 * Build a 16-bit callback function.
 *
 * Stack frame of the callback function:
 *  ...      ...
 * (ebp+24) arg2
 * (ebp+20) arg1
 * (ebp+16) 16-bit ds
 * (ebp+12) func to call
 * (ebp+8)  code selector
 * (ebp+4)  return address
 * (ebp)    previous ebp
 *
 * Prototypes for the CallTo16 functions:
 *   extern WORD CallTo16_word_xxx( FARPROC func, WORD ds, args... );
 *   extern LONG CallTo16_long_xxx( FARPROC func, WORD ds, args... );
 *   extern void CallTo16_regs_( FARPROC func, WORD ds, WORD es, WORD bp,
 *                               WORD ax, WORD bx, WORD cx, WORD dx,
 *                               WORD si, WORD di );
 */
static void BuildCall16Func( char *profile )
{
    int short_ret = 0;
    int reg_func = 0;
    char *args = profile + 5;

    if (!strncmp( "word_", profile, 5 )) short_ret = 1;
    else if (!strncmp( "regs_", profile, 5 )) reg_func = short_ret = 1;
    else if (strncmp( "long_", profile, 5 ))
    {
        fprintf( stderr, "Invalid function name '%s', ignored\n", profile );
        return;
    }

    /* Function header */

    printf( "/**********\n" );
    printf( " * " PREFIX "CallTo16_%s\n", profile );
    printf( " **********/\n" );
    printf( "\t.align 4\n" );
    printf( "\t.globl " PREFIX "CallTo16_%s\n\n", profile );
    printf( PREFIX "CallTo16_%s:\n", profile );

    /* Push code selector before return address to simulate a lcall */

    printf( "\tpopl %%eax\n" );
    printf( "\tpushl $0x%04x\n", WINE_CODE_SELECTOR );
    printf( "\tpushl %%eax\n" );

    /* Entry code */

    printf( "\tpushl %%ebp\n" );
    printf( "\tmovl %%esp,%%ebp\n" );

    /* Save the 32-bit registers */

    printf( "\tpushl %%ebx\n" );
    printf( "\tpushl %%ecx\n" );
    printf( "\tpushl %%edx\n" );
    printf( "\tpushl %%esi\n" );
    printf( "\tpushl %%edi\n" );

    /* Save the 32-bit stack */

    printf( "\tpushl " PREFIX "IF1632_Saved32_esp\n" );
    printf( "\tmovl %%esp," PREFIX "IF1632_Saved32_esp\n" );
    printf( "\tmovl %%ebp,%%ebx\n" );

    /* Print debugging info */

    if (debugging)
    {
        /* Push the address of the first argument */
        printf( "\tmovl %%ebx,%%eax\n" );
        printf( "\taddl $12,%%eax\n" );
        printf( "\tpushl $%d\n", reg_func ? 7 : strlen(args) );
        printf( "\tpushl %%eax\n" );
        printf( "\tcall " PREFIX "RELAY_DebugCall16\n" );
        printf( "\tpopl %%eax\n" );
        printf( "\tpopl %%eax\n" );
    }

    /* Switch to the 16-bit stack */

    printf( "\tmovw " PREFIX "IF1632_Saved16_ss,%%ss\n" );
    printf( "\tmovw " PREFIX "IF1632_Saved16_sp,%%sp\n" );

    /* Transfer the arguments */

    if (reg_func)
    {
        /* Get the registers. ebx is handled later on. */
        printf( "\tpushw 20(%%ebx)\n" );
        printf( "\tpopw %%es\n" );
        printf( "\tmovl 24(%%ebx),%%ebp\n" );
        printf( "\tmovl 28(%%ebx),%%eax\n" );
        printf( "\tmovl 36(%%ebx),%%ecx\n" );
        printf( "\tmovl 40(%%ebx),%%edx\n" );
        printf( "\tmovl 44(%%ebx),%%esi\n" );
        printf( "\tmovl 48(%%ebx),%%edi\n" );
    }
    else  /* not a register function */
    {
        int pos = 20;  /* first argument position */

        /* Make %bp point to the previous stackframe (built by CallTo32) */
        printf( "\tmovw %%sp,%%bp\n" );
        printf( "\taddw $16,%%bp\n" );

        while (*args)
        {
            switch(*args++)
            {
            case 'w': /* word */
                printf( "\tpushw %d(%%ebx)\n", pos );
                break;
            case 'l': /* long */
                printf( "\tpushl %d(%%ebx)\n", pos );
                break;
            }
            pos += 4;
        }
    }

    /* Push the return address */

    printf( "\tpushl " PREFIX "CALL16_RetAddr_%s\n",
            short_ret ? "word" : "long" );

    /* Push the called routine address */

    printf( "\tpushl 12(%%ebx)\n" );

    /* Get the 16-bit ds */
    /* FIXME: this shouldn't be necessary if function prologs fixup worked. */

    printf( "\tpushw 16(%%ebx)\n" );
    printf( "\tpopw %%ds\n" );

    if (reg_func)
    {
        /* Retrieve ebx from the 32-bit stack */
        printf( "\tmovl %%fs:28(%%ebx),%%ebx\n" );
    }
    else
    {
        /* Set ax equal to ds for window procedures */
        printf( "\tmovw %%ds,%%ax\n" );
    }

    /* Jump to the called routine */

    printf( "\t.byte 0x66\n" );
    printf( "\tlret\n" );
}


/*******************************************************************
 *         BuildRet16Func
 *
 * Build the return code for 16-bit callbacks
 */
static void BuildRet16Func()
{
    printf( "\t.globl " PREFIX "CALL16_Ret_word\n" );
    printf( "\t.globl " PREFIX "CALL16_Ret_long\n" );

    /* Put return value into eax */

    printf( PREFIX "CALL16_Ret_long:\n" );
    printf( "\tpushw %%dx\n" );
    printf( "\tpushw %%ax\n" );
    printf( "\tpopl %%eax\n" );
    printf( PREFIX "CALL16_Ret_word:\n" );

    /* Restore 32-bit segment registers */

    printf( "\tmovw $0x%04x,%%bx\n", WINE_DATA_SELECTOR );
    printf( "\tmovw %%bx,%%ds\n" );
    printf( "\tmovw %%bx,%%es\n" );
    printf( "\tmovw %%bx,%%ss\n" );

    /* Restore the 32-bit stack */

    printf( "\tmovl " PREFIX "IF1632_Saved32_esp,%%esp\n" );
    printf( "\tpopl " PREFIX "IF1632_Saved32_esp\n" );

    /* Restore the 32-bit registers */

    printf( "\tpopl %%edi\n" );
    printf( "\tpopl %%esi\n" );
    printf( "\tpopl %%edx\n" );
    printf( "\tpopl %%ecx\n" );
    printf( "\tpopl %%ebx\n" );

    /* Return to caller */

    printf( "\tpopl %%ebp\n" );
    printf( "\tlret\n" );

    /* Declare the return address variables */

    printf( "\t.data\n" );
    printf( "\t.globl " PREFIX "CALL16_RetAddr_word\n" );
    printf( "\t.globl " PREFIX "CALL16_RetAddr_long\n" );
    printf( PREFIX "CALL16_RetAddr_word:\t.long 0\n" );
    printf( PREFIX "CALL16_RetAddr_long:\t.long 0\n" );
    printf( "\t.text\n" );
}


static void usage(void)
{
    fprintf(stderr, "usage: build -spec SPECNAMES\n"
                    "       build -call32 FUNCTION_PROFILES\n"
                    "       build -call16 FUNCTION_PROFILES\n" );
    exit(1);
}


int main(int argc, char **argv)
{
    int i;

    if (argc <= 2) usage();

    if (!strcmp( argv[1], "-spec16" ))
    {
        for (i = 2; i < argc; i++) BuildSpec16Files( argv[i] );
    }
    else if (!strcmp( argv[1], "-spec32" ))
    {
        for (i = 2; i < argc; i++) BuildSpec32Files( argv[i] );
    }
    else if (!strcmp( argv[1], "-call32" ))  /* 32-bit callbacks */
    {
        /* File header */

        printf( "/* File generated automatically. Do no edit! */\n\n" );
        printf( "\t.text\n" );

        /* Build the callback functions */

        for (i = 2; i < argc; i++) BuildCall32Func( argv[i] );

        /* Output the argument debugging strings */

        if (debugging)
        {
            printf( "/* Argument strings */\n" );
            for (i = 2; i < argc; i++)
            {
                printf( "CALL32_Str_%s:\n", argv[i] );
                printf( "\t.ascii \"%s\\0\"\n", argv[i] + 5 );
            }
        }
    }
    else if (!strcmp( argv[1], "-call16" ))  /* 16-bit callbacks */
    {
        /* File header */

        printf( "/* File generated automatically. Do no edit! */\n\n" );
        printf( "\t.text\n" );
        printf( "\t.globl " PREFIX "CALL16_Start\n" );
        printf( PREFIX "CALL16_Start:\n" );

        /* Build the callback functions */

        for (i = 2; i < argc; i++) BuildCall16Func( argv[i] );

        /* Output the 16-bit return code */

        BuildRet16Func();

        printf( "\t.globl " PREFIX "CALL16_End\n" );
        printf( PREFIX "CALL16_End:\n" );
    }
    else usage();

    return 0;
}
