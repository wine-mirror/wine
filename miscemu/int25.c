#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "registers.h"
#include "msdos.h"
#include "segmem.h"
#include "wine.h"
#include "miscemu.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"

int do_int25(struct sigcontext_struct *context)
{
	BYTE *dataptr = SAFEMAKEPTR(DS, BX);
	DWORD begin, length;

	if (CX == 0xffff) {
		begin = getdword(dataptr);
		length = getword(&dataptr[4]);
		dataptr = (BYTE *) getdword(&dataptr[6]);
			
	} else {
		begin = DX;
		length = CX;
	}
	dprintf_int(stdnimp, "int25: abs diskread, drive %d, sector %ld, "
	"count %ld, buffer %d\n", (int)EAX & 0xff, begin, length, (int) dataptr);

	memset(dataptr, 0, length * 512);

	if (begin == 0 && length > 1) 
		*(dataptr + 512) = 0xf8;

	if (begin == 1) 
		*dataptr = 0xf8;

	ResetCflag;

	/* push flags on stack */
	SP -= sizeof(WORD);
	setword(SAFEMAKEPTR(SS,SP), (WORD) EFL);

	return 1;
}
