#include <stdio.h>
#include <stdlib.h>
#include "registers.h"
#include "wine.h"
#include "stddebug.h"
/* #define DEBUG_INT */
/* #undef  DEBUG_INT */
#include "debug.h"

void IntBarf(int i, struct sigcontext_struct *context)
{
	fprintf(stderr, "int%x: unknown/not implemented parameters:\n", i);
	fprintf(stderr, "int%x: AX %04x, BX %04x, CX %04x, DX %04x, "
	       "SI %04x, DI %04x, DS %04x, ES %04x\n",
	       i, AX, BX, CX, DX, SI, DI, DS, ES);
}

int do_int10(struct sigcontext_struct *context)
{
	switch(AH) {
	case 0x0f:
		AL = 0x5b;
		break;

	case 0x12:
		if (BL == 0x10) {
			BX = 0x0003;
			CX = 0x0009;
		}
		break;
			
	case 0x1a:
		BX = 0x0008;
		break;

	default:
		IntBarf(0x10, context);
	};
	return 1;
}
