%{
/*
 * Parser for command lines in the Wine debugger
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Morten Welinder
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

%token CONT STEP LIST NEXT QUIT HELP BACKTRACE INFO STACK SEGMENTS REGS
%token ENABLE DISABLE BREAK DELETE SET MODE PRINT EXAM DEFINE ABORT
%token NO_SYMBOL EOL
%token SYMBOLFILE

%token <string> IDENTIFIER
%token <integer> NUM FORMAT
%token <reg> REG

/* %left ',' */
/* %left '=' OP_OR_EQUAL OP_XOR_EQUAL OP_AND_EQUAL OP_SHL_EQUAL \
         OP_SHR_EQUAL OP_PLUS_EQUAL OP_MINUS_EQUAL \
         OP_TIMES_EQUAL OP_DIVIDE_EQUAL OP_MODULO_EQUAL */
/* %left OP_COND */ /* ... ? ... : ... */
%left OP_LOR
%left OP_LAND
%left '|'
%left '^'
%left '&'
%left OP_EQ OP_NE
%left '<' '>' OP_LE OP_GE
%left OP_SHL OP_SHR
%left '+' '-'
%left '*' '/' '%'
%left OP_SIGN '!' '~' OP_DEREF /* OP_INC OP_DEC OP_ADDR */
%nonassoc ':'

%type <integer> expr
%type <address> addr segaddr symbol

%%

 input:   line			{ issue_prompt(); }
	| input line		{ issue_prompt(); }

 line:  command 
	| EOL
	| error	EOL	       { yyerrok; }

 command: QUIT EOL             { exit(0); }
	| HELP EOL             { DEBUG_Help(); }
	| CONT EOL             { dbg_exec_mode = EXEC_CONT; return 0; }
	| STEP EOL             { dbg_exec_mode = EXEC_STEP_INSTR; return 0; }
	| NEXT EOL             { dbg_exec_mode = EXEC_STEP_OVER; return 0; }
	| LIST EOL	       { DEBUG_List( NULL, 15 ); }
	| LIST addr EOL	       { DEBUG_List( &$2, 15 ); }
	| ABORT	EOL            { kill(getpid(), SIGABRT); }
	| SYMBOLFILE IDENTIFIER EOL  { DEBUG_ReadSymbolTable( $2 ); }
	| DEFINE IDENTIFIER addr EOL { DEBUG_AddSymbol( $2, &$3 ); }
	| MODE NUM EOL	       { mode_command($2); }
	| ENABLE NUM EOL       { DEBUG_EnableBreakpoint( $2, TRUE ); }
	| DISABLE NUM EOL      { DEBUG_EnableBreakpoint( $2, FALSE ); }
	| BREAK '*' addr EOL   { DEBUG_AddBreakpoint( &$3 ); }
	| BREAK symbol EOL     { DEBUG_AddBreakpoint( &$2 ); }
	| BREAK EOL	       { DBG_ADDR addr = { CS_reg(DEBUG_context),
						     EIP_reg(DEBUG_context) };
				 DEBUG_AddBreakpoint( &addr );
			       }
	| DELETE BREAK NUM EOL { DEBUG_DelBreakpoint( $3 ); }
	| BACKTRACE EOL	       { DEBUG_BackTrace(); }
	| infocmd
	| x_command
	| print_command
	| deposit_command

deposit_command:
	SET REG '=' expr EOL	      { DEBUG_SetRegister( $2, $4 ); }
	| SET '*' addr '=' expr	EOL   { DEBUG_WriteMemory( &$3, $5 ); }
	| SET IDENTIFIER '=' addr EOL { if (!DEBUG_SetSymbolValue( $2, &$4 ))
				         {
					   fprintf( stderr,
						 "Symbol %s not found\n", $2 );
					   YYERROR;
				         }
				       }


x_command:
	  EXAM addr EOL         { DEBUG_ExamineMemory( &$2, 1, 'x'); }
	| EXAM FORMAT addr EOL  { DEBUG_ExamineMemory( &$3, $2>>8, $2&0xff ); }

 print_command:
	  PRINT addr EOL        { DEBUG_Print( &$2, 1, 'x' ); }
	| PRINT FORMAT addr EOL { DEBUG_Print( &$3, $2 >> 8, $2 & 0xff ); }

 symbol: IDENTIFIER   { if (!DEBUG_GetSymbolValue( $1, &$$ ))
			{
			   fprintf( stderr, "Symbol %s not found\n", $1 );
			   YYERROR;
			}
		      } 

 addr: expr				{ $$.seg = 0xffffffff; $$.off = $1; }
       | segaddr			{ $$ = $1; }

 segaddr: expr ':' expr			{ $$.seg = $1; $$.off = $3; }
       | symbol				{ $$ = $1; }

 expr:	NUM				{ $$ = $1;	}
	| REG				{ $$ = DEBUG_GetRegister($1); }
	| expr OP_LOR expr		{ $$ = $1 || $3; }
	| expr OP_LAND expr		{ $$ = $1 && $3; }
	| expr '|' expr			{ $$ = $1 | $3; }
	| expr '&' expr			{ $$ = $1 & $3; }
	| expr '^' expr			{ $$ = $1 ^ $3; }
	| expr OP_EQ expr		{ $$ = $1 == $3; }
	| expr '>' expr			{ $$ = $1 > $3; }
	| expr '<' expr			{ $$ = $1 < $3; }
	| expr OP_GE expr		{ $$ = $1 >= $3; }
	| expr OP_LE expr		{ $$ = $1 <= $3; }
	| expr OP_NE expr		{ $$ = $1 != $3; }
	| expr OP_SHL expr		{ $$ = (unsigned)$1 << $3; }
	| expr OP_SHR expr		{ $$ = (unsigned)$1 >> $3; }
	| expr '+' expr			{ $$ = $1 + $3; }
	| expr '-' expr			{ $$ = $1 - $3; }
	| expr '*' expr			{ $$ = $1 * $3; }
	| expr '/' expr
	  { if ($3) 
	      if ($3 == -1 && $1 == 0x80000000l)
		yyerror ("Division overflow");
	      else
		$$ = $1 / $3;
	    else
	      yyerror ("Division by zero"); }
	| expr '%' expr
	  { if ($3) 
	      if ($3 == -1 && $1 == 0x80000000l)
		$$ = 0; /* A sensible result in this case.  */
	      else
		$$ = $1 % $3;
	    else
	      yyerror ("Division by zero"); }
	| '-' expr %prec OP_SIGN	{ $$ = -$2; }
	| '+' expr %prec OP_SIGN	{ $$ = $2; }
	| '!' expr			{ $$ = !$2; }
	| '~' expr			{ $$ = ~$2; }
	| '(' expr ')'			{ $$ = $2; }
/* For parser technical reasons we can't use "addr" here.  */
	| '*' expr %prec OP_DEREF	{ DBG_ADDR addr = { 0xffffffff, $2 };
					  $$ = DEBUG_ReadMemory( &addr ); }
	| '*' segaddr %prec OP_DEREF	{ $$ = DEBUG_ReadMemory( &$2 ); }
	
 infocmd: INFO REGS EOL	          { DEBUG_InfoRegisters(); }
	| INFO STACK EOL          { DEBUG_InfoStack(); }
	| INFO BREAK EOL          { DEBUG_InfoBreakpoints(); }
	| INFO SEGMENTS EOL	  { LDT_Print( 0, -1 ); }
	| INFO SEGMENTS expr EOL  { LDT_Print( SELECTOR_TO_ENTRY($3), 1 ); }


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

        addr.seg = CS_reg(DEBUG_context);
        addr.off = EIP_reg(DEBUG_context);
        DBG_FIX_ADDR_SEG( &addr, 0 );

        if (!addr.seg) newmode = 32;
        else newmode = (GET_SEL_FLAGS(addr.seg) & LDT_FLAGS_32BIT) ? 32 : 16;

        if (newmode != dbg_mode)
            fprintf(stderr,"In %d bit mode.\n", dbg_mode = newmode);

        /* Show where we crashed */
        DEBUG_PrintAddress( &addr, dbg_mode );
        fprintf(stderr,":  ");
        if (DBG_CHECK_READ_PTR( &addr, 1 ))
        {
            DEBUG_Disasm( &addr );
            fprintf(stderr,"\n");
            instr_len = addr.off - EIP_reg(DEBUG_context);
        }

        do
        {
            issue_prompt();
            yyparse();
            flush_symbols();
            addr.seg = CS_reg(DEBUG_context);
            addr.off = EIP_reg(DEBUG_context);
            DBG_FIX_ADDR_SEG( &addr, 0 );
        } while (!DBG_CHECK_READ_PTR( &addr, 1 ));
    }

    DEBUG_RestartExecution( regs, dbg_exec_mode, instr_len );
}


int yyerror(char * s)
{
	fprintf(stderr,"%s\n", s);
        return 0;
}

