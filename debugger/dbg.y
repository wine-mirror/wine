%{
/*
 * Parser for command lines in the Wine debugger
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Morten Welinder
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include "winbase.h"
#include "class.h"
#include "module.h"
#include "task.h"
#include "options.h"
#include "queue.h"
#include "wine/winbase16.h"
#include "winnt.h"
#include "win.h"
#include "debugger.h"
#include "neexe.h"
#include "process.h"
#include "server.h"
#include "main.h"
#include "expr.h"
#include "user.h"

extern FILE * yyin;
unsigned int dbg_mode = 0;
HANDLE dbg_heap = 0;
int curr_frame = 0;

static enum exec_mode dbg_exec_mode = EXEC_CONT;
static int dbg_exec_count = 0;

void issue_prompt(void);
void mode_command(int);
void flush_symbols(void);
int yylex(void);
int yyerror(char *);

#ifdef DBG_need_heap
#define malloc(x) DBG_alloc(x)
#define realloc(x,y) DBG_realloc(x,y)
#define free(x) DBG_free(x)
#endif

extern void VIRTUAL_Dump(void);  /* memory/virtual.c */

%}

%union
{
    DBG_ADDR         address;
    enum debug_regs  reg;
    char *           string;
    int              integer;
    struct list_id   listing;
    struct expr *    expression;
    struct datatype * type;
}

%token tCONT tSTEP tLIST tNEXT tQUIT tHELP tBACKTRACE tINFO tWALK tUP tDOWN
%token tENABLE tDISABLE tBREAK tDELETE tSET tMODE tPRINT tEXAM tABORT tDEBUGMSG
%token tCLASS tMAPS tMODULE tSTACK tSEGMENTS tREGS tWND tQUEUE tLOCAL
%token tEOL tSTRING tDEBUGSTR
%token tFRAME tSHARE tCOND tDISPLAY tUNDISPLAY tDISASSEMBLE
%token tSTEPI tNEXTI tFINISH tSHOW tDIR
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
%type <address> expr_addr lval_addr
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
      tQUIT tEOL               { exit(0); }
    | tHELP tEOL               { DEBUG_Help(); }
    | tHELP tINFO tEOL         { DEBUG_HelpInfo(); }
    | tCONT tEOL               { dbg_exec_count = 1; 
				 dbg_exec_mode = EXEC_CONT; return 0; }
    | tCONT tNUM tEOL          { dbg_exec_count = $2; 
				 dbg_exec_mode = EXEC_CONT; return 0; }
    | tSTEP tEOL               { dbg_exec_count = 1; 
				 dbg_exec_mode = EXEC_STEP_INSTR; return 0; }
    | tNEXT tEOL               { dbg_exec_count = 1; 
				 dbg_exec_mode = EXEC_STEP_OVER; return 0; }
    | tSTEP tNUM tEOL          { dbg_exec_count = $2; 
				 dbg_exec_mode = EXEC_STEP_INSTR; return 0; }
    | tNEXT tNUM tEOL          { dbg_exec_count = $2; 
				 dbg_exec_mode = EXEC_STEP_OVER; return 0; }
    | tSTEPI tEOL              { dbg_exec_count = 1; 
				 dbg_exec_mode = EXEC_STEPI_INSTR; return 0; }
    | tNEXTI tEOL              { dbg_exec_count = 1; 
				 dbg_exec_mode = EXEC_STEPI_OVER; return 0; }
    | tSTEPI tNUM tEOL         { dbg_exec_count = $2; 
				 dbg_exec_mode = EXEC_STEPI_INSTR; return 0; }
    | tNEXTI tNUM tEOL         { dbg_exec_count = $2; 
				 dbg_exec_mode = EXEC_STEPI_OVER; return 0; }
    | tABORT tEOL              { kill(getpid(), SIGABRT); }
    | tMODE tNUM tEOL          { mode_command($2); }
    | tENABLE tNUM tEOL        { DEBUG_EnableBreakpoint( $2, TRUE ); }
    | tDISABLE tNUM tEOL       { DEBUG_EnableBreakpoint( $2, FALSE ); }
    | tDELETE tBREAK tNUM tEOL { DEBUG_DelBreakpoint( $3 ); }
    | tBACKTRACE tEOL	       { DEBUG_BackTrace(); }
    | tUP tEOL		       { DEBUG_SetFrame( curr_frame + 1 );  }
    | tUP tNUM tEOL	       { DEBUG_SetFrame( curr_frame + $2 ); }
    | tDOWN tEOL	       { DEBUG_SetFrame( curr_frame - 1 );  }
    | tDOWN tNUM tEOL	       { DEBUG_SetFrame( curr_frame - $2 ); }
    | tFRAME tNUM tEOL         { DEBUG_SetFrame( $2 ); }
    | tFINISH tEOL	       { dbg_exec_count = 0;
				 dbg_exec_mode = EXEC_FINISH; return 0; }
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
    | tDEBUGMSG tDEBUGSTR tEOL { MAIN_ParseDebugOptions($2); }
    | tSYMBOLFILE pathname tEOL{ DEBUG_ReadSymbolTable($2); }
    | list_command
    | disassemble_command
    | set_command
    | x_command
    | print_command
    | break_command
    | info_command
    | walk_command

