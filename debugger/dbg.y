
%{

/* Parser for command lines in the Wine debugger
 *
 * Version 1.0
 * Eric Youngdale
 * 9/93
 */

#include <stdio.h>
#define YYSTYPE int

#include "regpos.h"
extern FILE * yyin;
unsigned int * regval = NULL;
unsigned int dbg_mask = 0;
unsigned int dbg_mode = 0;

void issue_prompt();
%}


%token CONT
%token QUIT
%token HELP
%token INFO
%token STACK
%token REG
%token REGS
%token NUM
%token SET
%token PRINT
%token IDENTIFIER
%token NO_SYMBOL
%token SYMBOLFILE
%token DEFINE

%%

 input:  /* empty */
	| input line  { issue_prompt(); }

 line:		'\n'
	| infocmd '\n'
	| error '\n'       {yyerrok; }
	| QUIT  '\n'       { exit(0); };
	| HELP  '\n'       { dbg_help(); };
	| CONT '\n'        { return; };
	| SYMBOLFILE IDENTIFIER '\n' { read_symboltable($2); };
	| DEFINE IDENTIFIER expr '\n'  { add_hash($2, $3); };
	| x_command
	| print_command
	| deposit_command

deposit_command:
	SET REG '=' expr '\n' { regval[$2] = $4; }
	| SET '*' expr '=' expr '\n' { *((unsigned int *) $3) = $5; }
	| SET symbol '=' expr '\n' { *((unsigned int *) $2) = $4; }


x_command:
	  'x' expr  '\n' { examine_memory($2, 1, 'x'); };
	| 'x' '/' fmt expr  '\n' { examine_memory($4, 1, $3); };
	| 'x' '/' NUM fmt expr  '\n' { examine_memory($5, $3, $4); };

 print_command:
	PRINT expr  '\n' { examine_memory(((unsigned int) &$2 ), 1, 'x'); };

 fmt:  'x'     { $$ = 'x'; }
	| 'd'  { $$ = 'd'; }
	| 'i'  { $$ = 'i'; }
	| 'w'  { $$ = 'w'; }
	| 's'  { $$ = 's'; }
	| 'c'  { $$ = 'c'; }
	| 'b' { $$ = 'b'; }

 symbol: IDENTIFIER             { $$ = find_hash($1);
			           if($$ == 0xffffffff) {
					   fprintf(stderr,"Symbol %s not found\n", $1);
					   YYERROR;
				   };
			        }; 

 expr:  NUM			{ $$ = $1;	}
	| REG			{ $$ = regval[$1]; }
	| symbol   		{ $$ = *((unsigned int *) $1); }
	| expr '+' NUM		{ $$ = $1 + $3; }
	| expr '-' NUM		{ $$ = $1 - $3; };
	| '(' expr ')'		{ $$ = $2; };
	| '*' expr		{ $$ = *((unsigned int *) $2); };
	
 infocmd: INFO REGS { info_reg(); }
	| INFO STACK  { info_stack(); };


%%

void 
issue_prompt(){
#ifndef USE_READLINE
	fprintf(stderr,"Wine-dbg>");
#endif
}

static int loaded_symbols = 0;

void
wine_debug(int * regs)
{
	int i;
#ifdef YYDEBUG
	yydebug = 0;
#endif
	yyin = stdin;
	regval = regs;

#ifdef linux        
	if((SC_CS & 7) != 7) {
		dbg_mask = 0xffffffff;
		dbg_mode = 32;
	} else {
		dbg_mask = 0xffff;
		dbg_mode = 16;
	};
#endif
#ifdef __NetBSD__
	if(SC_CS == 0x1f) {
		dbg_mask = 0xffffffff;
		dbg_mode = 32;
	} else {
		dbg_mask = 0xffff;
		dbg_mode = 16;
	};
#endif

	/* This is intended to read the entry points from the Windows image, and
	   insert them in the hash table.  It does not work yet, so it is commented out. */
#if 0
	if(!loaded_symbols){
		loaded_symbols++;
		load_entrypoints();
	};
#endif

	/* Show where we crashed */
	examine_memory(SC_EIP(dbg_mask), 1, 'i');

	issue_prompt();

	yyparse();
	flush_symbols();
	fprintf(stderr,"Returning to Wine...\n");

}


yyerror(char * s){
	fprintf(stderr,"%s\n", s);
}

