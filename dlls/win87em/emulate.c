#include <stdlib.h>
#include "miscemu.h"
#include "debugtools.h"

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
/* FIXME: Still rather skeletal implementation only */

static BOOL Installed = 0;
static WORD RefCount = 0;
static WORD CtrlWord_1 = 0;
static WORD CtrlWord_2 = 0;
static WORD CtrlWord_Internal = 0;
static WORD StatusWord_1 = 0x000b;
static WORD StatusWord_2 = 0;
static WORD StatusWord_3 = 0;
static WORD StackTop = 175;
static WORD StackBottom = 0;
static WORD Inthandler02hVar = 1;

static void WIN87_ClearCtrlWord( CONTEXT86 *context )
{
    AX_reg(context) = 0;
    if (Installed)
#ifdef __i386__
        __asm__("fclex");
#else
        ;
#endif
    StatusWord_3 = StatusWord_2 = 0;
}

static void WIN87_SetCtrlWord( CONTEXT86 *context )
{
    CtrlWord_1 = AX_reg(context);
    AX_reg(context) &= 0xff3c;
    if (Installed) {
        CtrlWord_Internal = AX_reg(context);
#ifdef __i386__
        __asm__("wait;fldcw %0" : : "m" (CtrlWord_Internal));
#endif
    }
    CtrlWord_2 = AX_reg(context);
}

void WIN87_Init( CONTEXT86 *context )
{
    if (Installed) {
#ifdef __i386__
        __asm__("fninit");
        __asm__("fninit");
#endif
    }
    StackBottom = StackTop;
    AX_reg(context) = 0x1332;
    WIN87_SetCtrlWord(context);
    WIN87_ClearCtrlWord(context);
}

void WINAPI WIN87_fpmath( CONTEXT86 *context )
{
    TRACE("(cs:eip=%x:%lx es=%x bx=%04x ax=%04x dx==%04x)\n",
                 (WORD)CS_reg(context), EIP_reg(context),
                 (WORD)ES_reg(context), BX_reg(context),
                 AX_reg(context), DX_reg(context) );

    switch(BX_reg(context))
    {
    case 0: /* install (increase instanceref) emulator, install NMI vector */
        RefCount++;
        if (Installed)
            /* InstallIntVecs02hAnd75h(); */    ;
        WIN87_Init(context);
        AX_reg(context) = 0;
        break;

    case 1: /* Init Emulator */
        WIN87_Init(context);
        break;

    case 2: /* deinstall emulator (decrease instanceref), deinstall NMI vector	
             * if zero. Every '0' call should have a matching '2' call.
             */
        WIN87_Init(context);
        if (!(--RefCount) && (Installed))
            /* RestoreInt02h() */   ;
        
        break;

    case 3:
        /*INT_SetHandler(0x3E,MAKELONG(AX,DX));*/
        break;

    case 4: /* set control word (& ~(CW_Denormal|CW_Invalid)) */
        /* OUT: newset control word in AX */
        WIN87_SetCtrlWord(context);
        break;

    case 5: /* return internal control word in AX */
        AX_reg(context) = CtrlWord_1;
        break;

    case 6: /* round top of stack to integer using method AX & 0x0C00 */
        /* returns current controlword */
        {
            DWORD dw=0;
            WORD save,mask;
            /* I don't know much about asm() programming. This could be
             * wrong.
             */
#ifdef __i386__
           __asm__ __volatile__("fstcw %0;wait" : "=m" (save) : : "memory");
           __asm__ __volatile__("fstcw %0;wait" : "=m" (mask) : : "memory");
           __asm__ __volatile__("orw $0xC00,%0" : "=m" (mask) : : "memory");
           __asm__ __volatile__("fldcw %0;wait" : : "m" (mask));
           __asm__ __volatile__("frndint");
           __asm__ __volatile__("fist %0;wait" : "=m" (dw) : : "memory");
           __asm__ __volatile__("fldcw %0" : : "m" (save));
#endif
            TRACE("On top of stack is %ld\n",dw);
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
            TRACE("On top of stack was %ld\n",dw);
            AX_reg(context) = LOWORD(dw);
            DX_reg(context) = HIWORD(dw);
        }
        break;

    case 8: /* restore internal status words from emulator status word */
        AX_reg(context) = 0;
        if (Installed) {
#ifdef __i386__
            __asm__("fstsw %0;wait" : "=m" (StatusWord_1));
#endif
            AL_reg(context) = (BYTE)StatusWord_1 & 0x3f;
        }
        AX_reg(context) |= StatusWord_2;
        AX_reg(context) &= 0x1fff;
        StatusWord_2 = AX_reg(context);
        break;

    case 9: /* clear emu control word and some other things */
        WIN87_ClearCtrlWord(context);
        break;

    case 10: /* dunno. but looks like returning nr. of things on stack in AX */
	AX_reg(context) = 0;
        break;

    case 11: /* just returns the installed flag in DX:AX */
        DX_reg(context) = 0;
        AX_reg(context) = Installed;
        break;

    case 12: /* save AX in some internal state var */
        Inthandler02hVar = AX_reg(context);
        break;

    default: /* error. Say that loud and clear */
        FIXME("unhandled switch %d\n",BX_reg(context));
        AX_reg(context) = DX_reg(context) = 0xFFFF;
        break;
    }
}

/***********************************************************************
 *		WIN87_WinEm87Info
 */
void WINAPI WIN87_WinEm87Info(struct Win87EmInfoStruct *pWIS,
                              int cbWin87EmInfoStruct)
{
  FIXME("(%p,%d), stub !\n",pWIS,cbWin87EmInfoStruct);
}

/***********************************************************************
 *		WIN87_WinEm87Restore
 */
void WINAPI WIN87_WinEm87Restore(void *pWin87EmSaveArea,
                                 int cbWin87EmSaveArea)
{
  FIXME("(%p,%d), stub !\n",
	pWin87EmSaveArea,cbWin87EmSaveArea);
}

/***********************************************************************
 *		WIN87_WinEm87Save
 */
void WINAPI WIN87_WinEm87Save(void *pWin87EmSaveArea, int cbWin87EmSaveArea)
{
  FIXME("(%p,%d), stub !\n",
	pWin87EmSaveArea,cbWin87EmSaveArea);
}
