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

static void issue_prompt(void);
static void mode_command(int);
int yylex(void);
int yyerror(char *);

%}

%union
{
    DBG_VALUE        value;
    char *           string;
    int              integer;
    struct list_id   listing;
    struct expr *    expression;
    struct datatype* type;
}

%token tCONT tPASS tSTEP tLIST tNEXT tQUIT tHELP tBACKTRACE tINFO tWALK tUP tDOWN
%token tENABLE tDISABLE tBREAK tWATCH tDELETE tSET tMODE tPRINT tEXAM tABORT
%token tCLASS tMAPS tMODULE tSTACK tSEGMENTS tREGS tWND tQUEUE tLOCAL
%token tPROCESS tTHREAD tMODREF tEOL
%token tFRAME tSHARE tCOND tDISPLAY tUNDISPLAY tDISASSEMBLE
%token tSTEPI tNEXTI tFINISH tSHOW tDIR tWHATIS
%token <string> tPATH
%token <string> tIDENTIFIER tSTRING tDEBUGSTR tINTVAR
%token <integer> tNUM tFORMAT
%token tSYMBOLFILE tRUN tATTACH tNOPROCESS

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

input: line                   	{ issue_prompt(); }
    | input line                { issue_prompt(); }

line: command 
    | tEOL
    | error tEOL               	{ yyerrok; }

command:
      tQUIT tEOL		{ return FALSE; }
    | tHELP tEOL                { DEBUG_Help(); }
    | tHELP tINFO tEOL          { DEBUG_HelpInfo(); }
    | tCONT tEOL                { DEBUG_CurrThread->dbg_exec_count = 1; 
				  DEBUG_CurrThread->dbg_exec_mode = EXEC_CONT; return TRUE; }
    | tPASS tEOL                { DEBUG_CurrThread->dbg_exec_count = 1; 
				  DEBUG_CurrThread->dbg_exec_mode = EXEC_PASS; return TRUE; }
    | tCONT tNUM tEOL         	{ DEBUG_CurrThread->dbg_exec_count = $2; 
				  DEBUG_CurrThread->dbg_exec_mode = EXEC_CONT; return TRUE; }
    | tSTEP tEOL               	{ DEBUG_CurrThread->dbg_exec_count = 1; 
				  DEBUG_CurrThread->dbg_exec_mode = EXEC_STEP_INSTR; return TRUE; }
    | tNEXT tEOL                { DEBUG_CurrThread->dbg_exec_count = 1; 
				  DEBUG_CurrThread->dbg_exec_mode = EXEC_STEP_OVER; return TRUE; }
    | tSTEP tNUM tEOL           { DEBUG_CurrThread->dbg_exec_count = $2; 
				  DEBUG_CurrThread->dbg_exec_mode = EXEC_STEP_INSTR; return TRUE; }
    | tNEXT tNUM tEOL           { DEBUG_CurrThread->dbg_exec_count = $2; 
				  DEBUG_CurrThread->dbg_exec_mode = EXEC_STEP_OVER; return TRUE; }
    | tSTEPI tEOL               { DEBUG_CurrThread->dbg_exec_count = 1; 
				  DEBUG_CurrThread->dbg_exec_mode = EXEC_STEPI_INSTR; return TRUE; }
    | tNEXTI tEOL               { DEBUG_CurrThread->dbg_exec_count = 1; 
				  DEBUG_CurrThread->dbg_exec_mode = EXEC_STEPI_OVER; return TRUE; }
    | tSTEPI tNUM tEOL          { DEBUG_CurrThread->dbg_exec_count = $2; 
			 	  DEBUG_CurrThread->dbg_exec_mode = EXEC_STEPI_INSTR; return TRUE; }
    | tNEXTI tNUM tEOL          { DEBUG_CurrThread->dbg_exec_count = $2; 
                                  DEBUG_CurrThread->dbg_exec_mode = EXEC_STEPI_OVER; return TRUE; }
    | tABORT tEOL              	{ kill(getpid(), SIGABRT); }
    | tMODE tNUM tEOL          	{ mode_command($2); }
    | tENABLE tNUM tEOL        	{ DEBUG_EnableBreakpoint( $2, TRUE ); }
    | tDISABLE tNUM tEOL       	{ DEBUG_EnableBreakpoint( $2, FALSE ); }
    | tDELETE tBREAK tNUM tEOL 	{ DEBUG_DelBreakpoint( $3 ); }
    | tBACKTRACE tEOL	       	{ DEBUG_BackTrace(TRUE); }
    | tUP tEOL		       	{ DEBUG_SetFrame( curr_frame + 1 );  }
    | tUP tNUM tEOL	       	{ DEBUG_SetFrame( curr_frame + $2 ); }
    | tDOWN tEOL	       	{ DEBUG_SetFrame( curr_frame - 1 );  }
    | tDOWN tNUM tEOL	       	{ DEBUG_SetFrame( curr_frame - $2 ); }
    | tFRAME tNUM tEOL         	{ DEBUG_SetFrame( $2 ); }
    | tFINISH tEOL	       	{ DEBUG_CurrThread->dbg_exec_count = 0;
				  DEBUG_CurrThread->dbg_exec_mode = EXEC_FINISH; return TRUE; }
    | tSHOW tDIR tEOL	       	{ DEBUG_ShowDir(); }
    | tDIR pathname tEOL       	{ DEBUG_AddPath( $2 ); }
    | tDIR tEOL		       	{ DEBUG_NukePath(); }
    | tDISPLAY tEOL	       	{ DEBUG_InfoDisplay(); }
    | tDISPLAY expr tEOL       	{ DEBUG_AddDisplay($2, 1, 0); }
    | tDISPLAY tFORMAT expr tEOL{ DEBUG_AddDisplay($3, $2 >> 8, $2 & 0xff); }
    | tDELETE tDISPLAY tNUM tEOL{ DEBUG_DelDisplay( $3 ); }
    | tDELETE tDISPLAY tEOL    	{ DEBUG_DelDisplay( -1 ); }
    | tUNDISPLAY tNUM tEOL     	{ DEBUG_DelDisplay( $2 ); }
    | tUNDISPLAY tEOL          	{ DEBUG_DelDisplay( -1 ); }
    | tCOND tNUM tEOL          	{ DEBUG_AddBPCondition($2, NULL); }
    | tCOND tNUM expr tEOL	{ DEBUG_AddBPCondition($2, $3); }
    | tSYMBOLFILE pathname tEOL	{ DEBUG_ReadSymbolTable($2); }
    | tWHATIS expr_addr tEOL	{ DEBUG_PrintType(&$2); DEBUG_FreeExprMem(); }
    | tATTACH tNUM tEOL		{ DEBUG_Attach($2, FALSE); return TRUE; }
    | list_command
    | disassemble_command
    | set_command
    | x_command
    | print_command
    | break_command
    | watch_command
    | info_command
    | walk_command
    | run_command
    | noprocess_state

