#include <stdio.h>
#include <stdlib.h>
#include "msdos.h"
#include "wine.h"

static void Barf(struct sigcontext_struct *context)
{
	fprintf(stderr, "int2f: unknown/not implemented parameters:\n");
	fprintf(stderr, "int2f: AX %04x, BX %04x, CX %04x, DX %04x, "
	       "SI %04x, DI %04x, DS %04x, ES %04x\n",
	       AX, BX, CX, DX, SI, DI, DS, ES);
}

int do_int2f(struct sigcontext_struct *context)
{
	switch(context->sc_eax & 0xffff)
	{
	case 0x1600: /* windows enhanced mode install check */
		/* don't return anything as we're running in standard mode */
		break;

	default:
		Barf(context);
	};
	return 1;
}
