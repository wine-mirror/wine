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
#include "class.h"
#include "module.h"
#include "options.h"
#include "queue.h"
#include "win.h"
#include "debugger.h"

extern FILE * yyin;
unsigned int dbg_mode = 0;
int curr_frame = 0;

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

%token tCONT tSTEP tLIST tNEXT tQUIT tHELP tBACKTRACE tINFO tWALK tUP tDOWN
%token tENABLE tDISABLE tBREAK tDELETE tSET tMODE tPRINT tEXAM tDEFINE tABORT
%token tCLASS tMODULE tSTACK tSEGMENTS tREGS tWND tQUEUE tLOCAL
%token tNO_SYMBOL tEOL
%token tSYMBOLFILE
%token tFRAME


%token <string> tIDENTIFIER
%token <integer> tNUM tFORMAT
%token <reg> tREG

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

input: line                    { issue_prompt(); }
    | input line               { issue_prompt(); }

line: command 
    | tEOL
    | error tEOL               { yyerrok; }

command:
      tQUIT tEOL               { exit(0); }
    | tHELP tEOL               { DEBUG_Help(); }
    | tCONT tEOL               { dbg_exec_mode = EXEC_CONT; return 0; }
    | tSTEP tEOL               { dbg_exec_mode = EXEC_STEP_INSTR; return 0; }
    | tNEXT tEOL               { dbg_exec_mode = EXEC_STEP_OVER; return 0; }
    | tLIST tEOL               { DEBUG_List( NULL, 15 ); }
    | tLIST addr tEOL          { DEBUG_List( &$2, 15 ); }
    | tABORT tEOL              { kill(getpid(), SIGABRT); }
    | tSYMBOLFILE tIDENTIFIER tEOL  { DEBUG_ReadSymbolTable( $2 ); }
    | tDEFINE tIDENTIFIER addr tEOL { DEBUG_AddSymbol( $2, &$3, NULL ); }
    | tMODE tNUM tEOL          { mode_command($2); }
    | tENABLE tNUM tEOL        { DEBUG_EnableBreakpoint( $2, TRUE ); }
    | tDISABLE tNUM tEOL       { DEBUG_EnableBreakpoint( $2, FALSE ); }
    | tDELETE tBREAK tNUM tEOL { DEBUG_DelBreakpoint( $3 ); }
    | tBACKTRACE tEOL	       { DEBUG_BackTrace(); }
    | tUP tEOL		       { DEBUG_SetFrame( curr_frame + 1 );  }
    | tUP tNUM tEOL	       { DEBUG_SetFrame( curr_frame + $2 ); }
    | tDOWN tEOL	       { DEBUG_SetFrame( curr_frame - 1 );  }
    | tDOWN tNUM tEOL	       { DEBUG_SetFrame( curr_frame - $2 ); }
    | tFRAME expr tEOL	       { DEBUG_SetFrame( $2 ); }
    | set_command
    | x_command
    | print_command
    | break_command
    | info_command
    | walk_command

set_command:
      tSET tREG '=' expr tEOL	     { DEBUG_SetRegister( $2, $4 ); }
    | tSET '*' addr '=' expr tEOL    { DEBUG_WriteMemory( &$3, $5 ); }
    | tSET tIDENTIFIER '=' addr tEOL { if (!DEBUG_SetSymbolValue( $2, &$4 ))
                                       {
                                           fprintf( stderr,
                                                 "Symbol %s not found\n", $2 );
                                           YYERROR;
                                       }
                                     }

x_command:
      tEXAM addr tEOL          { DEBUG_ExamineMemory( &$2, 1, 'x'); }
    | tEXAM tFORMAT addr tEOL  { DEBUG_ExamineMemory( &$3, $2>>8, $2&0xff ); }

print_command:
      tPRINT addr tEOL         { DEBUG_Print( &$2, 1, 'x' ); }
    | tPRINT tFORMAT addr tEOL { DEBUG_Print( &$3, $2 >> 8, $2 & 0xff ); }

break_command:
      tBREAK '*' addr tEOL     { DEBUG_AddBreakpoint( &$3 ); }
    | tBREAK symbol tEOL       { DEBUG_AddBreakpoint( &$2 ); }
    | tBREAK symbol '+' expr tEOL { DBG_ADDR addr = $2;
                                    addr.off += $4;
                                    DEBUG_AddBreakpoint( &addr ); 
                                  }
    | tBREAK tEOL              { DBG_ADDR addr = { CS_reg(DEBUG_context),
                                                   EIP_reg(DEBUG_context) };
                                 DEBUG_AddBreakpoint( &addr );
                               }

