%{
/*
 * Parser for command lines in the Wine debugger
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Morten Welinder
 * Copyright 2000 Eric Pouech
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "wine/exception.h"
#include "debugger.h"
#include "expr.h"
#include "task.h"

extern FILE * yyin;
int curr_frame = 0;

static void issue_prompt(void);
static void mode_command(int);
void flush_symbols(void);
int yylex(void);
int yyerror(char *);

%}

%union
{
    DBG_VALUE        value;
    enum debug_regs  reg;
    char *           string;
    int              integer;
    struct list_id   listing;
    struct expr *    expression;
    struct datatype * type;
}

%token tCONT tPASS tSTEP tLIST tNEXT tQUIT tHELP tBACKTRACE tINFO tWALK tUP tDOWN
%token tENABLE tDISABLE tBREAK tWATCH tDELETE tSET tMODE tPRINT tEXAM tABORT
%token tCLASS tMAPS tMODULE tSTACK tSEGMENTS tREGS tWND tQUEUE tLOCAL
%token tPROCESS tMODREF
%token tEOL tSTRING tDEBUGSTR
%token tFRAME tSHARE tCOND tDISPLAY tUNDISPLAY tDISASSEMBLE
%token tSTEPI tNEXTI tFINISH tSHOW tDIR tWHATIS
%token <string> tPATH
%token <string> tIDENTIFIER tSTRING tDEBUGSTR
%token <integer> tNUM tFORMAT
%token <reg> tREG
%token tSYMBOLFILE

%token tCHAR tSHORT tINT tLONG tFLOAT tDOUBLE tUNSIGNED tSIGNED 
%token tSTRUCT tUNION tENUM

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
%left '.' '[' OP_DRF
%nonassoc ':'

%type <expression> expr lval lvalue 
%type <type> type_cast type_expr
%type <value> expr_addr lval_addr
%type <integer> expr_value
%type <string> pathname

%type <listing> list_arg

%%

input: line                    { issue_prompt(); }
    | input line               { issue_prompt(); }

line: command 
    | tEOL
    | error tEOL               { yyerrok; }

command:
      tQUIT tEOL               { DEBUG_Exit(0); }
    | tHELP tEOL               { DEBUG_Help(); }
    | tHELP tINFO tEOL         { DEBUG_HelpInfo(); }
    | tCONT tEOL               { DEBUG_CurrThread->dbg_exec_count = 1; 
				 DEBUG_CurrThread->dbg_exec_mode = EXEC_CONT; return 0; }
    | tPASS tEOL               { DEBUG_CurrThread->dbg_exec_count = 1; 
				 DEBUG_CurrThread->dbg_exec_mode = EXEC_PASS; return 0; }
    | tCONT tNUM tEOL          { DEBUG_CurrThread->dbg_exec_count = $2; 
				 DEBUG_CurrThread->dbg_exec_mode = EXEC_CONT; return 0; }
    | tSTEP tEOL               { DEBUG_CurrThread->dbg_exec_count = 1; 
				 DEBUG_CurrThread->dbg_exec_mode = EXEC_STEP_INSTR; return 0; }
    | tNEXT tEOL               { DEBUG_CurrThread->dbg_exec_count = 1; 
				 DEBUG_CurrThread->dbg_exec_mode = EXEC_STEP_OVER; return 0; }
    | tSTEP tNUM tEOL          { DEBUG_CurrThread->dbg_exec_count = $2; 
				 DEBUG_CurrThread->dbg_exec_mode = EXEC_STEP_INSTR; return 0; }
    | tNEXT tNUM tEOL          { DEBUG_CurrThread->dbg_exec_count = $2; 
				 DEBUG_CurrThread->dbg_exec_mode = EXEC_STEP_OVER; return 0; }
    | tSTEPI tEOL              { DEBUG_CurrThread->dbg_exec_count = 1; 
				 DEBUG_CurrThread->dbg_exec_mode = EXEC_STEPI_INSTR; return 0; }
    | tNEXTI tEOL              { DEBUG_CurrThread->dbg_exec_count = 1; 
				 DEBUG_CurrThread->dbg_exec_mode = EXEC_STEPI_OVER; return 0; }
    | tSTEPI tNUM tEOL         { DEBUG_CurrThread->dbg_exec_count = $2; 
				 DEBUG_CurrThread->dbg_exec_mode = EXEC_STEPI_INSTR; return 0; }
    | tNEXTI tNUM tEOL         { DEBUG_CurrThread->dbg_exec_count = $2; 
				 DEBUG_CurrThread->dbg_exec_mode = EXEC_STEPI_OVER; return 0; }
    | tABORT tEOL              { kill(getpid(), SIGABRT); }
    | tMODE tNUM tEOL          { mode_command($2); }
    | tENABLE tNUM tEOL        { DEBUG_EnableBreakpoint( $2, TRUE ); }
    | tDISABLE tNUM tEOL       { DEBUG_EnableBreakpoint( $2, FALSE ); }
    | tDELETE tBREAK tNUM tEOL { DEBUG_DelBreakpoint( $3 ); }
    | tBACKTRACE tEOL	       { DEBUG_BackTrace(TRUE); }
    | tUP tEOL		       { DEBUG_SetFrame( curr_frame + 1 );  }
    | tUP tNUM tEOL	       { DEBUG_SetFrame( curr_frame + $2 ); }
    | tDOWN tEOL	       { DEBUG_SetFrame( curr_frame - 1 );  }
    | tDOWN tNUM tEOL	       { DEBUG_SetFrame( curr_frame - $2 ); }
    | tFRAME tNUM tEOL         { DEBUG_SetFrame( $2 ); }
    | tFINISH tEOL	       { DEBUG_CurrThread->dbg_exec_count = 0;
				 DEBUG_CurrThread->dbg_exec_mode = EXEC_FINISH; return 0; }
    | tSHOW tDIR tEOL	       { DEBUG_ShowDir(); }
    | tDIR pathname tEOL       { DEBUG_AddPath( $2 ); }
    | tDIR tEOL		       { DEBUG_NukePath(); }
    | tDISPLAY tEOL	       { DEBUG_InfoDisplay(); }
    | tDISPLAY expr tEOL       { DEBUG_AddDisplay($2, 1, 0); }
    | tDISPLAY tFORMAT expr tEOL { DEBUG_AddDisplay($3, $2 >> 8, $2 & 0xff); }
    | tDELETE tDISPLAY tNUM tEOL { DEBUG_DelDisplay( $3 ); }
    | tDELETE tDISPLAY tEOL    { DEBUG_DelDisplay( -1 ); }
    | tUNDISPLAY tNUM tEOL     { DEBUG_DelDisplay( $2 ); }
    | tUNDISPLAY tEOL          { DEBUG_DelDisplay( -1 ); }
    | tCOND tNUM tEOL          { DEBUG_AddBPCondition($2, NULL); }
    | tCOND tNUM expr tEOL     { DEBUG_AddBPCondition($2, $3); }
    | tSYMBOLFILE pathname tEOL{ DEBUG_ReadSymbolTable($2); }
    | tWHATIS expr_addr tEOL   { DEBUG_PrintType(&$2); DEBUG_FreeExprMem(); }
    | list_command
    | disassemble_command
    | set_command
    | x_command
    | print_command
    | break_command
    | watch_command
    | info_command
    | walk_command

set_command:
      tSET tREG '=' expr_value tEOL	   { DEBUG_SetRegister( $2, $4 ); 
					     DEBUG_FreeExprMem(); }
    | tSET lval_addr '=' expr_value tEOL   { DEBUG_WriteMemory( &$2.addr, $4 ); 
 					     DEBUG_FreeExprMem(); }

pathname:
      tIDENTIFIER                    { $$ = $1; }
    | tPATH			     { $$ = $1; }

disassemble_command:
      tDISASSEMBLE tEOL              { DEBUG_Disassemble( NULL, NULL, 10 ); }
    | tDISASSEMBLE expr_addr tEOL    { DEBUG_Disassemble( & $2, NULL, 10 ); }
    | tDISASSEMBLE expr_addr ',' expr_addr tEOL { DEBUG_Disassemble( & $2, & $4, 0 ); }

list_command:
      tLIST tEOL               { DEBUG_List( NULL, NULL, 10 ); }
    | tLIST '-' tEOL	       { DEBUG_List( NULL, NULL, -10 ); }
    | tLIST list_arg tEOL      { DEBUG_List( & $2, NULL, 10 ); }
    | tLIST ',' list_arg tEOL  { DEBUG_List( NULL, & $3, -10 ); }
    | tLIST list_arg ',' list_arg tEOL { DEBUG_List( & $2, & $4, 0 ); }

list_arg:
      tNUM		       { $$.sourcefile = NULL; $$.line = $1; }
    | pathname ':' tNUM	       { $$.sourcefile = $1; $$.line = $3; }
    | tIDENTIFIER	       { DEBUG_GetFuncInfo( & $$, NULL, $1); }
    | pathname ':' tIDENTIFIER { DEBUG_GetFuncInfo( & $$, $1, $3); }
    | '*' expr_addr	       { DEBUG_FindNearestSymbol( & $2.addr, FALSE, NULL, 
							0, & $$ ); 
 					     DEBUG_FreeExprMem(); }

x_command:
      tEXAM expr_addr tEOL     { DEBUG_ExamineMemory( &$2, 1, 'x'); 
 					     DEBUG_FreeExprMem(); }
    | tEXAM tFORMAT expr_addr tEOL  { DEBUG_ExamineMemory( &$3, $2>>8, $2&0xff ); 
 					     DEBUG_FreeExprMem(); }

print_command:
      tPRINT expr_addr tEOL    { DEBUG_Print( &$2, 1, 0, 0 ); 
 					     DEBUG_FreeExprMem(); }
    | tPRINT tFORMAT expr_addr tEOL { DEBUG_Print( &$3, $2 >> 8, $2 & 0xff, 0 ); 
 					     DEBUG_FreeExprMem(); }

break_command:
      tBREAK '*' expr_addr tEOL { DEBUG_AddBreakpoint( &$3 ); 
 					     DEBUG_FreeExprMem(); }
    | tBREAK tIDENTIFIER tEOL  { DBG_VALUE value;
				 if( DEBUG_GetSymbolValue($2, -1, &value, TRUE) )
				   {
				     DEBUG_AddBreakpoint( &value );
				   }
				 else
				   {
				     fprintf(stderr,"Unable to add breakpoint\n");
				   }
				}
    | tBREAK tIDENTIFIER ':' tNUM tEOL  { DBG_VALUE value;
				 if( DEBUG_GetSymbolValue($2, $4, &value, TRUE) )
				   {
				     DEBUG_AddBreakpoint( &value );
				   }
				 else
				   {
				     fprintf(stderr,"Unable to add breakpoint\n");
				   }
				}
    | tBREAK tNUM tEOL	       { struct name_hash *nh;
				 DBG_VALUE value;
				 DEBUG_GetCurrentAddress( &value.addr );
				 DEBUG_FindNearestSymbol(&value.addr, TRUE,
							 &nh, 0, NULL);
				 if( nh != NULL )
				   {
				     DEBUG_GetLineNumberAddr(nh, $2, &value.addr, TRUE);
				     value.type = NULL;
				     value.cookie = DV_TARGET;
				     DEBUG_AddBreakpoint( &value );
				   }
				 else
				   {
				     fprintf(stderr,"Unable to add breakpoint\n");
				   }
                               }

    | tBREAK tEOL              { DBG_VALUE value;
				 DEBUG_GetCurrentAddress( &value.addr );
				 value.type = NULL;
				 value.cookie = DV_TARGET;
                                 DEBUG_AddBreakpoint( &value );
                               }

watch_command:
      tWATCH '*' expr_addr tEOL { DEBUG_AddWatchpoint( &$3, 1 ); 
 					     DEBUG_FreeExprMem(); }
    | tWATCH tIDENTIFIER tEOL  { DBG_VALUE value;
				 if( DEBUG_GetSymbolValue($2, -1, &value, TRUE) )
				     DEBUG_AddWatchpoint( &value, 1 );
				 else
				     fprintf(stderr,"Unable to add breakpoint\n");
				}

info_command:
      tINFO tBREAK tEOL         { DEBUG_InfoBreakpoints(); }
    | tINFO tCLASS tSTRING tEOL	{ DEBUG_InfoClass( $3 ); DEBUG_FreeExprMem(); }
    | tINFO tSHARE tEOL		{ DEBUG_InfoShare(); }
    | tINFO tMODULE expr_value tEOL   { DEBUG_DumpModule( $3 ); DEBUG_FreeExprMem(); }
    | tINFO tQUEUE expr_value tEOL    { DEBUG_DumpQueue( $3 ); DEBUG_FreeExprMem(); }
    | tINFO tREGS tEOL          { DEBUG_InfoRegisters(); }
    | tINFO tSEGMENTS expr_value tEOL { DEBUG_InfoSegments( $3, 1 ); DEBUG_FreeExprMem(); }
    | tINFO tSEGMENTS tEOL      { DEBUG_InfoSegments( 0, -1 ); }
    | tINFO tSTACK tEOL         { DEBUG_InfoStack(); }
    | tINFO tMAPS tEOL          { DEBUG_InfoVirtual(); }
    | tINFO tWND expr_value tEOL      { DEBUG_InfoWindow( (HWND)$3 ); 
 					     DEBUG_FreeExprMem(); }
    | tINFO tLOCAL tEOL         { DEBUG_InfoLocals(); }
    | tINFO tDISPLAY tEOL       { DEBUG_InfoDisplay(); }

walk_command:
      tWALK tCLASS tEOL         { DEBUG_WalkClasses(); }
    | tWALK tMODULE tEOL        { DEBUG_WalkModules(); }
    | tWALK tQUEUE tEOL         { DEBUG_WalkQueues(); }
    | tWALK tWND tEOL           { DEBUG_WalkWindows( 0, 0 ); }
    | tWALK tWND tNUM tEOL      { DEBUG_WalkWindows( $3, 0 ); }
    | tWALK tPROCESS tEOL       { DEBUG_WalkProcess(); }
    | tWALK tMODREF expr_value tEOL   { DEBUG_WalkModref( $3 ); }


type_cast: 
      '(' type_expr ')'		{ $$ = $2; }

type_expr:
      type_expr '*'		{ $$ = DEBUG_FindOrMakePointerType($1); }
    |  tINT			{ $$ = DEBUG_TypeCast(DT_BASIC, "int"); }
    | tCHAR			{ $$ = DEBUG_TypeCast(DT_BASIC, "char"); }
    | tLONG tINT		{ $$ = DEBUG_TypeCast(DT_BASIC, "long int"); }
    | tUNSIGNED tINT		{ $$ = DEBUG_TypeCast(DT_BASIC, "unsigned int"); }
    | tLONG tUNSIGNED tINT	{ $$ = DEBUG_TypeCast(DT_BASIC, "long unsigned int"); }
    | tLONG tLONG tINT		{ $$ = DEBUG_TypeCast(DT_BASIC, "long long int"); }
    | tLONG tLONG tUNSIGNED tINT { $$ = DEBUG_TypeCast(DT_BASIC, "long long unsigned int"); }
    | tSHORT tINT		{ $$ = DEBUG_TypeCast(DT_BASIC, "short int"); }
    | tSHORT tUNSIGNED tINT	{ $$ = DEBUG_TypeCast(DT_BASIC, "short unsigned int"); }
    | tSIGNED tCHAR		{ $$ = DEBUG_TypeCast(DT_BASIC, "signed char"); }
    | tUNSIGNED tCHAR		{ $$ = DEBUG_TypeCast(DT_BASIC, "unsigned char"); }
    | tFLOAT			{ $$ = DEBUG_TypeCast(DT_BASIC, "float"); }
    | tDOUBLE			{ $$ = DEBUG_TypeCast(DT_BASIC, "double"); }
    | tLONG tDOUBLE		{ $$ = DEBUG_TypeCast(DT_BASIC, "long double"); }
    | tSTRUCT tIDENTIFIER	{ $$ = DEBUG_TypeCast(DT_STRUCT, $2); }
    | tUNION tIDENTIFIER	{ $$ = DEBUG_TypeCast(DT_STRUCT, $2); }
    | tENUM tIDENTIFIER		{ $$ = DEBUG_TypeCast(DT_ENUM, $2); }

expr_addr:
      expr			{ $$ = DEBUG_EvalExpr($1); }

expr_value:
      expr        { DBG_VALUE	value = DEBUG_EvalExpr($1); 
                    /* expr_value is typed as an integer */
		    if (!value.addr.off || 
			!DEBUG_READ_MEM((void*)value.addr.off, &$$, sizeof($$)))
		       $$ = 0; }
