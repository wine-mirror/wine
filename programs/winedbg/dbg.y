%{
/*
 * Parser for command lines in the Wine debugger
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Morten Welinder
 * Copyright 2000 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debugger.h"
#include "wine/exception.h"
#include "expr.h"

int dbg_lex(void);
static int dbg_error(const char*);
static void parser(const char*);

%}

%define api.prefix {dbg_}

%union
{
    struct dbg_lvalue   lvalue;
    char*               string;
    dbg_lgint_t         integer;
    IMAGEHLP_LINE64     listing;
    struct expr*        expression;
    struct dbg_type     type;
    struct list_string* strings;
}

%token tCONT tPASS tSTEP tLIST tNEXT tQUIT tHELP tBACKTRACE tALL tINFO tUP tDOWN
%token tENABLE tDISABLE tBREAK tHBREAK tWATCH tRWATCH tDELETE tSET tPRINT tEXAM
%token tABORT tECHO
%token tCLASS tMAPS tSTACK tSEGMENTS tSYMBOL tREGS tALLREGS tWND tLOCAL tEXCEPTION
%token tPROCESS tTHREAD tEOL tEOF
%token tFRAME tSHARE tMODULE tCOND tDISPLAY tUNDISPLAY tDISASSEMBLE tSYSTEM
%token tSTEPI tNEXTI tFINISH tSHOW tDIR tWHATIS tSOURCE
%token <string> tPATH tIDENTIFIER tSTRING tINTVAR
%token <integer> tNUM tFORMAT
%token <type> tTYPEDEF
%token tSYMBOLFILE tEXECFILE tRUN tATTACH tDETACH tKILL tMAINTENANCE tTYPE tMINIDUMP
%token tNOPROCESS tWOW

/* can be prefixed by module name */
%token <string> tVOID tCHAR tWCHAR tSHORT tINT tLONG tFLOAT tDOUBLE tUNSIGNED tSIGNED
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

%type <expression> expr lvalue
%type <lvalue> expr_lvalue lvalue_addr
%type <integer> expr_rvalue
%type <string> pathname identifier
%type <listing> list_arg
%type <type> type_expr
%type <strings> list_of_words

%%

input:
      line
    | input line
    ;

line:
      command tEOL              { expr_free_all(); }
    | tEOL
    | tEOF                      { return 1; }
    | error tEOL               	{ yyerrok; expr_free_all(); }
    ;

command:
      tQUIT                     { return 1; }
    | tHELP                     { print_help(); }
    | tHELP tINFO               { info_help(); }
    | tPASS                     { dbg_wait_next_exception(DBG_EXCEPTION_NOT_HANDLED, 0, 0); }
    | tCONT                     { dbg_wait_next_exception(DBG_CONTINUE, 1,  dbg_exec_cont); }
    | tCONT tNUM              	{ dbg_wait_next_exception(DBG_CONTINUE, $2, dbg_exec_cont); }
    | tSTEP                    	{ dbg_wait_next_exception(DBG_CONTINUE, 1,  dbg_exec_step_into_line); }
    | tSTEP tNUM                { dbg_wait_next_exception(DBG_CONTINUE, $2, dbg_exec_step_into_line); }
    | tNEXT                     { dbg_wait_next_exception(DBG_CONTINUE, 1,  dbg_exec_step_over_line); }
    | tNEXT tNUM                { dbg_wait_next_exception(DBG_CONTINUE, $2, dbg_exec_step_over_line); }
    | tSTEPI                    { dbg_wait_next_exception(DBG_CONTINUE, 1,  dbg_exec_step_into_insn); }
    | tSTEPI tNUM               { dbg_wait_next_exception(DBG_CONTINUE, $2, dbg_exec_step_into_insn); }
    | tNEXTI                    { dbg_wait_next_exception(DBG_CONTINUE, 1,  dbg_exec_step_over_insn); }
    | tNEXTI tNUM               { dbg_wait_next_exception(DBG_CONTINUE, $2, dbg_exec_step_over_insn); }
    | tFINISH     	       	{ dbg_wait_next_exception(DBG_CONTINUE, 0,  dbg_exec_finish); }
    | tABORT                   	{ abort(); }
    | tBACKTRACE     	       	{ stack_backtrace(dbg_curr_tid); }
    | tBACKTRACE tNUM          	{ stack_backtrace($2); }
    | tBACKTRACE tALL           { stack_backtrace(-1); }
    | tUP     		       	{ stack_set_frame(dbg_curr_thread->curr_frame + 1);  }
    | tUP tNUM     	       	{ stack_set_frame(dbg_curr_thread->curr_frame + $2); }
    | tDOWN     	       	{ stack_set_frame(dbg_curr_thread->curr_frame - 1);  }
    | tDOWN tNUM     	       	{ stack_set_frame(dbg_curr_thread->curr_frame - $2); }
    | tFRAME tNUM              	{ stack_set_frame($2); }
    | tSHOW tDIR     	       	{ source_show_path(); }
    | tDIR pathname            	{ source_add_path($2); }
    | tDIR     		       	{ source_nuke_path(dbg_curr_process); }
    | tCOND tNUM               	{ break_add_condition($2, NULL); }
    | tCOND tNUM expr     	{ break_add_condition($2, $3); }
    | tSOURCE pathname          { parser($2); }
    | tSYMBOLFILE pathname     	{ symbol_read_symtable($2, 0); }
    | tSYMBOLFILE pathname expr_rvalue { symbol_read_symtable($2, $3); }
    | tWHATIS expr_lvalue       { dbg_printf("type = "); types_print_type(&$2.type, FALSE, NULL); dbg_printf("\n"); }
    | tATTACH tNUM              { if (dbg_attach_debuggee($2)) dbg_active_wait_for_first_exception(); }
    | tATTACH pathname          { minidump_reload($2); }
    | tDETACH                   { dbg_curr_process->process_io->close_process(dbg_curr_process, FALSE); }
    | tTHREAD tNUM              { dbg_set_curr_thread($2); }
    | tKILL                     { dbg_curr_process->process_io->close_process(dbg_curr_process, TRUE); }
    | tMINIDUMP pathname        { minidump_write($2, (dbg_curr_thread && dbg_curr_thread->in_exception) ? &dbg_curr_thread->excpt_record : NULL); }
    | tECHO tSTRING             { dbg_printf("%s\n", $2); }
    | tEXECFILE pathname        { dbg_set_exec_file($2); }
    | run_command
    | list_command
    | disassemble_command
    | set_command
    | x_command
    | print_command     
    | break_command
    | watch_command
    | display_command
    | info_command
    | maintenance_command
    | noprocess_state
    ;

pathname:
      identifier                { $$ = $1; }
    | tSTRING                   { $$ = $1; }
    | tPATH                     { $$ = $1; }
    ;

identifier:
      tIDENTIFIER              { $$ = $1; }
    ;

list_arg:
      tNUM		        { $$.FileName = NULL; $$.LineNumber = $1; }
    | pathname ':' tNUM	        { $$.FileName = $1; $$.LineNumber = $3; }
    | identifier	        { symbol_get_line(NULL, $1, &$$); }
    | pathname ':' identifier   { symbol_get_line($1, $3, &$$); }
    | '*' expr_lvalue	        { DWORD disp; ADDRESS64 addr; $$.SizeOfStruct = sizeof($$);
                                  types_extract_as_address(&$2, &addr);
                                  SymGetLineFromAddr64(dbg_curr_process->handle, (ULONG_PTR)memory_to_linear_addr(& addr), &disp, & $$); }
    ;

run_command:
      tRUN list_of_words        { dbg_run_debuggee($2); }
    ;

list_of_words:
      %empty                    { $$ = NULL; }
    | tSTRING list_of_words     { $$ = (struct list_string*)lexeme_alloc_size(sizeof(*$$)); $$->next = $2; $$->string = $1; }
    ;

list_command:
      tLIST                     { source_list(NULL, NULL, 10); }
    | tLIST '-'                 { source_list(NULL,  NULL, -10); }
    | tLIST list_arg            { source_list(& $2, NULL, 10); }
    | tLIST ',' list_arg        { source_list(NULL, & $3, -10); }
    | tLIST list_arg ',' list_arg      { source_list(& $2, & $4, 0); }
    ;

disassemble_command:
      tDISASSEMBLE              { memory_disassemble(NULL, NULL, 10); }
    | tDISASSEMBLE expr_lvalue  { memory_disassemble(&$2, NULL, 10); }
    | tDISASSEMBLE expr_lvalue ',' expr_lvalue { memory_disassemble(&$2, &$4, 0); }
    ;

set_command:
      tSET lvalue_addr '=' expr_lvalue { types_store_value(&$2, &$4); }
    | tSET '+' tIDENTIFIER      { info_wine_dbg_channel(TRUE, NULL, $3); }
    | tSET '+' tALL             { info_wine_dbg_channel(TRUE, NULL, "all"); }
    | tSET '-' tIDENTIFIER      { info_wine_dbg_channel(FALSE, NULL, $3); }
    | tSET '-' tALL             { info_wine_dbg_channel(FALSE, NULL, "all"); }
    | tSET tIDENTIFIER '+' tIDENTIFIER { info_wine_dbg_channel(TRUE, $2, $4); }
    | tSET tIDENTIFIER '+' tALL        { info_wine_dbg_channel(TRUE, $2, "all"); }
    | tSET tIDENTIFIER '-' tIDENTIFIER { info_wine_dbg_channel(FALSE, $2, $4); }
    | tSET tIDENTIFIER '-' tALL        { info_wine_dbg_channel(FALSE, $2, "all"); }
    | tSET '!' tIDENTIFIER tIDENTIFIER  { dbg_set_option($3, $4); }
    | tSET '!' tIDENTIFIER      { dbg_set_option($3, NULL); }
    ;

x_command:
      tEXAM expr_lvalue         { memory_examine(&$2, 1, 'x'); }
    | tEXAM tFORMAT expr_lvalue { memory_examine(&$3, $2 >> 8, $2 & 0xff); }
    ;

print_command:
      tPRINT expr_lvalue         { print_value(&$2, 0, 0); }
    | tPRINT tFORMAT expr_lvalue { if (($2 >> 8) == 1) print_value(&$3, $2 & 0xff, 0); else dbg_printf("Count is meaningless in print command\n"); }
    | tPRINT type_expr           { types_print_type(&$2, TRUE, NULL); dbg_printf("\n"); }
    ;

break_command:
      tBREAK '*' expr_lvalue    { break_add_break_from_lvalue(&$3, TRUE); }
    | tBREAK identifier         { break_add_break_from_id($2, -1, TRUE); }
    | tBREAK pathname ':' tNUM  { break_add_break_from_lineno($2, $4, TRUE); }
    | tBREAK tNUM               { break_add_break_from_lineno(NULL, $2, TRUE); }
    | tBREAK                    { break_add_break_from_lineno(NULL, -1, TRUE); }
    | tHBREAK '*' expr_lvalue   { break_add_break_from_lvalue(&$3, FALSE); }
    | tHBREAK identifier        { break_add_break_from_id($2, -1, FALSE); }
    | tHBREAK pathname ':' tNUM { break_add_break_from_lineno($2, $4, FALSE); }
    | tHBREAK tNUM              { break_add_break_from_lineno(NULL, $2, FALSE); }
    | tHBREAK                   { break_add_break_from_lineno(NULL, -1, FALSE); }
    | tENABLE tNUM              { break_enable_xpoint($2, TRUE); }
    | tENABLE tBREAK tNUM     	{ break_enable_xpoint($3, TRUE); }
    | tDISABLE tNUM             { break_enable_xpoint($2, FALSE); }
    | tDISABLE tBREAK tNUM     	{ break_enable_xpoint($3, FALSE); }
    | tDELETE tNUM      	{ break_delete_xpoint($2); }
    | tDELETE tBREAK tNUM      	{ break_delete_xpoint($3); }
    ;

watch_command:
      tWATCH expr_lvalue        { break_add_watch(&$2, TRUE); }
    | tRWATCH expr_lvalue       { break_add_watch(&$2, FALSE); }
    ;


display_command:
      tDISPLAY     	       	{ display_print(); }
    | tDISPLAY expr            	{ display_add($2, 1, 0); }
    | tDISPLAY tFORMAT expr     { display_add($3, $2 >> 8, $2 & 0xff); }
    | tENABLE tDISPLAY tNUM     { display_enable($3, TRUE); }
    | tDISABLE tDISPLAY tNUM    { display_enable($3, FALSE); }
    | tDELETE tDISPLAY tNUM     { display_delete($3); }
    | tDELETE tDISPLAY         	{ display_delete(-1); }
    | tUNDISPLAY tNUM          	{ display_delete($2); }
    | tUNDISPLAY               	{ display_delete(-1); }
    ;

info_command:
      tINFO tBREAK              { break_info(); }
    | tINFO tSHARE              { info_win32_module(0, FALSE); }
    | tINFO tWOW tSHARE         { info_win32_module(0, TRUE); }
    | tINFO tSHARE expr_rvalue  { info_win32_module($3, FALSE); }
    | tINFO tREGS               { dbg_curr_process->be_cpu->print_context(dbg_curr_thread->handle, &dbg_context, 0); }
    | tINFO tALLREGS            { dbg_curr_process->be_cpu->print_context(dbg_curr_thread->handle, &dbg_context, 1); }
    | tINFO tSEGMENTS expr_rvalue { info_win32_segments($3 >> 3, 1); }
    | tINFO tSEGMENTS           { info_win32_segments(0, -1); }
    | tINFO tSTACK tNUM         { stack_info($3); }
    | tINFO tSTACK              { stack_info(-1); }
    | tINFO tSYMBOL tSTRING     { symbol_info($3); }
    | tINFO tLOCAL              { symbol_info_locals(); }
    | tINFO tDISPLAY            { display_info(); }
    | tINFO tCLASS              { info_win32_class(NULL, NULL); }
    | tINFO tCLASS tSTRING     	{ info_win32_class(NULL, $3); }
    | tINFO tWND                { info_win32_window(NULL, FALSE); }
    | tINFO tWND expr_rvalue    { info_win32_window((HWND)(DWORD_PTR)$3, FALSE); }
    | tINFO '*' tWND            { info_win32_window(NULL, TRUE); }
    | tINFO '*' tWND expr_rvalue { info_win32_window((HWND)(DWORD_PTR)$4, TRUE); }
    | tINFO tPROCESS            { info_win32_processes(); }
    | tINFO tTHREAD             { info_win32_threads(); }
    | tINFO tFRAME              { info_win32_frame_exceptions(dbg_curr_tid); }
    | tINFO tFRAME expr_rvalue  { info_win32_frame_exceptions($3); }
    | tINFO tMAPS               { info_win32_virtual(dbg_curr_pid); }
    | tINFO tMAPS expr_rvalue   { info_win32_virtual($3); }
    | tINFO tEXCEPTION          { info_win32_exception(); }
    | tINFO tSYSTEM             { info_win32_system(); }
    ;

maintenance_command:
      tMAINTENANCE tTYPE        { print_types(); }
    | tMAINTENANCE tMODULE tSTRING { tgt_module_load($3, FALSE); }
    | tMAINTENANCE '*' tMODULE tSTRING { tgt_module_load($4, TRUE); }
    ;

noprocess_state:
      tNOPROCESS                 {} /* <CR> shall not barf anything */
    | tNOPROCESS tBACKTRACE tALL { stack_backtrace(-1); } /* can backtrace all threads with no attached process */
    | tNOPROCESS tSTRING         { dbg_printf("No process loaded, cannot execute '%s'\n", $2); }
    ;

type_expr:
      tVOID                     { if (!types_find_basic(L"void",                   $1, &$$)) YYERROR; }
    | tCHAR                     { if (!types_find_basic(L"char",                   $1, &$$)) YYERROR; }
    | tWCHAR                    { if (!types_find_basic(L"WCHAR",                  $1, &$$)) YYERROR; }
    | tSIGNED tCHAR             { if (!types_find_basic(L"signed char",            $1, &$$)) YYERROR; }
    | tUNSIGNED tCHAR           { if (!types_find_basic(L"unsigned char",          $1, &$$)) YYERROR; }
    | tSHORT tINT               { if (!types_find_basic(L"short int",              $1, &$$)) YYERROR; }
    | tSHORT                    { if (!types_find_basic(L"short int",              $1, &$$)) YYERROR; }
    | tSIGNED tSHORT tINT       { if (!types_find_basic(L"short int",              $1, &$$)) YYERROR; }
    | tSIGNED tSHORT            { if (!types_find_basic(L"short int",              $1, &$$)) YYERROR; }
    | tSHORT tSIGNED tINT       { if (!types_find_basic(L"short int",              $1, &$$)) YYERROR; }
    | tSHORT tSIGNED            { if (!types_find_basic(L"short int",              $1, &$$)) YYERROR; }
    | tSHORT tUNSIGNED          { if (!types_find_basic(L"unsigned short int",     $1, &$$)) YYERROR; }
    | tSHORT tUNSIGNED tINT     { if (!types_find_basic(L"unsigned short int",     $1, &$$)) YYERROR; }
    | tUNSIGNED tSHORT          { if (!types_find_basic(L"unsigned short int",     $1, &$$)) YYERROR; }
    | tUNSIGNED tSHORT tINT     { if (!types_find_basic(L"unsigned short int",     $1, &$$)) YYERROR; }
    | tINT                      { if (!types_find_basic(L"int",                    $1, &$$)) YYERROR; }
    | tSIGNED tINT              { if (!types_find_basic(L"int",                    $1, &$$)) YYERROR; }
    | tUNSIGNED                 { if (!types_find_basic(L"unsigned int",           $1, &$$)) YYERROR; }
    | tUNSIGNED tINT            { if (!types_find_basic(L"unsigned int",           $1, &$$)) YYERROR; }
    | tLONG                     { if (!types_find_basic(L"long int",               $1, &$$)) YYERROR; }
    | tLONG tINT                { if (!types_find_basic(L"long int",               $1, &$$)) YYERROR; }
    | tSIGNED tLONG             { if (!types_find_basic(L"long int",               $1, &$$)) YYERROR; }
    | tSIGNED tLONG tINT        { if (!types_find_basic(L"long int",               $1, &$$)) YYERROR; }
    | tLONG tSIGNED             { if (!types_find_basic(L"long int",               $1, &$$)) YYERROR; }
    | tLONG tSIGNED tINT        { if (!types_find_basic(L"long int",               $1, &$$)) YYERROR; }
    | tLONG tUNSIGNED           { if (!types_find_basic(L"unsigned long int",      $1, &$$)) YYERROR; }
    | tLONG tUNSIGNED tINT      { if (!types_find_basic(L"unsigned long int",      $1, &$$)) YYERROR; }
    | tUNSIGNED tLONG           { if (!types_find_basic(L"unsigned long int",      $1, &$$)) YYERROR; }
    | tUNSIGNED tLONG tINT      { if (!types_find_basic(L"unsigned long int",      $1, &$$)) YYERROR; }
    | tLONG tLONG               { if (!types_find_basic(L"long long int",          $1, &$$)) YYERROR; }
    | tLONG tLONG tINT          { if (!types_find_basic(L"long long int",          $1, &$$)) YYERROR; }
    | tSIGNED tLONG tLONG       { if (!types_find_basic(L"long long int",          $1, &$$)) YYERROR; }
    | tSIGNED tLONG tLONG tINT  { if (!types_find_basic(L"long long int",          $1, &$$)) YYERROR; }
    | tUNSIGNED tLONG tLONG     { if (!types_find_basic(L"unsigned long long int", $1, &$$)) YYERROR; }
    | tUNSIGNED tLONG tLONG tINT{ if (!types_find_basic(L"unsigned long long int", $1, &$$)) YYERROR; }
    | tLONG tLONG tUNSIGNED     { if (!types_find_basic(L"unsigned long long int", $1, &$$)) YYERROR; }
    | tLONG tLONG tUNSIGNED tINT{ if (!types_find_basic(L"unsigned long long int", $1, &$$)) YYERROR; }
    | tFLOAT                    { if (!types_find_basic(L"float",                  $1, &$$)) YYERROR; }
    | tDOUBLE                   { if (!types_find_basic(L"double",                 $1, &$$)) YYERROR; }
    | tLONG tDOUBLE             { if (!types_find_basic(L"long double",            $1, &$$)) YYERROR; }
    | tTYPEDEF                  { $$ = $1; }
    | type_expr '*'             { if (!types_find_pointer(&$1, &$$)) {yyerror("Cannot find pointer type\n"); YYERROR; } }
    | tCLASS identifier         { if (!types_find_type($2, SymTagUDT, &$$)) {yyerror("Unknown type\n"); YYERROR; } }
    | tSTRUCT identifier        { if (!types_find_type($2, SymTagUDT, &$$)) {yyerror("Unknown type\n"); YYERROR; } }
    | tUNION identifier         { if (!types_find_type($2, SymTagUDT, &$$)) {yyerror("Unknown type\n"); YYERROR; } }
    | tENUM identifier          { if (!types_find_type($2, SymTagEnum, &$$)) {yyerror("Unknown type\n"); YYERROR; } }
    ;

expr_lvalue:
      expr                      { $$ = expr_eval($1); }
    ;

expr_rvalue:
      expr_lvalue               { $$ = types_extract_as_integer(&$1); }
    ;

/*
 * The expr rule builds an expression tree.  When we are done, we call
 * EvalExpr to evaluate the value of the expression.  The advantage of
 * the two-step approach is that it is possible to save expressions for
 * use in 'display' commands, and in conditional watchpoints.
 */
expr:
      tNUM                      { $$ = expr_alloc_sconstant($1); }
    | tSTRING			{ $$ = expr_alloc_string($1); }
    | tINTVAR                   { $$ = expr_alloc_internal_var($1); }
    | identifier		{ $$ = expr_alloc_symbol($1); }
    | expr OP_DRF tIDENTIFIER	{ $$ = expr_alloc_pstruct($1, $3); }
    | expr '.' tIDENTIFIER      { $$ = expr_alloc_struct($1, $3); }
    | identifier '(' ')'	{ $$ = expr_alloc_func_call($1, 0); }
    | identifier '(' expr ')'	{ $$ = expr_alloc_func_call($1, 1, $3); }
    | identifier '(' expr ',' expr ')' { $$ = expr_alloc_func_call($1, 2, $3, $5); }
    | identifier '(' expr ',' expr ',' expr ')'	{ $$ = expr_alloc_func_call($1, 3, $3, $5, $7); }
    | identifier '(' expr ',' expr ',' expr ',' expr ')' { $$ = expr_alloc_func_call($1, 4, $3, $5, $7, $9); }
    | identifier '(' expr ',' expr ',' expr ',' expr ',' expr ')' { $$ = expr_alloc_func_call($1, 5, $3, $5, $7, $9, $11); }
    | expr '[' expr ']'		 { $$ = expr_alloc_binary_op(EXP_OP_ARR, $1, $3); }
    | expr ':' expr		 { $$ = expr_alloc_binary_op(EXP_OP_SEG, $1, $3); }
    | expr OP_LOR expr           { $$ = expr_alloc_binary_op(EXP_OP_LOR, $1, $3); }
    | expr OP_LAND expr          { $$ = expr_alloc_binary_op(EXP_OP_LAND, $1, $3); }
    | expr '|' expr              { $$ = expr_alloc_binary_op(EXP_OP_OR, $1, $3); }
    | expr '&' expr              { $$ = expr_alloc_binary_op(EXP_OP_AND, $1, $3); }
    | expr '^' expr              { $$ = expr_alloc_binary_op(EXP_OP_XOR, $1, $3); }
    | expr OP_EQ expr            { $$ = expr_alloc_binary_op(EXP_OP_EQ, $1, $3); }
    | expr '>' expr              { $$ = expr_alloc_binary_op(EXP_OP_GT, $1, $3); }
    | expr '<' expr              { $$ = expr_alloc_binary_op(EXP_OP_LT, $1, $3); }
    | expr OP_GE expr            { $$ = expr_alloc_binary_op(EXP_OP_GE, $1, $3); }
    | expr OP_LE expr            { $$ = expr_alloc_binary_op(EXP_OP_LE, $1, $3); }
    | expr OP_NE expr            { $$ = expr_alloc_binary_op(EXP_OP_NE, $1, $3); }
    | expr OP_SHL expr           { $$ = expr_alloc_binary_op(EXP_OP_SHL, $1, $3); }
    | expr OP_SHR expr           { $$ = expr_alloc_binary_op(EXP_OP_SHR, $1, $3); }
    | expr '+' expr              { $$ = expr_alloc_binary_op(EXP_OP_ADD, $1, $3); }
    | expr '-' expr              { $$ = expr_alloc_binary_op(EXP_OP_SUB, $1, $3); }
    | expr '*' expr              { $$ = expr_alloc_binary_op(EXP_OP_MUL, $1, $3); }
    | expr '/' expr              { $$ = expr_alloc_binary_op(EXP_OP_DIV, $1, $3); }
    | expr '%' expr              { $$ = expr_alloc_binary_op(EXP_OP_REM, $1, $3); }
    | '-' expr %prec OP_SIGN     { $$ = expr_alloc_unary_op(EXP_OP_NEG, $2); }
    | '+' expr %prec OP_SIGN     { $$ = $2; }
    | '!' expr                   { $$ = expr_alloc_unary_op(EXP_OP_NOT, $2); }
    | '~' expr                   { $$ = expr_alloc_unary_op(EXP_OP_LNOT, $2); }
    | '(' expr ')'               { $$ = $2; }
    | '*' expr %prec OP_DEREF    { $$ = expr_alloc_unary_op(EXP_OP_DEREF, $2); }
    | '&' expr %prec OP_DEREF    { $$ = expr_alloc_unary_op(EXP_OP_ADDR, $2); }
    | '(' type_expr ')' expr %prec OP_DEREF { $$ = expr_alloc_typecast(&$2, $4); }
    ;

/*
 * The lvalue rule builds an expression tree.  This is a limited form
 * of expression that is suitable to be used as an lvalue.
 */
lvalue_addr: 
      lvalue                     { $$ = expr_eval($1); }
    ;

lvalue: 
      tNUM                       { $$ = expr_alloc_sconstant($1); }
    | tINTVAR                    { $$ = expr_alloc_internal_var($1); }
    | identifier		 { $$ = expr_alloc_symbol($1); }
    | lvalue OP_DRF tIDENTIFIER	 { $$ = expr_alloc_pstruct($1, $3); }
    | lvalue '.' tIDENTIFIER	 { $$ = expr_alloc_struct($1, $3); }
    | lvalue '[' expr ']'	 { $$ = expr_alloc_binary_op(EXP_OP_ARR, $1, $3); }
    | '*' expr			 { $$ = expr_alloc_unary_op(EXP_OP_DEREF, $2); }
    ;

%%

static LONG WINAPI wine_dbg_cmd(EXCEPTION_POINTERS *eptr)
{
    switch (eptr->ExceptionRecord->ExceptionCode)
    {
    case DEBUG_STATUS_INTERNAL_ERROR:
        dbg_printf("\nWineDbg internal error\n");
        break;
    case DEBUG_STATUS_NO_SYMBOL:
        dbg_printf("\nUndefined symbol\n");
        break;
    case DEBUG_STATUS_DIV_BY_ZERO:
        dbg_printf("\nDivision by zero\n");
        break;
    case DEBUG_STATUS_BAD_TYPE:
        dbg_printf("\nNo type or type mismatch\n");
        break;
    case DEBUG_STATUS_NO_FIELD:
        dbg_printf("\nNo such field in structure or union\n");
        break;
    case DEBUG_STATUS_CANT_DEREF:
        dbg_printf("\nDereference failed (not a pointer, or out of array bounds)\n");
        break;
    case DEBUG_STATUS_ABORT:
        break;
    case DEBUG_STATUS_NOT_AN_INTEGER:
        dbg_printf("\nNeeding an integral value\n");
        break;
    case CONTROL_C_EXIT:
        /* this is generally sent by a ctrl-c when we run winedbg outside of wineconsole */
        /* stop the debuggee, and continue debugger execution, we will be reentered by the
         * debug events generated by stopping
         */
        dbg_interrupt_debuggee();
        return EXCEPTION_CONTINUE_EXECUTION;
    default:
        dbg_printf("\nException %lx\n", eptr->ExceptionRecord->ExceptionCode);
        break;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

struct parser_context
{
    const char* filename;
    HANDLE input;
    HANDLE output;
    unsigned line_no;
    char*  last_line;
    size_t last_line_idx;
};

static struct parser_context dbg_parser = {NULL, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, NULL, 0};

static int input_fetch_entire_line(const char* pfx, char** line)
{
    char*       buffer;
    char        ch;
    DWORD	nread;
    size_t      len, alloc;

    /* as of today, console handles can be file handles... so better use file APIs rather than
     * console's
     */
    WriteFile(dbg_parser.output, pfx, strlen(pfx), &nread, NULL);

    buffer = malloc(alloc = 16);
    assert(buffer != NULL);

    dbg_parser.line_no++;
    len = 0;
    do
    {
        if (!ReadFile(dbg_parser.input, &ch, 1, &nread, NULL) || nread == 0)
        {
            free(buffer);
            return -1;
        }

        if (len + 2 > alloc)
        {
            char* new;
            while (len + 2 > alloc) alloc *= 2;
            if (!(new = realloc(buffer, alloc)))
            {
                free(buffer);
                return -1;
            }
            buffer = new;
        }
        buffer[len++] = ch;
    }
    while (ch != '\n');
    buffer[len] = '\0';

    *line = buffer;
    return len;
}

size_t input_lex_read_buffer(char* buf, int size)
{
    int len;

    /* try first to fetch the remaining of an existing line */
    if (dbg_parser.last_line_idx == 0)
    {
        char* tmp = NULL;
        /* no remaining chars to be read from last line, grab a brand new line up to '\n' */
        lexeme_flush();
        len = input_fetch_entire_line("Wine-dbg>", &tmp);
        if (len < 0) return 0;  /* eof */

        /* remove carriage return in newline */
        if (len >= 2 && tmp[len - 2] == '\r')
        {
            tmp[len - 2] = '\n';
            tmp[len - 1] = '\0';
            len--;
        }

        /* recall last command when empty input buffer and not parsing a file */
        if (dbg_parser.last_line && (len == 0 || (len == 1 && tmp[0] == '\n')) &&
            dbg_parser.output != INVALID_HANDLE_VALUE)
        {
            free(tmp);
        }
        else
        {
            free(dbg_parser.last_line);
            dbg_parser.last_line = tmp;
        }
    }

    len = min(strlen(dbg_parser.last_line + dbg_parser.last_line_idx), size - 1);
    memcpy(buf, dbg_parser.last_line + dbg_parser.last_line_idx, len);
    buf[len] = '\0';
    if ((dbg_parser.last_line_idx += len) >= strlen(dbg_parser.last_line))
        dbg_parser.last_line_idx = 0;
    return len;
}

int input_read_line(const char* pfx, char* buf, int size)
{
    char*       line = NULL;

    int len = input_fetch_entire_line(pfx, &line);
    if (len < 0) return 0;
    /* remove trailing \n and \r */
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) len--;
    len = min(size - 1, len);
    memcpy(buf, line, len);
    buf[len] = '\0';
    free(line);
    return 1;
}

/***********************************************************************
 *           parser_handle
 *
 * Debugger command line parser
 */
void	parser_handle(const char* filename, HANDLE input)
{
    BOOL ret_ok;
    struct parser_context prev = dbg_parser;

    ret_ok = FALSE;

    if (input != INVALID_HANDLE_VALUE)
    {
        dbg_parser.output = INVALID_HANDLE_VALUE;
        dbg_parser.input  = input;
    }
    else
    {
        dbg_parser.output = GetStdHandle(STD_OUTPUT_HANDLE);
        dbg_parser.input  = GetStdHandle(STD_INPUT_HANDLE);
    }
    dbg_parser.line_no = 0;
    dbg_parser.filename = filename;
    dbg_parser.last_line = NULL;
    dbg_parser.last_line_idx = 0;
    do
    {
       __TRY
       {
	  ret_ok = TRUE;
	  dbg_parse();
       }
       __EXCEPT(wine_dbg_cmd)
       {
	  ret_ok = FALSE;
       }
       __ENDTRY;
       lexeme_flush();
       expr_free_all();
    } while (!ret_ok);

    dbg_parser = prev;
}

static void parser(const char* filename)
{
    HANDLE h = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0L, 0);
    if (h != INVALID_HANDLE_VALUE)
    {
        parser_handle(filename, h);
        CloseHandle(h);
    }
}

static int dbg_error(const char* s)
{
    if (dbg_parser.filename)
        dbg_printf("%s:%d:", dbg_parser.filename, dbg_parser.line_no);
    dbg_printf("%s\n", s);
    return 0;
}

HANDLE WINAPIV parser_generate_command_file(const char* pmt, ...)
{
    HANDLE      hFile;
    char        path[MAX_PATH], file[MAX_PATH];
    DWORD       w;
    const char* p;

    GetTempPathA(sizeof(path), path);
    GetTempFileNameA(path, "WD", 0, file);
    hFile = CreateFileA(file, GENERIC_READ|GENERIC_WRITE|DELETE, FILE_SHARE_DELETE, 
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, 0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        va_list ap;

        WriteFile(hFile, pmt, strlen(pmt), &w, 0);
        va_start(ap, pmt);
        while ((p = va_arg(ap, const char*)) != NULL)
        {
            WriteFile(hFile, "\n", 1, &w, 0);
            WriteFile(hFile, p, strlen(p), &w, 0);
        }
        va_end(ap);
        WriteFile(hFile, "\nquit\n", 6, &w, 0);
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    }
    return hFile;
}
