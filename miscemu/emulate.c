#include <stdlib.h>
#include <stdio.h>
#include "prototypes.h"
#include "regfunc.h"

struct Win87EmInfoStruct {
  unsigned short Version;
  unsigned short SizeSaveArea;
  unsigned short WinDataSeg;
  unsigned short WinCodeSeg;
  unsigned short Have80x87;
  unsigned short Unused;
};

int
WIN87_fpmath()
{
  printf( "_fpmath: (%x:%x %x %x)\n",_CONTEXT->sc_cs, _CONTEXT->sc_eip, 
	 _CONTEXT->sc_es, _BX & 0xffff);

  switch(_BX & 0xffff)
    {
    case 11:
      return 1;
    default:
      return 0;
    }

}

void
WIN87_WinEm87Info(struct Win87EmInfoStruct *pWIS, int cbWin87EmInfoStruct)
{
  printf( "__WinEm87Info(%p,%d)\n",pWIS,cbWin87EmInfoStruct);
}

void
WIN87_WinEm87Restore(void *pWin87EmSaveArea, int cbWin87EmSaveArea)
{
  printf( "__WinEm87Restore(%p,%d)\n",pWin87EmSaveArea,cbWin87EmSaveArea);
}

void
WIN87_WinEm87Save(void *pWin87EmSaveArea, int cbWin87EmSaveArea)
{
  printf( "__WinEm87Save(%p,%d)\n",pWin87EmSaveArea,cbWin87EmSaveArea);
}
