/* -*-C-*-
 * Lexical scanner for command line parsing
 *
 * Copyright 1993 Eric Youngdale
 *           2000 Eric Pouech
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

%{
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wincon.h"
#include "debugger.h"
#include "y.tab.h"

#undef YY_INPUT

HANDLE DEBUG_hParserInput;
HANDLE DEBUG_hParserOutput;

static int DEBUG_FetchFromLine(const char* pfx, char* buf, int size);

#define YY_INPUT(buf,result,max_size) \
	if ( (result = DEBUG_FetchFromLine("Wine-dbg>", buf, max_size)) < 0 ) \
	    YY_FATAL_ERROR( "FetchFromLine() in flex scanner failed" );

#define YY_NO_UNPUT

static int syntax_error;
%}

DIGIT	   [0-9]
HEXDIGIT   [0-9a-fA-F]
FORMAT     [ubcdgiswx]
IDENTIFIER [_a-zA-Z~][_a-zA-Z0-9~@]*
PATHNAME   [/_a-zA-Z\.~][/_a-zA-Z0-9\.~@]*
STRING     \"[^\n"]+\"

%s FORMAT_EXPECTED
%s PATH_EXPECTED
%s INFO_CMD
%s HELP_CMD
%s DEL_CMD
%s WALK_CMD
%s SHOW_CMD
%s MODE_CMD
%s NOCMD

%x ASTRING_EXPECTED
%x NOPROCESS
%%
                                        /* set to special state when no process is loaded. */
                                        if (!DEBUG_CurrProcess && YYSTATE == INITIAL) {BEGIN(NOPROCESS);}

<<EOF>>                                 { return tEOF; }
<*>\n		                        { BEGIN(INITIAL); syntax_error = 0; return tEOL; }
                                        /* Indicates end of command. Reset state. */

"||"					{ return OP_LOR; }
"&&"					{ return OP_LAND; }
"=="					{ return OP_EQ; }
"!="					{ return OP_NE; }
"<="					{ return OP_LE; }
">="					{ return OP_GE; }
"<<"					{ return OP_SHL; }
">>"					{ return OP_SHR; }
"->"					{ return OP_DRF; }
[-+<=>|&^()*/%:!~,\.]			{ return *yytext; }
"["					{ return *yytext; }
"]"					{ return *yytext; }

"0x"{HEXDIGIT}+      			{ sscanf(yytext, "%x", &yylval.integer); return tNUM; }
{DIGIT}+             			{ sscanf(yytext, "%d", &yylval.integer); return tNUM; }

<FORMAT_EXPECTED>"/"{DIGIT}+{FORMAT}	{ char* last;
                                          yylval.integer = strtol( yytext+1, &last, 0 ) << 8;
                                          yylval.integer |= *last;
                                          return tFORMAT; }

<FORMAT_EXPECTED>"/"{FORMAT}          	{ yylval.integer = (1 << 8) | yytext[1]; return tFORMAT; }

{STRING} 				{ yylval.string = DEBUG_MakeSymbol(yytext); return tSTRING; }
<ASTRING_EXPECTED>[^\n]+                { char* p = yytext; while (*p == ' ' || *p == '\t') p++;
                                          yylval.string = DEBUG_MakeSymbol(p); return tSTRING; }

<INITIAL>info|inf|in			{ BEGIN(INFO_CMD); return tINFO; }
<INITIAL>up				{ BEGIN(NOCMD); return tUP; }
<INITIAL>down|dow|do			{ BEGIN(NOCMD); return tDOWN; }
<INITIAL>frame|fram|fra|fr		{ BEGIN(NOCMD); return tFRAME; }
<INITIAL>list|lis|li|l			{ BEGIN(PATH_EXPECTED); return tLIST; }
<INITIAL>enable|enabl|enab|ena		{ BEGIN(NOCMD); return tENABLE;}
<INITIAL>disable|disabl|disab|disa|dis  { BEGIN(NOCMD); return tDISABLE; }
<INITIAL>disassemble|disassembl|disassemb|disassem|disasse|disass|disas { BEGIN(NOCMD); return tDISASSEMBLE; }
<INITIAL,INFO_CMD,DEL_CMD>display|displa|displ|disp	{ BEGIN(FORMAT_EXPECTED); return tDISPLAY; }
<INITIAL>undisplay|undispla|undispl|undisp|undis|undi|und	{ BEGIN(NOCMD); return tUNDISPLAY; }
<INITIAL>delete|delet|dele|del		{ BEGIN(DEL_CMD); return tDELETE; }
<INITIAL,NOPROCESS>quit|qui|qu|q	{ BEGIN(NOCMD); return tQUIT; }
<INITIAL>set|se				{ BEGIN(NOCMD); return tSET; }
<INITIAL,NOPROCESS>walk|w		{ BEGIN(WALK_CMD); return tWALK; }
<INITIAL>x				{ BEGIN(FORMAT_EXPECTED); return tEXAM; }
<INITIAL,NOPROCESS>help|hel|he|"?"		{ BEGIN(HELP_CMD); return tHELP; }

