#include <stdio.h>
#include <stdlib.h>
#include "msdos.h"
#include "wine.h"

int do_int31(struct sigcontext_struct *context)
{
	switch((context->sc_eax >> 8) & 0xff)
	{
	default:
		IntBarf(0x31, context);
	};
	return 1;
}
