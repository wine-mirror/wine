#ifndef WINELIB
/*
static char RCSId[] = "$Id: wine.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "windows.h"
#include "callback.h"
#include "wine.h"
#include <setjmp.h>
#include "ldt.h"
#include "stackframe.h"
#include "dlls.h"
#include "stddebug.h"
#include "debug.h"
#include "if1632.h"

extern unsigned short IF1632_Saved32_ss;
extern unsigned long  IF1632_Saved32_ebp;
extern unsigned long  IF1632_Saved32_esp;

static WORD ThunkSelector = 0;

struct thunk_s
{
    int used;
    unsigned char thunk[8];
};

#ifdef __ELF__
#define FIRST_SELECTOR 2
#define IS_16_BIT_ADDRESS(addr)  \
    (((unsigned int)(addr) < 0x8000000) || ((unsigned int)(addr) >= 0x8400000))
#else
#define FIRST_SELECTOR	8
#define IS_16_BIT_ADDRESS(addr)  \
     ((unsigned int)(addr) >= (((FIRST_SELECTOR << __AHSHIFT) | 7) << 16))
#endif

/**********************************************************************
 *					PushOn16
 */
static void
PushOn16(int size, unsigned int value)
{
    char *p = PTR_SEG_OFF_TO_LIN( IF1632_Saved16_ss, IF1632_Saved16_sp );
    if (size)
    {
	unsigned long *lp = (unsigned long *) p - 1;
	
	*lp = value;
	IF1632_Saved16_sp -= 4;
    }
    else
    {
	unsigned short *sp = (unsigned short *) p - 1;
	
	*sp = value;
	IF1632_Saved16_sp -= 2;
    }
}


/**********************************************************************
 *					CallBack16
 */
int
CallBack16(void *func, int n_args, ...)
{
    va_list ap;
    int i;
    int arg_type, arg_value;
    WORD ds = CURRENT_DS;

    va_start(ap, n_args);

    for (i = 0; i < n_args; i++)
    {
	arg_type = va_arg(ap, int);
	arg_value = va_arg(ap, int);
	PushOn16(arg_type, arg_value);
    }

    va_end(ap);

    return CallTo16((unsigned int) func, ds );
}

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
        ldt_entry entry;

          /* Allocate a segment for thunks */
        ThunkSelector = AllocSelector( 0 );
        entry.base           = (unsigned long) malloc( 0x10000 );
        entry.limit          = 0xffff;
        entry.seg_32bit      = 0;
        entry.limit_in_pages = 0;
        entry.type           = SEGMENT_CODE;
        entry.read_only      = 0;
        memset( (char *)entry.base, 0, 0x10000 );
        LDT_SetEntry( SELECTOR_TO_ENTRY(ThunkSelector), &entry );
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

    if (HIWORD((LONG)func) == WINE_CODE_SELECTOR) 
    {
	static struct dll_table_entry_s *user_tab = NULL;
	void *address = (void *) ((LONG) func & 0xffff);

	if (user_tab == NULL)
	    user_tab = FindDLLTable("USER");

	/* DefWindowProc */
	if (((LONG)user_tab[107].address &0xffff) == (LONG) address)
	    return DefWindowProc(hwnd, message, wParam, lParam);
	
	/* DefDlgProc */
	else if (((LONG)user_tab[308].address &0xffff) == (LONG)address)
	    return DefDlgProc(hwnd, message, wParam, lParam);
	
	/* DefMDIChildProc */
	else if (((LONG)user_tab[447].address &0xffff) == (LONG)address)
	    return DefMDIChildProc(hwnd, message, wParam, lParam);
	
	/* default */
	else
	{
	    fprintf(stderr, "wine: Unknown wine callback %08x\n", 
	    	(unsigned int) func);
	    exit(1);
	}
    }
    else if (IS_16_BIT_ADDRESS(func))
    {	
        WORD ds = CURRENT_DS;
	dprintf_callback(stddeb, "CallWindowProc // 16bit func=%08x ds=%04x!\n", 
		(unsigned int) func, ds );
	PushOn16( CALLBACK_SIZE_WORD, hwnd );
	PushOn16( CALLBACK_SIZE_WORD, message );
	PushOn16( CALLBACK_SIZE_WORD, wParam );
	PushOn16( CALLBACK_SIZE_LONG, lParam );
	return CallTo16((unsigned int) func, ds );
    }
    else
    {
	dprintf_callback(stddeb, "CallWindowProc // 32bit func=%08X !\n",
		(unsigned int) func);
	return (*func)(hwnd, message, wParam, lParam);
    }
}

