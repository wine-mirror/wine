

/* Lexical scanner for command line parsing in the Wine debugger
 *
 * Version 1.0
 * Eric Youngdale
 * 9/93
 */

%{
#include <stdio.h>
#include <string.h>
#include "dbg.tab.h"
#include "regpos.h"

#ifdef USE_READLINE
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = dbg_read((char *) buf, max_size )) < 0 ) \
	    YY_FATAL_ERROR( "read() in flex scanner failed" );
#endif

extern char * readline(char *);
static char * make_symbol(char *);
void flush_symbols();
static int syntax_error;
%}

DIGIT	[0-9]
HEXDIGIT [0-9a-fA-F]

IDENTIFIER [_a-zA-Z\.~][_a-zA-Z0-9\.~]*

%%

\n		{ syntax_error = 0; return '\n'; } /* Indicate end of command */

"+"		{ return '+'; } 

"-"		{ return '-'; } 

"/"		{ return '/'; } 

"="		{ return '='; } 

"("		{ return '('; } 

")"		{ return ')'; } 

"*"		{ return '*'; } 

"?"		{ return HELP; }

"0x"+{HEXDIGIT}+   {
		sscanf(yytext, "%lx", &yylval);
		return NUM;
		}

{DIGIT}+   {
		sscanf(yytext, "%lx", &yylval);
		return NUM;
		}

$pc		{ yylval = RN_EIP; return REG;}
$sp		{ yylval = RN_ESP; return REG;}
$eip		{ yylval = RN_EIP; return REG;}
$esp		{ yylval = RN_ESP; return REG;}
$ebp		{ yylval = RN_EBP; return REG;}
$eax		{ yylval = RN_EAX; return REG;}
$ebx		{ yylval = RN_EBX; return REG;}
$ecx		{ yylval = RN_ECX; return REG;}
$edx		{ yylval = RN_EDX; return REG;}
$esi		{ yylval = RN_ESI; return REG;}
$edi		{ yylval = RN_EDI; return REG;}

info|inf|in		{ return INFO; }

quit|qui|qu 	{ return QUIT; }

help|hel|he	{ return HELP; }

set|se		{ return SET; }

cont|con|co		{ return CONT; }

symbolfile|symbolfil|symbolfi|symbolf|symbol|symbo|symb { return SYMBOLFILE; }

define|defin|defi|def|de        { return DEFINE; }
print|prin|pri|pr		{ return PRINT; }

regs|reg|re	{ return REGS; }

stack|stac|sta|st     	{ return STACK; }

x		{ return 'x'; }
d		{ return 'd'; }
i		{ return 'i'; }
w		{ return 'w'; }
b		{ return 'b'; }
s		{ return 's'; }
c		{ return 'c'; }

{IDENTIFIER}	{yylval = (int) make_symbol(yytext); 
	          return IDENTIFIER;
	         }

[ \t]+        /* Eat up whitespace */

.		{ if(syntax_error == 0) {
		syntax_error ++; fprintf(stderr, "Syntax Error\n"); }
		}

%%

#ifdef USE_READLINE
#ifndef whitespace
#define whitespace(c) (((c) == ' ') || ((c) == '\t'))
#endif

#if 0
/* Used only with GNU readline */
#include "readline/readline.h"
#include "readline/chardefs.h"
#endif

dbg_read(char * buf, int size){
	char * line;
	int len;

	do{
		flush_symbols();
		line = readline ("Wine-dbg>");
		len = strlen(line);
				
		if (!line)
		{
			return 0;
		}
		else
		{
			/* Remove leading and trailing whitespace from the line.
			   Then, if there is anything left, add it to the history list
			   and execute it. */
			stripwhite (line);
			
			if (*line)
			{
				add_history (line);
				if(size < len + 1){
					fprintf(stderr,"Fatal readline goof.\n");
					exit(0);
				};
				strcpy(buf, line);
				buf[len] = '\n';
				buf[len+1] = 0;
				free(line);
				return len + 1;
			}
		}

	} while (1==1);
}

/* Strip whitespace from the start and end of STRING. */
stripwhite (string)
     char *string;
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
