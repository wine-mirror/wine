/*
 * Wine debugger utility routines
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Alexandre Julliard
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "debugger.h"
#include "expr.h"

/***********************************************************************
 *           DEBUG_PrintBasic
 *
 * Implementation of the 'print' command.
 */
void DEBUG_PrintBasic( const DBG_ADDR *addr, int count, char format )
{
  char        * default_format;
  long long int value;

  if( addr->type == NULL )
    {
      fprintf(stderr, "Unable to evaluate expression\n");
      return;
    }
  
  default_format = NULL;
  value = DEBUG_GetExprValue((DBG_ADDR *) addr, &default_format);

  switch(format)
    {
    case 'x':
      if (addr->seg) 
	{
	  DEBUG_nchar += fprintf( stderr, "0x%04lx", (long unsigned int) value );
	}
      else 
	{
	  DEBUG_nchar += fprintf( stderr, "0x%08lx", (long unsigned int) value );
	}
      break;
      
    case 'd':
      DEBUG_nchar += fprintf( stderr, "%ld\n", (long int) value );
      break;
      
    case 'c':
      DEBUG_nchar += fprintf( stderr, "%d = '%c'",
	       (char)(value & 0xff), (char)(value & 0xff) );
      break;
      
    case 'i':
    case 's':
    case 'w':
    case 'b':
      fprintf( stderr, "Format specifier '%c' is meaningless in 'print' command\n", format );
    case 0:
      if( default_format != NULL )
	{
	  DEBUG_nchar += fprintf( stderr, default_format, value );
	}
      break;
    }
}


/***********************************************************************
 *           DEBUG_PrintAddress
 *
 * Print an 16- or 32-bit address, with the nearest symbol if any.
 */
struct symbol_info
DEBUG_PrintAddress( const DBG_ADDR *addr, int addrlen, int flag )
{
    struct symbol_info rtn;

    const char *name = DEBUG_FindNearestSymbol( addr, flag, &rtn.sym, 0, 
						&rtn.list );

    if (addr->seg) fprintf( stderr, "0x%04lx:", addr->seg&0xFFFF );
    if (addrlen == 16) fprintf( stderr, "0x%04lx", addr->off );
    else fprintf( stderr, "0x%08lx", addr->off );
    if (name) fprintf( stderr, " (%s)", name );
    return rtn;
}
/***********************************************************************
 *           DEBUG_PrintAddressAndArgs
 *
 * Print an 16- or 32-bit address, with the nearest symbol if any.
 * Similar to DEBUG_PrintAddress, but we print the arguments to
 * each function (if known).  This is useful in a backtrace.
 */
struct symbol_info
DEBUG_PrintAddressAndArgs( const DBG_ADDR *addr, int addrlen, 
			   unsigned int ebp, int flag )
{
    struct symbol_info rtn;

    const char *name = DEBUG_FindNearestSymbol( addr, flag, &rtn.sym, ebp, 
						&rtn.list );

    if (addr->seg) fprintf( stderr, "0x%04lx:", addr->seg );
    if (addrlen == 16) fprintf( stderr, "0x%04lx", addr->off );
    else fprintf( stderr, "0x%08lx", addr->off );
    if (name) fprintf( stderr, " (%s)", name );

    return rtn;
}


/***********************************************************************
 *           DEBUG_Help
 *
 * Implementation of the 'help' command.
 */
void DEBUG_Help(void)
{
    int i = 0;
    static const char * const helptext[] =
{
"The commands accepted by the Wine debugger are a reasonable",
"subset of the commands that gdb accepts.",
"The commands currently are:",
"  break [*<addr>]                        delete break bpnum",
"  disable bpnum                          enable bpnum",
"  condition <bpnum> [<expr>]",

"  help                                   quit",
"  bt                                     cont [N]",
"  step [N]                               next [N]",
"  stepi [N]                              nexti [N]",
"  x <addr>                               print <expr>",
"  set <reg> = <expr>                     set *<addr> = <expr>",
"  up                                     down",
"  list <lines>                           disassemble [<addr>][,<addr>]",
"  frame <n>                              finish",
"  show dir                               dir <path>",
"  display <expr>                         undisplay <disnum>",
"  delete display <disnum>                debugmsg <class>[-+]<type>\n",

"Wine-specific commands:",
"  mode [16,32]                           walk [wnd,class,queue,module]",
"  info (see 'help info' for options)\n",

"The 'x' command accepts repeat counts and formats (including 'i') in the",
"same way that gdb does.\n",

" The following are examples of legal expressions:",
" $eax     $eax+0x3   0x1000   ($eip + 256)  *$eax   *($esp + 3)",
" Also, a nm format symbol table can be read from a file using the",
" symbolfile command.  Symbols can also be defined individually with",
" the define command.",
"",
NULL
};

    while(helptext[i]) fprintf(stderr,"%s\n", helptext[i++]);
}


/***********************************************************************
 *           DEBUG_HelpInfo
 *
 * Implementation of the 'help info' command.
 */
void DEBUG_HelpInfo(void)
{
    int i = 0;
    static const char * const infotext[] =
{
"The info commands allow you to get assorted bits of interesting stuff",
"to be displayed.  The options are:",
"  info break           Dumps information about breakpoints",
"  info display         Shows auto-display expressions in use",
"  info locals          Displays values of all local vars for current frame",
"  info maps            Dumps all virtual memory mappings",
"  info module <handle> Displays internal module state",
"  info queue <handle>  Displays internal queue state",
"  info reg             Displays values in all registers at top of stack",
"  info segments        Dumps information about all known segments",
"  info share           Dumps information about shared libraries",
"  info stack           Dumps information about top of stack",
"  info wnd <handle>    Displays internal window state",
"",
NULL
};

    while(infotext[i]) fprintf(stderr,"%s\n", infotext[i++]);
}