/*
 * The expr rule builds an expression tree.  When we are done, we call
 * EvalExpr to evaluate the value of the expression.  The advantage of
 * the two-step approach is that it is possible to save expressions for
 * use in 'display' commands, and in conditional watchpoints.
 */
expr:
      tNUM                       { $$ = DEBUG_ConstExpr($1); }
    | tSTRING			 { $$ = DEBUG_StringExpr($1); }
    | tREG                       { $$ = DEBUG_RegisterExpr($1); }
    | tIDENTIFIER		 { $$ = DEBUG_SymbolExpr($1); }
    | expr OP_DRF tIDENTIFIER	 { $$ = DEBUG_StructPExpr($1, $3); } 
    | expr '.' tIDENTIFIER	 { $$ = DEBUG_StructExpr($1, $3); } 
    | tIDENTIFIER '(' ')'	 { $$ = DEBUG_CallExpr($1, 0); } 
    | tIDENTIFIER '(' expr ')'	 { $$ = DEBUG_CallExpr($1, 1, $3); } 
    | tIDENTIFIER '(' expr ',' expr ')'	 { $$ = DEBUG_CallExpr($1, 2, $3, $5); } 
    | tIDENTIFIER '(' expr ',' expr ',' expr ')'	 { $$ = DEBUG_CallExpr($1, 3, $3, $5, $7); } 
    | tIDENTIFIER '(' expr ',' expr ',' expr ',' expr ')'	 { $$ = DEBUG_CallExpr($1, 3, $3, $5, $7, $9); } 
    | tIDENTIFIER '(' expr ',' expr ',' expr ',' expr ',' expr ')'	 { $$ = DEBUG_CallExpr($1, 3, $3, $5, $7, $9, $11); } 
    | expr '[' expr ']'		 { $$ = DEBUG_BinopExpr(EXP_OP_ARR, $1, $3); } 
    | expr ':' expr		 { $$ = DEBUG_BinopExpr(EXP_OP_SEG, $1, $3); } 
    | expr OP_LOR expr           { $$ = DEBUG_BinopExpr(EXP_OP_LOR, $1, $3); }
    | expr OP_LAND expr          { $$ = DEBUG_BinopExpr(EXP_OP_LAND, $1, $3); }
    | expr '|' expr              { $$ = DEBUG_BinopExpr(EXP_OP_OR, $1, $3); }
    | expr '&' expr              { $$ = DEBUG_BinopExpr(EXP_OP_AND, $1, $3); }
    | expr '^' expr              { $$ = DEBUG_BinopExpr(EXP_OP_XOR, $1, $3); }
    | expr OP_EQ expr            { $$ = DEBUG_BinopExpr(EXP_OP_EQ, $1, $3); }
    | expr '>' expr              { $$ = DEBUG_BinopExpr(EXP_OP_GT, $1, $3); }
    | expr '<' expr              { $$ = DEBUG_BinopExpr(EXP_OP_LT, $1, $3); }
    | expr OP_GE expr            { $$ = DEBUG_BinopExpr(EXP_OP_GE, $1, $3); }
    | expr OP_LE expr            { $$ = DEBUG_BinopExpr(EXP_OP_LE, $1, $3); }
    | expr OP_NE expr            { $$ = DEBUG_BinopExpr(EXP_OP_NE, $1, $3); }
    | expr OP_SHL expr           { $$ = DEBUG_BinopExpr(EXP_OP_SHL, $1, $3); }
    | expr OP_SHR expr           { $$ = DEBUG_BinopExpr(EXP_OP_SHR, $1, $3); }
    | expr '+' expr              { $$ = DEBUG_BinopExpr(EXP_OP_ADD, $1, $3); }
    | expr '-' expr              { $$ = DEBUG_BinopExpr(EXP_OP_SUB, $1, $3); }
    | expr '*' expr              { $$ = DEBUG_BinopExpr(EXP_OP_MUL, $1, $3); }
    | expr '/' expr              { $$ = DEBUG_BinopExpr(EXP_OP_DIV, $1, $3); }
    | expr '%' expr              { $$ = DEBUG_BinopExpr(EXP_OP_REM, $1, $3); }
    | '-' expr %prec OP_SIGN     { $$ = DEBUG_UnopExpr(EXP_OP_NEG, $2); }
    | '+' expr %prec OP_SIGN     { $$ = $2; }
    | '!' expr                   { $$ = DEBUG_UnopExpr(EXP_OP_NOT, $2); }
    | '~' expr                   { $$ = DEBUG_UnopExpr(EXP_OP_LNOT, $2); }
    | '(' expr ')'               { $$ = $2; }
    | '*' expr %prec OP_DEREF    { $$ = DEBUG_UnopExpr(EXP_OP_DEREF, $2); }
    | '&' expr %prec OP_DEREF    { $$ = DEBUG_UnopExpr(EXP_OP_ADDR, $2); }
    | type_cast expr %prec OP_DEREF { $$ = DEBUG_TypeCastExpr($1, $2); } 
	
