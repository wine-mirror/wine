
%{

/* Parser for command lines in the Wine debugger
 *
 * Version 1.0
 * Eric Youngdale
 * 9/93
 */

#include <stdio.h>
#include <signal.h>

#define YYSTYPE int

#include "regpos.h"
extern FILE * yyin;
unsigned int * regval = NULL;
unsigned int dbg_mask = 0;
unsigned int dbg_mode = 0;

void issue_prompt(void);
void mode_command(int);
%}


%token CONT
%token QUIT
%token HELP
%token BACKTRACE
%token INFO
%token STACK
%token REG
%token REGS
%token NUM
%token ENABLE
%token DISABLE
%token BREAK
%token SET
%token MODE
%token PRINT
%token IDENTIFIER
%token NO_SYMBOL
%token SYMBOLFILE
%token DEFINE
%token ABORT

%%

 input:  /* empty */
	| input line  { issue_prompt(); }

 line:		'\n'
	| infocmd '\n'
	| error '\n'       { yyerrok; }
	| QUIT  '\n'       { exit(0); }
	| 'q' '\n'         { exit(0); }
	| HELP  '\n'       { dbg_help(); }
	| CONT '\n'        { return; }
	| 'c' '\n'         { return; }
	| ABORT '\n'       { kill(getpid(), SIGABRT); }
	| SYMBOLFILE IDENTIFIER '\n' { read_symboltable($2); }
	| DEFINE IDENTIFIER expr '\n'  { add_hash($2, $3); }
	| MODE NUM	   { mode_command($2); }
	| ENABLE NUM	   { enable_break($2); }
	| DISABLE NUM	   { disable_break($2); }
	| BREAK '*' expr   { add_break($3); }
	| x_command
	| BACKTRACE '\n'   { dbg_bt(); }
	| print_command
	| deposit_command

deposit_command:
	SET REG '=' expr '\n' { if(regval) regval[$2] = $4; else application_not_running();}
	| SET '*' expr '=' expr '\n' { *((unsigned int *) $3) = $5; }
	| SET symbol '=' expr '\n' { *((unsigned int *) $2) = $4; }


x_command:
	  'x' expr  '\n' { examine_memory($2, 1, 'x'); }
	| 'x' '/' fmt expr  '\n' { examine_memory($4, 1, $3); }
	| 'x' '/' NUM fmt expr  '\n' { examine_memory($5, $3, $4); }

print:
	  'p'
	| print
	
 print_command:
	  print expr '\n' { examine_memory(((unsigned int) &$2 ), 1, 'x'); }
	| print '/' fmt expr '\n' { examine_memory((unsigned int) &$4, 1, $3); }
	| print '/' NUM fmt expr '\n' { examine_memory((unsigned int) &$5, $3, $4); }

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
				   }
			        } 

 expr:  NUM			{ $$ = $1;	}
	| REG			{ if(regval) $$ = regval[$1]; else application_not_running();}
	| symbol   		{ $$ = *((unsigned int *) $1); }
	| expr '+' NUM		{ $$ = $1 + $3; }
	| expr '-' NUM		{ $$ = $1 - $3; }
	| '(' expr ')'		{ $$ = $2; }
	| '*' expr		{ $$ = *((unsigned int *) $2); }
	
 infocmd: INFO REGS { info_reg(); }
	| INFO STACK  { info_stack(); }
	| INFO BREAK  { info_break(); }


%%

void 
issue_prompt(){
#ifndef USE_READLINE
	fprintf(stderr,"Wine-dbg>");
#endif
}

void mode_command(int newmode)
{
  if(newmode == 16){
    dbg_mask = 0xffff;
    dbg_mode = 16;
    return;
  }
  if(newmode == 32){ 
    dbg_mask = 0xffffffff;
    dbg_mode = 32;
    return;
  }
  fprintf(stderr,"Invalid mode (use 16 or 32)\n");
}

static int loaded_symbols = 0;

void
wine_debug(int signal, int * regs)
{
	static int dummy_regs[32];
	int i;
#ifdef YYDEBUG
	yydebug = 0;
#endif

	yyin = stdin;
	regval = regs ? regs : dummy_regs;

#ifdef linux        
	if((SC_CS & 7) != 7) {
		dbg_mask = 0xffffffff;
		dbg_mode = 32;
	} else {
		dbg_mask = 0xffff;
		dbg_mode = 16;
	}
#endif
#ifdef __NetBSD__
	if(SC_CS == 0x1f) {
		dbg_mask = 0xffffffff;
		dbg_mode = 32;
	} else {
		dbg_mask = 0xffff;
		dbg_mode = 16;
	}
#endif
	fprintf(stderr,"In %d bit mode.\n", dbg_mode);

	/* This is intended to read the entry points from the Windows image, and
	   insert them in the hash table.  It does not work yet, so it is commented out. */
	if(!loaded_symbols){
		loaded_symbols++;
		read_symboltable("wine.sym");
#if 0
		load_entrypoints();
#endif
	}

	/* Remove the breakpoints from memory... */
	fprintf(stderr,"Removing BPs\n");
	insert_break(0);

	/* If we stopped on a breakpoint, report this fact */
	if(signal == SIGTRAP)
	  {
	    unsigned int addr;
	    addr = SC_EIP(dbg_mask);
	    if((addr & 0xffff0000) == 0 && dbg_mode == 16)
	      addr |= SC_CS << 16;
	    fprintf(stderr,"Stopped on breakpoint %d\n", get_bpnum(addr));
	  }

	/* Show where we crashed */
	if(regs)
	  examine_memory(SC_EIP(dbg_mask), 1, 'i');

	issue_prompt();

	yyparse();
	flush_symbols();

	/* Re-insert the breakpoints from memory... */
	insert_break(1);

	fprintf(stderr,"Returning to Wine...\n");

}


yyerror(char * s){
	fprintf(stderr,"%s\n", s);
}

