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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

%option noinput nounput always-interactive 8bit prefix="dbg_"

%{
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define YY_NO_UNISTD_H
#include "debugger.h"
#include "dbg.tab.h"

#undef YY_INPUT

static char** local_lexemes /* = NULL */;
static int next_lexeme /* = 0 */;
static int alloc_lexeme /* = 0 */;

char* lexeme_alloc_size(int size)
{
    assert(0 <= next_lexeme && next_lexeme < alloc_lexeme + 1);
    if (next_lexeme >= alloc_lexeme)
    {
        alloc_lexeme += 32;
        local_lexemes = dbg_heap_realloc(local_lexemes, alloc_lexeme * sizeof(local_lexemes[0]));
        assert(local_lexemes);
    }
    return local_lexemes[next_lexeme++] = HeapAlloc(GetProcessHeap(), 0, size + 1);
}

static char* lexeme_alloc(const char* lexeme)
{
    char*       ptr = lexeme_alloc_size(strlen(lexeme) + 1);
    return strcpy(ptr, lexeme);
}

void lexeme_flush(void)
{
    while (--next_lexeme >= 0) HeapFree(GetProcessHeap(), 0, local_lexemes[next_lexeme]);
    next_lexeme = 0;
}

static size_t read_input(const char* pfx, char* buf, int size)
{
    int len;
    static char*  last_line = NULL;
    static size_t last_line_idx = 0;

    /* try first to fetch the remaining of an existing line */
    if (last_line_idx == 0)
    {
        char* tmp = NULL;
        /* no remaining chars to be read from last line, grab a brand new line up to '\n' */
        lexeme_flush();
        len = input_fetch_entire_line(pfx, &tmp);
        if (len < 0) return 0;  /* eof */

        /* remove carriage return in newline */
        if (len >= 2 && tmp[len - 2] == '\r')
        {
            tmp[len - 2] = '\n';
            tmp[len - 1] = '\0';
            len--;
        }

        /* FIXME: should have a pair of buffers, and switch between the two, instead of
         * reallocating a new one for each line
         */
        if (last_line && (len == 0 || (len == 1 && tmp[0] == '\n')))
        {
            HeapFree(GetProcessHeap(), 0, tmp);
        }
        else
        {
            HeapFree(GetProcessHeap(), 0, last_line);
            last_line = tmp;
        }
    }

    len = min(strlen(last_line + last_line_idx), size - 1);
    memcpy(buf, last_line + last_line_idx, len);
    buf[len] = '\0';
    if ((last_line_idx += len) >= strlen(last_line))
        last_line_idx = 0;
    return len;
}

#define YY_INPUT(buf,result,max_size) \
        (result = read_input("Wine-dbg>", buf, max_size))

static int syntax_error;
%}

OCTDIGIT   [0-7]
DIGIT	   [0-9]
HEXDIGIT   [0-9a-fA-F]
FORMAT     [ubcdgiswxa]
IDENTIFIER [_a-zA-Z~?][_a-zA-Z0-9~?@]*
PATHNAME   [\\/_a-zA-Z0-9\.~@][\\/\-_a-zA-Z0-9\.~@]*
STRING     \"[^\n"]+\"

%s FORMAT_EXPECTED
%s INFO_CMD
%s HELP_CMD
%s BD_CMD
%s LOCAL_CMD
%s SHOW_CMD
%s MODE_CMD
%s MAINT_CMD
%s NOCMD
%s PATH_ACCEPTED

%x PATH_EXPECTED
%x ASTRING_EXPECTED
%x NOPROCESS
%%
                                        /* set to special state when no process is loaded. */
                                        if (!dbg_num_processes() && YYSTATE == INITIAL) {BEGIN(NOPROCESS);}

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
"::"					{ return OP_SCOPE; }
"["					{ return *yytext; }
"]"					{ return *yytext; }

