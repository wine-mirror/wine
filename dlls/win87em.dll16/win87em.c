/*
 * Copyright 1993 Bob Amstadt
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdlib.h>
#include "windef.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

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

static BOOL Installed = TRUE; /* 8087 is installed */
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

static void WIN87_ClearCtrlWord( CONTEXT *context )
{
    context->Eax &= ~0xffff;  /* set AX to 0 */
    if (Installed)
        __asm__("fclex");
    StatusWord_3 = StatusWord_2 = 0;
}

static void WIN87_SetCtrlWord( CONTEXT *context )
{
    CtrlWord_1 = LOWORD(context->Eax);
    context->Eax &= ~0x00c3;
    if (Installed) {
        CtrlWord_Internal = LOWORD(context->Eax);
        __asm__("wait;fldcw %0" : : "m" (CtrlWord_Internal));
    }
    CtrlWord_2 = LOWORD(context->Eax);
}

static void WIN87_Init( CONTEXT *context )
{
    if (Installed) {
        __asm__("fninit");
        __asm__("fninit");
    }
    StackBottom = StackTop;
    context->Eax = (context->Eax & ~0xffff) | 0x1332;
    WIN87_SetCtrlWord(context);
    WIN87_ClearCtrlWord(context);
}

/***********************************************************************
 *		__fpMath (WIN87EM.1)
 */
void WINAPI __fpMath( CONTEXT *context )
{
    TRACE("(cs:eip=%04lx:%04lx es=%04lx bx=%04lx ax=%04lx dx=%04lx)\n",
          context->SegCs, context->Eip, context->SegEs, context->Ebx,
          context->Eax, context->Edx );

    switch(LOWORD(context->Ebx))
    {
    case 0: /* install (increase instanceref) emulator, install NMI vector */
        RefCount++;
#if 0
        if (Installed)
            InstallIntVecs02hAnd75h();
#endif
        WIN87_Init(context);
        context->Eax &= ~0xffff;  /* set AX to 0 */
        break;

    case 1: /* Init Emulator */
        WIN87_Init(context);
        break;

    case 2: /* deinstall emulator (decrease instanceref), deinstall NMI vector
             * if zero. Every '0' call should have a matching '2' call.
             */
        WIN87_Init(context);
	RefCount--;
#if 0
        if (!RefCount && Installed)
            RestoreInt02h();
#endif

        break;

    case 3:
        /*INT_SetHandler(0x3E,MAKELONG(AX,DX));*/
        break;

    case 4: /* set control word (& ~(CW_Denormal|CW_Invalid)) */
        /* OUT: newset control word in AX */
        WIN87_SetCtrlWord(context);
        break;

    case 5: /* return internal control word in AX */
        context->Eax = (context->Eax & ~0xffff) | CtrlWord_1;
        break;

    case 6: /* round top of stack to integer using method AX & 0x0C00 */
        /* returns current controlword */
        {
            WORD save,mask;
            /* I don't know much about asm() programming. This could be
             * wrong.
             */
           __asm__ __volatile__("fstcw %0;wait" : "=m" (save) : : "memory");
           __asm__ __volatile__("fstcw %0;wait" : "=m" (mask) : : "memory");
           __asm__ __volatile__("orw $0xC00,%0" : "=m" (mask) : : "memory");
           __asm__ __volatile__("fldcw %0;wait" : : "m" (mask));
           __asm__ __volatile__("frndint");
           __asm__ __volatile__("fldcw %0" : : "m" (save));
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
            context->Eax = (context->Eax & ~0xffff) | LOWORD(dw);
            context->Edx = (context->Edx & ~0xffff) | HIWORD(dw);
        }
        break;

    case 8: /* restore internal status words from emulator status word */
        context->Eax &= ~0xffff;  /* set AX to 0 */
        if (Installed) {
            __asm__("fstsw %0;wait" : "=m" (StatusWord_1));
            context->Eax |= StatusWord_1 & 0x3f;
        }
        context->Eax = (context->Eax | StatusWord_2) & ~0xe000;
        StatusWord_2 = LOWORD(context->Eax);
        break;

    case 9: /* clear emu control word and some other things */
        WIN87_ClearCtrlWord(context);
        break;

    case 10: /* dunno. but looks like returning nr. of things on stack in AX */
        context->Eax &= ~0xffff;  /* set AX to 0 */
        break;

    case 11: /* just returns the installed flag in DX:AX */
        context->Edx &= ~0xffff;  /* set DX to 0 */
        context->Eax = (context->Eax & ~0xffff) | Installed;
        break;

    case 12: /* save AX in some internal state var */
        Inthandler02hVar = LOWORD(context->Eax);
        break;

    default: /* error. Say that loud and clear */
        FIXME("unhandled switch %d\n",LOWORD(context->Ebx));
        context->Eax |= 0xffff;
        context->Edx |= 0xffff;
        break;
    }
}

/***********************************************************************
 *		__WinEm87Info (WIN87EM.3)
 */
void WINAPI __WinEm87Info(struct Win87EmInfoStruct *pWIS, int cbWin87EmInfoStruct)
{
  FIXME("(%p,%d), stub !\n",pWIS,cbWin87EmInfoStruct);
}

/***********************************************************************
 *		__WinEm87Restore (WIN87EM.4)
 */
void WINAPI __WinEm87Restore(void *pWin87EmSaveArea, int cbWin87EmSaveArea)
{
  FIXME("(%p,%d), stub !\n",
	pWin87EmSaveArea,cbWin87EmSaveArea);
}

/***********************************************************************
 *		__WinEm87Save (WIN87EM.5)
 */
void WINAPI __WinEm87Save(void *pWin87EmSaveArea, int cbWin87EmSaveArea)
{
  FIXME("(%p,%d), stub !\n",
	pWin87EmSaveArea,cbWin87EmSaveArea);
}