info_command:
      tINFO tBREAK tEOL         { DEBUG_InfoBreakpoints(); }
    | tINFO tCLASS expr tEOL    { CLASS_DumpClass( (CLASS *)$3 ); }
    | tINFO tMODULE expr tEOL   { MODULE_DumpModule( $3 ); }
    | tINFO tQUEUE expr tEOL    { QUEUE_DumpQueue( $3 ); }
    | tINFO tREGS tEOL          { DEBUG_InfoRegisters(); }
    | tINFO tSEGMENTS expr tEOL { LDT_Print( SELECTOR_TO_ENTRY($3), 1 ); }
    | tINFO tSEGMENTS tEOL      { LDT_Print( 0, -1 ); }
    | tINFO tSTACK tEOL         { DEBUG_InfoStack(); }
    | tINFO tWND expr tEOL      { WIN_DumpWindow( $3 ); } 
    | tINFO tLOCAL tEOL         { DEBUG_InfoLocals(); }

walk_command:
      tWALK tCLASS tEOL         { CLASS_WalkClasses(); }
    | tWALK tMODULE tEOL        { MODULE_WalkModules(); }
    | tWALK tQUEUE tEOL         { QUEUE_WalkQueues(); }
    | tWALK tWND tEOL           { WIN_WalkWindows( 0, 0 ); }
    | tWALK tWND tNUM tEOL      { WIN_WalkWindows( $3, 0 ); }

symbol:
    tIDENTIFIER            { if (!DEBUG_GetSymbolValue( $1, -1, &$$ ))
                             {
                               fprintf( stderr, "Symbol %s not found\n", $1 );
                               YYERROR;
                             }
                           }
    | tIDENTIFIER ':' tNUM { if (!DEBUG_GetSymbolValue( $1, $3, &$$ ))
                             {
                               fprintf( stderr, "No code at %s:%d\n", $1, $3 );
                               YYERROR;
                             }
                           }

addr:
      expr                       { $$.seg = 0xffffffff; $$.off = $1; }
    | segaddr                    { $$ = $1; }

segaddr:
      expr ':' expr              { $$.seg = $1; $$.off = $3; }
    | symbol                     { $$ = $1; }
    | symbol '+' expr            { $$ = $1; $$.off += $3; }
    | symbol '-' expr            { $$ = $1; $$.off -= $3; }

expr:
      tNUM                       { $$ = $1; }
    | tREG                       { $$ = DEBUG_GetRegister($1); }
    | expr OP_LOR expr           { $$ = $1 || $3; }
    | expr OP_LAND expr          { $$ = $1 && $3; }
    | expr '|' expr              { $$ = $1 | $3; }
    | expr '&' expr              { $$ = $1 & $3; }
    | expr '^' expr              { $$ = $1 ^ $3; }
    | expr OP_EQ expr            { $$ = $1 == $3; }
    | expr '>' expr              { $$ = $1 > $3; }
    | expr '<' expr              { $$ = $1 < $3; }
    | expr OP_GE expr            { $$ = $1 >= $3; }
    | expr OP_LE expr            { $$ = $1 <= $3; }
    | expr OP_NE expr            { $$ = $1 != $3; }
    | expr OP_SHL expr           { $$ = (unsigned)$1 << $3; }
    | expr OP_SHR expr           { $$ = (unsigned)$1 >> $3; }
    | expr '+' expr              { $$ = $1 + $3; }
    | expr '-' expr              { $$ = $1 - $3; }
    | expr '*' expr              { $$ = $1 * $3; }
    | expr '/' expr              { if ($3) 
                                       if ($3 == -1 && $1 == 0x80000000l)
                                           yyerror ("Division overflow");
                                       else $$ = $1 / $3;
                                   else yyerror ("Division by zero");
                                 }
    | expr '%' expr              { if ($3) 
                                       if ($3 == -1 && $1 == 0x80000000l)
                                           $$ = 0; /* A sensible result in this case.  */
                                       else $$ = $1 % $3;
                                   else yyerror ("Division by zero");
                                 }
    | '-' expr %prec OP_SIGN     { $$ = -$2; }
    | '+' expr %prec OP_SIGN     { $$ = $2; }
    | '!' expr                   { $$ = !$2; }
    | '~' expr                   { $$ = ~$2; }
    | '(' expr ')'               { $$ = $2; }
