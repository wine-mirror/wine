/* static char Copyright[] = "Copyright  Robert J. Amstadt, 1993"; */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "wine.h"

#ifdef linux
#ifdef __ELF__
#define UTEXTSEL 0x0f
#else
#define UTEXTSEL 0x23
#endif
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
#define UTEXTSEL 0x1f
#endif

/* ELF symbols do not have an underscore in front */
#ifdef __ELF__
#define PREFIX
#else
#define PREFIX "_"
#endif

#define VARTYPE_BYTE	0
#define VARTYPE_SIGNEDWORD	0
#define VARTYPE_WORD	1
#define VARTYPE_LONG	2
#define VARTYPE_FARPTR	3

#define FUNCTYPE_PASCAL_16	15
#define FUNCTYPE_PASCAL		16
#define FUNCTYPE_C		17
#define FUNCTYPE_REG		19

#define EQUATETYPE_ABS	18
#define TYPE_RETURN	20

/*#define MAX_ORDINALS	1024*/
#define MAX_ORDINALS	1299

#define PUSH_0		"\tpushl\t$0\n"
#define PUSH_SS		"\tpushw\t$0\n\tpushw\t%%ss\n"
#define PUSH_ESP	"\tpushl\t%%esp\n"
#define PUSH_EFL	"\tpushfl\n"
#define PUSH_CS		"\tpushw\t$0\n\tpushw\t%%cs\n"
#define PUSH_EIP	"\tpushl\t$0\n"
#define PUSH_DS		"\tpushw\t$0\n\tpushw\t%%ds\n"
#define PUSH_ES		"\tpushw\t$0\n\tpushw\t%%es\n"
#define PUSH_FS		"\tpushw\t$0\n\tpushw\t%%fs\n"
#define PUSH_GS		"\tpushw\t$0\n\tpushw\t%%gs\n"
#define PUSH_EAX	"\tpushl\t%%eax\n"
#define PUSH_ECX	"\tpushl\t%%ecx\n"
#define PUSH_EDX	"\tpushl\t%%edx\n"
#define PUSH_EBX	"\tpushl\t%%ebx\n"
#define PUSH_EBP	"\tpushl\t%%ebp\n"
#define PUSH_ESI	"\tpushl\t%%esi\n"
#define PUSH_EDI	"\tpushl\t%%edi\n"

#define POP_0		"\tadd\t$4,%%esp\n"
#define POP_SS		"\tpopw\t%%ss\n\tadd\t$2,%%esp\n"
#define POP_ESP		"\tpopl\t%%esp\n"
#define POP_EFL		"\tpopl\t%%gs:return_value\n"
#define POP_CS		"\tpopw\t%%cs\n\tadd\t$2,%%esp\n"
#define POP_EIP		"\tpopl\t$0\n"
#define POP_DS		"\tpopw\t%%ds\n\tadd\t$2,%%esp\n"
#define POP_ES		"\tpopw\t%%es\n\tadd\t$2,%%esp\n"
#define POP_FS		"\tpopw\t%%fs\n\tadd\t$2,%%esp\n"
#define POP_GS		"\tpopw\t%%gs\n\tadd\t$2,%%esp\n"
#define POP_EAX		"\tpopl\t%%eax\n"
#define POP_ECX		"\tpopl\t%%ecx\n"
#define POP_EDX		"\tpopl\t%%edx\n"
#define POP_EBX		"\tpopl\t%%ebx\n"
#define POP_EBP		"\tpopl\t%%ebp\n"
#define POP_ESI		"\tpopl\t%%esi\n"
#define POP_EDI		"\tpopl\t%%edi\n"

char **context_strings;
char **pop_strings;
int    n_context_strings;

typedef struct ordinal_definition_s
{
    int valid;
    int type;
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
    int n_args_16;
    int arg_types_16[16];
    int arg_16_offsets[16];
    int arg_16_size;
    char internal_name[80];
    int n_args_32;
    int arg_indices_32[16];
} ORDFUNCDEF;

typedef struct ordinal_return_definition_s
{
    int arg_size;
    int ret_value;
} ORDRETDEF;

ORDDEF OrdinalDefinitions[MAX_ORDINALS];

char LowerDLLName[80];
char UpperDLLName[80];
int Limit;
int DLLId;
FILE *SpecFp;

char *ParseBuffer = NULL;
char *ParseNext;
char ParseSaveChar;
int Line;

