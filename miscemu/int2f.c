#include <stdio.h>
#include <stdlib.h>
#include "msdos.h"
#include "wine.h"

int do_int2f(struct sigcontext_struct *context)
{
	switch((context->sc_eax >> 8) & 0xff)
	{
	case 0x15: /* mscdex */
		/* ignore requests */
		return 1;

	case 0x16:
		switch(context->sc_eax & 0xff)
		{
		case 0x00: /* windows enhanced mode install check */
			   /* don't return anything as we're running in standard mode */
			return 1;

		default:
		}

	default:
		IntBarf(0x2f, context);
	};
	return 1;
}
