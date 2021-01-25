/* -*-C-*-
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
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

%option stack
%option noinput nounput noyy_top_state
%option 8bit never-interactive prefix="parser_"

nl	\r?\n
ws	[ \f\t\r]
cident	[a-zA-Z_][0-9a-zA-Z_]*
u_suffix	(u|U)
l_suffix	(l|L)
int	[0-9]+({l_suffix}?{u_suffix}?|{u_suffix}?{l_suffix}?)?
hexd	[0-9a-fA-F]
hex	0(x|X){hexd}+({l_suffix}?{u_suffix}?|{u_suffix}?{l_suffix}?)?
uuid	{hexd}{8}-{hexd}{4}-{hexd}{4}-{hexd}{4}-{hexd}{12}
double	[0-9]+\.[0-9]+([eE][+-]?[0-9]+)*

%x QUOTE
%x WSTRQUOTE
%x ATTR
%x PP_LINE
%x PP_PRAGMA
%x SQUOTE

%{

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#define YY_NO_UNISTD_H
#endif

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "wine/wpp.h"

#include "parser.tab.h"

static void addcchar(char c);
static char *get_buffered_cstring(void);

static char *cbuffer;
static int cbufidx;
static int cbufalloc = 0;

static int kw_token(const char *kw);
static int attr_token(const char *kw);

static void switch_to_acf(void);

static warning_list_t *disabled_warnings = NULL;

#define MAX_IMPORT_DEPTH 20
struct {
  YY_BUFFER_STATE state;
  char *input_name;
  int   line_number;
  char *temp_name;
} import_stack[MAX_IMPORT_DEPTH];
int import_stack_ptr = 0;

/* converts an integer in string form to an unsigned long and prints an error
 * on overflow */
static unsigned int xstrtoul(const char *nptr, char **endptr, int base)
{
    unsigned long val;

    errno = 0;
    val = strtoul(nptr, endptr, base);
    if ((val == ULONG_MAX && errno == ERANGE) || ((unsigned int)val != val))
        error_loc("integer constant %s is too large\n", nptr);
    return val;
}

UUID *parse_uuid(const char *u)
{
  UUID* uuid = xmalloc(sizeof(UUID));
  char b[3];
  /* it would be nice to use UuidFromStringA */
  uuid->Data1 = strtoul(u, NULL, 16);
  uuid->Data2 = strtoul(u+9, NULL, 16);
  uuid->Data3 = strtoul(u+14, NULL, 16);
  b[2] = 0;
  memcpy(b, u+19, 2); uuid->Data4[0] = strtoul(b, NULL, 16);
  memcpy(b, u+21, 2); uuid->Data4[1] = strtoul(b, NULL, 16);
  memcpy(b, u+24, 2); uuid->Data4[2] = strtoul(b, NULL, 16);
  memcpy(b, u+26, 2); uuid->Data4[3] = strtoul(b, NULL, 16);
  memcpy(b, u+28, 2); uuid->Data4[4] = strtoul(b, NULL, 16);
  memcpy(b, u+30, 2); uuid->Data4[5] = strtoul(b, NULL, 16);
  memcpy(b, u+32, 2); uuid->Data4[6] = strtoul(b, NULL, 16);
  memcpy(b, u+34, 2); uuid->Data4[7] = strtoul(b, NULL, 16);
  return uuid;
}

%}

/*
 **************************************************************************
 * The flexer starts here
 **************************************************************************
 */
%%
<INITIAL>^{ws}*\#{ws}*pragma{ws}+ yy_push_state(PP_PRAGMA);
<INITIAL,ATTR>^{ws}*\#{ws}*	yy_push_state(PP_LINE);
<PP_LINE>[^\n]*         {
                            int lineno;
                            char *cptr, *fname;
                            yy_pop_state();
                            lineno = (int)strtol(yytext, &cptr, 10);
                            if(!lineno)
                                error_loc("Malformed '#...' line-directive; invalid linenumber\n");
                            fname = strchr(cptr, '"');
                            if(!fname)
                                error_loc("Malformed '#...' line-directive; missing filename\n");
                            fname++;
                            cptr = strchr(fname, '"');
                            if(!cptr)
                                error_loc("Malformed '#...' line-directive; missing terminating \"\n");
                            *cptr = '\0';
                            line_number = lineno - 1;  /* We didn't read the newline */
                            input_name = xstrdup(fname);
                        }
