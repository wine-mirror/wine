/* -*-C-*-
 * Lexical scanner for command line parsing
 *
 * Copyright 1993 Eric Youngdale
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debugger.h"
#include "xmalloc.h"
#include "y.tab.h"

#ifndef DONT_USE_READLINE
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = dbg_read((char *) buf, max_size )) < 0 ) \
	    YY_FATAL_ERROR( "read() in flex scanner failed" );

extern char * readline(char *);
extern void add_history(char *);
static int dbg_read(char * buf, int size);
static char * make_symbol(char *);
void flush_symbols();

#endif  /* DONT_USE_READLINE */

#define YY_NO_UNPUT

static int syntax_error;
%}

DIGIT	   [0-9]
HEXDIGIT   [0-9a-fA-F]
FORMAT     [bcdiswx]
IDENTIFIER [_a-zA-Z\.~][_a-zA-Z0-9\.~]*

%%

\n		{ syntax_error = 0; return tEOL; } /*Indicates end of command*/

"||"		{ return OP_LOR; }
"&&"		{ return OP_LAND; }
"=="		{ return OP_EQ; }
"!="		{ return OP_NE; }
"<="		{ return OP_LE; }
">="		{ return OP_GE; }
"<<"		{ return OP_SHL; }
">>"		{ return OP_SHR; }
[-+<=>|&^()*/%:!~]	{ return *yytext; }

"0x"{HEXDIGIT}+      { sscanf(yytext, "%x", &yylval.integer); return tNUM; }
{DIGIT}+             { sscanf(yytext, "%d", &yylval.integer); return tNUM; }

"/"{DIGIT}+{FORMAT}  { char * last;
                       yylval.integer = strtol( yytext+1, &last, NULL );
                       yylval.integer = (yylval.integer << 8) | *last;
                       return tFORMAT; }
"/"{FORMAT}          { yylval.integer = (1 << 8) | yytext[1]; return tFORMAT; }

$pc     { yylval.reg = REG_EIP; return tREG; }
$flags  { yylval.reg = REG_EFL; return tREG; }
$eip    { yylval.reg = REG_EIP; return tREG; }
$ip     { yylval.reg = REG_IP;  return tREG; }
$esp    { yylval.reg = REG_ESP; return tREG; }
$sp     { yylval.reg = REG_SP;  return tREG; }
$eax    { yylval.reg = REG_EAX; return tREG; }
$ebx    { yylval.reg = REG_EBX; return tREG; }
$ecx    { yylval.reg = REG_ECX; return tREG; }
$edx    { yylval.reg = REG_EDX; return tREG; }
$esi    { yylval.reg = REG_ESI; return tREG; }
$edi    { yylval.reg = REG_EDI; return tREG; }
$ebp    { yylval.reg = REG_EBP; return tREG; }
$ax     { yylval.reg = REG_AX;  return tREG; }
$bx     { yylval.reg = REG_BX;  return tREG; }
$cx     { yylval.reg = REG_CX;  return tREG; }
$dx     { yylval.reg = REG_DX;  return tREG; }
$si     { yylval.reg = REG_SI;  return tREG; }
$di     { yylval.reg = REG_DI;  return tREG; }
$bp     { yylval.reg = REG_BP;  return tREG; }
$es     { yylval.reg = REG_ES;  return tREG; }
$ds     { yylval.reg = REG_DS;  return tREG; }
$cs     { yylval.reg = REG_CS;  return tREG; }
$ss     { yylval.reg = REG_SS;  return tREG; }
$fs     { yylval.reg = REG_FS;  return tREG; }
$gs     { yylval.reg = REG_GS;  return tREG; }

info|inf|in			{ return tINFO; }
show|sho|sh			{ return tINFO; }
list|lis|li|l			{ return tLIST; }
break|brea|bre|br|b		{ return tBREAK; }
enable|enabl|enab|ena		{ return tENABLE;}
disable|disabl|disab|disa|dis	{ return tDISABLE; }
delete|delet|dele|del		{ return tDELETE; }
quit|qui|qu|q			{ return tQUIT; }
set|se				{ return tSET; }
walk|w				{ return tWALK; }
x				{ return tEXAM; }

class|clas|cla                  { return tCLASS; }
module|modul|modu|mod           { return tMODULE; }
queue|queu|que			{ return tQUEUE; }
registers|regs|reg|re		{ return tREGS; }
segments|segment|segm|seg|se	{ return tSEGMENTS; }
stack|stac|sta|st     		{ return tSTACK; }
window|windo|wind|win|wnd	{ return tWND; }

help|hel|he|"?"			{ return tHELP; }

backtrace|backtrac|backtra|backt|back|bac|ba|bt { return tBACKTRACE; }
where|wher|whe                  { return tBACKTRACE; }

cont|con|co|c   		{ return tCONT; }
step|ste|st|s   		{ return tSTEP; }
next|nex|ne|n   		{ return tNEXT; }

symbolfile|symbolfil|symbolfi|symbolf|symbol|symbo|symb { return tSYMBOLFILE; }

define|defin|defi|def|de        { return tDEFINE; }
abort|abor|abo         	        { return tABORT; }
print|prin|pri|pr|p		{ return tPRINT; }

mode				{ return tMODE; }

{IDENTIFIER}	{ yylval.string = make_symbol(yytext); return tIDENTIFIER; }

[ \t]+        /* Eat up whitespace */

.		{ if (syntax_error == 0)
                  {
		    syntax_error ++; fprintf(stderr, "Syntax Error\n");
                  }
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
        flush_symbols();
        line = readline ("Wine-dbg>");
        if (!line)
        {
            fprintf( stderr, "\n" );
            exit(0);
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
                fprintf(stderr,"Fatal readline goof.\n");
                exit(0);
            }
            strcpy(buf, line);
            buf[len] = '\n';
            buf[len+1] = 0;
            return len + 1;
        }
    }
}

static char *local_symbols[10];
static int next_symbol;

char * make_symbol(char * symbol){
        return local_symbols[next_symbol++] = xstrdup(symbol);
}

void flush_symbols()
{
	while(--next_symbol>= 0) free(local_symbols[next_symbol]);
	next_symbol = 0;
}

#endif  /* DONT_USE_READLINE */
