/* -*-C-*-
 * Lexical scanner for command line parsing
 *
 * Copyright 1993 Eric Youngdale
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debugger.h"
#include "xmalloc.h"
#include "y.tab.h"

#ifndef DONT_USE_READLINE
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = dbg_read((char *) buf, max_size )) < 0 ) \
	    YY_FATAL_ERROR( "read() in flex scanner failed" );

extern char * readline(char *);
extern void add_history(char *);
static int dbg_read(char * buf, int size);
static char * make_symbol(char *);
void flush_symbols();

#endif  /* DONT_USE_READLINE */

#define YY_NO_UNPUT

static int syntax_error;
%}

DIGIT	   [0-9]
HEXDIGIT   [0-9a-fA-F]
FORMAT     [bcdiswx]
IDENTIFIER [_a-zA-Z\.~][_a-zA-Z0-9\.~@]*
PATHNAME   [/_a-zA-Z\.~][/_a-zA-Z0-9\.~@]*
STRING     \"[^\n"]+\"

%s FORMAT_EXPECTED
%s PATH_EXPECTED
%s INFO_CMD
%s HELP_CMD
%s DEL_CMD
%s WALK_CMD
%s SHOW_CMD
%s NOCMD

%%

\n		{ BEGIN(0); syntax_error = 0; 
		  return tEOL; } /*Indicates end of command.  Reset state. */

"||"		{ return OP_LOR; }
"&&"		{ return OP_LAND; }
"=="		{ return OP_EQ; }
"!="		{ return OP_NE; }
"<="		{ return OP_LE; }
">="		{ return OP_GE; }
"<<"		{ return OP_SHL; }
">>"		{ return OP_SHR; }
"->"		{ return OP_DRF; }
[-+<=>|&^()*/%:!~,\.]	{ return *yytext; }
"["		{ return *yytext; }
"]"		{ return *yytext; }

"0x"{HEXDIGIT}+      { sscanf(yytext, "%x", &yylval.integer); return tNUM; }
{DIGIT}+             { sscanf(yytext, "%d", &yylval.integer); return tNUM; }


<FORMAT_EXPECTED>"/"{DIGIT}+{FORMAT}  { char * last;
                       yylval.integer = strtol( yytext+1, &last, NULL );
                       yylval.integer = (yylval.integer << 8) | *last;
                       return tFORMAT; }


<FORMAT_EXPECTED>"/"{FORMAT}          { yylval.integer = (1 << 8) | yytext[1]; return tFORMAT; }

{STRING} { yylval.string = make_symbol(yytext); return tSTRING; }

$pc     { yylval.reg = REG_EIP; return tREG; }
$flags  { yylval.reg = REG_EFL; return tREG; }
$eip    { yylval.reg = REG_EIP; return tREG; }
$ip     { yylval.reg = REG_IP;  return tREG; }
$esp    { yylval.reg = REG_ESP; return tREG; }
$sp     { yylval.reg = REG_SP;  return tREG; }
$eax    { yylval.reg = REG_EAX; return tREG; }
$ebx    { yylval.reg = REG_EBX; return tREG; }
$ecx    { yylval.reg = REG_ECX; return tREG; }
$edx    { yylval.reg = REG_EDX; return tREG; }
$esi    { yylval.reg = REG_ESI; return tREG; }
$edi    { yylval.reg = REG_EDI; return tREG; }
$ebp    { yylval.reg = REG_EBP; return tREG; }
$ax     { yylval.reg = REG_AX;  return tREG; }
$bx     { yylval.reg = REG_BX;  return tREG; }
$cx     { yylval.reg = REG_CX;  return tREG; }
$dx     { yylval.reg = REG_DX;  return tREG; }
$si     { yylval.reg = REG_SI;  return tREG; }
$di     { yylval.reg = REG_DI;  return tREG; }
$bp     { yylval.reg = REG_BP;  return tREG; }
$es     { yylval.reg = REG_ES;  return tREG; }
$ds     { yylval.reg = REG_DS;  return tREG; }
$cs     { yylval.reg = REG_CS;  return tREG; }
$ss     { yylval.reg = REG_SS;  return tREG; }
$fs     { yylval.reg = REG_FS;  return tREG; }
$gs     { yylval.reg = REG_GS;  return tREG; }

<INITIAL>info|inf|in			{ BEGIN(INFO_CMD); return tINFO; }
<INITIAL>up				{ BEGIN(NOCMD); return tUP; }
<INITIAL>down|dow|do			{ BEGIN(NOCMD); return tDOWN; }
<INITIAL>frame|fram|fra|fr		{ BEGIN(NOCMD); return tFRAME; }
<INITIAL>list|lis|li|l			{ BEGIN(PATH_EXPECTED); return tLIST; }
<INITIAL>enable|enabl|enab|ena		{ BEGIN(NOCMD); return tENABLE;}
<INITIAL>disable|disabl|disab|disa|dis  { BEGIN(NOCMD); return tDISABLE; }
<INITIAL>disassemble|disassembl|disassemb|disassem|disasse|disass|disas { BEGIN(NOCMD); return tDISASSEMBLE; }
<INITIAL,INFO_CMD,DEL_CMD>display|displa|displ|disp	{ BEGIN(FORMAT_EXPECTED); return tDISPLAY; }
<INITIAL>undisplay|undispla|undispl|undisp|undis|undi|und	{ BEGIN(NOCMD); return tUNDISPLAY; }
<INITIAL>delete|delet|dele|del		{ BEGIN(DEL_CMD); return tDELETE; }
<INITIAL>quit|qui|qu|q			{ BEGIN(NOCMD); return tQUIT; }
<INITIAL>set|se				{ BEGIN(NOCMD); return tSET; }
<INITIAL>walk|w				{ BEGIN(WALK_CMD); return tWALK; }
<INITIAL>x				{ BEGIN(FORMAT_EXPECTED); return tEXAM; }
<INITIAL>help|hel|he|"?"		{ BEGIN(HELP_CMD); return tHELP; }

