static char RCSId[] = "$Id$";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "prototypes.h"
#include "regfunc.h"

static void
GetTimeDate(int time_flag)
{
    struct tm *now;
    time_t ltime;
    
    ltime = time(NULL);
    now = localtime(&ltime);
    if (time_flag)
    {
	_CX = (now->tm_hour << 8) | now->tm_min;
	_DX = now->tm_sec << 8;
    }
    else
    {
	_CX = now->tm_year + 1900;
	_DX = ((now->tm_mon + 1) << 8) | now->tm_mday;
	_AX &= 0xff00;
	_AX |= now->tm_wday;
    }
#ifdef DEBUG_DOS
    printf("GetTimeDate: AX = %04x, CX = %04x, DX = %04x\n", _AX, _CX, _DX);
#endif
    
    ReturnFromRegisterFunc();
    /* Function does not return */
}

/**********************************************************************
 *					KERNEL_DOS3Call
 */
int
KERNEL_DOS3Call()
{
    switch ((_AX >> 8) & 0xff)
    {
      case 0x30:
	_AX = 0x0303;
	ReturnFromRegisterFunc();
	/* Function does not return */
	
      case 0x25:
      case 0x35:
	return 0;

      case 0x2a:
	GetTimeDate(0);
	/* Function does not return */
	
      case 0x2c:
	GetTimeDate(1);
	/* Function does not return */

      case 0x4c:
	exit(_AX & 0xff);

      default:
	fprintf(stderr, "DOS: AX %04x, BX %04x, CX %04x, DX %04x\n",
		_AX, _BX, _CX, _DX);
	fprintf(stderr, "     SP %04x, BP %04x, SI %04x, DI %04x\n",
		_SP, _BP, _SI, _DI);
	fprintf(stderr, "     DS %04x, ES %04x\n",
		_DS, _ES);
    }
    
    return 0;
}
