static char RCSId[] = "$Id$";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include "prototypes.h"

extern unsigned short *Stack16Frame;

/**********************************************************************
 *					KERNEL_GetVersion
 *
 * Return the version of Windows that we emulate.
 */
int
KERNEL_GetVersion(void)
{
    return 0x0301;
}

/**********************************************************************
 *					KERNEL_LockSegment
 */
int
KERNEL_LockSegment(int segment)
{
    if (segment == -1)
	segment = *(Stack16Frame + 6);

#ifdef RELAY_DEBUG
    fprintf(stderr, "LockSegment: segment %x\n", segment);
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
    fprintf(stderr, "UnlockSegment: segment %x\n", segment);
#endif

    return segment;
}

/**********************************************************************
 *					KERNEL_WaitEvent
 */
int
KERNEL_WaitEvent(int task)
{
#ifdef RELAY_DEBUG
    fprintf(stderr, "WaitEvent: task %d\n", task);
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
    fprintf(stderr, "GetModuleFileName: module %d, filename %x, bytes %d\n", 
	    module, filename, bytes);
#endif
    
    strcpy(filename, "TEST.EXE");
    
    return strlen(filename);
}

/**********************************************************************
 *					KERNEL_DOS3Call
 */
int
KERNEL_DOS3Call(int ax, int cx, int dx, int bx, int sp, int bp,
		int si, int di, int ds, int es)
{
    switch ((ax >> 8) & 0xff)
    {
      case 0x30:
	return 0x0303;
	
      case 0x25:
      case 0x35:
	return 0;

      default:
	fprintf(stderr, "DOS: AX %04x, BX %04x, CX %04x, DX %04x\n",
		ax, bx, cx, dx);
	fprintf(stderr, "     SP %04x, BP %04x, SI %04x, DI %04x\n",
		sp, bp, si, di);
	fprintf(stderr, "     DS %04x, ES %04x\n",
		ds, es);
    }
    
    return 0;
}
