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
#include "dbg.tab.h"
#include "debugger.h"

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
extern int yylval;
%}

DIGIT	   [0-9]
HEXDIGIT   [0-9a-fA-F]
FORMAT     [bcdiswx]
IDENTIFIER [_a-zA-Z\.~][_a-zA-Z0-9\.~]*

%%

\n		{ syntax_error = 0; return '\n'; } /*Indicate end of command*/

[-+=()*:]       { return *yytext; }

"0x"{HEXDIGIT}+      { sscanf(yytext, "%x", &yylval); return NUM; }
{DIGIT}+             { sscanf(yytext, "%d", &yylval); return NUM; }

"/"{DIGIT}+{FORMAT}  { char *last; yylval = strtol( yytext+1, &last, NULL );
                       yylval = (yylval << 8) | *last; return FORMAT; }
"/"{FORMAT}          { yylval = (1 << 8) | yytext[1]; return FORMAT; }

$pc     { yylval = REG_EIP; return REG; }
$flags  { yylval = REG_EFL; return REG; }
$eip    { yylval = REG_EIP; return REG; }
$ip     { yylval = REG_IP;  return REG; }
$esp    { yylval = REG_ESP; return REG; }
$sp     { yylval = REG_SP;  return REG; }
$eax    { yylval = REG_EAX; return REG; }
$ebx    { yylval = REG_EBX; return REG; }
$ecx    { yylval = REG_ECX; return REG; }
$edx    { yylval = REG_EDX; return REG; }
$esi    { yylval = REG_ESI; return REG; }
$edi    { yylval = REG_EDI; return REG; }
$ebp    { yylval = REG_EBP; return REG; }
$ax     { yylval = REG_AX;  return REG; }
$bx     { yylval = REG_BX;  return REG; }
$cx     { yylval = REG_CX;  return REG; }
$dx     { yylval = REG_DX;  return REG; }
$si     { yylval = REG_SI;  return REG; }
$di     { yylval = REG_DI;  return REG; }
$bp     { yylval = REG_BP;  return REG; }
$es     { yylval = REG_ES;  return REG; }
$ds     { yylval = REG_DS;  return REG; }
$cs     { yylval = REG_CS;  return REG; }
$ss     { yylval = REG_SS;  return REG; }

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

{IDENTIFIER}	{yylval = (int) make_symbol(yytext); 
	          return IDENTIFIER;
	         }

[ \t]+        /* Eat up whitespace */

.		{ if(syntax_error == 0) {
		syntax_error ++; fprintf(stderr, "Syntax Error\n"); }
		}

%%

#ifndef yywrap
int yywrap(void) { return 1; }
#endif

#ifdef USE_READLINE
#ifndef whitespace
#define whitespace(c) (((c) == ' ') || ((c) == '\t'))
#endif

#if 0
/* Used only with GNU readline */
#include "readline/readline.h"
#include "readline/chardefs.h"
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

        /* Remove leading and trailing whitespace from the line.
           Then, if there is anything left, add it to the history list
           and execute it. */
        stripwhite (line);

        if (*line)
        {
            add_history( line );
            strncpy( last_line, line, 255 );
            last_line[255] = '\0'; 
       }
        else line = last_line;  /* Repeat previous command */

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
            free(line);
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
