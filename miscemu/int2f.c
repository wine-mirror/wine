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
void INT_Int2fHandler( struct sigcontext_struct context )
{
    switch(AH_reg(&context))
    {
	case 0x10:
                AL_reg(&context) = 0xff; /* share is installed */
		break;

	case 0x15: /* mscdex */
		/* ignore requests */
		break;

	case 0x16:
		do_int2f_16( &context );
                break;
	default:
		INT_BARF( &context, 0x2f );
            }
}


static void do_int2f_16(struct sigcontext_struct *context)
{
    switch(AL_reg(context))
    {
    case 0x00:  /* Windows enhanced mode installation check */
        AX_reg(context) = Options.enhanced ? WINVERSION : 0;
        break;

    case 0x0a:  /* Get Windows version and type */
        AX_reg(context) = 0;
        BX_reg(context) = (WINVERSION >> 8) | ((WINVERSION << 8) & 0xff00);
        CX_reg(context) = Options.enhanced ? 3 : 2;
        break;

    case 0x80:  /* Release time-slice */
        break;  /* nothing to do */

    case 0x86:  /* DPMI detect mode */
        AX_reg(context) = 0;  /* Running under DPMI */
        break;

    case 0x87:  /* DPMI installation check */
        AX_reg(context) = 0x0000; /* DPMI Installed */
        BX_reg(context) = 0x0001; /* 32bits available */
        CL_reg(context) = 0x03;   /* processor 386 */
        DX_reg(context) = 0x005a; /* DPMI major/minor 0.90 */
        SI_reg(context) = 0;      /* # of para. of DOS extended private data */
        ES_reg(context) = 0;      /* ES:DI is DPMI switch entry point */
        DI_reg(context) = 0;
        break;

    default:
        INT_BARF( context, 0x2f );
    }
}