set_command:
      tSET lval_addr '=' expr_value tEOL   { DEBUG_WriteMemory( &$2, $4 );
 					     DEBUG_FreeExprMem(); }

pathname:
      tIDENTIFIER              { $$ = $1; }
    | tPATH		       { $$ = $1; }

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
    | '*' expr_addr	       { DEBUG_FindNearestSymbol( & $2.addr, FALSE, NULL, 0, & $$ ); 
                                 DEBUG_FreeExprMem(); }

x_command:
      tEXAM expr_addr tEOL     { DEBUG_ExamineMemory( &$2, 1, 'x'); DEBUG_FreeExprMem(); }
    | tEXAM tFORMAT expr_addr tEOL  { DEBUG_ExamineMemory( &$3, $2>>8, $2&0xff ); 
 				      DEBUG_FreeExprMem(); }

print_command:
      tPRINT expr_addr tEOL    { DEBUG_Print( &$2, 1, 0, 0 ); DEBUG_FreeExprMem(); }
    | tPRINT tFORMAT expr_addr tEOL { DEBUG_Print( &$3, $2 >> 8, $2 & 0xff, 0 ); 
 				      DEBUG_FreeExprMem(); }

break_command:
      tBREAK '*' expr_addr tEOL{ DEBUG_AddBreakpoint( &$3, NULL ); DEBUG_FreeExprMem(); }
    | tBREAK tIDENTIFIER tEOL  { DEBUG_AddBreakpointFromId($2, -1); }
    | tBREAK tIDENTIFIER ':' tNUM tEOL  { DEBUG_AddBreakpointFromId($2, $4); }
    | tBREAK tNUM tEOL	       { DEBUG_AddBreakpointFromLineno($2); }
    | tBREAK tEOL              { DEBUG_AddBreakpointFromLineno(-1); }

