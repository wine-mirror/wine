
%{

/* Parser for command lines in the Wine debugger
 *
 * Version 1.0
 * Eric Youngdale
 * 9/93
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "windows.h"
#include "debugger.h"

#define YYSTYPE int

extern FILE * yyin;
unsigned int dbg_mode = 0;

static enum exec_mode dbg_exec_mode = EXEC_CONT;

void issue_prompt(void);
void mode_command(int);
%}


%token CONT
%token STEP
%token NEXT
%token QUIT
%token HELP
%token BACKTRACE
%token INFO
%token STACK
%token SEGMENTS
%token REG
%token REGS
%token NUM
%token ENABLE
%token DISABLE
%token BREAK
%token DELETE
%token SET
%token MODE
%token PRINT
%token EXAM
%token IDENTIFIER
%token FORMAT
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
	| HELP  '\n'       { dbg_help(); }
	| CONT '\n'        { dbg_exec_mode = EXEC_CONT; return 0; }
	| STEP '\n'        { dbg_exec_mode = EXEC_STEP_INSTR; return 0; }
	| NEXT '\n'        { dbg_exec_mode = EXEC_STEP_OVER; return 0; }
	| ABORT '\n'       { kill(getpid(), SIGABRT); }
 	| SYMBOLFILE IDENTIFIER '\n'   { read_symboltable($2); }
	| DEFINE IDENTIFIER expr '\n'  { add_hash($2, 0, $3); }
	| MODE NUM '\n'         { mode_command($2); }
	| ENABLE NUM '\n'       { DEBUG_EnableBreakpoint( $2, TRUE ); }
	| DISABLE NUM '\n'      { DEBUG_EnableBreakpoint( $2, FALSE ); }
	| BREAK '*' expr '\n'       { DEBUG_AddBreakpoint( 0xffffffff, $3 ); }
	| BREAK '*' expr ':' expr '\n'	{ DEBUG_AddBreakpoint( $3, $5); }
        | BREAK '\n'            { DEBUG_AddBreakpoint( 0xffffffff, EIP ); }
        | DELETE BREAK NUM '\n' { DEBUG_DelBreakpoint( $3 ); }
	| BACKTRACE '\n'        { DEBUG_BackTrace(); }
	| x_command
	| print_command
	| deposit_command

deposit_command:
	SET REG '=' expr '\n'        { DEBUG_SetRegister( $2, $4 ); }
	| SET '*' expr '=' expr '\n' { *((unsigned int *) $3) = $5; }
	| SET symbol '=' expr '\n'   { *((unsigned int *) $2) = $4; }


x_command:
	  EXAM expr  '\n' { examine_memory( 0xffffffff, $2, 1, 'x'); }
	| EXAM FORMAT expr  '\n' { examine_memory( 0xffffffff, $3,
                                                  $2 >> 8, $2 & 0xff ); }
	| EXAM expr ':' expr '\n' { examine_memory( $2, $4, 1, 'x' ); }
	| EXAM FORMAT expr ':' expr'\n'  { examine_memory( $3, $5, 
	                                              $2 >> 8,  $2 & 0xff ); }

 print_command:
	  PRINT expr '\n' { examine_memory( 0, ((unsigned int) &$2 ), 1,'x'); }
	| PRINT FORMAT expr '\n' { examine_memory( 0, (unsigned int)&$3,
                                                   $2 >> 8, $2 & 0xff ); }

 symbol: IDENTIFIER   { if (($$ = find_hash($1)) == 0xffffffff)
                        {
                           fprintf(stderr,"Symbol %s not found\n", (char *)$1);
                           YYERROR;
                        }
                      } 

 expr:  NUM			{ $$ = $1;	}
	| REG			{ $$ = DEBUG_GetRegister($1); }
	| symbol   		{ $$ = $1; }
	| expr '+' NUM		{ $$ = $1 + $3; }
	| expr '-' NUM		{ $$ = $1 - $3; }
	| '(' expr ')'		{ $$ = $2; }
	| '*' expr		{ $$ = *((unsigned int *) $2); }
	
 infocmd: INFO REGS     { DEBUG_InfoRegisters(); }
	| INFO STACK    { DEBUG_InfoStack(); }
	| INFO BREAK    { DEBUG_InfoBreakpoints(); }
	| INFO SEGMENTS { LDT_Print(); }


%%

void 
issue_prompt(){
#ifndef USE_READLINE
	fprintf(stderr,"Wine-dbg>");
#endif
}

void mode_command(int newmode)
{
    if ((newmode == 16) || (newmode == 32)) dbg_mode = newmode;
    else fprintf(stderr,"Invalid mode (use 16 or 32)\n");
}


void wine_debug( int signal, struct sigcontext_struct *regs )
{
    static int loaded_symbols = 0;
    char SymbolTableFile[256];
    int instr_len = 0, newmode;
#ifdef YYDEBUG
    yydebug = 1;
#endif

    yyin = stdin;
    context = (struct sigcontext_struct *)regs;

    if (CS == WINE_CODE_SELECTOR) newmode = 32;
    else newmode = (GET_SEL_FLAGS(CS) & LDT_FLAGS_32BIT) ? 32 : 16;

    if (newmode != dbg_mode)
        fprintf(stderr,"In %d bit mode.\n", dbg_mode = newmode);

    if(dbg_mode == 32 && !loaded_symbols)
    {
        loaded_symbols++;
        GetPrivateProfileString("wine", "SymbolTableFile", "wine.sym",
                          SymbolTableFile, sizeof(SymbolTableFile), WINE_INI);
        read_symboltable(SymbolTableFile);
    }

    DEBUG_SetBreakpoints( FALSE );

    if ((signal != SIGTRAP) || !DEBUG_ShouldContinue( regs, dbg_exec_mode ))
    {
        unsigned int segment = (CS == WINE_CODE_SELECTOR) ? 0 : CS;

        /* Show where we crashed */
        print_address( segment, EIP, dbg_mode );
        fprintf(stderr,":  ");
        instr_len = db_disasm( segment, EIP ) - EIP;
        fprintf(stderr,"\n");
        
        issue_prompt();
        yyparse();
        flush_symbols();
    }

    DEBUG_RestartExecution( regs, dbg_exec_mode, instr_len );
}


int yyerror(char * s)
{
	fprintf(stderr,"%s\n", s);
        return 0;
}