int IsNumberString(char *s)
{
    while (*s != '\0')
	if (!isdigit(*s++))
	    return 0;

    return 1;
}

char *strlower(char *s)
{
    char *p;
    
    for(p = s; *p != '\0'; p++)
	*p = tolower(*p);

    return s;
}

char *strupper(char *s)
{
    char *p;
    
    for(p = s; *p != '\0'; p++)
	*p = toupper(*p);

    return s;
}

int stricmp(char *s1, char *s2)
{
    if (strlen(s1) != strlen(s2))
	return -1;
    
    while (*s1 != '\0')
	if (*s1++ != *s2++)
	    return -1;
    
    return 0;
}

char *
GetTokenInLine(void)
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
    
    if (*p == '\0')
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

char *
GetToken(void)
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

int
ParseVariable(int ordinal, int type)
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
    odp->valid = 1;
    odp->type = type;
    strcpy(odp->export_name, export_name);
    
    vdp = malloc(sizeof(*vdp));
    odp->additional_data = vdp;
    
    vdp->n_values = n_values;
    vdp->values = realloc(value_array, sizeof(*value_array) * n_values);

    return 0;
}

int
ParseExportFunction(int ordinal, int type)
{
    char *token;
    ORDDEF *odp;
    ORDFUNCDEF *fdp;
    int i;
    int current_offset;
    int arg_size;
	
    
    if (ordinal >= MAX_ORDINALS)
    {
	fprintf(stderr, "%d: Ordinal number too large\n", Line);
	exit(1);
    }
    
    odp = &OrdinalDefinitions[ordinal];
    strcpy(odp->export_name, GetToken());
    odp->valid = 1;
    odp->type = type;
    fdp = malloc(sizeof(*fdp));
    odp->additional_data = fdp;

    token = GetToken();
    if (*token != '(')
    {
	fprintf(stderr, "%d: Expected '(' got '%s'\n", Line, token);
	exit(1);
    }

    fdp->arg_16_size = 0;
    for (i = 0; i < 16; i++)
    {
	token = GetToken();
	if (*token == ')')
	    break;

	if (stricmp(token, "byte") == 0 || stricmp(token, "word") == 0)
	{
	    fdp->arg_types_16[i] = VARTYPE_WORD;
	    fdp->arg_16_size += 2;
	    fdp->arg_16_offsets[i] = 2;
	}
	else if (stricmp(token, "s_byte") == 0 || 
		 stricmp(token, "s_word") == 0)
	{
	    fdp->arg_types_16[i] = VARTYPE_SIGNEDWORD;
	    fdp->arg_16_size += 2;
	    fdp->arg_16_offsets[i] = 2;
	}
	else if (stricmp(token, "long") == 0 || stricmp(token, "s_long") == 0)
	{
	    fdp->arg_types_16[i] = VARTYPE_LONG;
	    fdp->arg_16_size += 4;
	    fdp->arg_16_offsets[i] = 4;
	}
	else if (stricmp(token, "ptr") == 0)
	{
	    fdp->arg_types_16[i] = VARTYPE_FARPTR;
	    fdp->arg_16_size += 4;
	    fdp->arg_16_offsets[i] = 4;
	}
	else
	{
	    fprintf(stderr, "%d: Unknown variable type '%s'\n", Line, token);
	    exit(1);
	}
    }
    fdp->n_args_16 = i;

    if (type == FUNCTYPE_PASCAL_16 || type == FUNCTYPE_PASCAL ||
	type == FUNCTYPE_REG )
    {
	current_offset = 0;
	for (i--; i >= 0; i--)
	{
	    arg_size = fdp->arg_16_offsets[i];
	    fdp->arg_16_offsets[i] = current_offset;
	    current_offset += arg_size;
	}
    }
    else
    {
	current_offset = 0;
	for (i = 0; i < fdp->n_args_16; i++)
	{
	    arg_size = fdp->arg_16_offsets[i];
	    fdp->arg_16_offsets[i] = current_offset;
	    current_offset += arg_size;
	}
    }

    strcpy(fdp->internal_name, GetToken());
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

	fdp->arg_indices_32[i] = atoi(token);
	if (fdp->arg_indices_32[i] < 1 || 
	    fdp->arg_indices_32[i] > fdp->n_args_16)
	{
	    fprintf(stderr, "%d: Bad argument index %d\n", Line,
		    fdp->arg_indices_32[i]);
	    exit(1);
	}
    }
    fdp->n_args_32 = i;

    return 0;
}

