
%{
/*
 * Parser for command lines in the Wine debugger
 *
 * Copyright 1993 Eric Youngdale
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "windows.h"
#include "debugger.h"

extern FILE * yyin;
unsigned int dbg_mode = 0;

static enum exec_mode dbg_exec_mode = EXEC_CONT;

void issue_prompt(void);
void mode_command(int);
void flush_symbols(void);
int yylex(void);
int yyerror(char *);

%}

%union
{
    DBG_ADDR         address;
    enum debug_regs  reg;
    char *           string;
    int              integer;
}

%token CONT STEP NEXT QUIT HELP BACKTRACE INFO STACK SEGMENTS REGS
%token ENABLE DISABLE BREAK DELETE SET MODE PRINT EXAM DEFINE ABORT
%token NO_SYMBOL
%token SYMBOLFILE

%token <string> IDENTIFIER
%token <integer> NUM FORMAT
%token <reg> REG

%type <integer> expr
%type <address> addr symbol

%%

 input:  /* empty */
	| input line  { issue_prompt(); }

 line:		'\n'
	| infocmd '\n'
	| error '\n'       { yyerrok; }
	| QUIT  '\n'       { exit(0); }
	| HELP  '\n'       { DEBUG_Help(); }
	| CONT '\n'        { dbg_exec_mode = EXEC_CONT; return 0; }
	| STEP '\n'        { dbg_exec_mode = EXEC_STEP_INSTR; return 0; }
	| NEXT '\n'        { dbg_exec_mode = EXEC_STEP_OVER; return 0; }
	| ABORT '\n'       { kill(getpid(), SIGABRT); }
 	| SYMBOLFILE IDENTIFIER '\n'  { DEBUG_ReadSymbolTable( $2 ); }
	| DEFINE IDENTIFIER addr '\n' { DEBUG_AddSymbol( $2, &$3 ); }
	| MODE NUM '\n'         { mode_command($2); }
	| ENABLE NUM '\n'       { DEBUG_EnableBreakpoint( $2, TRUE ); }
	| DISABLE NUM '\n'      { DEBUG_EnableBreakpoint( $2, FALSE ); }
	| BREAK '*' addr '\n'   { DEBUG_AddBreakpoint( &$3 ); }
        | BREAK symbol '\n'     { DEBUG_AddBreakpoint( &$2 ); }
        | BREAK '\n'            { DBG_ADDR addr = { CS_reg(DEBUG_context),
                                                    EIP_reg(DEBUG_context) };
                                  DEBUG_AddBreakpoint( &addr );
                                }
        | DELETE BREAK NUM '\n' { DEBUG_DelBreakpoint( $3 ); }
	| BACKTRACE '\n'        { DEBUG_BackTrace(); }
	| x_command
	| print_command
	| deposit_command

deposit_command:
	SET REG '=' expr '\n'          { DEBUG_SetRegister( $2, $4 ); }
	| SET '*' addr '=' expr '\n'   { DEBUG_WriteMemory( &$3, $5 ); }
	| SET IDENTIFIER '=' addr '\n' { if (!DEBUG_SetSymbolValue( $2, &$4 ))
                                         {
                                           fprintf( stderr, "Symbol %s not found\n", $2 );
                                           YYERROR;
                                         }
                                       }


x_command:
	  EXAM addr '\n' { DEBUG_ExamineMemory( &$2, 1, 'x'); }
	| EXAM FORMAT addr '\n' { DEBUG_ExamineMemory( &$3, $2>>8, $2&0xff ); }

 print_command:
	  PRINT addr '\n'        { DEBUG_Print( &$2, 1, 'x' ); }
	| PRINT FORMAT addr '\n' { DEBUG_Print( &$3, $2 >> 8, $2 & 0xff ); }

 symbol: IDENTIFIER   { if (!DEBUG_GetSymbolValue( $1, &$$ ))
                        {
                           fprintf( stderr, "Symbol %s not found\n", $1 );
                           YYERROR;
                        }
                      } 

 addr: expr                     { $$.seg = 0xffffffff; $$.off = $1; }
       | expr ':' expr          { $$.seg = $1; $$.off = $3; }
       | symbol   		{ $$ = $1; }

 expr:  NUM			{ $$ = $1;	}
	| REG			{ $$ = DEBUG_GetRegister($1); }
	| expr '+' NUM		{ $$ = $1 + $3; }
	| expr '-' NUM		{ $$ = $1 - $3; }
	| '(' expr ')'		{ $$ = $2; }
	| '*' addr		{ $$ = DEBUG_ReadMemory( &$2 ); }
	
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
    yydebug = 0;
#endif

    yyin = stdin;
    DEBUG_context = (struct sigcontext_struct *)regs;

    DEBUG_SetBreakpoints( FALSE );

    if (!loaded_symbols)
    {
        loaded_symbols++;
        GetPrivateProfileString("wine", "SymbolTableFile", "wine.sym",
                          SymbolTableFile, sizeof(SymbolTableFile), WINE_INI);
        DEBUG_ReadSymbolTable( SymbolTableFile );
        DEBUG_LoadEntryPoints();
    }

    if ((signal != SIGTRAP) || !DEBUG_ShouldContinue( regs, dbg_exec_mode ))
    {
        DBG_ADDR addr;

        addr.seg = (CS_reg(DEBUG_context) == WINE_CODE_SELECTOR) ?
                    0 : CS_reg(DEBUG_context);
        addr.off = EIP_reg(DEBUG_context);

        if (!addr.seg) newmode = 32;
        else newmode = (GET_SEL_FLAGS(addr.seg) & LDT_FLAGS_32BIT) ? 32 : 16;

        if (newmode != dbg_mode)
            fprintf(stderr,"In %d bit mode.\n", dbg_mode = newmode);

        /* Show where we crashed */
        DEBUG_PrintAddress( &addr, dbg_mode );
        fprintf(stderr,":  ");
        DEBUG_Disasm( &addr );
        fprintf(stderr,"\n");
        instr_len = addr.off - EIP_reg(DEBUG_context);
        
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

