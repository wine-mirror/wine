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

int do_int2f_16(struct sigcontext_struct *context);

int do_int2f(struct sigcontext_struct *context)
{
        dprintf_int(stddeb,"int2f: AX %04x, BX %04x, CX %04x, DX %04x, "
               "SI %04x, DI %04x, DS %04x, ES %04x\n",
               AX, BX, CX, DX, SI, DI, DS, ES);

	switch(AH)
	{
	case 0x10:
                AL = 0xff; /* share is installed */
		break;

	case 0x15: /* mscdex */
		/* ignore requests */
		return 1;

	case 0x16:
		return do_int2f_16(context);

	default:
		IntBarf(0x2f, context);
	};
	return 1;
}


int do_int2f_16(struct sigcontext_struct *context)
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

    case 0x86:  /* DPMI detect mode */
        AX = 0;  /* Running under DPMI */
        break;

    case 0x87:  /* DPMI installation check */
        AX = 0x0000;  /* DPMI Installed */
        BX = 0x0001;  /* 32bits available */
        CX = 0x04;    /* processor 486 */
        DX = 0x0009;  /* DPMI major/minor 0.9 */
        SI = 0;       /* # of para. of DOS extended private data */
        ES = 0;       /* ES:DI is DPMI switch entry point */
        DI = 0;
        break;

    default:
        IntBarf(0x2f, context);
    }
    return 1;
}


