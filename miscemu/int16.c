#include <stdio.h>
#include <stdlib.h>
#include "registers.h"
#include "wine.h"
#include "miscemu.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"

int do_int16(struct sigcontext_struct *context)
{
	switch(AH) {
	case 0xc0:
		
	default:
		IntBarf(0x16, context);
	};
	return 1;
}