int
ParseEquate(int ordinal)
{
    ORDDEF *odp;
    char *token;
    char *endptr;
    int value;
    
    if (ordinal >= MAX_ORDINALS)
    {
	fprintf(stderr, "%d: Ordinal number too large\n", Line);
	exit(1);
    }
    
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

    odp->valid = 1;
    odp->type = EQUATETYPE_ABS;
    odp->additional_data = (void *) value;

    return 0;
}

int
ParseReturn(int ordinal)
{
    ORDDEF *odp;
    ORDRETDEF *rdp;
    char *token;
    char *endptr;
    
    if (ordinal >= MAX_ORDINALS)
    {
	fprintf(stderr, "%d: Ordinal number too large\n", Line);
	exit(1);
    }

    rdp = malloc(sizeof(*rdp));
    
    odp = &OrdinalDefinitions[ordinal];
    strcpy(odp->export_name, GetToken());
    odp->valid = 1;
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

int
ParseOrdinal(int ordinal)
{
    char *token;
    
    token = GetToken();
    if (token == NULL)
    {
	fprintf(stderr, "%d: Expected type after ordinal\n", Line);
	exit(1);
    }

    if (stricmp(token, "byte") == 0)
	return ParseVariable(ordinal, VARTYPE_BYTE);
    else if (stricmp(token, "word") == 0)
	return ParseVariable(ordinal, VARTYPE_WORD);
    else if (stricmp(token, "long") == 0)
	return ParseVariable(ordinal, VARTYPE_LONG);
    else if (stricmp(token, "c") == 0)
	return ParseExportFunction(ordinal, FUNCTYPE_C);
    else if (stricmp(token, "p") == 0)
	return ParseExportFunction(ordinal, FUNCTYPE_PASCAL);
    else if (stricmp(token, "pascal") == 0)
	return ParseExportFunction(ordinal, FUNCTYPE_PASCAL);
    else if (stricmp(token, "pascal16") == 0)
	return ParseExportFunction(ordinal, FUNCTYPE_PASCAL_16);
    else if (stricmp(token, "register") == 0)
	return ParseExportFunction(ordinal, FUNCTYPE_REG);
    else if (stricmp(token, "equate") == 0)
	return ParseEquate(ordinal);
    else if (stricmp(token, "return") == 0)
	return ParseReturn(ordinal);
    else
    {
	fprintf(stderr, 
		"%d: Expected type after ordinal, found '%s' instead\n",
		Line, token);
	exit(1);
    }
}

int
ParseTopLevel(void)
{
    char *token;
    
    while ((token = GetToken()) != NULL)
    {
	if (stricmp(token, "name") == 0)
	{
	    strcpy(LowerDLLName, GetToken());
	    strlower(LowerDLLName);

	    strcpy(UpperDLLName, LowerDLLName);
	    strupper(UpperDLLName);
	}
	else if (stricmp(token, "id") == 0)
	{
	    token = GetToken();
	    if (!IsNumberString(token))
	    {
		fprintf(stderr, "%d: Expected number after id\n", Line);
		exit(1);
	    }
	    
	    DLLId = atoi(token);
	}
	else if (stricmp(token, "length") == 0)
	{
	    token = GetToken();
	    if (!IsNumberString(token))
	    {
		fprintf(stderr, "%d: Expected number after length\n", Line);
		exit(1);
	    }

	    Limit = atoi(token);
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

void
InitContext(void)
{
    struct sigcontext_struct context;
    int i;
    
    n_context_strings = sizeof(context) / 4;
    context_strings   = (char **) malloc(n_context_strings * sizeof(char **));
    pop_strings       = (char **) malloc(n_context_strings * sizeof(char **));
    for (i = 0; i < n_context_strings; i++)
    {
	context_strings[i] = PUSH_0;
	pop_strings[i]     = POP_0;
    }

    i = n_context_strings - 1 + ((int) &context - (int) &context.sc_esp) / 4;
    context_strings[i] = PUSH_ESP;

    i = n_context_strings - 1 + ((int) &context - (int) &context.sc_ebp) / 4;
    context_strings[i] = PUSH_EBP;
    pop_strings[n_context_strings - 1 - i] = POP_EBP;

    i = n_context_strings - 1 + ((int) &context - (int) &context.sc_eip) / 4;
    context_strings[i] = PUSH_EIP;

#ifndef __FreeBSD__
    i = n_context_strings - 1 + ((int) &context - (int)&context.sc_eflags) / 4;
#else
    i = n_context_strings - 1 + ((int) &context - (int)&context.sc_efl) / 4;
#endif
    context_strings[i] = PUSH_EFL;
    pop_strings[n_context_strings - 1 - i] = POP_EFL;

    i = n_context_strings - 1 + ((int) &context - (int) &context.sc_es) / 4;
    context_strings[i] = PUSH_ES;
    pop_strings[n_context_strings - 1 - i] = POP_ES;

    i = n_context_strings - 1 + ((int) &context - (int) &context.sc_ds) / 4;
    context_strings[i] = PUSH_DS;
    pop_strings[n_context_strings - 1 - i] = POP_DS;

    i = n_context_strings - 1 + ((int) &context - (int) &context.sc_cs) / 4;
    context_strings[i] = PUSH_CS;

    i = n_context_strings - 1 + ((int) &context - (int) &context.sc_ss) / 4;
    context_strings[i] = PUSH_SS;

    i = n_context_strings - 1 + ((int) &context - (int) &context.sc_edi) / 4;
    context_strings[i] = PUSH_EDI;
    pop_strings[n_context_strings - 1 - i] = POP_EDI;

    i = n_context_strings - 1 + ((int) &context - (int) &context.sc_esi) / 4;
    context_strings[i] = PUSH_ESI;
    pop_strings[n_context_strings - 1 - i] = POP_ESI;

    i = n_context_strings - 1 + ((int) &context - (int) &context.sc_ebx) / 4;
    context_strings[i] = PUSH_EBX;
    pop_strings[n_context_strings - 1 - i] = POP_EBX;

    i = n_context_strings - 1 + ((int) &context - (int) &context.sc_edx) / 4;
    context_strings[i] = PUSH_EDX;
    pop_strings[n_context_strings - 1 - i] = POP_EDX;

    i = n_context_strings - 1 + ((int) &context - (int) &context.sc_ecx) / 4;
    context_strings[i] = PUSH_ECX;
    pop_strings[n_context_strings - 1 - i] = POP_ECX;

    i = n_context_strings - 1 + ((int) &context - (int) &context.sc_eax) / 4;
    context_strings[i] = PUSH_EAX;
    pop_strings[n_context_strings - 1 - i] = POP_EAX;
}

void
OutputVariableCode(FILE *fp, char *storage, ORDDEF *odp)
{
    ORDVARDEF *vdp;
    int i;

    vdp = odp->additional_data;
    for (i = 0; i < vdp->n_values; i++)
    {
	if ((i & 7) == 0)
	    fprintf(fp, "\t%s\t", storage);
	    
	fprintf(fp, "%d", vdp->values[i]);
	
	if ((i & 7) == 7 || i == vdp->n_values - 1)
	    fprintf(fp, "\n");
	else
	    fprintf(fp, ", ");
    }
    fprintf(fp, "\n");
}

int main(int argc, char **argv)
{
    ORDDEF *odp;
    ORDFUNCDEF *fdp;
    ORDRETDEF *rdp;
    FILE *fp;
    char filename[80];
    int i, ci, add_count;
    int prev_index;      /* Index to previous #define (-1 if none) */

    /* the difference between last #define and the current */
    int prev_n_args;
    
    if (argc < 2)
    {
	fprintf(stderr, "usage: build SPECNAME\n       build -p\n");
	exit(1);
    }

    InitContext();

    if (strcmp("-p", argv[1]) == 0)
    {
	fp = fopen("pop.h", "w");
	add_count = 0;
	for (i = 0; i < n_context_strings; i++)
	{
	    if (strncmp(pop_strings[i], "\tadd\t", 5) == 0)
	    {
		add_count += atoi(pop_strings[i] + 6);
	    }
	    else
	    {
		if (add_count > 0)
		{
		    fprintf(fp, "\tadd\t$%d,%%esp\n", add_count);
		    add_count = 0;
		}
		
		fprintf(fp, pop_strings[i]);
	    }
	}
    
	if (add_count > 0)
	    fprintf(fp, "\tadd\t$%d,%%esp\n", add_count);

	fprintf(fp, "\tpushl\t%%gs:return_value\n\tpopfl\n");
		
	fclose(fp);
	exit(0);
    }

    SpecFp = fopen(argv[1], "r");
    if (SpecFp == NULL)
    {
	fprintf(stderr, "Could not open specification file, '%s'\n", argv[1]);
	exit(1);
    }

    ParseTopLevel();

    sprintf(filename, "dll_%s.S", LowerDLLName);
    fp = fopen(filename, "w");
#ifdef __ELF__
    fprintf (fp, "#define __ASSEMBLY__\n");
    fprintf (fp, "#include <asm/segment.h>\n");
#endif
#if 0
    fprintf(fp, "\t.globl " PREFIX "%s_Dispatch\n", UpperDLLName);
    fprintf(fp, PREFIX "%s_Dispatch:\n", UpperDLLName);
    fprintf(fp, "\tandl\t$0x0000ffff,%%esp\n");
    fprintf(fp, "\tandl\t$0x0000ffff,%%ebp\n");
#ifdef __ELF__
    fprintf(fp, "\tljmp\t$USER_CS, $" PREFIX "CallTo32\n\n");
#else
    fprintf(fp, "\tjmp\t_CallTo32\n\n");
#endif

    fprintf(fp, "\t.globl " PREFIX "%s_Dispatch_16\n", UpperDLLName);
    fprintf(fp, PREFIX "%s_Dispatch_16:\n", UpperDLLName);
    fprintf(fp, "\tandl\t$0x0000ffff,%%esp\n");
    fprintf(fp, "\tandl\t$0x0000ffff,%%ebp\n");
#ifdef __ELF__
    fprintf(fp, "\tljmp\t$USER_CS, $" PREFIX "CallTo32_16\n\n");
#else
    fprintf(fp, "\tjmp\t_CallTo32_16\n\n");
#endif
#endif

    odp = OrdinalDefinitions;
    for (i = 0; i <= Limit; i++, odp++)
    {
	fprintf(fp, "\t.globl " PREFIX "%s_Ordinal_%d\n", UpperDLLName, i);

	if (!odp->valid)
	{
	    fprintf(fp, PREFIX "%s_Ordinal_%d:\n", UpperDLLName, i);
	    fprintf(fp, "\tmovl\t$0x%08x,%%eax\n", (DLLId << 16) | i);
	    fprintf(fp, "\tpushw\t$0\n");
#ifdef __ELF__
            fprintf(fp, "\tljmp\t$USER_CS, $" PREFIX "CallTo32\n\n");
#else
            fprintf(fp, "\tjmp\t_CallTo32\n\n");
#endif
	}
	else
	{
	    fdp = odp->additional_data;
	    rdp = odp->additional_data;
	    
	    switch (odp->type)
	    {
	      case EQUATETYPE_ABS:
		fprintf(fp, PREFIX "%s_Ordinal_%d = %d\n\n", 
			UpperDLLName, i, (int) odp->additional_data);
		break;

	      case VARTYPE_BYTE:
                fprintf(fp, PREFIX "%s_Ordinal_%d:\n", UpperDLLName, i);
		OutputVariableCode(fp, ".byte", odp);
		break;

	      case VARTYPE_WORD:
                fprintf(fp, PREFIX "%s_Ordinal_%d:\n", UpperDLLName, i);
		OutputVariableCode(fp, ".word", odp);
		break;

	      case VARTYPE_LONG:
                fprintf(fp, PREFIX "%s_Ordinal_%d:\n", UpperDLLName, i);
		OutputVariableCode(fp, ".long", odp);
		break;

	      case TYPE_RETURN:
		fprintf(fp, PREFIX "%s_Ordinal_%d:\n", UpperDLLName, i);
		fprintf(fp, "\tmovw\t$%d,%%ax\n", rdp->ret_value & 0xffff);
		fprintf(fp, "\tmovw\t$%d,%%dx\n", 
			(rdp->ret_value >> 16) & 0xffff);
		fprintf(fp, "\t.byte\t0x66\n");
		if (rdp->arg_size != 0)
		    fprintf(fp, "\tlret\t$%d\n", rdp->arg_size);
		else
		    fprintf(fp, "\tlret\n");
		break;

	      case FUNCTYPE_REG:
		fprintf(fp, PREFIX "%s_Ordinal_%d:\n", UpperDLLName, i);
		fprintf(fp, "\tandl\t$0x0000ffff,%%esp\n");
		fprintf(fp, "\tandl\t$0x0000ffff,%%ebp\n");

		for (ci = 0; ci < n_context_strings; ci++)
		    fprintf(fp, context_strings[ci]);

		fprintf(fp, "\tmovl\t%%ebp,%%eax\n");
		fprintf(fp, "\tmovw\t%%esp,%%ebp\n");
		fprintf(fp, "\tpushl\t%d(%%ebp)\n",
			sizeof(struct sigcontext_struct));
		fprintf(fp, "\tmovl\t%%eax,%%ebp\n");
                fprintf(fp, "\tmovl\t$0x%08x,%%eax\n", (DLLId << 16) | i);
		fprintf(fp, "\tpushw\t$%d\n", 
			sizeof(struct sigcontext_struct) + 4);
#ifdef __ELF__
                fprintf(fp, "\tljmp\t$USER_CS, $" PREFIX "CallTo32\n\n");
#else
                fprintf(fp, "\tjmp\t_CallTo32\n\n");
#endif
		break;

	      case FUNCTYPE_PASCAL:
		fprintf(fp, PREFIX "%s_Ordinal_%d:\n", UpperDLLName, i);
                fprintf(fp, "\tmovl\t$0x%08x,%%eax\n", (DLLId << 16) | i);
		fprintf(fp, "\tpushw\t$%d\n", fdp->arg_16_size);
#ifdef __ELF__
                fprintf(fp, "\tljmp\t$USER_CS, $" PREFIX "CallTo32\n\n");
#else
                fprintf(fp, "\tjmp\t_CallTo32\n\n");
#endif
                break;
		
	      case FUNCTYPE_PASCAL_16:
		fprintf(fp, PREFIX "%s_Ordinal_%d:\n", UpperDLLName, i);
                fprintf(fp, "\tmovl\t$0x%08x,%%eax\n", (DLLId << 16) | i);
		fprintf(fp, "\tpushw\t$%d\n", fdp->arg_16_size);
#ifdef __ELF__
                fprintf(fp, "\tljmp\t$USER_CS, $" PREFIX "CallTo32_16\n\n");
#else
                fprintf(fp, "\tjmp\t_CallTo32_16\n\n");
#endif
		break;
		
	      case FUNCTYPE_C:
	      default:
		fprintf(fp, PREFIX "%s_Ordinal_%d:\n", UpperDLLName, i);
                fprintf(fp, "\tmovl\t$0x%08x,%%eax\n", (DLLId << 16) | i);
		fprintf(fp, "\tpushw\t$0\n");
#ifdef __ELF__
                fprintf(fp, "\tljmp\t$USER_CS, $" PREFIX "CallTo32\n\n");
#else
                fprintf(fp, "\tjmp\t_CallTo32\n\n");
#endif
		break;
	    }
	}
    }

    fclose(fp);

#ifndef SHORTNAMES
    sprintf(filename, "dll_%s_tab.c", LowerDLLName);
#else
    sprintf(filename, "dtb_%s.c", LowerDLLName);
#endif
    fp = fopen(filename, "w");

    fprintf(fp, "#include <stdio.h>\n");
    fprintf(fp, "#include <stdlib.h>\n");
    fprintf(fp, "#include \042dlls.h\042\n\n");

    for (i = 0; i <= Limit; i++)
    {
	fprintf(fp, "extern void %s_Ordinal_%d();\n", UpperDLLName, i);
    }
    
    odp = OrdinalDefinitions;
    for (i = 0; i <= Limit; i++, odp++)
    {
	if (odp->valid && 
	    (odp->type == FUNCTYPE_PASCAL || odp->type == FUNCTYPE_PASCAL_16 ||
	     odp->type == FUNCTYPE_REG || odp->type == FUNCTYPE_C ))
	{
	    fdp = odp->additional_data;
	    fprintf(fp, "extern int %s();\n", fdp->internal_name);
	}
    }
    /******* Michael Veksler 95-2-3 (pointers instead of fixed data) ****/
    fprintf(fp,"unsigned short  %s_offsets[]={\n" , UpperDLLName);
    prev_index=-1;  /* Index to previous #define (-1 if none) */

    /* the difference between last #define and the current */
    prev_n_args= 0;
    
    odp = OrdinalDefinitions;
    for (i = 0; i <= Limit; i++, odp++)
    {
	int argnum;
	fdp = odp->additional_data;

	switch (odp->type)
	{
	  case FUNCTYPE_PASCAL:
	  case FUNCTYPE_PASCAL_16:
	  case FUNCTYPE_REG:
	    if (!odp->valid || fdp->n_args_32 <=0 )
	       continue;
	    if (prev_index<0) 
		fprintf(fp,"#\tdefine %s_ref_%d   0\n\t", UpperDLLName, i);
	    else
		fprintf(fp,"#\tdefine %s_ref_%d   %s_ref_%d+%d\n\t",
			UpperDLLName,i, UpperDLLName,prev_index ,prev_n_args);
	    for (argnum = 0; argnum < fdp->n_args_32; argnum++)
		 fprintf(fp, "%d, ",
		         fdp->arg_16_offsets[fdp->arg_indices_32[argnum]-1]);
	    fprintf(fp,"\n");
	    
	    prev_n_args=fdp->n_args_32;
	    prev_index=i;
	}
    }    
    fprintf(fp,"};\n");


    fprintf(fp,"unsigned char  %s_types[]={\n" , UpperDLLName);

    odp = OrdinalDefinitions;
    for (i = 0; i <= Limit; i++, odp++)
    {
	int argnum;

	fdp = odp->additional_data;

	switch (odp->type)
	{
	  case FUNCTYPE_PASCAL:
	  case FUNCTYPE_PASCAL_16:
	  case FUNCTYPE_REG:
	    if (!odp->valid || fdp->n_args_32 <=0 )
	       continue;
	    
	    fprintf(fp,"/* %s_%d */\n\t", UpperDLLName, i);
	    
	    for (argnum = 0; argnum < fdp->n_args_32; argnum++)
		fprintf(fp, "%d, ", fdp->arg_types_16[argnum]);
	    fprintf(fp,"\n");
	}
    }    
    fprintf(fp,"};\n");


    /**************************************************/
    fprintf(fp, "\nstruct dll_table_entry_s %s_table[%d] =\n", 
	    UpperDLLName, Limit + 1);
    fprintf(fp, "{\n");
    odp = OrdinalDefinitions;
    for (i = 0; i <= Limit; i++, odp++)
    {
	fdp = odp->additional_data;

	if (!odp->valid)
	    odp->type = -1;
	
	switch (odp->type)
	{
	  case FUNCTYPE_PASCAL:
	  case FUNCTYPE_PASCAL_16:
	  case FUNCTYPE_REG:
	    fprintf(fp, "    { 0x%x, %s_Ordinal_%d, ", UTEXTSEL, UpperDLLName, i);
	    fprintf(fp, "\042%s\042, ", odp->export_name);
	    fprintf(fp, "%s, DLL_HANDLERTYPE_PASCAL, ", fdp->internal_name);
	    fprintf(fp, "%d, ", fdp->n_args_32);
	    if (fdp->n_args_32 > 0)
	       fprintf(fp,"%s_ref_%d", UpperDLLName, i);
	    else
	       fprintf(fp,"      0    ");
	       
#ifdef WINESTAT
	    fprintf(fp, ",0 ");
#endif	    
	    fprintf(fp, "}, \n");
	    break;
		
	  case FUNCTYPE_C:
	    fprintf(fp, "    { 0x%x, %s_Ordinal_%d, ", UTEXTSEL, UpperDLLName, i);
	    fprintf(fp, "\042%s\042, ", odp->export_name);
	    fprintf(fp, "%s, DLL_HANDLERTYPE_C, ", fdp->internal_name);
#ifdef WINESTAT
	    fprintf(fp, "0, ");
#endif	    
	    fprintf(fp, "%d, ", fdp->n_args_32);
	    if (fdp->n_args_32 > 0)
	    {
		int argnum;
		
		fprintf(fp, "\n      {\n");
		for (argnum = 0; argnum < fdp->n_args_32; argnum++)
		{
		    fprintf(fp, "        { %d, %d },\n",
			    fdp->arg_16_offsets[fdp->arg_indices_32[argnum]-1],
			    fdp->arg_types_16[argnum]);
		}
		fprintf(fp, "      }\n    ");
	    }
	    fprintf(fp, "}, \n");
	    break;
	    
	  default:
	    fprintf(fp, "    { 0x%x, %s_Ordinal_%d, \042\042, NULL },\n", 
		    UTEXTSEL, UpperDLLName, i);
	    break;
	}
    }
    fprintf(fp, "};\n");

    fclose(fp);
    return 0;
}