"0x"{HEXDIGIT}+      			{ sscanf(yytext, "%lx", &dbg_lval.integer); return tNUM; }
{DIGIT}+             			{ sscanf(yytext, "%ld", &dbg_lval.integer); return tNUM; }
"'\\''"                                 { dbg_lval.integer = '\''; return tNUM;}
"'\\0"{OCTDIGIT}*"'"                    { sscanf(yytext + 3, "%lo", &dbg_lval.integer); return tNUM;}
"'\\x"{HEXDIGIT}+"'"                    { sscanf(yytext + 3, "%lx", &dbg_lval.integer); return tNUM;}
"'\\"[a-z]"'"                           { dbg_lval.integer = yytext[2] - 'a'; return tNUM;}
"'"."'"                                 { dbg_lval.integer = yytext[1]; return tNUM;}

<FORMAT_EXPECTED>"/"{DIGIT}+{FORMAT}	{ char* last;
                                          dbg_lval.integer = strtol(yytext+1, &last, 0) << 8;
                                          dbg_lval.integer |= *last;
                                          return tFORMAT; }

<FORMAT_EXPECTED>"/"{FORMAT}          	{ dbg_lval.integer = (1 << 8) | yytext[1]; return tFORMAT; }

{STRING} 				{ dbg_lval.string = lexeme_alloc(yytext + 1); dbg_lval.string[strlen(dbg_lval.string) - 1] = '\0'; return tSTRING; }
<ASTRING_EXPECTED>[^\n]+                { char* p = yytext; while (*p == ' ' || *p == '\t') p++;
                                          dbg_lval.string = lexeme_alloc(p); return tSTRING; }

<INITIAL,NOPROCESS>info|inf|in		{ BEGIN(INFO_CMD); return tINFO; }
<INITIAL>up				{ BEGIN(NOCMD); return tUP; }
<INITIAL>down|dow|do			{ BEGIN(NOCMD); return tDOWN; }
<INITIAL,INFO_CMD>frame|fram|fra|fr	{ BEGIN(NOCMD); return tFRAME; }
<INITIAL>list|lis|li|l			{ BEGIN(PATH_ACCEPTED); return tLIST; }
<INITIAL>enable|enabl|enab|ena		{ BEGIN(BD_CMD); return tENABLE;}
<INITIAL>disable|disabl|disab|disa|dis  { BEGIN(BD_CMD); return tDISABLE; }
<INITIAL>disassemble|disassembl|disassemb|disassem|disasse|disass|disas { BEGIN(NOCMD); return tDISASSEMBLE; }
<INITIAL>locally|local			{ BEGIN(LOCAL_CMD); return tLOCAL; }
<INITIAL,LOCAL_CMD>display|displa|displ|disp	{ BEGIN(FORMAT_EXPECTED); return tDISPLAY; }
<INFO_CMD,BD_CMD>display|displa|displ|disp|dis|di|d	{ BEGIN(NOCMD); return tDISPLAY; }
<INITIAL>undisplay|undispla|undispl|undisp|undis|undi|und	{ BEGIN(NOCMD); return tUNDISPLAY; }
<INITIAL>delete|delet|dele|del		{ BEGIN(BD_CMD); return tDELETE; }
<INITIAL>thread|threa|thre|thr|th	{ BEGIN(NOCMD); return tTHREAD; }
<INITIAL,NOPROCESS>quit|qui|qu|q	{ BEGIN(NOCMD); return tQUIT; }
<INITIAL>set|se				{ BEGIN(NOCMD); return tSET; }
<INITIAL>x				{ BEGIN(FORMAT_EXPECTED); return tEXAM; }
<INITIAL,NOPROCESS>help|hel|he|"?"	{ BEGIN(HELP_CMD); return tHELP; }

<INITIAL,NOPROCESS>backtrace|backtrac|backtra|backt|back|bac|ba|bt { BEGIN(NOCMD); return tBACKTRACE; }
<INITIAL,NOPROCESS>where|wher|whe       { BEGIN(NOCMD); return tBACKTRACE; }

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

