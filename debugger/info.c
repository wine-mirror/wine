/*
 * Wine debugger utility routines
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include "debugger.h"


/***********************************************************************
 *           DEBUG_Print
 *
 * Implementation of the 'print' command.
 */
void DEBUG_Print( const DBG_ADDR *addr, int count, char format )
{
    if (count != 1)
    {
        fprintf( stderr, "Count other than 1 is meaningless in 'print' command\n" );
        return;
    }

    if (addr->seg && (addr->seg != 0xffffffff))
    {
        switch(format)
        {
	case 'x':
            fprintf( stderr, "0x%04lx:", addr->seg );
            break;

	case 'd':
            fprintf( stderr, "%ld:", addr->seg );
            break;

	case 'c':
            break;  /* No segment to print */

	case 'i':
	case 's':
	case 'w':
	case 'b':
            break;  /* Meaningless format */
        }
    }

    switch(format)
    {
    case 'x':
        if (addr->seg) fprintf( stderr, "0x%04lx\n", addr->off );
        else fprintf( stderr, "0x%08lx\n", addr->off );
        break;

    case 'd':
        fprintf( stderr, "%ld\n", addr->off );
        break;

    case 'c':
        fprintf( stderr, "%d = '%c'\n",
                 (char)(addr->off & 0xff), (char)(addr->off & 0xff) );
        break;

    case 'i':
    case 's':
    case 'w':
    case 'b':
        fprintf( stderr, "Format specifier '%c' is meaningless in 'print' command\n", format );
        break;
    }
}


/***********************************************************************
 *           DEBUG_PrintAddress
 *
 * Print an 16- or 32-bit address, with the nearest symbol if any.
 */
void DEBUG_PrintAddress( const DBG_ADDR *addr, int addrlen )
{
    const char *name = DEBUG_FindNearestSymbol( addr );

    if (addr->seg) fprintf( stderr, "0x%04lx:", addr->seg );
    if (addrlen == 16) fprintf( stderr, "0x%04lx", addr->off );
    else fprintf( stderr, "0x%08lx", addr->off );
    if (name) fprintf( stderr, " (%s)", name );
}


/***********************************************************************
 *           DEBUG_Help
 *
 * Implementation of the 'help' command.
 */
void DEBUG_Help(void)
{
    int i = 0;
    static const char * helptext[] =
{
"The commands accepted by the Wine debugger are a small subset",
"of the commands that gdb would accept.  The commands currently",
"are:\n",
"  break [*<addr>]                      delete break bpnum",
"  disable bpnum                        enable bpnum",
"  help                                 quit",
"  x <addr>                             cont",
"  step                                 next",
"  mode [16,32]                         print <expr>",
"  set <reg> = <expr>                   set *<addr> = <expr>",
"  info [reg,stack,break,segments]      bt",
"  symbolfile <filename>                define <identifier> <addr>",
"",
"The 'x' command accepts repeat counts and formats (including 'i') in the",
"same way that gdb does.",
"",
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
 *           DEBUG_List
 *
 * Implementation of the 'list' command.
 */
void DEBUG_List( DBG_ADDR *addr, int count )
{
    static DBG_ADDR lasttime = { 0xffffffff, 0 };

    if (addr == NULL) addr = &lasttime;
    DBG_FIX_ADDR_SEG( addr, CS_reg(DEBUG_context) );
    while (count-- > 0)
    {
        DEBUG_PrintAddress( addr, dbg_mode );
        fprintf( stderr, ":  " );
        if (!DBG_CHECK_READ_PTR( addr, 1 )) break;
        DEBUG_Disasm( addr );
        fprintf (stderr, "\n");
    }
    lasttime = *addr;
}