/*
 * The lvalue rule builds an expression tree.  This is a limited form
 * of expression that is suitable to be used as an lvalue.
 */
lval_addr:
    lval			 { $$ = DEBUG_EvalExpr($1); }

lval:
      lvalue                     { $$ = $1; }
    | '*' expr			 { $$ = DEBUG_UnopExpr(EXP_OP_FORCE_DEREF, $2); }
	
lvalue:
      tNUM                       { $$ = DEBUG_ConstExpr($1); }
    | tREG                       { $$ = DEBUG_RegisterExpr($1); }
    | tIDENTIFIER		 { $$ = DEBUG_SymbolExpr($1); }
    | lvalue OP_DRF tIDENTIFIER	 { $$ = DEBUG_StructPExpr($1, $3); } 
    | lvalue '.' tIDENTIFIER	 { $$ = DEBUG_StructExpr($1, $3); } 
    | lvalue '[' expr ']'	 { $$ = DEBUG_BinopExpr(EXP_OP_ARR, $1, $3); } 
	
%%

static void issue_prompt(void)
{
#ifdef DONT_USE_READLINE
   fprintf(stderr, "Wine-dbg>");
#endif
}

static void mode_command(int newmode)
{
    if ((newmode == 16) || (newmode == 32)) DEBUG_CurrThread->dbg_mode = newmode;
    else fprintf(stderr,"Invalid mode (use 16 or 32)\n");
}

