#include <stdlib.h>
#include "miscemu.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(int)

struct Win87EmInfoStruct
{
    unsigned short Version;
    unsigned short SizeSaveArea;
    unsigned short WinDataSeg;
    unsigned short WinCodeSeg;
    unsigned short Have80x87;
    unsigned short Unused;
};

/* Implementing this is easy cause Linux and *BSD* ALWAYS have a numerical
 * coprocessor. (either real or emulated on kernellevel)
 */
/* win87em.dll also sets interrupt vectors: 2 (NMI), 0x34 - 0x3f (emulator
 * calls of standard libraries, see Ralph Browns interrupt list), 0x75
 * (int13 error reporting of coprocessor)
 */

/* have a look at /usr/src/linux/arch/i386/math-emu/ *.[ch] for more info 
 * especially control_w.h and status_w.h
 */
/* FIXME: Only skeletal implementation for now */

void WINAPI WIN87_fpmath( CONTEXT86 *context )
{
    TRACE(int, "(cs:eip=%x:%lx es=%x bx=%04x ax=%04x dx==%04x)\n",
                 (WORD)CS_reg(context), EIP_reg(context),
                 (WORD)ES_reg(context), BX_reg(context),
                 AX_reg(context), DX_reg(context) );

    switch(BX_reg(context))
    {
    case 0: /* install (increase instanceref) emulator, install NMI vector */
        AX_reg(context) = 0;
        break;

    case 1: /* Init Emulator */
        AX_reg(context) = 0;
        break;

    case 2: /* deinstall emulator (decrease instanceref), deinstall NMI vector	
             * if zero. Every '0' call should have a matching '2' call.
             */
        AX_reg(context) = 0;   	
        break;

    case 3:
        /*INT_SetHandler(0x3E,MAKELONG(AX,DX));*/
        break;

    case 4: /* set control word (& ~(CW_Denormal|CW_Invalid)) */
        /* OUT: newset control word in AX */
        break;

    case 5: /* return internal control word in AX */
        break;

    case 6: /* round top of stack to integer using method AX & 0x0C00 */
        /* returns current controlword */
        {
            DWORD dw=0;
            /* I don't know much about asm() programming. This could be
             * wrong.
             */
           __asm__ __volatile__("frndint");
           __asm__ __volatile__("fist %0;wait" : "=m" (dw) : : "memory");
            TRACE(int,"On top of stack is %ld\n",dw);
        }
        break;

    case 7: /* POP top of stack as integer into DX:AX */
        /* IN: AX&0x0C00 rounding protocol */
        /* OUT: DX:AX variable popped */
        {
            DWORD dw=0;
            /* I don't know much about asm() programming. This could be 
             * wrong. 
             */
/* FIXME: could someone who really understands asm() fix this please? --AJ */
/*            __asm__("fistp %0;wait" : "=m" (dw) : : "memory"); */
            TRACE(int,"On top of stack was %ld\n",dw);
            AX_reg(context) = LOWORD(dw);
            DX_reg(context) = HIWORD(dw);
        }
        break;

    case 8: /* restore internal control words from emulator control word */
        break;

    case 9: /* clear emu control word and some other things */
        break;

    case 10: /* dunno. but looks like returning nr. of things on stack in AX */
	AX_reg(context) = 0;
        break;

    case 11: /* just returns the installed flag in DX:AX */
        DX_reg(context) = 0;
        AX_reg(context) = 1;
        break;

    case 12: /* save AX in some internal state var */
        break;

    default: /* error. Say that loud and clear */
        FIXME(int,"unhandled switch %d\n",BX_reg(context));
        AX_reg(context) = DX_reg(context) = 0xFFFF;
        break;
    }
}


void WINAPI WIN87_WinEm87Info(struct Win87EmInfoStruct *pWIS,
                              int cbWin87EmInfoStruct)
{
  TRACE(int, "(%p,%d)\n",pWIS,cbWin87EmInfoStruct);
}

void WINAPI WIN87_WinEm87Restore(void *pWin87EmSaveArea,
                                 int cbWin87EmSaveArea)
{
  TRACE(int, "(%p,%d)\n",
	pWin87EmSaveArea,cbWin87EmSaveArea);
}

void WINAPI WIN87_WinEm87Save(void *pWin87EmSaveArea, int cbWin87EmSaveArea)
{
  TRACE(int, "(%p,%d)\n",
	pWin87EmSaveArea,cbWin87EmSaveArea);
}
