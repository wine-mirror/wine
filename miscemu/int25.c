#include <stdio.h>
#include <stdlib.h>
#include "msdos.h"
#include "wine.h"

int do_int25(struct sigcontext_struct *context)
{
	BYTE *dataptr = pointer(DS, BX);
	DWORD begin, length;

	if( (ECX & 0xffff) == 0xffff)
	{
		begin = getdword(dataptr);
		length = getword(&dataptr[4]);
		dataptr = (BYTE *) getdword(&dataptr[6]);
			
	} else {
		begin = EDX & 0xffff;
		length = ECX & 0xffff;
	}
	fprintf(stderr, "int25: abs diskread, drive %d, sector %d, "
	"count %d, buffer %d\n", EAX & 0xff, begin, length, (int) dataptr);

	memset(dataptr, 0, length * 512);

	if (begin == 0 && length > 1) 
		*(dataptr + 512) = 0xf8;

	if (begin == 1) 
		*dataptr = 0xf8;

	ResetCflag;

	/* push flags on stack */
	SP -= sizeof(WORD);
	setword(pointer(SS,SP), (WORD) EFL);

	return 1;
}