static WINE_EXCEPTION_FILTER(wine_dbg)
{
   switch (GetExceptionCode()) {
   case DEBUG_STATUS_INTERNAL_ERROR:
      fprintf(stderr, "WineDbg internal error\n");
      break;
   case DEBUG_STATUS_NO_SYMBOL:
      fprintf(stderr, "Undefined symbol\n");
      break;
   case DEBUG_STATUS_DIV_BY_ZERO:
      fprintf(stderr, "Division by zero\n");
      break;
   case DEBUG_STATUS_BAD_TYPE:
      fprintf(stderr, "No type or type mismatch\n");
      break;
   default:
      fprintf(stderr, "Exception %lx\n", GetExceptionCode());
      break;
   }
   return EXCEPTION_EXECUTE_HANDLER;
}

/***********************************************************************
 *           DEBUG_Exit
 *
 * Kill current process.
 *
 */
void DEBUG_Exit( DWORD exit_code )
{
    TASK_KillTask( 0 ); /* FIXME: should not be necessary */
    TerminateProcess( DEBUG_CurrProcess->handle, exit_code );
}

/***********************************************************************
 *           DEBUG_Main
 *
 * Debugger main loop.
 */
BOOL DEBUG_Main( BOOL is_debug, BOOL force, DWORD code )
{
    int newmode;
    BOOL ret_ok;
    char ch;

#ifdef YYDEBUG
    yydebug = 0;
#endif

    yyin = stdin;

    DEBUG_SuspendExecution();

    if (!is_debug)
    {
#ifdef __i386__
        if (DEBUG_IsSelectorSystem(DEBUG_context.SegCs))
            fprintf( stderr, " in 32-bit code (0x%08lx).\n", DEBUG_context.Eip );
        else
            fprintf( stderr, " in 16-bit code (%04x:%04lx).\n",
                     (WORD)DEBUG_context.SegCs, DEBUG_context.Eip );
#else
        fprintf( stderr, " (%p).\n", GET_IP(&DEBUG_context) );
#endif
    }

    if (DEBUG_LoadEntryPoints("Loading new modules symbols:\n"))
       DEBUG_ProcessDeferredDebug();

    if (force || !(is_debug && DEBUG_ShouldContinue( code, 
						     DEBUG_CurrThread->dbg_exec_mode, 
						     &DEBUG_CurrThread->dbg_exec_count )))
    {
        DBG_ADDR addr;
        DEBUG_GetCurrentAddress( &addr );

#ifdef __i386__
        switch (newmode = DEBUG_GetSelectorType(addr.seg)) {
	case 16: case 32: break;
	default: fprintf(stderr, "Bad CS (%ld)\n", addr.seg); newmode = 32;
	}
#else
        newmode = 32;
#endif
        if (newmode != DEBUG_CurrThread->dbg_mode)
            fprintf(stderr,"In %d bit mode.\n", DEBUG_CurrThread->dbg_mode = newmode);

	DEBUG_DoDisplay();

        if (is_debug || force)
	{
	  /*
	   * Do a quiet backtrace so that we have an idea of what the situation
	   * is WRT the source files.
	   */
	    DEBUG_BackTrace(FALSE);
	}
	else
        {
	    /* This is a real crash, dump some info */
            DEBUG_InfoRegisters();
            DEBUG_InfoStack();
#ifdef __i386__
            if (DEBUG_CurrThread->dbg_mode == 16)
            {
	        DEBUG_InfoSegments( DEBUG_context.SegDs >> 3, 1 );
                if (DEBUG_context.SegEs != DEBUG_context.SegDs)
                    DEBUG_InfoSegments( DEBUG_context.SegEs >> 3, 1 );
            }
            DEBUG_InfoSegments( DEBUG_context.SegFs >> 3, 1 );
#endif
            DEBUG_BackTrace(TRUE);
        }

	if (!is_debug ||
            (DEBUG_CurrThread->dbg_exec_mode == EXEC_STEPI_OVER) ||
            (DEBUG_CurrThread->dbg_exec_mode == EXEC_STEPI_INSTR))
        {
	    /* Show where we crashed */
	    curr_frame = 0;
	    DEBUG_PrintAddress( &addr, DEBUG_CurrThread->dbg_mode, TRUE );
	    fprintf(stderr,":  ");
	    DEBUG_Disasm( &addr, TRUE );
	    fprintf( stderr, "\n" );
        }

        ret_ok = 0;
        do
        {
	    __TRY 
	    {
	       issue_prompt();
	       yyparse();
	       flush_symbols();

	       DEBUG_GetCurrentAddress( &addr );
	       if ((ret_ok = DEBUG_ValidateRegisters()))
		  ret_ok = DEBUG_READ_MEM_VERBOSE((void*)DEBUG_ToLinear( &addr ), &ch, 1 );
	    } 
	    __EXCEPT(wine_dbg)
	    {
	       ret_ok = 0;
	    }
	    __ENDTRY;

        } while (!ret_ok);
    }

    DEBUG_CurrThread->dbg_exec_mode = DEBUG_RestartExecution( DEBUG_CurrThread->dbg_exec_mode, DEBUG_CurrThread->dbg_exec_count );
    /*
     * This will have gotten absorbed into the breakpoint info
     * if it was used.  Otherwise it would have been ignored.
     * In any case, we don't mess with it any more.
     */
    if ((DEBUG_CurrThread->dbg_exec_mode == EXEC_CONT) || (DEBUG_CurrThread->dbg_exec_mode == EXEC_PASS))
	DEBUG_CurrThread->dbg_exec_count = 0;
    
    return (DEBUG_CurrThread->dbg_exec_mode == EXEC_PASS) ? 0 : DBG_CONTINUE;
}

int yyerror(char* s)
{
   fprintf(stderr,"%s\n", s);
   return 0;
}
