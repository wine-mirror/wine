#include <stdio.h>
#include <stdlib.h>
#include "registers.h"
#include "msdos.h"
#include "wine.h"
#include "stddebug.h"
/* #define DEBUG_INT */
/* #undef  DEBUG_INT */
#include "debug.h"

void IntBarf(int i, struct sigcontext_struct *context);

int do_int26(struct sigcontext_struct *context)
{
	BYTE *dataptr = pointer(DS, BX);
	DWORD begin, length;

	if (CX == 0xffff) {
		begin = getdword(dataptr);
		length = getword(&dataptr[4]);
		dataptr = (BYTE *) getdword(&dataptr[6]);
			
	} else {
		begin = DX;
		length = CX;
	}
		
	dprintf_int(stdnimp,"int26: abs diskwrite, drive %d, sector %d, count %d,"
		" buffer %d\n", EAX & 0xff, begin, length, (int) dataptr);

	ResetCflag;

	/* push flags on stack */
	SP -= sizeof(WORD);
	setword(pointer(SS,SP), (WORD) EFL);

	return 1;
}