set_command:
      tSET tREG '=' expr_value tEOL	   { DEBUG_SetRegister( $2, $4 ); 
					     DEBUG_FreeExprMem(); }
    | tSET lval_addr '=' expr_value tEOL   { DEBUG_WriteMemory( &$2, $4 ); 
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
    | '*' expr_addr	       { DEBUG_FindNearestSymbol( & $2, FALSE, NULL, 
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
    | tBREAK tIDENTIFIER tEOL  { DBG_ADDR addr;
				 if( DEBUG_GetSymbolValue($2, -1, &addr, TRUE) )
				   {
				     DEBUG_AddBreakpoint( &addr );
				   }
				 else
				   {
				     fprintf(stderr,"Unable to add breakpoint\n");
				   }
				}
    | tBREAK tIDENTIFIER ':' tNUM tEOL  { DBG_ADDR addr;
				 if( DEBUG_GetSymbolValue($2, $4, &addr, TRUE) )
				   {
				     DEBUG_AddBreakpoint( &addr );
				   }
				 else
				   {
				     fprintf(stderr,"Unable to add breakpoint\n");
				   }
				}
    | tBREAK tNUM tEOL	       { struct name_hash *nh;
				 DBG_ADDR addr;
				 TDB *pTask = (TDB*)GlobalLock16( GetCurrentTask() );

				 addr.type = NULL;
				 addr.seg = CS_reg(&DEBUG_context);
				 addr.off = EIP_reg(&DEBUG_context);

				 if (ISV86(&DEBUG_context))
				     addr.seg |= (DWORD)(pTask?(pTask->hModule):0)<<16;
				 DBG_FIX_ADDR_SEG( &addr, CS_reg(&DEBUG_context) );
				 GlobalUnlock16( GetCurrentTask() );
				 DEBUG_FindNearestSymbol(&addr, TRUE,
							 &nh, 0, NULL);
				 if( nh != NULL )
				   {
				     DEBUG_GetLineNumberAddr(nh, 
						      $2, &addr, TRUE);
				     DEBUG_AddBreakpoint( &addr );
				   }
				 else
				   {
				     fprintf(stderr,"Unable to add breakpoint\n");
				   }
                               }

    | tBREAK tEOL              { DBG_ADDR addr;
				 TDB *pTask = (TDB*)GlobalLock16( GetCurrentTask() );

				 addr.type = NULL;
				 addr.seg = CS_reg(&DEBUG_context);
				 addr.off = EIP_reg(&DEBUG_context);

				 if (ISV86(&DEBUG_context))
				     addr.seg |= (DWORD)(pTask?(pTask->hModule):0)<<16;
				 GlobalUnlock16( GetCurrentTask() );
                                 DEBUG_AddBreakpoint( &addr );
                               }

info_command:
      tINFO tBREAK tEOL         { DEBUG_InfoBreakpoints(); }
    | tINFO tCLASS expr_value tEOL    { CLASS_DumpClass( (CLASS *)$3 ); 
 					     DEBUG_FreeExprMem(); }
    | tINFO tSHARE tEOL		{ DEBUG_InfoShare(); }
    | tINFO tMODULE expr_value tEOL   { NE_DumpModule( $3 ); 
 					     DEBUG_FreeExprMem(); }
    | tINFO tQUEUE expr_value tEOL    { QUEUE_DumpQueue( $3 ); 
 					     DEBUG_FreeExprMem(); }
    | tINFO tREGS tEOL          { DEBUG_InfoRegisters(); }
    | tINFO tSEGMENTS expr_value tEOL { LDT_Print( SELECTOR_TO_ENTRY($3), 1 ); 
 					     DEBUG_FreeExprMem(); }
    | tINFO tSEGMENTS tEOL      { LDT_Print( 0, -1 ); }
    | tINFO tSTACK tEOL         { DEBUG_InfoStack(); }
    | tINFO tMAPS tEOL          { VIRTUAL_Dump(); }
    | tINFO tWND expr_value tEOL      { WIN_DumpWindow( $3 ); 
 					     DEBUG_FreeExprMem(); }
    | tINFO tLOCAL tEOL         { DEBUG_InfoLocals(); }
    | tINFO tDISPLAY tEOL       { DEBUG_InfoDisplay(); }

walk_command:
      tWALK tCLASS tEOL         { CLASS_WalkClasses(); }
    | tWALK tMODULE tEOL        { NE_WalkModules(); }
    | tWALK tQUEUE tEOL         { QUEUE_WalkQueues(); }
    | tWALK tWND tEOL           { WIN_WalkWindows( 0, 0 ); }
    | tWALK tWND tNUM tEOL      { WIN_WalkWindows( $3, 0 ); }


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
    expr			 { $$ = DEBUG_EvalExpr($1); }

expr_value:
      expr        { DBG_ADDR addr  = DEBUG_EvalExpr($1);
		    $$ = addr.off ? *(unsigned int *) addr.off : 0; }
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
    | tIDENTIFIER '(' expr ',' expr ')'	 { $$ = DEBUG_CallExpr($1, 2, $3, 
							       $5); } 
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

void 
issue_prompt(){
#ifdef DONT_USE_READLINE
	fprintf(stderr,"Wine-dbg>");
#endif
}

void mode_command(int newmode)
{
    if ((newmode == 16) || (newmode == 32)) dbg_mode = newmode;
    else fprintf(stderr,"Invalid mode (use 16 or 32)\n");
}


/***********************************************************************
 *           DEBUG_Main
 *
 * Debugger main loop.
 */
static void DEBUG_Main( int signal )
{
    static int loaded_symbols = 0;
    static BOOL frozen = FALSE;
    static BOOL in_debugger = FALSE;
    char SymbolTableFile[256];
    int newmode;
    BOOL ret_ok;
#ifdef YYDEBUG
    yydebug = 0;
#endif

    if (in_debugger)
    {
        fprintf( stderr, "Segmentation fault inside debugger, exiting.\n" );
        exit(1);
    }
    in_debugger = TRUE;
    yyin = stdin;

    DEBUG_SetBreakpoints( FALSE );

    if (!loaded_symbols)
    {
        loaded_symbols++;

        if ( !frozen )
        {
            CLIENT_DebuggerRequest( DEBUGGER_FREEZE_ALL );
            frozen = TRUE;
        }

#ifdef DBG_need_heap
	/*
	 * Initialize the debugger heap.
	 */
	dbg_heap = HeapCreate(HEAP_NO_SERIALIZE, 0x1000, 0x8000000); /* 128MB */
#endif

	/*
	 * Initialize the type handling stuff.
	 */
	DEBUG_InitTypes();

	/*
	 * In some cases we can read the stabs information directly
	 * from the executable.  If this is the case, we don't need
	 * to bother with trying to read a symbol file, as the stabs
	 * also have line number and local variable information.
	 * As long as gcc is used for the compiler, stabs will
	 * be the default.  On SVr4, DWARF could be used, but we
	 * don't grok that yet, and in this case we fall back to using
	 * the wine.sym file.
	 */
	if( DEBUG_ReadExecutableDbgInfo() == FALSE )
        {
	    char *symfilename = "wine.sym";
	    struct stat statbuf;
	    if (-1 == stat(symfilename, &statbuf) )
                symfilename = LIBDIR "wine.sym";

	    PROFILE_GetWineIniString( "wine", "SymbolTableFile", symfilename,
                                     SymbolTableFile, sizeof(SymbolTableFile));
	    DEBUG_ReadSymbolTable( SymbolTableFile );
        }

        DEBUG_LoadEntryPoints();
	DEBUG_ProcessDeferredDebug();
    }

#if 0
    fprintf(stderr, "Entering debugger 	PC=%x, mode=%d, count=%d\n",
	    EIP_reg(&DEBUG_context),
	    dbg_exec_mode, dbg_exec_count);
    
    sleep(1);
#endif

    if ((signal != SIGTRAP) || !DEBUG_ShouldContinue( dbg_exec_mode, 
						      &dbg_exec_count ))
    {
        DBG_ADDR addr;
        TDB *pTask = (TDB*)GlobalLock16( GetCurrentTask() );

        addr.seg = CS_reg(&DEBUG_context);
        addr.off = EIP_reg(&DEBUG_context);
        if (ISV86(&DEBUG_context)) addr.seg |= (DWORD)(pTask?(pTask->hModule):0)<<16;
	addr.type = NULL;
        DBG_FIX_ADDR_SEG( &addr, 0 );

        GlobalUnlock16( GetCurrentTask() );

        if ( !frozen )
        {
            CLIENT_DebuggerRequest( DEBUGGER_FREEZE_ALL );
            frozen = TRUE;
        }

        /* Put the display in a correct state */
	USER_Driver->pBeginDebugging();

        newmode = ISV86(&DEBUG_context) ? 16 : IS_SELECTOR_32BIT(addr.seg) ? 32 : 16;
        if (newmode != dbg_mode)
            fprintf(stderr,"In %d bit mode.\n", dbg_mode = newmode);

	DEBUG_DoDisplay();

        if (signal != SIGTRAP)  /* This is a real crash, dump some info */
        {
            DEBUG_InfoRegisters();
            DEBUG_InfoStack();
            if (dbg_mode == 16)
            {
                LDT_Print( SELECTOR_TO_ENTRY(DS_reg(&DEBUG_context)), 1 );
                if (ES_reg(&DEBUG_context) != DS_reg(&DEBUG_context))
                    LDT_Print( SELECTOR_TO_ENTRY(ES_reg(&DEBUG_context)), 1 );
            }
            DEBUG_BackTrace();
        }
	else
	{
	  /*
	   * Do a quiet backtrace so that we have an idea of what the situation
	   * is WRT the source files.
	   */
	    DEBUG_SilentBackTrace();
	}

	if ((signal != SIGTRAP) ||
            (dbg_exec_mode == EXEC_STEPI_OVER) ||
            (dbg_exec_mode == EXEC_STEPI_INSTR))
        {
	    /* Show where we crashed */
	    curr_frame = 0;
	    DEBUG_PrintAddress( &addr, dbg_mode, TRUE );
	    fprintf(stderr,":  ");
	    if (DBG_CHECK_READ_PTR( &addr, 1 ))
	      {
		DEBUG_Disasm( &addr, TRUE );
		fprintf(stderr,"\n");
	      }
        }

        ret_ok = 0;
        do
        {
            issue_prompt();
            yyparse();
            flush_symbols();
            addr.seg = CS_reg(&DEBUG_context) | (addr.seg&0xffff0000);
            addr.off = EIP_reg(&DEBUG_context);
            DBG_FIX_ADDR_SEG( &addr, 0 );
            ret_ok = DEBUG_ValidateRegisters();
            if (ret_ok) ret_ok = DBG_CHECK_READ_PTR( &addr, 1 );
        } while (!ret_ok);
    }

    dbg_exec_mode = DEBUG_RestartExecution( dbg_exec_mode, dbg_exec_count );
    /*
     * This will have gotten absorbed into the breakpoint info
     * if it was used.  Otherwise it would have been ignored.
     * In any case, we don't mess with it any more.
     */
    if( dbg_exec_mode == EXEC_CONT )
      {
	dbg_exec_count = 0;

        if ( frozen )
        {
            CLIENT_DebuggerRequest( DEBUGGER_UNFREEZE_ALL );
            frozen = FALSE;
        }
      }

    in_debugger = FALSE;

    USER_Driver->pEndDebugging();
}



/***********************************************************************
 *           DebugBreak   (KERNEL.203) (KERNEL32.181)
 */
void DebugBreak16( CONTEXT *regs )
{
    char module[10];
    if (!GetModuleName16( GetCurrentTask(), module, sizeof(module) ))
        strcpy( module, "???" );
    fprintf( stderr, "%s called DebugBreak\n", module );
    DEBUG_context = *regs;
    DEBUG_Main( SIGTRAP );
}


void ctx_debug( int signal, CONTEXT *regs )
{
    DEBUG_context = *regs;
    DEBUG_Main( signal );
    *regs = DEBUG_context;
}

void wine_debug( int signal, SIGCONTEXT *regs )
{
#if 0
    DWORD *stack = (DWORD *)ESP_sig(regs);
    *(--stack) = 0;
    *(--stack) = 0;
    *(--stack) = EH_NONCONTINUABLE;
    *(--stack) = EXCEPTION_ACCESS_VIOLATION;
    *(--stack) = EIP_sig(regs);
    ESP_sig(regs) = (DWORD)stack;
    EIP_sig(regs) = (DWORD)RaiseException;
#else
    DEBUG_SetSigContext( regs );
    DEBUG_Main( signal );
    DEBUG_GetSigContext( regs );
#endif
}

int yyerror(char * s)
{
	fprintf(stderr,"%s\n", s);
        return 0;
}
