#include <stdio.h>
#include <stdlib.h>
#include "registers.h"
#include "wine.h"
#include "miscemu.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"

void IntBarf(int i, struct sigcontext_struct *context)
{
	dprintf_int(stddeb, "int%x: unknown/not implemented parameters:\n", i);
	dprintf_int(stddeb, "int%x: AX %04x, BX %04x, CX %04x, DX %04x, "
	       "SI %04x, DI %04x, DS %04x, ES %04x\n",
	       i, AX, BX, CX, DX, SI, DI, DS, ES);
}

int do_int10(struct sigcontext_struct *context)
{
        dprintf_int(stddeb,"int10: AX %04x, BX %04x, CX %04x, DX %04x, "
               "SI %04x, DI %04x, DS %04x, ES %04x\n",
               AX, BX, CX, DX, SI, DI, DS, ES);

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
