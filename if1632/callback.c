#ifndef WINELIB
/*
static char RCSId[] = "$Id: wine.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <setjmp.h>
#include "windows.h"
#include "callback.h"
#include "wine.h"
#include "global.h"
#include "stackframe.h"
#include "dlls.h"
#include "stddebug.h"
#include "debug.h"
#include "if1632.h"

extern unsigned long  IF1632_Saved32_ebp;
extern unsigned long  IF1632_Saved32_esp;

static WORD ThunkSelector = 0;

struct thunk_s
{
    int used;
    unsigned char thunk[8];
};


/**********************************************************************
 *					MakeProcInstance
 */
FARPROC MakeProcInstance( FARPROC func, WORD instance )
{
    char *thunks;
    struct thunk_s *tp;
    int i;

    if (!ThunkSelector)
    {
        HGLOBAL handle = GLOBAL_Alloc( GMEM_ZEROINIT, 0x10000, 0, TRUE, FALSE);
        ThunkSelector = GlobalHandleToSel(handle);
    }

    thunks = (char *)PTR_SEG_OFF_TO_LIN( ThunkSelector, 0 );
    tp = (struct thunk_s *) thunks;
    for (i = 0; i < 0x10000 / sizeof(*tp); i++, tp++)
	if (!tp->used)
	    break;
    
    if (tp->used) return (FARPROC)0;
    
    tp->thunk[0] = 0xb8;    /* movw instance, %ax */
    tp->thunk[1] = (unsigned char) instance;
    tp->thunk[2] = (unsigned char) (instance >> 8);
    tp->thunk[3] = 0xea;    /* ljmp func */
    *(LONG *)&tp->thunk[4] = (LONG)func;
    tp->used = 1;

    return (FARPROC)MAKELONG( (char *)tp->thunk - thunks, ThunkSelector );
}

/**********************************************************************
 *					FreeProcInstance (KERNEL.52)
 */
void FreeProcInstance(FARPROC func)
{
    struct thunk_s *tp;
    int i;
    
    tp = (struct thunk_s *) PTR_SEG_OFF_TO_LIN( ThunkSelector, 0 );
    for (i = 0; i < 0x10000 / sizeof(*tp); i++, tp++)
    {
	if ((void *) tp->thunk == (void *) func)
	{
	    tp->used = 0;
	    break;
	}
    }
}

/**********************************************************************
 *	    GetCodeHandle    (KERNEL.93)
 */
HANDLE GetCodeHandle( FARPROC proc )
{
    struct thunk_s *tp = (struct thunk_s *)proc;

    /* Return the code segment containing 'proc'. */
    /* Not sure if this is really correct (shouldn't matter that much). */
    printf( "STUB: GetCodeHandle(%p) returning %x\n",
            proc, tp->thunk[8] + (tp->thunk[9] << 8) );
    return tp->thunk[8] + (tp->thunk[9] << 8);
}


/**********************************************************************
 *					CallWindowProc    (USER.122)
 */
LONG CallWindowProc( WNDPROC func, HWND hwnd, WORD message,
		     WORD wParam, LONG lParam )
{
    SpyMessage(hwnd, message, wParam, lParam);
    return CallWndProc( (FARPROC)func, hwnd, message, wParam, lParam );
}


/* ------------------------------------------------------------------------ */
/*
 * The following functions realize the Catch/Throw functionality.
 * My thought is to use the setjmp, longjmp combination to do the
 * major part of this one. All I have to remember, in addition to
 * whatever the jmp_buf contains, is the contents of the 16-bit
 * sp, bp and ss. I do this by storing them in the structure passed
 * to me by the 16-bit program (including my own jmp_buf...). 
 * Hopefully there isn't any program that modifies the contents!
 * Bad thing: I have to save part of the stack, since this will 
 * get reused on the next call after my return, leaving it in an
 * undefined state.
 */
#define	STACK_DEPTH_16 	28

struct special_buffer {
	jmp_buf buffer;
	long 	regs [5];
	char 	stack_part [STACK_DEPTH_16];
} *sb;

int Catch (LPCATCHBUF cbuf)
{
	WORD retval;
	jmp_buf *tmp_jmp;

	sb = malloc (sizeof (struct special_buffer));
	
	sb -> regs [0] = IF1632_Saved16_sp & 0xffff;
	sb -> regs [1] = IF1632_Saved16_bp & 0xffff;
	sb -> regs [2] = IF1632_Saved16_ss & 0xffff;
	sb -> regs [3] = IF1632_Saved32_esp;
	sb -> regs [4] = IF1632_Saved32_ebp;
	memcpy (sb -> stack_part, CURRENT_STACK16, STACK_DEPTH_16);
	tmp_jmp = &sb -> buffer;
	*((struct special_buffer **)cbuf) = sb;
	
	if ((retval = setjmp (*tmp_jmp)))
	{
		IF1632_Saved16_sp = sb -> regs [0] & 0xffff;
		IF1632_Saved16_bp = sb -> regs [1] & 0xffff;
		IF1632_Saved16_ss = sb -> regs [2] & 0xffff;
		IF1632_Saved32_esp = sb -> regs [3];
		IF1632_Saved32_ebp = sb -> regs [4];

		memcpy (CURRENT_STACK16, sb -> stack_part, STACK_DEPTH_16);
		dprintf_catch (stddeb, "Been thrown here: %d, retval = %d\n", 
			(int) sb, (int) retval);
		free ((void *) sb);
		return (retval);
	} else {
		dprintf_catch (stddeb, "Will somtime get thrown here: %d\n", 
			(int) sb);
		return (retval);
	}
}

void Throw (LPCATCHBUF cbuf, int val)
{
	sb = *((struct special_buffer **)cbuf);
	dprintf_catch (stddeb, "Throwing to: %d\n", (int) sb);
	longjmp (sb -> buffer, val);
}
#endif /* !WINELIB */