<PP_PRAGMA>midl_echo[^\n]*  yyless(9); yy_pop_state(); return tCPPQUOTE;
<PP_PRAGMA>winrt[^\n]*  {
                            if(import_stack_ptr) {
                                if(!winrt_mode)
                                    error_loc("winrt IDL file imported in non-winrt mode\n");
                            }else {
                                const char *ptr = yytext+5;

                                winrt_mode = TRUE;

                                while(isspace(*ptr))
                                    ptr++;
                                if(!strncmp(ptr, "ns_prefix", 9) && (!*(ptr += 9) || isspace(*ptr)))
                                    use_abi_namespace = TRUE;
                            }
                            yy_pop_state();
                        }
<PP_PRAGMA>[^\n]*       parser_lval.str = xstrdup(yytext); yy_pop_state(); return aPRAGMA;
<INITIAL>^{ws}*midl_pragma{ws}+warning return tPRAGMA_WARNING;
<INITIAL,ATTR>\"	yy_push_state(QUOTE); cbufidx = 0;
<QUOTE>\"		{
				yy_pop_state();
				parser_lval.str = get_buffered_cstring();
				return aSTRING;
			}
<INITIAL,ATTR>L\"	yy_push_state(WSTRQUOTE); cbufidx = 0;
<WSTRQUOTE>\"		{
				yy_pop_state();
				parser_lval.str = get_buffered_cstring();
				return aWSTRING;
			}
<INITIAL,ATTR>\'	yy_push_state(SQUOTE); cbufidx = 0;
<SQUOTE>\'		{
				yy_pop_state();
				parser_lval.str = get_buffered_cstring();
				return aSQSTRING;
			}
<QUOTE,WSTRQUOTE,SQUOTE>\\\\	|
<QUOTE,WSTRQUOTE>\\\"	addcchar(yytext[1]);
<SQUOTE>\\\'	addcchar(yytext[1]);
<QUOTE,WSTRQUOTE,SQUOTE>\\.	addcchar('\\'); addcchar(yytext[1]);
<QUOTE,WSTRQUOTE,SQUOTE>.	addcchar(yytext[0]);
<INITIAL,ATTR>\[	yy_push_state(ATTR); return '[';
<ATTR>\]		yy_pop_state(); return ']';
<ATTR>{cident}		return attr_token(yytext);
<ATTR>{uuid}			{
				parser_lval.uuid = parse_uuid(yytext);
				return aUUID;
			}
<INITIAL,ATTR>{hex}	{
				parser_lval.num = xstrtoul(yytext, NULL, 0);
				return aHEXNUM;
			}
<INITIAL,ATTR>{int}	{
				parser_lval.num = xstrtoul(yytext, NULL, 0);
				return aNUM;
			}
<INITIAL>{double}	{
				parser_lval.dbl = strtod(yytext, NULL);
				return aDOUBLE;
			}