<INITIAL>backtrace|backtrac|backtra|backt|back|bac|ba|bt { BEGIN(NOCMD); return tBACKTRACE; }
<INITIAL>where|wher|whe                 { BEGIN(NOCMD); return tBACKTRACE; }

<INITIAL>cont|con|co|c   		{ BEGIN(NOCMD); return tCONT; }
<INITIAL>pass|pas|pa   			{ BEGIN(NOCMD); return tPASS; }
<INITIAL>condition|conditio|conditi|condit|condi|cond	{ BEGIN(NOCMD); return tCOND; }
<INITIAL>step|ste|st|s   		{ BEGIN(NOCMD); return tSTEP; }
<INITIAL>next|nex|ne|n   		{ BEGIN(NOCMD); return tNEXT; }
<INITIAL>stepi|si	   		{ BEGIN(NOCMD); return tSTEPI; }
<INITIAL>nexti|ni	   		{ BEGIN(NOCMD); return tNEXTI; }
<INITIAL>finish|finis|fini|fin|fi	{ BEGIN(NOCMD); return tFINISH; }

<INITIAL>abort|abor|abo         	{ BEGIN(NOCMD); return tABORT; }
<INITIAL>print|prin|pri|pr|p		{ BEGIN(FORMAT_EXPECTED); return tPRINT; }

<INITIAL>mode				{ BEGIN(MODE_CMD); return tMODE; }
<INITIAL>show|sho|sh			{ BEGIN(SHOW_CMD); return tSHOW; }
<INITIAL,NOPROCESS>source|sourc|sour|src { BEGIN(PATH_EXPECTED); return tSOURCE; }
<INITIAL>symbolfile|symbols|symbol|sf   { BEGIN(PATH_EXPECTED); return tSYMBOLFILE; }

<INITIAL,INFO_CMD,DEL_CMD>break|brea|bre|br|b	{ BEGIN(NOCMD); return tBREAK; }
<INITIAL>watch|watc|wat			{ BEGIN(NOCMD); return tWATCH; }
<INITIAL>whatis|whati|what		{ BEGIN(NOCMD); return tWHATIS; }
<INITIAL,NOPROCESS>run|ru|r     	{ BEGIN(ASTRING_EXPECTED); return tRUN;}
<INITIAL>detach|detac|deta|det   	{ BEGIN(NOCMD); return tDETACH; }
<NOPROCESS>attach|attac|atta|att 	{ BEGIN(NOCMD); return tATTACH; }
<INFO_CMD>share|shar|sha		{ return tSHARE; }
<INFO_CMD>locals|local|loca|loc		{ return tLOCAL; }
<INFO_CMD,WALK_CMD>class|clas|cla       { return tCLASS; }
<INFO_CMD,WALK_CMD>module|modul|modu|mod { return tMODULE; }
<INFO_CMD,WALK_CMD>process|proces|proce|proc   		{ return tPROCESS; }
<INFO_CMD,WALK_CMD>threads|thread|threa|thre|thr|th { return tTHREAD; }
<WALK_CMD>exception|except|exc|ex	{ return tEXCEPTION; }
<INFO_CMD>registers|regs|reg|re		{ return tREGS; }
<INFO_CMD>segments|segment|segm|seg|se	{ return tSEGMENTS; }
<INFO_CMD>stack|stac|sta|st     	{ return tSTACK; }
<INFO_CMD>symbol|sym                    { BEGIN(ASTRING_EXPECTED); return tSYMBOL; }
<WALK_CMD>maps|map			{ return tMAPS; }
<INFO_CMD,WALK_CMD>window|windo|wind|win|wnd	{ return tWND; }
<HELP_CMD>info|inf|in                   { return tINFO; }
<MODE_CMD>vm86                          { return tVM86; }

<INITIAL,SHOW_CMD>directories|directorie|directori|director|directo|direct|direc|direc|dir {
			                  BEGIN(PATH_EXPECTED); return tDIR; }

char					{ return tCHAR; }
short					{ return tSHORT; }
int					{ return tINT; }
long					{ return tLONG; }
float					{ return tFLOAT; }
double					{ return tDOUBLE; }
unsigned				{ return tUNSIGNED; }
signed					{ return tSIGNED; }
struct					{ return tSTRUCT; }
union					{ return tUNION; }
enum					{ return tENUM; }

{IDENTIFIER}				{ yylval.string = DEBUG_MakeSymbol(yytext); return tIDENTIFIER; }
"$"{IDENTIFIER}				{ yylval.string = DEBUG_MakeSymbol(yytext+1); return tINTVAR; }

