static char RCSId[] = "$Id: kernel.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include "prototypes.h"
#include "regfunc.h"

extern unsigned short WIN_StackSize;

/**********************************************************************
 *					KERNEL_LockSegment
 */
int
KERNEL_LockSegment(int segment)
{
    if (segment == -1)
	segment = *(Stack16Frame + 6);

#ifdef RELAY_DEBUG
    printf("LockSegment: segment %x\n", segment);
#endif

    return segment;
}

/**********************************************************************
 *					KERNEL_UnlockSegment
 */
int
KERNEL_UnlockSegment(int segment)
{
    if (segment == -1)
	segment = *(Stack16Frame + 6);

#ifdef RELAY_DEBUG
    printf("UnlockSegment: segment %x\n", segment);
#endif

    return segment;
}

/**********************************************************************
 *					KERNEL_InitTask
 */
KERNEL_InitTask()
{
    _BX = 0x81;
    _AX = 1;
    _CX = WIN_StackSize;
    _DX = 1;
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
#ifdef RELAY_DEBUG
    printf("WaitEvent: task %d\n", task);
#endif
    return 0;
}
/**********************************************************************
 *					KERNEL_GetModuleFileName
 */
int
KERNEL_GetModuleFileName(int module, char *filename, int bytes)
{
#ifdef RELAY_DEBUG
    printf("GetModuleFileName: module %d, filename %x, bytes %d\n", 
	    module, filename, bytes);
#endif
    
    strcpy(filename, "TEST.EXE");
    
    return strlen(filename);
}
