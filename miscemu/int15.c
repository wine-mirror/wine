#include <stdio.h>
#include <stdlib.h>
#include "registers.h"
#include "wine.h"

int do_int15(struct sigcontext_struct *context)
{
	switch(AH) {
	case 0xc0:
		
	default:
		IntBarf(0x15, context);
	};
	return 1;
}