<PATH_EXPECTED>{PATHNAME}		{ yylval.string = DEBUG_MakeSymbol(yytext); return tPATH; }

<*>[ \t]+                               /* Eat up whitespace */

<NOPROCESS>.                            { BEGIN(ASTRING_EXPECTED); yyless(0); return tNOPROCESS;}
<*>.                                    { if (syntax_error == 0) {
                                             syntax_error++;
					     DEBUG_Printf(DBG_CHN_MESG, "Syntax Error (%s)\n", yytext); }
		                        }
%%

#ifndef yywrap
int yywrap(void) { return 1; }
#endif

#ifndef whitespace
#define whitespace(c) (((c) == ' ') || ((c) == '\t'))
#endif


/* Strip whitespace from the start and end of STRING. */
static void stripwhite(char *string)
{
    int         i, last;

    for (i = 0; whitespace(string[i]); i++);
    if (i) strcpy(string, string + i);

    last = i = strlen(string) - 1;
    if (string[last] == '\n') i--;

    while (i > 0 && whitespace(string[i])) i--;
    if (string[last] == '\n')
        string[++i] = '\n';
    string[++i] = '\0';
}

static int      DEBUG_FetchEntireLine(const char* pfx, char** line, size_t* alloc, BOOL check_nl)
{
    char 	buf_line[256];
    DWORD	nread;
    size_t      len;

    /* as of today, console handles can be file handles... so better use file APIs rather than
     * consoles
     */
    WriteFile(DEBUG_hParserOutput, pfx, strlen(pfx), NULL, NULL);

    len = 0;
    do
    {
	if (!ReadFile(DEBUG_hParserInput, buf_line, sizeof(buf_line) - 1, &nread, NULL) || nread == 0)
            break;
	buf_line[nread] = '\0';

        if (check_nl && len == 0 && nread == 1 && buf_line[0] == '\n')
            return 0;

        /* store stuff at the end of last_line */
        if (len + nread + 1 > *alloc)
        {
            *line = HeapReAlloc(GetProcessHeap(), 0, *line, *alloc += nread + 1);
        }
        strcpy(*line + len, buf_line);
        len += nread;
    } while (nread == 0 || buf_line[nread - 1] != '\n');

    if (!len)
    {
        *line = HeapReAlloc(GetProcessHeap(), 0, *line, *alloc = 1);
        **line = '\0';
    }

    /* Remove leading and trailing whitespace from the line */
    stripwhite(*line);
    return 1;
}

static int DEBUG_FetchFromLine(const char* pfx, char* buf, int size)
{
    size_t      len;
static char*  last_line = NULL;
static size_t last_line_size = 0;
static size_t last_line_idx = 0;

    /* first alloc of our current buffer */
    if (!last_line)
    {
        last_line = HeapAlloc(GetProcessHeap(), 0, last_line_size = 2);
        assert(last_line);
        last_line[0] = '\n';
        last_line[1] = '\0';
    }

    /* try first to fetch the remaining of an existing line */
    if (last_line_idx == 0)
    {
        /* no remaining chars to be read from last line, grab a brand new line up to '\n' */
        DEBUG_FlushSymbols();

        DEBUG_FetchEntireLine(pfx, &last_line, &last_line_size, TRUE);
    }

    len = min(strlen(last_line + last_line_idx), size - 1);
    memcpy(buf, last_line + last_line_idx, len);
    buf[len] = '\0';
    if ((last_line_idx += len) >= strlen(last_line))
        last_line_idx = 0;
    return len;
}

int DEBUG_ReadLine(const char* pfx, char* buf, int size)
{
    char*       line = NULL;
    size_t      len = 0;

    DEBUG_FetchEntireLine(pfx, &line, &len, FALSE);
    len = strlen(line);
    /* remove trailing \n */
    if (len > 0 && line[len - 1] == '\n') len--;
    len = min(size - 1, len);
    memcpy(buf, line, len);
    buf[len] = '\0';
    HeapFree(GetProcessHeap(), 0, line);
    return 1;
}

static char** local_symbols /* = NULL */;
static int next_symbol /* = 0 */;
static int alloc_symbol /* = 0 */;

char* DEBUG_MakeSymbol(const char* symbol)
{
    assert(0 <= next_symbol && next_symbol < alloc_symbol + 1);
    if (next_symbol >= alloc_symbol)
    {
        local_symbols = HeapReAlloc(GetProcessHeap(), 0, local_symbols,
                                    (alloc_symbol += 32) * sizeof(local_symbols[0]));
        assert(local_symbols);
    }
    return local_symbols[next_symbol++] = DBG_strdup(symbol);
}

void DEBUG_FlushSymbols(void)
{
    while(--next_symbol >= 0) DBG_free(local_symbols[next_symbol]);
    next_symbol = 0;
}
