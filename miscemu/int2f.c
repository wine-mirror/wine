#include <stdio.h>
#include <stdlib.h>
#include "msdos.h"
#include "wine.h"

int do_int2f(struct sigcontext_struct *context)
{
	switch(context->sc_eax & 0xffff)
	{
	case 0x1600: /* windows enhanced mode install check */
		/* don't return anything as we're running in standard mode */
		break;

	default:
		IntBarf(0x2f, context);
	};
	return 1;
}
