static char RCSId[] = "$Id: build.c,v 1.3 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef linux
#define UTEXTSEL 	0x23
#define UDATASEL	0x2b
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
#define UTEXTSEL 	0x1f
#define UDATASEL	0x27
#endif

#define VARTYPE_BYTE	0
#define VARTYPE_SIGNEDWORD	0
#define VARTYPE_WORD	1
#define VARTYPE_LONG	2
#define VARTYPE_FARPTR	3

#define FUNCTYPE_PASCAL	16
#define FUNCTYPE_C	17
#define FUNCTYPE_REG	19

#define EQUATETYPE_ABS	18
#define TYPE_RETURN	20

#define MAX_ORDINALS	1024

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
    int arg_types[16];
    int i;
    int arg_num;
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

    if (type == FUNCTYPE_PASCAL || type == FUNCTYPE_REG)
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
    int value;
    
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
OutputVariableCode(FILE *fp, char *storage, ORDDEF *odp)
{
    ORDVARDEF *vdp;
    int i;

    fprintf(fp, "_%s_Ordinal_%d:\n", UpperDLLName, i);

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

main(int argc, char **argv)
{
    ORDDEF *odp;
    ORDFUNCDEF *fdp;
    ORDRETDEF *rdp;
    FILE *fp;
    char filename[80];
    char buffer[80];
    char *p;
    int i;
    
    if (argc < 2)
    {
	fprintf(stderr, "usage: build SPECNAME\n");
	exit(1);
    }

    SpecFp = fopen(argv[1], "r");
    if (SpecFp == NULL)
    {
	fprintf(stderr, "Could not open specification file, '%s'\n", argv[1]);
	exit(1);
    }

    ParseTopLevel();

    /**********************************************************************
     *					DLL ENTRY POINTS
     */
    sprintf(filename, "dll_%s.S", LowerDLLName);
    fp = fopen(filename, "w");

    fprintf(fp, "\t.globl _%s_Dispatch\n", UpperDLLName);
    fprintf(fp, "_%s_Dispatch:\n", UpperDLLName);
    fprintf(fp, "\tandl\t$0x0000ffff,%%esp\n");
    fprintf(fp, "\tandl\t$0x0000ffff,%%ebp\n");
    fprintf(fp, "\torl\t$0x%08x,%%eax\n", DLLId << 16);
    fprintf(fp, "\tjmp\t_CallTo32\n\n");

    odp = OrdinalDefinitions;
    for (i = 0; i <= Limit; i++, odp++)
    {
	fprintf(fp, "\t.globl _%s_Ordinal_%d\n", UpperDLLName, i);

	if (!odp->valid)
	{
	    fprintf(fp, "_%s_Ordinal_%d:\n", UpperDLLName, i);
#ifdef BOB_SAYS_NO
	    fprintf(fp, "\tandl\t$0x0000ffff,%%esp\n");
	    fprintf(fp, "\tandl\t$0x0000ffff,%%ebp\n");
#endif
	    fprintf(fp, "\tmovl\t$%d,%%eax\n", i);
	    fprintf(fp, "\tpushw\t$0\n");
	    fprintf(fp, "\tjmp\t_%s_Dispatch\n\n", UpperDLLName);
	}
	else
	{
	    fdp = odp->additional_data;
	    rdp = odp->additional_data;
	    
	    switch (odp->type)
	    {
	      case EQUATETYPE_ABS:
		fprintf(fp, "_%s_Ordinal_%d = %d\n\n", 
			UpperDLLName, i, (int) odp->additional_data);
		break;

	      case VARTYPE_BYTE:
		OutputVariableCode(fp, ".byte", odp);
		break;

	      case VARTYPE_WORD:
		OutputVariableCode(fp, ".word", odp);
		break;

	      case VARTYPE_LONG:
		OutputVariableCode(fp, ".long", odp);
		break;

	      case TYPE_RETURN:
		fprintf(fp, "_%s_Ordinal_%d:\n", UpperDLLName, i);
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
		fprintf(fp, "_%s_Ordinal_%d:\n", UpperDLLName, i);
		fprintf(fp, "\tandl\t$0x0000ffff,%%esp\n");
		fprintf(fp, "\tandl\t$0x0000ffff,%%ebp\n");
		fprintf(fp, "\tpushl\t$0\n");			/* cr2     */
		fprintf(fp, "\tpushl\t$0\n");			/* oldmask */
		fprintf(fp, "\tpushl\t$0\n");			/* i387    */
		fprintf(fp, "\tpushw\t$0\n");			/* __ssh   */
		fprintf(fp, "\tpushw\t%%ss\n");			/* ss      */
		fprintf(fp, "\tpushl\t%%esp\n");		/* esp     */
		fprintf(fp, "\tpushfl\n");			/* eflags  */
		fprintf(fp, "\tpushw\t$0\n");			/* __csh   */
		fprintf(fp, "\tpushw\t%%cs\n");			/* cs      */
		fprintf(fp, "\tpushl\t$0\n");			/* eip     */
		fprintf(fp, "\tpushl\t$0\n");			/* err     */
		fprintf(fp, "\tpushl\t$0\n");			/* trapno  */
		fprintf(fp, "\tpushal\n");			/* AX, ... */
		fprintf(fp, "\tpushw\t$0\n");			/* __dsh   */
		fprintf(fp, "\tpushw\t%%ds\n");			/* ds      */
		fprintf(fp, "\tpushw\t$0\n");			/* __esh   */
		fprintf(fp, "\tpushw\t%%es\n");			/* es      */
		fprintf(fp, "\tpushw\t$0\n");			/* __fsh   */
		fprintf(fp, "\tpushw\t%%fs\n");			/* fs      */
		fprintf(fp, "\tpushw\t$0\n");			/* __gsh   */
		fprintf(fp, "\tpushw\t%%gs\n");			/* gs      */
		fprintf(fp, "\tmovl\t%%ebp,%%eax\n");
		fprintf(fp, "\tmovw\t%%esp,%%ebp\n");
		fprintf(fp, "\tpushl\t88(%%ebp)\n");
		fprintf(fp, "\tmovl\t%%eax,%%ebp\n");
		fprintf(fp, "\tmovl\t$%d,%%eax\n", i);
		fprintf(fp, "\tpushw\t$92\n");
		fprintf(fp, "\tjmp\t_%s_Relay_%d:\n", UpperDLLName, i);
		break;

	      case FUNCTYPE_PASCAL:
		fprintf(fp, "_%s_Ordinal_%d:\n", UpperDLLName, i);
#ifdef BOB_SAYS_NO
		fprintf(fp, "\tandl\t$0x0000ffff,%%esp\n");
		fprintf(fp, "\tandl\t$0x0000ffff,%%ebp\n");
#endif
		fprintf(fp, "\tmovl\t$%d,%%eax\n", i);
		fprintf(fp, "\tpushw\t$%d\n", fdp->arg_16_size);
		fprintf(fp, "\tjmp\t_%s_Relay_%d:\n", UpperDLLName, i);
		break;
		
	      case FUNCTYPE_C:
	      default:
		fprintf(fp, "_%s_Ordinal_%d:\n", UpperDLLName, i);
#ifdef BOB_SAYS_NO
		fprintf(fp, "\tandl\t$0x0000ffff,%%esp\n");
		fprintf(fp, "\tandl\t$0x0000ffff,%%ebp\n");
#endif
		fprintf(fp, "\tmovl\t$%d,%%eax\n", i);
		fprintf(fp, "\tpushw\t$0\n");
		fprintf(fp, "\tjmp\t_%s_Relay_%d:\n", UpperDLLName, i);
		break;
	    }
	}
    }

    fclose(fp);

    /**********************************************************************
     *					RELAY CODE
     */
    sprintf(filename, "rly_%s.S", LowerDLLName);
    fp = fopen(filename, "w");

    odp = OrdinalDefinitions;
    for (i = 0; i <= Limit; i++, odp++)
    {
	if (!odp->valid)
	    continue;

	fdp = odp->additional_data;
	
	fprintf(fp, "\t.globl _%s_Relay_%d\n", UpperDLLName, i);
	fprintf(fp, "_%s_Relay_%d:\n", UpperDLLName, i);
	fprintf(fp, "\tandl\t$0x0000ffff,%%esp\n");
	fprintf(fp, "\tandl\t$0x0000ffff,%%ebp\n");

	fprintf(fp, "\tpushl\t%%ebp\n");
	fprintf(fp, "\tmovl\t%%esp,%%ebp\n");

	/*
 	 * Save registers.  286 mode does not have fs or gs.
	 */
	fprintf(fp, "\tpushw\t%%ds\n");
	fprintf(fp, "\tpushw\t%%es\n");

	/*
	 * Restore segment registers.
	 */
	fprintf(fp, "\tmovw\t%d,%%ax\n", UDATASEL);
	fprintf(fp, "\tmovw\t%%ax,%%ds\n");
	fprintf(fp, "\tmovw\t%%ax,%%es\n");

	/*
	 * Save old stack save variables, save stack registers, reload
	 * stack registers.
	 */
	fprintf(fp, "\tpushl\t_IF1632_Saved16_esp\n");
	fprintf(fp, "\tpushl\t_IF1632_Saved16_ebp\n");
	fprintf(fp, "\tpushw\t_IF1632_Saved16_ss\n");

	fprintf(fp, "\tmovw\t%%ss,_IF1632_Saved16_ss\n");
	fprintf(fp, "\tmovl\t%%esp,_IF1632_Saved16_esp\n");
	fprintf(fp, "\tmovl\t%%ebp,_IF1632_Saved16_ebp\n");

	fprintf(fp, "\tmovw\t%%ss,%%ax\n");
	fprintf(fp, "\tshll\t16,%%eax\n");
	fprintf(fp, "\torl\t%%esp,%%eax\n");

	fprintf(fp, "\tmovw\t_IF1632_Saved32_ss,%%ss\n");
	fprintf(fp, "\tmovl\t_IF1632_Saved32_esp,%%esp\n");
	fprintf(fp, "\tmovl\t_IF1632_Saved32_ebp,%%ebp\n");

	fprintf(fp, "\tpushl\t_Stack16Frame\n");
	fprintf(fp, "\tmovl\t%%eax,_Stack16Frame\n");

	/*
	 * Move arguments.
	 */
	

	/*
	 * Call entry point
	 */
	fprintf(fp, "\tcall\t%s\n", fdp->internal_name);

	/*
 	 * Restore registers, but do not destroy return value.
	 */
	fprintf(fp, "\tmovw\t_IF1632_Saved16_ss,%%ss\n");
	fprintf(fp, "\tmovl\t_IF1632_Saved16_esp,%%esp\n");
	fprintf(fp, "\tmovl\t_IF1632_Saved16_ebp,%%ebp\n");

	fprintf(fp, "\tpopw\t_IF1632_Saved16_ss\n");
	fprintf(fp, "\tpopl\t_IF1632_Saved16_ebp\n");
	fprintf(fp, "\tpopl\t_IF1632_Saved16_esp\n");

	fprintf(fp, "\tpopw\t%%es\n");
	fprintf(fp, "\tpopw\t%%ds\n");

	fprintf(fp, "\t.align\t2,0x90\n");
	fprintf(fp, "\tleave\n");
	
	/*
	 * Now we need to ditch the parameter bytes that were left on the
	 * stack. We do this by effectively popping the number of bytes,
	 * and the return address, removing the parameters and then putting
	 * the return address back on the stack.
	 * Normally this field is filled in by the relevant function in
	 * the emulation library, since it should know how many bytes to
	 * expect.
	 */
	fprintf(fp, "\tpopw\t%%gs:nbytes\n");
	fprintf(fp, "\tcmpw\t$0,%%gs:nbytes\n");
	fprintf(fp, "\tje\tnoargs\n");
	fprintf(fp, "\tpopw\t%%gs:offset\n");
	fprintf(fp, "\tpopw\t%%gs:selector\n");
	fprintf(fp, "\taddw\t%%gs:nbytes,%%esp\n");
	fprintf(fp, "\tpushw\t%%gs:selector\n");
	fprintf(fp, "\tpushw\t%%gs:offset\n");
	fprintf(fp, "noargs:\n");

	/*
	 * Last, but not least we need to move the high word from eax to dx
	 */
	fprintf(fp, "\t	pushl\t%%eax\n");
	fprintf(fp, "\tpopw\t%%dx\n");
	fprintf(fp, "\tpopw\t%%dx\n");

	fprintf(fp, "\t.byte\t0x66\n");
	fprintf(fp, "\tlret\n");

    }

    fclose(fp);

    /**********************************************************************
     *					DLL ENTRY TABLE
     */
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
	    (odp->type == FUNCTYPE_PASCAL || odp->type == FUNCTYPE_C ||
	     odp->type == FUNCTYPE_REG))
	{
	    fdp = odp->additional_data;
	    fprintf(fp, "extern int %s();\n", fdp->internal_name);
	}
    }
    
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
	  case FUNCTYPE_REG:
	    fprintf(fp, "    { 0x%x, %s_Ordinal_%d, ", UTEXTSEL, UpperDLLName, i);
	    fprintf(fp, "\042%s\042, ", odp->export_name);
	    fprintf(fp, "%s, DLL_HANDLERTYPE_PASCAL, ", fdp->internal_name);
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
}

