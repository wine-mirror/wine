/* Lexical scanner for command line parsing in the Wine debugger
 *
 * Version 1.0
 * Eric Youngdale
 * 9/93
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debugger.h"
#include "dbg.tab.h"

#ifdef USE_READLINE
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = dbg_read((char *) buf, max_size )) < 0 ) \
	    YY_FATAL_ERROR( "read() in flex scanner failed" );
#endif

extern char * readline(char *);
extern void add_history(char *);
static int dbg_read(char * buf, int size);
static char * make_symbol(char *);
void flush_symbols();
static int syntax_error;
%}

DIGIT	   [0-9]
HEXDIGIT   [0-9a-fA-F]
FORMAT     [bcdiswx]
IDENTIFIER [_a-zA-Z\.~][_a-zA-Z0-9\.~]*

%%

\n		{ syntax_error = 0; return '\n'; } /*Indicate end of command*/

[-+=()*:]       { return *yytext; }

"0x"{HEXDIGIT}+      { sscanf(yytext, "%x", &yylval.integer); return NUM; }
{DIGIT}+             { sscanf(yytext, "%d", &yylval.integer); return NUM; }

"/"{DIGIT}+{FORMAT}  { char * last;
                       yylval.integer = strtol( yytext+1, &last, NULL );
                       yylval.integer = (yylval.integer << 8) | *last;
                       return FORMAT; }
"/"{FORMAT}          { yylval.integer = (1 << 8) | yytext[1]; return FORMAT; }

$pc     { yylval.reg = REG_EIP; return REG; }
$flags  { yylval.reg = REG_EFL; return REG; }
$eip    { yylval.reg = REG_EIP; return REG; }
$ip     { yylval.reg = REG_IP;  return REG; }
$esp    { yylval.reg = REG_ESP; return REG; }
$sp     { yylval.reg = REG_SP;  return REG; }
$eax    { yylval.reg = REG_EAX; return REG; }
$ebx    { yylval.reg = REG_EBX; return REG; }
$ecx    { yylval.reg = REG_ECX; return REG; }
$edx    { yylval.reg = REG_EDX; return REG; }
$esi    { yylval.reg = REG_ESI; return REG; }
$edi    { yylval.reg = REG_EDI; return REG; }
$ebp    { yylval.reg = REG_EBP; return REG; }
$ax     { yylval.reg = REG_AX;  return REG; }
$bx     { yylval.reg = REG_BX;  return REG; }
$cx     { yylval.reg = REG_CX;  return REG; }
$dx     { yylval.reg = REG_DX;  return REG; }
$si     { yylval.reg = REG_SI;  return REG; }
$di     { yylval.reg = REG_DI;  return REG; }
$bp     { yylval.reg = REG_BP;  return REG; }
$es     { yylval.reg = REG_ES;  return REG; }
$ds     { yylval.reg = REG_DS;  return REG; }
$cs     { yylval.reg = REG_CS;  return REG; }
$ss     { yylval.reg = REG_SS;  return REG; }

info|inf|in		      { return INFO; }
segments|segm                 { return SEGMENTS; }
break|brea|bre|br|b           { return BREAK; }
enable|enabl|enab|ena         { return ENABLE;}
disable|disabl|disab|disa|dis { return DISABLE; }
delete|delet|dele|del         { return DELETE; }
quit|qui|qu|q                 { return QUIT; }
x                             { return EXAM; }

help|hel|he	{ return HELP; }
"?"		{ return HELP; }

set|se		{ return SET; }

bt		{ return BACKTRACE; }

cont|con|co|c   { return CONT; }
step|ste|st|s   { return STEP; }
next|nex|ne|n   { return NEXT; }

symbolfile|symbolfil|symbolfi|symbolf|symbol|symbo|symb { return SYMBOLFILE; }

define|defin|defi|def|de        { return DEFINE; }
abort|abor|abo         	        { return ABORT; }
print|prin|pri|pr|p		{ return PRINT; }

mode				{ return MODE; }

registers|regs|reg|re	{ return REGS; }

stack|stac|sta|st     	{ return STACK; }

{IDENTIFIER}	{ yylval.string = make_symbol(yytext); return IDENTIFIER; }

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

#ifdef USE_READLINE
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
        if (!line) exit(0);

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
        return local_symbols[next_symbol++] = strdup(symbol);
}

void
flush_symbols(){
	while(--next_symbol>= 0) free(local_symbols[next_symbol]);
	next_symbol = 0;
}

#endif