/**********************************************************************
 *					CallLineDDAProc
 */
void CallLineDDAProc(FARPROC func, short xPos, short yPos, long lParam)
{
    WORD ds = CURRENT_DS;
    if (IS_16_BIT_ADDRESS(func))
    {
	PushOn16( CALLBACK_SIZE_WORD, xPos );
	PushOn16( CALLBACK_SIZE_WORD, yPos );
	PushOn16( CALLBACK_SIZE_LONG, lParam );
	CallTo16((unsigned int) func, ds );
    }
    else
    {
	(*func)(xPos, yPos, lParam);
    }
}

/**********************************************************************
 *					CallHookProc
 */
DWORD CallHookProc( HOOKPROC func, short code, WPARAM wParam, LPARAM lParam )
{
    WORD ds = CURRENT_DS;
    if (IS_16_BIT_ADDRESS(func))
    {
	PushOn16( CALLBACK_SIZE_WORD, code );
	PushOn16( CALLBACK_SIZE_WORD, wParam );
	PushOn16( CALLBACK_SIZE_LONG, lParam );
	return CallTo16((unsigned int) func, ds );
    }
    else
    {
	return (*func)( code, wParam, lParam );
    }
}

/**********************************************************************
 *					CallGrayStringProc
 */
BOOL CallGrayStringProc(FARPROC func, HDC hdc, LPARAM lParam, INT cch )
{
    WORD ds = CURRENT_DS;
    if (IS_16_BIT_ADDRESS(func))
    {
	PushOn16( CALLBACK_SIZE_WORD, hdc );
	PushOn16( CALLBACK_SIZE_LONG, lParam );
	PushOn16( CALLBACK_SIZE_WORD, cch );
	return CallTo16((unsigned int) func, ds );
    }
    else
    {
	return (*func)( hdc, lParam, cch );
    }
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
	long 	regs [6];
	char 	stack_part [STACK_DEPTH_16];
} *sb;

int Catch (LPCATCHBUF cbuf)
{
	WORD retval;
	jmp_buf *tmp_jmp;
	char *stack16 =  (char *) (((unsigned int)IF1632_Saved16_ss << 16) +
                                   IF1632_Saved16_sp);

	sb = malloc (sizeof (struct special_buffer));
	
	sb -> regs [0] = IF1632_Saved16_sp & 0xffff;
	sb -> regs [1] = IF1632_Saved16_bp & 0xffff;
	sb -> regs [2] = IF1632_Saved16_ss & 0xffff;
	sb -> regs [3] = IF1632_Saved32_esp;
	sb -> regs [4] = IF1632_Saved32_ebp;
	sb -> regs [5] = IF1632_Saved32_ss & 0xffff;
	memcpy (sb -> stack_part, stack16, STACK_DEPTH_16);
	tmp_jmp = &sb -> buffer;
	*((struct special_buffer **)cbuf) = sb;
	
	if ((retval = setjmp (*tmp_jmp)))
	{
		IF1632_Saved16_sp = sb -> regs [0] & 0xffff;
		IF1632_Saved16_bp = sb -> regs [1] & 0xffff;
		IF1632_Saved16_ss = sb -> regs [2] & 0xffff;
		IF1632_Saved32_esp = sb -> regs [3];
		IF1632_Saved32_ebp = sb -> regs [4];
		IF1632_Saved32_ss = sb -> regs [5] & 0xffff;
		stack16 = (char *) (((unsigned int)IF1632_Saved16_ss << 16) +
                                    IF1632_Saved16_sp);

		memcpy (stack16, sb -> stack_part, STACK_DEPTH_16);
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