<INITIAL>backtrace|backtrac|backtra|backt|back|bac|ba|bt { BEGIN(NOCMD); return tBACKTRACE; }
<INITIAL>where|wher|whe                  { BEGIN(NOCMD); return tBACKTRACE; }

<INITIAL>cont|con|co|c   		{ BEGIN(NOCMD); return tCONT; }
<INITIAL>condition|conditio|conditi|condit|condi|cond	{ BEGIN(NOCMD); return tCOND; }
<INITIAL>step|ste|st|s   		{ BEGIN(NOCMD); return tSTEP; }
<INITIAL>next|nex|ne|n   		{ BEGIN(NOCMD); return tNEXT; }
<INITIAL>stepi|si	   		{ BEGIN(NOCMD); return tSTEPI; }
<INITIAL>nexti|ni	   		{ BEGIN(NOCMD); return tNEXTI; }
<INITIAL>finish|finis|fini|fin|fi	{ BEGIN(NOCMD); return tFINISH; }

<INITIAL>abort|abor|abo         	{ BEGIN(NOCMD); return tABORT; }
<INITIAL>print|prin|pri|pr|p		{ BEGIN(FORMAT_EXPECTED); return tPRINT; }

<INITIAL>mode				{ BEGIN(NOCMD); return tMODE; }
<INITIAL>show|sho|sh			{ BEGIN(SHOW_CMD); return tSHOW; }

<INITIAL,INFO_CMD,DEL_CMD>break|brea|bre|br|b	{ BEGIN(PATH_EXPECTED); return tBREAK; }

<INFO_CMD>share|shar|sha		{ return tSHARE; }
<INFO_CMD>locals|local|loca|loc		{ return tLOCAL; }
<INFO_CMD,WALK_CMD>class|clas|cla                  { return tCLASS; }
<INFO_CMD,WALK_CMD>module|modul|modu|mod  { return tMODULE; }
<INFO_CMD,WALK_CMD>queue|queu|que			{ return tQUEUE; }
<INFO_CMD>registers|regs|reg|re		{ return tREGS; }
<INFO_CMD>segments|segment|segm|seg|se	{ return tSEGMENTS; }
<INFO_CMD>stack|stac|sta|st     		{ return tSTACK; }
<INFO_CMD>maps|map			{ return tMAPS; }
<INFO_CMD,WALK_CMD>window|windo|wind|win|wnd	{ return tWND; }
<HELP_CMD>info|inf|in                   { return tINFO; }

<INITIAL,SHOW_CMD>directories|directorie|directori|director|directo|direct|direc|direc|dir { 
			BEGIN(PATH_EXPECTED); return tDIR; }

char					{ return tCHAR; }
short					{ return tSHORT; }
int					{ return tINT; }
long					{ return tLONG; }
float					{ return tFLOAT; }
double					{ return tDOUBLE; }
unsigned				{ return tUNSIGNED; }
signed					{ return tSIGNED; }
struct					{ return tSTRUCT; }
union					{ return tUNION; }
enum					{ return tENUM; }

{IDENTIFIER}	{ yylval.string = make_symbol(yytext); return tIDENTIFIER; }

<PATH_EXPECTED>{PATHNAME}	{ yylval.string = make_symbol(yytext); return tPATH; }

[ \t]+        /* Eat up whitespace */

.		{ if (syntax_error == 0)
                  {
		    syntax_error ++; fprintf(stderr, "Syntax Error\n");
                  }
		}


%%

#ifndef yywrap
int yywrap(void) { return 1; }
#endif

#ifndef DONT_USE_READLINE

#ifndef whitespace
#define whitespace(c) (((c) == ' ') || ((c) == '\t'))
#endif


/* Strip whitespace from the start and end of STRING. */
static void stripwhite (char *string)
{
  register int i = 0;

  while (whitespace (string[i]))
    i++;

  if (i)
    strcpy (string, string + i);

  i = strlen (string) - 1;

  while (i > 0 && whitespace (string[i]))
    i--;

  string[++i] = '\0';
}

static int dbg_read(char * buf, int size)
{
    static char last_line[256] = "";
    char * line;
    int len;
    
    for (;;)
    {
        flush_symbols();
        line = readline ("Wine-dbg>");
        if (!line)
        {
            fprintf( stderr, "\n" );
            exit(0);
        }

        /* Remove leading and trailing whitespace from the line */

        stripwhite (line);

        /* If there is anything left, add it to the history list
           and execute it. Otherwise, re-execute last command. */

        if (*line)
        {
            add_history( line );
            strncpy( last_line, line, 255 );
            last_line[255] = '\0'; 
       }

        free( line );
        line = last_line;

        if ((len = strlen(line)) > 0)
        {
            if (size < len + 1)
            {
                fprintf(stderr,"Fatal readline goof.\n");
                exit(0);
            }
            strcpy(buf, line);
            buf[len] = '\n';
            buf[len+1] = 0;
            return len + 1;
        }
    }
}

static char *local_symbols[30];
static int next_symbol;

char * make_symbol(char * symbol){
        return local_symbols[next_symbol++] = xstrdup(symbol);
}

void flush_symbols()
{
	while(--next_symbol>= 0) free(local_symbols[next_symbol]);
	next_symbol = 0;
}

#endif  /* DONT_USE_READLINE */