<INITIAL>show|sho|sh			{ BEGIN(SHOW_CMD); return tSHOW; }
<INITIAL,NOPROCESS>source|sourc|sour|src { BEGIN(PATH_EXPECTED); return tSOURCE; }
<INITIAL>symbolfile|symbols|symbol|sf   { BEGIN(PATH_EXPECTED); return tSYMBOLFILE; }

<INITIAL,INFO_CMD,BD_CMD>break|brea|bre|br|b	{ BEGIN(PATH_ACCEPTED); return tBREAK; }
<INITIAL,INFO_CMD,BD_CMD>hbreak|hbrea|hbre|hbr|hb { BEGIN(PATH_ACCEPTED); return tHBREAK; }
<INITIAL>watch|watc|wat			{ BEGIN(NOCMD); return tWATCH; }
<INITIAL>rwatch|rwatc|rwat		{ BEGIN(NOCMD); return tRWATCH; }
<INITIAL>whatis|whati|what		{ BEGIN(NOCMD); return tWHATIS; }
<INITIAL,NOPROCESS>run|ru|r     	{ BEGIN(ASTRING_EXPECTED); return tRUN;}
<INITIAL>detach|detac|deta|det   	{ BEGIN(NOCMD); return tDETACH; }
<INITIAL>kill|kil|ki|k                  { BEGIN(NOCMD); return tKILL; }
<INITIAL,NOPROCESS>maintenance|maint    { BEGIN(MAINT_CMD); return tMAINTENANCE; }
<INITIAL>minidump|mdmp                  { BEGIN(PATH_EXPECTED); return tMINIDUMP; }
<INITIAL>echo				{ BEGIN(ASTRING_EXPECTED); return tECHO; }
<NOPROCESS>attach|attac|atta|att 	{ BEGIN(NOCMD); return tATTACH; }
<INFO_CMD>share|shar|sha                { return tSHARE; }
<MAINT_CMD>module|modul|mod             { BEGIN(ASTRING_EXPECTED); return tMODULE; }
<INFO_CMD>locals|local|loca|loc		{ return tLOCAL; }
<INFO_CMD>class|clas|cla                { return tCLASS; }
<INFO_CMD>process|proces|proce|proc   	{ return tPROCESS; }
<INFO_CMD>threads|thread|threa|thre|thr|th { return tTHREAD; }
<INFO_CMD>exception|except|exc|ex	{ return tEXCEPTION; }
<INFO_CMD>registers|regs|reg|re		{ return tREGS; }
<INFO_CMD>allregs|allreg|allre          { return tALLREGS; }
<INFO_CMD>"all-registers"|"all-regs"|"all-reg"|"all-re" { return tALLREGS; }
<INFO_CMD>segments|segment|segm|seg|se	{ return tSEGMENTS; }
<INFO_CMD>stack|stac|sta|st     	{ return tSTACK; }
<INFO_CMD>symbol|symbo|symb|sym         { BEGIN(ASTRING_EXPECTED); return tSYMBOL; }
<INFO_CMD>maps|map			{ return tMAPS; }
<INFO_CMD>window|windo|wind|win|wnd	{ return tWND; }
<HELP_CMD>info|inf|in                   { return tINFO; }
<MAINT_CMD>type                         { return tTYPE; }

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
all                                     { return tALL; }

{IDENTIFIER}				{ dbg_lval.string = lexeme_alloc(yytext); return tIDENTIFIER; }
"$"{IDENTIFIER}				{ dbg_lval.string = lexeme_alloc(yytext+1); return tINTVAR; }

<PATH_EXPECTED,PATH_ACCEPTED>{PATHNAME}	{ dbg_lval.string = lexeme_alloc(yytext); return tPATH; }

[-+<=>|&^()*/%:!~,\.]			{ return *yytext; }

<*>[ \t\r]+                             /* Eat up whitespace and DOS LF */

<NOPROCESS>.                            { BEGIN(ASTRING_EXPECTED); yyless(0); return tNOPROCESS;}
<*>.                                    { if (syntax_error == 0) { syntax_error++; dbg_printf("Syntax Error (%s)\n", yytext); } }
%%

#ifndef dbg_wrap
int dbg_wrap(void) { return 1; }
#endif
