/*
static char RCSId[] = "$Id: kernel.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
*/

#include <stdio.h>
#include <stdlib.h>
#include "prototypes.h"
#include "regfunc.h"
#include "options.h"
#include "stddebug.h"
#include "debug.h"

extern unsigned short WIN_StackSize;

/**********************************************************************
 *					KERNEL_InitTask
 */
void KERNEL_InitTask(void)
{
    _BX = 0x81;
    _AX = 1;
    _CX = WIN_StackSize;
    _DX = Options.cmdShow;
    _DI = _DS;

/* FIXME: DI should contain the instance handle of the caller, _DS doesn't
          always work as the caller might have changed it. */

    _SI = 0;
    ReturnFromRegisterFunc();
    /* Function does not return */
}

/**********************************************************************
 *					KERNEL_WaitEvent
 */
int
KERNEL_WaitEvent(int task)
{
    if (debugging_relay)
    	fprintf(stddeb,"WaitEvent: task %d\n", task);
    return 0;
}
