#include <stdlib.h>
#include <stdio.h>
#include "wine.h"
#include "registers.h"
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
WIN87_fpmath( struct sigcontext_struct sigcontext )
{
    /* Declare a context pointer so that registers macros work */
    struct sigcontext_struct *context = &sigcontext;

    dprintf_int(stddeb, "_fpmath: (%x:%lx %x %x)\n", CS, EIP, ES, BX );

    switch(BX)
    {
    case 11:
        AX = 1;
        break;
    default:
        AX = 0;
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
