#include <stdio.h>
#include <stdlib.h>
#include "msdos.h"
#include "wine.h"
#include "stddebug.h"
/* #define DEBUG_INT */
/* #undef  DEBUG_INT */
#include "debug.h"

void IntBarf(int i, struct sigcontext_struct *context);

int do_int2a(struct sigcontext_struct *context)
{
	switch((context->sc_eax >> 8) & 0xff)
	{
	case 0x00:                             /* NETWORK INSTALLATION CHECK */
		break;
		
	default:
		IntBarf(0x2a, context);
	};
	return 1;
}