watch_command:
      tWATCH '*' expr_addr tEOL { DEBUG_AddWatchpoint( &$3, 1 ); DEBUG_FreeExprMem(); }
    | tWATCH tIDENTIFIER tEOL   { DEBUG_AddWatchpointFromId($2, -1); }

info_command:
      tINFO tBREAK tEOL         { DEBUG_InfoBreakpoints(); }
    | tINFO tCLASS tSTRING tEOL	{ DEBUG_InfoClass( $3 ); }
    | tINFO tSHARE tEOL		{ DEBUG_InfoShare(); }
    | tINFO tMODULE expr_value tEOL   { DEBUG_DumpModule( $3 ); DEBUG_FreeExprMem(); }
    | tINFO tQUEUE expr_value tEOL    { DEBUG_DumpQueue( $3 ); DEBUG_FreeExprMem(); }
    | tINFO tREGS tEOL          { DEBUG_InfoRegisters(); }
    | tINFO tSEGMENTS expr_value tEOL { DEBUG_InfoSegments( $3, 1 ); DEBUG_FreeExprMem(); }
    | tINFO tSEGMENTS tEOL      { DEBUG_InfoSegments( 0, -1 ); }
    | tINFO tSTACK tEOL         { DEBUG_InfoStack(); }
    | tINFO tMAPS tEOL          { DEBUG_InfoVirtual(); }
    | tINFO tWND expr_value tEOL{ DEBUG_InfoWindow( (HWND)$3 ); DEBUG_FreeExprMem(); }
    | tINFO tLOCAL tEOL         { DEBUG_InfoLocals(); }
    | tINFO tDISPLAY tEOL       { DEBUG_InfoDisplay(); }

walk_command:
      tWALK tCLASS tEOL         { DEBUG_WalkClasses(); }
    | tWALK tMODULE tEOL        { DEBUG_WalkModules(); }
    | tWALK tQUEUE tEOL         { DEBUG_WalkQueues(); }
    | tWALK tWND tEOL           { DEBUG_WalkWindows( 0, 0 ); }
    | tWALK tWND tNUM tEOL      { DEBUG_WalkWindows( $3, 0 ); }
    | tWALK tPROCESS tEOL       { DEBUG_WalkProcess(); }
    | tWALK tTHREAD tEOL        { DEBUG_WalkThreads(); }
    | tWALK tMODREF expr_value tEOL   { DEBUG_WalkModref( $3 ); DEBUG_FreeExprMem(); }

run_command:
      tRUN tEOL                 { DEBUG_Run(NULL); }
    | tRUN tSTRING tEOL         { DEBUG_Run($2); }

noprocess_state:
      tNOPROCESS tEOL		{} /* <CR> shall not barf anything */
    | tNOPROCESS tSTRING tEOL	{ DEBUG_Printf(DBG_CHN_MESG, "No process loaded, cannot execute '%s'\n", $2); }

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
    | tLONG tLONG tUNSIGNED tINT{ $$ = DEBUG_TypeCast(DT_BASIC, "long long unsigned int"); }
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
      expr        		{ DBG_VALUE	value = DEBUG_EvalExpr($1); 
                                  /* expr_value is typed as an integer */
                                  $$ = DEBUG_ReadMemory(&value); }

/*
 * The expr rule builds an expression tree.  When we are done, we call
 * EvalExpr to evaluate the value of the expression.  The advantage of
 * the two-step approach is that it is possible to save expressions for
 * use in 'display' commands, and in conditional watchpoints.
 */