SAFEARRAY{ws}*/\(	return tSAFEARRAY;
{cident}		return kw_token(yytext);
<INITIAL,ATTR>\n	line_number++;
<INITIAL,ATTR>{ws}
<INITIAL,ATTR>\<\<	return SHL;
<INITIAL,ATTR>\>\>	return SHR;
<INITIAL,ATTR>\-\>	return MEMBERPTR;
<INITIAL,ATTR>==	return EQUALITY;
<INITIAL,ATTR>!=	return INEQUALITY;
<INITIAL,ATTR>\>=	return GREATEREQUAL;
<INITIAL,ATTR>\<=	return LESSEQUAL;
<INITIAL,ATTR>\|\|	return LOGICALOR;
<INITIAL,ATTR>&&	return LOGICALAND;
<INITIAL,ATTR>\.\.\.	return ELLIPSIS;
<INITIAL,ATTR>.		return yytext[0];
<<EOF>>			{
                            if (import_stack_ptr)
                                return aEOF;
                            if (acf_name)
                            {
                                switch_to_acf();
                                return aACF;
                            }
                            yyterminate();
			}
%%

#ifndef parser_wrap
int parser_wrap(void)
{
	return 1;
}
#endif

struct keyword {
	const char *kw;
	int token;
	int winrt_only : 1;
};

/* This table MUST be alphabetically sorted on the kw field */
static const struct keyword keywords[] = {
	{"FALSE",           tFALSE,          0},
	{"NULL",            tNULL,           0},
	{"TRUE",            tTRUE,           0},
	{"__cdecl",         tCDECL,          0},
	{"__fastcall",      tFASTCALL,       0},
	{"__int32",         tINT32,          0},
	{"__int3264",       tINT3264,        0},
	{"__int64",         tINT64,          0},
	{"__pascal",        tPASCAL,         0},
	{"__stdcall",       tSTDCALL,        0},
	{"_cdecl",          tCDECL,          0},
	{"_fastcall",       tFASTCALL,       0},
	{"_pascal",         tPASCAL,         0},
	{"_stdcall",        tSTDCALL,        0},
	{"apicontract",     tAPICONTRACT,    1},
	{"boolean",         tBOOLEAN,        0},
	{"byte",            tBYTE,           0},
	{"case",            tCASE,           0},
	{"cdecl",           tCDECL,          0},
	{"char",            tCHAR,           0},
	{"coclass",         tCOCLASS,        0},
	{"const",           tCONST,          0},
	{"cpp_quote",       tCPPQUOTE,       0},
	{"default",         tDEFAULT,        0},
	{"dispinterface",   tDISPINTERFACE,  0},
	{"double",          tDOUBLE,         0},
	{"enum",            tENUM,           0},
	{"error_status_t",  tERRORSTATUST,   0},
	{"extern",          tEXTERN,         0},
	{"float",           tFLOAT,          0},
	{"handle_t",        tHANDLET,        0},
	{"hyper",           tHYPER,          0},
	{"import",          tIMPORT,         0},
	{"importlib",       tIMPORTLIB,      0},
	{"inline",          tINLINE,         0},
	{"int",             tINT,            0},
	{"interface",       tINTERFACE,      0},
	{"library",         tLIBRARY,        0},
	{"long",            tLONG,           0},
	{"methods",         tMETHODS,        0},
	{"module",          tMODULE,         0},
	{"namespace",       tNAMESPACE,      1},
	{"pascal",          tPASCAL,         0},
	{"properties",      tPROPERTIES,     0},
	{"register",        tREGISTER,       0},
	{"short",           tSHORT,          0},
	{"signed",          tSIGNED,         0},
	{"sizeof",          tSIZEOF,         0},
	{"small",           tSMALL,          0},
	{"static",          tSTATIC,         0},
	{"stdcall",         tSTDCALL,        0},
	{"struct",          tSTRUCT,         0},
	{"switch",          tSWITCH,         0},
	{"typedef",         tTYPEDEF,        0},
	{"union",           tUNION,          0},
	{"unsigned",        tUNSIGNED,       0},
	{"void",            tVOID,           0},
	{"wchar_t",         tWCHAR,          0},
};
#define NKEYWORDS (sizeof(keywords)/sizeof(keywords[0]))

/* keywords only recognized in attribute lists
 * This table MUST be alphabetically sorted on the kw field
 */
static const struct keyword attr_keywords[] =
{
	{"aggregatable",                tAGGREGATABLE,              0},
	{"agile",                       tAGILE,                     1},
	{"all_nodes",                   tALLNODES,                  0},
	{"allocate",                    tALLOCATE,                  0},
	{"annotation",                  tANNOTATION,                0},
	{"apartment",                   tAPARTMENT,                 0},
	{"appobject",                   tAPPOBJECT,                 0},
	{"async",                       tASYNC,                     0},
	{"async_uuid",                  tASYNCUUID,                 0},
	{"auto_handle",                 tAUTOHANDLE,                0},
	{"bindable",                    tBINDABLE,                  0},
	{"both",                        tBOTH,                      0},
	{"broadcast",                   tBROADCAST,                 0},
	{"byte_count",                  tBYTECOUNT,                 0},
	{"call_as",                     tCALLAS,                    0},
	{"callback",                    tCALLBACK,                  0},
	{"code",                        tCODE,                      0},
	{"comm_status",                 tCOMMSTATUS,                0},
	{"context_handle",              tCONTEXTHANDLE,             0},
	{"context_handle_noserialize",  tCONTEXTHANDLENOSERIALIZE,  0},
	{"context_handle_serialize",    tCONTEXTHANDLENOSERIALIZE,  0},
	{"contract",                    tCONTRACT,                  1},
	{"contractversion",             tCONTRACTVERSION,           1},
	{"control",                     tCONTROL,                   0},
	{"custom",                      tCUSTOM,                    0},
	{"decode",                      tDECODE,                    0},
	{"defaultbind",                 tDEFAULTBIND,               0},
	{"defaultcollelem",             tDEFAULTCOLLELEM,           0},
	{"defaultvalue",                tDEFAULTVALUE,              0},
	{"defaultvtable",               tDEFAULTVTABLE,             0},
	{"disable_consistency_check",   tDISABLECONSISTENCYCHECK,   0},
	{"displaybind",                 tDISPLAYBIND,               0},
	{"dllname",                     tDLLNAME,                   0},
	{"dont_free",                   tDONTFREE,                  0},
	{"dual",                        tDUAL,                      0},
	{"enable_allocate",             tENABLEALLOCATE,            0},
	{"encode",                      tENCODE,                    0},
	{"endpoint",                    tENDPOINT,                  0},
	{"entry",                       tENTRY,                     0},
	{"explicit_handle",             tEXPLICITHANDLE,            0},
	{"fault_status",                tFAULTSTATUS,               0},
	{"force_allocate",              tFORCEALLOCATE,             0},
	{"free",                        tFREE,                      0},
	{"handle",                      tHANDLE,                    0},
	{"helpcontext",                 tHELPCONTEXT,               0},
	{"helpfile",                    tHELPFILE,                  0},
	{"helpstring",                  tHELPSTRING,                0},
	{"helpstringcontext",           tHELPSTRINGCONTEXT,         0},
	{"helpstringdll",               tHELPSTRINGDLL,             0},
	{"hidden",                      tHIDDEN,                    0},
	{"id",                          tID,                        0},
	{"idempotent",                  tIDEMPOTENT,                0},
	{"ignore",                      tIGNORE,                    0},
	{"iid_is",                      tIIDIS,                     0},
	{"immediatebind",               tIMMEDIATEBIND,             0},
	{"implicit_handle",             tIMPLICITHANDLE,            0},
	{"in",                          tIN,                        0},
	{"in_line",                     tIN_LINE,                   0},
	{"input_sync",                  tINPUTSYNC,                 0},
	{"lcid",                        tLCID,                      0},
	{"length_is",                   tLENGTHIS,                  0},
	{"licensed",                    tLICENSED,                  0},
	{"local",                       tLOCAL,                     0},
	{"marshaling_behavior",         tMARSHALINGBEHAVIOR,        1},
	{"maybe",                       tMAYBE,                     0},
	{"message",                     tMESSAGE,                   0},
	{"neutral",                     tNEUTRAL,                   0},
	{"nocode",                      tNOCODE,                    0},
	{"nonbrowsable",                tNONBROWSABLE,              0},
	{"noncreatable",                tNONCREATABLE,              0},
	{"none",                        tNONE,                      1},
	{"nonextensible",               tNONEXTENSIBLE,             0},
	{"notify",                      tNOTIFY,                    0},
	{"notify_flag",                 tNOTIFYFLAG,                0},
	{"object",                      tOBJECT,                    0},
	{"odl",                         tODL,                       0},
	{"oleautomation",               tOLEAUTOMATION,             0},
	{"optimize",                    tOPTIMIZE,                  0},
	{"optional",                    tOPTIONAL,                  0},
	{"out",                         tOUT,                       0},
	{"partial_ignore",              tPARTIALIGNORE,             0},
	{"pointer_default",             tPOINTERDEFAULT,            0},
	{"progid",                      tPROGID,                    0},
	{"propget",                     tPROPGET,                   0},
	{"propput",                     tPROPPUT,                   0},
	{"propputref",                  tPROPPUTREF,                0},
	{"proxy",                       tPROXY,                     0},
	{"ptr",                         tPTR,                       0},
	{"public",                      tPUBLIC,                    0},
	{"range",                       tRANGE,                     0},
	{"readonly",                    tREADONLY,                  0},
	{"ref",                         tREF,                       0},
	{"represent_as",                tREPRESENTAS,               0},
	{"requestedit",                 tREQUESTEDIT,               0},
	{"restricted",                  tRESTRICTED,                0},
	{"retval",                      tRETVAL,                    0},
	{"single",                      tSINGLE,                    0},
	{"single_node",                 tSINGLENODE,                0},
	{"size_is",                     tSIZEIS,                    0},
	{"source",                      tSOURCE,                    0},
	{"standard",                    tSTANDARD,                  1},
	{"strict_context_handle",       tSTRICTCONTEXTHANDLE,       0},
	{"string",                      tSTRING,                    0},
	{"switch_is",                   tSWITCHIS,                  0},
	{"switch_type",                 tSWITCHTYPE,                0},
	{"threading",                   tTHREADING,                 0},
	{"transmit_as",                 tTRANSMITAS,                0},
	{"uidefault",                   tUIDEFAULT,                 0},
	{"unique",                      tUNIQUE,                    0},
	{"user_marshal",                tUSERMARSHAL,               0},
	{"usesgetlasterror",            tUSESGETLASTERROR,          0},
	{"uuid",                        tUUID,                      0},
	{"v1_enum",                     tV1ENUM,                    0},
	{"vararg",                      tVARARG,                    0},
	{"version",                     tVERSION,                   0},
	{"vi_progid",                   tVIPROGID,                  0},
	{"wire_marshal",                tWIREMARSHAL,               0},
};

/* attributes TODO:
    first_is
    last_is
    max_is
    min_is
*/

#define KWP(p) ((const struct keyword *)(p))

static int kw_cmp_func(const void *s1, const void *s2)
{
	return strcmp(KWP(s1)->kw, KWP(s2)->kw);
}

static int kw_token(const char *kw)
{
	struct keyword key, *kwp;
	key.kw = kw;
	kwp = bsearch(&key, keywords, NKEYWORDS, sizeof(keywords[0]), kw_cmp_func);
	if (kwp && (!kwp->winrt_only || winrt_mode)) {
		parser_lval.str = xstrdup(kwp->kw);
		return kwp->token;
	}
	parser_lval.str = xstrdup(kw);
	return is_type(kw) ? aKNOWNTYPE : is_namespace(kw) ? aNAMESPACE : aIDENTIFIER;
}

static int attr_token(const char *kw)
{
        struct keyword key, *kwp;
        key.kw = kw;
        kwp = bsearch(&key, attr_keywords, sizeof(attr_keywords)/sizeof(attr_keywords[0]),
                      sizeof(attr_keywords[0]), kw_cmp_func);
        if (kwp && (!kwp->winrt_only || winrt_mode)) {
            parser_lval.str = xstrdup(kwp->kw);
            return kwp->token;
        }
        return kw_token(kw);
}

static void addcchar(char c)
{
	if(cbufidx >= cbufalloc)
	{
		cbufalloc += 1024;
		cbuffer = xrealloc(cbuffer, cbufalloc * sizeof(cbuffer[0]));
		if(cbufalloc > 65536)
			parser_warning("Reallocating string buffer larger than 64kB\n");
	}
	cbuffer[cbufidx++] = c;
}

static char *get_buffered_cstring(void)
{
	addcchar(0);
	return xstrdup(cbuffer);
}

void pop_import(void)
{
	int ptr = import_stack_ptr-1;

	fclose(yyin);
	yy_delete_buffer( YY_CURRENT_BUFFER );
	yy_switch_to_buffer( import_stack[ptr].state );
	if (temp_name) {
		unlink(temp_name);
		free(temp_name);
	}
	temp_name = import_stack[ptr].temp_name;
	input_name = import_stack[ptr].input_name;
	line_number = import_stack[ptr].line_number;
	import_stack_ptr--;
}

struct imports {
    char *name;
    struct imports *next;
} *first_import;

int do_import(char *fname)
{
    FILE *f;
    char *path, *name;
    struct imports *import;
    int ptr = import_stack_ptr;
    int ret, fd;

    import = first_import;
    while (import && strcmp(import->name, fname))
        import = import->next;
    if (import) return 0; /* already imported */

    import = xmalloc(sizeof(struct imports));
    import->name = xstrdup(fname);
    import->next = first_import;
    first_import = import;

    /* don't search for a file name with a path in the include directories,
     * for compatibility with MIDL */
    if (strchr( fname, '/' ) || strchr( fname, '\\' ))
        path = xstrdup( fname );
    else if (!(path = wpp_find_include( fname, input_name )))
        error_loc("Unable to open include file %s\n", fname);

    if (import_stack_ptr == MAX_IMPORT_DEPTH)
        error_loc("Exceeded max import depth\n");

    import_stack[ptr].temp_name = temp_name;
    import_stack[ptr].input_name = input_name;
    import_stack[ptr].line_number = line_number;
    import_stack_ptr++;
    input_name = path;
    line_number = 1;

    name = xstrdup( "widl.XXXXXX" );
    if((fd = mkstemps( name, 0 )) == -1)
        error("Could not generate a temp name from %s\n", name);

    temp_name = name;
    if (!(f = fdopen(fd, "wt")))
        error("Could not open fd %s for writing\n", name);

    ret = wpp_parse( path, f );
    fclose( f );
    if (ret) exit(1);

    if((f = fopen(temp_name, "r")) == NULL)
        error_loc("Unable to open %s\n", temp_name);

    import_stack[ptr].state = YY_CURRENT_BUFFER;
    yy_switch_to_buffer(yy_create_buffer(f, YY_BUF_SIZE));
    return 1;
}

void abort_import(void)
{
	int ptr;

	for (ptr=0; ptr<import_stack_ptr; ptr++)
		unlink(import_stack[ptr].temp_name);
}

static void switch_to_acf(void)
{
    int ptr = import_stack_ptr;
    int ret, fd;
    char *name;
    FILE *f;

    assert(import_stack_ptr == 0);

    input_name = acf_name;
    acf_name = NULL;
    line_number = 1;

    name = xstrdup( "widl.XXXXXX" );
    if((fd = mkstemps( name, 0 )) == -1)
        error("Could not generate a temp name from %s\n", name);

    temp_name = name;
    if (!(f = fdopen(fd, "wt")))
        error("Could not open fd %s for writing\n", name);

    ret = wpp_parse(input_name, f);
    fclose(f);
    if (ret) exit(1);

    if((f = fopen(temp_name, "r")) == NULL)
        error_loc("Unable to open %s\n", temp_name);

    import_stack[ptr].state = YY_CURRENT_BUFFER;
    yy_switch_to_buffer(yy_create_buffer(f, YY_BUF_SIZE));
}

static void warning_disable(int warning)
{
    warning_t *warning_entry;
    LIST_FOR_EACH_ENTRY(warning_entry, disabled_warnings, warning_t, entry)
        if(warning_entry->num == warning)
            return;
    warning_entry = xmalloc( sizeof(*warning_entry) );
    warning_entry->num = warning;
    list_add_tail(disabled_warnings, &warning_entry->entry);
}

static void warning_enable(int warning)
{
    warning_t *warning_entry;
    LIST_FOR_EACH_ENTRY(warning_entry, disabled_warnings, warning_t, entry)
        if(warning_entry->num == warning)
        {
            list_remove(&warning_entry->entry);
            free(warning_entry);
            break;
        }
}

int do_warning(char *toggle, warning_list_t *wnum)
{
    warning_t *warning, *next;
    int ret = 1;
    if(!disabled_warnings)
    {
        disabled_warnings = xmalloc( sizeof(*disabled_warnings) );
        list_init( disabled_warnings );
    }

    if(!strcmp(toggle, "disable"))
        LIST_FOR_EACH_ENTRY(warning, wnum, warning_t, entry)
            warning_disable(warning->num);
    else if(!strcmp(toggle, "enable"))
        LIST_FOR_EACH_ENTRY(warning, wnum, warning_t, entry)
            warning_enable(warning->num);
    else
        ret = 0;

    LIST_FOR_EACH_ENTRY_SAFE(warning, next, wnum, warning_t, entry)
        free(warning);
    return ret;
}

int is_warning_enabled(int warning)
{
    warning_t *warning_entry;
    if(!disabled_warnings)
        return 1;
    LIST_FOR_EACH_ENTRY(warning_entry, disabled_warnings, warning_t, entry)
        if(warning_entry->num == warning)
            return 0;
    return 1;
}
