#include <stdlib.h>
#include <stdio.h>
#include "wine.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"

struct Win87EmInfoStruct {
  unsigned short Version;
  unsigned short SizeSaveArea;
  unsigned short WinDataSeg;
  unsigned short WinCodeSeg;
  unsigned short Have80x87;
  unsigned short Unused;
};

void
WIN87_fpmath( struct sigcontext_struct context )
{
  dprintf_int(stddeb, "_fpmath: (%x:%lx %x %x)\n",context.sc_cs, 
              context.sc_eip, context.sc_es, (int)context.sc_ebx & 0xffff );

  switch(context.sc_ebx & 0xffff)
    {
    case 11:
        context.sc_eax = 1;
        break;
    default:
        context.sc_eax = 0;
        break;
    }
}

void
WIN87_WinEm87Info(struct Win87EmInfoStruct *pWIS, int cbWin87EmInfoStruct)
{
  dprintf_int(stddeb, "__WinEm87Info(%p,%d)\n",pWIS,cbWin87EmInfoStruct);
}

void
WIN87_WinEm87Restore(void *pWin87EmSaveArea, int cbWin87EmSaveArea)
{
  dprintf_int(stddeb, "__WinEm87Restore(%p,%d)\n",
	pWin87EmSaveArea,cbWin87EmSaveArea);
}

void
WIN87_WinEm87Save(void *pWin87EmSaveArea, int cbWin87EmSaveArea)
{
  dprintf_int(stddeb, "__WinEm87Save(%p,%d)\n",
	pWin87EmSaveArea,cbWin87EmSaveArea);
}