expr:
      tNUM                       { $$ = DEBUG_ConstExpr($1); }
    | tSTRING			 { $$ = DEBUG_StringExpr($1); }
    | tINTVAR                    { $$ = DEBUG_IntVarExpr($1); }
    | tIDENTIFIER		 { $$ = DEBUG_SymbolExpr($1); }
    | expr OP_DRF tIDENTIFIER	 { $$ = DEBUG_StructPExpr($1, $3); } 
    | expr '.' tIDENTIFIER	 { $$ = DEBUG_StructExpr($1, $3); } 
    | tIDENTIFIER '(' ')'	 { $$ = DEBUG_CallExpr($1, 0); } 
    | tIDENTIFIER '(' expr ')'	 { $$ = DEBUG_CallExpr($1, 1, $3); } 
    | tIDENTIFIER '(' expr ',' expr ')'	 { $$ = DEBUG_CallExpr($1, 2, $3, $5); } 
    | tIDENTIFIER '(' expr ',' expr ',' expr ')'	 { $$ = DEBUG_CallExpr($1, 3, $3, $5, $7); } 
    | tIDENTIFIER '(' expr ',' expr ',' expr ',' expr ')'	 { $$ = DEBUG_CallExpr($1, 4, $3, $5, $7, $9); } 
    | tIDENTIFIER '(' expr ',' expr ',' expr ',' expr ',' expr ')'	 { $$ = DEBUG_CallExpr($1, 5, $3, $5, $7, $9, $11); } 
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
    | tINTVAR                    { $$ = DEBUG_IntVarExpr($1); }
    | tIDENTIFIER		 { $$ = DEBUG_SymbolExpr($1); }
    | lvalue OP_DRF tIDENTIFIER	 { $$ = DEBUG_StructPExpr($1, $3); } 
    | lvalue '.' tIDENTIFIER	 { $$ = DEBUG_StructExpr($1, $3); } 
    | lvalue '[' expr ']'	 { $$ = DEBUG_BinopExpr(EXP_OP_ARR, $1, $3); } 
	
%%

static void issue_prompt(void)
{
#ifdef DONT_USE_READLINE
   DEBUG_Printf(DBG_CHN_MESG, "Wine-dbg>");
#endif
}

static void mode_command(int newmode)
{
    if ((newmode == 16) || (newmode == 32)) DEBUG_CurrThread->dbg_mode = newmode;
    else DEBUG_Printf(DBG_CHN_MESG,"Invalid mode (use 16 or 32)\n");
}

void DEBUG_Exit(DWORD ec)
{
   ExitProcess(ec);
}

static WINE_EXCEPTION_FILTER(wine_dbg_cmd)
{
   DEBUG_Printf(DBG_CHN_MESG, "\nwine_dbg_cmd: ");
   switch (GetExceptionCode()) {
   case DEBUG_STATUS_INTERNAL_ERROR:
      DEBUG_Printf(DBG_CHN_MESG, "WineDbg internal error\n");
      break;
   case DEBUG_STATUS_NO_SYMBOL:
      DEBUG_Printf(DBG_CHN_MESG, "Undefined symbol\n");
      break;
   case DEBUG_STATUS_DIV_BY_ZERO:
      DEBUG_Printf(DBG_CHN_MESG, "Division by zero\n");
      break;
   case DEBUG_STATUS_BAD_TYPE:
      DEBUG_Printf(DBG_CHN_MESG, "No type or type mismatch\n");
      break;
   default:
      DEBUG_Printf(DBG_CHN_MESG, "Exception %lx\n", GetExceptionCode());
      break;
   }

   return EXCEPTION_EXECUTE_HANDLER;
}

/***********************************************************************
 *           DEBUG_Parser
 *
 * Debugger editline parser
 */
BOOL	DEBUG_Parser(void)
{
    BOOL 	ret_ok;
    BOOL	ret = TRUE;
#ifdef YYDEBUG
    yydebug = 0;
#endif
    yyin = stdin;

    ret_ok = FALSE;
    do {
       __TRY {
	  issue_prompt();
	  ret_ok = TRUE;
	  if ((ret = yyparse())) {
	     DEBUG_FlushSymbols();
	  }
       } __EXCEPT(wine_dbg_cmd) {
	  ret_ok = FALSE;
       }
       __ENDTRY;
       
    } while (!ret_ok);
    return ret;
}

int yyerror(char* s)
{
   DEBUG_Printf(DBG_CHN_MESG, "%s\n", s);
   return 0;
}