/* For parser technical reasons we can't use "addr" here.  */
    | '*' expr %prec OP_DEREF    { DBG_ADDR addr = { 0xffffffff, $2 };
                                   $$ = DEBUG_ReadMemory( &addr ); }
    | '*' segaddr %prec OP_DEREF { $$ = DEBUG_ReadMemory( &$2 ); }
	
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
 *           DEBUG_EnterDebugger
 *
 * Force an entry into the debugger.
 */
void DEBUG_EnterDebugger(void)
{
    kill( getpid(), SIGHUP );
}


/***********************************************************************
 *           DebugBreak16   (KERNEL.203)
 */
void DebugBreak16( SIGCONTEXT *regs )
{
    const char *module = MODULE_GetModuleName( GetExePtr(GetCurrentTask()) );
    fprintf( stderr, "%s called DebugBreak\n", module ? module : "???" );
    wine_debug( SIGTRAP, regs );
}


void wine_debug( int signal, SIGCONTEXT *regs )
{
    static int loaded_symbols = 0;
    char SymbolTableFile[256];
    int instr_len = 0, newmode;
    BOOL32 ret_ok;
#ifdef YYDEBUG
    yydebug = 0;
#endif

    yyin = stdin;
    DEBUG_context = regs;

    DEBUG_SetBreakpoints( FALSE );

    if (!loaded_symbols)
    {
        loaded_symbols++;
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
	    PROFILE_GetWineIniString( "wine", "SymbolTableFile", "wine.sym",
                                     SymbolTableFile, sizeof(SymbolTableFile));
	    DEBUG_ReadSymbolTable( SymbolTableFile );
        }

	/*
	 * Read COFF, MSC, etc debug information that we noted when we
	 * started up the executable.
	 */
	DEBUG_ProcessDeferredDebug();

        DEBUG_LoadEntryPoints();
    }

    if ((signal != SIGTRAP) || !DEBUG_ShouldContinue( regs, dbg_exec_mode ))
    {
        DBG_ADDR addr;

        addr.seg = CS_reg(DEBUG_context);
        addr.off = EIP_reg(DEBUG_context);
        DBG_FIX_ADDR_SEG( &addr, 0 );

        /* Put the display in a correct state */

        XUngrabPointer( display, CurrentTime );
        XUngrabServer( display );
        XFlush( display );

        if (!addr.seg) newmode = 32;
        else newmode = (GET_SEL_FLAGS(addr.seg) & LDT_FLAGS_32BIT) ? 32 : 16;

        if (newmode != dbg_mode)
            fprintf(stderr,"In %d bit mode.\n", dbg_mode = newmode);

        if (signal != SIGTRAP)  /* This is a real crash, dump some info */
        {
            DEBUG_InfoRegisters();
            DEBUG_InfoStack();
            if (dbg_mode == 16)
            {
                LDT_Print( SELECTOR_TO_ENTRY(DS_reg(DEBUG_context)), 1 );
                if (ES_reg(DEBUG_context) != DS_reg(DEBUG_context))
                    LDT_Print( SELECTOR_TO_ENTRY(ES_reg(DEBUG_context)), 1 );
            }
            DEBUG_BackTrace();
        }

        /* Show where we crashed */
        curr_frame = 0;
        DEBUG_PrintAddress( &addr, dbg_mode, TRUE );
        fprintf(stderr,":  ");
        if (DBG_CHECK_READ_PTR( &addr, 1 ))
        {
            DEBUG_Disasm( &addr );
            fprintf(stderr,"\n");
            instr_len = addr.off - EIP_reg(DEBUG_context);
        }

        ret_ok = 0;
        do
        {
            issue_prompt();
            yyparse();
            flush_symbols();
            addr.seg = CS_reg(DEBUG_context);
            addr.off = EIP_reg(DEBUG_context);
            DBG_FIX_ADDR_SEG( &addr, 0 );
            ret_ok = DEBUG_ValidateRegisters();
            if (ret_ok) ret_ok = DBG_CHECK_READ_PTR( &addr, 1 );
        } while (!ret_ok);
    }

    DEBUG_RestartExecution( regs, dbg_exec_mode, instr_len );
}


int yyerror(char * s)
{
	fprintf(stderr,"%s\n", s);
        return 0;
}

