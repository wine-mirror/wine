/* -*-C-*-
 * Lexical scanner for command line parsing
 *
 * Copyright 1993 Eric Youngdale
 *           2000 Eric Pouech
 */

%{
#include <stdlib.h>
#include <string.h>
#include "debugger.h"
#include "y.tab.h"

#ifndef DONT_USE_READLINE
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = dbg_read((char *) buf, max_size )) < 0 ) \
	    YY_FATAL_ERROR( "read() in flex scanner failed" );

static int dbg_read(char * buf, int size);
static char * DEBUG_MakeSymbol(char *);

#endif  /* DONT_USE_READLINE */

#define YY_NO_UNPUT

static int syntax_error;
%}

DIGIT	   [0-9]
HEXDIGIT   [0-9a-fA-F]
FORMAT     [ubcdiswx]
IDENTIFIER [_a-zA-Z\.~][_a-zA-Z0-9\.~@]*
PATHNAME   [/_a-zA-Z\.~][/_a-zA-Z0-9\.~@]*
STRING     \"[^\n"]+\"

%s FORMAT_EXPECTED
%s PATH_EXPECTED
%s INFO_CMD
%s HELP_CMD
%s DEL_CMD
%s WALK_CMD
%s SHOW_CMD
%s NOCMD

%x ASTRING_EXPECTED
%x NOPROCESS
%%
                                        /* set to special state when no process is loaded. */
                                        if (!DEBUG_CurrProcess && YYSTATE == INITIAL) {BEGIN(NOPROCESS);}

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
                                          yylval.integer = strtol( yytext+1, &last, NULL ) << 8;
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
<INITIAL>help|hel|he|"?"		{ BEGIN(HELP_CMD); return tHELP; }

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

<INITIAL>mode				{ BEGIN(NOCMD); return tMODE; }
<INITIAL>show|sho|sh			{ BEGIN(SHOW_CMD); return tSHOW; }
<INITIAL>symbolfile|symbols|symbol|sf   { BEGIN(PATH_EXPECTED); return tSYMBOLFILE; }

<INITIAL,INFO_CMD,DEL_CMD>break|brea|bre|br|b	{ BEGIN(PATH_EXPECTED); return tBREAK; }
<INITIAL>watch|watc|wat			{ BEGIN(PATH_EXPECTED); return tWATCH; }
<INITIAL>whatis|whati|what		{ BEGIN(PATH_EXPECTED); return tWHATIS; }
<INITIAL,NOPROCESS>run|ru|r     	{ BEGIN(ASTRING_EXPECTED); return tRUN;}
<NOPROCESS>attach|attac|atta|att 	{ BEGIN(NOCMD); return tATTACH; }
<INFO_CMD>share|shar|sha		{ return tSHARE; }
<INFO_CMD>locals|local|loca|loc		{ return tLOCAL; }
<INFO_CMD,WALK_CMD>class|clas|cla       { return tCLASS; }
<INFO_CMD,WALK_CMD>module|modul|modu|mod { return tMODULE; }
<INFO_CMD,WALK_CMD>queue|queu|que	{ return tQUEUE; }
<INFO_CMD,WALK_CMD>process|proces|proce|proc   		{ return tPROCESS; }
<INFO_CMD,WALK_CMD>threads|thread|threa|thre|thr|th { return tTHREAD; }
<INFO_CMD,WALK_CMD>modref|modre|modr	{ return tMODREF; }
<INFO_CMD>registers|regs|reg|re		{ return tREGS; }
<INFO_CMD>segments|segment|segm|seg|se	{ return tSEGMENTS; }
<INFO_CMD>stack|stac|sta|st     	{ return tSTACK; }
<INFO_CMD>maps|map			{ return tMAPS; }
<INFO_CMD,WALK_CMD>window|windo|wind|win|wnd	{ return tWND; }
<HELP_CMD>info|inf|in                   { return tINFO; }

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

#ifndef DONT_USE_READLINE

#ifndef whitespace
#define whitespace(c) (((c) == ' ') || ((c) == '\t'))
#endif


/* Strip whitespace from the start and end of STRING. */
static void stripwhite (char *string)
{
  register int i = 0;

  while (whitespace (string[i]))
    i++;

  if (i)
    strcpy (string, string + i);

  i = strlen (string) - 1;

  while (i > 0 && whitespace (string[i]))
    i--;

  string[++i] = '\0';
}

static int dbg_read(char * buf, int size)
{
    static char last_line[256] = "";
    char * line;
    int len;
    
    for (;;)
    {
        DEBUG_FlushSymbols();
        line = readline ("Wine-dbg>");
        if (!line)
        {
            DEBUG_Printf( DBG_CHN_MESG, "\n" );
            DEBUG_Exit(0);
        }

        /* Remove leading and trailing whitespace from the line */

        stripwhite (line);

        /* If there is anything left, add it to the history list
           and execute it. Otherwise, re-execute last command. */

        if (*line)
        {
            add_history( line );
            strncpy( last_line, line, 255 );
            last_line[255] = '\0'; 
        }

        free( line );
        line = last_line;

        if ((len = strlen(line)) > 0)
        {
            if (size < len + 1)
            {
                DEBUG_Printf(DBG_CHN_MESG,"Fatal readline goof.\n");
		DEBUG_Exit(0);
            }
            strcpy(buf, line);
            buf[len] = '\n';
            buf[len+1] = 0;
            return len + 1;
        }
    }
}

static char *local_symbols[30];
static int next_symbol;

static char * DEBUG_MakeSymbol(char * symbol)
{
        assert(0 <= next_symbol && next_symbol < (sizeof(local_symbols) / sizeof(local_symbols[0])));
	return local_symbols[next_symbol++] = DBG_strdup(symbol);
}

void DEBUG_FlushSymbols(void)
{
	while(--next_symbol >= 0) DBG_free(local_symbols[next_symbol]);
	next_symbol = 0;
}

#endif  /* DONT_USE_READLINE */
