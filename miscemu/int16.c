#include <stdio.h>
#include <stdlib.h>
#include "registers.h"
#include "wine.h"
#include "stddebug.h"
/* #define DEBUG_INT */
/* #undef  DEBUG_INT */
#include "debug.h"

void IntBarf(int i, struct sigcontext_struct *context);

int do_int16(struct sigcontext_struct *context)
{
	switch(AH) {
	case 0xc0:
		
	default:
		IntBarf(0x16, context);
	};
	return 1;
}
