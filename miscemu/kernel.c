/*
static char RCSId[] = "$Id: kernel.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
*/

#include <stdio.h>
#include <stdlib.h>
#include "prototypes.h"
#include "options.h"
#include "wine.h"
#include "stddebug.h"
#include "debug.h"

extern unsigned short WIN_StackSize;

/**********************************************************************
 *					KERNEL_InitTask
 */
void KERNEL_InitTask( struct sigcontext_struct context )
{
    context.sc_eax = 1;
    context.sc_ebx = 0x81;
    context.sc_ecx = WIN_StackSize;
    context.sc_edx = Options.cmdShow;
    context.sc_edi = 0;
    context.sc_edi = context.sc_ds;
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
