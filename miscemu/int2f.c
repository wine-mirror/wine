#include <stdio.h>
#include <stdlib.h>
#include "registers.h"
#include "wine.h"
#include "msdos.h"
#include "miscemu.h"
#include "options.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"

static void do_int2f_16(struct sigcontext_struct *context);

/**********************************************************************
 *	    INT_Int2fHandler
 *
 * Handler for int 2fh (multiplex).
 */
void INT_Int2fHandler( struct sigcontext_struct sigcontext )
{
#define context (&sigcontext)
    switch(AH)
    {
	case 0x10:
                AL = 0xff; /* share is installed */
		break;

	case 0x15: /* mscdex */
		/* ignore requests */
		break;

	case 0x16:
		do_int2f_16( context );
                break;
	default:
		INT_BARF( 0x2f );
            }
#undef context
}


static void do_int2f_16(struct sigcontext_struct *context)
{
    switch(AL)
    {
    case 0x00:  /* Windows enhanced mode installation check */
        AX = Options.enhanced ? WINVERSION : 0;
        break;

    case 0x0a:  /* Get Windows version and type */
        AX = 0;
        BX = (WINVERSION >> 8) | ((WINVERSION << 8) & 0xff00);
        CX = Options.enhanced ? 3 : 2;
        break;

    case 0x80:  /* Release time-slice */
        break;  /* nothing to do */

    case 0x86:  /* DPMI detect mode */
        AX = 0;  /* Running under DPMI */
        break;

    case 0x87:  /* DPMI installation check */
        AX = 0x0000;  /* DPMI Installed */
        BX = 0x0001;  /* 32bits available */
        CL = 0x03;    /* processor 386 */
        DX = 0x005a;  /* DPMI major/minor 0.90 */
        SI = 0;       /* # of para. of DOS extended private data */
        ES = 0;       /* ES:DI is DPMI switch entry point */
        DI = 0;
        break;

    default:
        INT_BARF( 0x2f );
    }
}


